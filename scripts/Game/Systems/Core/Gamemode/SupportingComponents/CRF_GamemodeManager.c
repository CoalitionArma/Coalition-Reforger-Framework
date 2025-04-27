class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	const static ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
	// Instance of this component (this method only works if you KNOW there will only ever be one instance of this component) 
	protected static CRF_GamemodeManager s_Instance;
	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	//------------------------------------------------------------------------------------------------
	void CRF_GamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeManager GetInstance()
	{
		return s_Instance;
	}	
	
	//------------------------------------------------------------------------------------------------
	static bool IsSpectator(IEntity entity)
	{
		return entity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool IsSpectator()
	{
		if (SCR_PlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;
		else if(SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;
		else
			return false;
	}

	//Called to enter the actual game, just puts the player into a slot or spectator.
	//------------------------------------------------------------------------------------------------
	void InitilizePlayer(int playerId, vector overrideLocation = vector.Zero)
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		if (!slottingManager.IsPlayerInASlot(playerId)
			|| !CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId))
			|| slottingManager.IsPlayerConsideredDead(playerId)) {
				EnterSpectator(playerId);
				return;
		}
		
		IEntity playerCharacter = slottingManager.GetPlayerSlotCharacter(playerId);
		
		if (!playerCharacter)
		{
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			
			if(overrideLocation != vector.Zero)
				spawnParams.Transform[3] = overrideLocation;
			else
				slottingManager.GetPlayerSlotVector(playerId, spawnParams.Transform);
			
			playerCharacter = GetGame().SpawnEntityPrefab(Resource.Load(slottingManager.GetPlayerSlotResource(playerId)), GetGame().GetWorld(), spawnParams);
		
			slottingManager.UpdateSlotCharacter(slottingManager.GetPlayerSlotID(playerId), RplComponent.Cast(playerCharacter.FindComponent(RplComponent)).Id())
		};
			
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		playerController.SetInitialMainEntity(playerCharacter);

		SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(slottingManager.GetPlayerSlotFaction(playerId));

		int groupId = slottingManager.GetPlayerSlotGroup(playerId).GetGroupID();

		if (groupId != -1)
		{
			SCR_GroupsManagerComponent.GetInstance().AddPlayerToGroup(groupId, playerId);
			SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
		}
		
		CRF_RplBroadcastManager.GetInstance().InitilizePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void EnterSpectator(int playerId, IEntity entity = null)
	{
		IEntity initialEntity = GetGame().SpawnEntityPrefab(Resource.Load("{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et"), GetGame().GetWorld());
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		GetGame().GetCallqueue().CallLater(pc.SetInitialMainEntity, 250, false, initialEntity);

		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
		
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(initialEntity.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if (damManager)
			damManager.EnableDamageHandling(false);
		
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));

		vector cameraPos[4];
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			if (CRF_SlottingManager.GetInstance().IsPlayerInASlot(playerId) && entity != null)
			{
				entity.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			} else
				cameraPos[3] = CRF_Gamemode.GetInstance().m_vGenericSpawn[3];
		} else
			cameraPos[3] = "0 10000 0";

		CRF_RplBroadcastManager.GetInstance().SendSpecClientInit(playerId, cameraPos);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		m_aModerators.Insert(playerId);
		Replication.BumpMe();
	};
}
