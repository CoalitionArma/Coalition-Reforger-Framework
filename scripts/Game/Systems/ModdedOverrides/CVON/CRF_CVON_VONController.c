modded class SCR_VONController
{
	bool IsPlayerAndClientSpectator(int playerId)
	{
		if (playerId == 0)
			return false;
		
		return m_FactionManager.GetPlayerFaction(playerId).GetFactionKey() == "SPEC" && m_FactionManager.GetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey() == "SPEC";
	}
	
	bool IsPlayerSpectator(int playerId)
	{
		if (playerId == 0)
			return false;
		
		if (m_FactionManager.GetPlayerFaction(playerId) == null)
			return false;
		
		return m_FactionManager.GetPlayerFaction(playerId).GetFactionKey() == "SPEC";
	}
	
	bool IsPlayerInDeafenChannel()
	{
		return CRF_MenuManager.GetInstance().GetChannel(SCR_PlayerController.GetLocalPlayerId()) == 0;
	}
	
	override bool ShouldMuffleAudio(IEntity senderEntity, int playerId = 0, out int loweredDecibles = 0)
	{
		if (IsPlayerAndClientSpectator(playerId))
			return false;
		
		if (CanPlayerSeeSender(senderEntity))
			return false;
		
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		IEntity receiverBuilding;
		IEntity senderBuilding;
		bool isSenderInBuilding = IsInBuildingOrVehicle(senderEntity, senderBuilding);
		bool isPlayerInBuilding = IsInBuildingOrVehicle(player, receiverBuilding);
		
		if (!isSenderInBuilding && !isPlayerInBuilding)
			return false;
		
		if (isPlayerInBuilding != isSenderInBuilding)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING;
			return true;
		}
		
		if (senderBuilding != receiverBuilding)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING * 2;
			return true;
		}
		float top;
		float bottom;
		
		DetermineHearingWindow(player, top, bottom);
		vector senderOrigin = GetHeadHeight(senderEntity);
		if (senderOrigin[1] > top || senderOrigin[1] < bottom)
		{
			loweredDecibles = CVON_DB_ATTEN_BUILDING;
			return true;
		}
		return false;
	}
	
	override void ComputeStereoLR(
	    IEntity listener,
	    vector  sourcePos,
	    float   volume_m,    
		int playerId ,       // interpret as the inaudible distance (≈ −45 dB)
	    out float outLeft,
	    out float outRight,
	    out int  silencedDecibels = 0,
	    float   rearPanBoost   = 0.55,
	    float   rearShadow     = 0.12,
	    float   elevNarrow     = 0.25,
	    float   bleed          = 0.10,
	    bool    normalizePeak  = true
	)
	{
		float specLeft;
		float specRight;
		if (SpectatorLRCheck(playerId, specLeft, specRight))
		{
			outLeft = specLeft;
			outRight = specRight;
			silencedDecibels = 0;
			return;
		}
		super.ComputeStereoLR(listener, sourcePos, volume_m, playerId, outLeft, outRight, silencedDecibels, rearPanBoost, rearShadow, elevNarrow, bleed, normalizePeak);
	}
	
	bool SpectatorLRCheck(int playerId, out float left, out float right)
	{
		if (!IsPlayerAndClientSpectator(playerId))
			return false;
		
		if (IsPlayerInDeafenChannel())
		{
			left = 0;
			right = 0;
			return true;
		}
		CRF_MenuManager menuManager = CRF_MenuManager.GetInstance();
		// Get the current player's channel with improved frequency calculation
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int playerChannelId = CRF_MenuManager.GetInstance().GetChannel(localPlayerId);
		
		// Calculate unique frequency for the channel to prevent conflicts
		// Use a base frequency of 10000 + (channelId * 1000) to ensure separation
		// This prevents frequency collisions between different channels
		int frequency = 10000 + (playerChannelId * 1000);
		
		// For custom channels (ID > 1), add additional offset based on channel name hash
		// This ensures each custom channel gets a truly unique frequency
		if (playerChannelId > 1 && menuManager.m_aVONChannels.IsIndexValid(playerChannelId))
		{
			string channelData = menuManager.m_aVONChannels[playerChannelId];
			ref array<string> channelParts = {};
			channelData.Split("|", channelParts, true);
			
			if (channelParts.Count() > 0)
			{
				string channelName = channelParts[0];
				// Use channel name hash to create unique frequency offset
				int nameHash = channelName.Hash();
				// Ensure positive hash and limit range to prevent frequency overlap
				int frequencyOffset = Math.AbsInt(nameHash) % 500;
				frequency += frequencyOffset;
			}
		}
		
		int sendPlayerChannelId = CRF_MenuManager.GetInstance().GetChannel(playerId);
		
		// Calculate unique frequency for the channel to prevent conflicts
		// Use a base frequency of 10000 + (channelId * 1000) to ensure separation
		// This prevents frequency collisions between different channels
		int senderFrequency = 10000 + (sendPlayerChannelId * 1000);
		
		// For custom channels (ID > 1), add additional offset based on channel name hash
		// This ensures each custom channel gets a truly unique frequency
		if (sendPlayerChannelId > 1 && menuManager.m_aVONChannels.IsIndexValid(sendPlayerChannelId))
		{
			string channelData = menuManager.m_aVONChannels[sendPlayerChannelId];
			ref array<string> channelParts = {};
			channelData.Split("|", channelParts, true);
			
			if (channelParts.Count() > 0)
			{
				string channelName = channelParts[0];
				// Use channel name hash to create unique frequency offset
				int nameHash = channelName.Hash();
				// Ensure positive hash and limit range to prevent frequency overlap
				int frequencyOffset = Math.AbsInt(nameHash) % 500;
				senderFrequency += frequencyOffset;
			}
		}
		
		if (frequency == senderFrequency)
		{
			left = 1;
			right = 1;
			return true;
		}
		else
		{
			left = 0;
			right = 0;
			return true;
		}
	}
	
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		if (!CVON_VONGameModeComponent.GetInstance())
			return;
		
		if (!m_PlayerController)
		{
			m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		}
		if (!m_CharacterController)
			if (SCR_PlayerController.GetLocalControlledEntity())
				m_CharacterController = SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent));
		if (!m_VONGameModeComponent)
			m_VONGameModeComponent = CVON_VONGameModeComponent.GetInstance();
		if (!m_PlayerManager)
			m_PlayerManager = GetGame().GetPlayerManager();
		
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (!camera)
			return;
		
		ref array<int> playerIds = {};
		m_PlayerManager.GetPlayers(playerIds);
		int maxDistance = m_PlayerController.m_aVolumeValues.Get(4);
		bool isLocalSpectator = IsPlayerSpectator(SCR_PlayerController.GetLocalPlayerId());
    //When a player disconnects, they are no longer in the players array, so it just leaves an empty container.
		//This removes that container as when they reconnect they will no longer be heard.
		foreach (int playerId: m_PlayerController.m_aLocalActiveVONEntriesIds)
		{
			if (playerIds.Contains(playerId))
				continue;
			
			int index = m_PlayerController.m_aLocalActiveVONEntriesIds.Find(playerId);
			m_PlayerController.m_aLocalActiveVONEntriesIds.RemoveOrdered(index);
			m_PlayerController.m_aLocalActiveVONEntries.RemoveOrdered(index);
			continue;
		}
		foreach (int playerId: playerIds)
		{
			if (!SCR_PlayerController.GetLocalControlledEntity())
				continue;
			
			if (playerId == SCR_PlayerController.GetLocalPlayerId())
				continue;
			
			IEntity player = m_PlayerManager.GetPlayerControlledEntity(playerId);
			if (!player)
				continue;
			
			SCR_CharacterControllerComponent charCont = SCR_CharacterControllerComponent.Cast(ChimeraCharacter.Cast(player).GetCharacterController());
			if (charCont.IsDead() || charCont.IsUnconscious())
				if (m_PlayerController.m_aLocalActiveVONEntriesIds.Contains(playerId))
				{
					int index = m_PlayerController.m_aLocalActiveVONEntriesIds.Find(playerId);
					m_PlayerController.m_aLocalActiveVONEntriesIds.RemoveOrdered(index);
					m_PlayerController.m_aLocalActiveVONEntries.RemoveOrdered(index);
					continue;
				}
				else
					continue;
			
			float distance = vector.Distance(player.GetOrigin(), camera.GetOrigin());
			if (distance > maxDistance)
			{
				bool isOtherSpectator = IsPlayerSpectator(playerId);
				
				if (isLocalSpectator && isOtherSpectator)
				{
					if (m_PlayerController.m_aLocalActiveVONEntriesIds.Contains(playerId))
						continue;
					else
					{
						CVON_VONContainer container = new CVON_VONContainer();
						container.m_eVonType = CVON_EVONType.DIRECT;
						container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
						container.m_SenderRplId = RplComponent.Cast(player.FindComponent(RplComponent)).Id();
						container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
						container.m_iPlayerId = playerId;
						m_PlayerController.m_aLocalActiveVONEntries.Insert(container);
						m_PlayerController.m_aLocalActiveVONEntriesIds.Insert(playerId);
						continue;
					}
				}
					
				if (m_PlayerController.m_aLocalActiveVONEntriesIds.Contains(playerId))
				{
					//If this VON Transmission is radio, don't do shit
					if (m_PlayerController.m_aLocalActiveVONEntries.Get(m_PlayerController.m_aLocalActiveVONEntriesIds.Find(playerId)).m_eVonType == CVON_EVONType.RADIO)
						continue;
					int index = m_PlayerController.m_aLocalActiveVONEntriesIds.Find(playerId);
					m_PlayerController.m_aLocalActiveVONEntriesIds.RemoveOrdered(index);
					m_PlayerController.m_aLocalActiveVONEntries.RemoveOrdered(index);
					continue;
				}
				else
					continue;
			}
			else
			{
				if (m_PlayerController.m_aLocalActiveVONEntriesIds.Contains(playerId))
					continue;
				else
				{
					CVON_VONContainer container = new CVON_VONContainer();
					container.m_eVonType = CVON_EVONType.DIRECT;
					container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
					container.m_SenderRplId = RplComponent.Cast(player.FindComponent(RplComponent)).Id();
					container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
					container.m_iPlayerId = playerId;
					m_PlayerController.m_aLocalActiveVONEntries.Insert(container);
					m_PlayerController.m_aLocalActiveVONEntriesIds.Insert(playerId);
				}
				
			}
		}
		
		//Local processing of data being sent to us
		foreach (CVON_VONContainer container: m_PlayerController.m_aLocalActiveVONEntries)
		{
			if (!SCR_PlayerController.GetLocalControlledEntity())
				break;
			if (!container.m_SoundSource)
				continue;

			float distance = vector.Distance(container.m_SoundSource.GetOrigin(), camera.GetOrigin());
			if (distance < maxDistance)
				container.m_fDistanceToSender = distance;
			else
				container.m_fDistanceToSender = -1;
		}
		

		//Handles broadcasting to other players
		if (m_bIsBroadcasting)
		{
			if (m_CharacterController.GetLifeState() != ECharacterLifeState.ALIVE)
			{
				if (m_bToggleBuffer)
				{
					m_bToggleBuffer = false;
					DeactivateCVON();
					m_VONHud.DirectToggleDelay();
				}
				else
					DeactivateCVON();
				return;
			}
				
			ref array<int> broadcastToPlayerIds = {};
			foreach (int playerId: playerIds)
			{	
				#ifdef WORKBENCH
				#else
				if (playerId == SCR_PlayerController.GetLocalPlayerId())
					continue;
				#endif
				
//				if (m_CurrentVONContainer.m_eVonType == CVON_EVONType.DIRECT)
//				{
//					if (!IsPlayerSpectator(playerId))
//					{
//						IEntity player = m_PlayerManager.GetPlayerControlledEntity(playerId);
//						if (!player)
//							continue;
//						
//						if (vector.Distance(player.GetOrigin(), SCR_PlayerController.GetLocalControlledEntity().GetOrigin()) > maxDistance)
//						{
//							if (m_aPlayerIdsBroadcastedTo.Contains(playerId))
//							{
//								m_aPlayerIdsBroadcastedTo.RemoveItem(playerId);
//								m_PlayerController.BroadcastRemoveLocalVONToServer(playerId, SCR_PlayerController.GetLocalPlayerId());
//							}
//							continue;
//						}
//					}
//				}
				
				if (m_aPlayerIdsBroadcastedTo.Contains(playerId))
					continue;
				
				broadcastToPlayerIds.Insert(playerId);
				m_aPlayerIdsBroadcastedTo.Insert(playerId);
			}
			if (broadcastToPlayerIds.Count() > 0)
			{
//				if (m_CurrentVONContainer.m_eVonType == CVON_EVONType.DIRECT)
//					m_PlayerController.BroadcastLocalVONToServer(m_CurrentVONContainer, broadcastToPlayerIds, SCR_PlayerController.GetLocalPlayerId(), RplId.Invalid());
//				else
					m_PlayerController.BroadcastLocalVONToServer(m_CurrentVONContainer, broadcastToPlayerIds, SCR_PlayerController.GetLocalPlayerId(), m_CurrentVONContainer.m_iRadioId);
			}
				
		}
		WriteJSON();
	}
}