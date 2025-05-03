modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_AARMenu
}

/**
 * After Action Report Menu UI class
 * Responsible for displaying mission summary, player statistics, and slot management
 */
class CRF_AARMenuUI: ChimeraMenuBase
{
	//----------------------------------------
	// UI Widget References
	//----------------------------------------
	protected Widget m_wRoot;
	protected Widget m_wFactions;
	protected Widget m_wMissionDescription;
	protected Widget m_wRoleFrame;
	protected Widget m_wLeftFaction;
	protected ButtonWidget m_wBackButton;
	
	//----------------------------------------
	// Core Components
	//----------------------------------------
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_MapEntity m_MapEntity;
	protected CRF_Gamemode m_Gamemode;
	protected CRF_MenuManager m_MenuManager;
	protected SCR_ListBoxComponent m_cPlayerListBoxComponent;
	protected CRF_ListboxComponent m_cSlotListBoxComponent;
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent;
	
	//----------------------------------------
	// Faction & Slot Data
	//----------------------------------------
	protected Faction m_fSelectedFaction;
	protected ref array<ref CRF_MissionDescriptor> m_aActiveDescriptors = {};
	
	// Total slots per faction
	protected int m_iBluforSlots = 0;
	protected int m_iOpforSlots = 0;
	protected int m_iIndforSlots = 0;
	protected int m_iCivSlots = 0;
	
	// Currently alive slots per faction
	protected int m_iAliveBluforSlots = 0;
	protected int m_iAliveOpforSlots = 0;
	protected int m_iAliveIndforSlots = 0;
	protected int m_iAliveCivSlots = 0;
	
	//----------------------------------------
	// Menu Lifecycle Methods
	//----------------------------------------
	
	/**
	 * Initialize the menu when it's first created
	 */
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	/**
	 * Called when the menu is opened
	 * Initializes all UI elements and sets up data
	 */
	override void OnMenuOpen()
	{	
		// Exit if this is a dedicated server (menu is client-side only)
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		
		// Initialize map if available
		if (m_MapEntity)
		{	
			GetGame().GetCallqueue().CallLater(OpenMap, 0); 
		}
		
		// Set up input handling
		SetupInputHandlers();
		
		// Initialize UI components
		InitializeUIComponents();
		
		// Set up mission text and weather information
		SetupMissionInfo();
		
		// Initialize faction flags
		SetupFactionFlags();
		
		// Initialize faction selection colors
		SetupFactionColors();
		
		// Initialize slots
		InitSlots();
		
		// Select initial faction based on availability
		SelectInitialFaction();
		
		// Update the UI to reflect current slot status
		UpdateSlots();
		
		// Register for slot updates
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Insert(UpdateSlots);
		
		// Initialize back button
		m_wBackButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("BackButton"));
		m_wBackButton.SetOpacity(0);
		
		// Initialize mission description list
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("DescriptionList")).FindHandler(SCR_ListBoxComponent));
		
		// Initialize description panel
		DescriptionInit();
	}
	
	/**
	 * Initialize UI components and store references
	 */
	protected void InitializeUIComponents()
	{
		m_wRoot = GetRootWidget();
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		
		// Find main UI panels
		m_wFactions = m_wRoot.FindAnyWidget("Factions");
		m_wMissionDescription = m_wRoot.FindAnyWidget("DescriptionList");
		m_wRoleFrame = m_wRoot.FindAnyWidget("RoleList");
		m_wLeftFaction = m_wRoot.FindAnyWidget("LeftFaction");
		
		// Initialize chat panel
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		// Initialize list components
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayerList")).FindHandler(SCR_ListBoxComponent));
		m_cSlotListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleList")).FindHandler(CRF_ListboxComponent));
	}
	
	/**
	 * Set up input handlers for menu controls
	 */
	protected void SetupInputHandlers()
	{
		// Add action listeners
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	/**
	 * Set up mission information displays
	 */
	protected void SetupMissionInfo()
	{
		TextWidget missionText = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionText"));
		
		// Set mission name
		if(GetGame().GetMissionName())
			missionText.SetText(GetGame().GetMissionName());
		else
			missionText.SetText("Unknown Mission");
		
		// Add author information if available
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if(header)
			missionText.SetText(missionText.GetText() + " | By " + header.m_sAuthor);
		else
			missionText.SetText(missionText.GetText() + " | By " + "Unknown");
		
		// Set weather text
		string currentStateName = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetCurrentWeatherState().GetStateName();
		TextWidget.Cast(m_wRoot.FindAnyWidget("WeatherText")).SetText("Weather: " + currentStateName);
	}
	
	/**
	 * Set up faction flags in the UI
	 */
	protected void SetupFactionFlags()
	{
		// Set flag images for each faction
		SetFactionFlag("FlagBlufor", "BLUFOR");
		SetFactionFlag("FlagOpfor", "OPFOR");
		SetFactionFlag("FlagIndfor", "INDFOR");
		SetFactionFlag("FlagCiv", "CIV");
	}
	
	/**
	 * Helper method to set a faction flag
	 */
	protected void SetFactionFlag(string widgetName, string factionKey)
	{
		ImageWidget flagWidget = ImageWidget.Cast(m_wRoot.FindAnyWidget(widgetName));
		flagWidget.LoadImageTexture(1, SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey)).GetFactionFlag());
		flagWidget.SetImage(1);
	}
	
	/**
	 * Set up faction selection colors
	 */
	protected void SetupFactionColors()
	{
		m_wRoot.FindAnyWidget("BluforBGSelect").SetColor(Color.FromRGBA(34, 196, 244, 33));
		m_wRoot.FindAnyWidget("OpforBGSelect").SetColor(Color.FromRGBA(238, 49, 47, 33));
		m_wRoot.FindAnyWidget("IndforBGSelect").SetColor(Color.FromRGBA(0, 177, 79, 33));
		m_wRoot.FindAnyWidget("CivBGSelect").SetColor(Color.FromRGBA(168, 110, 207, 33));
	}
	
	/**
	 * Select initial faction based on availability
	 */
	protected void SelectInitialFaction()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		if(slottingManager.IsFactionValid("BLUFOR"))
			SelectFactionBlufor();
		else if(slottingManager.IsFactionValid("OPFOR"))
			SelectFactionOpfor();
		else if(slottingManager.IsFactionValid("INDFOR"))
			SelectFactionIndfor();
		else if(slottingManager.IsFactionValid("CIV"))
			SelectFactionCiv();
	}
	
	/**
	 * Clean up when menu is closed
	 */
	override void OnMenuClose()
	{
		// Unregister from slot updates
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Remove(UpdateSlots);
		
		// Remove input handlers
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	/**
	 * Update the menu each frame
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		// Activate map context if map exists
		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
		
		// Update time display
		UpdateTimeDisplay();
		
		// Update player count
		UpdatePlayerCount();
		
		// Update player list
		UpdatePlayerList(tDelta);
		
		// Update faction availability
		UpdateFactionAvailability();
		
		// Handle sidebar animation
		AnimateSidebar(tDelta);
	}
	
	/**
	 * Update the time display
	 */
	protected void UpdateTimeDisplay()
	{
		TimeContainer timeContainer = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetTime();
		int hours = timeContainer.m_iHours;
		int minutes = timeContainer.m_iMinutes;
		
		string minuteString;
		string hourString;
		
		if(minutes < 10)
			minuteString = "0" + minutes.ToString();
		else
			minuteString = minutes.ToString();
		
		if(hours < 10)
			hourString = "0" + hours.ToString();
		else
			hourString = hours.ToString();
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText")).SetText("Time: " + hourString + ":" + minuteString);
	}
	
	/**
	 * Update the player count display
	 */
	protected void UpdatePlayerCount()
	{
		TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersText")).SetText("Players: " + GetGame().GetPlayerManager().GetPlayerCount());
	}
	
	/**
	 * Update the player list with current players
	 */
	protected void UpdatePlayerList(float tDelta)
	{
		ref array<int> playerIds = {};
		
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		m_cPlayerListBoxComponent.Clear();
		
		foreach(int playerId : playerIds)
		{
			if(!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
				
			int index = m_cPlayerListBoxComponent.AddItem(GetGame().GetPlayerManager().GetPlayerName(playerId), null, "{51F58D728FBCAD99}UI/Listbox/PlayerListboxElementNoIcon.layout");
			SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(index);
			
			// Set color based on player status
			if(SCR_Global.IsAdmin(playerId))
				comp.SetColor(Color.Red);
			else if(CRF_GamemodeManager.GetInstance().IsModerator(playerId))
				comp.SetColor(Color.Yellow);
			else if(m_MenuManager.m_aPlayersTalking.Contains(playerId))
				comp.SetColor(Color.FromRGBA(255, 183, 0, 255));
		}
		
		// Update chat if available
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
	}
	
	/**
	 * Update the faction availability status
	 */
	protected void UpdateFactionAvailability()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		// Update BLUFOR status
		UpdateFactionStatus("BLUFOR", "SlotsBlufor", "BluforFactionLockBG", "BluforFactionLock", "ButtonBlufor", 
			m_iAliveBluforSlots, m_iBluforSlots, slottingManager.IsFactionValid("BLUFOR"));
			
		// Update OPFOR status
		UpdateFactionStatus("OPFOR", "SlotsOpfor", "OpforFactionLockBG", "OpforFactionLock", "ButtonOpfor",
			m_iAliveOpforSlots, m_iOpforSlots, slottingManager.IsFactionValid("OPFOR"));
			
		// Update INDFOR status
		UpdateFactionStatus("INDFOR", "SlotsIndfor", "IndforFactionLockBG", "IndforFactionLock", "ButtonIndfor",
			m_iAliveIndforSlots, m_iIndforSlots, slottingManager.IsFactionValid("INDFOR"));
			
		// Update CIV status
		UpdateFactionStatus("CIV", "SlotsCiv", "CivFactionLockBG", "CivFactionLock", "ButtonCiv",
			m_iAliveCivSlots, m_iCivSlots, slottingManager.IsFactionValid("CIV"));
	}
	
	/**
	 * Helper method to update a faction's status display
	 */
	protected void UpdateFactionStatus(string factionKey, string slotsWidget, string lockBgWidget, string lockWidget, string buttonWidget,
		int aliveSlots, int totalSlots, bool isValid)
	{
		if(isValid)
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget(slotsWidget)).SetText(aliveSlots.ToString() + "/" + totalSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget(lockBgWidget)).SetColor(Color.FromRGBA(63,63,63,0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget(lockWidget)).SetColor(Color.FromRGBA(255,255,255,0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget(buttonWidget)).SetEnabled(true);
		}
	}
	
	/**
	 * Animate the sidebar based on cursor position
	 */
	protected void AnimateSidebar(float tDelta)
	{
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		float leftFactionX = FrameSlot.GetPosX(m_wLeftFaction);
		
		if(cursorWidget)
		{
			Widget parentWidget = cursorWidget.GetParent();
			
			// Check if cursor is over a menu element
			if(parentWidget == m_wFactions || parentWidget == m_wMissionDescription || 
			   parentWidget == m_wRoleFrame || parentWidget == m_wLeftFaction || 
			   cursorWidget.FindHandler(CRF_ListBoxElementComponent))
			{
				// Slide in
				leftFactionX += tDelta * 2400.0;
				if (leftFactionX > -14)
					leftFactionX = -14;
			}
			else
			{
				// Slide out
				leftFactionX -= tDelta * 2400.0;
				if (leftFactionX < -671)
					leftFactionX = -671;
			}
		}
		else
		{
			// Slide out when no cursor
			leftFactionX -= tDelta * 2400.0;
			if (leftFactionX < -671)
				leftFactionX = -671;
		}
		
		FrameSlot.SetPosX(m_wLeftFaction, leftFactionX);
	}
	
	//----------------------------------------
	// Description Panel Methods
	//----------------------------------------
	
	/**
	 * Initialize the mission description panel
	 */
	void DescriptionInit()
	{
		// Disable scrolling initially
		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		scrollLayout.SetEnabled(false);
		
		// Reset back button
		m_wBackButton.SetOpacity(0);
		SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
		backButton.m_OnClicked.Clear();
		
		// Clear description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		missionDescriptionText.SetText("");
		
		// Set up description list
		m_cMissionDescriptionListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();
		
		// Populate description list
		foreach(ref CRF_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			m_cMissionDescriptionListBoxComponent.AddItem(description.m_sTitle, null, "{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout");
			m_aActiveDescriptors.Insert(description);
		}
		
		// Register selection handler
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Insert(DescriptionSelected);
	}
	
	/**
	 * Handler for when a description is selected
	 */
	void DescriptionSelected()
	{
		// Enable scrolling
		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		scrollLayout.SetEnabled(true);
		
		// Get selected description
		int index = m_cMissionDescriptionListBoxComponent.GetSelectedItem();
		string description = m_aActiveDescriptors.Get(index).m_sTextData;
		
		// Show back button
		m_wBackButton.SetOpacity(1);
		SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
		backButton.m_OnClicked.Insert(DescriptionInit);
		
		// Clear list and handler
		m_cMissionDescriptionListBoxComponent.Clear();
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();
		
		// Set description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		missionDescriptionText.SetText(description);
	}
	
	//----------------------------------------
	// Slot Management Methods
	//----------------------------------------
	
	/**
	 * Initialize slot counts for each faction
	 */
	void InitSlots()
	{
		// Reset slot counts
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iAliveBluforSlots = 0;
		m_iAliveOpforSlots = 0;
		m_iAliveIndforSlots = 0;
		m_iAliveCivSlots = 0;
		
		// Get slot data
		array<ref CRF_SlotDataContainer> slotArray = CRF_SlottingManager.GetInstance().GetSlotArray();
		
		// Count slots by faction
		foreach (int slotId, CRF_SlotDataContainer slotData : slotArray)
		{
			if(slotData.GetIsLockedSlot() || slotData.GetSlotCurrentPlayerId() == 0)
				continue;
			
			switch(slotData.GetSlotFactionKey())
			{
				case "BLUFOR":
					m_iBluforSlots++;
					if(!slotData.GetIsDeadSlot())
						m_iAliveBluforSlots++;
					break;
					
				case "OPFOR":
					m_iOpforSlots++;
					if(!slotData.GetIsDeadSlot())
						m_iAliveOpforSlots++;
					break;
					
				case "INDFOR":
					m_iIndforSlots++;
					if(!slotData.GetIsDeadSlot())
						m_iAliveIndforSlots++;
					break;
					
				case "CIV":
					m_iCivSlots++;
					if(!slotData.GetIsDeadSlot())
						m_iAliveCivSlots++;
					break;
			}
		}
	}
	
	/**
	 * Update slot display for the currently selected faction
	 */
	void UpdateSlots()
	{
		// Reset slot counts
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iAliveBluforSlots = 0;
		m_iAliveOpforSlots = 0;
		m_iAliveIndforSlots = 0;
		m_iAliveCivSlots = 0;
		
		// Clear the slot list
		m_cSlotListBoxComponent.Clear();
		
		// Update faction color for borders
		PanelWidget.Cast(m_wRoot.FindAnyWidget("PlayerBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		PanelWidget.Cast(m_wRoot.FindAnyWidget("RoleBorder")).SetColor(m_fSelectedFaction.GetFactionColor());
		
		// Reinitialize slot counts
		InitSlots();
		
		// Get slot data and groups
		array<ref CRF_SlotDataContainer> slotArray = CRF_SlottingManager.GetInstance().GetSlotArray();
		array<SCR_AIGroup> factionGroups = CRF_SlottingManager.GetInstance().GetAllGroups(m_fSelectedFaction.GetFactionKey());
		
		if (factionGroups.IsEmpty())
			return;
		
		// Process each group
		foreach(SCR_AIGroup group : factionGroups)
		{	
			int playersInGroup = 0;
			
			// Skip private groups
			if(group.IsPrivate())
				continue;
			
			// Add group to list
			int groupIndex = m_cSlotListBoxComponent.AddItemGroup(null, group);
			
			// Set group colors and icon
			SetGroupVisuals(group, groupIndex);
			
			// Process each slot in the group
			foreach(int slotId, CRF_SlotDataContainer slotData : slotArray)
			{	
				// Skip slots not in this group or faction
				if (!IsSlotInGroupAndFaction(slotData, group))
					continue;
				
				// Add slot to list
				int slotIndex = m_cSlotListBoxComponent.AddItemSlot(null, slotId);
				
				// Set slot information
				SetSlotVisuals(slotIndex, slotData);
				
				playersInGroup++;
			}
			
			// Remove empty groups
			if(playersInGroup == 0)
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
		}
	}
	
	/**
	 * Check if a slot is in the specified group and faction
	 */
	protected bool IsSlotInGroupAndFaction(CRF_SlotDataContainer slotData, SCR_AIGroup group)
	{
		if (slotData.GetSlotCurrentGroup() != RplComponent.Cast(group.FindComponent(RplComponent)).Id() 
			|| slotData.GetIsLockedSlot() 
			|| slotData.GetSlotCurrentPlayerId() == 0 
			|| GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
			return false;
			
		return true;
	}
	
	/**
	 * Set visual properties for a group in the list
	 */
	protected void SetGroupVisuals(SCR_AIGroup group, int groupIndex)
	{
		// Set group colors
		m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupWidget().SetColor(group.GetFaction().GetFactionColor());
		m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupUnderline().SetColor(group.GetFaction().GetFactionColor());
		
		// Set group icon
		m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().Update(
			SCR_GroupIdentityComponent.Cast(group.FindComponent(SCR_GroupIdentityComponent)).GetMilitarySymbol()
		);
	}
	
	/**
	 * Set visual properties for a slot in the list
	 */
	protected void SetSlotVisuals(int slotIndex, CRF_SlotDataContainer slotData)
	{
		CRF_ListBoxElementComponent elementComponent = m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex);
		
		// Handle player status (alive/dead/disconnected)
		if(!slotData.GetIsDeadSlot())
		{
			if(GetGame().GetPlayerManager().IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
				elementComponent.SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId()));
			else
			{
				elementComponent.SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId()));
				elementComponent.GetDisconnectWidget().SetVisible(true);
			}
		}		
		else
		{
			if(GetGame().GetPlayerManager().IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
			{
				elementComponent.SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId()));
				elementComponent.GetDeathWidget().SetVisible(true);
			}
			else
			{
				elementComponent.SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId()));
				elementComponent.GetDisconnectWidget().SetVisible(true);
				elementComponent.GetDeathWidget().SetVisible(true);
			}
		}
		
		// Disable slot button
		elementComponent.GetSlotButton().SetEnabled(false);
	}
	
	//----------------------------------------
	// Faction Selection Methods
	//----------------------------------------
	
	/**
	 * Select BLUFOR faction
	 */
	void SelectFactionBlufor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("BLUFOR");
		
		// Update faction selection UI
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		
		// Set slot background color
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(34, 196, 244, 33));
		
		// Update slots display
		UpdateSlots();
	}
	
	/**
	 * Select OPFOR faction
	 */
	void SelectFactionOpfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("OPFOR");
		
		// Update faction selection UI
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		
		// Set slot background color
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(238, 49, 47, 33));
		
		// Update slots display
		UpdateSlots();
	}
	
	/**
	 * Select INDFOR faction
	 */
	void SelectFactionIndfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("INDFOR");
		
		// Update faction selection UI
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(1);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		
		// Set slot background color
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(0, 177, 79, 33));
		
		// Update slots display
		UpdateSlots();
	}
	
	/**
	 * Select CIV faction
	 */
	void SelectFactionCiv()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("CIV");
		
		// Update faction selection UI
		m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
		m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(1);
		
		// Set slot background color
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(Color.FromRGBA(168, 110, 207, 33));
		
		// Update slots display
		UpdateSlots();
	}
	
	//----------------------------------------
	// Map Handling Methods
	//----------------------------------------
	
	/**
	 * Initialize map opening sequence
	 * Uses multiple frame delays to ensure proper loading
	 */
	void OpenMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWrap, 0); // Need two frames for proper initialization
	}
	
	/**
	 * Second step in map opening sequence
	 */
	void OpenMapWrap()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChange, 0);
	}
	
	/**
	 * Third step in map opening sequence
	 */
	void OpenMapWrapZoomChange()
	{
		// Additional frame delay to ensure map is ready
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChangeWrap, 0);
	}
	
	/**
	 * Final step in map opening sequence
	 * Sets the initial zoom level
	 */
	void OpenMapWrapZoomChangeWrap()
	{
		m_MapEntity.ZoomOut();
	}
	
	//----------------------------------------
	// Voice and Chat Methods
	//----------------------------------------
	
	/**
	 * Start voice transmission
	 */
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	
	/**
	 * Get the radio transceiver for voice communication
	 */
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		ref array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		
		// Find radio in inventory
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if(item.FindComponent(BaseRadioComponent))
				radioEntity = item;
		}
		
		// Configure radio
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);
			
		return transceiver;
	}
	
	/**
	 * Stop voice transmission with delay
	 */
	void Action_VONOff()
	{
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	/**
	 * Delayed function to disable voice
	 */
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	/**
	 * Toggle chat panel
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Frame delay to ensure proper toggling
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
	}
	
	/**
	 * Open chat panel wrapper
	 */
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	/**
	 * Exit action - opens pause menu instead of exiting to prevent accidental exits
	 */
	void Action_Exit()
	{
		// Open pause menu rather than exiting directly
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0);
	}
	
	/**
	 * Open pause menu wrapper
	 */
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}