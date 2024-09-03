[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RadioRespawnSystemComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RadioRespawnSystemComponent: SCR_BaseGameModeComponent
{
	[Attribute(desc: "Prefabs for Blufor", category: "Blufor Respawns")];
	protected ref array<string> m_bluforRespawns;
	
	[Attribute(desc: "Prefabs for Blufor", category: "Blufor Respawns")];
	protected ref array<string> m_bluforSpawnPoints;
	
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	protected ref map<IEntity, ref array<string>> m_entitySlots = new map<IEntity, ref array<string>>();
	CRF_SafestartGameModeComponent m_safestart;
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, true);
			Print("Checking if safestart is enabled");
		}
		
	}
	
	void RespawnInit()
	{
		if (m_safestart.GetSafestartStatus())
		{
			Print("Safestart was canceled");
			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
		}
		if (!m_safestart.GetSafestartStatus())
		{
			Print("Safestart still disabled getting slots");
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
			string groupIDString = groupID.ToString();
			foreach (int playerID: groupPlayersIDs)
			{
				IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);			
				string prefabName = controlledEntity.GetPrefabData().GetPrefabName();
				m_entitySlots.Insert(controlledEntity, {groupIDString, prefabName});
			}
		}
	}
	
	void WaitTillGameStart()
	{
		m_safestart = CRF_SafestartGameModeComponent.GetInstance();
		if (m_safestart.GetSafestartStatus()) 
		{
			Print("Safestart enabled");
			GetGame().GetCallqueue().Remove(WaitTillGameStart);
			GetGame().GetCallqueue().CallLater(WaitSafeStartEnd, 1000, true);
		}
		return;
	}
	
	void WaitSafeStartEnd()
	{
		if (!m_safestart.GetSafestartStatus()) 
		{
			Print("Safestart disabled");
			GetGame().GetCallqueue().Remove(WaitSafeStartEnd);
			GetGame().GetCallqueue().CallLater(RespawnInit, 1000);
		}
		return;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SpawnGroup(array<string> prefabs, array<string> spawnpoints)
	{
	}
}