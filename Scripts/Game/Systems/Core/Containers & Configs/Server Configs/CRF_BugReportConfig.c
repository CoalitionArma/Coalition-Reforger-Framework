class CRF_BugReportConfig
{
	static ref CRF_BugReportConfigStruct m_config;
	static string m_configFilePath = "$profile:CRF_BugReportConfig.json";

	static bool LoadConfig()
	{		
		SCR_JsonLoadContext configLoadContext = new SCR_JsonLoadContext();
		
		m_config = new CRF_BugReportConfigStruct();
				
		if (!FileIO.FileExists( m_configFilePath ))
			m_config.SetDefaultValue();
		else
		{
			if (!configLoadContext.LoadFromFile( m_configFilePath ))
				return false;
			
			if (!configLoadContext.ReadValue("", m_config))
				return false;
		};
		
		if (!m_config.m_mBugReports)
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
	
	static string GetToken()
	{
		if (!m_config)
			return "";
		
		return m_config.m_mBugReports.Get("token");
	};
	
	static string GetRepo()
	{
		if (!m_config)
			return "";
		
		return m_config.m_mBugReports.Get("repo");
	};
};

class CRF_BugReportConfigStruct
{
	ref map<string, string> m_mBugReports;
	
	void SetDefaultValue()
	{
		m_mBugReports = new map<string, string>();
		m_mBugReports.Insert("token", "TOKEN");
		m_mBugReports.Insert("repo", "repos/USERNAME/REPO/issues");
	};
};