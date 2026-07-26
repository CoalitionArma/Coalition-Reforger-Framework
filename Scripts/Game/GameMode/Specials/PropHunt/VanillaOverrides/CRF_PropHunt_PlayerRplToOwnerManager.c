// CRF_PropHunt_PlayerRplToOwnerManager.c
//
// Extends COA_PlayerRplToOwnerManager with Prop Hunt client-side features:
//
//  Prop transformation — Props press [T / CRF_PropHuntTransform] during grace to
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

modded class COA_PlayerRplToOwnerManager
{
	//------------------------------------------------------------
	// Prop transform state (client-local)
	//------------------------------------------------------------

	// True while [T] transform input is registered (grace phase only).
	protected bool m_bPropTransformEnabled = false;

	// True while [B] noise input is registered (hunt phase only).
	protected bool m_bPropNoiseEnabled = false;

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
			GetGame().GetInputManager().AddActionListener("CRF_PropHuntTransform", EActionTrigger.DOWN, ActionPerformTransform);
			Print("[PropHunt] ApplyPropTransformEnabled: T-key transform listener REGISTERED.", LogLevel.NORMAL);
		}
		else
		{
			GetGame().GetInputManager().RemoveActionListener("CRF_PropHuntTransform", EActionTrigger.DOWN, ActionPerformTransform);
			Print("[PropHunt] ApplyPropTransformEnabled: T-key transform listener REMOVED.", LogLevel.NORMAL);

			// Close the menu if it is still open (grace period ended while player had it open).
			// Without this the open menu keeps blocking character movement controls.
			CRF_PropHuntTransformMenu transMenu = CRF_PropHuntTransformMenu.GetInstance();
			if (transMenu)
				transMenu.Close();
		}
	}

	//============================================================
	// PROP TRANSFORMATION — client-side input handling
	//============================================================

	//------------------------------------------------------------
	// ActionPerformTransform — fires on the owning client when
	// the player presses [T / CRF_PropHuntTransform] during grace phase.
	// Sphere-queries for nearby valid world-prop entities and opens
	// the CRF_PropHuntTransformMenu selection UI.
	//------------------------------------------------------------
	protected void ActionPerformTransform(float value, EActionTrigger reason)
	{
		// Guard: input no longer enabled.
		if (!m_bPropTransformEnabled)
			return;

		// Faction guard — only Props team players may transform.
		// This acts as a belt-and-suspenders check in case m_bPropTransformEnabled
		// was left set due to a team-change, round-reset, or any other edge case.
		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (!propHunt)
			return;

		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionMgr)
		{
			int localId = SCR_PlayerController.GetLocalPlayerId();
			Faction localFaction = factionMgr.GetPlayerFaction(localId);
			if (!localFaction || localFaction.GetFactionKey() != propHunt.GetPropsTeamKey())
			{
				// Hunter (or unassigned) pressed F — disable the listener so it stops firing.
				ApplyPropTransformEnabled(false);
				return;
			}
		}

		Print(string.Format("[PropHunt] ActionPerformTransform fired. enabled=%1", m_bPropTransformEnabled), LogLevel.NORMAL);

		// If the menu is already open (player pressed F a second time), confirm the
		// first (nearest) entry as a keyboard fallback. This is the only way to select
		// in Workbench where cursor/InteractableDialogContext may not activate properly.
		if (CRF_PropHuntTransformMenu.IsOpen())
		{
			CRF_PropHuntTransformMenu inst = CRF_PropHuntTransformMenu.GetInstance();
			if (inst)
				inst.ConfirmFirst();
			return;
		}

		IEntity character = SCR_PlayerController.GetLocalControlledEntity();
		if (!character)
			return;

		// Collect nearby valid prop entities within 8 m.
		m_aNearbyEntities.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(character.GetOrigin(), 8.0, OnQueryEntitySphere);

		Print(string.Format("[PropHunt] ActionPerformTransform: found %1 nearby valid entities.", m_aNearbyEntities.Count()), LogLevel.NORMAL);

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
	// after the player selects an entry.
	//
	// On dedicated server the request is sent via RpcDo_RequestPropTransform,
	// which is declared in the NON-MODDED base class COA_PlayerRplToOwnerManager.
	// This is essential: RPCs added only to a modded class are NOT reliably
	// registered by Reforger's RPC table on dedicated servers.
	// The base-class RPC resolves playerId from GetOwner() and forwards to
	// CRF_PropHuntGamemode.HandleTransformRequest().
	//
	// In Workbench there is no actual network layer, so we call
	// HandleTransformRequest() directly.
	//------------------------------------------------------------
	void ConfirmPropTransform(ResourceName prefab)
	{
		if (!m_bPropTransformEnabled)
			return;

		if (!prefab)
			return;

		#ifdef WORKBENCH
		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (propHunt)
			propHunt.HandleTransformRequest(SCR_PlayerController.GetLocalPlayerId(), prefab);
		#else
		Rpc(RpcDo_RequestPropTransform, prefab);
		#endif
	}

	//------------------------------------------------------------
	// OnQueryEntitySphere — sphere-query accumulator callback.
	// Accepts only entities that:
	//   • have a non-empty prefab resource name (so the server can re-spawn them)
	//   • have a physics body (rejects decals, light switches, wall fixtures, and
	//     any other purely visual/interactive entity with no collision geometry)
	//   • are NOT characters (skip other players / AI)
	//   • are NOT buildings (too large)
	//   • are NOT attached to a character (skip worn gear / held weapons)
	//   • have a local bounding box whose largest axis is >= 0.3 m
	//     (secondary size guard for very small physical objects)
	//
	// SCR_DamageManagerComponent is NOT required: kill detection relies on the
	// invisible character capsule (TRACEABLE), so purely decorative interior
	// props (furniture, shelves, etc.) are valid disguises even without a
	// damage component of their own.
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

		// Require a physics body — decals, light switches, and other purely
		// visual/interactive world objects have none.
		if (!entity.GetPhysics())
			return true;

		// Skip characters — props should be world objects only.
		if (entity.FindComponent(SCR_CharacterControllerComponent))
			return true;

		// Skip buildings — they are too large to be valid prop disguises.
		if (entity.FindComponent(SCR_DestructibleBuildingComponent))
			return true;

		// Skip anything attached to a character (worn gear, held weapons, etc.).
		// Walk up the parent chain; if any ancestor is a character, reject.
		IEntity parent = entity.GetParent();
		while (parent)
		{
			if (parent.FindComponent(SCR_CharacterControllerComponent))
				return true;
			parent = parent.GetParent();
		}

		// Secondary size guard: reject objects whose largest bounding-box axis
		// is under 0.3 m (catches any small physical objects not filtered above).
		vector mins, maxs;
		entity.GetBounds(mins, maxs);
		vector size = maxs - mins;
		float largest = Math.Max(size[0], Math.Max(size[1], size[2]));
		if (largest < 0.3)
			return true;

		m_aNearbyEntities.Insert(entity);
		return true; // continue query
	}

	//============================================================
	// PROP NOISE HINT — called locally by CRF_PropHuntGamemode
	// Broadcast RPC handler (same pattern as ApplyPropTransformEnabled).
	//============================================================

	//------------------------------------------------------------
	// ApplyPropNoiseEnabled — runs on the local client only,
	// called from RpcDo_SetPropNoiseEnabled inside
	// CRF_PropHuntGamemode. Registers or removes the B-key listener
	// for the prop noise hint action.
	//------------------------------------------------------------
	void ApplyPropNoiseEnabled(bool enable)
	{
		m_bPropNoiseEnabled = enable;

		if (enable)
		{
			GetGame().GetInputManager().AddActionListener("CRF_PropHuntNoise", EActionTrigger.DOWN, ActionPerformNoise);
			GetGame().GetInputManager().AddActionListener("CRF_PropHuntNextNoise", EActionTrigger.DOWN, ActionCycleNoise);
			Print("[PropHunt] ApplyPropNoiseEnabled: B/N-key noise listeners REGISTERED.", LogLevel.NORMAL);
		}
		else
		{
			GetGame().GetInputManager().RemoveActionListener("CRF_PropHuntNoise", EActionTrigger.DOWN, ActionPerformNoise);
			GetGame().GetInputManager().RemoveActionListener("CRF_PropHuntNextNoise", EActionTrigger.DOWN, ActionCycleNoise);
			Print("[PropHunt] ApplyPropNoiseEnabled: B/N-key noise listeners REMOVED.", LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------
	// ActionCycleNoise — fires on the owning client when the
	// player presses [N / CRF_PropHuntNextNoise] during the hunt phase.
	// Sends a server RPC which increments the selected noise index
	// and sends a hint back showing the new selection.
	//------------------------------------------------------------
	protected void ActionCycleNoise(float value, EActionTrigger reason)
	{
		if (!m_bPropNoiseEnabled)
			return;

		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (!propHunt)
			return;

		// Faction guard.
		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionMgr)
		{
			int localId = SCR_PlayerController.GetLocalPlayerId();
			Faction localFaction = factionMgr.GetPlayerFaction(localId);
			if (!localFaction || localFaction.GetFactionKey() != propHunt.GetPropsTeamKey())
			{
				ApplyPropNoiseEnabled(false);
				return;
			}
		}

		#ifdef WORKBENCH
		propHunt.HandleNoiseCycleRequest(SCR_PlayerController.GetLocalPlayerId());
		#else
		Rpc(RpcDo_RequestPropNextNoise);
		#endif
	}

	//------------------------------------------------------------
	// ActionPerformNoise — fires on the owning client when the
	// player presses [B / CRF_PropHuntNoise] during the hunt phase.
	// Sends a server RPC to validate and broadcast the noise to
	// all clients. The sound plays 3D-positionally from the prop's
	// hidden character entity, which is at the prop's world position.
	//------------------------------------------------------------
	protected void ActionPerformNoise(float value, EActionTrigger reason)
	{
		if (!m_bPropNoiseEnabled)
			return;

		// Faction guard — only Props team players may emit noise.
		CRF_PropHuntGamemode propHunt = CRF_PropHuntGamemode.GetInstance();
		if (!propHunt)
			return;

		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (factionMgr)
		{
			int localId = SCR_PlayerController.GetLocalPlayerId();
			Faction localFaction = factionMgr.GetPlayerFaction(localId);
			if (!localFaction || localFaction.GetFactionKey() != propHunt.GetPropsTeamKey())
			{
				ApplyPropNoiseEnabled(false);
				return;
			}
		}

		Print("[PropHunt] ActionPerformNoise fired.", LogLevel.NORMAL);

		#ifdef WORKBENCH
		if (propHunt)
			propHunt.HandleNoiseRequest(SCR_PlayerController.GetLocalPlayerId());
		#else
		Rpc(RpcDo_RequestPropNoise);
		#endif
	}

}
