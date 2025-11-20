modded class SCR_VONController
{
	bool IsPlayerAndClientSpectator(int playerId)
	{
		if (playerId == 0)
			return false;
		
		return (m_FactionManager.GetPlayerFaction(playerId).GetFactionKey() == "SPEC" 
		&& m_FactionManager.GetPlayerFaction(m_PlayerController.GetPlayerId()).GetFactionKey() == "SPEC") ||
		(m_FactionManager.GetPlayerFaction(playerId).GetFactionKey() == "SPEC" && m_PlayerController.m_bIsListeningToSpec) ||
		(m_FactionManager.GetPlayerFaction(m_PlayerController.GetPlayerId()).GetFactionKey() == "SPEC" && m_VONGameModeComponent.IsPlayerListening(playerId));
	}
	
	override void ActivateCVON(CVON_EVONTransmitType transmitType = CVON_EVONTransmitType.NONE)
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			if(topMenu.IsInherited(CRF_Outro))
				return;
		
		super.ActivateCVON(transmitType);
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
		
		IEntity player = m_PlayerController.GetLocalControlledEntity();
		if (!player)
			return false;
		
		if (!senderEntity)
			return false;
		
		if (CanPlayerSeeSender(senderEntity, player))
			return false;
		
		IEntity receiverBuilding;
		IEntity senderBuilding;
		bool isSenderInBuilding = IsInBuildingOrVehicle(senderEntity, senderBuilding);
		bool isPlayerInBuilding = IsInBuildingOrVehicle(player, receiverBuilding);
		if (CheckIfInSameVehicle(senderEntity, player))
			return false;
		
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
		if (CRF_Gamemode.GetInstance().m_bIsInEndCredits)
		{
			outLeft = 0;
			outRight = 0;
			return;
		}
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
	
	override void ComputeSpectatorLR(int playerId, out float outLeft = 1, out float outRight = 1, out int silencedDecibels = 0)
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
		
		int frequency = CRF_MenuManager.GetInstance().GetChannel(m_PlayerController.GetPlayerId());
		int senderFrequency = CRF_MenuManager.GetInstance().GetChannel(playerId);
		
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
		if (m_fWriteTeamspeakClientIdCooldown > 0)
			m_fWriteTeamspeakClientIdCooldown -= timeSlice;
		else
			m_fWriteTeamspeakClientIdCooldown = 0;
		
		if (!CVON_VONGameModeComponent.GetInstance())
			return;
		if (!m_PlayerController)
			m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		//What the player is that we have to process this frame
		m_Player = m_PlayerController.GetControlledEntity();
		
		if (!m_CharacterController)
			if (m_Player)
				m_CharacterController = SCR_CharacterControllerComponent.Cast(m_Player.FindComponent(SCR_CharacterControllerComponent));
		
		if (!m_PlayerRplComponent)
			if (m_Player)
				m_PlayerRplComponent = RplComponent.Cast(m_Player.FindComponent(RplComponent));
		
		if (!m_PlayerRplComponent || !m_CharacterController || !m_Player)
			return;
		
		m_Camera = m_CameraManager.CurrentCamera();
		if (!m_Camera)
			return;
		
		m_PlayerIdTemp.Clear();
		m_PlayerManager.GetPlayers(m_PlayerIdTemp);
		
		if (m_fHeadCacheBuffer >= 0.2)
		{
			UpdateHeadCache();
			m_fHeadCacheBuffer = 0;
		}
		else
			m_fHeadCacheBuffer += timeSlice;
		
		m_PlayerIdTemp.Clear();
		m_PlayerManager.GetPlayers(m_PlayerIdTemp);
		bool isLocalSpectator = IsPlayerSpectator(m_PlayerController.GetPlayerId());
		bool isListeningToSpectator = m_PlayerController.m_bIsListeningToSpec;
		
    	//When a player disconnects, they are no longer in the players array, so it just leaves an empty container.
		//This removes that container as when they reconnect they will no longer be heard.
		//Also sound updating for maximum optimizations
		foreach (int playerId, CVON_VONContainer container: m_PlayerController.m_aLocalEntries)
		{
			if (!m_PlayerIdTemp.Contains(playerId))
			{
				m_PlayerController.m_aLocalEntries.Remove(playerId);
				continue;
			}
		
			if (container.m_SoundSource)
			{
				int maxDistance = m_VONGameModeComponent.GetPlayerVolume(playerId);
				maxDistance *= maxDistance;
				container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
				
				float distance = vector.DistanceSq(container.m_SoundSource.GetOrigin(), m_Camera.GetOrigin());
				if (distance < maxDistance)
					container.m_fDistanceToSender = distance;
				else
					container.m_fDistanceToSender = -1;
				container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
			}
			
		}
		
		foreach (int playerId: m_PlayerIdTemp)
		{
			if (!m_Player)
				continue;
			
			if (playerId == m_PlayerController.GetPlayerId())
				continue;
			
			bool isOtherSpectator = IsPlayerSpectator(playerId);
			bool isOtherListening = m_VONGameModeComponent.IsPlayerListening(playerId);
			//Not usual an issue but when the player is listening to an entity and he swaps to spectator, he goes into null space until he clicks game.
			//Meaning unless we remove his direct voice line here it just stays and he'll never be heard on spectator.;
			IEntity player = m_PlayerManager.GetPlayerControlledEntity(playerId);
			if (!player)
			{
				//Sometimes spectators and players listening are not in eachothers Rpl bubble.
				if ((isLocalSpectator || isListeningToSpectator) && (isOtherSpectator || isOtherListening))
				{
					if (m_PlayerController.m_aLocalEntries.Contains(playerId))
						continue;
					else
					{
						CVON_VONContainer container = new CVON_VONContainer();
						container.m_eVonType = CVON_EVONType.DIRECT;
						container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
						container.m_SenderRplId = RplComponent.Cast(player.FindComponent(RplComponent)).Id();
						container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
						container.m_iPlayerId = playerId;
						container.m_bIsSpectator = (isOtherSpectator || isOtherListening);
						m_PlayerController.m_aLocalEntries.Insert(playerId, container);
					}
				}
				else if (m_PlayerController.m_aLocalEntries.Contains(playerId))
				{
					//If this VON Transmission is radio, don't do shit
					
					if (m_PlayerController.m_aLocalEntries.Get(playerId).m_eVonType == CVON_EVONType.RADIO)
						continue;
					m_PlayerController.m_aLocalEntries.Remove(playerId);
					continue;
				}
				else
					continue;
			}
			
			SCR_CharacterControllerComponent charCont = SCR_CharacterControllerComponent.Cast(ChimeraCharacter.Cast(player).GetCharacterController());
			if (charCont.IsDead() || charCont.IsUnconscious())
				if (m_PlayerController.m_aLocalEntries.Contains(playerId))
				{
					m_PlayerController.m_aLocalEntries.Remove(playerId);
					continue;
				}
				else
					continue;
			
			int maxDistance = m_VONGameModeComponent.GetPlayerVolume(playerId);
			maxDistance *= maxDistance;
			float distance = vector.DistanceSq(player.GetOrigin(), m_Camera.GetOrigin());
			if (distance > maxDistance)
			{
				if ((isLocalSpectator || isListeningToSpectator) && (isOtherSpectator || isOtherListening))
				{
					if (m_PlayerController.m_aLocalEntries.Contains(playerId))
						continue;
					else
					{
						CVON_VONContainer container = new CVON_VONContainer();
						container.m_eVonType = CVON_EVONType.DIRECT;
						container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
						container.m_SenderRplId = RplComponent.Cast(player.FindComponent(RplComponent)).Id();
						container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
						container.m_iPlayerId = playerId;
						container.m_bIsSpectator = (isOtherSpectator || isOtherListening);
						m_PlayerController.m_aLocalEntries.Insert(playerId, container);
					}
				}
				else if (m_PlayerController.m_aLocalEntries.Contains(playerId))
				{
					//If this VON Transmission is radio, don't do shit
					if (m_PlayerController.m_aLocalEntries.Get(playerId).m_eVonType == CVON_EVONType.RADIO)
						continue;
					m_PlayerController.m_aLocalEntries.Remove(playerId);
					continue;
				}
				else
					continue;
			}
			else
			{
				if (m_PlayerController.m_aLocalEntries.Contains(playerId))
						continue;
				else
				{
					CVON_VONContainer container = new CVON_VONContainer();
					container.m_eVonType = CVON_EVONType.DIRECT;
					container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
					container.m_SenderRplId = RplComponent.Cast(player.FindComponent(RplComponent)).Id();
					container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
					container.m_iPlayerId = playerId;
					container.m_bIsSpectator = (isOtherSpectator || isOtherListening);
					m_PlayerController.m_aLocalEntries.Insert(playerId, container);
				}
				
			}
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
			m_PlayerController.BroadcastLocalVONToServer(m_CurrentVONContainer, m_PlayerIdTemp, m_PlayerController.GetPlayerId(), m_CurrentVONContainer.m_iRadioId);
					
		}
		WriteJSON();
	}
}