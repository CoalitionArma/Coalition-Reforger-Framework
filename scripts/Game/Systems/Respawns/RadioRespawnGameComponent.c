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
	int m_tempPlayerID;
	RplId m_tempPlayableID;
	
	IEntity m_tempEntity;
	int m_groupID;
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
			//GetGame().GetCallqueue().CallLater(SpawnGroup, 10000, false, 0);
		}
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
				Print(playerPlayableID);
				m_entitySlots.Insert(controlledEntity, groupID);
				m_entityPrefabs.Insert(controlledEntity, prefabName);
				m_entityID.Insert(controlledEntity, playerID);
				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
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
			return;

		m_respawnedGroups.Insert(groupID);
		
		bool canRespawn;
		m_respawnTimeout.Find(groupID, canRespawn);
		
		if(!canRespawn)
			return;
		
		m_respawnTimeout.Set(groupID, true);
		
		m_playableManager = PS_PlayableManager.GetInstance();
		
		for(int i, count = m_entitySlots.Count(); i < count; ++i)
		{
			m_groupID = groupID;
			m_tempEntity = m_entitySlots.GetKeyByValue(groupID);
			m_tempPlayerID = m_entityID.Get(m_tempEntity);
			Replication.BumpMe();
			m_tempPlayableID = m_entityPlayable.Get(m_tempEntity);
			Replication.BumpMe();
			
			if (!m_tempEntity)
				return;
			
			if(SCR_AIDamageHandling.IsAlive(m_tempEntity))
				{
					m_entitySlots.Remove(m_tempEntity);
					m_entityPrefabs.Remove(m_tempEntity);
					m_entityID.Remove(m_tempEntity);
					m_entityPlayable.Remove(m_tempEntity);
					m_entitySlots.Insert(m_tempEntity, m_groupID);
					m_entityPrefabs.Insert(m_tempEntity, m_tempPrefab);
					m_entityID.Insert(m_tempEntity, m_tempPlayerID);
					m_entityPlayable.Insert(m_tempEntity, m_tempPlayableID);
					return;
				}	
						
			m_tempPrefab = m_entityPrefabs.Get(m_tempEntity);
			Replication.BumpMe();
			
			SpawnPrefabs();
			
		}
	}
	
	void SpawnPrefabs()
	{
		m_playableManager = PS_PlayableManager.GetInstance();
		if(m_tempPlayableID == RplId.Invalid())
			return;
		PS_PlayableComponent playableComponent = m_playableManager.GetPlayableById(m_tempPlayableID);
		ResourceName prefabToSpawn = m_tempPrefab;
		PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
		m_GameModeCoop = PS_GameModeCoop.Cast(GetGame().GetGameMode());
		m_GameModeCoop.Respawn(m_tempPlayerID, respawnData);
	}
	
	void SetNewPlayerValues(int groupID)
	{
		SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
		array<int> groupPlayersIDs = group.GetPlayerIDs();
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
	
	void InitDeleteRedundantUnitsSlowly() 
	{
	  // Slowly delete AI on another thread so we dont create any massive lag spikes.
	  m_playableManager = PS_PlayableManager.GetInstance();
	  m_mPlayables = m_playableManager.GetPlayables();
	  m_mPlayablesCount = m_mPlayables.Count();
	  GetGame().GetCallqueue().CallLater(DeleteRedundantUnitsSlowly, 125, true);
	}
	
	//------------------------------------------------------------------------------------------------
	void DeleteRedundantUnitsSlowly() 
	{
	  if (m_mPlayablesCount > 0) {
	    PS_PlayableComponent playable = m_mPlayables.GetElement(m_mPlayablesCount - 1);
	    if(!playable)
	      return;
	    m_mPlayablesCount = m_mPlayablesCount - 1;
	    if (m_playableManager.GetPlayerByPlayable(playable.GetId()) <= 0)
	    {
	      SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
	      SCR_EntityHelper.DeleteEntityAndChildren(character);            
	    }        
	  } else
	    GetGame().GetCallqueue().Remove(DeleteRedundantUnitsSlowly);
	}
}