//------------------------------------------------------------------------------------------------
//! Takes care of dynamic and static onscreen popups
modded class SCR_PopUpNotification : GenericEntity
{	
	//------------------------------------------------------------------------------------------------
	//! Initializes the pop-up notification UI components and event handlers
	//! This method is called during the entity initialization phase
	override protected void ProcessInit()
	{
		
		if (!CRF_Gamemode.GetInstance())
		{
			super.ProcessInit();
			return;
		};
		
		// Check if HUD manager exists
		if (!GetGame().GetHUDManager())
			return;

		// Get the player controller
		PlayerController pc = GetGame().GetPlayerController();

		// Exit if no player controller or controlled entity is available
		if (!pc || !pc.GetControlledEntity())
			return;

		// Create UI layout based on HUD visibility setting
		Widget root;
		if (CRF_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			// If HUD is visible, place notifications at top layer
			root = GetGame().GetHUDManager().CreateLayout(LAYOUT_NAME, EHudLayers.ALWAYS_TOP, 0);
		}
		else
		{
			// If HUD is not visible, place notifications at lower layer
			root = GetGame().GetHUDManager().CreateLayout(LAYOUT_NAME, EHudLayers.LOW, 0);
		}
		
		// Exit if UI layout creation failed
		if (!root)
			return;

		// Remove initialization from call queue as it's being processed now
		GetGame().GetCallqueue().Remove(ProcessInit);

		// Get and initialize popup UI components
		m_wPopupMsg = RichTextWidget.Cast(root.FindAnyWidget("Popup"));
		m_wPopupMsgSmall = RichTextWidget.Cast(root.FindAnyWidget("PopupSmall"));
		m_wStatusProgress = ProgressBarWidget.Cast(root.FindAnyWidget("Progress"));
		
		// Store default alpha value for fade effects
		m_fDefaultAlpha = m_wPopupMsg.GetColor().A();
		
		// Hide all UI elements initially
		m_wPopupMsg.SetVisible(false);
		m_wPopupMsgSmall.SetVisible(false);
		m_wStatusProgress.SetVisible(false);

		// Set default horizontal position after a short delay
		//GetGame().GetCallqueue().CallLater(SetDefaultHorizontalPosition, 500);
		SetDefaultHorizontalPosition();

		// Register for controlled entity changes
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
		{
			playerController.m_OnControlledEntityChanged.Insert(RefreshInventoryInvoker);
		}

		// Process any pending notifications
		RefreshQueue();

		// Adjust z-order to ensure popups are not visible when map is open
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity)
		{
			MapConfiguration config = mapEntity.GetMapConfig();
			if (config)
			{
				Widget mapWidget = config.RootWidgetRef;
				if (mapWidget)
				{
					root.SetZOrder(mapWidget.GetZOrder() - 1);
				}
			}
		}
		
		// Initialize interface settings from user configuration
		SCR_HUDManagerComponent hudManager = GetGame().GetHUDManager();
		if (!hudManager)
			return;
		
		BaseContainer interfaceSettings = GetGame().GetGameUserSettings().GetModule(hudManager.GetInterfaceSettingsClass());
		if (!interfaceSettings)
			return;
		
		// Get current enabled state from settings
		bool state;
		interfaceSettings.Get(INTERFACE_SETTINGS_NAME, state);
		m_bIsEnabledInSettings = state;
		
		// Register for settings change events
		GetGame().OnUserSettingsChangedInvoker().Insert(OnSettingsChanged);	
	}
}