modded class COA_GamemodeManager
{
	//! Slot ID each player was last auto-assigned a CSI color team for.
	//! The role-based color is only a DEFAULT applied when a player takes a slot — players (and their
	//! SL) can override it from CSI's player settings menu, so it must not be re-derived every time
	//! AssignPlayerToGroup runs. That happens far more often than just on first spawn: re-opening the
	//! slotting menu and clicking "Game" re-runs the whole InitilizePlayer chain, as does every death
	//! and respawn, and each of those used to silently reset a deliberately chosen color back to the
	//! role default (white for any role DetermineCSIColorTeam doesn't color).
	protected ref map<int, int> m_mCSIColorTeamAssignedSlot = new map<int, int>();

	override protected void AssignPlayerToGroup(int playerId)
	{
		super.AssignPlayerToGroup(playerId);
		AssignCSIColorTeam(playerId);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		// Player IDs are recycled, so a stale entry would suppress the default color for whoever
		// takes this ID next.
		m_mCSIColorTeamAssignedSlot.Remove(playerId);
	}

	protected void AssignCSIColorTeam(int playerId, int retryCount = 0)
	{
		CSI_RplBroadcastManager csiBroadcastManager = CSI_RplBroadcastManager.GetInstance();
		if (!csiBroadcastManager)
			return;

		COA_SlotData slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return;

		// Apply the role default once per slot. Re-initializing into the same slot must leave any
		// manually chosen color alone; changing slots re-derives it for the new role.
		int slotId = slotData.GetSlotId();
		int assignedSlotId;
		if (m_mCSIColorTeamAssignedSlot.Find(playerId, assignedSlotId) && assignedSlotId == slotId)
			return;

		// CSI registers player data asynchronously after connection; retry until it is ready
		CSI_PlayerDataManager playerDataManager = CSI_PlayerDataManager.GetInstance();
		if (!playerDataManager || !playerDataManager.GetPlayerData(playerId))
		{
			if (retryCount < 20)
				GetGame().GetCallqueue().CallLater(AssignCSIColorTeam, 500, false, playerId, retryCount + 1);
			return;
		}

		m_mCSIColorTeamAssignedSlot.Set(playerId, slotId);

		CSI_EColorTeam colorTeam = DetermineCSIColorTeam(slotData);
		csiBroadcastManager.UpdatePlayerColorTeam(playerId, colorTeam);
	}

	//! Only squad-level roles receive a color; all other roles stay white (NONE).
	//! AR/AAR and the first team lead in a group are red; all other squad-level roles are green.
	//! Static so other systems (e.g. the lobby's slot list icons) can derive the same color from a
	//! slot without needing a COA_GamemodeManager instance - this is the single source of truth for
	//! the role-to-color mapping, kept here so it only exists in CRF.
	static CSI_EColorTeam DetermineCSIColorTeam(COA_SlotData slotData)
	{
		COA_EGearRole role = slotData.GetSlotRole();

		switch (role)
		{
			case COA_EGearRole.AUTOMATIC_RIFLEMAN:
			case COA_EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN:
				return CSI_EColorTeam.RED;

			case COA_EGearRole.TEAM_LEAD:
				return GetCSITeamLeadColor(slotData);

			case COA_EGearRole.GRENADIER:
			case COA_EGearRole.RIFLEMAN:
			case COA_EGearRole.RIFLEMAN_ANTITANK:
			case COA_EGearRole.RIFLEMAN_DEMO:
				return CSI_EColorTeam.GREEN;
		}

		// Leadership, specialties, vehicles, and all other roles stay white
		return CSI_EColorTeam.NONE;
	}

	//! Returns RED if this is the first TEAM_LEAD slot in the group (lowest slot ID),
	//! GREEN if a prior slot in the same group already holds a TEAM_LEAD role.
	static CSI_EColorTeam GetCSITeamLeadColor(COA_SlotData slotData)
	{
		RplId groupRplId = slotData.GetSlotCurrentGroup();
		if (groupRplId == RplId.Invalid())
			return CSI_EColorTeam.GREEN;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();

		// Slot IDs are sorted ascending by GetAllSlotIDsForGroup
		array<int> groupSlotIds = slottingManager.GetAllSlotIDsForGroup(groupRplId);
		int mySlotId = slotData.GetSlotId();

		foreach (int slotId : groupSlotIds)
		{
			// Reached our own slot first — we are the first team lead
			if (slotId == mySlotId)
				return CSI_EColorTeam.RED;

			// A prior slot already has the team lead role — we are the second
			COA_SlotData otherSlot = slottingManager.GetSlotData(slotId);
			if (otherSlot && otherSlot.GetSlotRole() == COA_EGearRole.TEAM_LEAD)
				return CSI_EColorTeam.GREEN;
		}

		return CSI_EColorTeam.GREEN;
	}

	//! Applies this slot's CSI fireteam color to its listbox row icon. Called after the row widget
	//! exists (from COA_ListboxComponent.AddItemSpecSlot/AddItemSlot - see the modded override in
	//! CRF_CSI_SlotIconColor.c), since SetRoleColor needs the actual widget, not just the slot data.
	static void ApplySlotIconColor(COA_ListboxComponent listbox, int index, int slotId)
	{
		COA_SlotData slotData = COA_SlottingManager.GetInstance().GetSlotData(slotId);
		if (!slotData)
			return;

		COA_ListBoxElementComponent comp = listbox.GetCRFElementComponent(index);
		if (!comp)
			return;

		CSI_EColorTeam colorTeam = DetermineCSIColorTeam(slotData);
		comp.SetRoleIconColor(CSI_UIHelper.ConvertColorTeamToColor(colorTeam));
	}
}
