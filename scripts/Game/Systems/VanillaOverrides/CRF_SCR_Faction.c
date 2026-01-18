modded class SCR_Faction
{
	ref array<string> m_aActiveSRChannels = {};
	ref array<string> m_aActiveLRChannels = {};
	
	array<ref SCR_EntityCatalog> GetEntityCatalogs()
	{
		return m_aEntityCatalogs;
	}
	
	static string NormalizeCallsign(string callsign)
	{
		ref array<string> callsignSplit = {};
		callsign.Split("(", callsignSplit, true);
		string newCallsign = callsignSplit.Get(0);
		newCallsign.ToLower();
		
		newCallsign.Replace("-", "_");
		newCallsign.Replace(" ", "_");
	
		// trim accidental underscores
		while (newCallsign.Contains("__"))
			newCallsign.Replace("__", "_");
	
		if (newCallsign.EndsWith("_"))
			newCallsign = newCallsign.Substring(0, newCallsign.Length() - 1);
	
		return newCallsign;
	}
		
	//Auto loads all the frequencies this factions needs to automate handing them out.
	//Compares to the freqconfig, if no SR is defined it uses the group name, if no LR is defined it defaults to the first one and barks an error.
	override void Init(IEntity owner)
	{
		super.Init(owner);
		if (!CVON_VONGameModeComponent.GetInstance())
			return;
		GetGame().GetCallqueue().CallLater(InitializeFactionChannels, 2000 , false);
	}
	
	void InitializeFactionChannels()
	{
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (GetFactionKey() == "CIV")
			return;
		ref array<ref SCR_CallsignInfo> squadCallsigns = {};
		SCR_FactionCallsignInfo callsignInfo = GetCallsignInfo();
		if (!callsignInfo)
			return;
		callsignInfo.GetSquadArray(squadCallsigns);
		CVON_VONGameModeComponent gamemodeComp = CVON_VONGameModeComponent.GetInstance();
		if (!gamemodeComp.m_FreqConfig)
			return;
		
		foreach (SCR_CallsignInfo group: squadCallsigns)
		{
			string groupName = group.GetCallsign();
			bool foundContainer = false;
			if (GetCallsignInfo().m_aGroupFrequencyOverrides)
			foreach (CVON_GroupFrequencyContainer container: GetCallsignInfo().m_aGroupFrequencyOverrides)
			{
				bool foundObject = false;
				foreach (string name: container.m_aGroupNames)
				{
					foundObject = CheckContainer(container, name, groupName);
					if (foundObject)
						break;
				}
				if (foundObject)
				{
					foundContainer = true;
					break;
				}
			}
			if (!foundContainer)
				foreach (CVON_GroupFrequencyContainer container: gamemodeComp.m_FreqConfig.m_aPresetGroupFrequencyContainers)
				{
					bool foundObject = false;
					foreach (string name: container.m_aGroupNames)
					{
						foundObject = CheckContainer(container, name, groupName);
						if (foundObject)
								break;
					}
					if (foundObject)
					{
						foundContainer = true;
						break;
					}
				}
			
			if (!foundContainer)
			{
				Print("[CVON-WARNING] " + groupName + " MISSING FREQUENCY CONFIGURATION!", LogLevel.WARNING);
				m_aActiveSRChannels.Insert(groupName);
			}
		}
		if (m_aActiveLRChannels.Count() == 0)
			m_aActiveLRChannels.Insert(GetFactionKey() + "LR");
		
		SCR_FactionManager.Cast(GetGame().GetFactionManager()).UpdateFactionActiveChannelSR(GetFactionKey(), m_aActiveSRChannels);
		SCR_FactionManager.Cast(GetGame().GetFactionManager()).UpdateFactionActiveChannelLR(GetFactionKey(), m_aActiveLRChannels);
	}
	
	void GenerateSR(string freq)
	{
		m_aActiveSRChannels.Insert(freq);
	}
	
	bool CheckContainer(CVON_GroupFrequencyContainer container, string name, string groupName)
	{
		if (!NormalizeCallsign(groupName).Contains(NormalizeCallsign(name)) && !NormalizeCallsign(name).Contains(NormalizeCallsign(groupName)))
			return false;
	
		if (!container.m_aSRFrequencies)
			GenerateSR(groupName);
		else if (container.m_aSRFrequencies.Count() == 0)
			GenerateSR(groupName);
		else
		{
			foreach (string freq: container.m_aSRFrequencies)
			{
				if (m_aActiveSRChannels.Contains(freq))
					continue;
				
				m_aActiveSRChannels.Insert(freq);
			}
		}
		if (!container.m_aLRFrequencies)
			return true;
		
		if (container.m_aLRFrequencies.Count() == 0)
			return true;
		
		foreach (string freq: container.m_aLRFrequencies)
		{
			if (m_aActiveLRChannels.Contains(freq))
				continue;
			
			m_aActiveLRChannels.Insert(freq);
		}
		
		return true;
	}
}