#ifdef WORKBENCH
[WorkbenchPluginAttribute(
	name: "Generate Mission Configuration", 
	description: "Generate Mission Configuration File", 
	shortcut: "", 
	wbModules: { "WorldEditor" }, 
	category: "Coalition Reforger Framework",
	awesomeFontCode: 0x0033)
] 
class CRF_MissionConfigurationPlugin : WorkbenchPlugin
{	
	//------------------------------------------------------------------------------------
	[Attribute("<Author>", "auto", "", category: "CRF Mission Config - Mission Info")]
	string m_iMissionAuthor;
	
	[Attribute("<Name>", "auto", "", category: "CRF Mission Config - Mission Info")]
	string m_iMissionName;
	
	[Attribute("45", "auto", "Mission Time (set to -1 to disable)", category: "CRF Mission Config - Mission Info")]
	int m_iMissionTimeLimitInMinutes;
	
	[Attribute("<Playercount>", "auto", "", category: "CRF Mission Config - Mission Info")]
	int m_iMissionPlayercount;
	
	[Attribute("<Mode>", "auto", "", category: "CRF Mission Config - Mission Info")]
	string m_iMissionMode;
	
	[Attribute("<Description>", "auto", "", category: "CRF Mission Config - Mission Info")]
	string m_iMissionDescription;
	
	[Attribute("<Terrain Name>", "auto", "", category: "CRF Mission Config - Mission Info")]
	string m_iMissionTerrain;

	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Mission Config - General")]
	bool m_bAllowEspionage;
	
	[Attribute("true", "auto", "Disable chat messages except tickets & messages from admins/mods", category: "CRF Mission Config - General")]
	bool m_bDisableChat;
	
	[Attribute("0", "auto", "Should this mission go to AAR after)", category: "CRF Mission Config - General")]
	bool m_bUseAAR;

	[Attribute("false", "auto", "Enables AI autonomy while in GAME state", category: "CRF Mission Config - AI")]
	bool EnableAIInGameState;
	
	[Attribute("1", "auto", "Disables AI Crouching", category: "CRF Mission Config - AI")]
	bool m_bDisableAICrouching;

	[Attribute("true", "auto", "If safestart turns on instantly after the lobby screen.", category: "CRF Mission Config - Safestart")]
	bool m_bSafestartEnabled;
	
	[Attribute("true", "auto", "Should we lock all JIP slots after SafeStart turns off? COOP = FALSE", category: "CRF Mission Config - Safestart")]
	bool m_bLockSlotsAfterSafestart;

	protected static const string DESCRIPTION_INTRO =					"Welcome to step-by-step setup of a CRF mission.\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY";
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(
		"Mission Config Generator", 
		DESCRIPTION_INTRO, 
		this))
			return;

		//CRF_MissionSetupPluginValidation dialog = new CRF_MissionSetupPluginValidation();
		
		//if (!Workbench.ScriptDialog(
		//"Mission Config Generator", 
		//DESCRIPTION_INTRO, 
		//this))
		//	return;

	}
	
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Next", true)]
	protected bool ButtonNext()
	{
		return true;
	}
}
	/*
	protected static const string DESCRIPTION_INTRO =					"Welcome to step-by-step setup of a CRF mission.\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY\n TANKA GAY";
	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		if (!Workbench.ScriptDialog(CAPTION_INTRO, DESCRIPTION_INTRO, this))
			return;

		string error;
		if (!LoadConfig(error))
		{
			Print(CAPTION_INTRO + ": " + error, LogLevel.WARNING);
			if (Workbench.ScriptDialog(CAPTION_INTRO, error, new SCR_GameModeSetupPluginError()) != 0)
				Run(); // not using ShowPage to prevent filling the history

			return;
		}

		//--- Next
		ShowPage(CRF_EMissionSetupPage.VALIDATION);
	}

	//------------------------------------------------------------------------------------------------
	protected void RunValidation()
	{
		m_Config.Init();

		CRF_MissionSetupPluginValidation dialog = new CRF_MissionSetupPluginValidation();
		if (!Workbench.ScriptDialog(CAPTION_VALIDATION, DESCRIPTION_VALIDATION, dialog))
			return;

		m_bIsWorldValid = false;
		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				ShowPrevPage();
				break;

			case CRF_EMissionSetupButton.NEXT:
				m_sDialogMessageValidation = string.Empty;
				m_bCanAutogenerateWorld = true;
				m_bIsWorldValid = m_Config.ValidateWorld(m_sDialogMessageValidation, m_bCanAutogenerateWorld);
				ShowPage(CRF_EMissionSetupPage.VALIDATION_RESULTS);
				break;

			case CRF_EMissionSetupButton.SKIP:
				ShowPage(CRF_EMissionSetupPage.GENERATION);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RunValidationResults()
	{
		CRF_MissionSetupPluginResults dialog = new CRF_MissionSetupPluginResults();
		if (!Workbench.ScriptDialog(CAPTION_VALIDATION_RESULTS, DESCRIPTION_VALIDATION_RESULTS + m_sDialogMessageValidation, dialog))
			return;

		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				ShowPrevPage();
				break;

			case CRF_EMissionSetupButton.NEXT:
				if (m_bIsWorldValid)
				{
					if (m_bIsMissionHeaderValid)
						ShowPage(CRF_EMissionSetupPage.OUTRO);
					else
						ShowPage(CRF_EMissionSetupPage.MISSION_HEADER);
				}
				else if (!m_bCanAutogenerateWorld)
					ShowPage(CRF_EMissionSetupPage.OUTRO);
				else
					ShowPage(CRF_EMissionSetupPage.GENERATION);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RunGeneration()
	{
		CRF_MissionSetupPluginGeneration dialog = new CRF_MissionSetupPluginGeneration();
		if (!Workbench.ScriptDialog(CAPTION_GENERATION, DESCRIPTION_GENERATION, dialog))
			return;

		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				ShowPrevPage();
				break;

			case CRF_EMissionSetupButton.NEXT:
				m_sDialogMessageGeneration = string.Empty;
				m_bIsWorldValid = m_Config.GenerateWorld(m_sDialogMessageGeneration);
				ShowPage(CRF_EMissionSetupPage.GENERATION_RESULTS);
				break;

			case CRF_EMissionSetupButton.SKIP:
				ShowPage(CRF_EMissionSetupPage.MISSION_HEADER);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RunGenerationResults()
	{
		CRF_MissionSetupPluginGenerationResults dialog = new CRF_MissionSetupPluginGenerationResults();
		if (!Workbench.ScriptDialog(CAPTION_GENERATION_RESULTS, DESCRIPTION_GENERATION_RESULTS + m_sDialogMessageGeneration, dialog))
			return;

		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				ShowPrevPage();
				break;

			case CRF_EMissionSetupButton.NEXT:
				ShowPage(CRF_EMissionSetupPage.MISSION_HEADER);
				break;

			case CRF_EMissionSetupButton.VALIDATE:
				ShowPage(CRF_EMissionSetupPage.VALIDATION);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RunMissionHeader()
	{
		//--- Mission header already exists, skip this step
		m_bIsMissionHeaderValid = m_Config.ValidateMissionHeader(m_sDialogMessageMissionHeader);
		if (m_bIsMissionHeaderValid)
		{
			m_aPageHistory.Resize(m_aPageHistory.Count() - 1);
			ShowPage(CRF_EMissionSetupPage.MISSION_HEADER_RESULTS);
			return;
		}

		CRF_MissionSetupPluginMissionHeader dialog = new CRF_MissionSetupPluginMissionHeader();
		if (!Workbench.ScriptDialog(CAPTION_MISSION_HEADER, DESCRIPTION_MISSION_HEADER, dialog))
			return;

		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				ShowPrevPage();
				break;

			case CRF_EMissionSetupButton.NEXT:
				m_sDialogMessageMissionHeader = string.Empty;
				m_bIsMissionHeaderValid = m_Config.GenerateMissionHeader(m_sTemplate, m_sDialogMessageMissionHeader);
				ShowPage(CRF_EMissionSetupPage.MISSION_HEADER_RESULTS);
				break;

			case CRF_EMissionSetupButton.SKIP:
				ShowPage(CRF_EMissionSetupPage.OUTRO);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RunMissionHeaderResults()
	{
		CRF_MissionSetupPluginResults dialog = new CRF_MissionSetupPluginResults();
		if (!Workbench.ScriptDialog(CAPTION_MISSION_HEADER_RESULTS, DESCRIPTION_MISSION_HEADER_RESULTS + m_sDialogMessageMissionHeader, dialog))
			return;

		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				m_bIsMissionHeaderValid = false;
				ShowPrevPage();
				break;

			case CRF_EMissionSetupButton.NEXT:
				ShowPage(CRF_EMissionSetupPage.OUTRO);
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RunOutro()
	{
		CRF_MissionSetupPluginOutro dialog = new CRF_MissionSetupPluginOutro();
		if (m_bIsWorldValid && m_bIsMissionHeaderValid)
		{
			if (!Workbench.ScriptDialog(CAPTION_OUTRO_GOOD, DESCRIPTION_OUTRO_COMPLETE, dialog))
				return;
		}
		else
		{
			if (!Workbench.ScriptDialog(CAPTION_OUTRO_BAD, DESCRIPTION_OUTRO_INCOMPLETE, dialog))
				return;
		}

		switch (dialog.m_eResult)
		{
			case CRF_EMissionSetupButton.BACK:
				ShowPrevPage();
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowPage(CRF_EMissionSetupPage page)
	{
		if (!m_aPageHistory.Contains(page))
			m_aPageHistory.Insert(page);

		switch (page)
		{
			case CRF_EMissionSetupPage.INTRO:					Run(); break;
			case CRF_EMissionSetupPage.VALIDATION:				RunValidation(); break;
			case CRF_EMissionSetupPage.VALIDATION_RESULTS:		RunValidationResults(); break;
			case CRF_EMissionSetupPage.GENERATION:				RunGeneration(); break;
			case CRF_EMissionSetupPage.GENERATION_RESULTS:		RunGenerationResults(); break;
			case CRF_EMissionSetupPage.MISSION_HEADER:			RunMissionHeader(); break;
			case CRF_EMissionSetupPage.MISSION_HEADER_RESULTS:	RunMissionHeaderResults(); break;
			case CRF_EMissionSetupPage.OUTRO:					RunOutro(); break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowPrevPage()
	{
		int pageHistoryCountMinus1 = m_aPageHistory.Count() - 1;
		m_aPageHistory.Resize(pageHistoryCountMinus1);
		ShowPage(m_aPageHistory[pageHistoryCountMinus1 - 1]);
	}

	//------------------------------------------------------------------------------------------------
	protected bool LoadConfig(out string error)
	{
		string worldPath;
		SCR_WorldEditorToolHelper.GetWorldEditorAPI().GetWorldPath(worldPath);
		if (worldPath.IsEmpty())
		{
			error = "No world is currently loaded, or the current world is not saved.";
			return false;
		}

		if (m_sTemplate.IsEmpty())
		{
			error = "No template defined! Please fill the Template field.";
			return false;
		}

		Resource templateResource = Resource.Load(m_sTemplate);
		if (!templateResource.IsValid())
		{
			error = "Template config " + FilePath.StripPath(m_sTemplate) + " is invalid.";
			return false;
		}

		BaseContainer templateContainer = templateResource.GetResource().ToBaseContainer();
		m_Config = GameModeSetupConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(templateContainer));
		if (!m_Config)
		{
			error = "Failed to load the " + FilePath.StripPath(m_sTemplate) + " template config.";
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Next", true)]
	protected bool ButtonNext()
	{
		return true;
	}
}

class CRF_MissionSetupPluginValidation
{
	CRF_EMissionSetupButton m_eResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Skip")]
	protected bool ButtonSkip()
	{
		m_eResult = CRF_EMissionSetupButton.SKIP;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Back")]
	protected bool ButtonBack()
	{
		m_eResult = CRF_EMissionSetupButton.BACK;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Scan world", true)]
	protected bool ButtonValidate()
	{
		m_eResult = CRF_EMissionSetupButton.NEXT;
		return true;
	}
}

class CRF_MissionSetupPluginResults
{
	CRF_EMissionSetupButton m_eResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Back")]
	protected bool ButtonBack()
	{
		m_eResult = CRF_EMissionSetupButton.BACK;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Next", true)]
	protected bool ButtonNext()
	{
		m_eResult = CRF_EMissionSetupButton.NEXT;
		return true;
	}
}

class CRF_MissionSetupPluginGenerationResults
{
	CRF_EMissionSetupButton m_eResult;

	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	[ButtonAttribute("Re-scan")]
	protected bool ButtonValidate()
	{
		m_eResult = CRF_EMissionSetupButton.VALIDATE;
		return true;
	}

	[ButtonAttribute("Back")]
	protected bool ButtonBack()
	{
		m_eResult = CRF_EMissionSetupButton.BACK;
		return true;
	}

	[ButtonAttribute("Next", true)]
	protected bool ButtonNext()
	{
		m_eResult = CRF_EMissionSetupButton.NEXT;
		return true;
	}
}

class CRF_MissionSetupPluginGeneration
{
	CRF_EMissionSetupButton m_eResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Skip")]
	protected bool ButtonSkip()
	{
		m_eResult = CRF_EMissionSetupButton.SKIP;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Back")]
	protected bool ButtonBack()
	{
		m_eResult = CRF_EMissionSetupButton.BACK;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Create entities", true)]
	protected bool ButtonGenerate()
	{
		m_eResult = CRF_EMissionSetupButton.NEXT;
		return true;
	}
}

class CRF_MissionSetupPluginMissionHeader
{
	CRF_EMissionSetupButton m_eResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Skip")]
	protected bool ButtonSkip()
	{
		m_eResult = CRF_EMissionSetupButton.SKIP;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Back")]
	protected bool ButtonBack()
	{
		m_eResult = CRF_EMissionSetupButton.BACK;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Create header", true)]
	protected bool ButtonGenerate()
	{
		m_eResult = CRF_EMissionSetupButton.NEXT;
		return true;
	}
}

class CRF_MissionSetupPluginOutro
{
	CRF_EMissionSetupButton m_eResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Back")]
	protected bool ButtonBack()
	{
		m_eResult = CRF_EMissionSetupButton.BACK;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Close", true)]
	protected bool ButtonClose()
	{
		return false;
	}
}

class CRF_MissionSetupPluginError
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
	*/
[EnumLinear()]
enum CRF_EMissionSetupPage
{
	INTRO,
	VALIDATION,
	VALIDATION_RESULTS,
	GENERATION,
	GENERATION_RESULTS,
	MISSION_HEADER,
	MISSION_HEADER_RESULTS,
	OUTRO,
}

[EnumLinear()]
enum CRF_EMissionSetupButton
{
	CANCEL,
	BACK,
	NEXT,
	SKIP,
	VALIDATE,
}
#endif