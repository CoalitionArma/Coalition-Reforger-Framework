[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RadioRespawnSystemComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RadioRespawnSystemComponent: SCR_BaseGameModeComponent
{	
	[Attribute("false", "auto", "Can Blufor Respawn")];
	protected bool m_canBluforRespawn;
	
	[Attribute("US", "auto", "Blufor Faction Key")];
	protected string m_bluforFactionKey;
	
	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements on Blufor.")];
	protected int m_bluforRespawnWaves;
	
	[Attribute("false", "auto", "Can Opfor Respawn")];
	protected bool m_canOpforRespawn;
	
	[Attribute("USSR", "auto", "Opfor Faction Key")];
	protected string m_opforFactionKey;
	
	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements on Opfor.")];
	protected int m_opforRespawnWaves;
	
	[Attribute("false", "auto", "Can Indfor Respawn")];
	protected bool m_canIndforRespawn;
	
	[Attribute("CDF", "auto", "Indfor Faction Key")];
	protected string m_indforFactionKey;
	
	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements on Indfor.")];
	protected int m_indforRespawnWaves;
	
	//[RplProp(onRplName: "SpawnPrefabs")]
	string m_tempPrefab;
	int m_tempPlayerID;
	RplId m_tempPlayableID;
	
	IEntity m_tempEntity;
	PS_PlayableManager m_playableManager;
	protected PS_GameModeCoop m_GameModeCoop;
	
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	CRF_SafestartGameModeComponent m_safestart;
	protected ref array<int> m_respawnedGroups = {};
	
	protected ref map<IEntity, int> m_entitySlots = new map<IEntity, int>();
	protected ref map<IEntity, ResourceName> m_entityPrefabs = new map<IEntity, ResourceName>();
	protected ref map<IEntity, int> m_entityID = new map<IEntity, int>();
	protected ref map<IEntity, RplId> m_entityPlayable = new map<IEntity, RplId>();
	protected ref map<int, bool> m_respawnTimeout = new map<int, bool>();
		
	protected ref map<RplId, PS_PlayableComponent> m_mPlayables = new map<RplId, PS_PlayableComponent>;
	protected int m_mPlayablesCount = 0;
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
	}
	
	void WaitTillGameStart()
	{
		m_safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (m_safestart.GetSafestartStatus()) 
		{	
			Print("Safestart has begun");
			GetGame().GetCallqueue().Remove(WaitTillGameStart);
			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
		}
		return;
	}
	
	void WaitSafeStartEnd()
	{
		if (!m_safestart.GetSafestartStatus()) 
		{
			Print("Safestart is over");
			GetGame().GetCallqueue().Remove(WaitSafeStartEnd);
			GetGame().GetCallqueue().CallLater(RespawnInit, 1000);
		}
		return;
	}
	
	void RespawnInit()
	{
		if (m_safestart.GetSafestartStatus())
		{
			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
		}
		if (!m_safestart.GetSafestartStatus())
		{
			Print("Safestart is really over");
			GetGroups();
		}
	}
	
	int GetAmountofWave(string factionKey)
	{
		if(factionKey == m_bluforFactionKey)
			return m_bluforRespawnWaves;
		if(factionKey == m_opforFactionKey)
			return m_opforRespawnWaves;
		return m_indforFactionKey;
	}
	
	int GetRespawnedGroups(int groupID)
	{
		int timesRespawned = 0;
		foreach (int wave : m_respawnedGroups)
		{
			if(wave == groupID)
			{
				timesRespawned++;
			}
		}
		return timesRespawned;
	}
	
	void AddRespawnWaves(string factionKey, int amount)
	{
		if(factionKey == m_bluforFactionKey)
		{
			m_bluforRespawnWaves = m_bluforRespawnWaves + amount;
			return;
		}
		if(factionKey == m_opforFactionKey)
		{
			m_opforRespawnWaves = m_opforRespawnWaves + amount;
			return;
		}
		m_indforRespawnWaves = m_indforRespawnWaves + amount;
		return;
	}
	
	void RemoveRespawnWaves(string factionKey, int amount)
	{
		if(factionKey == m_bluforFactionKey)
		{
			m_bluforRespawnWaves = m_bluforRespawnWaves - amount;
			return;
		}
		if(factionKey == m_opforFactionKey)
		{
			m_opforRespawnWaves = m_opforRespawnWaves - amount;
			return;
		}
		m_indforRespawnWaves = m_indforRespawnWaves - amount;
		return;
	}
	
	void SetCanFactionRespawn(string factionKey, bool canRespawn)
	{
		if(factionKey == m_bluforFactionKey)
		{
			m_canBluforRespawn = canRespawn;
			return;
		}
		if(factionKey == m_opforFactionKey)
		{
			m_canOpforRespawn = canRespawn;
			return;
		}
		m_canIndforRespawn = canRespawn;
		return;
	}
	
	bool CanFactionRespawn(string factionID)
	{
		if(factionID == m_bluforFactionKey)
		{
			return m_canBluforRespawn;
		}
		else if(factionID == m_opforFactionKey)
			return m_canOpforRespawn;
		else
			return m_canIndforRespawn;
	}
	
	bool InRespawnTimeout(int groupID)
	{
		bool canRespawn;
		m_respawnTimeout.Find(groupID, canRespawn);
		return canRespawn;
	}
	
	void GetGroups()
	{
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		array<SCR_AIGroup> outAllGroups;
		m_GroupsManagerComponent.GetAllPlayableGroups(outAllGroups);
		m_playableManager = PS_PlayableManager.GetInstance();
		
		foreach (SCR_AIGroup group : outAllGroups)
		{
			array<int> groupPlayersIDs = group.GetPlayerIDs();
			int groupID = group.GetGroupID();
			m_respawnTimeout.Insert(groupID, false);
			foreach (int playerID: groupPlayersIDs)
			{
				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);			
				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
				RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
				m_entitySlots.Insert(controlledEntity, groupID);
				m_entityPrefabs.Insert(controlledEntity, prefabName);
				m_entityID.Insert(controlledEntity, playerID);
				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
			}
		}
	}
	
	void SpawnGroupServer(int groupID)
	{
		Print("Spawning Group server side");
		Print(groupID);
		m_respawnedGroups.Insert(groupID);
		m_respawnTimeout.Set(groupID, true);
		
		m_playableManager = PS_PlayableManager.GetInstance();
		Print(m_playableManager);
		for(int i, count = m_entitySlots.Count(); i < count; ++i)
		{
			m_tempEntity = m_entitySlots.GetKeyByValue(groupID);
			
			if(!m_tempEntity)
				break;
			
			m_tempPlayerID = m_entityID.Get(m_tempEntity);
			Print(m_tempPlayerID);
			m_tempPlayableID = m_entityPlayable.Get(m_tempEntity);
			Print(m_tempPlayableID);
			m_tempPrefab = m_entityPrefabs.Get(m_tempEntity);
			Print(m_tempPrefab);
			
			if(SCR_AIDamageHandling.IsAlive(m_tempEntity))
				{
					m_entitySlots.Remove(m_tempEntity);
					m_entityPrefabs.Remove(m_tempEntity);
					m_entityID.Remove(m_tempEntity);
					m_entityPlayable.Remove(m_tempEntity);
					m_entitySlots.Insert(m_tempEntity, groupID);
					m_entityPrefabs.Insert(m_tempEntity, m_tempPrefab);
					m_entityID.Insert(m_tempEntity, m_tempPlayerID);
					m_entityPlayable.Insert(m_tempEntity, m_tempPlayableID);
					break;
				}	
			//Replication.BumpMe();
			SpawnPrefabs();
			GetGame().GetCallqueue().CallLater(SetNewPlayerValues, 500, false, m_tempEntity, groupID, m_tempPrefab, m_tempPlayerID);
			GetGame().GetCallqueue().CallLater(SetLatePlayerValues, 300000, false, groupID, 300000);
		}
	}
	
	void SpawnPrefabs()
	{
		Print("Spawning Prefabs to all");
		m_playableManager = PS_PlayableManager.GetInstance();
		if(m_tempPlayableID == RplId.Invalid())
			return;
		PS_PlayableComponent playableComponent = m_playableManager.GetPlayableById(m_tempPlayableID);
		ResourceName prefabToSpawn = m_tempPrefab;
		PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_GameModeCoop.Respawn(m_tempPlayerID, respawnData);
	}
	
	void SetNewPlayerValues(IEntity oldEntity, int groupID, string prefab, int playerID)
	{
		IEntity newEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		RplId newPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
		m_entitySlots.Remove(oldEntity);
		m_entityPrefabs.Remove(oldEntity);
		m_entityID.Remove(oldEntity);
		m_entityPlayable.Remove(oldEntity);
		if(!newEntity)
			return;
		m_entitySlots.Insert(newEntity, groupID);
		m_entityPrefabs.Insert(newEntity, prefab);
		m_entityID.Insert(newEntity, playerID);
		m_entityPlayable.Insert(newEntity, newPlayableID);
			
	}
	
	void SetLatePlayerValues(int groupID, int time)
	{
		SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
		array<int> groupPlayersIDs = group.GetPlayerIDs();
			m_respawnTimeout.Insert(groupID, false);
			foreach (int playerID: groupPlayersIDs)
			{
				
				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);	
				if(m_entitySlots.Get(controlledEntity))
					break;		
				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
				RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
				IEntity oldEntity = m_entityID.GetKeyByValue(playerID);
				if(oldEntity)
				{
					m_entitySlots.Remove(oldEntity);
					m_entityPrefabs.Remove(oldEntity);
					m_entityID.Remove(oldEntity);
					m_entityPlayable.Remove(oldEntity);
				}
				m_entitySlots.Insert(controlledEntity, groupID);
				m_entityPrefabs.Insert(controlledEntity, prefabName);
				m_entityID.Insert(controlledEntity, playerID);
				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
			}
		time = time - 60000;
		if(time > 0)
		{
			GetGame().GetCallqueue().CallLater(SetLatePlayerValues, time, false, groupID, time);
			return;
		}
		GetGame().GetCallqueue().CallLater(InitDeleteRedundantUnitsSlowly, 1000, false, groupID);
	}
	
	void InitDeleteRedundantUnitsSlowly(int groupID) 
	{
	  // Slowly delete AI on another thread so we dont create any massive lag spikes.
		m_playableManager = PS_PlayableManager.GetInstance();
		m_mPlayables = m_playableManager.GetPlayables();
		m_mPlayablesCount = m_mPlayables.Count();
		SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
		array<int> groupPlayersIDs = group.GetPlayerIDs();
			m_respawnTimeout.Insert(groupID, false);
			foreach (int playerID: groupPlayersIDs)
			{
				
				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);	
				if(m_entitySlots.Get(controlledEntity))
					continue;		
				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
				RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
				IEntity oldEntity = m_entityID.GetKeyByValue(playerID);
				if(oldEntity)
				{
					m_entitySlots.Remove(oldEntity);
					m_entityPrefabs.Remove(oldEntity);
					m_entityID.Remove(oldEntity);
					m_entityPlayable.Remove(oldEntity);
				}
				m_entitySlots.Insert(controlledEntity, groupID);
				m_entityPrefabs.Insert(controlledEntity, prefabName);
				m_entityID.Insert(controlledEntity, playerID);
				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
			}
	  GetGame().GetCallqueue().CallLater(DeleteRedundantUnitsSlowly, 125, true, groupID);
	}
	
	//------------------------------------------------------------------------------------------------
	void DeleteRedundantUnitsSlowly(int groupID) 
	{
	  if (m_mPlayablesCount > 0) 
		{
	    	PS_PlayableComponent playable = m_mPlayables.GetElement(m_mPlayablesCount - 1);
			
	    	if(!playable)
	    		return;
			
	    	m_mPlayablesCount = m_mPlayablesCount - 1;
	   		if (m_playableManager.GetPlayerByPlayable(playable.GetId()) <= 0)
	    	{
	    		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
				int characterGroupID = m_playableManager.GetPlayerGroupByPlayable(playable.GetId()).GetGroupID();
				if(characterGroupID == groupID)
				{
	     			 SCR_EntityHelper.DeleteEntityAndChildren(character); 
				} else
					m_mPlayables.RemoveElement(m_mPlayablesCount);
	    	}        
	  	} else
	    	GetGame().GetCallqueue().Remove(DeleteRedundantUnitsSlowly);
	}
}