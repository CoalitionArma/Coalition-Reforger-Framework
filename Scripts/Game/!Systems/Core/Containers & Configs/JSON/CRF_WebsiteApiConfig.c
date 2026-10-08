//! Server-side credentials for writing to the coalitiongroup.net backend (in-game mission ratings).
//! Lives in the dedicated server's profile folder so the key is never shipped in the mod. The file is
//! created empty on first use; paste the backend's GAME_SERVER_API_KEY into "gameServerKey".
class CRF_WebsiteApiConfig
{
	protected static const string FILE_PATH = "$profile:CRF_WebsiteApiConfig.json";
	protected static ref CRF_WebsiteApiConfigStruct s_Config;

	//------------------------------------------------------------------------------------------------
	//! Empty when not configured - callers should skip the request
	static string GetGameServerKey()
	{
		if (!s_Config)
			Load();

		return s_Config.gameServerKey;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Load()
	{
		s_Config = new CRF_WebsiteApiConfigStruct();

		if (FileIO.FileExists(FILE_PATH))
		{
			JsonLoadContext loadContext = new JsonLoadContext();
			if (loadContext.LoadFromFile(FILE_PATH) && loadContext.ReadValue("", s_Config))
				return;

			Print("[CRF_WebsiteApiConfig] Could not read " + FILE_PATH, LogLevel.WARNING);
			s_Config = new CRF_WebsiteApiConfigStruct();
			return;
		}

		// Write the empty template so server admins know where the key goes
		JsonSaveContext saveContext = new JsonSaveContext();
		saveContext.WriteValue("", s_Config);
		saveContext.SaveToFile(FILE_PATH);
	}
}

class CRF_WebsiteApiConfigStruct
{
	string gameServerKey;
}
