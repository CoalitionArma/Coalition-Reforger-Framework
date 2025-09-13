class CRF_RespawnManagerClass : ScriptComponentClass {}

class CRF_RespawnManager : ScriptComponent
{
	// Replicated Properties
	[RplProp(onRplName: "WaveRespawnTimer")]
	private int m_iRespawnWaveCurrentTime;
	int m_iRespawnTimer;
	[RplProp()]
	ref array<RplId> m_RespawnPointsRplID = {}; // Used for clients
	
	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	// Store state for UI selection
	bool m_RespawnConfirmed = false;
	RplId m_SelectedSpawnRplID;
	
	//For vehicle respawning, only tracked on the server
	protected ref array<CRF_VehicleSpawner> m_aVehicleSpawners = {};
	
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
	int GetFactionTickets(string faction)
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
		bool changed = SubtractTicketSilent(faction, amount, force);
		if (changed && !m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Subtract tickets silently without triggering replication
	* @param faction Faction to subtract tickets from
	* @param amount Number of tickets to subtract
	* @param force Force subtraction even during safestart
	* @return True if tickets were actually subtracted
	*/
	protected bool SubtractTicketSilent(FactionKey faction, int amount, bool force = false)
	{
		// Don't subtract tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return false;
			
		int currentTickets = GetFactionTickets(faction);
		
		// Only subtract if tickets are positive and not unlimited (-1)
		if (currentTickets <= 0 || currentTickets == -1)
			return false;
			
		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_Gamemode.m_iBLUFORTickets -= amount; break;
			case "OPFOR": m_Gamemode.m_iOPFORTickets -= amount; break;
			case "INDFOR": m_Gamemode.m_iINDFORTickets -= amount; break;
			case "CIV": m_Gamemode.m_iCIVTickets -= amount; break;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void AddTicket(FactionKey faction, int amount, bool force = false)
	{
		bool changed = AddTicketSilent(faction, amount, force);
		if (changed && !m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Add tickets silently without triggering replication
	* @param faction Faction to add tickets to
	* @param amount Number of tickets to add
	* @param force Force addition even during safestart
	* @return True if tickets were actually added
	*/
	protected bool AddTicketSilent(FactionKey faction, int amount, bool force = false)
	{
		// Don't add tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return false;
			
		int currentTickets = GetFactionTickets(faction);
		
		// Only add if tickets are not unlimited (-1)
		if (currentTickets == -1)
			return false;

		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_Gamemode.m_iBLUFORTickets += amount; break;
			case "OPFOR": m_Gamemode.m_iOPFORTickets += amount; break;
			case "INDFOR": m_Gamemode.m_iINDFORTickets += amount; break;
			case "CIV": m_Gamemode.m_iCIVTickets += amount; break;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Batch subtract tickets for multiple operations to minimize replication calls
	* @param ticketChanges Array of ticket changes {faction, amount, force}
	*/
	void BatchSubtractTickets(array<ref array<string>> ticketChanges)
	{
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		foreach (ref array<string> change : ticketChanges)
		{
			if (change.Count() < 2)
				continue;
				
			FactionKey faction = change[0];
			int amount = change[1].ToInt();
			bool force = (change.Count() > 2 && change[2] == "true");
			
			if (SubtractTicketSilent(faction, amount, force))
				anyChanged = true;
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Batch add tickets for multiple operations to minimize replication calls
	* @param ticketChanges Array of ticket changes {faction, amount, force}
	*/
	void BatchAddTickets(array<ref array<string>> ticketChanges)
	{
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		foreach (ref array<string> change : ticketChanges)
		{
			if (change.Count() < 2)
				continue;
				
			FactionKey faction = change[0];
			int amount = change[1].ToInt();
			bool force = (change.Count() > 2 && change[2] == "true");
			
			if (AddTicketSilent(faction, amount, force))
				anyChanged = true;
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
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

		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void CloseSlottingMenu()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu && topMenu.IsInherited(CRF_SlottingMenu))
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
		if (!topMenu.IsInherited(CRF_RespawnMenu) && topMenu.IsInherited(CRF_SpectatorMenu))
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
		
		// Collect all ticket changes for batching
		int totalTicketsToSubtract = 0;

		// Count player respawns first
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
				totalTicketsToSubtract += 1;
		}
		
		// Count vehicle respawns
		int vehicleTicketsToSubtract = 0;
		foreach (CRF_VehicleSpawner vehicle: m_aVehicleSpawners)
		{
			if (vehicle.m_sFactionKey != faction)
				continue;
			
			//Do we have enough tickets and are they not at 0.
			if (GetFactionTickets(faction) != 0 && GetFactionTickets(faction) < vehicle.m_iTicketsPerRespawn)
				continue;
			
			bool shouldRespawn = false;
			//Is the vehicle non existant anymore
			if (!vehicle.m_eVehicle && vehicle.m_bShouldRespawnOnSideRespawn)
			{
				shouldRespawn = true;
			}
			else if (vehicle.m_eVehicle && vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
			{
				SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
				if (vehicleDamageManager.GetState() == EDamageState.DESTROYED)
					shouldRespawn = true;
			}
			
			if (shouldRespawn && TicketsRemaining(faction))
				vehicleTicketsToSubtract += vehicle.m_iTicketsPerRespawn;
		}
		
		// Apply all ticket changes in one batch if any changes needed
		if (totalTicketsToSubtract > 0 || vehicleTicketsToSubtract > 0)
		{
			m_bSuppressReplication = true;
			if (totalTicketsToSubtract > 0)
				SubtractTicketSilent(faction, totalTicketsToSubtract);
			if (vehicleTicketsToSubtract > 0)
				SubtractTicketSilent(faction, vehicleTicketsToSubtract);
			m_bSuppressReplication = false;
			Replication.BumpMe();
		}
		
		// Now perform the actual respawns (without additional ticket operations)
		foreach (int playerId : allPlayers)
		{
			// Skip alive players or not in a slot
			if (!m_SlottingManager.IsPlayerConsideredDead(playerId) || !m_SlottingManager.IsPlayerInASlot(playerId))
				continue;

			// Get player's faction and verify it matches
			Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			if (!playerFaction || playerFaction.GetFactionKey() != faction)
				continue;

			RespawnPlayer(playerId);
		}
		
		//Vehicle respawn logic (without additional ticket operations)
		foreach (CRF_VehicleSpawner vehicle: m_aVehicleSpawners)
		{
			if (vehicle.m_sFactionKey != faction)
				continue;
			
			//Do we have enough tickets and are they not at 0.
			if (GetFactionTickets(faction) != 0 && GetFactionTickets(faction) < vehicle.m_iTicketsPerRespawn)
				continue;
			
			//Is the vehicle non existant anymore
			if (!vehicle.m_eVehicle && vehicle.m_bShouldRespawnOnSideRespawn)
			{
				vehicle.SpawnVehicle();
				continue;
			}
			
			//Vehicle is not vehicling wth
			if (!vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
				continue;
			
			SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
			if (vehicleDamageManager.GetState() != EDamageState.DESTROYED)
				continue;
			
			//Vehicle is destroyed respawn it.
			vehicle.SpawnVehicle();
			continue;
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, vector overrideSpawnLocation[4] = CRF_GamemodeManager.ZERO_SPAWN_VECTOR, int groupID = -1, RplId SpawnRplID = -1)
	{
		// Skip on client
		if (RplSession.Mode() == RplMode.Client)
			return;
		
	    vector spawnLocation[4];
	    spawnLocation[0] = overrideSpawnLocation[0];
	    spawnLocation[1] = overrideSpawnLocation[1];
	    spawnLocation[2] = overrideSpawnLocation[2];
	    spawnLocation[3] = overrideSpawnLocation[3];

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

		// Check if the respawn menu provided a spawn point
		if (SpawnRplID != -1 && !CRF_GamemodeManager.IsValidSpawnVector(spawnLocation[3]))
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
		if (!CRF_GamemodeManager.IsValidSpawnVector(spawnLocation[3]))
			FindSpawnPointLocation(factionKey, spawnLocation);
		
		// Fallback to slot origin 
		if (!CRF_GamemodeManager.IsValidSpawnVector(spawnLocation[3]))
			m_SlottingManager.GetPlayerSlotVector(playerId, spawnLocation);

		// If no spawn location found, enter spectator mode
		if (!CRF_GamemodeManager.IsValidSpawnVector(spawnLocation[3]))
		{
			m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), true);
			m_GamemodeManager.InitilizePlayer(playerId, CRF_GamemodeManager.ZERO_SPAWN_VECTOR);
			return;
		}
		
		// Respawn the player
		int slotID = m_SlottingManager.GetPlayerSlotID(playerId);
		m_SlottingManager.UpdateSlotDeathState(slotID, false);
		m_GamemodeManager.InitilizePlayer(playerId, spawnLocation);
	}
	
	//------------------------------------------------------------------------------------------------
	void FindSpawnPointLocation(FactionKey factionKey, out vector spawnPointLocation[4])
	{
		if (factionKey.IsEmpty())
		{
			spawnPointLocation = CRF_GamemodeManager.ZERO_SPAWN_VECTOR;
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
	
	//Vehicle respawn logic
	//------------------------------------------------------------------------------------------------
	
	int InsertVehicle(CRF_VehicleSpawner spawner)
	{
		return m_aVehicleSpawners.Insert(spawner);
	}
	
	array<CRF_VehicleSpawner> GetVehicleSpawners()
	{
		return m_aVehicleSpawners;
	}
}
