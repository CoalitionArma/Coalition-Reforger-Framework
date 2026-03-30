// CRF_PropHunt_PlayerRplToOwnerManager.c
//
// Extends CRF_PlayerRplToOwnerManager with Prop Hunt client-side features:
//
//  Prop transformation — Props press [F / PerformAction] during grace to
//  disguise themselves as a nearby world object:
//    a. The server tells this client "F-key is enabled" via a Broadcast RPC
//       on CRF_PropHuntGamemode, which then calls ApplyPropTransformEnabled()
//       here directly (no per-player RPC needed from this modded class).
//    b. Client: sphere-query finds the nearest entity that has both a
//       valid prefab resource name AND a SCR_DamageManagerComponent.
//    c. Client sends a [RplRcver.Server] RPC to the server carrying
//       the chosen prefab's ResourceName.
//    d. Server: validates grace phase + Props team + not already
//       transformed, then hides the character entity (ClearFlags), locks
//       movement, spawns a prop entity at the character's position, and
//       registers the player↔entity pair in CRF_PropHuntGamemode.
//
// NOTE: Screen blackout and hunter health bar RPCs have been moved to
// CRF_PropHuntGamemode so they use guaranteed base-class [RplRpc] registration.

modded class CRF_PlayerRplToOwnerManager
{
	//------------------------------------------------------------
	// Prop transform state (client-local)
	//------------------------------------------------------------

	// True while [F] transform input is registered (grace phase only).
	protected bool m_bPropTransformEnabled = false;

	// True once this client has already sent a transform request this round.
	// Prevents double-pressing F.
	protected bool m_bPropTransformed = false;

	// Accumulates nearby entity results during the sphere query callback.
	protected ref array<IEntity> m_aNearbyEntities = {};

	//============================================================
	// PROP TRANSFORMATION — called locally by CRF_PropHuntGamemode
	// Broadcast RPC handler (no RPC here — avoids modded-class RPC
	// registration concerns).
	//============================================================

	//------------------------------------------------------------
	// ApplyPropTransformEnabled — runs on the local client only,
	// called from RpcDo_SetPropTransformEnabled inside
	// CRF_PropHuntGamemode. Registers or removes the F-key listener
	// and resets the one-transform-per-round guard.
	//------------------------------------------------------------
	void ApplyPropTransformEnabled(bool enable)
	{
		m_bPropTransformEnabled = enable;

		if (enable)
		{
			m_bPropTransformed = false; // fresh grace period — allow one transform
			GetGame().GetInputManager().AddActionListener("PerformAction", EActionTrigger.DOWN, ActionPerformTransform);
		}
		else
		{
			GetGame().GetInputManager().RemoveActionListener("PerformAction", EActionTrigger.DOWN, ActionPerformTransform);
		}
	}

	//============================================================
	// PROP TRANSFORMATION — client-side input handling
	//============================================================

	//------------------------------------------------------------
	// ActionPerformTransform — fires on the owning client when
	// the player presses [F / PerformAction] during grace phase.
	// Sphere-queries for nearby valid world-prop entities and opens
	// the CRF_PropHuntTransformMenu selection UI.
	//------------------------------------------------------------
	protected void ActionPerformTransform(float value, EActionTrigger reason)
	{
		// Guard: already transformed or input no longer enabled.
		if (m_bPropTransformed || !m_bPropTransformEnabled)
			return;

		// Don't open the menu if it's already showing.
		if (GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.CRF_PropHuntTransformMenu))
			return;

		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		if (!character)
			return;

		// Collect nearby valid prop entities within 8 m.
		m_aNearbyEntities.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(character.GetOrigin(), 8.0, OnQueryEntitySphere);

		if (m_aNearbyEntities.IsEmpty())
		{
			SCR_PopUpNotification notify = SCR_PopUpNotification.GetInstance();
			if (notify)
				notify.PopupMsg("No valid props nearby! Move closer to a damageable world object.", 3.0, "Prop Hunt");
			return;
		}

		// Open the selection menu. The menu calls ConfirmPropTransform() when the
		// player picks an entry, or simply closes on cancel.
		CRF_PropHuntTransformMenu.Open(m_aNearbyEntities);
	}

	//------------------------------------------------------------
	// ConfirmPropTransform — called by CRF_PropHuntTransformMenu
	// after the player selects an entry. Marks the player as
	// transformed (blocks re-open) and fires the server RPC.
	//------------------------------------------------------------
	void ConfirmPropTransform(ResourceName prefab)
	{
		if (m_bPropTransformed || !m_bPropTransformEnabled)
			return;

		if (!prefab)
			return;

		// Lock out further transforms this grace phase.
		m_bPropTransformed = true;

		#ifdef WORKBENCH
		RpcDo_RequestTransform(prefab);
		#else
		Rpc(RpcDo_RequestTransform, prefab);
		#endif
	}

	//------------------------------------------------------------
	// OnQueryEntitySphere — sphere-query accumulator callback.
	// Accepts only entities that:
	//   • have a non-empty prefab resource name (so the server
	//     can re-spawn them)
	//   • have SCR_DamageManagerComponent (so our damage hook
	//     can intercept a hit on the spawned clone)
	//   • are NOT characters (skip other players / AI)
	//------------------------------------------------------------
	protected bool OnQueryEntitySphere(IEntity entity)
	{
		if (!entity)
			return true;

		// Must have prefab data with a valid resource name.
		EntityPrefabData pd = entity.GetPrefabData();
		if (!pd)
			return true;

		ResourceName prefab = pd.GetPrefabName();
		if (!prefab)
			return true;

		// Skip characters — props should be world objects only.
		if (entity.FindComponent(SCR_CharacterControllerComponent))
			return true;

		// Must be damageable so the damage hook can detect kills.
		if (!entity.FindComponent(SCR_DamageManagerComponent))
			return true;

		m_aNearbyEntities.Insert(entity);
		return true; // continue query
	}

	//============================================================
	// PROP TRANSFORMATION — server-side RPC handler
	//============================================================

	//------------------------------------------------------------
	// RpcDo_RequestTransform — runs on the server (Authority).
	// Validates the request, hides the character entity, spawns a
	// prop clone, and registers the player↔entity pair so the
	// damage hook can map hits back to the correct player.
	//------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_RequestTransform(ResourceName prefab)
	{
		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (!propHunt || !propHunt.IsGracePhaseActive())
			return;

		// Resolve which player sent this request.
		PlayerController pc = PlayerController.Cast(GetOwner());
		if (!pc)
			return;

		int playerId = pc.GetPlayerId();
		if (playerId <= 0)
			return;

		// Validate: must be a living Prop team player who has not yet transformed.
		if (!propHunt.IsValidPropPlayer(playerId))
			return;

		if (propHunt.IsPlayerTransformed(playerId))
			return;

		// Basic prefab sanity check.
		if (!prefab)
			return;

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		// Capture the character's full world transform (position + orientation)
		// so the spawned prop appears exactly where the player is standing.
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		character.GetWorldTransform(spawnParams.Transform);

		// Hide the character — clear both VISIBLE (rendering) and TRACEABLE
		// (bullet hit-detection) flags recursively to cover all child meshes.
		character.ClearFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);

		// Freeze the invisible character so the player cannot ghost around.
		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(
			character.FindComponent(SCR_CharacterControllerComponent)
		);
		if (charCtrl)
			charCtrl.SetDisableMovementControls(true);

		// Spawn the prop entity at the character's former position.
		IEntity propEnt = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!propEnt)
		{
			// Spawn failed — undo visibility/movement changes so the player
			// is not permanently stuck.
			character.SetFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
			if (charCtrl)
				charCtrl.SetDisableMovementControls(false);
			return;
		}

		// Register the player↔entity pair in the game mode.
		propHunt.SetPlayerTransformed(playerId, propEnt);

		// Notify the player.
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.SendHint("You are DISGUISED! Stay still — the hunt begins soon.", playerId);
	}

}
