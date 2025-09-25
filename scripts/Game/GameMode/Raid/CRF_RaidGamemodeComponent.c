class CRF_RaidGamemodeComponentClass: SCR_BaseGameModeComponentClass
{
}

class CRF_RaidGamemodeComponent: SCR_BaseGameModeComponent
{
	static CRF_RaidGamemodeComponent m_sInstance;
	
	[Attribute("100")] int m_iPointsToWin;
	[Attribute("50")] float m_fPercentAttackersRetreat;
	[Attribute("OPFOR")] string m_sDefendingSide;
	[Attribute("BLUFOR")] string m_sAttackingSide;
	[Attribute("INDFOR")] string m_sIndependentFaction;
	
	[RplProp()] int m_iPointsDestroyed = 0;
	
	CRF_SlottingManager m_SlottingManager;
	int m_iCurrentPhase = 1;
	
	//Client Values
	Widget m_wCurrentAlert;
	int m_iWidgetChange = 0;
	
	void CRF_RaidGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	void ~CRF_RaidGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!GetGame().GetWorld())
			return;
		//Cleans it up on scernario reload if it's still there
		if (m_wCurrentAlert)
			delete m_wCurrentAlert;
	}
	
	static CRF_RaidGamemodeComponent GetInstance()
	{
		return m_sInstance;
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (m_iCurrentPhase == 2)
			return;
		
		//Quick delay so we can make sure the players slot dead state is updated.
		GetGame().GetCallqueue().CallLater(CheckAttackersDelay, 250, false);
	}
	
	void CheckAttackersDelay()
	{
		if (IsAttackersBelowThreshold() && m_iCurrentPhase != 2)
			NextPhase();
	}
	
	//Checks to see if side is below percentage.
	bool IsAttackersBelowThreshold()
	{
		int attackersSlotted = 0;
		int attackersDead = 0;
		foreach (int slotId, CRF_SlotDataContainer slotContainer: m_SlottingManager.GetSlotMap())
		{
			if (slotContainer.GetSlotCurrentPlayerId() == 0)
				continue;
			
			if (slotContainer.GetSlotFactionKey() != m_sAttackingSide)
				continue;
			
			attackersSlotted++;
			if (slotContainer.GetIsDeadSlot())
				attackersDead++;
		}
		
		//No 0 division
		if (attackersSlotted == 0 || attackersDead == 0)
			return false;
		
		if ((attackersDead/attackersSlotted) * 10 < m_fPercentAttackersRetreat)
			return true;
		
		return false;
	}
	
	void OnObjectDestroyed(int pointsDestroyed)
	{
		m_iPointsDestroyed += pointsDestroyed;
		PointsCheck();
		Replication.BumpMe();
		float percent = (float)m_iPointsDestroyed/(float)m_iPointsToWin * 100;
		Rpc(RpcDo_DrawPointUpdate, pointsDestroyed, percent);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_DrawPointUpdate(int pointsAdded, float currentPercent)
	{
		if (m_wCurrentAlert)
			delete m_wCurrentAlert;
		
		m_iWidgetChange++;
		m_wCurrentAlert = GetGame().GetWorkspace().CreateWidgets("{66DCB94B8F932419}UI/layouts/HUD/Raid/RaidPopUp.layout");
		m_wCurrentAlert.SetOpacity(0);
		AnimateWidget.StopAllAnimations(m_wCurrentAlert);
		AnimateWidget.Opacity(m_wCurrentAlert, 1, 1);
		TextWidget.Cast(m_wCurrentAlert.FindWidget("TotalAdded")).SetText("+ " + pointsAdded.ToString());
		Print(currentPercent);
		ProgressBarWidget.Cast(m_wCurrentAlert.FindWidget("Progress")).SetCurrent(currentPercent);
		GetGame().GetCallqueue().CallLater(AnimatePointUpdateFade, 2000, false, m_iWidgetChange);
	}
	
	void AnimatePointUpdateFade(int widgetChange)
	{
		if (widgetChange != m_iWidgetChange)
			return;
		
		AnimateWidget.Opacity(m_wCurrentAlert, 0, 3);
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
		m_iCurrentPhase = 2;
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
			
			if (!m_SlottingManager.GetPlayerSlotFaction(playerId))
				continue;
			
			if (m_SlottingManager.GetPlayerSlotFaction(playerId).GetFactionKey() != m_sAttackingSide || !m_SlottingManager.GetPlayerSlotData(playerId).GetIsDeadSlot())
				continue;
			
			CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(m_SlottingManager.GetPlayerSlotResource(playerId));
			CRF_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
			if (roleConfig.m_SlottingType == CRF_ESlotType.SQUAD_LEADER || roleConfig.m_SlottingType == CRF_ESlotType.TEAM_LEADER)
				leaders.Insert(playerId);
			else
				joes.Insert(playerId);
			
			
			GetGame().GetCallqueue().CallLater(AssignFactionToPlayer, 100, false, playerController, indfor);
			GetGame().GetCallqueue().CallLater(SpawnEntity, 300, false, roleConfig, indParams, playerController);
		}
		
		int joeSize = joes.Count();
		int amountOfSquads = 1;
		if (joeSize > 0)
			amountOfSquads = Math.Ceil(joeSize / 8.0);
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
		
		GetGame().GetCallqueue().CallLater(SCR_Faction.Cast(indfor).InitializeFactionChannels, 200, false);
		
		for (int i = 0; i < joeSize; i++)
		{
			int index = i % amountOfSquads;
			SCR_AIGroup group = groups.Get(index);
			RplId groupId = RplComponent.Cast(group.FindComponent(RplComponent)).Id();
			GetGame().GetCallqueue().CallLater(AssignPlayerToGroup, 700, false, group.GetGroupID(), joes.Get(i), groupId);
		}
		
		for (int i = 0; i < leaders.Count(); i++)
		{
			int index = i % amountOfSquads;
			SCR_AIGroup group = groups.Get(index);
			RplId groupId = RplComponent.Cast(group.FindComponent(RplComponent)).Id();
			GetGame().GetCallqueue().CallLater(AssignPlayerToGroup, 700, false, group.GetGroupID(), leaders.Get(i), groupId);
		}
		
	}
	
	void AssignFactionToPlayer(PlayerController playerController, Faction faction)
	{
		SCR_PlayerFactionAffiliationComponent affiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(
				playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
			);
			
		if (affiliationComponent)
			affiliationComponent.RequestFaction(faction);
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
	
	void AssignPlayerToGroup(int groupId, int playerId, RplId groupRplId)
	{
		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (groupComponent)
			groupComponent.RequestJoinGroup(groupId);
		CRF_SlotDataContainer currentData = m_SlottingManager.GetSlotData(m_SlottingManager.GetPlayerSlotID(playerId));
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		RplId characterRplId = RplComponent.Cast(character.FindComponent(RplComponent)).Id();
		currentData.SetSlotFactionKey(m_sIndependentFaction);
		m_SlottingManager.BatchUpdateSlot(m_SlottingManager.GetPlayerSlotID(playerId), playerId, groupRplId, characterRplId, character.GetPrefabData().GetPrefabName(), currentData.GetSlotName(), false, false);
	}
}