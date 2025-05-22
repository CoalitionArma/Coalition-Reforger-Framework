class CRF_ModeratorConfig
{
	static ref CRF_ModeratorConfigStruct m_config;
	static string m_configFilePath = "$profile:CRF_ModeratorConfig.json";

	static bool LoadConfig()
	{		
		SCR_JsonLoadContext configLoadContext = new SCR_JsonLoadContext();
		
		m_config = new CRF_ModeratorConfigStruct();
				
		if (!FileIO.FileExists( m_configFilePath ))
			m_config.SetDefaultModeratorValue();
		else
		{
			if (!configLoadContext.LoadFromFile( m_configFilePath ))
				return false;
			
			if (!configLoadContext.ReadValue("", m_config))
				return false;
		};
		
		if (!m_config.m_mModerators)
		{
		   m_config.SetDefaultModeratorValue();
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
	
	static bool IsModerator(string identityId)
	{
		if (!m_config)
			return false;
		
		string match = m_config.m_mModerators.Get(identityId);
		if (!match.IsEmpty())
			return true;
		
		return false;
	};
};

class CRF_ModeratorConfigStruct
{
	ref map<string, string> m_mModerators;
	
	void SetDefaultModeratorValue()
	{
		m_mModerators = new map<string, string>();
		m_mModerators.Insert("00000000-0000-0000-0000-000000000001", "Moderator Example");
	};
};