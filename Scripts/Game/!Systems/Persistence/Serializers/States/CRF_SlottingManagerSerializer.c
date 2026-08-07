//------------------------------------------------------------------------------------------------
// State data for the CRF slotting manager.
//------------------------------------------------------------------------------------------------
class COA_SlottingManagerStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Preserves the slot table across a crash resume: who was in which slot, their role and group, and
// how many respawns they had left.
//
// KEYED ON IDENTITY GUID, NOT PLAYER ID
// The previous version of this serializer saved COA_SlotData.GetSlotCurrentPlayerId(). A player ID
// is a per-session runtime handle - the Nth player to connect gets N. It is not stable across a
// server restart, so restoring it would have handed slots to whoever happened to connect in the same
// order, which on a 60-player event means near-guaranteed mis-slotting.
// Vanilla has the same problem and solves it the same way: SCR_SpawnLogic keys persistent player
// data on SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId), which is stable for a given account.
// We store that GUID and resolve it back to whatever player ID that account holds after the restart.
//------------------------------------------------------------------------------------------------
class COA_SlottingManagerSerializer : ScriptedStateSerializer
{
	protected const int SERIALIZER_VERSION = 2;

	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return COA_SlottingManagerStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull SaveContext context)
	{
		// See the note in COA_GamemodeSerializer: this line appearing during a save is the proof
		// that scripted states are registered and running.
		Print("[COA_SlottingManagerSerializer] Serialize() called - slot table IS being written.", LogLevel.NORMAL);

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
		{
			Print("[COA_SlottingManagerSerializer] COA_SlottingManager unavailable - writing nothing.", LogLevel.WARNING);
			return ESerializeResult.DEFAULT;
		}

		map<int, ref COA_SlotData> slotsMap = slottingManager.GetSlotMap();
		if (!slotsMap || slotsMap.IsEmpty())
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", SERIALIZER_VERSION);

		// Write the slot IDs explicitly. The old version wrote only a count and then, on load, had no
		// way to know which IDs those were - its read loop was a stub with a comment admitting as
		// much. Storing the ID list makes the load side deterministic.
		array<int> slotIds = {};
		foreach (int slotId, COA_SlotData slotData : slotsMap)
		{
			if (slotData)
				slotIds.Insert(slotId);
		}

		context.WriteValue("slotIds", slotIds);

		PlayerManager playerManager = GetGame().GetPlayerManager();

		foreach (int slotId : slotIds)
		{
			COA_SlotData slotData = slotsMap.Get(slotId);
			if (!slotData)
				continue;

			string p = string.Format("slot_%1_", slotId);

			// Occupancy, stored as the account GUID rather than the session player ID.
			// Empty string means the slot was unoccupied.
			string occupantGuid = string.Empty;
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0 && playerManager && playerManager.IsPlayerConnected(playerId))
				occupantGuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);

			context.WriteValue(p + "occupantGuid", occupantGuid);
			context.WriteValue(p + "isDead", slotData.GetIsDeadSlot());
			context.WriteValue(p + "isLocked", slotData.GetIsLockedSlot());
			context.WriteValue(p + "respawnsRemaining", slotData.GetSlotRespawnsRemaining());
			context.WriteValue(p + "role", slotData.GetSlotRole());
			context.WriteValue(p + "factionEnum", slotData.GetSlotFactionEnum());
			context.WriteValue(p + "respawnPoolType", slotData.GetRespawnPoolType());

			// Group is stored by its stable group ID, not the RplId. RplIds are assigned per session
			// and are meaningless after a restart; group IDs come from the mission's group setup and
			// survive. Resolved back to a live group on load.
			context.WriteValue(p + "groupId", ResolveGroupId(slotData));
		}

		Print(string.Format("[COA_SlottingManagerSerializer] Saved %1 slots.", slotIds.Count()), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull LoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		// A version 1 save predates GUID-based occupancy. Its player IDs cannot be trusted, so the
		// slot table is left at mission defaults rather than restored wrongly.
		if (version < 2)
		{
			Print("[COA_SlottingManagerSerializer] Save predates GUID-based slotting - slots will not be restored. Players will need to re-slot.", LogLevel.WARNING);
			return true;
		}

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		map<int, ref COA_SlotData> slotsMap = slottingManager.GetSlotMap();
		if (!slotsMap)
			return false;

		array<int> slotIds = {};
		if (!context.ReadValue("slotIds", slotIds))
			return false;

		int restored = 0;

		foreach (int slotId : slotIds)
		{
			COA_SlotData slotData = slotsMap.Get(slotId);
			if (!slotData)
				continue;	// mission changed since the save - skip rather than fabricate a slot

			string p = string.Format("slot_%1_", slotId);

			bool isLocked;
			if (context.ReadValue(p + "isLocked", isLocked))
				slotData.SetIsLockedSlot(isLocked);

			bool isDead;
			if (context.ReadValue(p + "isDead", isDead))
				slotData.SetIsDeadSlot(isDead);

			int respawnsRemaining;
			if (context.ReadValue(p + "respawnsRemaining", respawnsRemaining))
				slotData.SetSlotRespawnsRemaining(respawnsRemaining);

			// Declared as their enum types rather than int - the setters take enums, and relying on
			// implicit int->enum conversion is exactly the sort of thing that compiles on one engine
			// version and stops on the next.
			COA_EGearRole role;
			if (context.ReadValue(p + "role", role))
				slotData.SetSlotRole(role);

			COA_EFactions factionEnum;
			if (context.ReadValue(p + "factionEnum", factionEnum))
				slotData.SetSlotFactionEnum(factionEnum);

			COA_ERespawnPoolType respawnPoolType;
			if (context.ReadValue(p + "respawnPoolType", respawnPoolType))
				slotData.SetRespawnPoolType(respawnPoolType);

			RestoreGroup(slotData, context, p);
			RestoreOccupant(slotData, context, p);

			restored++;
		}

		Print(string.Format("[COA_SlottingManagerSerializer] Restored %1 slots.", restored), LogLevel.NORMAL);
		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Turn a slot's group RplId into a stable group ID for saving. -1 when the slot has no group.
	protected int ResolveGroupId(notnull COA_SlotData slotData)
	{
		RplId groupRplId = slotData.GetSlotCurrentGroup();
		if (!groupRplId.IsValid())
			return -1;

		RplComponent groupRpl = RplComponent.Cast(Replication.FindItem(groupRplId));
		if (!groupRpl)
			return -1;

		SCR_AIGroup group = SCR_AIGroup.Cast(groupRpl.GetEntity());
		if (!group)
			return -1;

		return group.GetGroupID();
	}

	//------------------------------------------------------------------------------------------------
	//! Map a saved group ID back onto whatever RplId that group holds in this session.
	protected void RestoreGroup(notnull COA_SlotData slotData, notnull LoadContext context, string prefix)
	{
		int groupId;
		if (!context.ReadValue(prefix + "groupId", groupId) || groupId < 0)
			return;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return;

		SCR_AIGroup group = groupsManager.FindGroup(groupId);
		if (!group)
			return;

		RplComponent groupRpl = RplComponent.Cast(group.FindComponent(RplComponent));
		if (groupRpl)
			slotData.SetSlotCurrentGroup(groupRpl.Id());
	}

	//------------------------------------------------------------------------------------------------
	//! Re-seat the saved occupant.
	//!
	//! The account is almost never connected at this point - on a crash resume the server comes back
	//! up before anyone reconnects - so the slot cannot simply be assigned to a player ID here.
	//!
	//! Instead this feeds COA_Gamemode's existing reconnect map (GUID -> slot ID), which already
	//! re-seats a returning player into the slot they held. From the framework's point of view a
	//! crash resume then looks exactly like everyone reconnecting at once, which is a path that is
	//! already exercised in every session.
	protected void RestoreOccupant(notnull COA_SlotData slotData, notnull LoadContext context, string prefix)
	{
		string occupantGuid;
		if (!context.ReadValue(prefix + "occupantGuid", occupantGuid) || occupantGuid.IsEmpty())
			return;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return;

		gamemode.RestoreReconnectSlot(occupantGuid, slotData.GetSlotId());
	}
}
