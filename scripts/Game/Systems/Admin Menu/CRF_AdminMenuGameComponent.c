[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_AdminMenuGameComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_AdminMenuGameComponent: SCR_BaseGameModeComponent
{
	protected SCR_GroupsManagerComponent m_groupsManager;
	protected CRF_GearScriptGamemodeComponent m_GearScriptEditor;
	protected static CRF_AdminMenuGameComponent s_Instance;
	
	static CRF_AdminMenuGameComponent GetInstance()
	{
		return s_Instance;
	}
	
	void SetPlayerGroup(SCR_AIGroup group, int playerID)
	{
		m_groupsManager.AddPlayerToGroup(group.GetGroupID(), playerID);
	}
		
	void SpawnGroupServer(int playerId, string prefab, vector spawnLocation, int groupID)
	{
		Rpc(Respawn, playerId, prefab, spawnLocation, groupID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void Respawn(int playerId, string prefab, vector spawnLocation, int groupID)
	{
		RandomGenerator random = new RandomGenerator();
		random.SetSeed(Math.Randomize(-1));
		m_groupsManager = SCR_GroupsManagerComponent.GetInstance();
		Resource resource = Resource.Load(prefab);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
		vector finalSpawnLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, spawnLocation, 3);
        spawnParams.Transform[3] = finalSpawnLocation;
		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		
		PS_PlayableComponent playableComponentNew = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		playableComponentNew.SetPlayable(true);
		
		GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, entity, 4, groupID);
	}
	
	void SwitchToSpawnedEntity(int playerId, IEntity entity, int frameCounter, int groupID)
	{
		if (frameCounter > 0) // Await four frames
		{		
			GetGame().GetCallqueue().Call(SwitchToSpawnedEntity, playerId, entity, frameCounter - 1, groupID);
			return;
		}
		
		PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
		
		PS_PlayableComponent playableComponent = PS_PlayableComponent.Cast(entity.FindComponent(PS_PlayableComponent));
		RplId playableId = playableComponent.GetId();
		
		if (playerId >= 0)
		{
			playableManager.SetPlayerPlayable(playerId, playableId);
			playableManager.ForceSwitch(playerId);
		}
		SCR_AIGroup playerGroup = m_groupsManager.FindGroup(groupID);
		GetGame().GetCallqueue().CallLater(SetPlayerGroup, 500, false, playerGroup, playerId);
		
	}
	
	void SetPlayerGear(int playerID, ResourceName prefab)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		m_GearScriptEditor = CRF_GearScriptGamemodeComponent.GetInstance();
		if(m_GearScriptEditor)
		{
			m_GearScriptEditor.AddGearToEntity(entity, prefab);
		}
	}
	
	void ClearGear(int playerID)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		SCR_CharacterInventoryStorageComponent entityInventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		InventoryStorageManagerComponent entityInventoryManager = InventoryStorageManagerComponent.Cast(entity.FindComponent(InventoryStorageManagerComponent));
		ref array<Managed> weaponSlotComponentArray = {};
		
		entity.FindComponents(WeaponSlotComponent, weaponSlotComponentArray);
		
		for(int i = 0; i < 18; i++)
		{
			if(entityInventory.Get(i))
			{
				IEntity item = entityInventory.Get(i);
				entityInventoryManager.TryRemoveItemFromStorage(item, entityInventory);
				SCR_EntityHelper.DeleteEntityAndChildren(item);
			}
		}
		
		foreach(Managed weaponSlot : weaponSlotComponentArray)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(weaponSlot);
			if(weaponSlotComponent.GetWeaponEntity())
				delete weaponSlotComponent.GetWeaponEntity();
		}
	}
}