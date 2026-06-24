
modded class SCR_MapMarkerSquadLeader
{	
	//------------------------------------------------------------------------------------------------
	//! Check whether we are in a squad and if it should be visible on map
	override void UpdateLocalVisibility()
	{
		m_bDoLocalVisibilityUpdate = false;

		PlayerController pController = GetGame().GetPlayerController();
		if (!pController)
			return;

		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;

		/*
		if (m_Group && !m_Group.m_bBlueForceTrackerEnabled)
			SetLocalVisible(false);
			return;
		*/
		
		SCR_AIGroup localPlayerGroup = groupManager.GetPlayerGroup(pController.GetPlayerId());
		if (!localPlayerGroup)
		{
			SetLocalVisible(false);
			return;
		}
		
		Faction groupFaction = localPlayerGroup.GetFaction();
		if (!groupFaction)
		{
			SetLocalVisible(false);
			return;
		}
		
		if (CRF_Gamemode.GetInstance() && !CRF_Gamemode.GetInstance().IsSideBFTEnabled(groupFaction.GetFactionKey()))
		{	
			SetLocalVisible(false);
			return;
		}

		bool isLocalPlayerLeader = localPlayerGroup.IsPlayerLeader(pController.GetPlayerId());

		if (isLocalPlayerLeader && CanLeaderSeeOtherLeaders())
		{
			SetLocalVisible(true);
			return;
		}

		if (!isLocalPlayerLeader && (CanMemberSeeOtherLeaders() || localPlayerGroup.IsPlayerInGroup(m_PlayerID)))
		{
			SetLocalVisible(true);
			return;
		}

		SetLocalVisible(false);
	}
}
