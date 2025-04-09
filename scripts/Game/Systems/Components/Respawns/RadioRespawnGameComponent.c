[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RadioRespawnSystemComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RadioRespawnSystemComponent: SCR_BaseGameModeComponent
{	
//	[Attribute("false", "auto", "Can Blufor Respawn")];
//	protected bool m_canBluforRespawn;
//	
//	[Attribute("US", "auto", "Blufor Faction Key")];
//	protected string m_bluforFactionKey;
//	
//	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements on Blufor.")];
//	protected int m_bluforRespawnWaves;
//	
//	[Attribute("false", "auto", "Can Opfor Respawn")];
//	protected bool m_canOpforRespawn;
//	
//	[Attribute("USSR", "auto", "Opfor Faction Key")];
//	protected string m_opforFactionKey;
//	
//	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements on Opfor.")];
//	protected int m_opforRespawnWaves;
//	
//	[Attribute("false", "auto", "Can Indfor Respawn")];
//	protected bool m_canIndforRespawn;
//	
//	[Attribute("CDF", "auto", "Indfor Faction Key")];
//	protected string m_indforFactionKey;
//	
//	[Attribute("1", "auto", "Amount of times SLs can call in reinforcements on Indfor.")];
//	protected int m_indforRespawnWaves;
//	
//	[RplProp()]
//	private string m_clientBluforFactionKey;
//	[RplProp()]
//	private bool m_clientCanBluforRespawn;
//	[RplProp()]
//	private int m_clientBluforRespawnWaves;
//	[RplProp()]
//	private string m_clientOpforFactionKey;
//	[RplProp()]
//	private bool m_clientCanOpforRespawn;
//	[RplProp()]
//	private int m_clientOpforRespawnWaves;
//	[RplProp()]
//	private string m_clientIndforFactionKey;
//	[RplProp()]
//	private bool m_clientCanIndforRespawn;
//	[RplProp()]
//	private int m_clientIndforRespawnWaves;
//	[RplProp()]
//	private ref array<int> m_respawnedGroups = {};
//	[RplProp()]
//	private ref array<int> m_respawnTimeout = {};
//	
//	
//	//[RplProp(onRplName: "SpawnPrefabs")]
//	ResourceName m_tempPrefab;
//	int m_tempPlayerID;
//	RplId m_tempPlayableID;
//	
//	IEntity m_tempEntity;
//	PS_PlayableManager m_playableManager;
//	protected PS_GameModeCoop m_GameModeCoop;
//	
//	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
//	CRF_Gamemode m_safestart;
//	
//	protected ref map<IEntity, int> m_entitySlots = new map<IEntity, int>();
//	protected ref map<IEntity, ResourceName> m_entityPrefabs = new map<IEntity, ResourceName>();
//	protected ref map<IEntity, int> m_entityID = new map<IEntity, int>();
//	protected ref map<IEntity, RplId> m_entityPlayable = new map<IEntity, RplId>();
//		
//	protected ref map<RplId, PS_PlayableComponent> m_mPlayables = new map<RplId, PS_PlayableComponent>;
//	protected int m_mPlayablesCount = 0;
//	
//	override protected void OnPostInit(IEntity owner)
//	{
//		if (!GetGame().InPlayMode()) 
//			return;
//		
//		if (RplSession.Mode() == RplMode.Dedicated)
//		{
//			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
//		}
//	}
//	
//	void WaitTillGameStart()
//	{
//		m_safestart = CRF_Gamemode.GetInstance();
//		if (m_safestart.GetSafestartStatus()) 
//		{	
//			GetGame().GetCallqueue().Remove(WaitTillGameStart);
//			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
//			m_clientBluforFactionKey = m_bluforFactionKey;
//			m_clientCanBluforRespawn = m_canBluforRespawn;
//			m_clientBluforRespawnWaves = m_bluforRespawnWaves;
//			m_clientOpforFactionKey = m_opforFactionKey;
//			m_clientCanOpforRespawn = m_canOpforRespawn;
//			m_clientOpforRespawnWaves = m_opforRespawnWaves;
//			m_clientIndforFactionKey = m_indforFactionKey;
//			m_clientCanIndforRespawn = m_canIndforRespawn;
//			m_clientIndforRespawnWaves = m_indforRespawnWaves;
//			Replication.BumpMe();
//		}
//		return;
//	}
//	
//	void WaitSafeStartEnd()
//	{
//		if (!m_safestart.GetSafestartStatus()) 
//		{
//			GetGame().GetCallqueue().Remove(WaitSafeStartEnd);
//			GetGame().GetCallqueue().CallLater(RespawnInit, 1000);
//		}
//		return;
//	}
//	
//	void RespawnInit()
//	{
//		if (m_safestart.GetSafestartStatus())
//		{
//			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
//		}
//		if (!m_safestart.GetSafestartStatus())
//		{
//			GetGroups();
//		}
//	}
//	
//	int GetAmountofWave(string factionKey)
//	{
//		if(factionKey == m_clientBluforFactionKey)
//			return m_clientBluforRespawnWaves;
//		if(factionKey == m_clientOpforFactionKey)
//			return m_clientOpforRespawnWaves;
//		return m_clientIndforRespawnWaves;
//	}
//	
//	int GetRespawnedGroups(int groupID)
//	{
//		int timesRespawned = 0;
//		foreach (int wave : m_respawnedGroups)
//		{
//			if(wave == groupID)
//			{
//				timesRespawned++;
//			}
//		}
//		return timesRespawned;
//	}
//	
//	void AddRespawnWaves(string factionKey, int amount)
//	{
//		if(Replication.IsClient())
//		{
//			Print("RUN AddRespawnWaves ONLY ON THE SERVER");
//			return;
//		}
//		if(factionKey == m_bluforFactionKey)
//		{
//			m_bluforRespawnWaves = m_bluforRespawnWaves + amount;
//			m_clientBluforRespawnWaves = m_bluforRespawnWaves;
//			Replication.BumpMe();
//			return;
//		}
//		if(factionKey == m_opforFactionKey)
//		{
//			m_opforRespawnWaves = m_opforRespawnWaves + amount;
//			m_clientOpforRespawnWaves = m_opforRespawnWaves;
//			Replication.BumpMe();
//			return;
//		}
//		m_indforRespawnWaves = m_indforRespawnWaves + amount;
//		m_clientIndforRespawnWaves = m_indforRespawnWaves;
//		Replication.BumpMe();
//		return;
//	}
//	
//	void RemoveRespawnWaves(string factionKey, int amount)
//	{
//		if(Replication.IsClient())
//		{
//			Print("RUN RemoveRespawnWaves ONLY ON THE SERVER");
//			return;
//		}
//		if(factionKey == m_bluforFactionKey)
//		{
//			m_bluforRespawnWaves = m_bluforRespawnWaves - amount;
//			m_clientBluforRespawnWaves = m_bluforRespawnWaves;
//			Replication.BumpMe();
//			return;
//		}
//		if(factionKey == m_opforFactionKey)
//		{
//			m_opforRespawnWaves = m_opforRespawnWaves - amount;
//			m_clientOpforRespawnWaves = m_opforRespawnWaves;
//			Replication.BumpMe();
//			return;
//		}
//		m_indforRespawnWaves = m_indforRespawnWaves - amount;
//		m_clientIndforRespawnWaves = m_indforRespawnWaves;
//		Replication.BumpMe();
//		return;
//	}
//	
//	void SetCanFactionRespawn(string factionKey, bool canRespawn)
//	{
//		if(Replication.IsClient())
//		{
//			Print("RUN SetCanFactionRespawn ONLY ON THE SERVER");
//			return;
//		}
//		if(factionKey == m_bluforFactionKey)
//		{
//			m_canBluforRespawn = canRespawn;
//			m_clientCanBluforRespawn = m_canBluforRespawn;
//			Replication.BumpMe();
//			return;
//		}
//		if(factionKey == m_opforFactionKey)
//		{
//			m_canOpforRespawn = canRespawn;
//			m_clientCanOpforRespawn = m_canOpforRespawn;
//			Replication.BumpMe();
//			return;
//		}
//		m_canIndforRespawn = canRespawn;
//		m_clientCanIndforRespawn = m_canIndforRespawn;
//		Replication.BumpMe();
//		return;
//	}
//	
//	bool CanFactionRespawn(string factionID)
//	{
//		if(factionID == m_clientBluforFactionKey)
//		{
//			return m_clientCanBluforRespawn;
//		}
//		else if(factionID == m_clientOpforFactionKey)
//			return m_clientCanOpforRespawn;
//		else
//			return m_clientCanIndforRespawn;
//	}
//	
//	bool InRespawnTimeout(int groupID)
//	{
//		bool canRespawn;
//		if(m_respawnTimeout.Find(groupID) != -1)
//			return true;
//		return false;
//	}
//	
//	void GetGroups()
//	{
//		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
//		array<SCR_AIGroup> outAllGroups;
//		m_GroupsManagerComponent.GetAllPlayableGroups(outAllGroups);
//		m_playableManager = PS_PlayableManager.GetInstance();
//		
//		foreach (SCR_AIGroup group : outAllGroups)
//		{
//			array<int> groupPlayersIDs = group.GetPlayerIDs();
//			int groupID = group.GetGroupID();
//			Replication.BumpMe();
//			foreach (int playerID: groupPlayersIDs)
//			{
//				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);			
//				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
//				RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
//				m_entitySlots.Insert(controlledEntity, groupID);
//				m_entityPrefabs.Insert(controlledEntity, prefabName);
//				m_entityID.Insert(controlledEntity, playerID);
//				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
//			}
//		}
//		
//	}
//	
//	void SpawnGroupServer(int groupID)
//	{
//		m_respawnedGroups.Insert(groupID);
//		m_respawnTimeout.Insert(groupID);
//		Replication.BumpMe();
//		
//		m_playableManager = PS_PlayableManager.GetInstance();
//		for(int i, count = m_entitySlots.Count(); i < count; ++i)
//		{
//			m_tempEntity = m_entitySlots.GetKeyByValue(groupID);
//			
//			m_tempPlayerID = m_entityID.Get(m_tempEntity);
//			m_tempPlayableID = m_entityPlayable.Get(m_tempEntity);
//			m_tempPrefab = m_entityPrefabs.Get(m_tempEntity);
//			
//			m_entitySlots.Remove(m_tempEntity);
//			m_entityPrefabs.Remove(m_tempEntity);
//			m_entityID.Remove(m_tempEntity);
//			m_entityPlayable.Remove(m_tempEntity);
//			
//			if(SCR_AIDamageHandling.IsAlive(m_tempEntity))
//				{
//					continue;
//				}	
//			
//			//Replication.BumpMe();
//			//SpawnPrefabs(m_tempPlayerID, m_tempPrefab);
//			if(!GetGame().GetPlayerManager().IsPlayerConnected(m_tempPlayerID))
//				continue;
//			Rpc(SpawnPrefabs, m_tempPlayerID, m_tempPrefab, m_tempPlayableID);
//		}
//		GetGame().GetCallqueue().CallLater(SetNewPlayerValues, 500, false, groupID);
//		GetGame().GetCallqueue().CallLater(SetLatePlayerValues, 60000, false, groupID, 60000);
//	}
//	
//	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
//	void SpawnPrefabs(int playerID, ResourceName loadoutPrefab, RplId playerPlayableID)
//	{
//		PS_PlayableManager clientplayableManager = PS_PlayableManager.GetInstance();
//		if(playerPlayableID == RplId.Invalid())
//			return;
//		PS_PlayableComponent playableComponent = clientplayableManager.GetPlayableById(playerPlayableID);
//		ResourceName prefabToSpawn = loadoutPrefab;
//		PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
//		Respawn(playerID, respawnData);
//	}
//	
//	void Respawn(int playerId, PS_RespawnData respawnData)
//	{
//		Resource resource = Resource.Load(respawnData.m_sPrefabName);
//		EntitySpawnParams params = new EntitySpawnParams();
//		Math3D.MatrixCopy(respawnData.m_aSpawnTransform, params.Transform);
//		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
//		PS_PlayableComponent playableComponentNew = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
//		playableComponentNew.SetPlayable(true);
//		
//		GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, respawnData, entity, 4);
//	}
//	
//	void SwitchToSpawnedEntity(int playerId, PS_RespawnData respawnData, IEntity entity, int frameCounter)
//	{
//		if (frameCounter > 0) // Await four frames
//		{		
//			GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, respawnData, entity, frameCounter - 1);
//			return;
//		}
//		
//		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
//		
//		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
//		RplId playableId = playableComponent.GetId();
//		
//		playableComponent.CopyState(respawnData);
//		if (playerId > 0)
//		{
//			playableManager.SetPlayerPlayable(playerId, playableId);
//			playableManager.ForceSwitch(playerId);
//		}
//		SCR_AIGroup aiGroup = m_playableManager.GetPlayerGroupByPlayable(respawnData.m_iId);
//		GetGame().GetCallqueue().CallLater(SetPlayerGroup, 500, false, aiGroup, playerId);
//	}
//	
//	void SetPlayerGroup(SCR_AIGroup group, int playerID)
//	{
//		m_GroupsManagerComponent.AddPlayerToGroup(group.GetGroupID(), playerID);
//	}
//	
//	void SetNewPlayerValues(int groupID)
//	{
//		SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
//		array<int> groupPlayersIDs = group.GetPlayerIDs();
//		foreach (int playerID: groupPlayersIDs)
//		{
//			IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);			
//			ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
//			RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
//			m_entitySlots.Insert(controlledEntity, groupID);
//			m_entityPrefabs.Insert(controlledEntity, prefabName);
//			m_entityID.Insert(controlledEntity, playerID);
//			m_entityPlayable.Insert(controlledEntity, playerPlayableID);
//		}		
//	}
//	
//	void SetLatePlayerValues(int groupID, int time)
//	{
//		SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
//		array<int> groupPlayersIDs = group.GetPlayerIDs();
//			foreach (int playerID: groupPlayersIDs)
//			{
//				
//				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);	
//				if(m_entitySlots.Get(controlledEntity))
//					continue;		
//				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
//				RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
//				IEntity oldEntity = m_entityID.GetKeyByValue(playerID);
//				if(oldEntity)
//				{
//					m_entitySlots.Remove(oldEntity);
//					m_entityPrefabs.Remove(oldEntity);
//					m_entityID.Remove(oldEntity);
//					m_entityPlayable.Remove(oldEntity);
//				}
//				m_entitySlots.Insert(controlledEntity, groupID);
//				m_entityPrefabs.Insert(controlledEntity, prefabName);
//				m_entityID.Insert(controlledEntity, playerID);
//				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
//			}
//		time = time + 1;
//		if(time < 60004)
//		{
//			GetGame().GetCallqueue().CallLater(SetLatePlayerValues, time, false, groupID, time);
//			return;
//		}
//		GetGame().GetCallqueue().CallLater(InitDeleteRedundantUnitsSlowly, 1000, false, groupID);
//	}
//	
//	void InitDeleteRedundantUnitsSlowly(int groupID) 
//	{
//	  // Slowly delete AI on another thread so we dont create any massive lag spikes.
//		m_playableManager = PS_PlayableManager.GetInstance();
//		m_mPlayables = m_playableManager.GetPlayables();
//		m_mPlayablesCount = m_mPlayables.Count();
//		SCR_AIGroup group = m_GroupsManagerComponent.FindGroup(groupID);
//		array<int> groupPlayersIDs = group.GetPlayerIDs();
//			m_respawnTimeout.Remove(m_respawnTimeout.Find(groupID));
//			Replication.BumpMe();
//			foreach (int playerID: groupPlayersIDs)
//			{
//				
//				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);	
//				if(m_entitySlots.Get(controlledEntity))
//					continue;		
//				ResourceName prefabName = controlledEntity.GetPrefabData().GetPrefabName();
//				RplId playerPlayableID = m_playableManager.GetPlayableByPlayer(playerID);
//				IEntity oldEntity = m_entityID.GetKeyByValue(playerID);
//				if(oldEntity)
//				{
//					m_entitySlots.Remove(oldEntity);
//					m_entityPrefabs.Remove(oldEntity);
//					m_entityID.Remove(oldEntity);
//					m_entityPlayable.Remove(oldEntity);
//				}
//				m_entitySlots.Insert(controlledEntity, groupID);
//				m_entityPrefabs.Insert(controlledEntity, prefabName);
//				m_entityID.Insert(controlledEntity, playerID);
//				m_entityPlayable.Insert(controlledEntity, playerPlayableID);
//			}
//	  GetGame().GetCallqueue().CallLater(DeleteRedundantUnitsSlowly, 125, true, groupID);
//	}
//	
//	//------------------------------------------------------------------------------------------------
//	void DeleteRedundantUnitsSlowly(int groupID) 
//	{
//	  if (m_mPlayablesCount > 0) 
//		{
//	    	PS_PlayableComponent playable = m_mPlayables.GetElement(m_mPlayablesCount - 1);
//			
//	    	if(!playable)
//	    		return;
//			
//	    	m_mPlayablesCount = m_mPlayablesCount - 1;
//	   		if (m_playableManager.GetPlayerByPlayable(playable.GetId()) <= 0)
//	    	{
//	    		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playable.GetOwner());
//				int characterGroupID = m_playableManager.GetPlayerGroupByPlayable(playable.GetId()).GetGroupID();
//				if(characterGroupID == groupID)
//				{
//	     			 SCR_EntityHelper.DeleteEntityAndChildren(character); 
//				} else
//					m_mPlayables.RemoveElement(m_mPlayablesCount);
//	    	}        
//	  	} else
//	    	GetGame().GetCallqueue().Remove(DeleteRedundantUnitsSlowly);
//	}
}