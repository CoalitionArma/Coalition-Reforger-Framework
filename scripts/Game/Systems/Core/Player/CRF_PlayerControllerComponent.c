[ComponentEditorProps(category: "Player Controller Components", description: "")]
class CRF_PlayerControllerComponentClass : ScriptComponentClass
{

}

class CRF_PlayerControllerComponent : ScriptComponent
{
	string m_sHintText = "Type Here";
	
	bool m_bHUDVisible = true;

	ref array<string> m_aScriptedMarkers = {};
	
	//Stores local camera entity to delete whenever you take over a player
	IEntity m_eCamera;

	bool m_bActivated = false;
	int m_iFPS = -1;
	int m_iAudioSetting = -1;
	private vector m_vStoredCameraPos[4];
	Widget m_wSavedHintWidget;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_RplToAuthorityManager m_RplToAuthorityManager;

	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------

	static CRF_PlayerControllerComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_PlayerControllerComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_PlayerControllerComponent));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	void InitilizePlayerControllerComp()
	{

		if (!GetGame().InPlayMode() || RplSession.Mode() == RplMode.Dedicated)
			return;
		
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();

		GetGame().GetInputManager().AddActionListener("CRF_ToggleSideReady", EActionTrigger.DOWN, ToggleSideReady);
		GetGame().GetInputManager().AddActionListener("CRF_AdminForceReady", EActionTrigger.DOWN, AdminForceReady);
		GetGame().GetInputManager().AddActionListener("CRF_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("CRF_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
		GetGame().GetInputManager().AddActionListener("SwitchSpectatorUI", EActionTrigger.DOWN, UpdateHUDVisible);
		
		GetGame().GetCallqueue().CallLater(AddMsgAction, 1000, false);
		GetGame().GetCallqueue().CallLater(AddAdvanceAction, 1000, false);
		
		GetGame().GetCallqueue().CallLater(OpenCurrentStateMenu, 500, false);
	}

	//------------------------------------------------------------------------------------------------
	void InitilizePlayer()
	{
		//Call to server to enter slot and or get put into a initial entity to spectate
		if (m_eCamera)
			delete m_eCamera; 

		GetGame().GetMenuManager().CloseAllMenus();
		
		ResetSettingsToStoredValues();

		GetGame().GetCallqueue().CallLater(SetupRadioFrequency, 1500, false);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateHUDVisible()
	{
		m_bHUDVisible = !m_bHUDVisible;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateStoredCameraPos(vector cameraPosToStore[4])
	{
		m_vStoredCameraPos = cameraPosToStore;
	}

	//------------------------------------------------------------------------------------------------
	void SpecCameraInit(vector cameraPos[4])
	{
		if(!m_RplToAuthorityManager || !m_Gamemode)
		{
			m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
			m_Gamemode = CRF_Gamemode.GetInstance();
		};
		
		if (SCR_EditorManagerEntity.GetInstance().IsOpened())
			return;

		if (cameraPos[3] != vector.Zero && cameraPos[3] != m_Gamemode.m_vGenericSpawn[3])
			m_vStoredCameraPos = cameraPos;

		if (m_vStoredCameraPos[3] != vector.Zero && cameraPos[3] == m_Gamemode.m_vGenericSpawn[3])
			cameraPos = m_vStoredCameraPos;

		if (cameraPos[3] == vector.Zero || cameraPos[3] == "0 10000 0" || cameraPos[3][0] < 1 || cameraPos[3][2] < 1)
			cameraPos = m_Gamemode.m_vGenericSpawn;

		EntitySpawnParams cameraSpawnParams = new EntitySpawnParams();
		cameraSpawnParams.TransformMode = ETransformMode.WORLD;
		cameraSpawnParams.Transform = cameraPos;

		if (!m_eCamera)
			m_eCamera = GetGame().SpawnEntityPrefab(Resource.Load("{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et"), GetGame().GetWorld(), cameraSpawnParams);
		
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));

		m_RplToAuthorityManager.CheckVONRegister(SCR_PlayerController.GetLocalPlayerId());
		
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SpectatorMenu);
		
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	}

	//------------------------------------------------------------------------------------------------
	void UpdateEntityPos(vector cameraPos[4])
	{
		//Rpc(RpcDo_UpdateCameraPos, entityID, cameraPos);
		IEntity player = GetGame().GetPlayerController().GetControlledEntity();

		//~ Align to terrain if not a character
		if (!ChimeraCharacter.Cast(player))
			SCR_TerrainHelper.OrientToTerrain(cameraPos);

		BaseGameEntity baseGameEntity = BaseGameEntity.Cast(player);
		if (baseGameEntity)
			baseGameEntity.Teleport(cameraPos);
		else
			player.SetWorldTransform(cameraPos);

		Physics phys = player.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}
	
	// Toggle Spec NVG
	//------------------------------------------------------------------------------------------------
	void ToggleNVGs()
	{
		m_bActivated = !m_bActivated;

		if (m_bActivated)
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{0AD0A1ADEBCF893F}Assets/Items/Equipment/NVG/pvs14/data/SpecNVGFilm.emat", true);
		else
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetSettingsToStoredValues()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		
		if (m_iFPS != -1)
			video.Set("MaxFps", m_iFPS);
		GetGame().UserSettingsChanged();

		if (m_iAudioSetting == -1)
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);
		else
			AudioSystem.SetMasterVolume(AudioSystem.SFX, m_iAudioSetting);
	}

	//------------------------------------------------------------------------------------------------
	void SetupRadioFrequency()
	{
		// Get player's radio
		IEntity entity = SCR_PlayerController.GetLocalMainEntity();
		if (!entity || CRF_GamemodeManager.IsSpectator(entity))
			return;
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

		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		BaseTransceiver grpTsv = radio.GetTransceiver(0);

		// Set Player's Freq to whatever group they are in
		SCR_GroupsManagerComponent m_GroupManager = SCR_GroupsManagerComponent.GetInstance();
		if (!m_GroupManager)
			return;

		SCR_AIGroup group = m_GroupManager.GetPlayerGroup(SCR_PlayerController.GetLocalPlayerId());
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && group)
		{
			RadioHandlerComponent rhc = RadioHandlerComponent.Cast(pc.FindComponent(RadioHandlerComponent));
			if (rhc)
				rhc.SetFrequency(grpTsv, group.GetRadioFrequency());
		}

		SCR_VoNComponent von = SCR_VoNComponent.Cast(entity.FindComponent(SCR_VoNComponent));

		von.SetTransmitRadio(grpTsv);

		BaseTransceiver pltTsv = radio.GetTransceiver(1);

		if (pltTsv)
			von.SetTransmitRadio(pltTsv);
	}
	
	//------------------------------------------------------------------------------------------------
	void OpenCurrentStateMenu()
	{
		if(!m_RplToAuthorityManager || !m_Gamemode)
		{
			m_RplToAuthorityManager = CRF_RplToAuthorityManager.GetInstance();
			m_Gamemode = CRF_Gamemode.GetInstance();
		};
		
		//Close any menu that wriggles its way in
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseAllMenus();
		//Opens menu based on current game state : )
		switch (m_Gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.BRIEFING: {GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);						break; }
			case CRF_EGamemodeState.SLOTTING:	{GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);					break; }
			case CRF_EGamemodeState.GAME: 	{m_RplToAuthorityManager.RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());		break; }
			case CRF_EGamemodeState.AAR: 		{GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_AARMenu);						break; }
		}
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
			if (m_iFPS == -1)
				video.Get("MaxFps", m_iFPS);
			
			video.Set("MaxFps", 30);
			GetGame().UserSettingsChanged();
			
			if (m_iAudioSetting == -1)
				m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
			
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		}
	}
	
	//Opens the slotting menu for players in game
	//------------------------------------------------------------------------------------------------
	void OpenSlottingMenu(float value = 0.0, EActionTrigger reason = 0)
	{
		if (value != 1)
			return;

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			if (topMenu.IsInherited(CRF_PreviewMenuUI) || topMenu.IsInherited(CRF_SlottingMenuUI))
				return;
			else if (topMenu.IsInherited(CRF_SpectatorMenuUI))
				GetGame().GetMenuManager().CloseMenu(topMenu);

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
			
			if (m_iFPS)
				video.Get("MaxFps", m_iFPS);
			
			video.Set("MaxFps", 30);
			GetGame().UserSettingsChanged();
			
			if (m_iAudioSetting)
				m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
			
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		}
	}	

	//------------------------------------------------------------------------------------------------
	// Admin Messaging
	//------------------------------------------------------------------------------------------------
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
	}
	
	//------------------------------------------------------------------------------------------------
	// Functions for Frontline
	//------------------------------------------------------------------------------------------------
	void UpdateMapMarkers(array<string> zoneStatus, array<string> zoneObjectNames, FactionKey bluforSide, FactionKey opforSide)
	{
		RemoveALLScriptedMarkers();

		foreach (int i, string zoneName : zoneObjectNames)
		{
			string status = zoneStatus[i];
			string imageTexture;
			int imageColor;

			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);

			string zoneLocked = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];

			switch (i)
			{
				case 0 : {imageTexture = "{21A2A457BD0E42C1}UI\Objectives\A.edds"; break; };
				case 1 : {imageTexture = "{7F4A8D140283CCCE}UI\Objectives\B.edds"; break; };
				case 2 : {imageTexture = "{8B42CA8C0F5EA4BA}UI\Objectives\C.edds"; break; };
				case 3 : {imageTexture = "{C29ADF937D98D0D0}UI\Objectives\D.edds"; break; };
				case 4 : {imageTexture = "{3692980B7045B8A4}UI\Objectives\E.edds"; break; };
			}

			if (zoneLocked == "Locked")
				AddScriptedMarker(zoneName, "0 0 0", 0, "", "{91427B7866707601}UI\Objectives\lock.edds", 50, ARGB(255, 142, 142, 142));

			switch (zoneFactionStored)
			{
				case bluforSide : {imageColor = ARGB(255, 0, 25, 225); break; }; //Blufor
				case opforSide : {imageColor = ARGB(255, 225, 25, 0); break; }; //Opfor
				default : {imageColor = ARGB(255, 225, 225, 225); break; }; //Uncaptured
			}

			AddScriptedMarker(zoneName, "0 0 0", 0, "", imageTexture, 45, imageColor);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Functions for scripted markers
	//------------------------------------------------------------------------------------------------
	TStringArray GetScriptedMarkersArray()
	{
		return m_aScriptedMarkers;
	}

	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Adds a scripted marker on the users map which will follow the specified entity
	//! \param[in] markerEntityName is the name of the entity the marker will track.
	//! \param[in] markerOffset is the offset from the marker entity. (This can also be the vector pos for a static marker, simply set the "markerEntityName" param to "Static Marker").
	//! \param[in] timeDelay is the delay between marker updates.
	//! \param[in] markerText is the text that will be displayed on the map just under the image.
	//! \param[in] markerImage is the image that will be displayed on the map.
	//! \param[in] markerColor is the color the image will be.
	void AddScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5||%6||%7", markerEntityName, markerOffset.ToString(), timeDelay.ToString(), markerText, markerImage, zOrder.ToString(), markerColor.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Removes the scripted marker from the users map, must have all params be the same params that were initially put in the AddScriptedMarkers function
	void RemoveScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.RemoveItemOrdered(string.Format("%1||%2||%3||%4||%5||%6||%7", markerEntityName, markerOffset.ToString(), timeDelay.ToString(), markerText, markerImage, zOrder.ToString(), markerColor.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Removes all scripted markers from the users maps
	void RemoveALLScriptedMarkers()
	{
		m_aScriptedMarkers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	// Admin Menu
	//------------------------------------------------------------------------------------------------
	void SendAdminMessage(SCR_ChatPanel panel, string data)
	{

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(string.Format("Message Sent: \"%1\"", data));
		data = string.Format("playerId: %1 | Player Name: %3 | \"%2\"", GetGame().GetPlayerController().GetPlayerId(), data, GetGame().GetPlayerManager().GetPlayerName(GetGame().GetPlayerController().GetPlayerId()));
		m_RplToAuthorityManager.SendAdminMessage(data);
	}

	//------------------------------------------------------------------------------------------------
	void ReplyAdminMessage(SCR_ChatPanel panel, string data)
	{
		if (!SCR_Global.IsAdmin() && !m_GamemodeManager.IsModerator())
			return;
		
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
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
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

		chatComponent.ShowMessage(string.Format("Message Sent to %2: \"%1\"", toSend, GetGame().GetPlayerManager().GetPlayerName(playerId)));
		toSend = string.Format("\"%1\"", toSend);
		m_RplToAuthorityManager.ReplyAdminMessage(toSend, playerId, true);
	}

	//------------------------------------------------------------------------------------------------
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

		if (playersGroup.IsPlayerLeader(SCR_PlayerController.GetLocalPlayerId()))
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			Faction faction = factionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		
			if (faction.GetFactionKey() == "")
				return;
			
			m_RplToAuthorityManager.ToggleSideReady(faction.GetFactionKey(), playerName, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	void AdminForceReady()
	{
		if (!SCR_Global.IsAdmin())
			return;
		m_RplToAuthorityManager.ToggleSideReady("", GetGame().GetPlayerManager().GetPlayerName(SCR_PlayerController.GetLocalPlayerId()), true);
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	// AAR Screen
	void AddAdvanceAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("aar");
		invoker.Insert(Advance_Callback);
	}
	
	void Advance_Callback(SCR_ChatPanel panel, string data)
	{
		m_RplToAuthorityManager.RequestAdvanceGamemodeState();
	}
}
