class CRF_RespawnManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_RespawnManager : SCR_BaseGameModeComponent
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
	void RespawnSide(string faction)
	{
		array<int> allPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayers);

		foreach (int player : allPlayers)
		{
			if (!m_Gamemode.m_aSlots.Contains(player))
				continue;

			// Find player's group through the chain of lookups
			int slotIndex = m_Gamemode.m_aSlots.Find(player);
			int playerGroupID = m_Gamemode.m_aPlayerGroupIDs.Get(slotIndex);
			int groupRplIndex = m_Gamemode.m_aGroupRplIDs.Find(playerGroupID);
			RplId groupID = m_Gamemode.m_aActivePlayerGroupsIDs.Get(groupRplIndex);

			// Get player group entity
			RplComponent groupComponent = RplComponent.Cast(Replication.FindItem(groupID));
			SCR_AIGroup playerGroup = SCR_AIGroup.Cast(groupComponent.GetEntity());

			// Find AI group associated with player
			RplComponent playerGroupComponent = RplComponent.Cast(playerGroup.FindComponent(RplComponent));
			int activeGroupIndex = m_Gamemode.m_aActivePlayerGroupsIDs.Find(playerGroupComponent.Id());
			RplId aiGroupId = m_Gamemode.m_aGroupRplIDs.Get(activeGroupIndex);

			RplComponent aiGroupComponent = RplComponent.Cast(Replication.FindItem(aiGroupId));
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(aiGroupComponent.GetEntity());

			// Get player's faction
			string playerFactionKey = aiGroup.GetFaction().GetFactionKey();

			// Make sure the player is still in that faction
			if (playerFactionKey != faction)
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

		// Initialize variables
		string respawnPrefab = "";
		SCR_AIGroup group = null;

		// Determine player's group
		if (groupID == -1)
		{
			int slotIndex = m_Gamemode.m_aSlots.Find(playerId);
			int playerGroupID = m_Gamemode.m_aPlayerGroupIDs.Get(slotIndex);
			int groupRplIndex = m_Gamemode.m_aGroupRplIDs.Find(playerGroupID);
			RplId groupRPLID = m_Gamemode.m_aActivePlayerGroupsIDs.Get(groupRplIndex);

			RplComponent groupComponent = RplComponent.Cast(Replication.FindItem(groupRPLID));
			if (groupComponent != null)
			{
				group = SCR_AIGroup.Cast(groupComponent.GetEntity());
			}
		}
		else
		{
			SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
			if (groupsManager != null)
			{
				group = groupsManager.FindGroup(groupID);
			}
		}

		if (group == null)
			return;

		// Get faction from group
		SCR_Faction groupFaction = SCR_Faction.Cast(group.GetFaction());
		if (groupFaction == null)
			return;

		string faction = groupFaction.GetFactionKey();

		// Set respawn prefab based on faction
		if (respawnPrefab == "")
		{
			switch (faction)
			{
				case "BLUFOR":
				{
					respawnPrefab = "{6F99DE8453E6B423}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Rifleman_P.et";
					break;
				}
				case "OPFOR":
				{
					respawnPrefab = "{FC0904D71EF8DB6A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Rifleman_P.et";
					break;
				}
				case "INDFOR":
				{
					respawnPrefab = "{A303C25424BC7149}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Rifleman_P.et";
					break;
				}
				case "CIV":
				{
					respawnPrefab = "{2046F9D64B1221F1}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_1SG_P.et";
					break;
				}
			}
		}

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

				if (respawnComponent.m_sRespawnPointFaction != faction)
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
			m_Gamemode.EnterSpectator(playerId);
		}

		// Respawn the player
		RespawnPlayerRplId(playerId, respawnPrefab, spawnLocation, group);
	}

	//------------------------------------------------------------------------------------------------
	// Should only ever be ran on the server
	void RespawnPlayerRplId(int playerId, string prefab, vector position, SCR_AIGroup group)
	{
		// Return if we are on client
		if (RplSession.Mode() == RplMode.Client)
			return;

		// Create spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector finalSpawnLocation = vector.Zero;

		// Find a valid spawn position
		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, position, 10);
		spawnParams.Transform[3] = finalSpawnLocation;

		// Load resource and spawn entity
		Resource prefabResource = Resource.Load(prefab);
		World gameWorld = GetGame().GetWorld();
		IEntity newEntity = GetGame().SpawnEntityPrefab(prefabResource, gameWorld, spawnParams);

		// Schedule delayed processing
		GetGame().GetCallqueue().CallLater(RespawnPlayerRplIdDelay, 100, false, playerId, group, newEntity);
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayerRplIdDelay(int playerId, SCR_AIGroup group, IEntity newEntity)
	{
		// Get the RplComponent from the group
		RplComponent groupRplComponent = RplComponent.Cast(group.FindComponent(RplComponent));

		// Get the group ID
		RplId groupId = groupRplComponent.Id();

		// Find the index in the active player groups IDs
		int activeGroupIndex = m_Gamemode.m_aActivePlayerGroupsIDs.Find(groupId);

		// Get the group Rpl ID
		RplId groupRplId = m_Gamemode.m_aGroupRplIDs.Get(activeGroupIndex);

		// Find the Rpl component using the ID
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(groupRplId));

		// Get the AI group
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(rplComponent.GetEntity());

		// Add the new entity to the group
		aiGroup.AddAIEntityToGroup(newEntity);

		// Add the entity to playable entities and get its index
		int index = m_Gamemode.AddPlayableEntity(newEntity);

		// Clear the player's previous slot if they had one
		int playerSlotIndex = m_Gamemode.m_aSlots.Find(playerId);
		if (playerSlotIndex != -1)
		{
			m_Gamemode.SetSlot(playerSlotIndex, -2);
		}

		// Assign the player to the new slot
		m_Gamemode.SetSlot(index, playerId);

		// Initialize the player
		CRF_RplBroadcastManager.GetInstance().InitilizePlayer(playerId);
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
