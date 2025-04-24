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
		
		if(!Replication.IsServer())
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
		switch (faction)
		{
			case "BLUFOR" : {
				if (m_Gamemode.m_iBLUFORTickets > 0 && m_Gamemode.m_iBLUFORTickets != -1 && !CRF_SafestartManager.GetInstance().GetSafestartStatus())
					m_Gamemode.m_iBLUFORTickets = m_Gamemode.m_iBLUFORTickets - 1;
					break;
			}
			case "OPFOR" : {
				if (m_Gamemode.m_iOPFORTickets > 0 && m_Gamemode.m_iOPFORTickets != -1 && !CRF_SafestartManager.GetInstance().GetSafestartStatus())
					m_Gamemode.m_iOPFORTickets = m_Gamemode.m_iOPFORTickets - 1;
					break;
			}
			case "INDFOR" : {
				if (m_Gamemode.m_iINDFORTickets > 0 && m_Gamemode.m_iINDFORTickets != -1 && !CRF_SafestartManager.GetInstance().GetSafestartStatus())
					m_Gamemode.m_iINDFORTickets = m_Gamemode.m_iINDFORTickets - 1;
					break;
			}
			case "CIV" : {
				if (m_Gamemode.m_iCIVTickets > 0 && m_Gamemode.m_iCIVTickets != -1 && !CRF_SafestartManager.GetInstance().GetSafestartStatus())
					m_Gamemode.m_iCIVTickets = m_Gamemode.m_iCIVTickets - 1;
					break;
			}
		}
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void WaveRespawnTimer()
	{
		if (m_Gamemode.m_GamemodeState != CRF_GamemodeState.GAME)
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
		m_iRespawnTimer--;

		if (m_iRespawnTimer <= 0 || m_Gamemode.m_GamemodeState == CRF_GamemodeState.AAR)
		{
			m_iRespawnTimer = m_iRespawnWaveCurrentTime;
				
			if(m_Gamemode.m_GamemodeState != CRF_GamemodeState.AAR)
			{
				CRF_RespawnManager.GetInstance().RespawnPlayer(SCR_PlayerController.GetLocalPlayerId());
				GetGame().GetMenuManager().CloseAllMenus();
			};
			GetGame().GetCallqueue().Remove(RespawnTimer);
			return;
		}

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();

		if (!topMenu.IsInherited(CRF_RespawnMenu) && topMenu.IsInherited(CRF_SpectatorMenuUI))
			MenuBase respawnMenu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RespawnMenu);
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

			RplId groupID = m_Gamemode.m_aActivePlayerGroupsIDs.Get(m_Gamemode.m_aGroupRplIDs.Find(m_Gamemode.m_aPlayerGroupIDs.Get(m_Gamemode.m_aSlots.Find(player))));
			SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(m_Gamemode.m_aActivePlayerGroupsIDs.Find(RplComponent.Cast(playerGroup.FindComponent(RplComponent)).Id())))).GetEntity());
			string playerFactionKey = aiGroup.GetFaction().GetFactionKey();

			// Make sure the player is still in that faction
			if (playerFactionKey != faction)
				continue;

			// If tickets are enabled by MM
			if (TicketsRemaining(faction)) { // Always true if tickets > 0 or tickets == -1
				RespawnPlayer(player);
				SubtractTicket(faction); // only subtract if tickets > 0
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, vector spawnLocation = vector.Zero, int groupID = -1)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		if (SCR_FactionManager.SGetPlayerFaction(playerId).GetFactionKey() == "SPEC" && GetGame().GetPlayerManager().IsPlayerConnected(playerId))
		{
			string respawnPrefab;

			SCR_AIGroup group;
			if (groupID == -1)
			{
				RplId groupRPLID = m_Gamemode.m_aActivePlayerGroupsIDs.Get(m_Gamemode.m_aGroupRplIDs.Find(m_Gamemode.m_aPlayerGroupIDs.Get(m_Gamemode.m_aSlots.Find(playerId))));
				group = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupRPLID)).GetEntity());
			} else {
				group = SCR_GroupsManagerComponent.GetInstance().FindGroup(groupID);
			};

			string faction = group.GetFaction().GetFactionKey();

			if (respawnPrefab.IsEmpty())
			{
				switch (faction)
				{
					case "BLUFOR" 	: {respawnPrefab = "{6F99DE8453E6B423}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Rifleman_P.et"; 	break; }
					case "OPFOR" 	: {respawnPrefab = "{FC0904D71EF8DB6A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Rifleman_P.et"; 	break; }
					case "INDFOR" 	: {respawnPrefab = "{A303C25424BC7149}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Rifleman_P.et";	break; }
					case "CIV" 		: {respawnPrefab = "{2046F9D64B1221F1}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_1SG_P.et";				break; }
				}
			}

			foreach (IEntity spawnPoint : m_aRespawnPoints)
			{
				if (!spawnPoint || CRF_RespawnPointComponent.Cast(spawnPoint.FindComponent(CRF_RespawnPointComponent)).m_sRespawnPointFaction != faction || !CRF_RespawnPointComponent.Cast(spawnPoint.FindComponent(CRF_RespawnPointComponent)).m_bActiveRespawnPoint || spawnLocation != vector.Zero)
					continue;

				spawnLocation = spawnPoint.GetOrigin();
			};

			if (spawnLocation == vector.Zero)
				m_Gamemode.EnterSpectator(playerId);

			RespawnPlayerRplId(playerId, respawnPrefab, spawnLocation, group);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Should only ever be ran on the server
	void RespawnPlayerRplId(int playerId, string prefab, vector position, SCR_AIGroup group)
	{
		if (RplSession.Mode() == RplMode.Client)
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector finalSpawnLocation = vector.Zero;

		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, position, 10);
		spawnParams.Transform[3] = finalSpawnLocation;

		IEntity newEntity = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);

		GetGame().GetCallqueue().CallLater(RespawnPlayerRplIdDelay, 100, false, playerId, group, newEntity);
	}

	//------------------------------------------------------------------------------------------------
	void RespawnPlayerRplIdDelay(int playerId, SCR_AIGroup group, IEntity newEntity)
	{
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_Gamemode.m_aGroupRplIDs.Get(m_Gamemode.m_aActivePlayerGroupsIDs.Find(RplComponent.Cast(group.FindComponent(RplComponent)).Id())))).GetEntity());
		aiGroup.AddAIEntityToGroup(newEntity);

		int index = m_Gamemode.AddPlayableEntity(newEntity);

		if (m_Gamemode.m_aSlots.Find(playerId) != -1)
			m_Gamemode.SetSlot(m_Gamemode.m_aSlots.Find(playerId), -2);

		m_Gamemode.SetSlot(index, playerId);

		CRF_RplBroadcastManager.GetInstance().InitilizePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void OnPlayerJoinedGroup(SCR_AIGroup aiGroup, int playerId)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			IEntity currentLeaderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(aiGroup.GetLeaderID());
			if (!currentLeaderEntity)
				return;

			if (!CRF_RoleHelper.IsSquadLeaderRole(currentLeaderEntity))
			{
				IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
				if (!player)
					return;

				if (CRF_RoleHelper.IsSquadLeaderRole(player))
				{
					SCR_GroupsManagerComponent.GetInstance().SetGroupLeader(aiGroup.GetGroupID(), playerId);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerLeftGroup(SCR_AIGroup aiGroup, int playerId)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			IEntity currentLeaderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(aiGroup.GetLeaderID());
			if (!currentLeaderEntity)
				return;

			if (!CRF_RoleHelper.IsSquadLeaderRole(currentLeaderEntity))
			{
				array<int> groupMembers = aiGroup.GetPlayerIDs();

				foreach (int member : groupMembers)
				{
					IEntity memberEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(member);
					if (!memberEntity)
						return;

					if (CRF_RoleHelper.IsTeamLeaderRole(memberEntity))
					{
						SCR_GroupsManagerComponent.GetInstance().SetGroupLeader(aiGroup.GetGroupID(), member);
						break;
					}
				}
			}
		}
	}
}