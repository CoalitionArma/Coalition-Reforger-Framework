[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RadioRespawnSystemComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RadioRespawnSystemComponent: SCR_BaseGameModeComponent
{

	[Attribute(desc: "Prefabs for Blufor", category: "Blufor Respawns")];
	protected ref array<string> m_bluforSpawnPoints;
	
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	protected ref map<int, int> m_entitySlots = new map<int, int>();
	protected ref map<int, string> m_entityPrefabs = new map<int, string>();
	CRF_SafestartGameModeComponent m_safestart;
	protected ref array<int> m_respawnedGroups;
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
		}
		
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
				m_entitySlots.Insert(1, groupID);
				m_entitySlots.Insert(2, groupID);
				m_entitySlots.Insert(3, 1);
				m_entityPrefabs.Insert(1, prefabName);
				m_entityPrefabs.Insert(2, prefabName);
				m_entityPrefabs.Insert(3, prefabName);
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
		RPC_SpawnGroup(groupID);
		Rpc(RPC_SpawnGroup, groupID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SpawnGroup(int groupID)
	{
		for(int i, count = m_entitySlots.Count(); i < count; ++i)
		{
			int tempEntity = m_entitySlots.GetKeyByValue(groupID);
			if (!tempEntity)
				return;
			
			Print("hello");
			m_entitySlots.Remove(tempEntity);
		}
	}
}