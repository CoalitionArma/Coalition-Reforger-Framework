#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Auto Generate Gear Index", description: "Generates a list of all .conf gear configs recursively", shortcut: "", wbModules: { "ScriptEditor", "ResourceManager" })]
class AutoGenerateGearIndexPlugin : ResourceManagerPlugin
{
	static ref ConfigStruct m_config;

	override void Run()
	{
		GenerateIndex();
	}

	// Search for config files
	void GenerateIndex()
	{
		m_config = new ConfigStruct();
		m_config.gearset = new map<string, string>();
		array<string> allConfigs = {};

		FileIO.FindFiles(allConfigs.Insert, "Configs/GearScripts/Standard/", ".conf");
		
		PrintFormat("Found %1 config files", allConfigs.Count());

		foreach (string config : allConfigs)
		{
			string key = config.Substring(config.LastIndexOf("/") + 1, config.LastIndexOf(".") - config.LastIndexOf("/") - 1);
			key.Replace("CRF_GS_", "");
			
			m_config.gearset.Set(key, config);
		}
		
		SaveConfig();
	}

	// Write config list to json file
	static void SaveConfig()
	{
		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		ctx.WriteValue("", m_config);
		ctx.SaveToFile("configs/GearScripts/GearScriptsConfigList.json");

		Print("Saved to Configs/GearScripts/GearScriptsConfigList.json");
	}
	
	// This method is executed every time some new resource is registered
	override void OnRegisterResource(string absFileName, BaseContainer metaFile)
	{
		if (!absFileName.Contains("CRF_GS_"))
			return;
		
		GenerateIndex();
	}
}

class ConfigStruct
{
	ref map<string, string> gearset;
}
#endif
