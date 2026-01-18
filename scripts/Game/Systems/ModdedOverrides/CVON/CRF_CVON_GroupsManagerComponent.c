modded class SCR_GroupsManagerComponent
{
	//Needed so we wait for the group to initialize
	//==========================================================================================================================================================================
	override void TuneFreqDelayWithPresets(int playerId, IEntity player)
	{
		if (!player)
			return;
		
		if (!AnyPlayerFrequencies(playerId))
		{
			TuneFreqWithoutPresets(playerId, player);
			return;
		}
			
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
		
		int SRIndex = 0;
		int LRIndex = 0;
		array<IEntity> radiosDone = {};
		if (playerController.m_aRadioSettings.Count() > 0)
		{
			foreach (IEntity radio: playerController.m_aRadios)
			{
				CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
				int index = playerController.m_aRadios.Find(radio);
				if (playerController.m_aRadioSettings.Count() <= index)
					break;
				CVON_RadioSettingObject radioSetting = playerController.m_aRadioSettings.Get(index);
				if (radioComp.m_aChannels.Contains(radioSetting.m_sFreq))
				{
					radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(radioSetting.m_sFreq) + 1);
					radioComp.UpdateFrequncyServer(radioSetting.m_sFreq, -1, false);
					radiosDone.Insert(radio);
					if (radioComp.m_eRadioType == 0)
						SRIndex++;
					else
						LRIndex++;
				}			
			}
		}
		
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_Faction playerFaction = SCR_Faction.Cast(factionMan.GetPlayerFaction(playerId));
		
		array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(playerFaction);
		SCR_AIGroup playersGroup = GetPlayerGroup(playerId);
		int index = groups.Find(playersGroup);
		if (index == -1)
			return;
	
		//Gonna get the groups callsign so we know which frequency object is theres
		string company;
		string platoon;
		string squad;
		string character;
		string format;
		playersGroup.GetCallsigns(company, platoon, squad, character, format);
		string playersGroupName = squad;
		CVON_GroupFrequencyContainer freqContainer;
		CVON_VONGameModeComponent gamemodeComp = CVON_VONGameModeComponent.GetInstance();
		if (playerFaction.GetCallsignInfo())
			foreach (CVON_GroupFrequencyContainer container: playerFaction.GetCallsignInfo().m_aGroupFrequencyOverrides)
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
		if (!freqContainer)
		{
			freqContainer = new CVON_GroupFrequencyContainer;
			freqContainer.m_aSRFrequencies = {};
			freqContainer.m_aSRFrequencies.Insert(playersGroupName);
			freqContainer.m_aLRFrequencies = {};
			
			// Safety check: Ensure LR channels exist before accessing
			array<string> lrChannels = factionMan.GetFactionActiveChannelLR(playerFaction.GetFactionKey());
			if (lrChannels && lrChannels.Count() > 0)
				freqContainer.m_aLRFrequencies.Insert(lrChannels.Get(0));
			else
				freqContainer.m_aLRFrequencies.Insert("55500"); // Fallback frequency
		}
		for (int i = 0; i < playerController.m_aRadios.Count(); i++)
		{
			if (radiosDone.Contains(playerController.m_aRadios.Get(i)))
				continue;
			bool m_bFrequencyFound = false;
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(playerController.m_aRadios.Get(i).FindComponent(CVON_RadioComponent));
			switch (radioComp.m_eRadioType)
			{
				case CVON_ERadioType.SHORT: 
				{
					if (!freqContainer.m_aSRFrequencies)
					{
						string freq = playersGroupName;
						SRIndex++;
						radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
						radioComp.UpdateFrequncyServer(freq);
						m_bFrequencyFound = true;
						break;
					}
					if (freqContainer.m_aSRFrequencies.Count() < SRIndex + 1)
					{
						string freq = playersGroupName;
						SRIndex++;
						radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
						radioComp.UpdateFrequncyServer(freq);
						m_bFrequencyFound = true;
						break;
					}
					string freq = freqContainer.m_aSRFrequencies.Get(SRIndex);
					if (!radioComp.m_aChannels.Contains(freq))
						break;
					SRIndex++;
					radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
					radioComp.UpdateFrequncyServer(freq);
					m_bFrequencyFound = true;
					break;
				}
				case CVON_ERadioType.LONG: 
				{
					if (!freqContainer.m_aLRFrequencies)
						break;
					if (freqContainer.m_aLRFrequencies.Count() < LRIndex + 1)
					{
						// Safety check: Ensure LR channels exist before accessing
						array<string> lrChannels = factionMan.GetFactionActiveChannelLR(playerFaction.GetFactionKey());
						string freq;
						if (lrChannels && lrChannels.Count() > 0)
							freq = lrChannels.Get(0);
						else
							freq = "55500"; // Fallback frequency
						
						LRIndex++;
						radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
						radioComp.UpdateFrequncyServer(freq);
						m_bFrequencyFound = true;
						break;
					}
					string freq = freqContainer.m_aLRFrequencies.Get(LRIndex);
					if (!radioComp.m_aChannels.Contains(freq))
						break;
					LRIndex++;
					radioComp.UpdateChannelServer(radioComp.m_aChannels.Find(freq) + 1);
					radioComp.UpdateFrequncyServer(freq);
					m_bFrequencyFound = true;
					break;
				}
			}
			if (m_bFrequencyFound)
				continue;
			
			radioComp.UpdateChannelServer(1);
			if (radioComp.m_aChannels.Count() > 0)
				radioComp.UpdateFrequncyServer(radioComp.m_aChannels.Get(0));
			else
				radioComp.UpdateFrequncyServer("55500");
		}
	}
	
}