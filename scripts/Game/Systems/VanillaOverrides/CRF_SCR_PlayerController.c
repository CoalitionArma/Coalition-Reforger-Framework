modded class SCR_PlayerController
{
	//Stores local camera entity to delete whenever you take over a player
	IEntity m_eCamera;

	protected bool m_bActivated = false;
	int m_iFPS = 0;
	int m_iAudioSetting;
	private vector m_vStoredCameraPos[4];

	//Adds action lisener to open menu in game
	//------------------------------------------------------------------------------------------------
	override protected void UpdateLocalPlayerController()
	{
		super.UpdateLocalPlayerController();
		
		if(!CRF_Gamemode.GetInstance())
			return;

		m_bIsLocalPlayerController = this == GetGame().GetPlayerController();
		if (!m_bIsLocalPlayerController)
			return;

		s_pLocalPlayerController = this;
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		GetGame().GetInputManager().AddActionListener("CRF_OpenLobby", EActionTrigger.PRESSED, OpenSlottingMenu);
		GetGame().GetInputManager().AddActionListener("CRF_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);

		PlayerJoined();
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
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if(!CRF_Gamemode.GetInstance())
			return;
		
		GetGame().GetInputManager().RemoveActionListener("SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
		if (m_bActivated)
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);

		m_bActivated = false;

		super.OnControlledEntityChanged(from, to);
	}

	//------------------------------------------------------------------------------------------------
	override void DisconnectFromGame()
	{
		if(!CRF_Gamemode.GetInstance())
			return;
		
		super.DisconnectFromGame();

		ResetSettingsToStoredValues();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateStoredCameraPos(vector cameraPosToStore[4])
	{
		m_vStoredCameraPos = cameraPosToStore;
	}

	//------------------------------------------------------------------------------------------------
	void SpecCameraInit(vector cameraPos[4])
	{
		if (m_eCamera)
			delete m_eCamera;

		if (SCR_EditorManagerEntity.GetInstance().IsOpened())
			return;

		if (cameraPos[3] != vector.Zero && cameraPos[3] != CRF_Gamemode.GetInstance().m_vGenericSpawn[3])
			m_vStoredCameraPos = cameraPos;

		if (m_vStoredCameraPos[3] != vector.Zero && cameraPos[3] == CRF_Gamemode.GetInstance().m_vGenericSpawn[3])
			cameraPos = m_vStoredCameraPos;

		if (cameraPos[3] == vector.Zero || cameraPos[3] == "0 10000 0" || cameraPos[3][0] < 1 || cameraPos[3][2] < 1)
			cameraPos = CRF_Gamemode.GetInstance().m_vGenericSpawn;

		EntitySpawnParams cameraSpawnParams = new EntitySpawnParams();
		cameraSpawnParams.TransformMode = ETransformMode.WORLD;
		cameraSpawnParams.Transform = cameraPos;

		m_eCamera = GetGame().SpawnEntityPrefab(Resource.Load("{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et"), GetGame().GetWorld(), cameraSpawnParams);
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));

		CheckVONRegister();
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_GamemodeState.GAME)
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SpectatorMenu);
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	}

	//------------------------------------------------------------------------------------------------
	void PlayerJoined()
	{
		if (GetPlayerId() == 0)
		{
			GetGame().GetCallqueue().CallLater(PlayerJoined, 100, false);
			return;
		}

		if (CRF_Gamemode.GetInstance().m_aSlots.Find(GetPlayerId()) == -1)
		{
			CRF_ClientComponent.GetInstance().RequestSpectator(GetPlayerId());
			GetGame().GetCallqueue().CallLater(CRF_Gamemode.GetInstance().OpenCurrentStateMenu, 500, false);
		}
		else
		{
			CRF_ClientComponent.GetInstance().RequestSpectator(GetPlayerId());
			GetGame().GetCallqueue().CallLater(EnterGame, 500, false, GetPlayerId());
		};
	}

	//------------------------------------------------------------------------------------------------
	// Ask server to respawn player after timer ends
	void RespawnPlayer(int playerId, vector spawnLocation = vector.Zero)
	{
		Rpc(RpcDo_RespawnPlayer, playerId, spawnLocation)
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_RespawnPlayer(int playerID, vector spawnLocation)
	{
		CRF_Gamemode.GetInstance().RespawnPlayer(playerID, spawnLocation);
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

	//Communicates to server that the player is talking
	//------------------------------------------------------------------------------------------------
	void SetTalking(bool input, int playerID)
	{
		Rpc(RpcDo_SetTalking, input, playerID);
	}

	//Communicates to server that the player is talking
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_SetTalking(bool input, int playerID)
	{
		if (input)
			CRF_Gamemode.GetInstance().SetPlayerTalking(playerID);
		else
			CRF_Gamemode.GetInstance().RemovePlayerTalking(playerID);
	}

	//Communicates to server to advance state
	//------------------------------------------------------------------------------------------------
	void AdvanceGamemodeState()
	{
		Rpc(RpcDo_AdvanceGamemodeState);
	}

	//Communicates to server to advance state
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_AdvanceGamemodeState()
	{
		CRF_Gamemode.GetInstance().AdvanceGamemodeState();
	}

	//Communicates to server to set slot
	//------------------------------------------------------------------------------------------------
	void SetSlot(int index, int playerID)
	{
		Rpc(RpcDo_SetSlot, index, playerID);
	}

	//Communicates to server to set slot
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_SetSlot(int index, int playerID)
	{
		CRF_Gamemode.GetInstance().SetSlot(index, playerID);
	}

	//Communicates to server to set group locked
	//------------------------------------------------------------------------------------------------
	void SetGroupLocked(int index, bool input)
	{
		Rpc(RpcDo_SetGroupLocked, index, input);
	}

	//Communicates to server to set group locked
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_SetGroupLocked(int index, bool input)
	{
		CRF_Gamemode.GetInstance().SetGroupLockedStatus(index, input);
	}

	//Called to enter the game, decides if you will go into spectator on the client
	//------------------------------------------------------------------------------------------------
	void EnterGame(int playerID)
	{
		//Call to server to enter slot and or get put into a initial entity to spectate
		if (m_eCamera)
			delete m_eCamera;

		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PreviewMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SpectatorMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_AARMenu);
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_RespawnMenu);

		if (CRF_Gamemode.GetInstance().m_aSlots.Find(playerID) == -1)
			CRF_ClientComponent.GetInstance().RequestSpectator(playerID);

		Rpc(RpcDo_EnterGame, playerID);

		ResetSettingsToStoredValues();

		GetGame().GetCallqueue().CallLater(SetupRadioFrequency, 1500, false);
	}
	//------------------------------------------------------------------------------------------------
	void ResetSettingsToStoredValues()
	{
		BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		video.Set("MaxFps", m_iFPS);
		GetGame().UserSettingsChanged();

		if (m_iAudioSetting == 0)
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);
		else
			AudioSystem.SetMasterVolume(AudioSystem.SFX, m_iAudioSetting);
	}

	//------------------------------------------------------------------------------------------------
	void SetupRadioFrequency()
	{
		// Get player's radio
		IEntity entity = GetMainEntity();
		if (entity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
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

		SCR_AIGroup group = m_GroupManager.GetPlayerGroup(GetPlayerId());
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

	//Communicates to server to enter slot and or get put into a initial entity to spectate
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_EnterGame(int playerID)
	{
		CRF_Gamemode.GetInstance().EnterGame(playerID);
	}

	//------------------------------------------------------------------------------------------------
	void RequestToJoinChannel(int channel, int requestId)
	{
		Rpc(RpcDo_RequestToJoinChannel, channel, requestId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_RequestToJoinChannel(int channel, int requestId)
	{
		CRF_Gamemode.GetInstance().RequestToJoinChannel(channel, requestId);
	}

	//------------------------------------------------------------------------------------------------
	void CheckVONRegister()
	{
		Rpc(RpcDo_CheckVONRegister, GetLocalPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_CheckVONRegister(int playerId)
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		int channelIndex;
		if (!gamemode.IsPlayerInAnyChannel(playerId, channelIndex))
		{
			gamemode.AddPlayerToChannel(playerId, 1, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	void CreateChannel()
	{
		Rpc(RpcDo_CreateChannel, GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_CreateChannel(int playerId)
	{
		CRF_Gamemode.GetInstance().CreateChannel(GetGame().GetPlayerManager().GetPlayerName(playerId) + "'s Channel", playerId);
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
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_GamemodeState.GAME)
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

	//Communicates to server to advance the slotting phase
	//------------------------------------------------------------------------------------------------
	void AdvanceSlottingPhase()
	{
		Rpc(RpcDo_AdvanceSlottingPhase);
	}

	//Communicates to server to advance the slotting phase
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_AdvanceSlottingPhase()
	{
		CRF_Gamemode.GetInstance().AdvanceSlottingState();
	}

	//------------------------------------------------------------------------------------------------
	void JoinChannel(int playerId, int channel)
	{
		Rpc(RpcDo_JoinChannel, playerId, channel);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_JoinChannel(int playerId, int channel)
	{
		CRF_Gamemode.GetInstance().AddPlayerToChannel(playerId, channel, false);
	}

	//------------------------------------------------------------------------------------------------
	void Accept(int playerId, int channel)
	{
		Rpc(RpcDo_Accept, playerId, channel);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_Accept(int playerId, int channel)
	{
		CRF_Gamemode.GetInstance().AddPlayerToChannel(playerId, channel, false);
	}
}
