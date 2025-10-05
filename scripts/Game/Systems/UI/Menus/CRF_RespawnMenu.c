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

		// Start timer update loop
		GetGame().GetCallqueue().CallLater(UpdateTimer, 1000, true);
		
		// Set up Respawn Selection
		InitializeSpawnpointSelection();
		
		// Initialize map if available
		if (m_MapEntity)
			InitializeMap();
		
		// Register input handlers
		RegisterInputHandlers();
		
		// Set up chat panel
		InitializeChatPanel();
		
		// Initializes invoker
		InitializeInvoker()
		
	}
	
	/**
	 * Initializes the map when menu is opened
	 */
	protected void InitializeMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWithConfig, 100);
	}
		
	/**
	 * Initializes invoker for UI updates on respawn point changes
	 */
	protected void InitializeInvoker()
	{
		CRF_RespawnManager.GetInstance().OnRespawnPointStateChanged().Insert(OnSpawnpointStateChanged)
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
		
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		m_factionKey = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerID).GetFactionKey();

		array<RplId> factionRespawnPoints = CRF_RespawnManager.GetInstance().GetFactionSpawnpointsRplIDs(m_factionKey);

		// Populates spawnpoints list with players faction spawns entites and create their markers on the map
		int index = 0;
		foreach(RplId rplID : factionRespawnPoints)
		{ 
			IEntity point = CRF_RespawnManager.GetInstance().GetSpawnEntityFromRplID(rplID);
			if (!point)
				continue;
			
			CRF_RespawnPointComponent respawnPointComponent = CRF_RespawnPointComponent.Cast(point.FindComponent(CRF_RespawnPointComponent));
			if (!respawnPointComponent)
				continue;
			
			vector worldPos = point.GetOrigin();
			
			// Create map marker
			CreateSpawnPointMarker(respawnPointComponent.m_sRespawnPointName, worldPos);
			
			// Add option to menu and store the component with it
			m_wSpawnListBox.AddItem(respawnPointComponent.m_sRespawnPointName);
			
			if (respawnPointComponent.m_bIsDefaultRespawn)
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
	 * @param Rplid of the respawn point
	 * @param state of the respawn point. True will add it. False will remove it 
	 */
	protected void OnSpawnpointStateChanged(RplId rplID, bool active)
	{
		IEntity point = CRF_RespawnManager.GetInstance().GetSpawnEntityFromRplID(rplID);
		if (!point)
			return;
		
		vector worldPos = point.GetOrigin();
		
		CRF_RespawnPointComponent respawnComponent = CRF_RespawnPointComponent.Cast(point.FindComponent(CRF_RespawnPointComponent));
			if (!respawnComponent)
				return;
		
		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		if (!rm)
			return;
		
		// Ignore spawn point if not for player faction
		if (respawnComponent.m_sRespawnPointFaction != m_factionKey)
			return;
		
		if (active)
		{
			// Add the option to respawn selection and add the marker to the map
			m_wSpawnListBox.AddItem(respawnComponent.m_sRespawnPointName);
			CreateSpawnPointMarker(respawnComponent.m_sRespawnPointName, worldPos);
		}
		else
		{
			int index = CRF_RespawnManager.GetInstance().m_RespawnPointsRplID.Find(rplID);
			m_wSpawnListBox.RemoveItem(index);
			RemoveSpawnPointMarker(respawnComponent.m_sRespawnPointName, worldPos);
			
			// if the respawn being changed was confirmed, unconfirm it
			if (rplID == rm.m_SelectedSpawnRplID && rm.m_RespawnConfirmed)
			{
				rm.m_SelectedSpawnRplID = -1;
				ToggleSpawnSelection();
			}
		}
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
	}
	
	/**
	 * Cleans up resources when the menu is closed
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();
		
		// Stop timer updates
		GetGame().GetCallqueue().Remove(UpdateTimer);
		
		// Remove input handlers
		UnregisterInputHandlers();
		
		// Remove Script Invokers
		RemoveScriptInvokers();
		
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
	 * Remove script invokers for spawn points
	 */
	protected void RemoveScriptInvokers()
	{
		CRF_RespawnManager.GetInstance().OnRespawnPointStateChanged().Remove(OnSpawnpointStateChanged);
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
		
		// Grab rplIDs for player faction
		array<RplId> factionRespawnPointsRplIDs = rm.GetFactionSpawnpointsRplIDs(m_factionKey);
		
		// Grab the entity from the rplID
		RplId rplID = factionRespawnPointsRplIDs[m_wSpawnListBox.GetSelectedItem()];
		IEntity point = rm.GetSpawnEntityFromRplID(rplID);
		
		// Pan the map to the spawn point
		PanMapToSpawnPoint(point);
		
		// Update selected respawn used by the server
		rm.m_SelectedSpawnRplID = rplID;
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
		
		CRF_PlayerControllerManager gameModePlayerComponent = CRF_PlayerControllerManager.GetInstance();
				if (!gameModePlayerComponent) 
					return;
		
		// Create marker		
		gameModePlayerComponent.AddScriptedMarker("Static Marker",
		 worldPosFormatted,
		 1,
		 name,
		 "{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds",
		 50,
		 ARGB(255, 0, 0, 225));
		
		// Track marker for deletion later
		m_MapMarkers.Insert(name, worldPos);
		
		// Refresh the markers on the map
		MapMarkersUIRefresh();
	}
	
	/**
	 * Removes the marker on the map for the respawn point
	 * @param Nickname of the respawn point
	 * @param World position of the respawn point
	 */
	protected void RemoveSpawnPointMarker(string name, vector worldPos)
	{
		// Format the string for scripted markers
		string worldPosFormatted = string.Format("%1 %2 %3", worldPos[0], worldPos[1], worldPos[2]);
		
		CRF_PlayerControllerManager gameModePlayerComponent = CRF_PlayerControllerManager.GetInstance();
				if (!gameModePlayerComponent) 
					return;
		
		// Remove marker		
		gameModePlayerComponent.RemoveScriptedMarker("Static Marker",
		worldPosFormatted,
		 1,
		 name,
		 "{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds",
		 50,
		 ARGB(255, 0, 0, 225));
		
		// Untrack marker
		m_MapMarkers.Remove(name);
		
		MapMarkersUIRefresh();
	}	
	
	/**
	 * Removes all the tracked markers
	 */
	protected void RemoveAllSpawnPointMarker()
	{
		foreach(string name, vector worldPos: m_MapMarkers)
		{ 		
			RemoveSpawnPointMarker(name, worldPos);
		}		
	}
	
	/**
	 * Refreshes the markers on the map
	 */
	protected void MapMarkersUIRefresh()
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity) 
			return;

		// Better way of doign this?
		m_MapEntity.CloseMap();
		OpenMapWithConfig();
	}
}