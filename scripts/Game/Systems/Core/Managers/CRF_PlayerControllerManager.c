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
	bool m_bActivated = false;              // NVG activation state for spectator
	protected vector m_vStoredCameraPos[4];   // Stores camera transform between sessions
	
	// Game Performance Settings
	protected int m_iFPS = -1;              // Stored user FPS setting (-1 means uninitialized)
	protected int m_iAudioSetting = -1;     // Stored audio volume (-1 means uninitialized)
	
	// Game Systems
	protected CRF_Gamemode m_Gamemode;                      // Reference to the active gamemode
	protected CRF_GamemodeManager m_GamemodeManager;        // Reference to the gamemode manager
	protected CRF_RplToAuthorityManager m_RplToAuthorityManager;  // Network authority manager
	
	// Map and Markers
	ref array<string> m_aScriptedMarkers = {};  // Custom map markers

	//------------------------------------------------------------------------------------------------
	// STATIC ACCESSORS
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the instance of this component from the player controller
	 * @return CRF_PlayerControllerManager - The player controller component instance or null if unavailable
	 */
	static CRF_PlayerControllerManager GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_PlayerControllerManager.Cast(GetGame().GetPlayerController().FindComponent(CRF_PlayerControllerManager));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	// INITIALIZATION AUTHORITY
	//------------------------------------------------------------------------------------------------
	
	/**
	 * (ONLY ON THE AUTHORITY) Initializes a looped check if the local player character has all gear from the GS manager
	 * @param playerCharacter - Player character to pass along to InitilizePlayerCharacter from the gamemode manager
	 * @param faction - Faction to pass alongto InitilizePlayerCharacter from the gamemode manager
	 */
	void InitilizePlayerCharacterCheck(SCR_ChimeraCharacter playerCharacter, Faction faction)
	{
		// Get the playable character comp from our entity so we can check if the gearscript has completed
		CRF_PlayableCharacter playable = CRF_PlayableCharacter.Cast(playerCharacter.FindComponent(CRF_PlayableCharacter));
		
		// Check if we are already in a loop check, if we arent, start a loop check
		GetGame().GetCallqueue().CallLater(PlayerCharacterCheck, 50, true, playable, playerCharacter, faction);
	};
	
	/**
	 * (ONLY ON THE AUTHORITY) Loop that checks if the local player character has all gear from the GS manager
	 * @param playerCharacter - Player character to pass along to InitilizePlayerCharacter from the gamemode manager
	 * @param faction - Faction to pass along to InitilizePlayerCharacter from the gamemode manager
	 */
	protected void PlayerCharacterCheck(CRF_PlayableCharacter playable, SCR_ChimeraCharacter playerCharacter, Faction faction)
	{
		// If gearscript hasnt been completed, dont initilize the entity
		if (!playable.GetGearscriptCompleted())
			return;
		
		// Initilize playable entity
		GetGame().GetCallqueue().CallLater(InitilizePlayerCharacter, 325, false, playerCharacter, faction, false);
		// Kill the loop
		GetGame().GetCallqueue().Remove(PlayerCharacterCheck);
	};
	
	/**
	 * (ONLY ON THE AUTHORITY) Initilize the player character for the local player controller
	 * @param playerCharacter - Player character that we are initilizing
	 * @param faction - Faction to assign to the playercontroller
	 */
	void InitilizePlayerCharacter(SCR_ChimeraCharacter playerCharacter, Faction faction)
	{	
		// Get the local player controller
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetOwner());
		
		// Get the local player id from the controller
		int playerId = playerController.GetPlayerId();
		
		// Check if the entity we are setting is a spectator
		bool isSpectator = CRF_GamemodeManager.IsSpectator(playerCharacter);
		
		CRF_GamemodeManager.AssignFactionToPlayer(playerController, faction);
		CRF_GamemodeManager.AssignCharacterToPlayer(playerController, playerCharacter);

		if (!isSpectator)
		{
			CRF_GamemodeManager.GetInstance().AssignPlayerToGroup(playerId);
		}
		
		CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast(playerId, isSpectator);
	}

	//------------------------------------------------------------------------------------------------
	// INITIALIZATION CLIENT
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
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();

		// Register input action handlers
		GetGame().GetInputManager().AddActionListener("CRF_ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
		GetGame().GetInputManager().AddActionListener("CRF_AdminForceReady", EActionTrigger.DOWN, AdminForceReady);
		GetGame().GetInputManager().AddActionListener("CRF_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("CRF_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
		
		// Schedule delayed initialization
		GetGame().GetCallqueue().CallLater(AddMsgAction, 1000, false);
		GetGame().GetCallqueue().CallLater(InitFPSLock, 100, false);
		GetGame().GetCallqueue().CallLater(InitAudioLock, 100, false);
		GetGame().GetCallqueue().CallLater(OpenCurrentStateMenu, 500, false);
	}

	/**
	 * Initializes the player client
	 * Cleans up previous camera, closes menus, and sets up player-specific settings
	 * @param IsSpectator - If we should initilize the Spec camera and menu
	 */
	void InitilizePlayerClient(bool IsSpectator = false)
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
		
		// Close all menus
		if(m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			GetGame().GetMenuManager().CloseAllMenus();
		
			// Schedule delayed initialization of player-specific settings
			GetGame().GetCallqueue().CallLater(ResetSettingsToStoredValues, 100, false);
		};
		
		if (IsSpectator)
		{	
			// Set up camera initilal position
			vector cameraPos[4];
			SCR_ChimeraCharacter char = CRF_SlottingManager.GetInstance().GetPlayerSlotCharacter(SCR_PlayerController.GetLocalPlayerId());
			
			if (char && m_vStoredCameraPos[3] == vector.Zero)
			{
				char.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			} else if (m_vStoredCameraPos[3] != vector.Zero) {
				cameraPos = m_vStoredCameraPos;
			} else {
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
		} else { 
			// Clean up previous camera if exists
			if (m_eCamera)
				delete m_eCamera;
			
			// Reset Stored Pos
			GetGame().GetCallqueue().CallLater(UpdateStoredCameraPos, 1275, false, vector.Zero, vector.Zero, vector.Zero, vector.Zero);
		};
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
	
	/**
	 * Toggles night vision goggles in spectator mode
	 */
	void ToggleNVGs()
	{
		m_bActivated = !m_bActivated;

		if (m_bActivated)
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{0AD0A1ADEBCF893F}Assets/Items/Equipment/NVG/pvs14/data/SpecNVGFilm.emat", true);
		else
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);
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
		
		if(!SCR_PlayerController.GetLocalMainEntity())
			m_RplToAuthorityManager.RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
		
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
			if (topMenu.IsInherited(CRF_PreviewMenuUI) || topMenu.IsInherited(CRF_SlottingMenuUI))
				return;
			else if (topMenu.IsInherited(CRF_SpectatorMenuUI))
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
	 * Callback for advancing gamemode state
	 */
	void Advance_Callback(SCR_ChatPanel panel, string data)
	{
		m_RplToAuthorityManager.RequestAdvanceGamemodeState(true);
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
	void AddScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset.ToString(), 
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
	void RemoveScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.RemoveItemOrdered(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset.ToString(), 
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
}
