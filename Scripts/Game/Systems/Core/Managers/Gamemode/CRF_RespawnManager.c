class CRF_RespawnManagerClass : ScriptComponentClass {}

class CRF_RespawnManager : ScriptComponent
{

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	// Replicated Properties
	[RplProp(onRplName: "WaveRespawnTimer")]
	private int m_iRespawnWaveCurrentTime;
	float m_fRespawnTimer;
	
	// Ticket System - Moved from CRF_Gamemode for efficient replication
	[RplProp()]
	int m_iBLUFORTickets;
	[RplProp()]
	int m_iOPFORTickets;
	[RplProp()]
	int m_iINDFORTickets;
	[RplProp()]
	int m_iCIVTickets;
	
	[RplProp()]
	bool m_bIsParadropEnabled = true;
	
	//Respawn variables
	[RplProp()] bool m_bCurrentRespawnEnabled;
	[RplProp()] bool m_bCurrentWaveRespawn;
	[RplProp()] int m_iCurrentTimeToRespawn;
	int m_iLocalTimeToRespawn = 0;

	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	// Store state for UI selection
	bool m_RespawnConfirmed = false;
	CRF_SpawnPointData m_SelectedSpawnPoint;
	
	//For vehicle respawning, only tracked on the server
	protected ref array<CRF_VehicleSpawner> m_aVehicleSpawners = {};
	
	protected ref map<int, ref CRF_SpawnPointData> m_mSpawnPointMap = new map<int, ref CRF_SpawnPointData>;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	
	protected bool m_bNeedsRespawn = false;
	protected bool m_bRespawnInit = false;
	
	// Latest Spawn ID used
	protected int m_iLatestSpawnPointID;
	
	// Invoker for data updates
	protected ref ScriptInvoker m_OnSpawnPointsUpdate;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MANAGER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		InitializeManagers();
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
		if (!Replication.IsServer())
			return;

		InitializeTicketsFromGamemode();
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeManagers()
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeRespawnTimers()
	{
		m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
		m_fRespawnTimer = (float)m_iRespawnWaveCurrentTime;
		m_bRespawnInit = true;
	}
	
	//------------------------------------------------------------------------------------------------
	private void InitializeTicketsFromGamemode()
	{
		if (!m_Gamemode)
			return;
			
		// Copy ticket values from gamemode attributes to local replicated properties
		m_iBLUFORTickets = m_Gamemode.m_iBLUFORTickets;
		m_iOPFORTickets = m_Gamemode.m_iOPFORTickets;
		m_iINDFORTickets = m_Gamemode.m_iINDFORTickets;
		m_iCIVTickets = m_Gamemode.m_iCIVTickets;
		m_iCurrentTimeToRespawn = m_Gamemode.m_iTimeToRespawn;
		m_bCurrentWaveRespawn = m_Gamemode.m_bWaveRespawn;
		m_bCurrentRespawnEnabled = m_Gamemode.m_bRespawnEnabled;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 INVOKERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void ForceSpawnPointsUpdated()
	{
		if (m_OnSpawnPointsUpdate)
			m_OnSpawnPointsUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSpawnPointsUpdated()
	{
		if (!m_OnSpawnPointsUpdate)
			m_OnSpawnPointsUpdate = new ScriptInvoker();

		return m_OnSpawnPointsUpdate;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	bool TicketsRemaining(string faction)
	{
		int tickets = GetFactionTickets(faction);
		return (tickets > 0 || tickets == -1);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if respawns are allowed based on time cutoff setting
	//! \return True if respawns are allowed, false if past the cutoff time
	bool IsRespawnTimeAllowed()
	{
		// No cutoff configured (0 = never disable)
		if (m_Gamemode.m_iRespawnCutoffMinutes <= 0)
			return true;
		
		// Check if we're within the cutoff window
		int currentTime = GetGame().GetWorld().GetWorldTime();
		int missionEndTime = CRF_GameTimerManager.GetInstance().m_iTimeMissionEnds;
		int cutoffTime = missionEndTime - (m_Gamemode.m_iRespawnCutoffMinutes * 60000);
		
		// If current time is past the cutoff, disable respawns
		return currentTime < cutoffTime;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetFactionTickets(string faction)
	{
		switch (faction)
		{
			case "BLUFOR": return m_iBLUFORTickets;
			case "OPFOR": return m_iOPFORTickets;
			case "INDFOR": return m_iINDFORTickets;
			case "CIV": return m_iCIVTickets;
		}
		return 0;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GROUP SPAWNPOINT METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void ClearGroupSpawnPoints()
	{
		foreach (int spawnPointId, CRF_SpawnPointData spawnPointData : m_mSpawnPointMap)
			if (spawnPointData && spawnPointData.GetIsTempSpawnPoint())
				UnRegisterRespawnPoint(spawnPointId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TICKET METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks if the player can respawn
	//! \param[in] playerEntity the entity of the player
	//! \param[in] factionKey the faction key of the player
	//! \return True if the player can respawn
	bool CanPlayerResawn(IEntity playerEntity, string factionKey)
	{
		//This is a mess theres got to be a better way, one day we'll find it - Salami
		if (m_bCurrentRespawnEnabled && 
			!CRF_EntityHelper.IsSpectator(playerEntity) && 
			m_Gamemode.m_GamemodeState != CRF_EGamemodeState.AAR && 
			TicketsRemaining(factionKey) &&
			IsRespawnTimeAllowed() &&
			!GetFactionSpawnpoints(factionKey).IsEmpty() &&
			!factionKey.IsEmpty())
				return true;
		
		return false;
	}
	//------------------------------------------------------------------------------------------------
	void SubtractTicket(FactionKey faction, int amount, bool force = false)
	{
		bool changed = SubtractTicketSilent(faction, amount, force);
		if (changed && !m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Subtract tickets silently without triggering replication
	//! \param[in] faction Faction to subtract tickets from
	//! \param[in] amount Number of tickets to subtract
	//! \param[in] force Force subtraction even during safestart
	//! \return True if tickets were actually subtracted
	protected bool SubtractTicketSilent(FactionKey faction, int amount, bool force = false)
	{
		// Don't subtract tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return false;
			
		int currentTickets = GetFactionTickets(faction);
		
		// Don't subtract if tickets are unlimited (-1) or already at 0
		if (currentTickets == -1 || currentTickets <= 0)
			return false;

		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_iBLUFORTickets -= amount;
				if (force && m_iBLUFORTickets < 0)
					m_iBLUFORTickets = 0;
				else if (m_iBLUFORTickets < 0 && m_iBLUFORTickets != -1) 
					m_iBLUFORTickets = 0;
				break;
			
			case "OPFOR": m_iOPFORTickets -= amount;
				if (force && m_iOPFORTickets < 0)
					m_iOPFORTickets = 0;
				else if (m_iOPFORTickets < 0 && m_iOPFORTickets != -1) 
					m_iOPFORTickets = 0;
				break;
			
			case "INDFOR": m_iINDFORTickets -= amount;
				if (force && m_iINDFORTickets < 0)
					m_iINDFORTickets = 0;
				else if (m_iINDFORTickets < 0 && m_iINDFORTickets != -1) 
					m_iINDFORTickets = 0;
				break;
			
			case "CIV": m_iCIVTickets -= amount;
				if (force && m_iCIVTickets < 0)
					m_iCIVTickets = 0;
				else if (m_iCIVTickets < 0 && m_iCIVTickets != -1) 
					m_iCIVTickets = 0;
				break;
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
	//! Add tickets silently without triggering replication
	//! \param[in] faction Faction to add tickets to
	//! \param[in] amount Number of tickets to add
	//! \param[in] force Force addition even during safestart
	//! \return True if tickets were actually added
	protected bool AddTicketSilent(FactionKey faction, int amount, bool force = false)
	{
		// Don't add tickets during safestart
		if (m_SafestartManager.GetSafestartStatus() && !force)
			return false;
			
		int currentTickets = GetFactionTickets(faction);

		// Update the appropriate faction's tickets
		switch (faction)
		{
			case "BLUFOR": m_iBLUFORTickets += amount; break;
			case "OPFOR": m_iOPFORTickets += amount; break;
			case "INDFOR": m_iINDFORTickets += amount; break;
			case "CIV": m_iCIVTickets += amount; break;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Batch subtract tickets for multiple operations to minimize replication calls
	//! \param[in] ticketChanges Array of ticket changes {faction, amount, force}
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
	//! Batch add tickets for multiple operations to minimize replication calls
	//! \param[in] ticketChanges Array of ticket changes {faction, amount, force}
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

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TIMER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	float m_fUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		#ifdef WORKBENCH
		if (!m_bRespawnInit && m_bCurrentRespawnEnabled)
			InitializeRespawnTimers();
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bCurrentWaveRespawn)
				WaveRespawnTimer();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		#else
		if (System.IsConsoleApp())
		{
			if (!m_bRespawnInit && m_bCurrentRespawnEnabled)
				InitializeRespawnTimers();
			if (m_fUpdateBuffer >= 1)
			{
				if (m_bCurrentWaveRespawn)
					WaveRespawnTimer();
				m_fUpdateBuffer = 0;
			}
			m_fUpdateBuffer += timeSlice;
			return;
		}
		#endif
		if (m_fRespawnTimer > 0 || m_bNeedsRespawn)
			RespawnTimer(timeSlice);
	}
	
	//------------------------------------------------------------------------------------------------
	void WaveRespawnTimer()
	{
		// Client-side: Just update local timer display
		if (!Replication.IsServer())
		{
			// Timer value already updated via replication
			// Update local display time if needed
			if (m_iRespawnWaveCurrentTime == 0)
			{
				m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
			}
			return;
		}
		
		// Server-side: Update timer and trigger replication
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			return;

		m_iRespawnWaveCurrentTime--;
		
		if (m_iRespawnWaveCurrentTime == 0)
		{
			m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
			m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
			RespawnAllVehicles();
		}
		
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void RespawnTimer(float timeSlice)
	{
		string factionKey = m_SlottingManager.GetPlayerSlotFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey();
		int tickets = GetFactionTickets(factionKey);
		if (!m_bCurrentRespawnEnabled || GetFactionSpawnpoints(factionKey).IsEmpty())
		{
			GetGame().GetMenuManager().CloseAllMenus();
			m_fRespawnTimer = 0;
			m_SelectedSpawnPoint = null;
			m_RespawnConfirmed = false; 
			m_bNeedsRespawn = false;
			
			CRF_PlayerRplToAuthorityManager.GetInstance().RequestInitilizePlayer(SCR_PlayerController.GetLocalPlayerId());
			return;
		}
		
		// Check if player is actually dead/needs respawn
		bool playerIsDead = m_SlottingManager.IsPlayerConsideredDead(SCR_PlayerController.GetLocalPlayerId());
		if (!playerIsDead)
		{
			// Player is alive, reset respawn state and allow slotting menu
			m_bNeedsRespawn = false;
			m_fRespawnTimer = 0;
			m_SelectedSpawnPoint = null;
			m_RespawnConfirmed = false;
			return;
		}
		
		if (!m_bNeedsRespawn)
			m_bNeedsRespawn = true;
		// Decrease the respawn timer
		if (m_fRespawnTimer > 0)
			m_fRespawnTimer -= timeSlice;
		//Handles adding more time to the players UI if more time is added.
		if (m_iLocalTimeToRespawn != m_iCurrentTimeToRespawn)
		{
			int timeToAdd = m_iCurrentTimeToRespawn - m_iLocalTimeToRespawn;
			m_fRespawnTimer += (float)timeToAdd;
			m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
		}
		CloseSlottingMenu();
		// Check if timer expired or we're in AAR
		bool isTimerExpired = m_fRespawnTimer <= 0;
		bool isGameInAARState = (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.AAR);
		
		if (isTimerExpired || isGameInAARState)
		{
			// Check if Respawn Screen is open
			MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
			if (!topMenu)
				return;
			if (topMenu.IsInherited(CRF_RespawnMenu))
			{
				// Check if respawn selection was confirmed in the UI
				CRF_RespawnMenu respawnMenuUI = CRF_RespawnMenu.Cast(topMenu);
				if (m_SelectedSpawnPoint != null && m_RespawnConfirmed)
				{
					// Reset the timer
					m_fRespawnTimer = (float)m_iRespawnWaveCurrentTime;
					m_iLocalTimeToRespawn = m_iCurrentTimeToRespawn;
					m_bNeedsRespawn = false;
					// Only perform respawn if not in AAR state
					if (!isGameInAARState)
					{
						GetGame().GetMenuManager().CloseAllMenus();
						CRF_PlayerRplToAuthorityManager.GetInstance().RespawnPlayer(SCR_PlayerController.GetLocalPlayerId(), m_SelectedSpawnPoint.GetSpawnPointId());
						
						// Set menu state back to default
						m_SelectedSpawnPoint = null;
						m_RespawnConfirmed = false; 
					}
	
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
	protected void CloseSlottingMenu()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu && topMenu.IsInherited(CRF_SlottingMenu))
		{
			GetGame().GetMenuManager().CloseMenu(topMenu);
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RESPAWN POINT METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	CRF_SpawnPointData GetSpawnPoint(int spawnPointId)
	{
		return m_mSpawnPointMap.Get(spawnPointId);
	}
	
	//------------------------------------------------------------------------------------------------
	void RegisterRespawnPoint(CRF_SpawnPointData spawnPointData, GenericEntity spawnPointEntity)
	{
		if (!spawnPointEntity || !spawnPointData)
			return;

		RplComponent rplComp = RplComponent.Cast(spawnPointEntity.FindComponent(RplComponent));
		if (!rplComp)
			return;
	
		// Retry until RplId is valid
		if (rplComp.Id() == RplId.Invalid())
		{
			GetGame().GetCallqueue().CallLater(RegisterRespawnPoint, 100, false, spawnPointData, spawnPointEntity);
			return;
		};
		
		m_iLatestSpawnPointID++;
		
		spawnPointData.SetSpawnPointEntity(rplComp.Id());
		spawnPointData.SetSpawnPointId(m_iLatestSpawnPointID);
		
		m_mSpawnPointMap.Set(m_iLatestSpawnPointID, spawnPointData);
		m_RplBroadcastManager.UpdateSpawnPointData(spawnPointData);
		SetSpawnId(spawnPointEntity, m_iLatestSpawnPointID);
		
		m_Gamemode.UpdateGenericSpawn();
	}

	//------------------------------------------------------------------------------------------------
	void UnRegisterRespawnPoint(int spawnPointId)
	{	
		if (spawnPointId <= 0)
			return;
		
		CRF_SpawnPointData spawnPointData = GetSpawnPoint(spawnPointId);
		
		if (spawnPointData.GetIsTempSpawnPoint())
			SCR_EntityHelper.DeleteEntityAndChildren(CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity()));
		
		m_mSpawnPointMap.Remove(spawnPointId);
		m_RplBroadcastManager.RemoveSpawnPoint(spawnPointId);
	}
	
	//------------------------------------------------------------------------------------------------
	array<CRF_SpawnPointData> GetFactionSpawnpoints(FactionKey factionKey, SCR_AIGroup group = null)
	{
		array<CRF_SpawnPointData> sideSpawnPoints = {};

		foreach(int spawnPointId, CRF_SpawnPointData spawnPointData : m_mSpawnPointMap)
		{
			if (!spawnPointData 
				|| !spawnPointData.GetIsActiveSpawnPoint()  
				|| spawnPointData.GetIsTempSpawnPoint() 
				|| spawnPointData.GetSpawnPointEntity() == RplId.Invalid()
				|| SCR_Enum.GetEnumName(CRF_EFactions, spawnPointData.GetSpawnPointFaction()) != factionKey)
				continue;
			
			// Filter out group specific spawns
			if (group && (spawnPointData.GetRestrictedToGroup() != "" && spawnPointData.GetRestrictedToGroup() != group.GetCustomNameWithOriginal()))
					continue;

			if (IsDefaultSpawn(spawnPointData))
				sideSpawnPoints.InsertAt(spawnPointData, 0);
			else
				sideSpawnPoints.Insert(spawnPointData);
		}

		return sideSpawnPoints;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SpawnPointData FindInitalFactionSpawnpoint(FactionKey factionKey, SCR_AIGroup group = null)
	{	
		if (group)
		{
			string company, platoon, squad, character, format
			group.GetCallsigns(company, platoon, squad, character, format);
	
			foreach(int spawnPointId, CRF_SpawnPointData spawnPointData : m_mSpawnPointMap)
			{
				if (!spawnPointData 
					|| !spawnPointData.GetIsActiveSpawnPoint() 
					|| !spawnPointData.GetIsTempSpawnPoint()
					|| spawnPointData.GetSpawnPointEntity() == RplId.Invalid()
					|| SCR_Enum.GetEnumName(CRF_EFactions, spawnPointData.GetSpawnPointFaction()) != factionKey)
					continue;
	
				if (CRF_GroupSpawnPoint.Cast(CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity())).m_sCallsignOfGroupToSpawn == squad)
					return spawnPointData;
			}
		};
		
		array<CRF_SpawnPointData> factionSpawnDataArray = GetFactionSpawnpoints(factionKey);

		if (!factionSpawnDataArray.IsEmpty())
			return factionSpawnDataArray.Get(0);
		else	
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsDefaultSpawn(CRF_SpawnPointData spawnPointData)
	{
		IEntity entity = CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
		
		CRF_StaticSpawnPoint staticSpawn = CRF_StaticSpawnPoint.Cast(entity);
		if (staticSpawn && staticSpawn.m_bIsDefaultSpawn)
			return true;
		
		CRF_VehicleSpawnPoint vehSpawn = CRF_VehicleSpawnPoint.Cast(entity);
		if (vehSpawn && vehSpawn.m_bIsDefaultSpawn)
			return true;
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnId(GenericEntity spawnPointEntity, int spawnPointId)
	{		
		CRF_StaticSpawnPoint staticSpawn = CRF_StaticSpawnPoint.Cast(spawnPointEntity);
		if (staticSpawn)
			staticSpawn.SetLocalSpawnPointId(spawnPointId);
		
		CRF_VehicleSpawnPoint vehSpawn = CRF_VehicleSpawnPoint.Cast(spawnPointEntity);
		if (vehSpawn)
			vehSpawn.SetLocalSpawnPointId(spawnPointId);
		
		CRF_RallyPoint rallyPoint = CRF_RallyPoint.Cast(spawnPointEntity);
		if (rallyPoint)
			rallyPoint.SetLocalSpawnPointId(spawnPointId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RESPAWN METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
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
			
			// Check if we have enough tickets to respawn this vehicle
			int factionTickets = GetFactionTickets(faction);
			// Skip if we're out of tickets (but allow if unlimited -1)
			if (factionTickets == 0)
				continue;
			// Skip if we don't have enough tickets (but allow if unlimited -1)
			if (factionTickets > 0 && factionTickets < vehicle.m_iTicketsPerRespawn)
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
		
		RespawnSideVehicles(faction);
	}
	
	//------------------------------------------------------------------------------------------------
	void RespawnAllVehicles()
	{
		//Makes my life 20x easier
		array<string> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string faction: factionKeys)
		{
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
					CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
					continue;
				}
				
				//Vehicle is not vehicling wth
				if (!vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
					continue;
				
				SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
				if (vehicleDamageManager.GetState() != EDamageState.DESTROYED)
					continue;
				
				//Vehicle is destroyed respawn it.
				CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
				continue;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void RespawnSideVehicles(FactionKey faction)
	{
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
				CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
				continue;
			}
			
			//Vehicle is not vehicling wth
			if (!vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent))
				continue;
			
			SCR_VehicleDamageManagerComponent vehicleDamageManager = SCR_VehicleDamageManagerComponent.Cast(vehicle.m_eVehicle.FindComponent(SCR_VehicleDamageManagerComponent));
			if (vehicleDamageManager.GetState() != EDamageState.DESTROYED)
				continue;
			
			//Vehicle is destroyed respawn it.
			CRF_VehicleGearscriptManager.GetInstance().SpawnVehicle(vehicle);
			continue;
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, int spawnPointID = -1, RplId entityRplID = RplId.Invalid())
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

		// If no spawn location found, enter spectator mode
		if (GetFactionSpawnpoints(factionKey).IsEmpty())
		{
			m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), true);
			m_GamemodeManager.InitilizePlayer(playerId);
			return;
		}
		
		// Respawn the player
		int slotID = m_SlottingManager.GetPlayerSlotID(playerId);
		m_SlottingManager.UpdateSlotDeathState(slotID, false);
		m_GamemodeManager.InitilizePlayer(playerId, spawnPointID, entityRplID);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS/MISC
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Getter for the current wave timer
	int GetCurrentWaveTimer()
	{
		return m_iRespawnWaveCurrentTime;
	}

	//------------------------------------------------------------------------------------------------
	int InsertVehicle(CRF_VehicleSpawner spawner)
	{
		return m_aVehicleSpawners.Insert(spawner);
	}
	
	//------------------------------------------------------------------------------------------------
	array<CRF_VehicleSpawner> GetVehicleSpawners()
	{
		return m_aVehicleSpawners;
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleRespawnWave()
	{
		m_bCurrentWaveRespawn = !m_bCurrentWaveRespawn;
		m_iRespawnWaveCurrentTime = m_iCurrentTimeToRespawn;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleParadrop()
	{
		m_bIsParadropEnabled = !m_bIsParadropEnabled;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetRespawnTime(int seconds)
	{
		m_iCurrentTimeToRespawn = seconds;
		m_iRespawnWaveCurrentTime = seconds;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleRespawn()
	{
		m_bCurrentRespawnEnabled = !m_bCurrentRespawnEnabled;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Explicitly enable or disable voluntary player respawn.
	//! Unlike ToggleRespawn, this is idempotent — calling SetRespawnEnabled(false) twice
	//! does not accidentally re-enable respawn. Used by PropHunt to suppress mid-round
	//! respawn screens while keeping the forced RespawnAllSides() round-reset path intact.
	void SetRespawnEnabled(bool enabled)
	{
		if (m_bCurrentRespawnEnabled == enabled)
			return;
		m_bCurrentRespawnEnabled = enabled;
		Replication.BumpMe();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CLIENT SIDE REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Update single spawn point from RPC (called by CRF_RplBroadcastManager)
	//! Only updates if data actually changed (prevents unnecessary UI rebuilds)
	void UpdateSpawnPointDataClient(CRF_SpawnPointData spawnPointData)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		int spawnPointId = spawnPointData.GetSpawnPointId();
		CRF_SpawnPointData oldSpawnPointData = m_mSpawnPointMap.Get(spawnPointId);

		if(!oldSpawnPointData)
			m_mSpawnPointMap.Set(spawnPointId, spawnPointData);
		else
			oldSpawnPointData.DataUpdate(spawnPointData);
		
		ForceSpawnPointsUpdated();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Client-side: Remove spawn point from RPC (called by CRF_RplBroadcastManager)
	void RemoveSpawnPointClient(int spawnPointId)
	{
		if (Replication.IsServer())
			return;  // Server doesn't receive these, only sends
		
		CRF_SpawnPointData spawnPointData = GetSpawnPoint(spawnPointId);
		spawnPointData.SetSpawnPointActive(false);
		
		m_mSpawnPointMap.Remove(spawnPointId);
		
		ForceSpawnPointsUpdated();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override protected bool RplSave(ScriptBitWriter writer)
	{
		// Save spawnPointData
		int spawnPointCount = m_mSpawnPointMap.Count();
		writer.WriteInt(spawnPointCount);
		foreach (int spawnPointId, CRF_SpawnPointData spawnPointData : m_mSpawnPointMap)
		{
			spawnPointData.Save(writer);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override protected bool RplLoad(ScriptBitReader reader)
	{
		// Load spawnPointData
		int spawnPointCount;
		reader.ReadInt(spawnPointCount);
		for (int i = 0; i < spawnPointCount; i++)
		{
			CRF_SpawnPointData spawnPointData = new CRF_SpawnPointData();
			spawnPointData.Load(reader);
			UpdateSpawnPointDataClient(spawnPointData);
		}

		return true;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_RespawnManager m_sInstance;
	void CRF_RespawnManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_RespawnManager GetInstance()
	{
		return m_sInstance;
	}
}