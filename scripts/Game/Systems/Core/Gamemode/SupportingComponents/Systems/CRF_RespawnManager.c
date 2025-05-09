class CRF_RespawnManagerClass : ScriptComponentClass {}

class CRF_RespawnManager : ScriptComponent
{
	[RplProp(onRplName: "WaveRespawnTimer")]
	int m_iRespawnWaveCurrentTime;

	int m_iRespawnTimer;
	
	protected ref array<IEntity> m_aRespawnPoints = {};
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_GamemodeManager m_GamemodeManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_SlottingManager m_SlottingManager;

	//------------------------------------------------------------------------------------------------
	static CRF_RespawnManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_RespawnManager.Cast(gameMode.FindComponent(CRF_RespawnManager));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Get all instances we need for this manager.
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		
		if (!Replication.IsServer())
			return;

		// If respawn is enabled
		if (m_Gamemode.m_bRespawnEnabled)
		{
			m_iRespawnWaveCurrentTime = m_Gamemode.m_iTimeToRespawn;
			m_iRespawnTimer = m_iRespawnWaveCurrentTime;
	
			// If wave respawn is enabled and we're in a non-client enviorment
			if (m_Gamemode.m_bWaveRespawn && RplSession.Mode() != RplMode.Client)
				// Start wave respawn timer
				GetGame().GetCallqueue().CallLater(WaveRespawnTimer, 1000, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	bool TicketsRemaining(string faction)
	{
		bool result = false;
		switch (faction)
		{
			case "BLUFOR" : {
				if (m_Gamemode.m_iBLUFORTickets > 0 || m_Gamemode.m_iBLUFORTickets == -1)
					result = true;
				break;
			};
			case "OPFOR" : {
				if (m_Gamemode.m_iOPFORTickets > 0 || m_Gamemode.m_iOPFORTickets == -1)
					result = true;
				break;
			}
			case "INDFOR" : {
				if (m_Gamemode.m_iINDFORTickets > 0 || m_Gamemode.m_iINDFORTickets == -1)
					result = true;
				break;
			}
			case "CIV" : {
				if (m_Gamemode.m_iCIVTickets > 0 || m_Gamemode.m_iCIVTickets == -1)
					result = true;
				break;
			}
		}
		return result;
	}

	//------------------------------------------------------------------------------------------------
	void SubtractTicket(string faction)
	{
		bool canSubtract = !m_SafestartManager.GetSafestartStatus();

		switch (faction)
		{
			case "BLUFOR":
			{
				if (m_Gamemode.m_iBLUFORTickets > 0 && m_Gamemode.m_iBLUFORTickets != -1 && canSubtract)
				{
					m_Gamemode.m_iBLUFORTickets = m_Gamemode.m_iBLUFORTickets - 1;
				}
				break;
			}
			case "OPFOR":
			{
				if (m_Gamemode.m_iOPFORTickets > 0 && m_Gamemode.m_iOPFORTickets != -1 && canSubtract)
				{
					m_Gamemode.m_iOPFORTickets = m_Gamemode.m_iOPFORTickets - 1;
				}
				break;
			}
			case "INDFOR":
			{
				if (m_Gamemode.m_iINDFORTickets > 0 && m_Gamemode.m_iINDFORTickets != -1 && canSubtract)
				{
					m_Gamemode.m_iINDFORTickets = m_Gamemode.m_iINDFORTickets - 1;
				}
				break;
			}
			case "CIV":
			{
				if (m_Gamemode.m_iCIVTickets > 0 && m_Gamemode.m_iCIVTickets != -1 && canSubtract)
				{
					m_Gamemode.m_iCIVTickets = m_Gamemode.m_iCIVTickets - 1;
				}
				break;
			}
		}
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void WaveRespawnTimer()
	{
		if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			return;

		m_iRespawnWaveCurrentTime--;
		m_iRespawnTimer = m_iRespawnWaveCurrentTime;
		

		if (m_iRespawnWaveCurrentTime == 0)
		{
			m_iRespawnWaveCurrentTime = m_Gamemode.m_iTimeToRespawn;
		}

		Replication.BumpMe();
	}
	
	void MenuFuckOff()
	{
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu.IsInherited(CRF_SlottingMenuUI))
		{
			GetGame().GetMenuManager().CloseMenu(topMenu);
		};
	}

	//------------------------------------------------------------------------------------------------
	void RespawnTimer()
	{
		// Decrease the respawn timer
		m_iRespawnTimer--;

		// Check if timer has expired or we're in AAR state
		if (m_iRespawnTimer <= 0 || m_Gamemode.m_GamemodeState == CRF_EGamemodeState.AAR)
		{
			// Reset the timer
			m_iRespawnTimer = m_iRespawnWaveCurrentTime;
			// Only perform respawn if not in AAR state
			if (m_Gamemode.m_GamemodeState != CRF_EGamemodeState.AAR)
			{
				CRF_RplToAuthorityManager.GetInstance().RespawnPlayer(SCR_PlayerController.GetLocalPlayerId());
				GetGame().GetCallqueue().Remove(MenuFuckOff);
				GetGame().GetMenuManager().CloseAllMenus();
			}

			// Remove this timer function from the callqueue
			GetGame().GetCallqueue().Remove(RespawnTimer);
			return;
		}

		// Get current top menu
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();

		// Check if we need to open respawn menu
		if (topMenu != null)
		{
			if (!topMenu.IsInherited(CRF_RespawnMenu) && topMenu.IsInherited(CRF_SpectatorMenuUI))
			{
				MenuBase respawnMenu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RespawnMenu);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void RegisterRespawnPoint(IEntity respawnPoint)
	{
		m_aRespawnPoints.Insert(respawnPoint);
	}

	//------------------------------------------------------------------------------------------------
	void UnRegisterRespawnPoint(IEntity respawnPoint)
	{
		if (m_aRespawnPoints.Find(respawnPoint) != -1)
			m_aRespawnPoints.Remove(m_aRespawnPoints.Find(respawnPoint));
	}

	//------------------------------------------------------------------------------------------------
	void RespawnAllSides()
	{
		// Stubbed for old mission support
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

		foreach (int player : allPlayers)
		{
			if (!m_SlottingManager.IsPlayerInASlot(player))
				continue;

			// Get player's faction
			Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(player);

			// Make sure the player is still in that faction
			if (playerFaction.GetFactionKey() != faction)
				continue;

			// If tickets are enabled by MM
			if (TicketsRemaining(faction)) {
				RespawnPlayer(player);
				SubtractTicket(faction);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, vector spawnLocation = vector.Zero, int groupID = -1)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		bool isPlayerConnected = playerManager.IsPlayerConnected(playerId);
		if (!isPlayerConnected)
			return;

		FactionKey factionKey = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId).GetFactionKey();
		if (factionKey.IsEmpty())
			return;
		
		vector finalSpawnLocation = vector.Zero;

		// Find spawn location if not provided
		if (spawnLocation == vector.Zero)
		{
			foreach (IEntity spawnPoint : m_aRespawnPoints)
			{
				if (spawnPoint == null)
					continue;

				CRF_RespawnPointComponent respawnComponent = CRF_RespawnPointComponent.Cast(spawnPoint.FindComponent(CRF_RespawnPointComponent));
				if (!respawnComponent)
					continue;

				if (respawnComponent.m_sRespawnPointFaction != factionKey)
					continue;

				if (!respawnComponent.m_bActiveRespawnPoint)
					continue;

				spawnLocation = spawnPoint.GetOrigin();
				break;
			}
		}

		// If no spawn location found, enter spectator mode
		if (spawnLocation == vector.Zero)
		{
			m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), true);
			m_GamemodeManager.InitilizePlayer(playerId);
			return;
		}

		// Find a valid spawn position
		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, spawnLocation, 10);
		
		// Respawn the player
		m_SlottingManager.UpdateSlotDeathState(m_SlottingManager.GetPlayerSlotID(playerId), false);
		m_GamemodeManager.InitilizePlayer(playerId, finalSpawnLocation);
	}
}
