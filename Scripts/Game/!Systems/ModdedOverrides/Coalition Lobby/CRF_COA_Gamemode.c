modded class COA_Gamemode
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Vehicle Gearscript Enable/Disable per Side
	//------------------------------------------------------------------------------------
	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for BLUFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bBLUFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for OPFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bOPFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for INDFOR vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bINDFORVehicleGearscriptEnabled;

	[Attribute("true", UIWidgets.CheckBox, desc: "Enable vehicle gearscript for CIV vehicles", category: "CRF Gearscript Settings - Advanced")]
	bool m_bCIVILIANVehicleGearscriptEnabled;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected CRF_LoggingManager m_LoggingManager;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 INITIALIZATION AND SETUP
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initialize the gamemode and all required manager instances
	//! \param[in] owner The entity that owns this component
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Load configs on dedicated server
		if (RplSession.Mode() == RplMode.Dedicated) {
			CRF_DonatorConfig.LoadConfig();
			CRF_BugReportConfig.LoadConfig();

			// Initialize sight arsenal registry for optimized RPC
			CRF_SightArsenalRegistry.InitializeRegistry();
		};
		
		m_LoggingManager = CRF_LoggingManager.GetInstance();
	}

	//------------------------------------------------------------------------------------------------
	//! Handle gamemode state changes
	//! Triggers UI updates and state-specific logic
	protected override void OnGamemodeStateChanged()
	{
		// Server-side state change handling
		if (Replication.IsServer())
		{
			// Invoke state changed (invoker already initialized in constructor)
			m_OnStateChanged.Invoke();
			
			// Set basic game mode states for basegamemode
			// useful for default components that reference it like datacollector
			switch (m_GamemodeState) {
				case COA_EGamemodeState.SLOTTING: {
					// Clear reconnect tracking — a new game cycle means all previous
					// disconnect records are no longer valid (slots may be reassigned).
					m_mReconnectSlotByGuid.Clear();
					break;
				}
				
				case COA_EGamemodeState.GAME: {
					SetGameState(SCR_EGameModeState.GAME);
					ApplyMissionTimeScale();
					foreach (Vehicle vehicle : CRF_VehicleGearscriptManager.GetInstance().GetSpawnedVehicleArray())
					{
						if (!vehicle)
							continue;
						vehicle.SpawnVehiclePassengers();
					}
					break;
				}
				
				case COA_EGamemodeState.AAR: {
					// Flush all player stats BEFORE SetGameState
					CRF_ServerStatsManager statsManager = CRF_ServerStatsManager.GetInstance();
					if (statsManager)
						statsManager.NotifyMissionEnded();

					SetGameState(SCR_EGameModeState.POSTGAME);

					// Open the outro screen on all clients, passing winning faction so clients can display it
					COA_RplBroadcastManager rplBroadcastManager = COA_RplBroadcastManager.GetInstance();
					if (rplBroadcastManager)
					{
						string winningFaction = "";
						CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
						if (loggingManager)
							winningFaction = loggingManager.GetWinningFaction();
						rplBroadcastManager.BroadcastOutro(winningFaction);
					}
					break;
				}
			}	
		}
		
		COA_PlayerMenuManager playerMenuManager = COA_PlayerMenuManager.GetInstance();
		if (playerMenuManager)
			playerMenuManager.OpenCurrentStateMenu();
	}

	//------------------------------------------------------------------------------------------------
	//! Process player connection after authentication
	//! \param[in] iPlayerID ID of the connecting player
	protected override void OnPlayerAuditSuccess(int iPlayerID)
	{
		vanilla.OnPlayerAuditSuccess(iPlayerID);
		
		// Skip processing on client
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		// Reconnect restore: if this player has a pending GUID entry, force-update their slot's
		// player ID before InitilizePlayer runs so IsPlayerInASlot() finds the correct slot.
		// This handles dedicated-server scenarios where a reconnecting player may get a new
		// numeric player ID but the GUID (BI account identity) remains the same.
		if (IsMaster() && m_SlottingManager)
		{
			string reconnectGuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(iPlayerID);
			int savedSlotId;
			if (!reconnectGuid.IsEmpty() && m_mReconnectSlotByGuid.Find(reconnectGuid, savedSlotId))
			{
				m_mReconnectSlotByGuid.Remove(reconnectGuid);
				m_SlottingManager.ForceUpdateSlotPlayerID(savedSlotId, iPlayerID);
			}
		}
		
		QueuePlayerInitialization(iPlayerID);

		// Get player's BI account GUID for privilege checks
		string playerGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(iPlayerID);
		
		// Check if player is the mission designer and grant admin chat
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		
		if (missionHeader && missionHeader.m_sAuthorGUID && !missionHeader.m_sAuthorGUID.IsEmpty() && !playerGUID.IsEmpty())
		{
			// Compare player's BI account GUID with mission author's GUID
			if (playerGUID == missionHeader.m_sAuthorGUID)
			{
				// Grant session admin (admin chat) to mission designer
				GetGame().GetPlayerManager().GivePlayerRole(iPlayerID, EPlayerRole.SESSION_ADMINISTRATOR);
			}
		}

		// Check if player is a moderator/donator and set privileges
		if (!playerGUID.IsEmpty()) {
			if (COA_ModeratorConfig.IsModerator(playerGUID))
				m_PermissionManager.SetPlayerStatus(iPlayerID, "mod");
			
			if (CRF_DonatorConfig.IsDonator(playerGUID))
				m_PermissionManager.SetPlayerStatus(iPlayerID, "don");
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS/UPDATERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	bool DoesFactionShareMarker(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": 	return m_BLUFORGearScriptSettings.m_bEnableShareableMarkers;
			case "OPFOR": 	return m_OPFORGearScriptSettings.m_bEnableShareableMarkers;
			case "INDFOR": 	return m_INDFORGearScriptSettings.m_bEnableShareableMarkers;
			case "CIV": 	return m_CIVILIANGearScriptSettings.m_bEnableShareableMarkers;
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns true when the vehicle gearscript system is enabled for the given faction.
	//! Returns true by default for unknown factions.
	//! \param[in] factionKey Faction identifier (BLUFOR, OPFOR, INDFOR, CIV)
	bool IsVehicleGearscriptEnabled(FactionKey factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return m_bBLUFORVehicleGearscriptEnabled;
			case "OPFOR":  return m_bOPFORVehicleGearscriptEnabled;
			case "INDFOR": return m_bINDFORVehicleGearscriptEnabled;
			case "CIV":    return m_bCIVILIANVehicleGearscriptEnabled;
		}
		return true;
	}
}