class SCR_RadioRespawnSystem : SCR_InventoryAction
{
	protected SCR_RadioComponent m_RadioComp;
	protected int m_groupRespawns;
	protected int m_respawnWaves;
	protected CRF_RadioRespawnSystemComponent m_radioComponent;
	protected string m_factionKey;
	protected int m_groupID;
	protected CRF_SafestartGameModeComponent m_safestart;
	protected IEntity m_player;
	protected IEntity m_gamemode;
	protected int m_playerID = -1;
	protected SCR_GroupsManagerComponent m_groupManager;
	SCR_AIGroup m_playerGroup;
	
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_RadioComp)
			return false;

		if(!m_player)
			m_player = user;
		
		CharacterControllerComponent charComp = CharacterControllerComponent.Cast(user.FindComponent(CharacterControllerComponent));
		return charComp.GetInspect();
	}
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_RadioComp = SCR_RadioComponent.Cast(pOwnerEntity.FindComponent(SCR_RadioComponent));
	}
	
	override protected void PerformActionInternal(SCR_InventoryStorageManagerComponent manager, IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_SoundManagerEntity soundMan = GetGame().GetSoundManagerEntity();
		m_groupManager = SCR_GroupsManagerComponent.GetInstance();
		m_gamemode = GetGame().GetWorld().FindEntityByName("CRF_GameMode_Lobby_1");
		m_radioComponent = CRF_RadioRespawnSystemComponent.Cast(m_gamemode.FindComponent(CRF_RadioRespawnSystemComponent));
		if(m_radioComponent)
		{
		
			m_playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
			m_playerGroup = m_groupManager.GetPlayerGroup(m_playerID);
			m_factionKey = m_playerGroup.GetFaction().GetFactionKey();
			m_groupID = m_playerGroup.GetGroupID();
			m_groupRespawns = m_radioComponent.GetRespawnedGroups(m_groupID);
			m_respawnWaves = m_radioComponent.GetAmountofWave(m_factionKey);
			bool canRespawn = m_radioComponent.CanFactionRespawn(m_factionKey);
		
			if (!soundMan)
				return;
			
			if (!canRespawn)
			{
				Print("can't respawn");
				soundMan.CreateAndPlayAudioSource(pOwnerEntity,SCR_SoundEvent.SOUND_ITEM_RADIO_TUNE_ERROR);
				return;
			}
			
			if (m_groupRespawns >= m_respawnWaves)
			{
				soundMan.CreateAndPlayAudioSource(pOwnerEntity,SCR_SoundEvent.SOUND_ITEM_RADIO_TUNE_ERROR);
				return;
			}
			Print(m_radioComponent);
			
			PlayerController playerController = GetGame().GetPlayerController();
			//playerController.m_radioComponent.SpawnGroup(m_groupID);
			Print(m_radioComponent.SpawnGroup(m_groupID, playerController));
			
			soundMan.CreateAndPlayAudioSource(pOwnerEntity,SCR_SoundEvent.SOUND_ITEM_RADIO_TUNE_UP);
		} else
			soundMan.CreateAndPlayAudioSource(pOwnerEntity,SCR_SoundEvent.SOUND_ITEM_RADIO_TUNE_ERROR);
	}

	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
	
	override bool GetActionNameScript(out string outName)
	{
		int wavesLeft = GetWavesLeft();
		outName = string.Format("Call for Reinforcements | %1 Left", wavesLeft);

		return true;
	}
	
	int GetWavesLeft()
	{
		if(m_playerID == -1)
			valueinit();
		m_groupRespawns = m_radioComponent.GetRespawnedGroups(m_groupID);
		m_respawnWaves = m_radioComponent.GetAmountofWave(m_factionKey);		
		return m_respawnWaves - m_groupRespawns;
	}
	
	void valueinit()
	{
		m_gamemode = GetGame().GetWorld().FindEntityByName("CRF_GameMode_Lobby_1");
		m_radioComponent = CRF_RadioRespawnSystemComponent.Cast(m_gamemode.FindComponent(CRF_RadioRespawnSystemComponent));
		m_playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(m_player);
		m_groupManager = SCR_GroupsManagerComponent.GetInstance();
		m_playerGroup = m_groupManager.GetPlayerGroup(m_playerID);
		m_factionKey = m_playerGroup.GetFaction().GetFactionKey();
		m_groupID = m_playerGroup.GetGroupID();
	}
	
	
}