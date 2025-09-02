modded class SCR_PlayerController
{
	/**
	 * Called when the player controller updates (typically whenever a player joins/rejoins)
	 */
	override protected void UpdateLocalPlayerController()
	{
		super.UpdateLocalPlayerController();
		
		if (RplSession.Mode() == RplMode.Dedicated || !CRF_Gamemode.GetInstance() || !CRF_PlayerControllerManager.GetInstance())
			return;
		
		CRF_PlayerControllerManager.GetInstance().InitilizePlayerControllerComp();
	}
	
	/**
	 * Called when the entity controlled by this player controller changes
	 * @param from The previous entity being controlled
	 * @param to The new entity being controlled
	 */
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{	
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
		{
			// Call the parent implementation
			super.OnControlledEntityChanged(from, to);
			return;
		};
		
		// Get the CRF player controller comp
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();

		// Handle race condition: If player is being assigned initial entity when they should have a playable character
		if (to && to.GetPrefabData().GetPrefabName() == CRF_GamemodeManager.GetSpectatorResource() && 
			CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			int playerId = GetPlayerId();
			CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
			
			// Check if this player should have a proper character instead of initial entity
			if (slottingManager && slottingManager.IsPlayerInASlot(playerId) && !slottingManager.IsPlayerConsideredDead(playerId))
			{
				// Request re-initialization from server to fix race condition
				CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
				if (rplManager)
				{
					GetGame().GetCallqueue().CallLater(rplManager.RequestInitilizePlayer, 250, false, playerId);
				}
			}
		}

		// Call the parent implementation
		super.OnControlledEntityChanged(from, to);
	}

	/**
	 * Called when the player disconnects from the game
	 * Ensures settings are reset to their stored values
	 */
	override void DisconnectFromGame()
	{
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
		{
			// Call the parent implementation
			super.DisconnectFromGame();
			return;
		};

		// Get the CRF player controller comp
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerControllerComp)
			// Reset settings to previously stored values
			playerControllerComp.ResetSettingsToStoredValues();
		
		super.DisconnectFromGame();
	}
	
	//Handles initializing the m_aRadios array for both this client and the server so both are on the same page
	//Also used to load any settings the radios may have had on respawn.
	//Loading settings only works if the radios where pe configured with the CVON_FreqConfig.
	override void InitializeRadios(IEntity to)
	{
		m_aRadios.Clear();
		array<RplId> radios = CVON_VONGameModeComponent.GetInstance().GetRadios(to);
		if (!radios)
			return;
		if (radios.Count() == 0)
			return;
		ref array<IEntity> shortRangeRadios = {};
		ref array<IEntity> longRangeRadios = {};
		int SRIndex = 0;
		int LRIndex = 0;
		int FreqSRIndex = 0;
		int FreqLRIndex = 0;
		foreach (RplId radio: radios)
		{
			if (!Replication.FindItem(radio))
				continue;
			
			IEntity radioObject = RplComponent.Cast(Replication.FindItem(radio)).GetEntity();
			if (!radioObject)
				continue;
			
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radioObject.FindComponent(CVON_RadioComponent));
			FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(GetControlledEntity().FindComponent(FactionAffiliationComponent));
			if (!factionComp)
				return;
			string factionKey = factionComp.GetAffiliatedFactionKey();
			
			//Used so we can assing settings to frequencies.
			
			SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			SCR_Faction faction = SCR_Faction.Cast(factionMan.GetFactionByKey(factionKey));
			
			SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
			array<SCR_AIGroup> groups = groupManager.GetPlayableGroupsByFaction(faction);
			SCR_AIGroup playersGroup = groupManager.GetPlayerGroup(GetPlayerId());
			
			CVON_GroupFrequencyContainer freqContainer;
			int index = -1;
			if (playersGroup)
				index = groups.Find(playersGroup);
			
			string playersGroupName;
			if (index != -1)
			{
				string company;
				string platoon;
				string squad;
				string character;
				string format;
				playersGroup.GetCallsigns(company, platoon, squad, character, format);
				playersGroupName = squad;
			}
			
			if (playersGroup)
			{
				foreach (CVON_GroupFrequencyContainer container: faction.GetCallsignInfo().m_aGroupFrequencyOverrides)
				{
					foreach (string groupName: container.m_aGroupNames)
					{
						if (!SCR_Faction.NormalizeCallsign(playersGroupName).Contains(SCR_Faction.NormalizeCallsign(groupName)) && !SCR_Faction.NormalizeCallsign(groupName).Contains(SCR_Faction.NormalizeCallsign(playersGroupName)))
							continue;
						
						freqContainer = container;
						break;
					}
					if (freqContainer)
						break;
				}
				CVON_VONGameModeComponent gamemodeComp = CVON_VONGameModeComponent.GetInstance();
				if (!freqContainer)
					if (gamemodeComp.m_FreqConfig)
					{
						foreach (CVON_GroupFrequencyContainer freqItem: gamemodeComp.m_FreqConfig.m_aPresetGroupFrequencyContainers)
						{
							foreach (string groupName: freqItem.m_aGroupNames)
							{
								if (!SCR_Faction.NormalizeCallsign(playersGroupName).Contains(SCR_Faction.NormalizeCallsign(groupName)) && !SCR_Faction.NormalizeCallsign(groupName).Contains(SCR_Faction.NormalizeCallsign(playersGroupName)))
									continue;
								
								freqContainer = freqItem;
								break;
							}
							if (freqContainer)
								break;
						}
					}
			}
			
			switch (radioComp.m_eRadioType)
			{
				case CVON_ERadioType.SHORT:
				{
					if (!shortRangeRadios.Contains(radioObject))
					{
						shortRangeRadios.Insert(radioObject);
						if (System.IsConsoleApp())
							break;
						if (!freqContainer)
							break;
						if (m_RadioSettings.m_aSRRadioSettings)
						{
							if (m_RadioSettings.m_aSRRadioSettings.Count() - 1 < SRIndex)
							{
								ref CVON_RadioSettingObject settings = new CVON_RadioSettingObject();
								if (freqContainer.m_aSRFrequencies)
								{
									if (freqContainer.m_aSRFrequencies.Count() - 1 < FreqSRIndex)
										settings.m_sFreq = playersGroupName;
									else
									{
										settings.m_sFreq = freqContainer.m_aSRFrequencies.Get(FreqSRIndex);
										FreqSRIndex++;
									}
								}
								else
									settings.m_sFreq = playersGroupName;
								
								m_RadioSettings.m_aSRRadioSettings.Insert(settings);
								SRIndex++;
							}
							else
							{
								ref CVON_RadioSettingObject settings = m_RadioSettings.m_aSRRadioSettings.Get(SRIndex);
								if (freqContainer.m_aSRFrequencies)
								{
									if (freqContainer.m_aSRFrequencies.Count() - 1 < FreqSRIndex)
										settings.m_sFreq = playersGroupName;
									else
									{
										settings.m_sFreq = freqContainer.m_aSRFrequencies.Get(FreqSRIndex);
										FreqSRIndex++;
									}
								}
								else
									settings.m_sFreq = playersGroupName;
								radioComp.m_eStereo = settings.m_Stereo;
								radioComp.m_iVolume = settings.m_iVolume;
								SRIndex++;
							}
						}
					}
						
					break;
				}
				case CVON_ERadioType.LONG:
				{
					if (!longRangeRadios.Contains(radioObject))
					{
						longRangeRadios.Insert(radioObject);
						if (System.IsConsoleApp())
							break;
						if (!freqContainer)
							break;
						if (m_RadioSettings.m_aLRRadioSettings)
						{
							if (m_RadioSettings.m_aLRRadioSettings.Count() - 1 < LRIndex)
							{
								ref CVON_RadioSettingObject settings = new CVON_RadioSettingObject();
								if (freqContainer.m_aLRFrequencies)
								{
									if (freqContainer.m_aLRFrequencies.Count() - 1 < FreqLRIndex)
										settings.m_sFreq = factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()).Get(0);
									else
									{
										settings.m_sFreq = freqContainer.m_aLRFrequencies.Get(FreqLRIndex);
										FreqLRIndex++;
									}
								}
								else
									settings.m_sFreq = factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()).Get(0);
								m_RadioSettings.m_aLRRadioSettings.Insert(settings);
								LRIndex++;
							}
							else
							{
								ref CVON_RadioSettingObject settings = m_RadioSettings.m_aLRRadioSettings.Get(LRIndex);
								if (freqContainer.m_aLRFrequencies)
								{
									if (freqContainer.m_aLRFrequencies.Count() - 1 < FreqLRIndex)
										settings.m_sFreq = factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()).Get(0);
									else
									{
										settings.m_sFreq = freqContainer.m_aLRFrequencies.Get(FreqLRIndex);
										FreqLRIndex++;
									}
								}
								else
									settings.m_sFreq = factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()).Get(0);
								radioComp.m_eStereo = settings.m_Stereo;
								radioComp.m_iVolume = settings.m_iVolume;
								LRIndex++;
							}
						}
					}
						
					break;
				}
			}
		}
		if (shortRangeRadios)
			m_aRadios.InsertAll(shortRangeRadios);
		if (longRangeRadios)
			m_aRadios.InsertAll(longRangeRadios);
		IEntity radioEntity = RplComponent.Cast(Replication.FindItem(radios.Get(0))).GetEntity();
		CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radioEntity.FindComponent(CVON_RadioComponent));
		if (GetGame().GetPlayerController())
		{
			SCR_VONController vonController = SCR_VONController.Cast(GetGame().GetPlayerController().FindComponent(SCR_VONController));
			vonController.m_CharacterController = SCR_CharacterControllerComponent.Cast(to.FindComponent(SCR_CharacterControllerComponent));
			radioComp.WriteJSON(to);
		}
			
	}
}