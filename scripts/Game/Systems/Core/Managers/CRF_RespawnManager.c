class CRF_RespawnManagerClass : ScriptComponentClass {}

class CRF_RespawnManager : ScriptComponent
{
	// Replicated Properties
	[RplProp(onRplName: "WaveRespawnTimer")]
	private int m_iRespawnWaveCurrentTime;
	int m_iRespawnTimer;
	[RplProp()]
	ref array<RplId> m_RespawnPointsRplID = {}; // Used for clients
	
	// Store state for UI selection
	bool m_RespawnConfirmed = false;
	RplId m_SelectedSpawnRplID;
	
	// Protected Member Variables
	protected ref array<IEntity> m_aRespawnPoints = {}; // Used for server
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_SlottingManager m_SlottingManager;
	
	ref ScriptInvoker m_OnRespawnPointStateChanged = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	// Singleton accessor
	static CRF_RespawnManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
			
		return CRF_RespawnManager.Cast(gameMode.FindComponent(CRF_RespawnManager));
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		InitializeManagers();
		
		if (!Replication.IsServer())
			return;

		InitializeRespawnTimers();
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeManagers()
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeRespawnTimers()
	{
		// Skip if respawn is disabled
		if (!m_Gamemode.m_bRespawnEnabled)
			return;
			
		m_iRespawnWaveCurrentTime = m_Gamemode.m_iTimeToRespawn;
		m_iRespawnTimer = m_iRespawnWaveCurrentTime;

		// Start wave respawn timer if enabled and not in client mode
		if (m_Gamemode.m_bWaveRespawn && RplSession.Mode() != RplMode.Client)
			GetGame().GetCallqueue().CallLater(WaveRespawnTimer, 1000, true);
	}

	//------------------------------------------------------------------------------------------------
	bool TicketsRemaining(string faction)
	{
		int tickets = GetFactionTickets(faction);
		return (tickets > 0 || tickets == -1);
	}
	
	//------------------------------------------------------------------------------------------------
	private int GetFactionTickets(string faction)
	{
		switch (faction)
		{
			case "BLUFOR": return m_Gamemode.m_iBLUFORTickets;
			case "OPFOR": return m_Gamemode.m_iOPFORTickets;
			case "INDFOR": return m_Gamemode.m_iINDFORTickets;
			case "CIV": return m_Gamemode.m_iCIVTickets;
		}
		return 0;
	}

	//------------------------------------------------------------------------------------------------
	void SubtractTicket(FactionKey faction, int amount, bool force = false)
	{
		// Don't subtract tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return;
			
		int currentTickets = GetFactionTickets(faction);
		
		// Only subtract if tickets are positive and not unlimited (-1)
		if (currentTickets <= 0 || currentTickets == -1)
			return;
			
		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_Gamemode.m_iBLUFORTickets -= amount; break;
			case "OPFOR": m_Gamemode.m_iOPFORTickets -= amount; break;
			case "INDFOR": m_Gamemode.m_iINDFORTickets -= amount; break;
			case "CIV": m_Gamemode.m_iCIVTickets -= amount; break;
		}
		
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void AddTicket(FactionKey faction, int amount, bool force = false)
	{
		// Don't add tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return;
			
		int currentTickets = GetFactionTickets(faction);
		
		// Only subtract if tickets are not unlimited (-1)
		if (currentTickets == -1)
			return;

		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_Gamemode.m_iBLUFORTickets += amount; break;
			case "OPFOR": m_Gamemode.m_iOPFORTickets += amount; break;
			case "INDFOR": m_Gamemode.m_iINDFORTickets += amount; break;
			case "CIV": m_Gamemode.m_iCIVTickets += amount; break;
		}
		
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void WaveRespawnTimer()
	{
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			return;

		m_iRespawnWaveCurrentTime--;
		
		if (m_iRespawnWaveCurrentTime == 0)
			m_iRespawnWaveCurrentTime = m_Gamemode.m_iTimeToRespawn;

		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void CloseSlottingMenu()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu && topMenu.IsInherited(CRF_SlottingMenuUI))
		{
			GetGame().GetMenuManager().CloseMenu(topMenu);
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnTimer()
	{
		// Decrease the respawn timer
		if (m_iRespawnTimer > 0)
			m_iRespawnTimer--;

		// Check if timer expired or we're in AAR
		bool isTimerExpired = m_iRespawnTimer <= 0;
		bool isGameInAARState = (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.AAR);
		
		if (isTimerExpired || isGameInAARState)
		{
			// Check if Respawn Screen is open
			MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
			if (topMenu.IsInherited(CRF_RespawnMenu))
			{
				// Check if respawn selection was confirmed in the UI
				CRF_RespawnMenu respawnMenuUI = CRF_RespawnMenu.Cast(topMenu);
				if (m_SelectedSpawnRplID != -1 && m_RespawnConfirmed)
				{
					// Reset the timer
					m_iRespawnTimer = m_iRespawnWaveCurrentTime;
					// Only perform respawn if not in AAR state
					if (!isGameInAARState)
					{
						GetGame().GetCallqueue().Remove(CloseSlottingMenu);
						GetGame().GetMenuManager().CloseAllMenus();
						CRF_RplToAuthorityManager.GetInstance().RespawnPlayer(SCR_PlayerController.GetLocalPlayerId(), m_SelectedSpawnRplID);
						
						// Set menu state back to default
						m_SelectedSpawnRplID = -1;
						m_RespawnConfirmed = false; 
					}
		
					// Remove this timer function from the callqueue
					GetGame().GetCallqueue().Remove(RespawnTimer);
					return;
				}
			}
		}

		// Handle respawn menu
		ShowRespawnMenuIfNeeded();
	}
	
	//------------------------------------------------------------------------------------------------
	private void ShowRespawnMenuIfNeeded()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		
		if (!topMenu)
			return;
			
		// If we're in spectator but not in respawn menu, open respawn menu
		if (!topMenu.IsInherited(CRF_RespawnMenu) && topMenu.IsInherited(CRF_SpectatorMenuUI))
		{
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RespawnMenu);
		}
	}

	//------------------------------------------------------------------------------------------------
	void RegisterRespawnPoint(IEntity respawnPoint)
	{
		if (!respawnPoint)
			return;

		RplComponent rplComp = RplComponent.Cast(respawnPoint.FindComponent(RplComponent));
		if (!rplComp)
			return;
	
		// Retry until RplId is valid
		if (rplComp.Id() == RplId.Invalid())
		{
			GetGame().GetCallqueue().CallLater(RegisterRespawnPoint, 100, false, respawnPoint);
			return;
		}

		// Store Respawnpoints and sync with client
		m_aRespawnPoints.Insert(respawnPoint);
		m_RespawnPointsRplID.Insert(rplComp.Id());
		
		// Broadcast UI updates to clients
		CRF_RplBroadcastManager.GetInstance().SendRespawnScreenUpdate(rplComp.Id(), true);
	}

	//------------------------------------------------------------------------------------------------
	void UnRegisterRespawnPoint(IEntity respawnPoint)
	{
		if (!respawnPoint)
			return;
			
		int index = m_aRespawnPoints.Find(respawnPoint);
		if (index != -1)
			m_aRespawnPoints.Remove(index);
		
		RplId rplID = GetSpawnRplIDFromEntity(respawnPoint);
		m_RespawnPointsRplID.RemoveItemOrdered(rplID);
		
		// Broadcast UI updates to clients
		CRF_RplBroadcastManager.GetInstance().SendRespawnScreenUpdate(rplID, false);
	}
	
	//------------------------------------------------------------------------------------------------
	array<IEntity> GetFactionSpawnpoints(FactionKey faction)
	{
		array<IEntity> sideRespawnPoints = {};
		
		foreach(IEntity point : m_aRespawnPoints)
		{
			if (point == null)
				continue;

			CRF_RespawnPointComponent respawnComponent = CRF_RespawnPointComponent.Cast(point.FindComponent(CRF_RespawnPointComponent));

			if (!respawnComponent)
				continue;

			if (respawnComponent.m_sRespawnPointFaction != faction)
				continue;

			if (!respawnComponent.m_bActiveRespawnPoint)
				continue;

			sideRespawnPoints.Insert(point)
		}
		
		return sideRespawnPoints;
	}

	//------------------------------------------------------------------------------------------------
	void RespawnAllSides()
	{
		// Respawn each faction
		RespawnSide("BLUFOR");
		RespawnSide("INDFOR");
		RespawnSide("OPFOR");
		RespawnSide("CIV");
	}

	//------------------------------------------------------------------------------------------------
	void RespawnSide(FactionKey faction)
	{
		array<int> allPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayers);

		foreach (int playerId : allPlayers)
		{
			// Skip alive players or not in a slot
			if (!m_SlottingManager.IsPlayerConsideredDead(playerId) || !m_SlottingManager.IsPlayerInASlot(playerId))
				continue;

			// Get player's faction and verify it matches
			Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			if (!playerFaction || playerFaction.GetFactionKey() != faction)
				continue;

			// Check if tickets are available
			if (TicketsRemaining(faction)) 
				SubtractTicket(faction, 1);
			
			RespawnPlayer(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, vector spawnLocation[4] = {"0 0 0", "0 0 0", "0 0 0", "0 0 0"}, int groupID = -1, RplId SpawnRplID = -1)
	{
		// Skip on client
		if (RplSession.Mode() == RplMode.Client)
			return;

		// Validate player
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		if (!playerManager.IsPlayerConnected(playerId))
			return;

		// Get player faction
		FactionKey factionKey = "";
		Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		if (playerFaction)
			factionKey = playerFaction.GetFactionKey();
			
		if (factionKey.IsEmpty())
			return;
		
		// Determine spawn location
		vector finalSpawnLocation[4];

		// Check if the respawn menu provided a spawn point
		if (SpawnRplID != -1 && spawnLocation[3] == vector.Zero)
		{
			RplComponent rplComp = RplComponent.Cast(Replication.FindItem(SpawnRplID));
			if (rplComp)
			{
				IEntity point = rplComp.GetEntity();
				CRF_RespawnPointComponent respawnComponent = CRF_RespawnPointComponent.Cast(point.FindComponent(CRF_RespawnPointComponent));
			
				if (respawnComponent.m_bActiveRespawnPoint)
					spawnLocation[3] = point.GetOrigin();
			}
		}
		
		// Use provided spawn location or fall back to factions default spawn
		if (spawnLocation[3] == vector.Zero)
			FindSpawnPointLocation(factionKey, spawnLocation);
		
		// Fallback to slot origin 
		if (spawnLocation[3] == vector.Zero)
			m_SlottingManager.GetPlayerSlotVector(playerId, spawnLocation);

		// If no spawn location found, enter spectator mode
		if (spawnLocation[3] == vector.Zero)
		{
			m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), true);
			m_GamemodeManager.InitilizePlayer(playerId);
			return;
		}

		// Find a valid spawn position
		vector validSpawnPos = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(validSpawnPos, spawnLocation[3], 10);
		finalSpawnLocation[3] = validSpawnPos;
		
		// Respawn the player
		int slotID = m_SlottingManager.GetPlayerSlotID(playerId);
		m_SlottingManager.UpdateSlotDeathState(slotID, false);
		m_GamemodeManager.InitilizePlayer(playerId, finalSpawnLocation);
	}
	
	//------------------------------------------------------------------------------------------------
	void FindSpawnPointLocation(FactionKey factionKey, out vector spawnPointLocation[4])
	{
		if (factionKey.IsEmpty())
		{
			CRF_GamemodeManager.SetVectorZero(spawnPointLocation);
			return;
		};
		
		foreach (IEntity spawnPoint : m_aRespawnPoints)
		{
			if (!spawnPoint)
				continue;

			CRF_RespawnPointComponent respawnComponent = CRF_RespawnPointComponent.Cast(spawnPoint.FindComponent(CRF_RespawnPointComponent));
			if (!respawnComponent)
				continue;

			if (respawnComponent.m_sRespawnPointFaction != factionKey)
				continue;

			if (!respawnComponent.m_bActiveRespawnPoint)
				continue;

			spawnPoint.GetWorldTransform(spawnPointLocation);
			break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker OnRespawnPointStateChanged()
	{
		return m_OnRespawnPointStateChanged;
	}
	
	/**
	 * Getter for the current wave timer
	 */
	int GetCurrentWaveTimer()
	{
		return m_iRespawnWaveCurrentTime;
	}
	
	/**
	 * Helper for converting Spawnpoint RplID to Entity
	 */
	IEntity GetSpawnEntityFromRplID(RplId rplID)
	{
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(rplID));
		if (!rplComp)
			return null;

		return rplComp.GetEntity();
	}
	
	/**
	 * Helper for converting Spawnpoint Entity to RplID
	 */
	RplId GetSpawnRplIDFromEntity(IEntity point)
	{
		RplComponent rplComp = RplComponent.Cast(point.FindComponent(RplComponent));
		if (!rplComp)
			return -1;
		
		return rplComp.Id();
	}
}
