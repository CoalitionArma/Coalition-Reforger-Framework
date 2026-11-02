//! Mission info card on the right of the pause menu: mission name and summary, objectives,
//! time left, admins online and a button to the server rules. Created and driven by CRF's COA_PauseMenuUI.
//! Gamemodes add objective lines through CRF_MissionInfo (see CRF_MissionInfo.c). In the CRF, not the Lobby: other communities use the Lobby and have their own rules.
class CRF_MissionInfoPanel
{
	static const ResourceName LAYOUT = "{2E3A7C25A396AB62}UI/layouts/Menus/PauseMenu/CRF_MissionInfoPanel.layout";

	protected static const float REFRESH_SECONDS = 1;
	protected static const int MAX_DESCRIPTION_CHARS = 280;
	protected static const int MAX_OBJECTIVE_CHARS = 420;
	protected static const int MAX_OBJECTIVE_LINES = 7;
	protected static const int MAX_EXTRA_LINES = 6;
	protected static const int MAX_ADMIN_NAMES = 10;

	protected Widget m_wRoot;
	protected RichTextWidget m_wMissionName;
	protected TextWidget m_wMissionMeta;
	protected RichTextWidget m_wMissionDescription;
	protected RichTextWidget m_wObjectivesExtra;
	protected RichTextWidget m_wObjectivesText;
	protected Widget m_wTimeDivider;
	protected Widget m_wTimeHeader;
	protected TextWidget m_wTimeHeaderText;
	protected TextWidget m_wTimeValue;
	protected TextWidget m_wTimeDetail;
	protected TextWidget m_wAdminCount;
	protected RichTextWidget m_wAdminList;
	protected SCR_ButtonTextComponent m_FullRulesButton;

	protected float m_fRefreshTimer;
	protected string m_sLastExtraText;
	protected ref array<string> m_aExtraLines = {};

	// Colours are linear (Enfusion UI); built from the sRGB design values once per open
	protected ref Color m_ValueColor;
	protected ref Color m_AmberColor;
	protected ref Color m_RedColor;
	protected ref Color m_GreenColor;

	//------------------------------------------------------------------------------------------------
	//! Builds the card under the pause menu's root. Returns false if the layout could not be created.
	bool Init(notnull Widget parent)
	{
		m_wRoot = GetGame().GetWorkspace().CreateWidgets(LAYOUT, parent);
		if (!m_wRoot)
		{
			Print("CRF_MissionInfoPanel: could not create " + LAYOUT, LogLevel.WARNING);
			return false;
		}

		m_ValueColor = Color.FromSRGBA(239, 242, 247, 255);
		m_AmberColor = Color.FromSRGBA(232, 170, 72, 255);
		m_RedColor = Color.FromSRGBA(232, 96, 96, 255);
		m_GreenColor = Color.FromSRGBA(110, 200, 130, 255);

		m_wMissionName = RichTextWidget.Cast(m_wRoot.FindAnyWidget("MissionName"));
		m_wMissionMeta = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionMeta"));
		m_wMissionDescription = RichTextWidget.Cast(m_wRoot.FindAnyWidget("MissionDescription"));
		m_wObjectivesExtra = RichTextWidget.Cast(m_wRoot.FindAnyWidget("ObjectivesExtra"));
		m_wObjectivesText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("ObjectivesText"));
		m_wTimeDivider = m_wRoot.FindAnyWidget("TimeDivider");
		m_wTimeHeader = m_wRoot.FindAnyWidget("TimeHeader");
		m_wTimeHeaderText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimeHeaderText"));
		m_wTimeValue = TextWidget.Cast(m_wRoot.FindAnyWidget("TimeValue"));
		m_wTimeDetail = TextWidget.Cast(m_wRoot.FindAnyWidget("TimeDetail"));
		m_wAdminCount = TextWidget.Cast(m_wRoot.FindAnyWidget("AdminCount"));
		m_wAdminList = RichTextWidget.Cast(m_wRoot.FindAnyWidget("AdminList"));

		m_FullRulesButton = SCR_ButtonTextComponent.GetButtonText("FullRulesButton", m_wRoot);
		if (m_FullRulesButton)
			m_FullRulesButton.m_OnClicked.Insert(OpenFullRules);

		UpdateMissionHeader();
		UpdateObjectives();
		RefreshLive();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Call every frame from the menu; refreshes the live parts about once a second
	void Update(float tDelta)
	{
		m_fRefreshTimer += tDelta;
		if (m_fRefreshTimer < REFRESH_SECONDS)
			return;

		m_fRefreshTimer = 0;
		RefreshLive();
	}

	//------------------------------------------------------------------------------------------------
	//! Call from the menu's OnMenuClose. The widgets go with the menu's root.
	void Clear()
	{
		if (m_FullRulesButton)
			m_FullRulesButton.m_OnClicked.Remove(OpenFullRules);

		m_FullRulesButton = null;
		m_wRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshLive()
	{
		if (!m_wRoot)
			return;

		UpdateExtraLines();
		UpdateTime();
		UpdateAdmins();
	}

	//------------------------------------------------------------------------------------------------
	//! Opens the full rules page in the platform browser (Steam overlay / system browser), like vanilla news links
	protected void OpenFullRules()
	{
		PlatformService platformService = GetGame().GetPlatformService();
		if (!platformService)
			return;

		platformService.OpenBrowser(CRF_MissionInfo.RULES_URL);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISSION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void UpdateMissionHeader()
	{
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());

		string missionName = GetGame().GetMissionName();
		if (missionName.IsEmpty() && header)
			missionName = header.m_sName;
		if (missionName.IsEmpty())
			missionName = "Unknown Mission";

		if (m_wMissionName)
			m_wMissionName.SetText(StripMarkup(missionName));

		// "by Author · Gamemode"
		if (m_wMissionMeta)
		{
			string meta;
			if (header && !header.m_sAuthor.IsEmpty())
				meta = "by " + header.m_sAuthor;

			if (header && !header.m_sGameMode.IsEmpty())
			{
				if (!meta.IsEmpty())
					meta += "  ·  ";
				meta += header.m_sGameMode;
			}

			m_wMissionMeta.SetText(meta);
			m_wMissionMeta.SetVisible(!meta.IsEmpty());
		}

		// Summary: the everyone-visible "Situation" briefing entry, else the mission header description
		string description;
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode && gamemode.m_aMissionDescriptors)
		{
			foreach (COA_MissionDescriptor descriptor : gamemode.m_aMissionDescriptors)
			{
				if (descriptor && descriptor.m_bShowForAnyFaction && TitleContains(descriptor.m_sTitle, "situation"))
				{
					description = FirstParagraph(descriptor.m_sTextData);
					break;
				}
			}
		}

		if (description.IsEmpty() && header)
			description = header.m_sDescription;

		description = Shorten(StripMarkup(description), MAX_DESCRIPTION_CHARS);

		if (m_wMissionDescription)
		{
			m_wMissionDescription.SetText(description);
			m_wMissionDescription.SetVisible(!description.IsEmpty());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Briefing text for the local player's side: an "Objectives" entry, else the MISSION part of
	//! their side's briefing. Uses the same visibility rules as the briefing list.
	protected void UpdateObjectives()
	{
		if (!m_wObjectivesText)
			return;

		array<COA_MissionDescriptor> visible = {};
		GetVisibleDescriptors(visible);

		string objectives;
		foreach (COA_MissionDescriptor descriptor : visible)
		{
			if (TitleContains(descriptor.m_sTitle, "objective"))
			{
				objectives = descriptor.m_sTextData;
				break;
			}
		}

		if (objectives.IsEmpty())
		{
			foreach (COA_MissionDescriptor descriptor : visible)
			{
				if (!descriptor.m_bShowForAnyFaction && TitleContains(descriptor.m_sTitle, "briefing"))
				{
					objectives = ExtractMissionSection(descriptor.m_sTextData);
					break;
				}
			}
		}

		objectives = StripMarkup(objectives);
		objectives = LimitLines(objectives, MAX_OBJECTIVE_LINES);
		objectives = Shorten(objectives, MAX_OBJECTIVE_CHARS);

		if (objectives.IsEmpty())
			objectives = "No objectives in the briefing for your side. Open the map for the full briefing.";

		m_wObjectivesText.SetText(objectives);
	}

	//------------------------------------------------------------------------------------------------
	//! Live lines from gamemode components (CRF_MissionInfo hook)
	protected void UpdateExtraLines()
	{
		if (!m_wObjectivesExtra)
			return;

		CRF_MissionInfo.CollectLines(m_aExtraLines);

		array<string> shown = {};
		foreach (string line : m_aExtraLines)
		{
			string clean = StripMarkup(line).Trim();
			if (clean.IsEmpty())
				continue;

			shown.Insert(clean);
			if (shown.Count() >= MAX_EXTRA_LINES)
				break;
		}

		string text = SCR_StringHelper.Join("\n", shown);
		if (text == m_sLastExtraText && m_wObjectivesExtra.IsVisible() == !text.IsEmpty())
			return;

		m_sLastExtraText = text;
		m_wObjectivesExtra.SetText(text);
		m_wObjectivesExtra.SetVisible(!text.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected void GetVisibleDescriptors(notnull array<COA_MissionDescriptor> outDescriptors)
	{
		outDescriptors.Clear();

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || !gamemode.m_aMissionDescriptors)
			return;

		string playerFaction;
		PlayerController playerController = GetGame().GetPlayerController();
		if (playerController)
		{
			SCR_PlayerFactionAffiliationComponent factionComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
			if (factionComponent)
				playerFaction = factionComponent.GetAffiliatedFactionKey();
		}

		foreach (COA_MissionDescriptor descriptor : gamemode.m_aMissionDescriptors)
		{
			if (!descriptor)
				continue;

			if (descriptor.m_bShowForAnyFaction)
			{
				outDescriptors.Insert(descriptor);
				continue;
			}

			if (!playerFaction.IsEmpty() && descriptor.m_aFactionKeys && descriptor.m_aFactionKeys.Contains(playerFaction))
				outDescriptors.Insert(descriptor);
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TIME
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Mirrors the HUD mission timer (COA_GameTimerDisplay): the replicated COA_GameTimerManager string
	protected void UpdateTime()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GameTimerManager timerManager = COA_GameTimerManager.GetInstance();

		bool show = gamemode != null && timerManager != null;
		if (m_wTimeDivider)
			m_wTimeDivider.SetVisible(show);
		if (m_wTimeHeader)
			m_wTimeHeader.SetVisible(show);
		if (m_wTimeDetail)
			m_wTimeDetail.SetVisible(show);

		if (!show || !m_wTimeValue || !m_wTimeHeaderText || !m_wTimeDetail)
			return;

		int limitMinutes = gamemode.m_iTimeLimitMinutes;
		string limitText = "No time limit";
		if (limitMinutes > 0)
			limitText = string.Format("%1 min time limit", limitMinutes);

		string header = "TIME LEFT";
		string value;
		string detail;
		Color valueColor = m_ValueColor;

		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		bool safestart = safestartManager != null && safestartManager.GetSafestartStatus();

		if (gamemode.m_GamemodeState < COA_EGamemodeState.GAME)
		{
			header = "MISSION TIME";
			value = "NOT STARTED";
			detail = limitText + ", counted from the end of safe start";
		}
		else if (gamemode.m_GamemodeState == COA_EGamemodeState.AAR)
		{
			header = "MISSION TIME";
			value = "ENDED";
			detail = "After-action review";
		}
		else if (safestart)
		{
			header = "SAFE START";
			value = ShortTime(timerManager.GetServerWorldTime());
			detail = "Elapsed. " + limitText + ", counted from the end of safe start";
		}
		else
		{
			string serverTime = timerManager.GetServerWorldTime();
			if (limitMinutes <= 0 || serverTime == "N/A")
			{
				header = "MISSION TIME";
				value = "UNLIMITED";
				detail = "This mission has no time limit";
			}
			else if (serverTime == "Mission Time Expired!")
			{
				value = "EXPIRED";
				detail = limitText;
				valueColor = m_RedColor;
			}
			else if (serverTime.IsEmpty())
			{
				value = "--:--";
				detail = limitText;
			}
			else
			{
				value = ShortTime(serverTime);
				detail = limitText;

				// Same thresholds as the HUD timer: amber under 15 minutes, red under 5
				int secondsLeft = ParseSeconds(serverTime);
				if (secondsLeft >= 0 && secondsLeft < 300)
					valueColor = m_RedColor;
				else if (secondsLeft >= 0 && secondsLeft < 900)
					valueColor = m_AmberColor;
			}
		}

		m_wTimeHeaderText.SetText(header);
		m_wTimeValue.SetText(value);
		m_wTimeValue.SetColor(valueColor);
		m_wTimeDetail.SetText(detail);
	}

	//------------------------------------------------------------------------------------------------
	//! "00:12:34" -> "12:34"; drops a stray sign from the safe start elapsed time
	protected static string ShortTime(string time)
	{
		string clean = time;
		clean.Replace("-", "");

		if (clean.Length() == 8 && clean.StartsWith("00:"))
			return clean.Substring(3, 5);

		return clean;
	}

	//------------------------------------------------------------------------------------------------
	//! "hh:mm:ss" -> seconds, or -1
	protected static int ParseSeconds(string time)
	{
		array<string> parts = {};
		time.Split(":", parts, false);
		if (parts.Count() != 3)
			return -1;

		return parts[0].ToInt() * 3600 + parts[1].ToInt() * 60 + parts[2].ToInt();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ADMINS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Connected admins (server-replicated list in COA_PermissionManager), then moderators tagged "(mod)"
	protected void UpdateAdmins()
	{
		if (!m_wAdminCount || !m_wAdminList)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		COA_PermissionManager permissionManager = COA_PermissionManager.GetInstance();

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		array<string> admins = {};
		array<string> moderators = {};
		foreach (int playerId : playerIds)
		{
			bool isAdmin = false;
			bool isModerator = false;
			if (permissionManager)
			{
				isAdmin = permissionManager.IsAdminOnline(playerId);
				isModerator = permissionManager.IsModerator(playerId);
			}
			else
			{
				isAdmin = SCR_Global.IsAdmin(playerId);
			}

			if (!isAdmin && !isModerator)
				continue;

			string name = playerManager.GetPlayerName(playerId);
			name.Replace("<", "");
			name.Replace(">", "");
			if (name.IsEmpty())
				name = "Player " + playerId;

			if (isAdmin)
				admins.Insert(name);
			else
				moderators.Insert(name + " (mod)");
		}

		int staffCount = admins.Count() + moderators.Count();
		if (staffCount == 0)
		{
			m_wAdminCount.SetText("NONE");
			m_wAdminCount.SetColor(m_AmberColor);
			m_wAdminList.SetText("No admins online right now. You can still use /a in chat, or open a ticket on our Discord.");
			return;
		}

		m_wAdminCount.SetText(string.Format("%1 ONLINE", staffCount));
		m_wAdminCount.SetColor(m_GreenColor);

		admins.Sort();
		moderators.Sort();

		array<string> names = {};
		foreach (string adminName : admins)
			names.Insert(adminName);
		foreach (string moderatorName : moderators)
			names.Insert(moderatorName);

		int hidden = names.Count() - MAX_ADMIN_NAMES;
		if (hidden > 0)
			names.Resize(MAX_ADMIN_NAMES);

		string text = SCR_StringHelper.Join(", ", names);
		if (hidden > 0)
			text += string.Format(" +%1 more", hidden);

		m_wAdminList.SetText(text);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 TEXT HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected static bool TitleContains(string title, string lowerNeedle)
	{
		string lower = title;
		lower.ToLower();
		return lower.Contains(lowerNeedle);
	}

	//------------------------------------------------------------------------------------------------
	//! Removes rich text tags so cut-off text can't leave a tag open
	protected static string StripMarkup(string text)
	{
		string result = text;
		int open = result.IndexOf("<");
		while (open >= 0)
		{
			int close = result.IndexOfFrom(open, ">");
			if (close < 0)
			{
				result = result.Substring(0, open);
				break;
			}

			result = result.Substring(0, open) + result.Substring(close + 1, result.Length() - close - 1);
			open = result.IndexOf("<");
		}

		return result.Trim();
	}

	//------------------------------------------------------------------------------------------------
	//! Lines up to the first blank line
	protected static string FirstParagraph(string text)
	{
		array<string> lines = {};
		text.Split("\n", lines, false);

		array<string> paragraph = {};
		foreach (string line : lines)
		{
			string trimmed = line.Trim();
			if (trimmed.IsEmpty())
			{
				if (!paragraph.IsEmpty())
					break;

				continue;
			}

			paragraph.Insert(trimmed);
		}

		return SCR_StringHelper.Join("\n", paragraph);
	}

	//------------------------------------------------------------------------------------------------
	//! The paragraph under a "MISSION" heading (the CRF briefing template's MISSION / INTENT /
	//! EXECUTION layout), else the first paragraph
	protected static string ExtractMissionSection(string text)
	{
		array<string> lines = {};
		text.Split("\n", lines, false);

		int start = -1;
		foreach (int i, string line : lines)
		{
			string heading = line.Trim();
			heading.Replace(":", "");
			heading.ToUpper();
			if (heading == "MISSION")
			{
				start = i + 1;
				break;
			}
		}

		if (start < 0)
			return FirstParagraph(text);

		array<string> section = {};
		for (int lineIndex = start; lineIndex < lines.Count(); lineIndex++)
		{
			string trimmed = lines[lineIndex].Trim();
			if (trimmed.IsEmpty())
			{
				if (!section.IsEmpty())
					break;

				continue;
			}

			// Next all-caps heading (INTENT, EXECUTION...) ends the section
			if (IsHeading(trimmed))
				break;

			section.Insert(trimmed);
		}

		return SCR_StringHelper.Join("\n", section);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsHeading(string line)
	{
		if (line.Length() > 24)
			return false;

		string upper = line;
		upper.ToUpper();
		string lower = line;
		lower.ToLower();
		return upper == line && lower != line;
	}

	//------------------------------------------------------------------------------------------------
	protected static string LimitLines(string text, int maxLines)
	{
		array<string> lines = {};
		text.Split("\n", lines, false);
		if (lines.Count() <= maxLines)
			return text;

		lines.Resize(maxLines);
		return SCR_StringHelper.Join("\n", lines) + "...";
	}

	//------------------------------------------------------------------------------------------------
	//! Cuts at a word boundary and adds "..."
	protected static string Shorten(string text, int maxChars)
	{
		if (text.Length() <= maxChars)
			return text;

		string cut = text.Substring(0, maxChars);
		int lastSpace = cut.LastIndexOf(" ");
		if (lastSpace > maxChars / 2)
			cut = cut.Substring(0, lastSpace);

		return cut.Trim() + "...";
	}
}
