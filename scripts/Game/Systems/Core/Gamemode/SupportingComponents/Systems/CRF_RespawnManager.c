class CRF_RespawnManagerClass : ScriptComponentClass {}

class CRF_RespawnManager : ScriptComponent
{
	[RplProp(onRplName: "WaveRespawnTimer")]
	int m_iRespawnWaveCurrentTime;

	int m_iRespawnTimer
	protected ref array<IEntity> m_aRespawnPoints = {};
	protected CRF_Gamemode m_Gamemode;

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

		if (!Replication.IsServer())
			return;

		m_Gamemode = CRF_Gamemode.GetInstance();

		SCR_AIGroup.GetOnPlayerAdded().Insert(OnPlayerJoinedGroup);
		SCR_AIGroup.GetOnPlayerRemoved().Insert(OnPlayerLeftGroup);

		if (m_Gamemode.m_bRespawnEnabled)
			CRF_RespawnManager.GetInstance().InitilizeRespawns();
	}

	//------------------------------------------------------------------------------------------------
	void InitilizeRespawns()
	{
		m_iRespawnWaveCurrentTime = m_Gamemode.m_iTimeToRespawn;
		m_iRespawnTimer = m_iRespawnWaveCurrentTime;

		if (m_Gamemode.m_bWaveRespawn && RplSession.Mode() == RplMode.Dedicated)
		{
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
		bool canSubtract = !CRF_SafestartManager.GetInstance().GetSafestartStatus();

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

		if (m_iRespawnWaveCurrentTime == 0)
		{
			m_iRespawnWaveCurrentTime = m_Gamemode.m_iTimeToRespawn;
		}

		Replication.BumpMe();
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
				CRF_RespawnManager.GetInstance().RespawnPlayer(SCR_PlayerController.GetLocalPlayerId());
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
			if (!CRF_SlottingManager.GetInstance().IsPlayerInASlot(player))
				continue;

			// Get player's faction
			Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(player);

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

		// Check if player is in spectator faction and connected
		SCR_Faction playerFaction = SCR_Faction.Cast(SCR_FactionManager.SGetPlayerFaction(playerId));
		if (playerFaction == null)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		bool isPlayerConnected = playerManager.IsPlayerConnected(playerId);
		if (!isPlayerConnected)
			return;

		string factionKey = playerFaction.GetFactionKey();
		if (factionKey != "SPEC")
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
				if (respawnComponent == null)
					continue;

				if (respawnComponent.m_sRespawnPointFaction != playerFaction.GetFactionKey())
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
			CRF_GamemodeManager.GetInstance().EnterSpectator(playerId);
			return;
		}

		// Find a valid spawn position
		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, spawnLocation, 10);
		
		// Respawn the player
		CRF_GamemodeManager.GetInstance().InitilizePlayer(playerId, finalSpawnLocation);
	}
	
	//------------------------------------------------------------------------------------------------
	void OnPlayerJoinedGroup(SCR_AIGroup aiGroup, int playerId)
	{
		// Only run this logic on dedicated server
		if (RplSession.Mode() != RplMode.Dedicated)
			return;

		// Get the current leader entity
		PlayerManager playerManager = GetGame().GetPlayerManager();
		IEntity currentLeaderEntity = playerManager.GetPlayerControlledEntity(aiGroup.GetLeaderID());

		// Check if leader entity exists
		if (!currentLeaderEntity)
			return;

		// If current leader is not a squad leader role
		if (!CRF_RoleHelper.IsSquadLeaderRole(currentLeaderEntity))
		{
			// Get joining player entity
			IEntity player = playerManager.GetPlayerControlledEntity(playerId);

			// Check if player entity exists
			if (!player)
				return;

			// If joining player has squad leader role, make them the new leader
			if (CRF_RoleHelper.IsSquadLeaderRole(player))
			{
				SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
				groupsManager.SetGroupLeader(aiGroup.GetGroupID(), playerId);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerLeftGroup(SCR_AIGroup aiGroup, int playerId)
	{
		// Only proceed on dedicated server
		if (RplSession.Mode() != RplMode.Dedicated)
			return;

		// Get player manager
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		// Get the current group leader entity
		int leaderID = aiGroup.GetLeaderID();
		IEntity currentLeaderEntity = playerManager.GetPlayerControlledEntity(leaderID);

		// Check if leader entity exists
		if (!currentLeaderEntity)
			return;

		// If current leader is not a squad leader, find a team leader to promote
		if (!CRF_RoleHelper.IsSquadLeaderRole(currentLeaderEntity))
		{
			// Get all group members
			array<int> groupMembers = aiGroup.GetPlayerIDs();
			if (!groupMembers || groupMembers.IsEmpty())
				return;

			// Get groups manager component
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (!groupsManager)
				return;

			// Look for a team leader to promote
			for (int i = 0; i < groupMembers.Count(); i++)
			{
				int member = groupMembers[i];
				IEntity memberEntity = playerManager.GetPlayerControlledEntity(member);

				if (!memberEntity)
					continue;

				if (CRF_RoleHelper.IsTeamLeaderRole(memberEntity))
				{
					groupsManager.SetGroupLeader(aiGroup.GetGroupID(), member);
					break;
				}
			}
		}
	}
}
