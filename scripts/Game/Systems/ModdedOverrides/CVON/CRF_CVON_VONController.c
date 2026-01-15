modded class SCR_VONController
{
	ref map<int, bool> m_SpectatorChecks = new map<int, bool>;
    int m_iLastChannelChanges = -1; // Track last known channel change count
	CRF_MenuManager m_CRFMenuManager;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_CRFMenuManager = CRF_MenuManager.GetInstance();
	}
    
    // Only update when something actually changes
    void UpdateSpectatorChecksIfNeeded()
    {
        if (!m_CRFMenuManager)
            return;
            
        // Check if channels have changed
        if (m_iLastChannelChanges != m_CRFMenuManager.m_iChannelChanges)
        {
            m_iLastChannelChanges = m_CRFMenuManager.m_iChannelChanges;
            UpdateSpectatorChecks(); // Only update when channels change
        }
    }
    
    // Cache individual player spectator status - update on demand
    bool SpectatorCheck(int playerId)
    {
        // Check if we have cached data
        if (m_SpectatorChecks.Contains(playerId))
            return m_SpectatorChecks.Get(playerId);
        
        // Not cached - calculate and cache it
        bool result = CalculateSpectatorStatus(playerId);
        m_SpectatorChecks.Insert(playerId, result);
        return result;
    }
    
    bool CalculateSpectatorStatus(int playerId)
    {
        bool isPlayerAndClientSpec = IsOtherPlayerSpectator(playerId);
        bool inSameChannel = InSameChannel(playerId);
        return isPlayerAndClientSpec && inSameChannel;
    }
	
	void UpdateSpectatorChecks()
	{
		m_SpectatorChecks.Clear();
		array<int> playerIds = {};
		m_PlayerManager.GetPlayers(playerIds);
		foreach (int playerId: playerIds)
		{
			bool isPlayerAndClientSpec = IsOtherPlayerSpectator(playerId);
			bool inSameChannel = InSameChannel(playerId);

			if (isPlayerAndClientSpec && inSameChannel)
				m_SpectatorChecks.Insert(playerId, true);
			else
				m_SpectatorChecks.Insert(playerId, false);
		}
	}
	
	bool IsOtherPlayerSpectator(int playerId)
	{
		if (playerId == 0)
			return false;

		if (!m_FactionManager.GetPlayerFaction(playerId))
			return false;
		
		string otherFactionKey = m_FactionManager.GetPlayerFaction(playerId).GetFactionKey();
		bool isOtherPlayerSpec = otherFactionKey == "SPEC" || otherFactionKey == "SPEC" || m_VONGameModeComponent.IsPlayerListening(playerId);
		
		return isOtherPlayerSpec;
	}
	
	bool InSameChannel(int playerId)
	{
		int frequency = CRF_MenuManager.GetInstance().GetChannel(m_PlayerController.GetPlayerId());
		int senderFrequency = CRF_MenuManager.GetInstance().GetChannel(playerId);
		
		//Are we deafened
		if (frequency == 0)
			return false;
		
		if (frequency == senderFrequency)
			return true;
		else
			return false;
	}
	
	override void ActivateCVON(CVON_EVONTransmitType transmitType = CVON_EVONTransmitType.NONE)
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			if(topMenu.IsInherited(CRF_Outro))
				return;
		
		super.ActivateCVON(transmitType);
	}
	
	override bool ShouldMuffleAudio(IEntity senderEntity, int playerId = 0, out int loweredDecibles = 0)
	{
		if (SpectatorCheck(playerId))
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
	
	override void ComputeSpectatorLR(int playerId, out float outLeft = 1, out float outRight = 1, out int silencedDecibels = 0)
	{
		if (CRF_Gamemode.GetInstance().m_bIsInEndCredits)
		{
			outLeft = 0;
			outRight = 0;
			return;
		}
		
		if (SpectatorCheck(playerId))
		{
			outLeft = 1;
			outRight = 1;
			silencedDecibels = 0;
			return;
		}
	}
	
	float m_fSpecCheckBuffer = 0;
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
			
			if (container.m_bIsSpectator)
			{
				if (!SpectatorCheck(playerId))
					m_PlayerController.m_aLocalEntries.Remove(playerId);
			}
		
			if (container.m_SoundSource)
			{
				int volume = m_VONGameModeComponent.GetPlayerVolume(playerId);
				int maxDistance = volume;
				maxDistance *= maxDistance;
				container.m_iVolume = volume;
				
				float distance = vector.DistanceSq(container.m_SoundSource.GetOrigin(), m_Camera.GetOrigin());
				if (distance < maxDistance)
					container.m_fDistanceToSender = distance;
				else
					container.m_fDistanceToSender = -1;
			}
			
		}
		
		bool isLocalSpectator = SpectatorCheck(m_PlayerController.GetPlayerId());
		if (m_fSpecCheckBuffer >= 0.1)
		{
			m_fSpecCheckBuffer = 0;
			UpdateSpectatorChecksIfNeeded();
		}
		else
			m_fSpecCheckBuffer += timeSlice;
		foreach (int playerId: m_PlayerIdTemp)
		{
			if (!m_Player)
				continue;
			
			if (playerId == m_PlayerController.GetPlayerId())
				continue;

			bool isOtherSpectator = SpectatorCheck(playerId);
			//Not usual an issue but when the player is listening to an entity and he swaps to spectator, he goes into null space until he clicks game.
			//Meaning unless we remove his direct voice line here it just stays and he'll never be heard on spectator.;
			IEntity player = m_PlayerManager.GetPlayerControlledEntity(playerId);
			if (!player)
			{
				//Sometimes spectators and players listening are not in eachothers Rpl bubble.
				if (isLocalSpectator && isOtherSpectator)
				{
					if (m_PlayerController.m_aLocalEntries.Contains(playerId))
						continue;
					else
					{
						CVON_VONContainer container = new CVON_VONContainer();
						container.m_eVonType = CVON_EVONType.DIRECT;
						container.m_iVolume = m_VONGameModeComponent.GetPlayerVolume(playerId);
						container.m_iClientId = m_PlayerController.GetPlayersTeamspeakClientId(playerId);
						container.m_iPlayerId = playerId;
						container.m_bIsSpectator = isOtherSpectator;
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
					if (isLocalSpectator && isOtherSpectator)
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
							container.m_bIsSpectator = isOtherSpectator;
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
						container.m_bIsSpectator = isOtherSpectator;
						m_PlayerController.m_aLocalEntries.Insert(playerId, container);
					}
					
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
		}
		
		if (!m_bHasBroadcasted && m_bIsBroadcasting)
		{
			m_PlayerController.BroadcastLocalVONToServer(m_CurrentVONContainer, m_PlayerController.GetPlayerId(), m_CurrentVONContainer.m_iRadioId);
			m_bHasBroadcasted = true;
		}
		
		//Our plugin only checks every 50ms
		if (m_fVONSaveBuffer >= 0.05)
		{
			WriteJSON();
			m_fVONSaveBuffer = 0;
		}
		else m_fVONSaveBuffer += timeSlice;
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
		
		if (SpectatorCheck(playerId))
		{
			outLeft = 1;
			outRight = 1;
			return;
		}
		
		super.ComputeStereoLR(listener, sourcePos, volume_m, playerId, outLeft, outRight, silencedDecibels, rearPanBoost, rearShadow, elevNarrow, bleed, normalizePeak);
	}
}