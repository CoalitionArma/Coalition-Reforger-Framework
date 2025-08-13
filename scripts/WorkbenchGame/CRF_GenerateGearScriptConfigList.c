#ifdef WORKBENCH
[WorkbenchPluginAttribute(name: "Gear Script Config Generator", description: "Generates a list of all .conf gear configs recursively", shortcut: "", wbModules: { "ScriptEditor", "ResourceManager" })]
/* 
	This Workbench plugin automatically scans your "Configs/GearScripts/Standard/*" folder for all ".conf"
	gear configuration files and generates a JSON index mapping each gearset name to its config file path for use in the admin menu.  

	- Saved to Configs/GearScripts/GearScriptsConfigList.json.
	- Can be run manually from Workbench plugins or auto-updates when new configs are created.
	- Needs manually running when removing old configs
*/
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
		if (!absFileName.Contains("Configs") && !absFileName.Contains("GearScripts") && !absFileName.Contains("Standard"))
			return;
		
		GenerateIndex();
	}
}

class ConfigStruct
{
	ref map<string, string> gearset;
}
#endif
