class CRF_PlayerKeybindManagerClass : ScriptComponentClass {}

class CRF_PlayerKeybindManager : ScriptComponent
{		
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		// Register input action handlers
		GetGame().GetInputManager().AddActionListener("CRF_ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
		GetGame().GetInputManager().AddActionListener("CRF_AdminForceReady", EActionTrigger.DOWN, AdminForceReady);
		GetGame().GetInputManager().AddActionListener("CRF_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
	}
	
	protected void UpdateHUDVisible()
	{
		CRF_PlayerControllerManager playerControllerManager = CRF_PlayerControllerManager.GetInstance();
		
		playerControllerManager.m_bHUDVisible = !playerControllerManager.m_bHUDVisible
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Opens the slotting menu for player assignment
	 * @param value - Input value (1.0 for pressed)
	 * @param reason - Trigger reason
	 */
	protected void OpenSlottingMenu(float value = 0.0, EActionTrigger reason = 0)
	{
		if (value != 1)
			return;
		
		CRF_PlayerMenuManager.GetInstance().OpenSlottingMenu()
	}	

	//------------------------------------------------------------------------------------------------
	/**
	 * Toggles ready state for player's side/faction
	 * Only faction leaders can toggle ready state
	 */
	protected void ToggleSideReady()
	{
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupManager)
			return;

		SCR_AIGroup playersGroup = groupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		if (!playersGroup)
			return;

		string playerName = GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId());
		if (!playerName || playerName == "")
			return;

		// Only group leaders can toggle ready state
		if (playersGroup.IsPlayerLeader(SCR_PlayerController.GetLocalPlayerId()))
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		
			if (faction.GetFactionKey() == "")
				return;
			
			CRF_PlayerRplToAuthorityManager.GetInstance().ToggleSideReady(faction.GetFactionKey(), playerName, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Admin command to force ready state for all sides
	 */
	protected void AdminForceReady()
	{
		if (!SCR_Global.IsAdmin())
			return;
		
		CRF_PlayerRplToAuthorityManager.GetInstance().ToggleSideReady("", 
			GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId()), 
			true);
	}

	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerKeybindManager m_sInstance;
	void CRF_PlayerKeybindManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_PlayerKeybindManager GetInstance()
	{
		return m_sInstance;
	}
}