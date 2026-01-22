class CRF_DonatorConfig
{
	static ref CRF_DonatorConfigStruct m_config;
	static string m_configFilePath = "$profile:CRF_DonatorConfig.json";

	static bool LoadConfig()
	{		
		SCR_JsonLoadContext configLoadContext = new SCR_JsonLoadContext();
		
		m_config = new CRF_DonatorConfigStruct();
				
		if (!FileIO.FileExists( m_configFilePath ))
			m_config.SetDefaultValue();
		else
		{
			if (!configLoadContext.LoadFromFile( m_configFilePath ))
				return false;
			
			if (!configLoadContext.ReadValue("", m_config))
				return false;
		};
		
		if (!m_config.m_mDonators)
		{
		   m_config.SetDefaultValue();
		}
		
		SaveConfig();
		
		return true;
	};

	static bool SaveConfig()
	{
		SCR_JsonSaveContext configSaveContext = new SCR_JsonSaveContext();
		configSaveContext.WriteValue("", m_config);
		
		if (!configSaveContext.SaveToFile( m_configFilePath ))
			return false;
		
		return true;
	};
	
	static bool IsDonator(string identityId)
	{
		if (!m_config)
			return false;
		
		string match = m_config.m_mDonators.Get(identityId);
		if (!match.IsEmpty())
			return true;
		
		return false;
	};
};

class CRF_DonatorConfigStruct
{
	ref map<string, string> m_mDonators;
	
	void SetDefaultValue()
	{
		m_mDonators = new map<string, string>();
		m_mDonators.Insert("00000000-0000-0000-0000-000000000001", "Donator Example");
	};
};