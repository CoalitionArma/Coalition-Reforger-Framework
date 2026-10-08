//! Adds "Forward deploy" (the /fd chat command) to the player radial menu
modded class COA_UtilityRadialActions
{
	protected ref SCR_SelectionMenuEntry m_ForwardDeploy;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		COA_PlayerRadialMenuManager radialMenu = COA_PlayerRadialMenuManager.Cast(owner.FindComponent(COA_PlayerRadialMenuManager));
		if (!radialMenu)
			return;

		m_ForwardDeploy = new SCR_SelectionMenuEntry();
		m_ForwardDeploy.SetId("support_forward_deploy");
		m_ForwardDeploy.SetName("Forward deploy");
		radialMenu.RegisterEntry("Support", m_ForwardDeploy);
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdateAvailability()
	{
		super.UpdateAvailability();

		if (!m_ForwardDeploy)
			return;

		// Same conditions as /fd (CRF_COA_PLayerChatCommandManager.ReopenForwardDeployMenu)
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		IEntity controlled = SCR_PlayerController.GetLocalControlledEntity();
		m_ForwardDeploy.Enable(gamemode && safestartManager && !gamemode.m_bLockUnusedSlots && !safestartManager.GetSafestartStatus()
			&& controlled && !COA_EntityHelper.IsSpectator(controlled));
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPerformAction(SCR_SelectionMenuEntry entry)
	{
		if (entry != m_ForwardDeploy)
		{
			super.OnPerformAction(entry);
			return;
		}

		COA_PlayerChatCommandManager chatCommands = COA_PlayerChatCommandManager.GetInstance();
		if (chatCommands)
			chatCommands.ReopenForwardDeployMenu(null, string.Empty);
	}
}
