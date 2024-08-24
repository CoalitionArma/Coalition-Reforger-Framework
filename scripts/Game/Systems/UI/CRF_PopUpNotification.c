//------------------------------------------------------------------------------------------------
//! Takes care of dynamic and static onscreen popups
modded class SCR_PopUpNotification : GenericEntity
{	
	override protected void ProcessInit()
	{
		if (!GetGame().GetHUDManager())
			return;

		PlayerController pc = GetGame().GetPlayerController();

		if (!pc || !pc.GetControlledEntity())
			return;

		Widget root;
		if(CRF_SafestartGameModeComponent.GetInstance().m_bHUDVisible)
			root = GetGame().GetHUDManager().CreateLayout(LAYOUT_NAME, EHudLayers.ALWAYS_TOP, 0);
		else
			root = GetGame().GetHUDManager().CreateLayout(LAYOUT_NAME, EHudLayers.LOW, 0);
		
		if (!root)
			return;

		// Init can be safely processed
		GetGame().GetCallqueue().Remove(ProcessInit);

		// Initialize popups UI
		m_wPopupMsg = RichTextWidget.Cast(root.FindAnyWidget("Popup"));
		m_wPopupMsgSmall = RichTextWidget.Cast(root.FindAnyWidget("PopupSmall"));
		m_wStatusProgress = ProgressBarWidget.Cast(root.FindAnyWidget("Progress"));
		m_fDefaultAlpha = m_wPopupMsg.GetColor().A();
		m_wPopupMsg.SetVisible(false);
		m_wPopupMsgSmall.SetVisible(false);
		m_wStatusProgress.SetVisible(false);

		GetGame().GetCallqueue().CallLater(SetDefaultHorizontalPosition, 500);

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());

		if (playerController)
			playerController.m_OnControlledEntityChanged.Insert(RefreshInventoryInvoker);

		RefreshQueue();

		// Popups should not be visible in map
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();

		if (mapEntity)
		{
			MapConfiguration config = mapEntity.GetMapConfig();

			if (!config)
				return;

			Widget mapWidget = config.RootWidgetRef;

			if (mapWidget)
				root.SetZOrder(mapWidget.GetZOrder() - 1);
		}
		
		// Initialize Interface settings
		SCR_HUDManagerComponent hudManager = GetGame().GetHUDManager();
		if (!hudManager)
			return;
		
		BaseContainer interfaceSettings = GetGame().GetGameUserSettings().GetModule(hudManager.GetInterfaceSettingsClass());
		if (!interfaceSettings)
			return;
		
		bool state;
		interfaceSettings.Get(INTERFACE_SETTINGS_NAME, state);
		m_bIsEnabledInSettings = state;
		
		GetGame().OnUserSettingsChangedInvoker().Insert(OnSettingsChanged);	
	}
}