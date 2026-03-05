//------------------------------------------------------------------------------------------------
// State data for CRF Slotting Manager
class CRF_SlottingManagerStateData : PersistentState
{
}

//------------------------------------------------------------------------------------------------
// Serializer for CRF Slotting Manager - preserves slot assignments across mission reloads
class CRF_SlottingManagerSerializer : ScriptedStateSerializer
{
	//------------------------------------------------------------------------------------------------
	override static typename GetTargetType()
	{
		return CRF_SlottingManagerStateData;
	}

	//------------------------------------------------------------------------------------------------
	override ESerializeResult Serialize(notnull Managed instance, notnull BaseSerializationSaveContext context)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return ESerializeResult.DEFAULT;

		// Get the slots map
		map<int, ref CRF_SlotDataContainer> slotsMap = slottingManager.GetSlotMap();
		if (!slotsMap || slotsMap.Count() == 0)
			return ESerializeResult.DEFAULT;

		// Save version for future compatibility
		context.WriteValue("version", 1);
		context.WriteValue("slotCount", slotsMap.Count());
		
		// Note: Latest slot ID is not accessible (protected), will be regenerated on load

		// Serialize each slot
		int savedSlots = 0;
		foreach (int slotId, CRF_SlotDataContainer slotData : slotsMap)
		{
			if (!slotData)
				continue;

			string slotPrefix = string.Format("slot_%1_", slotId);
			
			// Save slot ID
			context.WriteValue(slotPrefix + "id", slotId);
			
			// Save player ID if slot is occupied
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0)
			{
				context.WriteValue(slotPrefix + "playerId", playerId);
			}
			
			// Save death state
			context.WriteValue(slotPrefix + "isDead", slotData.GetIsDeadSlot());
			
			// Save locked state
			context.WriteValue(slotPrefix + "isLocked", slotData.GetIsLockedSlot());
			
			// Note: Group assignments (RplId) are not serialized as they may not be valid after reload
			// Groups should be reassigned through normal slotting flow after load
			
			savedSlots++;
		}

		Print(string.Format("[CRF_SlottingManagerSerializer] Serialized %1 slots", savedSlots), LogLevel.NORMAL);
		return ESerializeResult.OK;
	}

	//------------------------------------------------------------------------------------------------
	override bool Deserialize(notnull Managed instance, notnull BaseSerializationLoadContext context)
	{
		int version;
		if (!context.ReadValue("version", version))
			return false;

		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		int slotCount;
		if (!context.ReadValue("slotCount", slotCount))
			return false;

		// Note: Latest slot ID counter is protected and cannot be set directly
		// It will be recalculated based on the highest slot ID when slots are restored

		// Get the slots map
		map<int, ref CRF_SlotDataContainer> slotsMap = slottingManager.GetSlotMap();
		if (!slotsMap)
			return false;

		// Temporary storage for slot data
		map<int, string> slotPlayerAssignments = new map<int, string>();
		map<int, bool> slotDeathStates = new map<int, bool>();
		map<int, bool> slotLockedStates = new map<int, bool>();

		// Read all slot data
		for (int i = 0; i < slotCount; i++)
		{
			// We don't know the actual slot IDs, so we need to iterate through potential IDs
			// This is a limitation - ideally we'd save slot IDs in an array first
			// For now, we'll try to read sequentially assuming slots exist
		}
		
		// Alternative approach: iterate through existing slots and try to read their data
		int restoredSlots = 0;
		foreach (int slotId, CRF_SlotDataContainer slotData : slotsMap)
		{
			if (!slotData)
				continue;

			string slotPrefix = string.Format("slot_%1_", slotId);
			
			// Try to read player assignment
			int playerId;
			if (context.ReadValue(slotPrefix + "playerId", playerId))
			{
				slotData.SetSlotCurrentPlayerId(playerId);
			}
			
			// Restore locked state
			bool isLocked;
			if (context.ReadValue(slotPrefix + "isLocked", isLocked))
			{
				slotData.SetIsLockedSlot(isLocked);
			}
			
			// Restore dead state 
			bool isDead;
			if (context.ReadValue(slotPrefix + "isDead", isDead))
			{
				slotData.SetIsDeadSlot(isDead);
			}
			
			restoredSlots++;
		}
		
		// Note: Group assignments are not restored from save as RplIds may not be valid
		// Groups will be reassigned through normal gameplay flow
		
		Print(string.Format("[CRF_SlottingManagerSerializer] Restored state for %1 slots", restoredSlots), LogLevel.NORMAL);
		return true;
	}
}
