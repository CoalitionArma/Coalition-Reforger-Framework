class CRF_RaidGamemodeComponentClass: SCR_BaseGameModeComponentClass
{
}

class CRF_RaidGamemodeComponent: SCR_BaseGameModeComponent
{
	static CRF_RaidGamemodeComponent m_sInstance;
	
	[Attribute("100")] int m_iPointsToWin;
	[Attribute("OPFOR")] string m_sDefendingSide;
	[Attribute("BLUFOR")] string m_sAttackingSide;
	[Attribute("INDFOR")] string m_sIndependentFaction;
	
	[RplProp()] int m_iPointsDestroyed = 0;
	
	void CRF_RaidGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_RaidGamemodeComponent GetInstance()
	{
		return m_sInstance;
	}
	
	void OnObjectDestroyed(int pointsDestroyed)
	{
		m_iPointsDestroyed += pointsDestroyed;
		PointsCheck();
		Replication.BumpMe();
	}
	
	void PointsCheck()
	{
		if (m_iPointsDestroyed >= m_iPointsToWin)
			NextPhase();
	}
	
	string GetRespawnResourceName(string side)
	{
		string resourceName;
		switch (side)
		{
			case "BLUFOR": {resourceName = "{62865C82AB534D91}Prefabs/Structures/FlagPoles/RespawnPoles/BLUFOR_Respawn.et"; break;}
			case "OPFOR": {resourceName = "{0B3312C6940005B9}Prefabs/Structures/FlagPoles/RespawnPoles/OPFOR_Respawn.et"; break;}
			case "INDFOR": {resourceName = "{A8C13E34C9597EA4}Prefabs/Structures/FlagPoles/RespawnPoles/INDFOR_Respawn.et"; break;}
			default: {resourceName = "{62865C82AB534D91}Prefabs/Structures/FlagPoles/RespawnPoles/BLUFOR_Respawn.et"; break;}
		}
		return resourceName;
	}
	
	void DelayRespawn(string side)
	{
		CRF_RespawnManager.GetInstance().RespawnSide(side);
	}
	
	void NextPhase()
	{
		CRF_RespawnManager respawnMan = CRF_RespawnManager.GetInstance();
		IEntity defendersRespawn = GetGame().GetWorld().FindEntityByName("DefenderRespawn");
		EntitySpawnParams params = new EntitySpawnParams();
		//Respawns the defenders
		if (!defendersRespawn)
			Print("[CRF RAID ERROR] NO DEFENDER RESPAWN LOCATION");
		else
		{
			defendersRespawn.GetTransform(params.Transform);
			GetGame().SpawnEntityPrefab(Resource.Load(GetRespawnResourceName(m_sDefendingSide)), null, params);
			GetGame().GetCallqueue().CallLater(DelayRespawn, 1000, false, m_sDefendingSide);
		}
		
		//Below is to sort and respawn the dead attackers into independent faction
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		CRF_SlottingManager slottingMan = CRF_SlottingManager.GetInstance();
		CRF_GearScriptRolesConfig rolesConfig = CRF_GamemodeManager.RolesConfig();
		PlayerManager playerMan = GetGame().GetPlayerManager();
		Faction indfor = factionMan.GetFactionByKey("INDFOR");
		ref array<int> players = {};
		ref array<int> leaders = {};
		ref array<int> joes = {};
		
		IEntity independentRespawn = GetGame().GetWorld().FindEntityByName("IndependentRespawn");
		
		playerMan.GetPlayers(players);
		
		EntitySpawnParams indParams = new EntitySpawnParams();
		independentRespawn.GetTransform(indParams.Transform);
		foreach (int playerId: players)
		{	
			Faction playerFaction = factionMan.GetPlayerFaction(playerId);
			PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(playerId);
			if (!playerFaction)
				continue;
			
			if (playerFaction.GetFactionKey() != "SPEC")
				continue;
			
			if (!slottingMan.GetPlayerSlotFaction(playerId))
				continue;
			
			if (slottingMan.GetPlayerSlotFaction(playerId).GetFactionKey() != m_sAttackingSide)
				continue;
			
			CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(slottingMan.GetPlayerSlotResource(playerId));
			CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
			if (roleConfig.m_SlottingType == CRF_ESlotType.SQUAD_LEADER || roleConfig.m_SlottingType == CRF_ESlotType.TEAM_LEADER)
				leaders.Insert(playerId);
			else
				joes.Insert(playerId);
			
			SCR_PlayerFactionAffiliationComponent affiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(
				playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
			);
			
			if (affiliationComponent)
				affiliationComponent.RequestFaction(indfor);
			
			GetGame().GetCallqueue().CallLater(SpawnEntity, 2100, false, roleConfig, indParams, playerController);
		}
		
		int joeSize = joes.Count();
		int amountOfSquads = Math.Ceil(joeSize/8);
		if (amountOfSquads == 0)
			amountOfSquads = 1;
		ref array<SCR_AIGroup> groups = {};
		SCR_GroupsManagerComponent groupsMan = SCR_GroupsManagerComponent.GetInstance();
		//Create the groups for indfor players
		for (int i = 0; i < amountOfSquads; i++)
		{
			SCR_AIGroup newGroup = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(indfor);
			newGroup.SetFaction(indfor);
			newGroup.SetGroupFlag(CRF_EFlagType.INFANTRY, true);
			newGroup.SetCanDeleteIfNoPlayer(false);
			newGroup.SetDeleteWhenEmpty(false);
			newGroup.SetMaxMembers(15);
			newGroup.SetIsPlayableGroup();
			groups.Insert(newGroup);
		}
		
		GetGame().GetCallqueue().CallLater(SCR_Faction.Cast(indfor).InitializeFactionChannels, 2000, false);
		
		for (int i = 0; i < joeSize; i++)
		{
			int index = 0;
			if (i > 0)
			 index = Math.Floor(i/8);
			SCR_AIGroup group = groups.Get(index);
			GetGame().GetCallqueue().CallLater(AssignPlayerToGroup, 2200, false, group.GetGroupID(), joes.Get(i));
		}
		
		for (int i = 0; i < leaders.Count(); i++)
		{
			int index = 0;
			if (i >= amountOfSquads)
				index = i % amountOfSquads;
			SCR_AIGroup group = groups.Get(index);
			GetGame().GetCallqueue().CallLater(AssignPlayerToGroup, 2000, false, group.GetGroupID(), leaders.Get(i));
		}
	}
	
	void SpawnEntity(CRF_RoleConfig roleConfig, EntitySpawnParams indParams, PlayerController playerController)
	{
		IEntity newEntity = GetGame().SpawnEntityPrefab(Resource.Load(roleConfig.m_IndforVariant), null, indParams);
		GetGame().GetCallqueue().CallLater(AssignPlayerToCharacter, 250, false, SCR_PlayerController.Cast(playerController), newEntity);
	}
	
	void AssignPlayerToCharacter(SCR_PlayerController playerController, IEntity entity)
	{
		playerController.SetInitialMainEntity(entity);
		RplComponent playerRplComp = RplComponent.Cast(entity.FindComponent(RplComponent));
		GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast, 250, false, playerController.GetPlayerId(), playerRplComp.Id());
	}
	
	void AssignPlayerToGroup(int groupId, int playerId)
	{
		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (groupComponent)
			groupComponent.RequestJoinGroup(groupId);
	}
}