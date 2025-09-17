//Used because I can't store the active channels for frequency automation in SCR_Faction as you can't have replicated items in there.
modded class SCR_FactionManager
{
	[RplProp()] ref array<string> m_aBLUFORActiveSRChannels = {};
	[RplProp()] ref array<string> m_aBLUFORActiveLRChannels = {};
	[RplProp()] ref array<string> m_aOPFORActiveSRChannels  = {};
	[RplProp()] ref array<string> m_aOPFORActiveLRChannels  = {};
	[RplProp()] ref array<string> m_aINDFORActiveSRChannels = {};
	[RplProp()] ref array<string> m_aINDFORActiveLRChannels = {};
	[RplProp()] ref array<string> m_aCIVFORActiveSRChannels = {};
	[RplProp()] ref array<string> m_aCIVFORActiveLRChannels = {};
	
	array<string> GetFactionActiveChannelSR(string factionId)
	{
		switch(factionId)
		{
			case "BLUFOR": 	{return m_aBLUFORActiveSRChannels;}
			case "OPFOR": 	{return m_aOPFORActiveSRChannels;}
			case "INDFOR": 	{return m_aINDFORActiveSRChannels;}
			case "CIV": 	{return m_aCIVFORActiveSRChannels;}
		}
		
		return m_aBLUFORActiveSRChannels;
	}
	
	array<string> GetFactionActiveChannelLR(string factionId)
	{
		switch (factionId)
		{
			case "BLUFOR": 	{return m_aBLUFORActiveLRChannels;}
			case "OPFOR": 	{return m_aOPFORActiveLRChannels;}
			case "INDFOR": 	{return m_aINDFORActiveLRChannels;}
			case "CIV": 	{return m_aCIVFORActiveLRChannels;}
		}
		
		return m_aBLUFORActiveLRChannels;
	}
	
	void UpdateFactionActiveChannelSR(string factionId, array<string> input)
	{
		switch (factionId)
		{
			case "BLUFOR" :
			{
				m_aBLUFORActiveSRChannels.Clear();
				m_aBLUFORActiveSRChannels.InsertAll(input);
				break;
			}
			case "OPFOR" :
			{
				m_aOPFORActiveSRChannels.Clear();
				m_aOPFORActiveSRChannels.InsertAll(input);
				break;
			}
			case "INDFOR" :
			{
				m_aINDFORActiveSRChannels.Clear();
				m_aINDFORActiveSRChannels.InsertAll(input);
				break;
			}
			case "CIV" :
			{
				m_aCIVFORActiveSRChannels.Clear();
				m_aCIVFORActiveSRChannels.InsertAll(input);
				break;
			}
		}
		Replication.BumpMe();
	}
	
	void UpdateFactionActiveChannelLR(string factionId, array<string> input)
	{
		switch (factionId)
		{
			case "BLUFOR" :
			{
				m_aBLUFORActiveLRChannels.Clear();
				m_aBLUFORActiveLRChannels.InsertAll(input);
				break;
			}
			case "OPFOR" :
			{
				m_aOPFORActiveLRChannels.Clear();
				m_aOPFORActiveLRChannels.InsertAll(input);
				break;
			}
			case "INDFOR" :
			{
				m_aINDFORActiveLRChannels.Clear();
				m_aINDFORActiveLRChannels.InsertAll(input);
				break;
			}
			case "CIV" :
			{
				m_aCIVFORActiveLRChannels.Clear();
				m_aCIVFORActiveLRChannels.InsertAll(input);
				break;
			}
		}
		Replication.BumpMe();
	}
}