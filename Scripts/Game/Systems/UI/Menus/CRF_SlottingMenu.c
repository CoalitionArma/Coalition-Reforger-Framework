class CRF_SlottingMenu: ChimeraMenuBase
{
	//---------------------------------------------------------------------
	// UI Widgets
	//---------------------------------------------------------------------
	protected Widget m_wRoot;                   // Root widget for the entire menu
	protected ImageWidget m_wPreview;           // Phase indicator for preview phase
	protected ImageWidget m_wSlotting;          // Phase indicator for slotting phase
	protected ImageWidget m_wGame;              // Phase indicator for game phase
	protected ImageWidget m_wAAR;               // Phase indicator for After Action Report phase
	
	//---------------------------------------------------------------------
	// List Components
	//---------------------------------------------------------------------
	protected SCR_ListBoxComponent m_cPlayerListBoxComponent;         // Component for showing all players
	protected SCR_ListBoxComponent m_cUnslotPlayerListBoxComponent;   // Component for showing unslotted players
	protected CRF_ListboxComponent m_cSlotListBoxComponent;           // Component for showing available slots
	protected CRF_ListboxComponent m_cOrbatListBoxComponent;          // Component for showing organization structure
	
	//---------------------------------------------------------------------
	// Game Systems
	//---------------------------------------------------------------------
	protected CRF_Gamemode m_Gamemode;          		// Reference to the current gamemode
	protected CRF_MenuManager m_MenuManager;   	 		// Reference to the menu manager
	protected SCR_ChatPanel m_ChatPanel;        		// Reference to the chat panel
	protected SCR_PlayerController m_PlayerController;	// Reference to the player controller
	protected SCR_VONController m_VONController;		// Reference to the VON Controlle;
	
	//---------------------------------------------------------------------
	// UI Components
	//---------------------------------------------------------------------
	protected SCR_ButtonTextComponent m_wAdvanceButton;   // Button to advance gamemode phase
	protected SCR_ButtonTextComponent m_wPreviewButton;   // Button to return to preview
	
	//---------------------------------------------------------------------
	// Faction Slot Counts
	//---------------------------------------------------------------------
	protected int m_iBluforSlots = 0;           // Total number of BLUFOR slots
	protected int m_iOpforSlots = 0;            // Total number of OPFOR slots
	protected int m_iIndforSlots = 0;           // Total number of INDFOR slots
	protected int m_iCivSlots = 0;              // Total number of CIV slots
	
	protected int m_iTakenBluforSlots = 0;      // Number of taken BLUFOR slots
	protected int m_iTakenOpforSlots = 0;       // Number of taken OPFOR slots
	protected int m_iTakenIndforSlots = 0;      // Number of taken INDFOR slots
	protected int m_iTakenCivSlots = 0;         // Number of taken CIV slots

	//---------------------------------------------------------------------
	// Selection State
	//---------------------------------------------------------------------
	protected Faction m_fSelectedFaction;       // Currently selected faction
	protected int m_iSelectedplayerId = 0;      // Currently selected player ID
	protected int m_LocalSlottingState;         // Local copy of slotting state
	
	//---------------------------------------------------------------------
	// Faction Resources
	//---------------------------------------------------------------------
	ResourceName m_rBluforIcon;                 // BLUFOR faction icon resource
	ResourceName m_rOpforIcon;                  // OPFOR faction icon resource
	ResourceName m_rIndforIcon;                 // INDFOR faction icon resource
	ResourceName m_rCivIcon;                    // CIV faction icon resource
	
	//---------------------------------------------------------------------
	// Player Panel State
	//---------------------------------------------------------------------
	protected bool m_bShowingUnslotted = false;  // Whether the unslotted tab is active

	//---------------------------------------------------------------------
	// Controller Slot List Navigation
	//---------------------------------------------------------------------
	protected int m_iFocusedSlotIndex = -1;      // Index in m_cSlotListBoxComponent currently focus-highlighted, -1 if none
	protected Widget m_wHighlightedSlotWidget;   // PlayerName TextWidget of the currently focus-highlighted slot row
	protected ref Color m_cSlotListOriginalColor; // Captured original color of m_wHighlightedSlotWidget, to restore on move-away
	protected float m_fSlotListHoldTimer = 0;    // Time the current d-pad list-nav direction has been held
	protected int m_iSlotListHoldDirection = 0;  // -1/0/1 - which direction CRF_SlottingListNext/Prev is currently held in

	//---------------------------------------------------------------------
	// Consts
	//---------------------------------------------------------------------
	const string EMPTY_RESOURCE = "{2E717F4664C6E49D}UI/Textures/Nametags/Nametag-Filter-Icons/Player.edds";

	/**
	 * Called when the menu is opened
	 * Initializes UI elements and sets up event listeners
	 */
	override void OnMenuOpen()
	{	
		super.OnMenuOpen();

		// Don't open this menu on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}
		
		// Add input event listeners
		SetupInputListeners();
		
		// Initialize UI components
		InitializeUIComponents();
		
		// Initialize faction display
		InitializeFactionDisplay();
		
		// Setup ratio display
		SetupRatioDisplay();
		
		// Select default faction
		SelectDefaultFaction();

		// Initialize and update slots
		UpdateSlots();
		
		// Register for slot updates
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Insert(UpdateSlots);
		
		// Register for surgical per-slot player-ID updates (no full rebuild needed)
		CRF_SlottingManager.GetInstance().GetOnSlotChanged().Insert(UpdateSlotInPlace);

		// Fetch community tags + ranks and register for updates when they arrive
		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (tagMgr)
		{
			tagMgr.FetchPlayerInfo();
			tagMgr.GetOnPlayerInfoUpdated().Remove(RefreshTagsAndRanks);
			tagMgr.GetOnPlayerInfoUpdated().Insert(RefreshTagsAndRanks);
			tagMgr.GetOnPlayerRosterChanged().Remove(OnPlayerRosterChanged);
			tagMgr.GetOnPlayerRosterChanged().Insert(OnPlayerRosterChanged);
		}
		
		// Setup faction button event handlers
		SetupFactionButtons();

		// Give a widget default focus so controller D-pad/stick navigation has somewhere to start
		SetupDefaultFocus();
	}
	
	/**
	 * Sets up input event listeners for the menu
	 */
	protected void SetupInputListeners()
	{
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

		// Controller shoulder-button faction cycling (also bound to [ and ] on keyboard)
		GetGame().GetInputManager().AddActionListener("CRF_SlottingFactionNext", EActionTrigger.DOWN, Action_CycleFactionNext);
		GetGame().GetInputManager().AddActionListener("CRF_SlottingFactionPrev", EActionTrigger.DOWN, Action_CycleFactionPrev);

		// Controller Y button / keyboard Tab toggles the admin-only Players/Unslotted tab
		GetGame().GetInputManager().AddActionListener("CRF_SlottingToggleTab", EActionTrigger.DOWN, Action_ToggleUnslottedTab);

		// Controller D-pad / keyboard Up-Down navigate the slot/role list, since these
		// dynamically-created list items are not reachable via the engine's default focus navigation
		GetGame().GetInputManager().AddActionListener("CRF_SlottingListNext", EActionTrigger.DOWN, Action_SlotListNext);
		GetGame().GetInputManager().AddActionListener("CRF_SlottingListPrev", EActionTrigger.DOWN, Action_SlotListPrev);
	}

	/**
	 * Initializes all UI components
	 */
	protected void InitializeUIComponents()
	{
		// Find chat panel widget
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		// Store root widget
		m_wRoot = GetRootWidget();
		
		// Get gamemode and menu manager references
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		
		// Store local slotting state
		m_LocalSlottingState = m_Gamemode.m_SlottingState;
		
		m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		m_VONController = SCR_VONController.Cast(GetGame().GetPlayerController().FindComponent(SCR_VONController));
		
		// Setup mission info text
		SetupMissionInfo();
		
		// Setup weather text
		SetupWeatherInfo();
		
		// Setup phase indicators
		SetupPhaseIndicators();
		
		// Setup buttons
		SetupButtons();
		
		// Initialize list components
		InitializeListComponents();
	}
	
	/**
	 * Sets up mission information display
	 */
	protected void SetupMissionInfo()
	{
		TextWidget missionText = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionText"));
		
		if(GetGame().GetMissionName())
			missionText.SetText(GetGame().GetMissionName());
		else
			missionText.SetText("Unknown Mission");
		
		// Add author information if available
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if(missionHeader)
			missionText.SetText(missionText.GetText() + " | By " + missionHeader.m_sAuthor);
		else
			missionText.SetText(missionText.GetText() + " | By " + "Unknown");
	}
	
	/**
	 * Sets up weather information display
	 */
	protected void SetupWeatherInfo()
	{
		string currentStateName = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetCurrentWeatherState().GetStateName();
		TextWidget.Cast(m_wRoot.FindAnyWidget("WeatherText")).SetText("Weather: " + currentStateName);
	}
	
	/**
	 * Sets up phase indicator widgets
	 */
	protected void SetupPhaseIndicators()
	{
		// Get phase indicator widgets
		m_wPreview = ImageWidget.Cast(m_wRoot.FindAnyWidget("PreviewBorder"));
		m_wSlotting = ImageWidget.Cast(m_wRoot.FindAnyWidget("SlottingBorder"));
		m_wGame = ImageWidget.Cast(m_wRoot.FindAnyWidget("GameBorder"));
		m_wAAR = ImageWidget.Cast(m_wRoot.FindAnyWidget("AARBorder"));
		
		// Highlight the current phase
		int gameState = CRF_Gamemode.Cast(GetGame().GetGameMode()).m_GamemodeState; 
		switch(gameState)
		{
			case 0: {m_wPreview.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
			case 1: {m_wSlotting.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
			case 2: {m_wGame.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
			case 3: {m_wAAR.SetColor(Color.FromRGBA(122, 0, 0, 255)); break;}
		}
	}
	
	/**
	 * Sets up button widgets and attaches event handlers
	 */
	protected void SetupButtons()
	{
		ButtonWidget previewButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PreviewButton"));
		ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
		ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
		ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
		
		// Disable buttons by default
		aarButton.SetEnabled(false);
		advanceButton.SetEnabled(false);
		gameButton.SetEnabled(false);
		
		// Hide admin-only UI elements by default; show Unslotted tab for admins
		bool isAdmin = SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId());
		m_wRoot.FindAnyWidget("TabButtonUnslotted").SetVisible(isAdmin);
		m_wRoot.FindAnyWidget("SlottingPhases").SetOpacity(0);
		FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(0);
		
		// If in game state, enable game button
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			gameButton.SetEnabled(true);
		
		// Set up button click handlers
		SCR_ButtonTextComponent.Cast(gameButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(InitilizePlayer);
		SCR_ButtonTextComponent.Cast(previewButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(OpenSlottingMenu);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlotPhaseButton")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceSlottingPhase);
		SCR_ButtonTextComponent.Cast(advanceButton.FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(AdvanceMenu);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("TabButtonPlayers")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(ShowPlayersTab);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("TabButtonUnslotted")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(ShowUnslottedTab);
	}
	
	/**
	 * Initializes list components
	 */
	protected void InitializeListComponents()
	{
		m_cPlayerListBoxComponent = SCR_ListBoxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayerList")).FindHandler(SCR_ListBoxComponent));
		m_cOrbatListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("OrbatList")).FindHandler(CRF_ListboxComponent));
		m_cUnslotPlayerListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("UnslotPlayerList")).FindHandler(CRF_ListboxComponent));
		m_cSlotListBoxComponent = CRF_ListboxComponent.Cast(OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleList")).FindHandler(CRF_ListboxComponent));
	}
	
	/**
	 * Initializes faction display with faction icons
	 */
	protected void InitializeFactionDisplay()
	{
		// Set up each faction if valid
		SetupFactionUIDisplay("BLUFOR", m_rBluforIcon, "BluforFrame", "FlagBlufor", "BluforBGSelect", Color.FromRGBA(34, 196, 244, 33));
		SetupFactionUIDisplay("OPFOR", m_rOpforIcon, "OpforFrame", "FlagOpfor", "OpforBGSelect", Color.FromRGBA(238, 49, 47, 33));
		SetupFactionUIDisplay("INDFOR", m_rIndforIcon, "IndforFrame", "FlagIndfor", "IndforBGSelect", Color.FromRGBA(0, 177, 79, 33));
		SetupFactionUIDisplay("CIV", m_rCivIcon, "CivFrame", "FlagCiv", "CivBGSelect", Color.FromRGBA(168, 110, 207, 33));
	}
	
	/**
	 * Sets up UI display for a single faction
	 * @param factionKey Key identifying the faction
	 * @param iconResource Resource to store the faction icon
	 * @param frameWidget Name of the faction frame widget
	 * @param flagWidget Name of the faction flag widget
	 * @param bgSelectWidget Name of the faction background select widget
	 * @param bgColor Background color for the faction
	 */
	protected void SetupFactionUIDisplay(string factionKey, out ResourceName iconResource, string frameWidget, string flagWidget, string bgSelectWidget, Color bgColor)
	{
		// Only process if faction is valid
		if(!CRF_SlottingManager.GetInstance().IsFactionValid(factionKey))
			return;
		
		// Try to get faction icon from gearscript if available
		ResourceName gearScriptResource = CRF_Gamemode.GetInstance().GetGearScriptResource(factionKey);
		if(!gearScriptResource.IsEmpty())
		{
			CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
				BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
				
			if(gearConfig && !gearConfig.m_FactionIcon.IsEmpty())
				iconResource = gearConfig.m_FactionIcon;
		}
		
		// If no icon was set from gearscript, use default faction flag
		if(iconResource.IsEmpty())
			iconResource = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey)).GetFactionFlag();
		
		// Set up UI elements
		m_wRoot.FindAnyWidget(frameWidget).SetVisible(true);
		ImageWidget.Cast(m_wRoot.FindAnyWidget(flagWidget)).LoadImageTexture(0, iconResource);
		m_wRoot.FindAnyWidget(bgSelectWidget).SetColor(bgColor);
	}
	
	/**
	 * Sets up ratio display between factions
	 */
	protected void SetupRatioDisplay()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		bool validRatios = true;
		
		// Set up first faction ratio
		if (gamemode.m_iFactionOneRatio > 0 && !gamemode.m_sFactionOneKey.IsEmpty())
		{
			EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).SetText(gamemode.m_iFactionOneRatio.ToString());
			TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Text")).SetText(gamemode.m_sFactionOneKey);
		
			// Set appropriate color
			Color colorOne = GetFactionColor(gamemode.m_sFactionOneKey);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Image")).SetColor(colorOne);
		}
		else
		{
			validRatios = false;
		}
		
		// Set up second faction ratio
		if (gamemode.m_iFactionTwoRatio > 0 && !gamemode.m_sFactionTwoKey.IsEmpty())
		{
			EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).SetText(gamemode.m_iFactionTwoRatio.ToString());
			TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Text")).SetText(gamemode.m_sFactionTwoKey);
		
			// Set appropriate color
			Color colorTwo = GetFactionColor(gamemode.m_sFactionTwoKey);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Image")).SetColor(colorTwo);
		} else {
			validRatios = false;
		}

		// Hide ratio display if ratios are invalid
		if (!validRatios)
			HideRatioDisplay();
	}
	
	/**
	 * Gets a color for a faction based on its key
	 * @param factionKey Abbreviated faction key (BLU, OPF, IND, CIV)
	 * @return Color representation of the faction
	 */
	protected Color GetFactionColor(string factionKey)
	{
		switch(factionKey)
		{
			case "BLU": return Color.FromRGBA(0, 20, 255, 255);
			case "OPF": return Color.FromRGBA(188, 0, 0, 255);
			case "IND": return Color.FromRGBA(0, 145, 43, 255);
			case "CIV": return Color.FromRGBA(137, 0, 188, 255);
		}
		
		return Color.White;
	}
	
	/**
	 * Hides all ratio display elements when ratio info is invalid
	 */
	protected void HideRatioDisplay()
	{
		EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).SetVisible(false);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Image")).SetVisible(false);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1IntImage")).SetVisible(false);
		TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1Text")).SetVisible(false);
		EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).SetVisible(false);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Image")).SetVisible(false);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2IntImage")).SetVisible(false);
		TextWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2Text")).SetVisible(false);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("FinalImage")).SetVisible(false);
		TextWidget.Cast(m_wRoot.FindAnyWidget("Final")).SetVisible(false);
	}
	
	/**
	 * Selects the default faction based on availability
	 */
	protected void SelectDefaultFaction()
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
	 * Sets up faction selection button handlers
	 */
	protected void SetupFactionButtons()
	{
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionBlufor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionOpfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionIndfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionCiv);
	}

	/**
	 * Sets initial widget focus so controller D-pad/stick navigation has a starting point.
	 * Without this the engine's focus-based navigation has nothing focused to move from.
	 */
	protected void SetupDefaultFocus()
	{
		FocusSelectedFactionButton();
	}

	/**
	 * Cycles to the next valid faction (controller shoulder button / keyboard ])
	 */
	void Action_CycleFactionNext()
	{
		CycleFaction(1);
	}

	/**
	 * Cycles to the previous valid faction (controller shoulder button / keyboard [)
	 */
	void Action_CycleFactionPrev()
	{
		CycleFaction(-1);
	}

	/**
	 * Selects the next/previous valid faction relative to the currently selected one,
	 * wrapping around, and moves widget focus onto its button.
	 * @param direction - +1 for next, -1 for previous
	 */
	protected void CycleFaction(int direction)
	{
		array<string> factionKeys = GetValidFactionKeysOrdered();
		if (factionKeys.IsEmpty())
			return;

		int currentIndex = -1;
		if (m_fSelectedFaction)
			currentIndex = factionKeys.Find(m_fSelectedFaction.GetFactionKey());

		int nextIndex = (currentIndex + direction + factionKeys.Count()) % factionKeys.Count();

		switch (factionKeys[nextIndex])
		{
			case "BLUFOR": SelectFactionBlufor(); break;
			case "OPFOR": SelectFactionOpfor(); break;
			case "INDFOR": SelectFactionIndfor(); break;
			case "CIV": SelectFactionCiv(); break;
		}

		FocusSelectedFactionButton();
	}

	/**
	 * Returns the keys of all valid factions for this mission, in display order
	 */
	protected array<string> GetValidFactionKeysOrdered()
	{
		array<string> factionKeys = {};
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();

		if (slottingManager.IsFactionValid("BLUFOR"))
			factionKeys.Insert("BLUFOR");
		if (slottingManager.IsFactionValid("OPFOR"))
			factionKeys.Insert("OPFOR");
		if (slottingManager.IsFactionValid("INDFOR"))
			factionKeys.Insert("INDFOR");
		if (slottingManager.IsFactionValid("CIV"))
			factionKeys.Insert("CIV");

		return factionKeys;
	}

	/**
	 * Moves widget focus onto the button of the currently selected faction
	 */
	protected void FocusSelectedFactionButton()
	{
		if (!m_fSelectedFaction)
			return;

		Widget focusTarget;
		switch (m_fSelectedFaction.GetFactionKey())
		{
			case "BLUFOR": focusTarget = m_wRoot.FindAnyWidget("ButtonBlufor"); break;
			case "OPFOR": focusTarget = m_wRoot.FindAnyWidget("ButtonOpfor"); break;
			case "INDFOR": focusTarget = m_wRoot.FindAnyWidget("ButtonIndfor"); break;
			case "CIV": focusTarget = m_wRoot.FindAnyWidget("ButtonCiv"); break;
		}

		if (focusTarget)
			GetGame().GetWorkspace().SetFocusedWidget(focusTarget);
	}

	/**
	 * Controller D-pad down / keyboard Down handler - moves the slot list focus highlight
	 * forward to the next selectable slot row.
	 */
	void Action_SlotListNext()
	{
		MoveSlotListFocus(1);
	}

	/**
	 * Controller D-pad up / keyboard Up handler - moves the slot list focus highlight
	 * back to the previous selectable slot row.
	 */
	void Action_SlotListPrev()
	{
		MoveSlotListFocus(-1);
	}

	/**
	 * Steps the focus-highlighted slot list row in the given direction, skipping over
	 * group-header rows (which have no m_iSlotId), and wrapping around at either end.
	 * Driven entirely by script since these dynamically-created list rows are not part
	 * of the engine's default widget focus-navigation graph.
	 * @param direction - +1 for next, -1 for previous
	 */
	protected void MoveSlotListFocus(int direction)
	{
		int itemCount = m_cSlotListBoxComponent.GetItemCount();
		if (itemCount <= 0)
			return;

		int index = m_iFocusedSlotIndex;

		for (int attempts = 0; attempts < itemCount; attempts++)
		{
			if (index < 0)
				index = 0;
			else
				index = (index + direction + itemCount) % itemCount;

			CRF_ListBoxElementComponent elem = m_cSlotListBoxComponent.GetCRFElementComponent(index);
			if (elem && elem.m_iSlotId > 0)
			{
				FocusSlotListItem(index, elem);
				return;
			}
		}
	}

	/**
	 * Focuses and highlights a specific slot list row so it's unmistakable which slot
	 * a controller/keyboard user currently has selected.
	 * @param index - Index of the row within m_cSlotListBoxComponent
	 * @param elem - The row's element component
	 */
	protected void FocusSlotListItem(int index, CRF_ListBoxElementComponent elem)
	{
		// The row's root/SlotButton widget has no Color authored in its .layout at all, so
		// GetColor() on it returns null and SetColor(null) crashes on restore. Highlight the
		// row's text instead - PlayerName has an authored default Color, so capture/restore
		// is safe there.
		TextWidget textWidget = elem.GetPlayerText();
		if (textWidget)
		{
			if (m_wHighlightedSlotWidget && m_wHighlightedSlotWidget != textWidget)
			{
				TextWidget prevText = TextWidget.Cast(m_wHighlightedSlotWidget);
				if (prevText && m_cSlotListOriginalColor)
					prevText.SetColor(m_cSlotListOriginalColor);
			}

			if (m_wHighlightedSlotWidget != textWidget)
			{
				// GetColor() returns a live handle to the widget's own color state, not a
				// snapshot - SetColor(gold) below would otherwise mutate this "captured"
				// reference in place too. Deep-copy the components instead.
				Color captured = textWidget.GetColor();
				if (captured)
					m_cSlotListOriginalColor = new Color(captured.R(), captured.G(), captured.B(), captured.A());
				else
					m_cSlotListOriginalColor = Color.White;
			}

			textWidget.SetColor(Color.FromRGBA(255, 196, 0, 255));
			m_wHighlightedSlotWidget = textWidget;
		}

		m_iFocusedSlotIndex = index;

		// Still move engine focus onto whichever widget actually owns the click handler
		// (dedicated inner SlotButton for admin layouts, row root otherwise), so confirm/A
		// fires the same click path a mouse click would.
		Widget clickTarget;
		SCR_ButtonTextComponent slotBtn = elem.GetSlotButton();
		if (slotBtn)
			clickTarget = slotBtn.GetRootWidget();
		else
			clickTarget = elem.GetRootWidget();

		if (clickTarget)
			GetGame().GetWorkspace().SetFocusedWidget(clickTarget);
	}

	/**
	 * Clears slot list focus-navigation state. Called whenever the list is rebuilt
	 * (faction change, initial load) so a stale index doesn't point at rebuilt content.
	 */
	protected void ResetSlotListFocus()
	{
		m_iFocusedSlotIndex = -1;
		m_wHighlightedSlotWidget = null;
	}

	/**
	 * Cleans up resources when the menu is closed
	 * Removes event listeners and slot update callback
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		// Unregister from slot updates to prevent memory leaks
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Remove(UpdateSlots);
		CRF_SlottingManager.GetInstance().GetOnSlotChanged().Remove(UpdateSlotInPlace);

		// Unregister from community tag/rank updates
		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (tagMgr)
		{
			tagMgr.GetOnPlayerInfoUpdated().Remove(RefreshTagsAndRanks);
			tagMgr.GetOnPlayerRosterChanged().Remove(OnPlayerRosterChanged);
		}
		
		// Remove all input action listeners
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

		GetGame().GetInputManager().RemoveActionListener("CRF_SlottingFactionNext", EActionTrigger.DOWN, Action_CycleFactionNext);
		GetGame().GetInputManager().RemoveActionListener("CRF_SlottingFactionPrev", EActionTrigger.DOWN, Action_CycleFactionPrev);
		GetGame().GetInputManager().RemoveActionListener("CRF_SlottingToggleTab", EActionTrigger.DOWN, Action_ToggleUnslottedTab);
		GetGame().GetInputManager().RemoveActionListener("CRF_SlottingListNext", EActionTrigger.DOWN, Action_SlotListNext);
		GetGame().GetInputManager().RemoveActionListener("CRF_SlottingListPrev", EActionTrigger.DOWN, Action_SlotListPrev);
	}
	
	/**
	 * Advances to the next slotting phase if not already at final phase
	 * Admin-only functionality
	 */
	void AdvanceSlottingPhase()
	{
		// Skip if already at final phase (phase 2)
		if(m_Gamemode.m_SlottingState == 2)
			return;
		
		// Request phase advancement through RPL system
		CRF_PlayerRplToAuthorityManager.GetInstance().RequestAdvanceSlottingPhase();
	}
	
	/**
	 * Initializes the player and transitions from slotting to game
	 * Closes current menu and requests player initialization
	 */
	void InitilizePlayer()
	{
		// Close the slotting menu
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		
		// Request server to initialize the local player
		CRF_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
	}
	
	/**
	 * Schedules the player selection with a short delay
	 * Prevents immediate UI update issues
	 */
	void SelectPlayerDelay()
	{
		GetGame().GetCallqueue().CallLater(SelectPlayer);
	}
	
	/**
	 * Toggles selection of a player in the unslotted player list
	 * Used by admins to manage player slots
	 */
	void SelectPlayer()
	{
		int selectedPlayerId = m_cUnslotPlayerListBoxComponent.GetElementComponent(
			m_cUnslotPlayerListBoxComponent.GetSelectedItem()).m_iPlayerId;
		
		// Toggle selection state
		if(m_iSelectedplayerId == selectedPlayerId)
			m_iSelectedplayerId = 0; // Deselect if already selected
		else
			m_iSelectedplayerId = selectedPlayerId; // Select new player
			
		// Update UI to reflect changes
		UpdateSlots();
	}
	
	/**
	 * Switches the player panel to the Players tab
	 */
	void ShowPlayersTab()
	{
		m_bShowingUnslotted = false;
		m_wRoot.FindAnyWidget("PlayerList").SetVisible(true);
		m_wRoot.FindAnyWidget("UnslotPlayerList").SetVisible(false);
		ButtonWidget.Cast(m_wRoot.FindAnyWidget("TabButtonPlayers")).SetColor(Color.FromRGBA(37, 37, 37, 255));
		ButtonWidget.Cast(m_wRoot.FindAnyWidget("TabButtonUnslotted")).SetColor(Color.FromRGBA(11, 11, 11, 255));
	}
	
	/**
	 * Switches the player panel to the Unslotted Players tab
	 */
	void ShowUnslottedTab()
	{
		m_bShowingUnslotted = true;
		m_wRoot.FindAnyWidget("PlayerList").SetVisible(false);
		m_wRoot.FindAnyWidget("UnslotPlayerList").SetVisible(true);
		ButtonWidget.Cast(m_wRoot.FindAnyWidget("TabButtonPlayers")).SetColor(Color.FromRGBA(11, 11, 11, 255));
		ButtonWidget.Cast(m_wRoot.FindAnyWidget("TabButtonUnslotted")).SetColor(Color.FromRGBA(37, 37, 37, 255));
	}

	/**
	 * Controller Y / keyboard Tab handler - toggles between the Players and Unslotted tabs.
	 * No-op for non-admins since the Unslotted tab button is hidden for them.
	 */
	void Action_ToggleUnslottedTab()
	{
		if (!SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
			return;

		if (m_bShowingUnslotted)
			ShowPlayersTab();
		else
			ShowUnslottedTab();

		FocusActiveTabButton();
	}

	/**
	 * Moves widget focus onto whichever of the Players/Unslotted tab buttons is currently active
	 */
	protected void FocusActiveTabButton()
	{
		Widget focusTarget;
		if (m_bShowingUnslotted)
			focusTarget = m_wRoot.FindAnyWidget("TabButtonUnslotted");
		else
			focusTarget = m_wRoot.FindAnyWidget("TabButtonPlayers");

		if (focusTarget)
			GetGame().GetWorkspace().SetFocusedWidget(focusTarget);
	}
	
	/**
	 * Updates UI to show BLUFOR faction is selected
	 */
	void SelectFactionBlufor()
	{
		SelectFaction("BLUFOR", 
			Color.FromRGBA(34, 196, 244, 33), 
			true, false, false, false);
	}
	
	/**
	 * Updates UI to show OPFOR faction is selected
	 */
	void SelectFactionOpfor()
	{
		SelectFaction("OPFOR", 
			Color.FromRGBA(238, 49, 47, 33), 
			false, true, false, false);
	}
	
	/**
	 * Updates UI to show INDFOR faction is selected
	 */
	void SelectFactionIndfor()
	{
		SelectFaction("INDFOR", 
			Color.FromRGBA(0, 177, 79, 33), 
			false, false, true, false);
	}
	
	/**
	 * Updates UI to show CIV faction is selected
	 */
	void SelectFactionCiv()
	{
		SelectFaction("CIV", 
			Color.FromRGBA(168, 110, 207, 33), 
			false, false, false, true);
	}
	
	/**
	 * Common helper method for faction selection
	 * @param factionKey - Key identifying the faction
	 * @param bgColor - Background color for faction UI
	 * @param bluforVisible - Whether Blufor highlight is visible
	 * @param opforVisible - Whether Opfor highlight is visible
	 * @param indforVisible - Whether Indfor highlight is visible
	 * @param civVisible - Whether Civilian highlight is visible
	 */
	protected void SelectFaction(string factionKey, Color bgColor, bool bluforVisible, bool opforVisible, bool indforVisible, bool civVisible)
	{
		// Set selected faction
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		
		// Update UI visibility for faction selection indicators
		// This would be so much better if we had ternary operators #bohemiapls
		if (bluforVisible)
			m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(1);
		else
			m_wRoot.FindAnyWidget("BluforBGSelect").SetOpacity(0);
			
		if (opforVisible)
			m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(1);
		else
			m_wRoot.FindAnyWidget("OpforBGSelect").SetOpacity(0);
			
		if (indforVisible)
			m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(1);
		else
			m_wRoot.FindAnyWidget("IndforBGSelect").SetOpacity(0);
			
		if (civVisible)
			m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(1);
		else
			m_wRoot.FindAnyWidget("CivBGSelect").SetOpacity(0);
		
		// Set slot background color to match faction
		m_wRoot.FindAnyWidget("SlotsBG").SetColor(bgColor);
		
		// Update slot list for selected faction
		UpdateSlots();
	}
	
	/**
	 * Initializes slot counts for all factions
	 * Counts total and taken slots per faction
	 */
	void InitSlots()
	{	
		// Reset slot counters
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iTakenBluforSlots = 0;
		m_iTakenOpforSlots = 0;
		m_iTakenIndforSlots = 0;
		m_iTakenCivSlots = 0;
		
		// Get all slot data
		map<int, ref CRF_SlotData> slotMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		// Count slots for each faction
		foreach (int slotId, CRF_SlotData slotData : slotMap)
		{			
			// Skip locked or dead slots
			if(slotData.GetIsLockedSlot())
				continue;
			
			// Increment appropriate faction counter
			switch(slotData.GetSlotFactionKey())
			{
				case "BLUFOR":
					m_iBluforSlots++;
					if(slotData.GetSlotCurrentPlayerId() > 0) 
						m_iTakenBluforSlots++;
					break;
					
				case "OPFOR":
					m_iOpforSlots++;
					if(slotData.GetSlotCurrentPlayerId() > 0) 
						m_iTakenOpforSlots++;
					break;
					
				case "INDFOR":
					m_iIndforSlots++;
					if(slotData.GetSlotCurrentPlayerId() > 0) 
						m_iTakenIndforSlots++;
					break;
					
				case "CIV":
					m_iCivSlots++;
					if(slotData.GetSlotCurrentPlayerId() > 0) 
						m_iTakenCivSlots++;
					break;
			}
		}
	}
	
	/**
	 * Updates the slot display UI with current data
	 * Shows available slots for the selected faction
	 */
	//! Lightweight refresh — updates only tag and rank widgets on already-displayed list items.
	//! Called when the async tag/rank fetch completes; avoids a full rebuild of the slot list.
	protected void RefreshTagsAndRanks()
	{
		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (!tagMgr)
			return;

		map<int, ref CRF_SlotData> slotMap = CRF_SlottingManager.GetInstance().GetSlotMap();

		// Update slot list
		for (int i = 0; i < m_cSlotListBoxComponent.GetItemCount(); i++)
		{
			CRF_ListBoxElementComponent elem = m_cSlotListBoxComponent.GetCRFElementComponent(i);
			if (!elem)
				continue;
			CRF_SlotData slotData;
			if (!slotMap.Find(elem.m_iSlotId, slotData) || !slotData)
				continue;
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId <= 0)
				continue;
			elem.SetTagText(tagMgr.GetPlayerTag(playerId));
			elem.SetRankChevron(tagMgr.GetPlayerXp(playerId), tagMgr.GetPlayerRankTrack(playerId));
		}

		// Update ORBAT list
		for (int i = 0; i < m_cOrbatListBoxComponent.GetItemCount(); i++)
		{
			CRF_ListBoxElementComponent elem = m_cOrbatListBoxComponent.GetCRFElementComponent(i);
			if (!elem)
				continue;
			CRF_SlotData slotData;
			if (!slotMap.Find(elem.m_iSlotId, slotData) || !slotData)
				continue;
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId <= 0)
				continue;
			elem.SetTagText(tagMgr.GetPlayerTag(playerId));
			elem.SetRankChevron(tagMgr.GetPlayerXp(playerId), tagMgr.GetPlayerRankTrack(playerId));
		}

		// Update unslotted players list
		// Use m_iPlayerId stored on each element — avoids fragile index-order matching.
		for (int i = 0; i < m_cUnslotPlayerListBoxComponent.GetItemCount(); i++)
		{
			SCR_ListBoxElementComponent comp = m_cUnslotPlayerListBoxComponent.GetElementComponent(i);
			CRF_ListBoxElementComponent crfComp = CRF_ListBoxElementComponent.Cast(comp);
			if (!crfComp || crfComp.m_iPlayerId <= 0)
				continue;
			crfComp.SetTagText(tagMgr.GetPlayerTag(crfComp.m_iPlayerId));
			crfComp.SetRankChevron(tagMgr.GetPlayerXp(crfComp.m_iPlayerId), tagMgr.GetPlayerRankTrack(crfComp.m_iPlayerId));
		}
	}

	/**
	 * Called when roster changes (join/leave); rebuilds slot and unslotted lists
	 * so list-box state updates immediately without screen switches.
	 */
	protected void OnPlayerRosterChanged()
	{
		UpdateSlots();
	}

	void UpdateSlots()
	{
		// Re-initialize slot counts
		InitSlots();

		// Clear existing UI lists
		m_cSlotListBoxComponent.Clear();
		m_cOrbatListBoxComponent.Clear();

		// The list is being rebuilt, so any script-tracked controller focus index is now stale
		ResetSlotListFocus();
		
		// Exit if no faction is selected
		if(!m_fSelectedFaction)
			return;
		
		// Update UI border colors to match selected faction
		UpdateUIBorderColors();
		
		// Get slot data and groups for the selected faction
		map<int, ref CRF_SlotData> slotMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		array<SCR_AIGroup> groups = GetPlayableGroupsForSelectedFaction();
		
		// Populate UI with groups and slots
		PopulateGroupsAndSlots(groups, slotMap);
		
		// Reset selected player if they are now in a slot
		if(CRF_SlottingManager.GetInstance().IsPlayerInASlot(m_iSelectedplayerId))
			m_iSelectedplayerId = 0;

		// Update unslotted players list
		UpdateUnslottedPlayersList();
	}
	
	/**
	 * Updates UI border colors to match selected faction
	 */
	private void UpdateUIBorderColors()
	{
		Color factionColor = m_fSelectedFaction.GetFactionColor();
		PanelWidget.Cast(m_wRoot.FindAnyWidget("PlayerBorder")).SetColor(factionColor);
		PanelWidget.Cast(m_wRoot.FindAnyWidget("UnslotPlayerBorder")).SetColor(factionColor);
		PanelWidget.Cast(m_wRoot.FindAnyWidget("RoleBorder")).SetColor(factionColor);
	}
	
	/**
	 * Surgically updates a single slot element when only its player ID has changed.
	 * Subscribed to GetOnSlotChanged() so only player-claim/vacate events trigger this —
	 * lock, death, group, and role deltas still fall through to UpdateSlots() via GetOnSlottingUpdate().
	 * Eliminates the full Clear+rebuild that previously fired on every client for every slot click.
	 */
	void UpdateSlotInPlace()
	{
		int slotId = CRF_SlottingManager.GetInstance().GetLastChangedSlotId();
		if (slotId <= 0)
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		CRF_SlotData slotData = slottingManager.GetSlotData(slotId);
		
		if (!slotData || !m_fSelectedFaction)
			return;
		
		// Update faction slot counters (lightweight, no widget changes)
		InitSlots();
		
		// If the changed slot belongs to a different faction than the one currently displayed,
		// we only need to refresh the unslotted player list (player may have moved in/out).
		if (GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
		{
			UpdateUnslottedPlayersList();
			return;
		}
		
		// Locate the matching element in the main slot list and update it in place
		int currentPlayerId = slotData.GetSlotCurrentPlayerId();
		int elemCount = m_cSlotListBoxComponent.GetItemCount();
		
		for (int i = 0; i < elemCount; i++)
		{
			CRF_ListBoxElementComponent elem = CRF_ListBoxElementComponent.Cast(
				m_cSlotListBoxComponent.GetElementComponent(i));
			
			if (!elem || elem.m_iSlotId != slotId)
				continue;
			
			if (currentPlayerId > 0)
			{
				string playerName = GetGame().GetPlayerManager().GetPlayerName(currentPlayerId);
				string playerTag = "";
				if (CRF_CommunityTagManager.GetInstance())
					playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(currentPlayerId);
				
				elem.SetPlayerText(playerName);
				elem.SetTagText(playerTag);
				
				Color factionColor = GetGame().GetFactionManager().GetFactionByKey(
					slotData.GetSlotFactionKey()).GetFactionColor();
				elem.GetPlayerText().SetColor(factionColor);
				elem.GetRoleText().SetColor(factionColor);
				elem.GetPlayerText().SetOpacity(0.5);
				elem.GetRoleText().SetOpacity(0.5);
				
				elem.GetDisconnectWidget().SetVisible(
					!GetGame().GetPlayerManager().IsPlayerConnected(currentPlayerId));
			}
			else
			{
				// Slot vacated — restore OPEN/CLOSED status text using the same phase/type
				// logic as CRF_ListBoxComponent._AddItemSlot so the display matches initial state.
				CRF_ESlotType vacatedSlotType = slotData.GetSlotType();
				string statusText;
				if (slotData.GetIsLockedSlot())
				{
					statusText = "CLOSED";
				}
				else if (m_Gamemode.m_SlottingState == 0)
				{
					if (vacatedSlotType != CRF_ESlotType.TEAM_LEADER
						&& vacatedSlotType != CRF_ESlotType.SQUAD_LEADER
						&& vacatedSlotType != CRF_ESlotType.MEDIC)
						statusText = "CLOSED";
					else
						statusText = "OPEN";
				}
				else if (m_Gamemode.m_SlottingState == 1)
				{
					if (vacatedSlotType != CRF_ESlotType.TEAM_LEADER
						&& vacatedSlotType != CRF_ESlotType.SQUAD_LEADER
						&& vacatedSlotType != CRF_ESlotType.MEDIC
						&& vacatedSlotType != CRF_ESlotType.SPECIALTY
						&& vacatedSlotType != CRF_ESlotType.SPECIALTY_ASSISTANT)
						statusText = "CLOSED";
					else
						statusText = "OPEN";
				}
				else
				{
					statusText = "OPEN";
				}
				
				elem.SetPlayerText(statusText);
				elem.SetTagText("");
				elem.GetDisconnectWidget().SetVisible(false);
				elem.GetPlayerText().SetColor(Color.White);
				elem.GetRoleText().SetColor(Color.White);
				elem.GetPlayerText().SetOpacity(1.0);
				elem.GetRoleText().SetOpacity(1.0);
			}
			break;
		}
		
		// Rebuild orbat if this slot affects the ORBAT view (leader/medic roles)
		CRF_ESlotType slotType = slotData.GetSlotType();
		if (slotType == CRF_ESlotType.TEAM_LEADER
			|| slotType == CRF_ESlotType.SQUAD_LEADER
			|| slotType == CRF_ESlotType.MEDIC)
		{
			RebuildOrbat();
		}
		
		// Refresh unslotted players list
		UpdateUnslottedPlayersList();

		// If game is live and the local player was just placed into this specific slot, close and insert into unit
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME
			&& currentPlayerId == SCR_PlayerController.GetLocalPlayerId()
			&& !slotData.GetIsDeadSlot())
			InitilizePlayer();
	}

	/**
	 * Rebuilds only the ORBAT list without touching the main slot list.
	 * Called by UpdateSlotInPlace when a leader or medic slot changes.
	 * Replicates the leader-insertion logic of PopulateGroupsAndSlots, but orbat-only.
	 */
	private void RebuildOrbat()
	{
		m_cOrbatListBoxComponent.Clear();
		
		if (!m_fSelectedFaction)
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		map<int, ref CRF_SlotData> slotMap = slottingManager.GetSlotMap();
		array<SCR_AIGroup> groups = GetPlayableGroupsForSelectedFaction();
		bool isAdmin = SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId());
		
		foreach (SCR_AIGroup group : groups)
		{
			if (group.IsPrivate() && !isAdmin)
				continue;
			
			RplId groupId;
			if (!CRF_ReplicationHelper.GetRplId(group, groupId))
				continue;
			int leadersInGroup = 0;
			array<int> slotStored = {};
			
			int orbatGroupIndex = m_cOrbatListBoxComponent.AddItemGroup(
				null, group, "{55D48B298362DA71}UI/Listbox/GroupListBoxOrbatElementNonAdmin.layout");
			
			Color groupColor = group.GetFaction().GetFactionColor();
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).GetGroupUnderline().SetColor(groupColor);
			
			// Walk slots in their configured order (same as PopulateGroupsAndSlots)
			foreach (int id : slottingManager.GetAllSlotIDsForGroup(groupId))
			{
				ResourceName prefab = slottingManager.GetSlotData(id).GetSlotResource();
				foreach (int slotId, CRF_SlotData slotData : slotMap)
				{
					if (slotData.GetSlotResource() != prefab || slotStored.Contains(slotId))
						continue;
					if (slotData.GetSlotCurrentGroup() != groupId)
						continue;
					if (GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
						continue;
					
					slotStored.Insert(slotId);
					
					CRF_ESlotType slotType = slotData.GetSlotType();
					if ((slotType == CRF_ESlotType.TEAM_LEADER
						|| slotType == CRF_ESlotType.SQUAD_LEADER
						|| slotType == CRF_ESlotType.MEDIC)
						&& slotData.GetSlotCurrentPlayerId() > 0)
					{
						AddLeaderToOrbat(slotData, slotId, orbatGroupIndex, leadersInGroup);
						leadersInGroup++;
					}
					break;
				}
			}
			
			if (leadersInGroup == 0)
				m_cOrbatListBoxComponent.RemoveItem(orbatGroupIndex);
		}
	}
	
	/**
	 * Gets all playable groups for the selected faction
	 * @return Array of playable groups
	 */
	private array<SCR_AIGroup> GetPlayableGroupsForSelectedFaction()
	{
		array<SCR_AIGroup> factionGroups = CRF_SlottingManager.GetInstance().GetAllGroups(m_fSelectedFaction.GetFactionKey());
		
		if (factionGroups.IsEmpty())
			return new array<SCR_AIGroup>;
		
		return factionGroups;
	}
	
	/**
	 * Populates the UI with groups and their slots
	 * @param groups - Array of groups to display
	 * @param slotMap - Array of all slot data
	 */
	private void PopulateGroupsAndSlots(array<SCR_AIGroup> groups, map<int, ref CRF_SlotData> slotMap)
	{
		bool isAdmin = SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId());
		
		foreach(SCR_AIGroup group : groups)
		{	
			// Skip private groups for non-admins
			if(group.IsPrivate() && !isAdmin)
				continue;
			
			//For UI
			bool isGroupFull = true;
			
			// Track counts for this group
			int leadersInGroup = 0;
			int playersInGroup = 0;
			int deadPlayersInGroup = 0;
			
			// Add group to UI
			int groupIndex = m_cSlotListBoxComponent.AddItemGroup(null, group);
			int orbatGroupIndex = m_cOrbatListBoxComponent.AddItemGroup(null, group, "{55D48B298362DA71}UI/Listbox/GroupListBoxOrbatElementNonAdmin.layout");
			
			// Set group colors
			Color groupColor = group.GetFaction().GetFactionColor();
			
			if(group.GetFaction().GetFactionKey() == "INDFOR")
				m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().SetColor(groupColor);
			
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupUnderline().SetColor(groupColor);
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).GetGroupUnderline().SetColor(groupColor);
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().LoadImageFromSet(0, SCR_Faction.Cast(group.GetFaction()).GetGroupFlagImageSet(), group.GetGroupFlag());
			
			// Add admin-only controls
			if(isAdmin)
			{	
				SCR_ButtonTextComponent lockGroupBtn = m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetLockButton();
				if (lockGroupBtn)
					lockGroupBtn.m_OnClicked.Insert(LockGroupSlotsDelayed);
				GetGame().GetCallqueue().Call(SetupAdminGroupIcons, group, groupIndex);
			}
			
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupIcon().LoadImageFromSet(0, SCR_Faction.Cast(group.GetFaction()).GetGroupFlagImageSet(), group.GetGroupFlag());
			
			// Add slots to this group
			AddSlotsToGroup(group, slotMap, groupIndex, orbatGroupIndex, leadersInGroup, playersInGroup, deadPlayersInGroup, isAdmin, isGroupFull);
			
			if (isGroupFull)
				m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).GetGroupText().SetColor(groupColor);
			
			// Clean up empty groups
			RemoveEmptyGroups(groupIndex, orbatGroupIndex, leadersInGroup, playersInGroup, deadPlayersInGroup, isAdmin);
		}
	}
	
	/**
	 * Sets up admin-specific icons for a group
	 * @param group - Group to set up controls for
	 * @param groupIndex - UI index of the group
	 */
	private void SetupAdminGroupIcons(SCR_AIGroup group, int groupIndex)
	{
		if(group.IsPrivate())
			m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex).SetLockImage(
				"{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds", "lockimage");
	}
	
	/**
	 * Adds slots to a group in the UI
	 * @param group - Group to add slots to
	 * @param slotMap - Array of all slot data
	 * @param groupIndex - UI index of the group
	 * @param orbatGroupIndex - UI index in orbat view
	 * @param leadersInGroup - Counter for leaders in group
	 * @param playersInGroup - Counter for players in group
	 * @param deadPlayersInGroup - Counter for dead players in group
	 * @param isAdmin - Whether current player is admin
	 */
	private void AddSlotsToGroup(SCR_AIGroup group, map<int, ref CRF_SlotData> slotMap, 
		int groupIndex, int orbatGroupIndex, out int leadersInGroup, out int playersInGroup, 
		out int deadPlayersInGroup, bool isAdmin, out bool isGroupFull)
	{
		RplId groupId;
		if (!CRF_ReplicationHelper.GetRplId(group, groupId))
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		array<int> slotStored = {};
		isGroupFull = true;
		foreach (int id: slottingManager.GetAllSlotIDsForGroup(groupId))
		{
			ResourceName prefab = slottingManager.GetSlotData(id).GetSlotResource();
			foreach(int slotId, CRF_SlotData slotData : slotMap)
			{	
				if (slotData.GetSlotResource() != prefab || slotStored.Contains(slotId))
					continue;
				// Skip slots not in this group or faction
				if (slotData.GetSlotCurrentGroup() != groupId || 
					GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
					continue;
				
				// Skip locked slots for non-admins
				if (slotData.GetIsLockedSlot() && !isAdmin && slotData.GetSlotCurrentPlayerId() <= 0)
					continue;
				
				// Track dead slots but don't display them
				if (slotData.GetIsDeadSlot())
				{
					deadPlayersInGroup++;
					continue;
				}
				
				// Skip dead empty slots
				if (slotData.GetSlotCurrentPlayerId() == 0 && slotData.GetIsDeadSlot())
					continue;
				
				// Add slot to UI
				int slotIndex = m_cSlotListBoxComponent.AddItemSlot(null, slotId);
				slotStored.Insert(slotId);
				
				// Count players
				if (slotData.GetSlotCurrentPlayerId() >= 0)
					playersInGroup++;
				
				// Set player text if slot is taken
				if (slotData.GetSlotCurrentPlayerId() > 0)
				{
					string playerName = GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId());
					string playerTag = "";
					int slotPlayerXp = -1;
					string slotPlayerTrack = "enlisted";
					if (CRF_CommunityTagManager.GetInstance())
					{
						playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(slotData.GetSlotCurrentPlayerId());
						slotPlayerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(slotData.GetSlotCurrentPlayerId());
						slotPlayerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(slotData.GetSlotCurrentPlayerId());
					}
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetPlayerText(playerName);
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetTagText(playerTag);
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetRankChevron(slotPlayerXp, slotPlayerTrack);
					//Sets slot to faction color when selected
					//m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetSlottedWidget().SetVisible(true);
					Color factionColor = GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()).GetFactionColor();
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetPlayerText().SetColor(factionColor);
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetRoleText().SetColor(factionColor);
					
					//Sets the opacity too
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetPlayerText().SetOpacity(0.5);
					m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetRoleText().SetOpacity(0.5);
					
				}
				else
					isGroupFull = false;
				
				// subscribe to SlotButton's m_OnClicked specifically so that
				// clicking LockButton or KickButton does NOT propagate up and accidentally trigger
				// slot selection. Fall back to the element's own m_OnClicked only for layouts that
				// have no dedicated SlotButton (those layouts also have no admin sub-buttons, so
				// propagation interference is not a concern).
				CRF_ListBoxElementComponent slotElem = m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex);
				if (slotElem)
				{
					SCR_ButtonTextComponent slotBtn = slotElem.GetSlotButton();
					if (slotBtn)
						slotBtn.m_OnClicked.Insert(SelectSlotDelay);
					else
						slotElem.m_OnClicked.Insert(SelectSlotDelay);
				}
				
				CRF_ESlotType slotType = slotData.GetSlotType();
				
				// Add leaders/medics to ORBAT view
				if ((slotType == CRF_ESlotType.TEAM_LEADER 
					|| slotType == CRF_ESlotType.SQUAD_LEADER 
					|| slotType == CRF_ESlotType.MEDIC) 
					&& slotData.GetSlotCurrentPlayerId() > 0)
				{
					AddLeaderToOrbat(slotData, slotId, orbatGroupIndex, leadersInGroup);
					leadersInGroup++;
				}
				
				// Add admin-only slot controls
				if (isAdmin)
					SetupAdminSlotControls(slotIndex, slotData);
				
				break;
			}
		}
	}
	
	/**
	 * Adds a leader slot to the ORBAT view
	 * @param slotData - Slot data for the leader
	 * @param slotId - ID of the slot
	 * @param orbatGroupIndex - UI index of group in orbat view
	 * @param leadersInGroup - Counter for leaders in group
	 */
	private void AddLeaderToOrbat(CRF_SlotData slotData, int slotId, int orbatGroupIndex, int leadersInGroup)
	{
		int orbatIndex = m_cOrbatListBoxComponent.AddItemSlot(null, slotId, 
			"{BD36FFAE9AB69175}UI/Listbox/PlayerSlotListboxOrbatElementNonAdmin.layout");
		
		// Set player text
		string playerName = GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId());
		string playerTag = "";
		int orbatPlayerXp = -1;
		string orbatPlayerTrack = "enlisted";
		if (CRF_CommunityTagManager.GetInstance())
		{
			playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(slotData.GetSlotCurrentPlayerId());
			orbatPlayerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(slotData.GetSlotCurrentPlayerId());
			orbatPlayerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(slotData.GetSlotCurrentPlayerId());
		}
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetPlayerText(playerName);
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetTagText(playerTag);
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetRankChevron(orbatPlayerXp, orbatPlayerTrack);
		
		// Show disconnect indicator if player not connected
		if (!GetGame().GetPlayerManager().IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetDisconnectWidget().SetVisible(true);
		
		// Hide slot button in orbat view
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetSlotButton().SetVisible(false);
		
		// Set group icon based on first leader's role
		if (leadersInGroup == 0)
		{
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetRoleImage(
				slotData.GetSlotIconResource(), "groupRoleName");
			
			Color factionColor = GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()).GetFactionColor();
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetGroupIconColor(factionColor);
		}
	}
	
	/**
	 * Sets up admin-specific controls for a slot
	 * @param slotIndex - UI index of the slot
	 * @param slotData - Slot data
	 */
	private void SetupAdminSlotControls(int slotIndex, CRF_SlotData slotData)
	{
		SCR_ButtonTextComponent lockBtn = m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetLockButton();
		if (lockBtn)
			lockBtn.m_OnClicked.Insert(LockSlotDelay);
		SCR_ButtonTextComponent kickBtn = m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetKickButton();
		if (kickBtn)
			kickBtn.m_OnClicked.Insert(KickSlotDelay);
		
		if (slotData.GetIsLockedSlot())
			m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetLockImage(
				"{564794579B2DB679}UI/Textures/Editor/Attributes/Attribute_Locked.edds", "lockimage");
	}
	
	/**
	 * Removes empty groups from the UI
	 * @param groupIndex - UI index of the group
	 * @param orbatGroupIndex - UI index in orbat view
	 * @param leadersInGroup - Counter for leaders in group
	 * @param playersInGroup - Counter for players in group
	 * @param deadPlayersInGroup - Counter for dead players in group
	 * @param isAdmin - Whether current player is admin
	 */
	private void RemoveEmptyGroups(int groupIndex, int orbatGroupIndex, int leadersInGroup, 
		int playersInGroup, int deadPlayersInGroup, bool isAdmin)
	{
		// Remove groups with no leaders from orbat view
		if(leadersInGroup == 0)	
			m_cOrbatListBoxComponent.RemoveItem(orbatGroupIndex);
		
		// Non-admins don't see empty groups
		if(playersInGroup == 0 && !isAdmin)
			m_cSlotListBoxComponent.RemoveItem(groupIndex);
		
		// Admins don't see groups that are empty and have only dead slots
		if(deadPlayersInGroup > 0 && playersInGroup == 0 && isAdmin)
			m_cSlotListBoxComponent.RemoveItem(groupIndex);
	}
	
	/**
	 * Updates the list of unslotted players
	 */
	private void UpdateUnslottedPlayersList()
	{
		m_cUnslotPlayerListBoxComponent.Clear();
		
		// Get all player IDs
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		foreach(int playerId : playerIds)
		{	
			// Skip invalid players, players without faction, already slotted players, or disconnected players
			if (playerId <= 0)
				continue;
			
			if (slottingManager.GetPlayerSlotFaction(playerId, true))
				continue;
				
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
			
			// Add player to unslotted list
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string playerTag = "";
			int unslottedXp = -1;
			string unslottedTrack = "enlisted";
			if (CRF_CommunityTagManager.GetInstance())
			{
				playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(playerId);
				unslottedXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(playerId);
				unslottedTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(playerId);
			}
			int index = m_cUnslotPlayerListBoxComponent.AddItemAndIconPlayer(
				playerName, 
				EMPTY_RESOURCE, 
				"flag", 
				null,
				"{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout", 
				playerId);
			
			// Set up selection handler — subscribe to the element's own m_OnClicked
			// (same pattern as the slot list) because GetSelectButton() can return null
			// when SCR_ButtonTextComponent fails to attach, and even when it doesn't,
			// a sub-button click may not update the listbox's m_iCurrentItem before
			// SelectPlayer reads GetSelectedItem(). The element's m_OnClicked fires
			// after OnItemClick has already updated the selection state, so it's safe.
			SCR_ListBoxElementComponent comp = m_cUnslotPlayerListBoxComponent.GetElementComponent(index);
			CRF_ListBoxElementComponent crfComp = CRF_ListBoxElementComponent.Cast(comp);
			if (crfComp)
			{
				crfComp.SetTagText(playerTag);
				crfComp.SetRankChevron(unslottedXp, unslottedTrack);
				crfComp.m_OnClicked.Insert(SelectPlayerDelay);
			}
			
			// Highlight admins, moderators, and selected players
			SetPlayerStatusColor(playerId, comp);
		}
	}
	
	/**
	 * Schedules the kick slot action with a short delay to avoid UI issues
	 */
	void KickSlotDelay()
	{
		GetGame().GetCallqueue().CallLater(KickSlot, 10, false);
	}
	
	/**
	 * Removes a player from their selected slot
	 * Sets the slot's player ID to 0 (empty)
	 */
	void KickSlot()
	{
		int selectedSlotId = m_cSlotListBoxComponent.GetCRFElementComponent(
			m_cSlotListBoxComponent.GetSelectedItem()).m_iSlotId;
		// Use batched RPC to remove player from slot
		CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(selectedSlotId, 0);
	}
	
	/**
	 * Schedules the group lock action with a short delay to avoid UI issues
	 */
	void LockGroupSlotsDelayed()
	{
		GetGame().GetCallqueue().Call(LockGroupSlots);
	}
	
	/**
	 * Locks or unlocks all slots in the selected group
	 * Toggles the locked state for the entire group
	 */
	void LockGroupSlots()
	{
		// Get selected group
		CRF_ListBoxElementComponent selectedElement = m_cSlotListBoxComponent.GetCRFElementComponent(
			m_cSlotListBoxComponent.GetSelectedItem());
		
		if(!selectedElement || !selectedElement.group)
			return;
		
		SCR_AIGroup aiGroup = selectedElement.group;
		
		// Get group ID for network sync
		RplId groupRplID;
		if (!CRF_ReplicationHelper.GetRplId(aiGroup, groupRplID))
			return;
		
		// Get all slot IDs for this group
		array<int> slotsInGroup = CRF_SlottingManager.GetInstance().GetAllSlotIDsForGroup(groupRplID);
		
		// If group is currently private (locked), unlock it
		if(aiGroup.IsPrivate())
		{
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateGroupLockedState(groupRplID, false);
			
			// Unlock all slots in group using batched updates
			foreach(int slotId : slotsInGroup)
			{
				// Use batched RPC to unlock slot
				CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotLockedState(slotId, false);
			}
		}
		// If group is currently unlocked, lock it
		else
		{
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateGroupLockedState(groupRplID, true);
			
			// Lock all slots in group using batched updates
			foreach(int slotId : slotsInGroup)
			{
				// Use batched RPC: lock slot and remove player in one call
				CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotLockedState(slotId, true);
			}
		}
	}
	
	/**
	 * Schedules the slot lock action with a short delay to avoid UI issues
	 */
	void LockSlotDelay()
	{
		GetGame().GetCallqueue().Call(LockSlot);
	}
	
	/**
	 * Locks or unlocks a selected slot
	 * Toggles between locked and unlocked state
	 */
	void LockSlot()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager || !m_cSlotListBoxComponent)
			return;

		CRF_ListBoxElementComponent selectedComponent = m_cSlotListBoxComponent.GetCRFElementComponent(
			m_cSlotListBoxComponent.GetSelectedItem());
		if (!selectedComponent)
			return;

		int selectedSlotId = selectedComponent.m_iSlotId;
		
		RplComponent groupRplComponent = RplComponent.Cast(Replication.FindItem(slottingManager.GetSlotData(selectedSlotId).GetSlotCurrentGroup()));
		if (!groupRplComponent)
			return;

		SCR_AIGroup tempGroup = SCR_AIGroup.Cast(groupRplComponent.GetEntity());
		if (!tempGroup || tempGroup.IsPrivate())
			return;
		
		bool isCurrentlyLocked = slottingManager.GetSlotData(selectedSlotId).GetIsLockedSlot();
		
		// Toggle slot lock state using batched updates
		if(isCurrentlyLocked)
			// Use batched RPC to unlock slot
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotLockedState(selectedSlotId, false);
		else
			// Use batched RPC to lock slot
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotLockedState(selectedSlotId, true);
	}
	
	/**
	 * Opens the preview menu
	 * Closes the slotting menu and returns to the preview screen
	 */
	void OpenSlottingMenu()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
	}
	
	/**
	 * Advances the gamemode to the next state
	 * Admin-only functionality
	 */
	void AdvanceMenu()
	{
		CRF_PlayerRplToAuthorityManager.GetInstance().RequestAdvanceGamemodeState(false);
	}
	
	/**
	 * Updates the menu each frame
	 * @param tDelta - Time since last update
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		// Update time display
		UpdateTimeDisplay();
		
		// Update player lists
		UpdatePlayerLists();
		
		// Update chat panel if available
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		// Update player count text
		int playerCount = GetGame().GetPlayerManager().GetPlayerCount();
		TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersText")).SetText("Players: " + playerCount);
		
		// Update faction ratio calculation
		UpdateRatioCalculation(playerCount);
		
		// Update faction slot counts
		UpdateFactionSlotCounts();
		
		// Update slotting phase display
		UpdateSlottingPhaseDisplay();
		
		// Show additional controls for admins
		UpdateAdminUI();

		// Repeat slot-list navigation while D-pad up/down is held, not just on the initial tap
		UpdateSlotListRepeat(tDelta);
	}

	/**
	 * Polled every frame so holding CRF_SlottingListNext/Prev keeps stepping through the slot
	 * list, not just the initial tap that AddActionListener's DOWN trigger already handles.
	 * The underlying list widget scrolls on its own while a d-pad direction is held (native
	 * engine behavior, unrelated to this), which is what motivated adding this: without it the
	 * list visibly scrolls but the script-tracked highlighted slot never follows along.
	 * @param tDelta - Time since last update
	 */
	protected void UpdateSlotListRepeat(float tDelta)
	{
		InputManager im = GetGame().GetInputManager();
		float nextVal = im.GetActionValue("CRF_SlottingListNext");
		float prevVal = im.GetActionValue("CRF_SlottingListPrev");

		int heldDirection = 0;
		if (nextVal > 0.5)
			heldDirection = 1;
		else if (prevVal > 0.5)
			heldDirection = -1;

		// Direction released or just changed - the DOWN-trigger listener already handles the
		// initial step for a fresh press, so just (re)start the hold timer here.
		if (heldDirection == 0 || heldDirection != m_iSlotListHoldDirection)
		{
			m_iSlotListHoldDirection = heldDirection;
			m_fSlotListHoldTimer = 0;
			return;
		}

		// Same direction still held - wait out an initial delay, then repeat at a steady rate
		m_fSlotListHoldTimer += tDelta;

		if (m_fSlotListHoldTimer >= 0.4)
		{
			m_fSlotListHoldTimer -= 0.12;
			MoveSlotListFocus(heldDirection);
		}
	}
	
	/**
	 * Updates the in-game time display with proper formatting
	 */
	private void UpdateTimeDisplay()
	{
		TimeContainer timeContainer = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager().GetTime();
		int hours = timeContainer.m_iHours;
		int minutes = timeContainer.m_iMinutes;
		
		string minuteString;
		string hourString;
		
		// Format minutes with leading zero if needed
		if(minutes < 10)
			minuteString = "0" + minutes.ToString();
		else
			minuteString = minutes.ToString();
		
		// Format hours with leading zero if needed
		if(hours < 10)
			hourString = "0" + hours.ToString();
		else
			hourString = hours.ToString();
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText")).SetText("Time: " + hourString + ":" + minuteString);
	}
	
	/**
	 * Updates player lists in the UI
	 * Shows admins first, then regular players
	 */
	private void UpdatePlayerLists()
	{
		// Get all player IDs
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		
		// Clear the player list
		m_cPlayerListBoxComponent.Clear();
		
		// First add admin players to list
		foreach(int playerId : playerIds)
		{
			if(!GetGame().GetPlayerManager().IsPlayerConnected(playerId) || !SCR_Global.IsAdmin(playerId))
				continue;
				
			AddPlayerToList(playerId);
		}
		
		// Then add non-admin players to list
		foreach(int playerId : playerIds)
		{
			if(!GetGame().GetPlayerManager().IsPlayerConnected(playerId) || SCR_Global.IsAdmin(playerId))
				continue;
				
			AddPlayerToList(playerId);
		}
	}
	
	private void SetPlayerStatusColor(int playerId, SCR_ListBoxElementComponent comp)
	{
		// Selected player
		if (playerId == m_iSelectedplayerId)
		{
			comp.SetColor(Color.DarkYellow);
			return;
		}
		// Enforce precedence: Admin > Moderator > Donator 
		if (SCR_Global.IsAdmin(playerId))
		{
			comp.SetColor(Color.Red);
			return;
		}

		if (CRF_PermissionManager.GetInstance().IsModerator(playerId))
		{
			comp.SetColor(Color.Yellow);
			return;
		}
		
		if (CRF_PermissionManager.GetInstance().IsDonator(playerId))
		{
			comp.SetColor(Color.Violet);
			return;
		}
	}
	
	/**
	 * Adds a player to the player list with appropriate faction icon and status color
	 * @param playerId - ID of the player to add
	 */
	private void AddPlayerToList(int playerId)
	{
		int listIndex;
		ResourceName playerIconResource;
		Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId, true);
		
		// Add player with appropriate faction icon
		if (playerFaction)
			playerIconResource = GetFactionIcon(playerFaction.GetFactionKey());
		 else 
			playerIconResource = EMPTY_RESOURCE;
			
		string displayName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string playerTag = "";
		int playerXp = -1;
		string playerTrack = "enlisted";
		if (CRF_CommunityTagManager.GetInstance())
		{
			playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(playerId);
			playerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(playerId);
			playerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(playerId);
		}

		listIndex = m_cPlayerListBoxComponent.AddItemAndIconPlayer(
			displayName, 
			playerIconResource, 
			"flag", 
			null, 
			"{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout");
		
		// Apply appropriate color based on player status
		SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(listIndex);
		CRF_ListBoxElementComponent crfComp = CRF_ListBoxElementComponent.Cast(comp);
		if (crfComp)
		{
			crfComp.SetTagText(playerTag);
			crfComp.SetRankChevron(playerXp, playerTrack);
		}
		
		SetPlayerStatusColor(playerId, comp);
		
		// Highlight players who are talking
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			if (m_MenuManager.m_aPlayersTalking.Contains(playerId))
				comp.SetTalking();
		}
		else
		{
			if (playerId == SCR_PlayerController.GetLocalPlayerId())
			{
				if (m_VONController.m_bIsBroadcasting)
					comp.SetTalking();
			}
		}
	}
	
	/**
	 * Gets the faction icon resource for a given faction key
	 * @param factionKey - The faction key to get the icon for
	 * @return ResourceName of the faction icon
	 */
	private ResourceName GetFactionIcon(string factionKey)
	{
		if (factionKey == "BLUFOR")
			return m_rBluforIcon;
		if (factionKey == "OPFOR")
			return m_rOpforIcon;
		if (factionKey == "INDFOR")
			return m_rIndforIcon;
		if (factionKey == "CIV")
			return m_rCivIcon;
			
		return EMPTY_RESOURCE;
	}
	
	/**
	 * Updates the ratio calculation display based on player count
	 * @param playerCount - Current number of players
	 */
	private void UpdateRatioCalculation(int playerCount)
	{
		// Get ratio values from UI
		int leftRatio = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox1")).GetText().ToInt();
		int rightRatio = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("RatioBox2")).GetText().ToInt();
		
		// Avoid division by zero
		if(leftRatio + rightRatio == 0)
			return;
		
		// Calculate and display actual player counts based on ratio
		int leftPlayers = Math.Round(playerCount / (leftRatio + rightRatio) * leftRatio);
		int rightPlayers = Math.Round(playerCount / (leftRatio + rightRatio) * rightRatio);
		
		TextWidget.Cast(m_wRoot.FindAnyWidget("Final")).SetText(leftPlayers.ToString() + " : " + rightPlayers.ToString());
	}
	
	/**
	 * Updates the faction slot count displays for each valid faction
	 */
	private void UpdateFactionSlotCounts()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		// Update BLUFOR slot count if faction is valid
		if(slottingManager.IsFactionValid("BLUFOR"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsBlufor")).SetText(m_iTakenBluforSlots.ToString() + "/" + m_iBluforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLockBG")).SetColor(Color.FromRGBA(63, 63, 63, 0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("BluforFactionLock")).SetColor(Color.FromRGBA(255, 255, 255, 0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonBlufor")).SetEnabled(true);
		}

		// Update OPFOR slot count if faction is valid
		if(slottingManager.IsFactionValid("OPFOR"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsOpfor")).SetText(m_iTakenOpforSlots.ToString() + "/" + m_iOpforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLockBG")).SetColor(Color.FromRGBA(63, 63, 63, 0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("OpforFactionLock")).SetColor(Color.FromRGBA(255, 255, 255, 0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonOpfor")).SetEnabled(true);
		}

		// Update INDFOR slot count if faction is valid
		if(slottingManager.IsFactionValid("INDFOR"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsIndfor")).SetText(m_iTakenIndforSlots.ToString() + "/" + m_iIndforSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLockBG")).SetColor(Color.FromRGBA(63, 63, 63, 0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("IndforFactionLock")).SetColor(Color.FromRGBA(255, 255, 255, 0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonIndfor")).SetEnabled(true);
		}

		// Update CIV slot count if faction is valid
		if(slottingManager.IsFactionValid("CIV"))
		{
			TextWidget.Cast(m_wRoot.FindAnyWidget("SlotsCiv")).SetText(m_iTakenCivSlots.ToString() + "/" + m_iCivSlots);
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLockBG")).SetColor(Color.FromRGBA(63, 63, 63, 0));
			ImageWidget.Cast(m_wRoot.FindAnyWidget("CivFactionLock")).SetColor(Color.FromRGBA(255, 255, 255, 0));
			ButtonWidget.Cast(m_wRoot.FindAnyWidget("ButtonCiv")).SetEnabled(true);
		}
	}
	
	/**
	 * Updates the slotting phase display and plays notification sound when phase changes
	 */
	private void UpdateSlottingPhaseDisplay()
	{
		// Check if slotting state has changed
		if(m_LocalSlottingState != m_Gamemode.m_SlottingState)
		{
			// Update local state and play notification sound
			m_LocalSlottingState = m_Gamemode.m_SlottingState;
			AudioSystem.PlaySound("{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav");
		}
		
		// Update phase text based on current slotting state
		string phaseText;
		if(m_Gamemode.m_SlottingState == 0)
			phaseText = "Leaders and Medics";
		else if(m_Gamemode.m_SlottingState == 1)
			phaseText = "Specialties";
		else
			phaseText = "Everyone";
			
		TextWidget.Cast(m_wRoot.FindAnyWidget("CurrentSlotPhase")).SetText(phaseText);
	}
	
	/**
	 * Updates admin-only UI elements
	 * Shows additional controls for admins
	 */
	private void UpdateAdminUI()
	{
		// Only show admin controls for admins
		if(SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
		{
			ButtonWidget previewButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PreviewButton"));
			ButtonWidget gameButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("GameButton"));
			ButtonWidget aarButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("AARButton"));
			ButtonWidget advanceButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("Advance"));
			
			// Enable admin buttons
			gameButton.SetEnabled(true);
			advanceButton.SetEnabled(true);
			
			// Show admin-only UI sections
			m_wRoot.FindAnyWidget("SlottingPhases").SetOpacity(1);
			FrameWidget.Cast(m_wRoot.FindAnyWidget("AdvanceFrame")).SetOpacity(1);
			m_wRoot.FindAnyWidget("TabButtonUnslotted").SetVisible(true);
		}
	}
	
	/**
	 * Schedules the slot selection action with a short delay to avoid UI issues
	 */
	void SelectSlotDelay()
	{
		GetGame().GetCallqueue().Call(SelectSlot);
	}
	
	/**
	 * Handles a player selecting a slot
	 * Different behavior for admins and regular players
	 */
	void SelectSlot()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager || !m_cSlotListBoxComponent)
			return;
		
		// Get selected slot information
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(
			m_cSlotListBoxComponent.GetElementComponent(m_cSlotListBoxComponent.GetSelectedItem()));
		if (!comp)
			return;

		int slotId = comp.m_iSlotId;
		
		// Exit if no valid slot selected
		if (slotId == 0)
			return;
		
		RplComponent groupRplComponent = RplComponent.Cast(Replication.FindItem(slottingManager.GetSlotData(slotId).GetSlotCurrentGroup()));
		if (!groupRplComponent)
			return;

		SCR_AIGroup tempGroup = SCR_AIGroup.Cast(groupRplComponent.GetEntity());
		if (!tempGroup || tempGroup.IsPrivate())
			return;
		
		// Get current player and slot information
		bool isAdmin = SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId());
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		
		// Check slotting phase restrictions
		bool leaderAndMedicPhase = m_Gamemode.m_SlottingState == 0;
		CRF_ESlotType slotType = slottingManager.GetSlotData(slotId).GetSlotType();
		
		bool slotNotLeaderOrMedic = 
				( slotType != CRF_ESlotType.TEAM_LEADER 
				&& slotType != CRF_ESlotType.SQUAD_LEADER 
				&& slotType != CRF_ESlotType.MEDIC);
		
		bool specialtyPhase = m_Gamemode.m_SlottingState == 1;
		
		bool slotNotSpecialtyOrLM = 
				(slotType != CRF_ESlotType.TEAM_LEADER 
				&& slotType != CRF_ESlotType.SQUAD_LEADER  
				&& slotType != CRF_ESlotType.MEDIC 
				&& slotType != CRF_ESlotType.SPECIALTY 
				&& slotType != CRF_ESlotType.SPECIALTY_ASSISTANT);
		
		// Check phase restrictions (if not admin)
		if (!isAdmin) {
			// In leader/medic phase, only allow those slots
			if (leaderAndMedicPhase && slotNotLeaderOrMedic) 
				return;
				
			// In specialty phase, only allow specialty or leader/medic slots
			if (specialtyPhase && slotNotSpecialtyOrLM)
				return;
		}
		
		// Handle admin selecting a player for a slot
		if (m_iSelectedplayerId > 0 && isAdmin)
		{
			HandleAdminSlotSelection(slotId, slottingManager);
			return;
		}
		
		// Handle regular player slotting
		HandlePlayerSlotSelection(slotId, slottingManager, localPlayerId);
	}
	
	/**
	 * Handles an admin selecting a player for a slot
	 * @param slotId - ID of the selected slot
	 * @param slottingManager - Reference to the slotting manager
	 */
	private void HandleAdminSlotSelection(int slotId, CRF_SlottingManager slottingManager)
	{
		int currentPlayerId = slottingManager.GetSlotData(slotId).GetSlotCurrentPlayerId();
		
		// If selected player is already in this slot, unslot them
		if (currentPlayerId == m_iSelectedplayerId)
		{
			// remove player from slot
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, 0);
			m_iSelectedplayerId = 0;
			// Clear visual selection from whichever player list is currently active
			if (m_bShowingUnslotted)
				m_cUnslotPlayerListBoxComponent.SetItemSelected(m_cUnslotPlayerListBoxComponent.GetSelectedItem(), false, false, false);
			else
				m_cPlayerListBoxComponent.SetItemSelected(m_cPlayerListBoxComponent.GetSelectedItem(), false, false, false);
		} 
		// If slot is empty, move player to this slot
		else if (currentPlayerId == 0) 
		{
			// If player is already in another slot, remove them first
			if (slottingManager.IsPlayerInASlot(m_iSelectedplayerId))
			{
				int currentSlotId = slottingManager.GetPlayerSlotID(m_iSelectedplayerId);
				// remove player from current slot
				CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(currentSlotId, 0);
			}
			
			// Move player to the new slot
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, m_iSelectedplayerId);
			
			// Reset selection
			m_iSelectedplayerId = 0;
			// Clear visual selection from whichever player list is currently active
			if (m_bShowingUnslotted)
				m_cUnslotPlayerListBoxComponent.SetItemSelected(m_cUnslotPlayerListBoxComponent.GetSelectedItem(), false, false, false);
			else
				m_cPlayerListBoxComponent.SetItemSelected(m_cPlayerListBoxComponent.GetSelectedItem(), false, false, false);
		}
	}
	
	/**
	 * Handles a regular player selecting a slot
	 * @param slotId - ID of the selected slot
	 * @param slottingManager - Reference to the slotting manager
	 * @param localPlayerId - ID of the local player
	 */
	private void HandlePlayerSlotSelection(int slotId, CRF_SlottingManager slottingManager, int localPlayerId)
	{
		int currentPlayerId = slottingManager.GetSlotData(slotId).GetSlotCurrentPlayerId();
		
		// Skip if slot is already taken by someone else
		if (currentPlayerId != 0 && currentPlayerId != localPlayerId)
			return;
		
		// If player is already in this slot, unslot them
		if (currentPlayerId == localPlayerId)
		{
			// remove player from slot
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, 0);
		} 
		// If slot is empty, move player to this slot
		else if (currentPlayerId == 0) 
		{
			// If player is already in another slot, remove them first
			if (slottingManager.IsPlayerInASlot(localPlayerId))
			{
				int currentSlotId = slottingManager.GetPlayerSlotID(localPlayerId);
				// remove player from current slot
				CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(currentSlotId, 0);
			}
			
			// Move player to the new slot
			CRF_PlayerRplToAuthorityManager.GetInstance().UpdateSlotPlayerID(slotId, localPlayerId);
		}
	}
	
	/**
	 * Enables Voice-Over-Network (VON) when the push-to-talk button is pressed
	 */
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);

		// Get VON component and configure for transmission
		SCR_VoNComponent von = SCR_VoNComponent.Cast(
			GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));

		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}

	/**
	 * Gets a radio transceiver for VON communication
	 * @return RadioTransceiver object configured for voice communication
	 */
	RadioTransceiver GetVoNTransiver()
	{
		// Get player entity and inventory
		IEntity playerEntity = GetGame().GetPlayerController().GetControlledEntity();
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(
			playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));

		// Get all items in inventory
		array<IEntity> items = {};
		inventory.GetItems(items);

		// Find radio entity
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if(item.FindComponent(BaseRadioComponent))
				radioEntity = item;
		}

		// Configure radio
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);

		// Configure and return transceiver
		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);

		return transceiver;
	}

	/**
	 * Delayed function to disable VON
	 * Called after push-to-talk button is released
	 */
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(
			GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));

		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}

	/**
	 * Disables VON when push-to-talk button is released
	 */
	void Action_VONOff()
	{
		GetGame().GetCallqueue().Call(LobbyVoNDisableDelayed);
	}
	
	/**
	 * Handles chat toggle action
	 * Opens chat panel when chat key is pressed
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Use a small frame delay to avoid UI interaction issues
		GetGame().GetCallqueue().Call(OpenChatWrap);
	}
	
	/**
	 * Opens the chat panel if not already open
	 */
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	/**
	 * Handles exit/back action
	 * Opens pause menu instead of directly exiting to prevent accidental exits
	 */
	void Action_Exit()
	{
		// Use a small frame delay to avoid UI interaction issues
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}
	
	/**
	 * Opens the pause menu
	 */
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}
