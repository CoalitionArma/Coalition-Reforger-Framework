modded class CVON_RadioComponent
{
	//Handles assigning 
	//==========================================================================================================================================================================
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		
		//Have this ifdef here cause in the workshop its a listen server, I explain the code in the non workshop version.
		#ifdef WORKBENCH
		if (m_sFactionKey != "")
			return;
	
		if (!GetOwner().GetRootParent())
			return;

		if (!SCR_ChimeraCharacter.Cast(GetOwner().GetRootParent()))
			return;
		
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(GetOwner().GetRootParent().FindComponent(FactionAffiliationComponent));
		if (!factionComp)
			return;
		m_sFactionKey = factionComp.GetAffiliatedFactionKey();
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		SCR_Faction faction = SCR_Faction.Cast(factionMan.GetFactionByKey(m_sFactionKey));
		m_aChannels.InsertAll(factionMan.GetFactionActiveChannelSR(faction.GetFactionKey()));
		m_aChannels.InsertAll(factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()));
		if (m_aChannels.Count() == 0)
		{
			m_aChannels.Insert("55500");
			m_iCurrentChannel = 1;
			m_sFrequency = m_aChannels.Get(0);
		}
		Replication.BumpMe();
		
		if (m_iTempChannel != m_iCurrentChannel || m_sTempFrequency != m_sFrequency || m_iTempTimeDeviation != m_iTimeDeviation || m_sTempFactionKey != m_sFactionKey)
		{
			m_iTempChannel = m_iCurrentChannel;
			m_sTempFrequency = m_sFrequency;
			m_iTempTimeDeviation = m_iTimeDeviation;
			m_sTempFactionKey = m_sFactionKey;
			WriteJSON(SCR_PlayerController.GetLocalControlledEntity());
		}
		#else
		if (System.IsConsoleApp())
		{
			//Faction found no longer needed.
			if (m_sFactionKey != "")
				return;
		
			if (!GetOwner().GetRootParent())
				return;
	
			//Radio is in the inventory of a player.
			if (!SCR_ChimeraCharacter.Cast(GetOwner().GetRootParent()))
				return;
			
			FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(GetOwner().GetRootParent().FindComponent(FactionAffiliationComponent));
			if (!factionComp)
				return;
			m_sFactionKey = factionComp.GetAffiliatedFactionKey();
			//Add that faction to this radio and get the faction to prep to load the factions frequencies
			SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			SCR_Faction faction = SCR_Faction.Cast(factionMan.GetFactionByKey(m_sFactionKey));
			m_aChannels.InsertAll(factionMan.GetFactionActiveChannelSR(faction.GetFactionKey()));
			m_aChannels.InsertAll(factionMan.GetFactionActiveChannelLR(faction.GetFactionKey()));
			//If there are no frequencies added, just add a default channel.
			if (m_aChannels.Count() == 0)
			{
				m_aChannels.Insert("55500");
				m_iCurrentChannel = 1;
				m_sFrequency = m_aChannels.Get(0);
			}
			//hehe
			Replication.BumpMe();
		}
		else
		{
			//Woah the client, just checking if anythings changed, this is mostly redundant but neccessary mostly for unit creation and onccupation.
			if (m_iTempChannel != m_iCurrentChannel || m_sTempFrequency != m_sFrequency || m_iTempTimeDeviation != m_iTimeDeviation || m_sTempFactionKey != m_sFactionKey)
			{
				m_iTempChannel = m_iCurrentChannel;
				m_sTempFrequency = m_sFrequency;
				m_iTempTimeDeviation = m_iTimeDeviation;
				m_sTempFactionKey = m_sFactionKey;
				WriteJSON(SCR_PlayerController.GetLocalControlledEntity());
			}
		}
		#endif
	}
}