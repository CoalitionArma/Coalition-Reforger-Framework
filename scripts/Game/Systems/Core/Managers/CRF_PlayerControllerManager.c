/*
 * Coalition Reforger Framework (CRF) Player Controller Component
 * 
 * This component manages player-specific functionality including:
 * - Camera and spectator controls
 * - UI and menu management
 * - Game state handling
 * - Admin commands and messaging
 * - Radio setup and audio controls
 * - Map markers and gameplay indicators
 */
[ComponentEditorProps(category: "Player Controller Components", description: "")]
class CRF_PlayerControllerManagerClass : ScriptComponentClass
{

}

class CRF_PlayerControllerManager : ScriptComponent
{
	// UI and Display
	string m_sHintText = "Type Here";      // Text displayed for hints to player
	bool m_bHUDVisible = true;             // Controls visibility of HUD elements
	Widget m_wSavedHintWidget;             // Reference to hint widget for reuse
	
	// Camera and Spectator
	IEntity m_eCamera;                      // Stores local camera entity for spectator mode
	protected vector m_vStoredCameraPos[4];   // Stores camera transform between sessions
	
	// Game Performance Settings
	protected int m_iFPS = -1;              // Stored user FPS setting (-1 means uninitialized)
	protected int m_iAudioSetting = -1;     // Stored audio volume (-1 means uninitialized)
	
	// Game Systems
	protected CRF_Gamemode m_Gamemode;                      // Reference to the active gamemode
	protected CRF_GamemodeManager m_GamemodeManager;        // Reference to the gamemode manager
	protected CRF_SlottingManager m_SlottingManager;		 // Reference to the slotting manager
	protected CRF_RplToAuthorityManager m_RplToAuthorityManager;  // Network authority manager
	
	// Map and Markers
	ref array<string> m_aScriptedMarkers = {};  // Custom map markers
	
	protected static CRF_PlayerControllerManager m_sInstance;

	//------------------------------------------------------------------------------------------------
	// STATIC ACCESSORS
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the instance of this component from the player controller
	 * @return CRF_PlayerControllerManager - The player controller component instance or null if unavailable
	 */
	
	void CRF_PlayerControllerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_PlayerControllerManager GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	// INITIALIZATION
	//------------------------------------------------------------------------------------------------

	/**
	 * Initializes the player controller component
	 * Sets up input listeners and schedules initial setup calls
	 */
	void InitilizePlayerControllerComp()
	{
		// Skip initialization on dedicated servers or in editor
		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Get references to required systems
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();

		// Register input action handlers
		GetGame().GetInputManager().AddActionListener("CRF_ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
		GetGame().GetInputManager().AddActionListener("CRF_AdminForceReady", EActionTrigger.DOWN, AdminForceReady);
		GetGame().GetInputManager().AddActionListener("CRF_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
		
		GetGame().GetCallqueue().Call(AddMsgAction);
		GetGame().GetCallqueue().Call(InitFPSLock);
		GetGame().GetCallqueue().Call(InitAudioLock);
		GetGame().GetCallqueue().Call(OpenCurrentStateMenu);
	}

	/**
	 * Initializes the player client
	 * Cleans up previous camera, closes menus, and sets up player-specific settings
	 * @param playerCharacter - The spectator entity the server created and set to this player
	 */
	void InitilizePlayerClient(RplId playerCharID)
	{
		// Get player character
		IEntity playerCharacter = m_SlottingManager.GetCharacterFromRplId(playerCharID);
		
		// if we cant get the player character or it's null, wait another full initilization time before attempting again
		if (!playerCharacter || !SCR_ChimeraCharacter.Cast(playerCharacter))
		{
			// Schedule another verification attempt
			GetGame().GetCallqueue().CallLater(InitilizePlayerClient, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME, false, playerCharID);
			return;
		};
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
		
		// Close all menus
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			GetGame().GetMenuManager().CloseAllMenus();
			ResetSettingsToStoredValues();
			if (!CVON_VONGameModeComponent.GetInstance())
				SetupRadioFrequency();
		}; 
		
		if (playerCharacter.GetPrefabData().GetPrefabName() == CRF_GamemodeManager.GetSpectatorResource())
			InitilizeLocalSpectator(playerCharacter);
		else
			InitilizeLocalCharacter();
	}
	
	/**
	 * Initilizes players if they have a valid spectator entity
	 * @param playerCharacter - The spectator entity the server created and set to this player
	 */
	void InitilizeLocalSpectator(IEntity playerCharacter)
	{
		vector cameraPos[4];
		cameraPos = SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_vPlayersLastDeath;
		
		//If Respawns are enabled, everybody goes to the debug zone
		if (CRF_RespawnManager.GetInstance().m_bCurrentRespawnEnabled)
			cameraPos[3] = CRF_PlayableCharacter.GenerateRandomSpreadPosition("0 10000 0", 500.0);
		// Use provided death position if available
		else if (CRF_GamemodeManager.IsValidSpawnVector(cameraPos[3])) {
			cameraPos[3][1] = cameraPos[3][1] + 1.5; // Elevate camera slightly above death position
		}
		// Use stored camera position if available
		else if (CRF_GamemodeManager.IsValidSpawnVector(m_vStoredCameraPos[3])) {
			cameraPos = m_vStoredCameraPos;
		} 
		// Fallback to generic spawn position
		else {
			cameraPos = m_Gamemode.m_vGenericSpawn;
		}
			
		// Set up camera entity
		EntitySpawnParams cameraSpawnParams = new EntitySpawnParams();
		cameraSpawnParams.TransformMode = ETransformMode.WORLD;
		cameraSpawnParams.Transform = cameraPos;

		// Spawn or reposition camera
		if (!m_eCamera)
			m_eCamera = GetGame().SpawnEntityPrefab(Resource.Load("{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et"), GetGame().GetWorld(), cameraSpawnParams);
		else
			m_eCamera.SetWorldTransform(cameraPos);
		
		// Level camera horizon
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));

		// Register for VON (voice chat)
		m_RplToAuthorityManager.CheckVONRegister(SCR_PlayerController.GetLocalPlayerId());
		
		// Open spectator menu if in game state
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SpectatorMenu);
		
		// Switch to spectator camera
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
		
		// Turn on killfeed for specs
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeDeadLocal();
	}
	
	/**
	 * Initilizes players if they have a valid slotted character
	 */
	void InitilizeLocalCharacter()
	{
		// Clean up previous camera if exists
		if (m_eCamera)
			delete m_eCamera;
		
		// Originally added for data collector
		m_Gamemode.GetOnPlayerSpawned().Invoke(SCR_PlayerController.GetLocalPlayerId(), SCR_PlayerController.GetLocalMainEntity());
		
		// Reset Stored Pos
		GetGame().GetCallqueue().CallLater(UpdateStoredCameraPos, 200, false, vector.Zero, vector.Zero, vector.Zero, vector.Zero);
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();
	}
	
	//------------------------------------------------------------------------------------------------
	// HUD AND CAMERA CONTROLS
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Toggles the visibility of the HUD
	 */
	void UpdateHUDVisible()
	{
		m_bHUDVisible = !m_bHUDVisible;
	}
	
	/**
	 * Updates stored camera position for persistence between sessions
	 * @param cameraPosToStore - Array of 4 vectors representing camera transform
	 */
	void UpdateStoredCameraPos(vector cameraPosToStoreOne, vector cameraPosToStoreTwo, vector cameraPosToStoreThree, vector cameraPosToStoreFour)
	{
		m_vStoredCameraPos[0] = cameraPosToStoreOne;
		m_vStoredCameraPos[1] = cameraPosToStoreTwo;
		m_vStoredCameraPos[2] = cameraPosToStoreThree;
		m_vStoredCameraPos[3] = cameraPosToStoreFour;
	}

	/**
	 * Updates entity position and resets physics
	 * @param cameraPos - New position/transform
	 */
	void UpdateEntityPos(vector cameraPos[4])
	{
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();

		// Align to terrain if not a character
		if (!ChimeraCharacter.Cast(player))
			SCR_TerrainHelper.OrientToTerrain(cameraPos);

		// Teleport or transform entity
		BaseGameEntity baseGameEntity = BaseGameEntity.Cast(player);
		if (baseGameEntity)
			baseGameEntity.Teleport(cameraPos);
		else
			player.SetWorldTransform(cameraPos);

		// Reset physics to prevent unwanted movement
		Physics phys = player.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// PLAYER EQUIPMENT
	//------------------------------------------------------------------------------------------------

	/**
	 * Sets up radio frequencies based on player group
	 * Configures both group and platoon frequencies
	 */
	void SetupRadioFrequency()
	{
		// Get player's entity
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		if (!entity || CRF_GamemodeManager.IsSpectator(entity))
			return;

		// Find radio in inventory
		array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		IEntity radioEntity;
		foreach (IEntity item : items)
		{
			if (item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return;

		// Get radio components
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		BaseTransceiver grpTsv = radio.GetTransceiver(0);

		// Get player's group
		SCR_GroupsManagerComponent m_GroupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!m_GroupManager)
			return;

		SCR_AIGroup group = m_GroupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		PlayerController pc = GetGame().GetPlayerController();

		// Set frequency based on group
		if (pc && group)
		{
			RadioHandlerComponent rhc = RadioHandlerComponent.Cast(pc.FindComponent(RadioHandlerComponent));
			if (rhc)
				rhc.SetFrequency(grpTsv, group.GetRadioFrequency());
		}

		// Set up Voice over Network component
		SCR_VONController vc = SCR_VONController.Cast(pc.FindComponent(SCR_VONController));
		SCR_VoNComponent von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));

		von.SetTransmitRadio(grpTsv);

		// Set up platoon radio if available
		BaseTransceiver pltTsv = radio.GetTransceiver(1);
		if (pltTsv)
			von.SetTransmitRadio(pltTsv);

		vc.PublicResetVON();
		vc.SetVONComponent(von);
	}
	
	//------------------------------------------------------------------------------------------------
	// GAME SETTINGS MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Initializes audio lock by storing current volume and setting to 0
	 */
	void InitAudioLock()
	{
		m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		SetSFXVolume(0);
	}
	
	/**
	 * Sets SFX volume to specified level
	 * @param volume - Volume level to set
	 */
	void SetSFXVolume(int volume)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, volume);
	}
	
	/**
	 * Sets FPS limit to specified value
	 * @param video - Video settings container
	 * @param fps - FPS limit to set
	 */
	void SetFPS(BaseContainer video, int fps)
	{
		video.Set("MaxFps", fps);
		GetGame().UserSettingsChanged();
	}
	
	/**
	 * Retrieves and stores initial user FPS setting
	 * @param video - Video settings container
	 */
	void GetInitialUserFPSValue(BaseContainer video)
	{
		video.Get("MaxFps", m_iFPS);
	}
	
	/**
	 * Initializes FPS lock by storing current value and setting to 30
	 */
	void InitFPSLock()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		//GetInitialUserFPSValue(video);
		SetFPS(video, 30);
	}
	
	/**
	 * Restores user settings to original values
	 */
	void ResetSettingsToStoredValues()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		
		// Restore FPS if initialized
		SetFPS(video, 0);
		
		// Restore audio if initialized
		SetSFXVolume(100);
	}
	
	//------------------------------------------------------------------------------------------------
	// MENU MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Opens appropriate menu based on current gamemode state
	 */
	void OpenCurrentStateMenu()
	{	
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		// Close any existing menus
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseAllMenus();
		
		// Open appropriate menu based on gamemode state
		switch (m_Gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.BRIEFING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
				break;
			}
			case CRF_EGamemodeState.SLOTTING:
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
				break;
			}
			case CRF_EGamemodeState.GAME: 
			{
				m_RplToAuthorityManager.RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
				break;
			}
			case CRF_EGamemodeState.AAR: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_AARMenu);
				break;
			}
		}
	}
	
	/**
	 * Opens the slotting menu for player assignment
	 * @param value - Input value (1.0 for pressed)
	 * @param reason - Trigger reason
	 */
	void OpenSlottingMenu(float value = 0.0, EActionTrigger reason = 0)
	{
		if (value != 1)
			return;

		// Check if appropriate menu is already open
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
		{
			if (topMenu.IsInherited(CRF_PreviewMenu) || topMenu.IsInherited(CRF_SlottingMenu))
				return;
			else if (topMenu.IsInherited(CRF_SpectatorMenu))
				GetGame().GetMenuManager().CloseMenu(topMenu);
		}

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
	}	

	//------------------------------------------------------------------------------------------------
	// ADMIN MESSAGING SYSTEM
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Registers chat commands for admin messaging
	 */
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("admin");
		invoker.Insert(SendAdminMessage);
		
		ChatCommandInvoker invoker2 = chatPanelManager.GetCommandInvoker("a");
		invoker2.Insert(SendAdminMessage);
		
		ChatCommandInvoker invoker3 = chatPanelManager.GetCommandInvoker("r");
		invoker3.Insert(ReplyAdminMessage);
		
		ChatCommandInvoker invoker4 = chatPanelManager.GetCommandInvoker("reply");
		invoker4.Insert(ReplyAdminMessage);
		
		ChatCommandInvoker invoker5 = chatPanelManager.GetCommandInvoker("aar");
		invoker5.Insert(Advance_Callback);
	}
	
	/**
	 * Sends an admin message from the player to server
	 * @param panel - Chat panel
	 * @param data - Message content
	 */
	void SendAdminMessage(SCR_ChatPanel panel, string data)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		if (!data.Length() > 0)
		{
			chatComponent.ShowMessage("You need to include your message after /a");
			return;
		}	
		
		chatComponent.ShowMessage(string.Format("Message Sent: \"%1\"", data));
		m_RplToAuthorityManager.SendAdminMessage(data, playerID);
	}

	/**
	 * Allows admins to reply to specific players
	 * @param panel - Chat panel
	 * @param data - Message including player ID and content
	 */
	void ReplyAdminMessage(SCR_ChatPanel panel, string data)
	{
		// Verify sender is admin or moderator
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
		// Parse player ID and message from input
		array<string> dataSplit = {};
		data.Split(" ", dataSplit, false);
		int playerId;
		string toSend;
		
		for (int i = 0; i < dataSplit.Count(); i++)
		{
			if (dataSplit[i] == "0")
			{
				dataSplit.RemoveOrdered(i);
				playerId = 0;
				toSend = SCR_StringHelper.Join(" ", dataSplit, true);
				break;
			}

			if (dataSplit[i].ToInt() > 0)
			{
				playerId = dataSplit[i].ToInt();
				dataSplit.RemoveOrdered(i);
				toSend = SCR_StringHelper.Join(" ", dataSplit, true);
				break;
			}
		}
		
		// Get chat component
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		// Validate player ID
		if (!playerId)
		{
			chatComponent.ShowMessage("INVALID PLAYER ID");
			return;
		}
		
		if (!GetGame().GetPlayerManager().GetPlayerName(playerId))
		{
			chatComponent.ShowMessage("INVALID PLAYER ID");
			return;
		}
		
		// Get the ID of the admin replying to the ticket
		int adminID = SCR_PlayerController.GetLocalPlayerId();

		// Send message
		chatComponent.ShowMessage(string.Format("Message Sent to %2: \"%1\"", 
			toSend, 
			GetGame().GetPlayerManager().GetPlayerName(playerId)));
		
		toSend = string.Format("\"%1\"", toSend);
		m_RplToAuthorityManager.ReplyAdminMessage(toSend, playerId, adminID, true);
	}

	//------------------------------------------------------------------------------------------------
	// GAMEMODE CONTROLS
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Toggles ready state for player's side/faction
	 * Only faction leaders can toggle ready state
	 */
	void ToggleSideReady()
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
			
			m_RplToAuthorityManager.ToggleSideReady(faction.GetFactionKey(), playerName, false);
		}
	}

	/**
	 * Admin command to force ready state for all sides
	 */
	void AdminForceReady()
	{
		if (!SCR_Global.IsAdmin())
			return;
		
		m_RplToAuthorityManager.ToggleSideReady("", 
			GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId()), 
			true);
	}
	
	/**
	 * Teleports a player to another player's location
	 * @param playerId1 - Player to teleport
	 * @param playerId2 - Destination player
	 */
	void TeleportLocalPlayer(int playerId1, int playerId2)
	{
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 10);
		spawnParams.Transform[3] = teleportLocation;

		SCR_Global.TeleportPlayer(playerId1, teleportLocation);
	}
	
	/**
	 * Callback for advancing gamemode state with optional faction winner parameter
	 * Usage: /aar [faction]
	 * Examples: /aar, /aar blufor, /aar blu, /aar opfor, /aar opf, /aar indfor, /aar ind, /aar civ
	 */
	void Advance_Callback(SCR_ChatPanel panel, string data)
	{
		// Check if admin privileges are required
		if (!SCR_Global.IsAdmin())
		{
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("You need admin privileges to use the /aar command.");
			}
			return;
		}
		
		// Parse faction parameter if provided
		if (data && data.Length() > 0)
		{
			// Clean up the input - remove extra spaces and convert to uppercase
			data.Trim();
			data.ToUpper();
			
			// Map short faction names to full names
			FactionKey winningFaction = "";
			switch (data)
			{
				case "BLUFOR":
				case "BLU":
				case "B":
				case "BLUE":
					winningFaction = "BLUFOR";
					break;
					
				case "OPFOR":
				case "OPF":
				case "O":
				case "RED":
					winningFaction = "OPFOR";
					break;
					
				case "INDFOR":
				case "IND":
				case "I":
				case "INDEPENDENT":
				case "GREEN":
					winningFaction = "INDFOR";
					break;
					
				case "CIV":
				case "C":
				case "CIVILIAN":
					winningFaction = "CIV";
					break;
					
				default:
					// Invalid faction specified
					if (panel)
					{
						SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
						if (chatComponent)
						{
							string validOptions = "Valid faction options: blufor (blu), opfor (opf), indfor (ind), civ";
							chatComponent.ShowMessage(string.Format("Invalid faction '%1'. %2", data, validOptions));
						}
					}
					return;
			}
			
			// Set the winning faction in the logging manager
			CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
			if (loggingManager)
			{
				loggingManager.SetWinningFaction(winningFaction, "manual");
				
				// Show confirmation message
				if (panel)
				{
					SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
					if (chatComponent)
						chatComponent.ShowMessage(string.Format("Winner set to %1. Advancing to AAR...", winningFaction));
				}
			}

			// Advance to AAR state
			m_RplToAuthorityManager.RequestAdvanceGamemodeState(true);
		}
		else
		{
			// No faction specified - show current usage
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("Usage: /aar [faction] - Examples: /aar blufor, /aar opfor, /aar indfor, /aar civ");
					
			}
			return;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// MAP MARKER SYSTEM
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the array of scripted markers
	 * @return TStringArray - Array of scripted marker data strings
	 */
	TStringArray GetScriptedMarkersArray()
	{
		return m_aScriptedMarkers;
	}

	/**
	 * Adds a scripted marker on the user's map
	 * @param markerEntityName - Name of entity to track (or "Static Marker" for static position)
	 * @param markerOffset - Position offset from entity or static position
	 * @param timeDelay - Update frequency for entity tracking
	 * @param markerText - Text displayed on map
	 * @param markerImage - Image resource path
	 * @param zOrder - Display order/priority
	 * @param markerColor - ARGB color value
	 */
	void AddScriptedMarker(string markerEntityName, string markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset, 
			timeDelay.ToString(), 
			markerText, 
			markerImage, 
			zOrder.ToString(), 
			markerColor.ToString()));
	}

	/**
	 * Removes a specific scripted marker
	 * @param markerEntityName - Name of entity tracked
	 * @param markerOffset - Position offset
	 * @param timeDelay - Update frequency
	 * @param markerText - Text displayed
	 * @param markerImage - Image resource path
	 * @param zOrder - Display order/priority
	 * @param markerColor - ARGB color value
	 */
	void RemoveScriptedMarker(string markerEntityName, string markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.RemoveItemOrdered(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset, 
			timeDelay.ToString(), 
			markerText, 
			markerImage, 
			zOrder.ToString(), 
			markerColor.ToString()));
	}

	/**
	 * Removes all scripted markers
	 */
	void RemoveALLScriptedMarkers()
	{
		m_aScriptedMarkers.Clear();
	}
	
	void UpdateMapMarkers(array<string> zoneStatus, array<string> zoneObjectNames, FactionKey bluforSide, FactionKey opforSide)
	{
		RemoveALLScriptedMarkers();

		foreach (int i, string zoneName : zoneObjectNames)
		{
			string status = zoneStatus[i];
			string imageTexture;
			int imageColor;

			// Parse zone status
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);

			string zoneLocked = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];

			// Select image based on zone index
			switch (i)
			{
				case 0: {imageTexture = "{21A2A457BD0E42C1}UI\Objectives\A.edds"; break; };
				case 1: {imageTexture = "{7F4A8D140283CCCE}UI\Objectives\B.edds"; break; };
				case 2: {imageTexture = "{8B42CA8C0F5EA4BA}UI\Objectives\C.edds"; break; };
				case 3: {imageTexture = "{C29ADF937D98D0D0}UI\Objectives\D.edds"; break; };
				case 4: {imageTexture = "{3692980B7045B8A4}UI\Objectives\E.edds"; break; };
			}

			// Add lock marker if zone is locked
			if (zoneLocked == "Locked")
				AddScriptedMarker(zoneName, "0 0 0", 0, "", "{91427B7866707601}UI\Objectives\lock.edds", 50, ARGB(255, 142, 142, 142));

			// Set color based on controlling faction
			switch (zoneFactionStored)
			{
				case bluforSide: {imageColor = ARGB(255, 0, 25, 225); break; }; // Blufor
				case opforSide: {imageColor = ARGB(255, 225, 25, 0); break; }; // Opfor
				default: {imageColor = ARGB(255, 225, 225, 225); break; }; // Uncaptured
			}

			// Add zone marker
			AddScriptedMarker(zoneName, "0 0 0", 0, "", imageTexture, 45, imageColor);
		}
	}
}
