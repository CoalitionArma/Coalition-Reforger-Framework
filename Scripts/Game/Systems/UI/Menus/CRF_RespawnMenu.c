/**
 * Custom respawn menu implementation for the Coalition Reforger Framework
 * Provides timer display, map view, and communication capabilities during respawn
 */
class CRF_RespawnMenu: ChimeraMenuBase
{
	// UI components
	protected Widget m_wRoot;
	protected SCR_MapEntity m_MapEntity;
	protected SCR_ChatPanel m_ChatPanel;
	protected OverlayWidget m_wSpawnListRoot;
	protected SCR_ListBoxComponent m_wSpawnListBox;
	protected SCR_ButtonTextComponent m_bConfirmSpawnButton;
	protected ref array<CRF_SpawnPointData> m_aSpawnPoints = {};
	protected ref map<string, vector> m_MapMarkers = new map<string, vector>;
	protected FactionKey m_factionKey;
	
	/**
	 * Updates the respawn timer display on the UI
	 */
	void UpdateTimer()
	{	
		TextWidget timerWidget = TextWidget.Cast(GetRootWidget().FindAnyWidget("timerCountDown"));
		if (!timerWidget)
			return;
			
		timerWidget.SetText(SCR_FormatHelper.FormatTime((int)CRF_RespawnManager.GetInstance().m_fRespawnTimer));
	}
	
	/**
	 * Handles actions when menu is opened
	 * Sets up timer, map, voice chat and input listeners
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		CRF_RespawnManager.GetInstance().GetOnSpawnPointsUpdated().Insert(OnSpawnpointStateChanged);
		
		// Set up Respawn Selection
		InitializeSpawnpointSelection();
		
		// Initialize map if available
		if (m_MapEntity)
			InitializeMap();
		
		// Register input handlers
		RegisterInputHandlers();
		
		// Set up chat panel
		InitializeChatPanel();
	}
	
	/**
	 * Initializes the map when menu is opened
	 */
	protected void InitializeMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWithConfig, 100);
	}
	
	/**
	 * Populate the respawn selection with avaible spawn point to the player
	 */
	protected void InitializeSpawnpointSelection()
	{
		// Setup UI elements 
		m_wSpawnListRoot = OverlayWidget.Cast(GetRootWidget().FindAnyWidget("List1Box"));
		m_wSpawnListBox = SCR_ListBoxComponent.Cast(m_wSpawnListRoot.FindHandler(SCR_ListBoxComponent));
		m_bConfirmSpawnButton = SCR_ButtonTextComponent.GetButtonText("ActionButton", GetRootWidget());
		
		// Setup list update event handlers 
		m_wSpawnListBox.m_OnChanged.Insert(UpdateSpawnSelection);
		
		// Setup button event handlers
		m_bConfirmSpawnButton.m_OnClicked.Insert(ToggleSpawnSelection);
		PopulateListBox();
	}
	
	protected void PopulateListBox()
	{
		m_aSpawnPoints.Clear();
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		m_factionKey = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerID).GetFactionKey();

		array<CRF_SpawnPointData> factionRespawnPoints = respawnManager.GetFactionSpawnpoints(m_factionKey);
		
		// Populates spawnpoints list with players faction spawns entites and create their markers on the map
		int index = 0;
		foreach(CRF_SpawnPointData spawnPointData : factionRespawnPoints)
		{ 
			IEntity point = CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
			if (!point || !spawnPointData.GetIsActiveSpawnPoint())
				continue;
			
			m_aSpawnPoints.Insert(spawnPointData);
			
			vector worldPos = point.GetOrigin();
			
			// Create map marker
			CreateSpawnPointMarker(spawnPointData.GetSpawnPointName(), worldPos);
			
			// Add option to menu and store the component with it
			m_wSpawnListBox.AddItem(spawnPointData.GetSpawnPointName());
			
			if (index == 0)
			{
				m_wSpawnListBox.SetItemSelected(index, true, true, true);
				GetGame().GetCallqueue().CallLater(UpdateSpawnSelection, 500, false);
			}
			index++;
		}
	}
	
	/**
	 * Registers all input action listeners for the menu
	 */
	protected void RegisterInputHandlers()
	{
		InputManager inputManager = GetGame().GetInputManager();		
		// Menu navigation handlers
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}	
	
	/**
	 * Update options on menu depending on state of respawn point
	 */
	protected void OnSpawnpointStateChanged()
	{	
		RemoveAllSpawnPointMarker();
		m_wSpawnListBox.Clear();
		
		GetGame().GetCallqueue().Call(PopulateListBox);
	}
	
	/**
	 * Initializes the chat panel component
	 */
	protected void InitializeChatPanel()
	{
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
	}
	
	/**
	 * Initializes menu components when the menu is first created
	 */
	override void OnMenuInit()
	{	
		super.OnMenuInit();

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}
	
	/**
	 * Updates the menu state during each frame
	 * @param tDelta - Time elapsed since last frame
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
		
		UpdateTimer();
	}
	
	/**
	 * Cleans up resources when the menu is closed
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		CRF_RespawnManager.GetInstance().GetOnSpawnPointsUpdated().Remove(OnSpawnpointStateChanged);
		
		// Remove input handlers
		UnregisterInputHandlers();
		
		// Remove Map Markers
		RemoveAllSpawnPointMarker();
		
		// Close the map if open
		if (m_MapEntity)
			m_MapEntity.CloseMap();
		
		// Untrack all markers
		m_MapMarkers.Clear();
	}
	
	/**
	 * Unregisters all input action listeners when closing the menu
	 */
	protected void UnregisterInputHandlers()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			inputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			inputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}	
	
	/**
	 * Configures and opens the map with proper settings
	 */
	protected void OpenMapWithConfig()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		
		// Apply zoom out after short delay to ensure map is ready
		GetGame().GetCallqueue().CallLater(AdjustMapZoom, 100);
	}
	
	/**
	 * Adjusts map zoom level after map is opened
	 */
	protected void AdjustMapZoom()
	{
		if (m_MapEntity)
			m_MapEntity.ZoomOut();
	}
	
	/**
	 * Activates voice communication when key is pressed
	 */
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);

		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		if (!von)
			return;

		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}

	/**
	 * Retrieves the player's radio transceiver for voice communication
	 * @return Radio transceiver entity
	 */
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		if (!entity)
			return null;

		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(
			entity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (!inventoryManager)
			return null;

		ref array<IEntity> items = {};
		inventoryManager.GetItems(items);

		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if(item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return null;

		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);

		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);

		return transceiver;
	}

	/**
	 * Deactivates voice communication when key is released
	 */
	void Action_VONOff()
	{
		// Slight delay to avoid cutting off voice too abruptly
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}

	/**
	 * Delayed function to disable voice communication
	 */
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		if (!von)
			return;

		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	/**
	 * Toggles the chat panel when chat key is pressed
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Short delay to prevent conflicts with other UI actions
		GetGame().GetCallqueue().CallLater(OpenChatPanel, 5);
	}
	
	/**
	 * Opens the chat panel if it's currently closed
	 */
	protected void OpenChatPanel()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
			if (chatManager)
				chatManager.OpenChatPanel(m_ChatPanel);
		}
	}
	
	/**
	 * Handles the exit/back action by opening the pause menu
	 */
	void Action_Exit()
	{
		// Open pause menu instead of exiting directly to prevent accidental exits
		GetGame().GetCallqueue().CallLater(OpenPauseMenu, 0);
	}
	
	/**
	 * Opens the game's pause menu
	 */
	protected void OpenPauseMenu()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
	
	/**
	 * Store selected respawn point in respawn manager
	 */
	protected void UpdateSpawnSelection()
	{	
		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		if (!rm)
			return;
		
		// Grab the entity from the rplID
		CRF_SpawnPointData spawnPointData = m_aSpawnPoints[m_wSpawnListBox.GetSelectedItem()];
		IEntity point = CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
		
		// Pan the map to the spawn point
		PanMapToSpawnPoint(point);
		
		// Update selected respawn used by the server
		rm.m_SelectedSpawnPoint = spawnPointData;
	}
	
	/**
	 * Pans the map to a entity
	 * @param Entity to pan too
	 */
	protected void PanMapToSpawnPoint(IEntity spawnpoint)
	{
		if (spawnpoint)
		{	
			SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
            vector worldPos = spawnpoint.GetOrigin();

			SCR_MapEntity.GetMapInstance().ZoomPanSmooth(0.5, worldPos[0], worldPos[2]); 
		}
	}
	
	/**
	 * Update the spawn selection button in the UI and set it state for use in the respawn timer
	 */
	protected void ToggleSpawnSelection()
	{
		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		if (!rm)
			return;
		
		if (rm.m_RespawnConfirmed)
		{
			TextWidget.Cast(m_bConfirmSpawnButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Select Spawn");
			rm.m_RespawnConfirmed = false;
		}
		else
		{
			TextWidget.Cast(m_bConfirmSpawnButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Cancel Selection");
			rm.m_RespawnConfirmed = true;
		}
	}
	
	/**
	 * Creates a marker on the map for the respawn point
	 * @param Nickname of the respawn point
	 * @param World position of the respawn point
	 */
	protected void CreateSpawnPointMarker(string name, vector worldPos)
	{
		// Format the string for scripted markers
		string worldPosFormatted = string.Format("%1 %2 %3", worldPos[0], worldPos[1], worldPos[2]);
		
		CRF_PlayerScriptedMarkerManager playerScriptedMarkerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
				if (!playerScriptedMarkerManager) 
					return;
		
		// Create marker		
		playerScriptedMarkerManager.AddScriptedMarker("Static Marker",
		 worldPosFormatted,
		 1,
		 name,
		 "{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds",
		 50,
		 ARGB(255, 0, 0, 225));
		
		// Track marker for deletion later
		m_MapMarkers.Insert(name, worldPos);
	}
	
	/**
	 * Removes all the tracked markers without triggering a map refresh (used on close).
	 */
	protected void RemoveAllSpawnPointMarker()
	{
		CRF_PlayerScriptedMarkerManager psm = CRF_PlayerScriptedMarkerManager.GetInstance();
		foreach (string name, vector worldPos : m_MapMarkers)
		{
			if (!psm)
				break;
			string worldPosFormatted = string.Format("%1 %2 %3", worldPos[0], worldPos[1], worldPos[2]);
			psm.RemoveScriptedMarker("Static Marker",
				worldPosFormatted, 1, name,
				"{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds",
				50, ARGB(255, 0, 0, 225));
		}
		m_MapMarkers.Clear();
	}
}