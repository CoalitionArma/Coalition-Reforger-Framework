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
		CRF_RadioRespawnSystemComponent RadioComponent = CRF_RadioRespawnSystemComponent.Cast(gamemode .FindComponent(CRF_RadioRespawnSystemComponent));
		SCR_SoundManagerEntity soundMan = GetGame().GetSoundManagerEntity();
		
		SCR_ChimeraCharacter cc = SCR_ChimeraCharacter.Cast(pUserEntity);
		if (cc && cc.GetFactionKey() == m_tempString) m_entities.Insert(ent);
		
		Color factionColor = faction.GetFactionColor();
		float rg = Math.Max(factionColor.R(), factionColor.G());
		float rgb = Math.Max(rg, factionColor.B());
		
		RadioComponent.SpawnGroup();

		if (!soundMan)
			return;
		
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