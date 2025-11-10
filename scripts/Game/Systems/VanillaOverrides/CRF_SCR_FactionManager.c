//Used because I can't store the active channels for frequency automation in SCR_Faction as you can't have replicated items in there.
modded class SCR_FactionManager
{
	protected ref array<string> m_aBLUFORActiveSRChannels = {};
	protected ref array<string> m_aBLUFORActiveLRChannels = {};
	protected ref array<string> m_aOPFORActiveSRChannels  = {};
	protected ref array<string> m_aOPFORActiveLRChannels  = {};
	protected ref array<string> m_aINDFORActiveSRChannels = {};
	protected ref array<string> m_aINDFORActiveLRChannels = {};
	protected ref array<string> m_aCIVFORActiveSRChannels = {};
	protected ref array<string> m_aCIVFORActiveLRChannels = {};
	
	//------------------------------------------------------------------------------------------------
	// Getters (unchanged)
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
	
	//------------------------------------------------------------------------------------------------
	// Server: Update SR channels and replicate to clients
	void UpdateFactionActiveChannelSR(string factionId, array<string> input)
	{
		// Only server should modify state
		if (!Replication.IsServer())
			return;
		
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
		
		// Send only this faction's channels to all clients via broadcast manager
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.UpdateFactionChannelsSR(factionId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	// Server: Update LR channels and replicate to clients
	void UpdateFactionActiveChannelLR(string factionId, array<string> input)
	{
		// Only server should modify state
		if (!Replication.IsServer())
			return;
		
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
		
		// Send only this faction's channels to all clients via broadcast manager
		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.UpdateFactionChannelsLR(factionId, input);
	}
	
	//------------------------------------------------------------------------------------------------
	// Client-side: Update SR channels (called by RPC handler in broadcast manager)
	void UpdateChannelsSRClient(string factionId, array<string> channels)
	{
		// Update only the specified faction's channels
		switch (factionId)
		{
			case "BLUFOR":
				m_aBLUFORActiveSRChannels.Clear();
				m_aBLUFORActiveSRChannels.InsertAll(channels);
				break;
			case "OPFOR":
				m_aOPFORActiveSRChannels.Clear();
				m_aOPFORActiveSRChannels.InsertAll(channels);
				break;
			case "INDFOR":
				m_aINDFORActiveSRChannels.Clear();
				m_aINDFORActiveSRChannels.InsertAll(channels);
				break;
			case "CIV":
				m_aCIVFORActiveSRChannels.Clear();
				m_aCIVFORActiveSRChannels.InsertAll(channels);
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Client-side: Update LR channels (called by RPC handler in broadcast manager)
	void UpdateChannelsLRClient(string factionId, array<string> channels)
	{
		// Update only the specified faction's channels
		switch (factionId)
		{
			case "BLUFOR":
				m_aBLUFORActiveLRChannels.Clear();
				m_aBLUFORActiveLRChannels.InsertAll(channels);
				break;
			case "OPFOR":
				m_aOPFORActiveLRChannels.Clear();
				m_aOPFORActiveLRChannels.InsertAll(channels);
				break;
			case "INDFOR":
				m_aINDFORActiveLRChannels.Clear();
				m_aINDFORActiveLRChannels.InsertAll(channels);
				break;
			case "CIV":
				m_aCIVFORActiveLRChannels.Clear();
				m_aCIVFORActiveLRChannels.InsertAll(channels);
				break;
		}
	}
}