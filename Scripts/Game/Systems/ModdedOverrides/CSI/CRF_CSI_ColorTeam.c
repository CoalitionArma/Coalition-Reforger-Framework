modded class CRF_GamemodeManager
{
	override protected void AssignPlayerToGroup(int playerId)
	{
		super.AssignPlayerToGroup(playerId);
		AssignCSIColorTeam(playerId);
	}

	protected void AssignCSIColorTeam(int playerId, int retryCount = 0)
	{
		CSI_RplBroadcastManager csiBroadcastManager = CSI_RplBroadcastManager.GetInstance();
		if (!csiBroadcastManager)
			return;

		CRF_SlotData slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return;

		// CSI registers player data asynchronously after connection; retry until it is ready
		CSI_PlayerDataManager playerDataManager = CSI_PlayerDataManager.GetInstance();
		if (!playerDataManager || !playerDataManager.GetPlayerData(playerId))
		{
			if (retryCount < 20)
				GetGame().GetCallqueue().CallLater(AssignCSIColorTeam, 500, false, playerId, retryCount + 1);
			return;
		}

		CSI_EColorTeam colorTeam = DetermineCSIColorTeam(slotData);
		csiBroadcastManager.UpdatePlayerColorTeam(playerId, colorTeam);
	}

	//! Only squad-level roles receive a color; all other roles stay white (NONE).
	//! AR/AAR and the first team lead in a group are red; all other squad-level roles are green.
	protected CSI_EColorTeam DetermineCSIColorTeam(CRF_SlotData slotData)
	{
		CRF_EGearRole role = slotData.GetSlotRole();

		switch (role)
		{
			case CRF_EGearRole.AUTOMATIC_RIFLEMAN:
			case CRF_EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN:
				return CSI_EColorTeam.RED;

			case CRF_EGearRole.TEAM_LEAD:
				return GetCSITeamLeadColor(slotData);

			case CRF_EGearRole.GRENADIER:
			case CRF_EGearRole.RIFLEMAN:
			case CRF_EGearRole.RIFLEMAN_ANTITANK:
			case CRF_EGearRole.RIFLEMAN_DEMO:
				return CSI_EColorTeam.GREEN;
		}

		// Leadership, specialties, vehicles, and all other roles stay white
		return CSI_EColorTeam.NONE;
	}

	//! Returns RED if this is the first TEAM_LEAD slot in the group (lowest slot ID),
	//! GREEN if a prior slot in the same group already holds a TEAM_LEAD role.
	protected CSI_EColorTeam GetCSITeamLeadColor(CRF_SlotData slotData)
	{
		RplId groupRplId = slotData.GetSlotCurrentGroup();
		if (groupRplId == RplId.Invalid())
			return CSI_EColorTeam.GREEN;

		// Slot IDs are sorted ascending by GetAllSlotIDsForGroup
		array<int> groupSlotIds = m_SlottingManager.GetAllSlotIDsForGroup(groupRplId);
		int mySlotId = slotData.GetSlotId();

		foreach (int slotId : groupSlotIds)
		{
			// Reached our own slot first — we are the first team lead
			if (slotId == mySlotId)
				return CSI_EColorTeam.RED;

			// A prior slot already has the team lead role — we are the second
			CRF_SlotData otherSlot = m_SlottingManager.GetSlotData(slotId);
			if (otherSlot && otherSlot.GetSlotRole() == CRF_EGearRole.TEAM_LEAD)
				return CSI_EColorTeam.GREEN;
		}

		return CSI_EColorTeam.GREEN;
	}
}
