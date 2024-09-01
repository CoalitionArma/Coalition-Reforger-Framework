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
	
	override protected void OnPostInit(IEntity owner)
	{
		if (!GetGame().InPlayMode()) 
			return;
		
	}
	
	//Sides are as follows
	//BLUFOR = 0
	//OPFOR = 1
	//INDFOR = 2
	void SpawnGroup(int side)
	{
		if (side == 0)
		{
			RPC_SpawnGroup(m_bluforRespawns, m_bluforSpawnPoints);
			Rpc(RPC_SpawnGroup, m_bluforRespawns, m_bluforSpawnPoints);
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RPC_SpawnGroup(array<string> prefabs, array<string> spawnpoints)
	{
		protected vector spawnVector;
		foreach (int index, string prefab : prefabs)
		{
			spawnVector = GetGame().GetWorld().FindEntityByName(spawnpoints[index]).GetOrigin();
			EntitySpawnParams spawnParams = new EntitySpawnParams();
            spawnParams.TransformMode = ETransformMode.WORLD;
            spawnParams.Transform[3] = spawnVector;
			GetGame().SpawnEntityPrefab(Resource.Load(prefab),GetGame().GetWorld(),spawnParams);
		}
	}
}