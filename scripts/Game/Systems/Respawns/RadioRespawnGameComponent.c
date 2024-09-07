[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RadioRespawnSystemComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RadioRespawnSystemComponent: SCR_BaseGameModeComponent
{

	[Attribute("respawn", "auto", "BLUFOR Respawn Point")];
	protected string m_bluforSpawnPoint;
	
	[Attribute("respawn", "auto", "OFPOR Respawn Point")];
	protected string m_opforSpawnPoint;
	
	[Attribute("respawn", "auto", "INDFOR Respawn Point")];
	protected string m_indforSpawnPoint;
	
	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements.")];
	protected int m_respawnWaves;
	
	[RplProp(onRplName: "SpawnPrefabs")]
	string m_tempPrefab;
	ref EntitySpawnParams spawnParams = new EntitySpawnParams();
	int m_groupID;
	IEntity tempEntity;
	int m_tempPlayerID;
	IEntity tempEnt;
	
	PS_PlayableManager playableManager;
	protected PS_GameModeCoop m_GameModeCoop;
	
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	CRF_SafestartGameModeComponent m_safestart;
	protected ref array<int> m_respawnedGroups = {};
	
	protected ref map<IEntity, int> m_entitySlots = new map<IEntity, int>();
	protected ref map<IEntity, ResourceName> m_entityPrefabs = new map<IEntity, ResourceName>();
	protected ref map<IEntity, int> m_entityID = new map<IEntity, int>();
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
	}
	
	int GetAmountofWave()
	{
		return m_respawnWaves;
	}
	
	int GetRespawnedGroups(int groupID)
	{
		int timesRespawned;
		foreach (int wave : m_respawnedGroups)
		{
			if(wave == groupID)
			{
				timesRespawned++;
			}
		}
		return timesRespawned;
	}
	void RespawnInit()
	{
		if (m_safestart.GetSafestartStatus())
		{
			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
		}
		if (!m_safestart.GetSafestartStatus())
		{
			GetGroups();
			GetGame().GetCallqueue().CallLater(SpawnGroup, 10000, false, 0);
		}
	}
	
	void GetGroups()
	{
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		array<SCR_AIGroup> outAllGroups;
		m_GroupsManagerComponent.GetAllPlayableGroups(outAllGroups);
		
		foreach (SCR_AIGroup group : outAllGroups)
		{
			array<int> groupPlayersIDs = group.GetPlayerIDs();
			int groupID = group.GetGroupID();
			foreach (int playerID: groupPlayersIDs)
			{
				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);			
				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
				m_entitySlots.Insert(controlledEntity, groupID);
				m_entityPrefabs.Insert(controlledEntity, prefabName);
				m_entityID.Insert(controlledEntity, playerID);
			}
		}
	}
	
	void WaitTillGameStart()
	{
		m_safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (m_safestart.GetSafestartStatus()) 
		{
			GetGroups();	
			GetGame().GetCallqueue().Remove(WaitTillGameStart);
			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
		}
		return;
	}
	
	
	void WaitSafeStartEnd()
	{
		if (!m_safestart.GetSafestartStatus()) 
		{
			GetGame().GetCallqueue().Remove(WaitSafeStartEnd);
			GetGame().GetCallqueue().CallLater(RespawnInit, 1000);
		}
		return;
	}
	
	void SpawnGroup(int groupID)
	{
		Rpc(RPC_SpawnGroup, groupID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RPC_SpawnGroup(int groupID)
	{
		int timesRespawned;
		foreach (int wave : m_respawnedGroups)
		{
			if(wave == groupID)
			{
				timesRespawned++;
			}
		}
		
		if(timesRespawned >= m_respawnWaves)
		{
			Print("Out of respawns");
			return;
		}
		m_respawnedGroups.Insert(groupID);
		playableManager = PS_PlayableManager.GetInstance();
		IEntity factionEntity = m_entitySlots.GetKeyByValue(groupID);
		int factionPlayerID = m_entityID.Get(factionEntity);
		Faction faction = SCR_FactionManager.SGetPlayerFaction(factionPlayerID);
		
		Color factionColor = faction.GetFactionColor();
		float rg = Math.Max(factionColor.R(), factionColor.G());
		float rgb = Math.Max(rg, factionColor.B());
		
		for(int i, count = m_entitySlots.Count(); i < count; ++i)
		{
			m_groupID = groupID;
			Replication.BumpMe();
			tempEntity = m_entitySlots.GetKeyByValue(groupID);
			Replication.BumpMe();
			m_tempPlayerID = m_entityID.Get(tempEntity);
			Replication.BumpMe();
			
			if (!tempEntity)
				return;
			if(SCR_AIDamageHandling.IsAlive(tempEntity))
				return;
				
			switch (true)
			{
				case (rgb == factionColor.B()):
					GetSpawnParamsByFaction(0);
					break;
				
				case (rgb == factionColor.R()):
					GetSpawnParamsByFaction(1);
					break;
				
				case (rgb == factionColor.G()):
					GetSpawnParamsByFaction(2);
					break;
			}
			
			m_tempPrefab = m_entityPrefabs.Get(tempEntity);
			Replication.BumpMe();
			
			SpawnPrefabs();
			m_entitySlots.Remove(tempEntity);
			m_entityPrefabs.Remove(tempEntity);
			m_entityID.Remove(tempEntity);
			m_entitySlots.Insert(tempEnt, m_groupID);
			m_entityPrefabs.Insert(tempEnt, m_tempPrefab);
			m_entityID.Insert(tempEnt, m_tempPlayerID);
		}
	}
	//old
	//void OldSpawnPrefabs()
	//{
	//	tempEnt = GetGame().SpawnEntityPrefab(Resource.Load(m_tempPrefab),GetGame().GetWorld(),spawnParams);
//		Replication.BumpMe();
	//	Print(m_groupID);
	//	SCR_AIGroup playablegroup = m_GroupsManagerComponent.FindGroup(m_groupID);
	//	Print(playablegroup);
	//	SCR_AIGroup slavegroup;
	//	playablegroup.SetSlave(slavegroup);
	//	SCR_AIGroup playablegroupAI = playablegroup.GetSlave();
	//	Print(playablegroupAI);
	//	playablegroupAI.AddAIEntityToGroup(tempEnt);
	//	PS_PlayableComponent playableComponentNew = PS_PlayableComponent.Cast(tempEnt.FindComponent(PS_PlayableComponent));
	//	playableComponentNew.SetPlayable(true);
	//}
	
	void SpawnPrefabs()
	{
		Print(m_tempPlayerID);
		RplId playerPlayableID = playableManager.GetPlayableByPlayer(m_tempPlayerID);
		Print(playerPlayableID);
		PS_PlayableComponent playableComponent = playableManager.GetPlayableById(playerPlayableID);
		Print(playableComponent);
		ResourceName prefabToSpawn = m_tempPrefab;
		Print(prefabToSpawn);
		PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
		Print(respawnData);
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_GameModeCoop.Respawn(m_tempPlayerID, respawnData);
	}
	
	void IsPlayerInGroup()
	{
		
		
		if(EntityUtils.IsPlayer(tempEnt))
		{
			Print("Adding player to group");
			SCR_PlayerControllerGroupComponent playerComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(m_tempPlayerID);
			int newGroupID = playerComponent.GetGroupID();
			m_GroupsManagerComponent.MovePlayerToGroup(m_tempPlayerID, newGroupID, m_groupID);
		}
		
	}
	
	void testCallQueue(string text)
	{
		if (text == "end")
		{
			Print(text);
			GetGame().GetCallqueue().Remove(testCallQueue);
		}
		Print(text);
	}
	
	//0 = BLUFOR
	//1 = OPFOR
	//2 = INDFOR
	void GetSpawnParamsByFaction(int factionNumber)
	{
		if (factionNumber == 0)
		{
			protected vector spawnPoint = GetGame().GetWorld().FindEntityByName(m_bluforSpawnPoint).GetOrigin();
        	spawnParams.TransformMode = ETransformMode.WORLD;
        	spawnParams.Transform[3] = spawnPoint;
			Replication.BumpMe();
		}
		if (factionNumber == 1)
		{
			protected vector spawnPoint = GetGame().GetWorld().FindEntityByName(m_opforSpawnPoint).GetOrigin();
        	spawnParams.TransformMode = ETransformMode.WORLD;
        	spawnParams.Transform[3] = spawnPoint;
			Replication.BumpMe();
		}
		if (factionNumber == 2)
		{
			protected vector spawnPoint = GetGame().GetWorld().FindEntityByName(m_indforSpawnPoint).GetOrigin();
        	spawnParams.TransformMode = ETransformMode.WORLD;
        	spawnParams.Transform[3] = spawnPoint;
			Replication.BumpMe();
		}
	}
}