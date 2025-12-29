#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "Generate Mission Configuration", 
	description: "Generate Mission Configuration File", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0xF6E8)
] 
class CRF_MissionConfigurationPlugin : WorkbenchPlugin
{
	[Attribute("{6389BA4D41B187DC}Configs/Workbench/GameModeSetup/GameMaster.conf", desc: "Game mode configuration rules", params: "conf class=GameModeSetupConfig", category: "Game Mode Template")]
	protected ResourceName m_sTemplate;

	//[Attribute(category: "Game Mode Template")] //--- Used only for debugging, game crashes when recompiling Workbench scripts and running the plugin repeatedly.
	protected ref GameModeSetupConfig m_Config;

	protected bool m_bIsWorldValid;
	protected bool m_bIsWorldAttentionNeeded;
	protected bool m_bCanAutogenerateWorld;
	protected bool m_bIsMissionHeaderValid;
	protected ref array<SCR_EGameModeSetupPage> m_aPageHistory = { SCR_EGameModeSetupPage.INTRO };

	protected string m_sDialogMessageValidation;
	protected string m_sDialogMessageGeneration;
	protected string m_sDialogMessageMissionHeader;

	protected static const string CAPTION_INTRO =						"Game Mode Setup";
	protected static const string CAPTION_VALIDATION =					"Game Mode Setup - World Scan";
	protected static const string CAPTION_VALIDATION_RESULTS =			"Game Mode Setup - World Scan Results";
	protected static const string CAPTION_GENERATION =					"Game Mode Setup - World Configuration";
	protected static const string CAPTION_GENERATION_RESULTS =			"Game Mode Setup - World Configuration Completed";
	protected static const string CAPTION_MISSION_HEADER =				"Game Mode Setup - Mission Header";
	protected static const string CAPTION_MISSION_HEADER_RESULTS =		"Game Mode Setup - Mission Header Created";
	protected static const string CAPTION_OUTRO_GOOD =					"Game Mode Setup - Success";
	protected static const string CAPTION_OUTRO_BAD =					"Game Mode Setup - Actions Required";

	protected static const string DESCRIPTION_INTRO =					"Welcome to step-by-step setup of a game mode.\nA game mode is a set of rules that define how the world will function.\nWithout it, players won't be able to try your world in game.\nEven opening the world in Game Master requires you to create a Game Master game mode for it.\n\nThe plugin will explain what's needed to get a game mode up an running,\nand offer automatic creation of required configuration.\n\nBefore we start, please select a template of the game mode you wish to set up.\n";
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(CAPTION_INTRO, DESCRIPTION_INTRO, this))
			return;

		//ShowPage(SCR_EGameModeSetupPage.VALIDATION);
	}
}
	
class CRF_GameModeSetupPluginError
{
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Back", true)]
	protected int ButtonBack()
	{
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Close")]
	protected bool ButtonClose()
	{
		return 0;
	}
}