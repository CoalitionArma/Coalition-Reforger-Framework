[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_AdminMenuGameComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_AdminMenuGameComponent: SCR_BaseGameModeComponent
{
	protected SCR_GroupsManagerComponent m_groupsManager;
	protected CRF_GearScriptGamemodeComponent m_GearScriptEditor;
	protected static CRF_AdminMenuGameComponent s_Instance;
	protected ref EntitySpawnParams m_SpawnParams = new EntitySpawnParams();
	
	static CRF_AdminMenuGameComponent GetInstance()
	{
		return s_Instance;
	}
	
	override void OnPostInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(AddMsgAction, 0, false);
	}
	
	void AddMsgAction()
	{
		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		ChatCommandInvoker invoker = chatPanelManager.GetCommandInvoker("admin");
		invoker.Insert(SendAdminMessage_Callback);
		ChatCommandInvoker invoker2 = chatPanelManager.GetCommandInvoker("a");
		invoker2.Insert(SendAdminMessage_Callback);
		ChatCommandInvoker invoker3 = chatPanelManager.GetCommandInvoker("r");
		invoker3.Insert(ReplyAdminMessage_Callback);
		ChatCommandInvoker invoker4 = chatPanelManager.GetCommandInvoker("reply");
		invoker4.Insert(ReplyAdminMessage_Callback);
	}
	

	void SendAdminMessage_Callback(SCR_ChatPanel panel, string data)
	{
		CRF_ClientAdminMenuComponent.GetInstance().SendAdminMessage(data);
	}
	
	void ReplyAdminMessage_Callback(SCR_ChatPanel panel, string data)
	{
	CRF_ClientAdminMenuComponent.GetInstance().ReplyAdminMessage(data);
	}
	
	void SendAdminMessage(string data)
	{
		Rpc(RpcAsk_SendAdminMessage, data);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_SendAdminMessage(string data)
	{
		if(!SCR_Global.IsAdmin(GetGame().GetPlayerController().GetPlayerId()))
			return;
		
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		chatComponent.ShowMessage(data);
	}
	
	void ReplyAdminMessage(string data, int playerID)
	{
		Rpc(RpcAsk_ReplyAdminMessage, data, playerID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcAsk_ReplyAdminMessage(string data, int playerID)
	{
		if(GetGame().GetPlayerController().GetPlayerId() != playerID)
			return;
		
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		chatComponent.ShowMessage(data);
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
	
	void SetPlayerGearServer(int playerID, ResourceName prefab)
	{
		GetGame().GetCallqueue().CallLater(SetPlayerGear, 500, false, playerID, prefab);
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
	
	void AddItem(int playerID, string prefab)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		InventoryStorageManagerComponent entityInventoryManager = InventoryStorageManagerComponent.Cast(entity.FindComponent(InventoryStorageManagerComponent));
		m_SpawnParams.TransformMode = ETransformMode.WORLD;
        m_SpawnParams.Transform[3] = entity.GetOrigin();
		ref IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), m_SpawnParams);
		if(!entityInventoryManager.TryInsertItem(resourceSpawned))
			delete resourceSpawned;
	}
	
	void TeleportPlayers(int playerID1, int playerID2)
	{
		Rpc(Rpc_TeleportPlayers, playerID1, playerID2);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_TeleportPlayers(int playerID1, int playerID2)
	{
		if(SCR_PlayerController.GetLocalPlayerId() != playerID1)
			return;
		
		IEntity entity2 = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID2);
		EntitySpawnParams spawnParams = new EntitySpawnParams();
	    spawnParams.TransformMode = ETransformMode.WORLD;
		vector teleportLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(teleportLocation, entity2.GetOrigin(), 3);
	    spawnParams.Transform[3] = teleportLocation;
	
		SCR_Global.TeleportPlayer(playerID1, teleportLocation);
	}
	
	void SendHintAll(string data)
	{
		Rpc(Rpc_SendHintAll, data);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintAll(string data)
	{
		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");
		
		if (!widget)
			return;
		
		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		float duration = 8000;
		hint.ShowHint(data, duration);
	}
	
	void SendHintPlayer(string data, int playerID)
	{
		Rpc(Rpc_SendHintPlayer, data, playerID);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintPlayer(string data, int playerID)
	{
		if(SCR_PlayerController.GetLocalPlayerId() != playerID)
			return;
		
		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");
		
		if (!widget)
			return;
		
		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		float duration = 8000;
		hint.ShowHint(data, duration);
	
	}
	
	void SendHintFaction(string data, string factionKey)
	{
		Rpc(Rpc_SendHintFaction, data, factionKey);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void Rpc_SendHintFaction(string data, string factionKey)
	{
		if(SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction()).GetFactionKey() != factionKey)
			return;
		
		Widget widget;
		widget = GetGame().GetWorkspace().CreateWidgets("{43FC66BA3D85E9C7}UI/layouts/Hint/hint.layout");
		
		if (!widget)
			return;
		
		CRF_Hint hint = CRF_Hint.Cast(widget.FindHandler(CRF_Hint));
		float duration = 8000;
		hint.ShowHint(data, duration);
	}
}