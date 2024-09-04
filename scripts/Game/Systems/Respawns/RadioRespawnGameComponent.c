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
	
	IEntity tempEnt;
	
	PS_PlayableManager playableManager;
	
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	CRF_SafestartGameModeComponent m_safestart;
	protected ref array<int> m_respawnedGroups = {};
	
	protected ref map<IEntity, int> m_entitySlots = new map<IEntity, int>();
	protected ref map<IEntity, string> m_entityPrefabs = new map<IEntity, string>();
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
				string prefabName = controlledEntity.GetPrefabData().GetPrefabName();
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
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
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
			int m_tempPlayerID = m_entityID.Get(tempEntity);
			
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
			m_entitySlots.Insert(tempEnt, groupID);
			m_entityPrefabs.Insert(tempEnt, m_tempPrefab);
			m_entityID.Insert(tempEnt, m_tempPlayerID);
		}
	}
	
	void SpawnPrefabs()
	{
		tempEnt = GetGame().SpawnEntityPrefab(Resource.Load(m_tempPrefab),GetGame().GetWorld(),spawnParams);
		SCR_AIGroup playablegroup = m_GroupsManagerComponent.FindGroup(m_groupID);
		playablegroup.AddAIEntityToGroup(tempEnt);
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