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
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
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
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SpawnGroup(array<string> prefabs, array<string> spawnpoints)
	{
	}
}