class SCR_RadioRespawnSystem : SCR_InventoryAction
{
	protected SCR_RadioComponent m_RadioComp;
	
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_RadioComp)
			return false;

		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(user.FindComponent(CharacterControllerComponent));
		return charComp.GetInspect();
	}
	
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		IEntity gamemode = GetGame().GetWorld().FindEntityByName("CRF_GameMode_Lobby_1");
		CRF_RadioRespawnSystemComponent RadioComponent = CRF_RadioRespawnSystemComponent.Cast(gamemode.FindComponent(CRF_RadioRespawnSystemComponent));
		SCR_SoundManagerEntity soundMan = GetGame().GetSoundManagerEntity();
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		
		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		
		SCR_AIGroup playerGroup = groupManager.GetPlayerGroup(playerID);
		int groupID = playerGroup.GetGroupID();
		int groupRespawns = RadioComponent.GetRespawnedGroups(groupID);
		int respawnWaves = RadioComponent.GetAmountofWave();
		
		RadioComponent.SpawnGroup(groupID);
		
		if (!soundMan)
			return;
		if (groupRespawns >= respawnWaves)
		{
			soundMan.CreateAndPlayAudioSource(pOwnerEntity,SCR_SoundEvent.SOUND_ITEM_RADIO_TUNE_ERROR);
			return;
		}
		
		soundMan.CreateAndPlayAudioSource(pOwnerEntity,SCR_SoundEvent.SOUND_ITEM_RADIO_TUNE_UP);
	}
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_RadioComp = SCR_RadioComponent.Cast(pOwnerEntity.FindComponent(SCR_RadioComponent));
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
}