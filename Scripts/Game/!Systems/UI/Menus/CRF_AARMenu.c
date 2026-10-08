/**
 * After Action Review screen. Opened by the cinematic outro (CRF_COA_Outro) once it finishes, and
 * fades in from black. Shows:
 *  - the full-screen map with every map marker of every faction (CRF_AARManager sends them)
 *  - voice channels, one per group (CRF_AARManager), which players can click to switch between
 *  - the result, the mission/author, and the coalitiongroup.net AAR page for this round
 *  - the local player's personal stats panel (CRF_AARStatsHUD)
 *  - the per-faction roster (alive/total), mission description, chat, time/weather/player count
 */
class COA_AARMenu: ChimeraMenuBase
{
	//----------------------------------------
	// Constants
	//----------------------------------------
	protected static const ResourceName STATS_LAYOUT = "{7CDA3F81B4920E56}UI/layouts/HUD/Intro/CRF_AARStats.layout";
	protected static const ResourceName CHANNEL_PLAYER_LAYOUT = "{68D74FF57296AFFB}UI/Listbox/PlayerListboxElementVON.layout";
	protected static const string WEBSITE_AAR_URL = "coalitiongroup.net/aar";

	// Seconds the screen takes to fade in from the outro's black background
	protected static const float FADE_IN_TIME = 3.0;
	// Seconds between refreshes of the time/weather/player count header
	protected static const float INFO_REFRESH_INTERVAL = 1.0;

	// Faction sidebar slide positions (LeftFaction PositionX in AAR.layout)
	protected static const float SIDEBAR_CLOSED_X = -671;
	protected static const float SIDEBAR_OPEN_X = -14;

	// Stats drawer slide positions (StatsHolder PositionY in AAR.layout, measured up from the bottom
	// edge). Closed leaves just the panel's "MISSION STATISTICS" header showing as a tab.
	protected static const float STATS_CLOSED_Y = -52;
	protected static const float STATS_OPEN_Y = -400;

	// Ease-out rate for the sidebar's per-frame slide - higher is snappier; 12 settles in about a
	// quarter of a second
	protected static const float DRAWER_SLIDE_SPEED = 12;

	// AnimateWidget speeds are progress per second (1 / duration)
	protected static const float DRAWER_ANIMATION_SPEED = 4;	// stats drawer: 0.25 s
	protected static const float REVEAL_SPEED = 2.2;			// entrance per panel: ~0.45 s
	protected static const float REVEAL_SLIDE = 40;				// entrance slide distance (layout units)
	protected static const float STATS_HIDDEN_Y = 20;			// stats tab starts just below the screen

	// WCAG relative luminance of the header background (AAR.layout HeaderBG 0.044 0.048 0.062), and
	// the WCAG AA minimum contrast for text - see ReadableOnHeader
	protected static const float HEADER_LUMINANCE = 0.0039;
	protected static const float MIN_TEXT_CONTRAST = 4.5;

	//----------------------------------------
	// UI Widget References
	//----------------------------------------
	protected Widget m_wRoot;
	protected Widget m_wLeftFaction;
	protected Widget m_wSidebarHitArea; // "FactionSelector": covers the sidebar's visible content
	protected bool m_bSidebarOpen;
	protected Widget m_wStatsHolder;
	protected bool m_bStatsOpen;
	protected Widget m_wFadeOverlay;
	protected Widget m_wStatsRoot;
	protected TextWidget m_wResultText;
	protected TextWidget m_wLinkText;
	protected TextWidget m_wTimeText;
	protected TextWidget m_wPlayersText;
	protected ButtonWidget m_wBackButton;

	//----------------------------------------
	// Core Components
	//----------------------------------------
	protected SCR_ChatPanel m_ChatPanel;
	protected SCR_MapEntity m_MapEntity;
	protected COA_Gamemode m_Gamemode;
	protected COA_MenuManager m_MenuManager;
	protected COA_ListboxComponent m_cChannelListBoxComponent;
	protected COA_ListboxComponent m_cSlotListBoxComponent;
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent;
	protected ref CRF_AARStatsHUD m_StatsHUD;

	//----------------------------------------
	// State
	//----------------------------------------
	protected Faction m_fSelectedFaction;
	protected ref array<ref COA_MissionDescriptor> m_aActiveDescriptors = {};
	protected ref array<Widget> m_aAnimatedWidgets = {}; // see TrackAnimation
	protected float m_fInfoRefreshTimer;
	protected int m_iShownChannelChanges = -1;

	// Set when the round leaves the AAR, so the menu doesn't reopen itself (see OnMenuClose)
	protected bool m_bAllowClose;

	// Total and alive (not dead) slotted players per faction
	protected int m_iBluforSlots;
	protected int m_iOpforSlots;
	protected int m_iIndforSlots;
	protected int m_iCivSlots;
	protected int m_iAliveBluforSlots;
	protected int m_iAliveOpforSlots;
	protected int m_iAliveIndforSlots;
	protected int m_iAliveCivSlots;

	//----------------------------------------
	// Menu Lifecycle Methods
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnMenuInit()
	{
		super.OnMenuInit();

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Exit if this is a dedicated server (menu is client-side only)
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			m_bAllowClose = true;
			Close();
			return;
		}

		m_wRoot = GetRootWidget();
		m_Gamemode = COA_Gamemode.GetInstance();
		m_MenuManager = COA_MenuManager.GetInstance();

		// The outro muted voice - the AAR is where everyone talks again. The game world stays muted
		// (sound effects) for as long as the AAR is up, see OnMenuUpdate/OnMenuClose.
		if (m_Gamemode)
			m_Gamemode.m_bIsInEndCredits = false;
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);

		InitializeUIComponents();
		SetupInputHandlers();
		SetupMissionInfo();
		SetupResultAndLink();
		SetupFactionFlags();
		SetupFactionColors();
		SetupFactionButtons();
		SetupHoverEffects();
		CreateStatsPanel();
		PlayEntrance();

		if (m_MapEntity)
			GetGame().GetCallqueue().Call(OpenMap);

		// Faction roster
		SelectInitialFaction();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			slottingManager.GetOnSlottingUpdate().Insert(UpdateSlots);
			slottingManager.GetOnSlotChanged().Insert(UpdateSlots);
		}

		// Voice channels
		if (m_MenuManager)
			m_MenuManager.GetOnPlayerChannelChanged().Insert(OnPlayerChannelChanged);
		RebuildChannelList();

		// Mission description list
		m_wBackButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("BackButton"));
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(m_wRoot.FindAnyWidget("DescriptionList").FindHandler(SCR_ListBoxComponent));
		DescriptionInit();

		UpdateInfoDisplay();

		// Ask the server for every faction's map markers and the website link
		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestAARData();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			slottingManager.GetOnSlottingUpdate().Remove(UpdateSlots);
			slottingManager.GetOnSlotChanged().Remove(UpdateSlots);
		}

		if (m_MenuManager)
			m_MenuManager.GetOnPlayerChannelChanged().Remove(OnPlayerChannelChanged);

		CRF_AARSessionStats.s_OnMissionIdReceived.Remove(UpdateLinkText);

		// Don't leave entrance/drawer animations running against widgets that are going away
		foreach (Widget animatedWidget : m_aAnimatedWidgets)
		{
			if (animatedWidget)
				AnimateWidget.StopAnimation(animatedWidget, WidgetAnimationBase);
		}
		m_aAnimatedWidgets.Clear();

		// Give the game world its sound back (if the AAR reopens itself it mutes it again)
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 100);

		if (m_StatsHUD)
		{
			m_StatsHUD.Cleanup();
			m_StatsHUD = null;
		}

		if (!CVON_VONGameModeComponent.GetInstance())
		{
			GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);

		// Like the outro, the AAR can't be dismissed while the round is still in it - otherwise a
		// stray menu close would leave the player looking at the world with nothing to do
		if (!m_bAllowClose)
			GetGame().GetCallqueue().Call(ReopenIfStillInAAR);
	}

	//------------------------------------------------------------------------------------------------
	static void ReopenIfStillInAAR()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode || gamemode.m_GamemodeState != COA_EGamemodeState.AAR)
			return;

		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager || menuManager.FindMenuByPreset(ChimeraMenuPreset.COA_AARMenu))
			return;

		menuManager.OpenMenu(ChimeraMenuPreset.COA_AARMenu);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_Gamemode && m_Gamemode.m_GamemodeState != COA_EGamemodeState.AAR)
			m_bAllowClose = true;

		// Keep the game world (ambient sound, gunfire, vehicles) muted - re-applied every frame, as
		// the outro does, so nothing that resets the volume brings it back
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);

		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");


		// Cheap int compare; the list is only rebuilt when the channels actually change
		if (m_MenuManager && m_MenuManager.m_iChannelChanges != m_iShownChannelChanges)
			RebuildChannelList();

		m_fInfoRefreshTimer += tDelta;
		if (m_fInfoRefreshTimer >= INFO_REFRESH_INTERVAL)
		{
			m_fInfoRefreshTimer = 0;
			UpdateInfoDisplay();
		}

		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);

		AnimateSidebar(tDelta);
		AnimateStatsDrawer();
	}

	//----------------------------------------
	// Setup
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void InitializeUIComponents()
	{
		m_wLeftFaction = m_wRoot.FindAnyWidget("LeftFaction");
		m_wSidebarHitArea = m_wRoot.FindAnyWidget("FactionSelector");
		m_wFadeOverlay = m_wRoot.FindAnyWidget("FadeOverlay");
		m_wResultText = TextWidget.Cast(m_wRoot.FindAnyWidget("ResultText"));
		m_wLinkText = TextWidget.Cast(m_wRoot.FindAnyWidget("LinkText"));
		m_wTimeText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText"));
		m_wPlayersText = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayersText"));

		Widget chatPanelWidget = m_wRoot.FindAnyWidget("ChatPanel");
		if (chatPanelWidget)
			m_ChatPanel = SCR_ChatPanel.Cast(chatPanelWidget.FindHandler(SCR_ChatPanel));

		m_cChannelListBoxComponent = COA_ListboxComponent.Cast(m_wRoot.FindAnyWidget("PlayerList").FindHandler(COA_ListboxComponent));
		m_cSlotListBoxComponent = COA_ListboxComponent.Cast(m_wRoot.FindAnyWidget("RoleList").FindHandler(COA_ListboxComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetupInputHandlers()
	{
		// CVON handles push-to-talk itself; this is the vanilla VON fallback
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetupMissionInfo()
	{
		TextWidget missionText = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionText"));
		if (missionText)
		{
			string missionName = "Unknown Mission";
			if (GetGame().GetMissionName())
				missionName = GetGame().GetMissionName();

			string author = "Unknown";
			SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
			if (header && header.m_sAuthor)
				author = header.m_sAuthor;

			missionText.SetText(missionName + " | By " + author);
		}

		TextWidget weatherText = TextWidget.Cast(m_wRoot.FindAnyWidget("WeatherText"));
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (weatherText && world && world.GetTimeAndWeatherManager())
			weatherText.SetText("Weather: " + world.GetTimeAndWeatherManager().GetCurrentWeatherState().GetStateName());
	}

	//------------------------------------------------------------------------------------------------
	//! Result banner (winner, coloured by faction) and the coalitiongroup.net AAR page link
	protected void SetupResultAndLink()
	{
		if (m_wResultText)
		{
			string winningFaction;
			COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
			if (broadcastManager)
				winningFaction = broadcastManager.m_sOutroWinningFaction;

			Faction faction;
			if (winningFaction != "" && GetGame().GetFactionManager())
				faction = GetGame().GetFactionManager().GetFactionByKey(winningFaction);

			if (faction)
			{
				m_wResultText.SetText(GetOutcomeText(winningFaction));
				m_wResultText.SetColor(ReadableOnHeader(faction.GetFactionColor()));
			}
			else
			{
				m_wResultText.SetText("MISSION COMPLETE");
			}
		}

		// The mission ID can arrive after the menu opens (the server looks it up when the AAR starts)
		CRF_AARSessionStats.s_OnMissionIdReceived.Insert(UpdateLinkText);
		UpdateLinkText();
	}

	//------------------------------------------------------------------------------------------------
	//! Faction colours come from mission config and can be too dark to read on the header. Lightens
	//! the colour towards white just enough to reach WCAG AA contrast (4.5:1) against the header
	//! background (AAR.layout HeaderBG, 0.044 0.048 0.062); readable colours are returned unchanged.
	protected Color ReadableOnHeader(Color color)
	{
		Color readable = new Color(color.R(), color.G(), color.B(), 1);
		for (int step = 0; step < 12; step++)
		{
			if ((RelativeLuminance(readable) + 0.05) / (HEADER_LUMINANCE + 0.05) >= MIN_TEXT_CONTRAST)
				break;

			readable.Lerp(Color.White, 0.15);
		}

		return readable;
	}

	//------------------------------------------------------------------------------------------------
	//! WCAG relative luminance of an sRGB colour
	protected static float RelativeLuminance(Color color)
	{
		return 0.2126 * LinearChannel(color.R()) + 0.7152 * LinearChannel(color.G()) + 0.0722 * LinearChannel(color.B());
	}

	//------------------------------------------------------------------------------------------------
	protected static float LinearChannel(float value)
	{
		if (value <= 0.04045)
			return value / 12.92;

		return Math.Pow((value + 0.055) / 1.055, 2.4);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetOutcomeText(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return "BLUFOR VICTORY";
			case "OPFOR": return "OPFOR VICTORY";
			case "INDFOR": return "INDFOR VICTORY";
			case "CIV": return "CIVILIAN VICTORY";
		}
		return "MISSION COMPLETE";
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateLinkText()
	{
		if (!m_wLinkText)
			return;

		if (CRF_AARSessionStats.s_iMissionId > 0)
			m_wLinkText.SetText(string.Format("Full AAR: %1/%2", WEBSITE_AAR_URL, CRF_AARSessionStats.s_iMissionId));
		else
			m_wLinkText.SetText("Past missions: " + WEBSITE_AAR_URL);
	}

	//------------------------------------------------------------------------------------------------
	//! The personal stats panel that used to appear on the outro
	protected void CreateStatsPanel()
	{
		Widget statsHolder = m_wRoot.FindAnyWidget("StatsHolder");
		if (!statsHolder)
		{
			Print("[COA_AARMenu] StatsHolder widget missing from the AAR layout - no stats panel", LogLevel.WARNING);
			return;
		}

		// Starts closed, as a tab on the bottom edge - see AnimateStatsDrawer
		m_wStatsHolder = statsHolder;
		m_bStatsOpen = false;
		FrameSlot.SetPosY(m_wStatsHolder, STATS_CLOSED_Y);

		m_wStatsRoot = GetGame().GetWorkspace().CreateWidgets(STATS_LAYOUT, statsHolder);
		if (!m_wStatsRoot)
		{
			Print("[COA_AARMenu] Could not create the stats panel layout " + STATS_LAYOUT, LogLevel.WARNING);
			return;
		}

		// The stats layout's root frame has no slot of its own, so stretch it over the holder -
		// otherwise it can end up zero-sized and draw nothing
		FrameSlot.SetAnchorMin(m_wStatsRoot, 0, 0);
		FrameSlot.SetAnchorMax(m_wStatsRoot, 1, 1);
		FrameSlot.SetOffsets(m_wStatsRoot, 0, 0, 0, 0);
		m_wStatsRoot.SetVisible(true);
		m_wStatsRoot.SetOpacity(1);

		// The panel background is off by default (it sat on the outro's black screen); turn it on so
		// the text is readable over the map
		Widget panelBackground = m_wStatsRoot.FindAnyWidget("PanelBG");
		if (panelBackground)
			panelBackground.SetOpacity(1);

		m_StatsHUD = new CRF_AARStatsHUD(m_wStatsRoot);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetupFactionFlags()
	{
		SetFactionFlag("FlagBlufor", "BLUFOR");
		SetFactionFlag("FlagOpfor", "OPFOR");
		SetFactionFlag("FlagIndfor", "INDFOR");
		SetFactionFlag("FlagCiv", "CIV");
	}

	//------------------------------------------------------------------------------------------------
	protected void SetFactionFlag(string widgetName, string factionKey)
	{
		ImageWidget flagWidget = ImageWidget.Cast(m_wRoot.FindAnyWidget(widgetName));
		if (!flagWidget || !GetGame().GetFactionManager())
			return;

		// A mission doesn't have to define all four factions
		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
		if (!faction)
			return;

		flagWidget.LoadImageTexture(1, faction.GetFactionFlag());
		flagWidget.SetImage(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetupFactionColors()
	{
		m_wRoot.FindAnyWidget("BluforBGSelect").SetColor(Color.FromRGBA(34, 196, 244, 33));
		m_wRoot.FindAnyWidget("OpforBGSelect").SetColor(Color.FromRGBA(238, 49, 47, 33));
		m_wRoot.FindAnyWidget("IndforBGSelect").SetColor(Color.FromRGBA(0, 177, 79, 33));
		m_wRoot.FindAnyWidget("CivBGSelect").SetColor(Color.FromRGBA(168, 110, 207, 33));
	}

	//------------------------------------------------------------------------------------------------
	//! The old AAR screen never wired these up, so its faction tabs did nothing when clicked
	protected void SetupFactionButtons()
	{
		SCR_ButtonTextComponent button = GetFactionButton("ButtonBlufor");
		if (button)
			button.m_OnClicked.Insert(SelectFactionBlufor);

		button = GetFactionButton("ButtonOpfor");
		if (button)
			button.m_OnClicked.Insert(SelectFactionOpfor);

		button = GetFactionButton("ButtonIndfor");
		if (button)
			button.m_OnClicked.Insert(SelectFactionIndfor);

		button = GetFactionButton("ButtonCiv");
		if (button)
			button.m_OnClicked.Insert(SelectFactionCiv);
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_ButtonTextComponent GetFactionButton(string widgetName)
	{
		Widget buttonWidget = m_wRoot.FindAnyWidget(widgetName);
		if (!buttonWidget)
			return null;

		return SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectInitialFaction()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		if (slottingManager.IsFactionValid("BLUFOR"))
			SelectFactionBlufor();
		else if (slottingManager.IsFactionValid("OPFOR"))
			SelectFactionOpfor();
		else if (slottingManager.IsFactionValid("INDFOR"))
			SelectFactionIndfor();
		else if (slottingManager.IsFactionValid("CIV"))
			SelectFactionCiv();
	}

	//----------------------------------------
	// Fade-in
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Fade up from the outro's black, with the panels appearing one after another as it lifts rather
	//! than all at once. Uses vanilla AnimateWidget, so every step is eased and runs off the menu's
	//! own update.
	protected void PlayEntrance()
	{
		if (m_wFadeOverlay)
		{
			m_wFadeOverlay.SetVisible(true);
			m_wFadeOverlay.SetOpacity(1);

			WidgetAnimationOpacity fade = AnimateWidget.Opacity(m_wFadeOverlay, 0, 1 / FADE_IN_TIME, true);
			if (fade)
				TrackAnimation(fade, m_wFadeOverlay, 0, EAnimationCurve.EASE_IN_OUT_SINE);
			else
				m_wFadeOverlay.SetVisible(false);
		}

		// Delays are seconds after the menu opens. Only widgets anchored to a point on both axes
		// slide - the others stretch on one axis, which a position animation would disturb.
		RevealWidget("Header", 0.8, 0, 0);
		RevealWidget("TimeWeather", 1.1, REVEAL_SLIDE, 0);
		RevealWidget("PlayerListTextBG", 1.3, 0, 0);
		RevealWidget("PlayerListText", 1.3, 0, 0);
		RevealWidget("PlayerList", 1.4, 0, 0);
		RevealWidget("LeftFaction", 1.6, 0, 0);
		if (m_wStatsHolder)
			RevealWidget("StatsHolder", 1.8, 0, STATS_HIDDEN_Y - STATS_CLOSED_Y);
	}

	//------------------------------------------------------------------------------------------------
	//! Fade a widget in after a delay, optionally sliding it into place from an offset
	protected void RevealWidget(string widgetName, float delay, float slideX, float slideY)
	{
		Widget widget = m_wRoot.FindAnyWidget(widgetName);
		if (!widget)
			return;

		widget.SetOpacity(0);
		WidgetAnimationOpacity fade = AnimateWidget.Opacity(widget, 1, REVEAL_SPEED);
		if (!fade)
		{
			// Animation system unavailable - just show it
			widget.SetOpacity(1);
			return;
		}

		TrackAnimation(fade, widget, delay, EAnimationCurve.EASE_OUT_CUBIC);

		if (slideX == 0 && slideY == 0)
			return;

		float restingPosition[2];
		restingPosition[0] = FrameSlot.GetPosX(widget);
		restingPosition[1] = FrameSlot.GetPosY(widget);

		FrameSlot.SetPos(widget, restingPosition[0] + slideX, restingPosition[1] + slideY);
		WidgetAnimationPosition slide = AnimateWidget.Position(widget, restingPosition, REVEAL_SPEED);
		if (slide)
			TrackAnimation(slide, widget, delay, EAnimationCurve.EASE_OUT_CUBIC);
		else
			FrameSlot.SetPos(widget, restingPosition[0], restingPosition[1]);
	}

	//------------------------------------------------------------------------------------------------
	//! Apply curve/delay, and remember the widget so its animations are stopped when the menu closes
	protected void TrackAnimation(WidgetAnimationBase animation, Widget widget, float delay, EAnimationCurve curve)
	{
		animation.SetCurve(curve);
		if (delay > 0)
			animation.SetDelay(delay);

		if (!m_aAnimatedWidgets.Contains(widget))
			m_aAnimatedWidgets.Insert(widget);
	}

	//------------------------------------------------------------------------------------------------
	//! Hover, pressed and focus feedback on everything clickable (see COA_UIHoverEffect). The channel
	//! headers get theirs as the list is built, in AddChannelRows.
	protected void SetupHoverEffects()
	{
		// Faction tabs: the tile's background lights up. Attached explicitly - their buttons carry a
		// ButtonComponent, so the generic pass below would skip them.
		array<string> factionButtons = {"ButtonBlufor", "ButtonOpfor", "ButtonIndfor", "ButtonCiv"};
		foreach (string buttonName : factionButtons)
		{
			Widget button = m_wRoot.FindAnyWidget(buttonName);
			if (button && button.GetParent())
				COA_UIHoverEffect.Attach(button, button.GetParent().FindAnyWidget("FactionBG"));
		}

		// Everything else (e.g. the description's "Back" button)
		COA_UIPolish.AttachHoverEffects(m_wRoot);
	}

	//----------------------------------------
	// Header info
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void UpdateInfoDisplay()
	{
		if (m_wPlayersText)
			m_wPlayersText.SetText("Players: " + GetGame().GetPlayerManager().GetPlayerCount());

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!m_wTimeText || !world || !world.GetTimeAndWeatherManager())
			return;

		TimeContainer time = world.GetTimeAndWeatherManager().GetTime();
		m_wTimeText.SetText(string.Format("Time: %1:%2", time.m_iHours.ToString(2), time.m_iMinutes.ToString(2)));
	}

	//----------------------------------------
	// Voice channels
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	//! A single player moved channel. The AAR rebuilds the (small) list rather than patching rows.
	protected void OnPlayerChannelChanged(int playerId, int newChannelIndex, int oldChannelIndex)
	{
		m_iShownChannelChanges = -1;
	}

	//------------------------------------------------------------------------------------------------
	//! One header per channel (click to join it) followed by its players
	protected void RebuildChannelList()
	{
		if (!m_cChannelListBoxComponent || !m_MenuManager)
			return;

		m_iShownChannelChanges = m_MenuManager.m_iChannelChanges;
		m_cChannelListBoxComponent.Clear();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int localChannel = m_MenuManager.GetChannel(localPlayerId);

		// Parse every channel's members first: Global (index 1) also shows everyone who isn't listed in
		// any channel, and that is only known once the later channels have been read too
		array<int> unlistedPlayers = {};
		playerManager.GetPlayers(unlistedPlayers);

		array<string> channelNames = {};
		array<ref array<int>> channelMembers = {};

		foreach (string channelData : m_MenuManager.m_aVONChannels)
		{
			array<string> channelParts = {};
			channelData.Split("|", channelParts, true);

			string channelName;
			if (!channelParts.IsEmpty())
				channelName = channelParts[0];

			array<int> members = {};
			if (channelParts.Count() > 1)
			{
				array<string> memberIds = {};
				channelParts[1].Split(",", memberIds, true);
				foreach (string memberId : memberIds)
				{
					int playerId = memberId.ToInt();
					if (playerId <= 0 || !playerManager.IsPlayerConnected(playerId) || members.Contains(playerId))
						continue;

					members.Insert(playerId);
					unlistedPlayers.RemoveItem(playerId);
				}
			}

			channelNames.Insert(channelName);
			channelMembers.Insert(members);
		}

		for (int channelIndex = 0, channelCount = channelNames.Count(); channelIndex < channelCount; channelIndex++)
		{
			if (channelNames[channelIndex].IsEmpty())
				continue;

			array<int> members = channelMembers[channelIndex];

			// Global also holds everyone who hasn't been put in a channel
			if (channelIndex == 1)
			{
				foreach (int unlistedId : unlistedPlayers)
					members.Insert(unlistedId);
			}

			// Like the spectator list, Deafen only ever shows yourself
			if (channelIndex == 0)
			{
				members.Clear();
				if (localChannel == 0)
					members.Insert(localPlayerId);
			}

			AddChannelRows(channelIndex, channelNames[channelIndex], members, channelIndex == localChannel, playerManager);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddChannelRows(int channelIndex, string channelName, array<int> members, bool isLocalChannel, PlayerManager playerManager)
	{
		string headerText = string.Format("%1 [%2]", channelName, members.Count());
		if (isLocalChannel)
			headerText = "> " + headerText;

		int headerIndex = m_cChannelListBoxComponent.AddItemChannel(null, headerText);
		COA_ListBoxElementComponent channelComponent = m_cChannelListBoxComponent.GetCRFElementComponent(headerIndex);
		if (channelComponent)
		{
			channelComponent.m_iChannelId = channelIndex;
			channelComponent.GetChannelButton().m_OnClicked.Insert(JoinSelectedChannelDelayed);

			// Header rows are clickable (join), so they react to hover/press/focus
			Widget headerRoot = channelComponent.GetRootWidget();
			if (headerRoot)
				COA_UIHoverEffect.Attach(headerRoot.FindAnyWidget("SlotButton"), headerRoot.FindAnyWidget("Image0"));
		}

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		foreach (int playerId : members)
		{
			// Mark yourself in words as well as with the header's ">" - not by colour alone
			string rowText = playerManager.GetPlayerName(playerId);
			if (playerId == localPlayerId)
				rowText += " (you)";

			int rowIndex = m_cChannelListBoxComponent.AddItem(rowText, null, CHANNEL_PLAYER_LAYOUT);
			COA_ListBoxElementComponent playerComponent = m_cChannelListBoxComponent.GetCRFElementComponent(rowIndex);
			if (playerComponent)
			{
				playerComponent.m_iPlayerId = playerId;
				playerComponent.m_bIsPlayer = true;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void JoinSelectedChannelDelayed()
	{
		GetGame().GetCallqueue().Call(JoinSelectedChannel);
	}

	//------------------------------------------------------------------------------------------------
	//! Group channels and Deafen/Global are joined directly. A player-created channel (named
	//! "Name's Channel (ID)") still asks its creator first, as in the spectator menu.
	protected void JoinSelectedChannel()
	{
		if (!m_cChannelListBoxComponent || !m_MenuManager)
			return;

		COA_ListBoxElementComponent selectedComponent = m_cChannelListBoxComponent.GetCRFElementComponent(m_cChannelListBoxComponent.GetSelectedItem());
		if (!selectedComponent)
			return;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!authorityManager)
			return;

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int channelId = selectedComponent.m_iChannelId;
		if (channelId < 0 || channelId >= m_MenuManager.m_aVONChannels.Count() || channelId == m_MenuManager.GetChannel(localPlayerId))
			return;

		if (m_MenuManager.m_aVONChannels[channelId].Contains("("))
			authorityManager.RequestToJoinChannel(channelId, localPlayerId);
		else
			authorityManager.JoinChannel(localPlayerId, channelId);
	}

	//----------------------------------------
	// Faction roster
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void UpdateFactionStatus(string slotsWidget, string lockBgWidget, string lockWidget, string buttonWidget, int aliveSlots, int totalSlots, bool isValid)
	{
		if (!isValid)
			return;

		TextWidget.Cast(m_wRoot.FindAnyWidget(slotsWidget)).SetText(aliveSlots.ToString() + "/" + totalSlots);
		ImageWidget.Cast(m_wRoot.FindAnyWidget(lockBgWidget)).SetColor(Color.FromRGBA(63, 63, 63, 0));
		ImageWidget.Cast(m_wRoot.FindAnyWidget(lockWidget)).SetColor(Color.FromRGBA(255, 255, 255, 0));
		ButtonWidget.Cast(m_wRoot.FindAnyWidget(buttonWidget)).SetEnabled(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Slide the faction sidebar open while the cursor is over it (see IsDrawerHovered)
	protected void AnimateSidebar(float tDelta)
	{
		if (!m_wLeftFaction || !m_wSidebarHitArea)
			return;

		float currentX = FrameSlot.GetPosX(m_wLeftFaction);
		m_bSidebarOpen = IsDrawerHovered(m_wSidebarHitArea, m_bSidebarOpen, SIDEBAR_OPEN_X - currentX, 0);

		float targetX = SIDEBAR_CLOSED_X;
		if (m_bSidebarOpen)
			targetX = SIDEBAR_OPEN_X;

		FrameSlot.SetPosX(m_wLeftFaction, EaseTowards(currentX, targetX, tDelta));
	}

	//------------------------------------------------------------------------------------------------
	//! Slide the stats panel up from the bottom edge while the cursor is over it. Closed, only its
	//! "MISSION STATISTICS" header shows as a tab.
	protected void AnimateStatsDrawer()
	{
		if (!m_wStatsHolder)
			return;

		float currentY = FrameSlot.GetPosY(m_wStatsHolder);
		bool open = IsDrawerHovered(m_wStatsHolder, m_bStatsOpen, 0, STATS_OPEN_Y - currentY);
		if (open == m_bStatsOpen)
			return;

		// Only (re)start the slide when the state changes - AnimateWidget then eases it from wherever
		// it currently is, so reversing mid-slide is smooth
		m_bStatsOpen = open;

		float target[2];
		target[0] = FrameSlot.GetPosX(m_wStatsHolder);
		target[1] = STATS_CLOSED_Y;
		if (open)
			target[1] = STATS_OPEN_Y;

		WidgetAnimationPosition slide = AnimateWidget.Position(m_wStatsHolder, target, DRAWER_ANIMATION_SPEED);
		if (slide)
			TrackAnimation(slide, m_wStatsHolder, 0, EAnimationCurve.EASE_OUT_CUBIC);
		else
			FrameSlot.SetPos(m_wStatsHolder, target[0], target[1]);
	}

	//------------------------------------------------------------------------------------------------
	//! Hover test for a sliding drawer, by rectangle rather than the widget under the cursor (which
	//! changes as different child widgets slide past, making a drawer flicker open/closed), with
	//! hysteresis: a closed drawer opens when the cursor touches the part currently on screen, and an
	//! open one stays open while the cursor is anywhere the fully-open drawer covers - even mid-slide.
	//! \param[in] hitArea widget covering the drawer's content
	//! \param[in] isOpen whether the drawer is currently open (or opening)
	//! \param[in] toOpenX,toOpenY layout-unit distance from the drawer's current to its open position
	protected bool IsDrawerHovered(Widget hitArea, bool isOpen, float toOpenX, float toOpenY)
	{
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);

		float left, top, width, height;
		hitArea.GetScreenPos(left, top);
		hitArea.GetScreenSize(width, height);
		float right = left + width;
		float bottom = top + height;

		if (isOpen)
		{
			// Where the drawer will sit once fully open, in screen pixels
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			float shiftX = workspace.DPIScale(toOpenX);
			float shiftY = workspace.DPIScale(toOpenY);
			left += shiftX;
			right += shiftX;
			top += shiftY;
			bottom += shiftY;
		}
		else
		{
			// Only the part actually on screen can be hovered to open it
			float screenWidth, screenHeight;
			m_wRoot.GetScreenSize(screenWidth, screenHeight);
			left = Math.Max(left, 0);
			top = Math.Max(top, 0);
			right = Math.Min(right, screenWidth);
			bottom = Math.Min(bottom, screenHeight);
		}

		return mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;
	}

	//------------------------------------------------------------------------------------------------
	//! Ease-out step towards target; snaps once close enough not to matter
	protected float EaseTowards(float current, float target, float tDelta)
	{
		float next = current + (target - current) * (1 - Math.Pow(2.71828, -DRAWER_SLIDE_SPEED * tDelta));
		if (Math.AbsFloat(target - next) < 0.5)
			return target;

		return next;
	}

	//------------------------------------------------------------------------------------------------
	//! Count slotted players (and those still alive) per faction
	protected void CountSlots()
	{
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iAliveBluforSlots = 0;
		m_iAliveOpforSlots = 0;
		m_iAliveIndforSlots = 0;
		m_iAliveCivSlots = 0;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		foreach (int slotId, COA_SlotData slotData : slottingManager.GetSlotMap())
		{
			if (!slotData || slotData.GetIsLockedSlot() || slotData.GetSlotCurrentPlayerId() == 0)
				continue;

			bool alive = !slotData.GetIsDeadSlot();
			switch (slotData.GetSlotFactionKey())
			{
				case "BLUFOR": m_iBluforSlots++; if (alive) m_iAliveBluforSlots++; break;
				case "OPFOR": m_iOpforSlots++; if (alive) m_iAliveOpforSlots++; break;
				case "INDFOR": m_iIndforSlots++; if (alive) m_iAliveIndforSlots++; break;
				case "CIV": m_iCivSlots++; if (alive) m_iAliveCivSlots++; break;
			}
		}

		UpdateFactionStatus("SlotsBlufor", "BluforFactionLockBG", "BluforFactionLock", "ButtonBlufor", m_iAliveBluforSlots, m_iBluforSlots, slottingManager.IsFactionValid("BLUFOR"));
		UpdateFactionStatus("SlotsOpfor", "OpforFactionLockBG", "OpforFactionLock", "ButtonOpfor", m_iAliveOpforSlots, m_iOpforSlots, slottingManager.IsFactionValid("OPFOR"));
		UpdateFactionStatus("SlotsIndfor", "IndforFactionLockBG", "IndforFactionLock", "ButtonIndfor", m_iAliveIndforSlots, m_iIndforSlots, slottingManager.IsFactionValid("INDFOR"));
		UpdateFactionStatus("SlotsCiv", "CivFactionLockBG", "CivFactionLock", "ButtonCiv", m_iAliveCivSlots, m_iCivSlots, slottingManager.IsFactionValid("CIV"));
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuild the roster for the selected faction (driven by slotting update events)
	void UpdateSlots()
	{
		CountSlots();

		if (!m_cSlotListBoxComponent || !m_fSelectedFaction)
			return;

		m_cSlotListBoxComponent.Clear();

		// Only the roster's border follows the selected faction - the voice channel list keeps the
		// neutral panel border so it doesn't change colour when browsing factions
		PanelWidget.Cast(m_wRoot.FindAnyWidget("RoleBorder")).SetColor(m_fSelectedFaction.GetFactionColor());

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		map<int, ref COA_SlotData> slotMap = slottingManager.GetSlotMap();
		array<SCR_AIGroup> factionGroups = slottingManager.GetAllGroups(m_fSelectedFaction.GetFactionKey());
		if (!factionGroups)
			return;

		foreach (SCR_AIGroup group : factionGroups)
		{
			if (!group || group.IsPrivate())
				continue;

			int groupIndex = m_cSlotListBoxComponent.AddItemGroup(null, group);
			SetGroupVisuals(group, groupIndex);

			int playersInGroup = 0;
			foreach (int slotId, COA_SlotData slotData : slotMap)
			{
				if (!IsSlotInGroupAndFaction(slotData, group))
					continue;

				int slotIndex = m_cSlotListBoxComponent.AddItemSlot(null, slotId);
				SetSlotVisuals(slotIndex, slotData);
				playersInGroup++;
			}

			if (playersInGroup == 0)
				m_cSlotListBoxComponent.RemoveItem(groupIndex);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsSlotInGroupAndFaction(COA_SlotData slotData, SCR_AIGroup group)
	{
		RplId groupId;
		if (!slotData || !COA_ReplicationHelper.GetRplId(group, groupId))
			return false;

		return slotData.GetSlotCurrentGroup() == groupId
			&& !slotData.GetIsLockedSlot()
			&& slotData.GetSlotCurrentPlayerId() != 0
			&& GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) == m_fSelectedFaction;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetGroupVisuals(SCR_AIGroup group, int groupIndex)
	{
		COA_ListBoxElementComponent element = m_cSlotListBoxComponent.GetCRFElementComponent(groupIndex);
		if (!element)
			return;

		Color factionColor = group.GetFaction().GetFactionColor();
		element.GetGroupUnderline().SetColor(factionColor);

		if (group.GetFaction().GetFactionKey() == "INDFOR")
			element.GetGroupIcon().SetColor(factionColor);

		element.GetGroupIcon().LoadImageFromSet(0, SCR_Faction.Cast(group.GetFaction()).GetGroupFlagImageSet(), group.GetGroupFlag());
	}

	//------------------------------------------------------------------------------------------------
	protected void SetSlotVisuals(int slotIndex, COA_SlotData slotData)
	{
		COA_ListBoxElementComponent element = m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex);
		if (!element)
			return;

		int playerId = slotData.GetSlotCurrentPlayerId();
		element.SetPlayerText(GetGame().GetPlayerManager().GetPlayerName(playerId));

		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			element.GetDisconnectWidget().SetVisible(true);

		element.GetSlotButton().SetEnabled(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SelectFaction(string factionKey, string selectedBackground, Color slotsColor)
	{
		if (!GetGame().GetFactionManager())
			return;

		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		if (!faction)
			return;

		m_fSelectedFaction = faction;

		array<string> backgrounds = {"BluforBGSelect", "OpforBGSelect", "IndforBGSelect", "CivBGSelect"};
		foreach (string background : backgrounds)
		{
			float opacity = 0;
			if (background == selectedBackground)
				opacity = 1;

			m_wRoot.FindAnyWidget(background).SetOpacity(opacity);
		}

		m_wRoot.FindAnyWidget("SlotsBG").SetColor(slotsColor);
		UpdateSlots();
	}

	//------------------------------------------------------------------------------------------------
	void SelectFactionBlufor()
	{
		SelectFaction("BLUFOR", "BluforBGSelect", Color.FromRGBA(34, 196, 244, 33));
	}

	//------------------------------------------------------------------------------------------------
	void SelectFactionOpfor()
	{
		SelectFaction("OPFOR", "OpforBGSelect", Color.FromRGBA(238, 49, 47, 33));
	}

	//------------------------------------------------------------------------------------------------
	void SelectFactionIndfor()
	{
		SelectFaction("INDFOR", "IndforBGSelect", Color.FromRGBA(0, 177, 79, 33));
	}

	//------------------------------------------------------------------------------------------------
	void SelectFactionCiv()
	{
		SelectFaction("CIV", "CivBGSelect", Color.FromRGBA(168, 110, 207, 33));
	}

	//----------------------------------------
	// Mission description
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	void DescriptionInit()
	{
		if (!m_cMissionDescriptionListBoxComponent || !m_Gamemode)
			return;

		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		if (scrollLayout)
			scrollLayout.SetEnabled(false);

		if (m_wBackButton)
		{
			m_wBackButton.SetOpacity(0);
			m_wBackButton.SetEnabled(false);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton)
				backButton.m_OnClicked.Clear();
		}

		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		if (missionDescriptionText)
			missionDescriptionText.SetText("");

		m_cMissionDescriptionListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();

		foreach (COA_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			m_cMissionDescriptionListBoxComponent.AddItem(description.m_sTitle, null, "{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout");
			m_aActiveDescriptors.Insert(description);
		}

		m_cMissionDescriptionListBoxComponent.m_OnChanged.Insert(DescriptionSelected);
	}

	//------------------------------------------------------------------------------------------------
	void DescriptionSelected()
	{
		int index = m_cMissionDescriptionListBoxComponent.GetSelectedItem();
		if (!m_aActiveDescriptors.IsIndexValid(index))
			return;

		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(m_wRoot.FindAnyWidget("ScrollLayout"));
		if (scrollLayout)
			scrollLayout.SetEnabled(true);

		string description = m_aActiveDescriptors.Get(index).m_sTextData;

		if (m_wBackButton)
		{
			m_wBackButton.SetOpacity(1);
			m_wBackButton.SetEnabled(true);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton)
				backButton.m_OnClicked.Insert(DescriptionInit);
		}

		m_cMissionDescriptionListBoxComponent.Clear();
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();

		RichTextWidget missionDescriptionText = RichTextWidget.Cast(m_wRoot.FindAnyWidget("DescriptionInfo"));
		if (missionDescriptionText)
			missionDescriptionText.SetText(description);
	}

	//----------------------------------------
	// Map
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	//! The map needs a few frames to initialise - each step defers to the next frame
	void OpenMap()
	{
		GetGame().GetCallqueue().Call(OpenMapWrap);
	}

	//------------------------------------------------------------------------------------------------
	void OpenMapWrap()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode || !gameMode.FindComponent(SCR_MapConfigComponent))
			return;

		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		GetGame().GetCallqueue().Call(OpenMapWrapZoomChange);
	}

	//------------------------------------------------------------------------------------------------
	void OpenMapWrapZoomChange()
	{
		GetGame().GetCallqueue().Call(OpenMapWrapZoomChangeWrap);
	}

	//------------------------------------------------------------------------------------------------
	//! Zoom out over the AO and make sure every marker (all factions) is shown
	void OpenMapWrapZoomChangeWrap()
	{
		m_MapEntity.ZoomOut();

		vector aoCenter = COA_Gamemode.GetInstance().GetGenericSpawn();
		if (aoCenter != vector.Zero)
			m_MapEntity.ZoomPanSmooth(0.3, aoCenter[0], aoCenter[2]);

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			markerManager.UpdateAllMarkerVisibilities();
	}

	//----------------------------------------
	// Voice (vanilla VON fallback) and chat
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	void Action_VONon()
	{
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);

		IEntity controlled = GetGame().GetPlayerController().GetControlledEntity();
		if (!controlled)
			return;

		SCR_VoNComponent von = SCR_VoNComponent.Cast(controlled.FindComponent(SCR_VoNComponent));
		if (!von)
			return;

		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}

	//------------------------------------------------------------------------------------------------
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		if (!entity)
			return null;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return null;

		array<IEntity> items = {};
		inventory.GetItems(items);

		BaseRadioComponent radio;
		foreach (IEntity item : items)
		{
			BaseRadioComponent itemRadio = BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent));
			if (itemRadio)
				radio = itemRadio;
		}

		if (!radio)
			return null;

		radio.SetPower(true);
		RadioTransceiver transceiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		if (transceiver)
			transceiver.SetFrequency(10000);

		return transceiver;
	}

	//------------------------------------------------------------------------------------------------
	void Action_VONOff()
	{
		GetGame().GetCallqueue().Call(LobbyVoNDisableDelayed);
	}

	//------------------------------------------------------------------------------------------------
	void LobbyVoNDisableDelayed()
	{
		IEntity controlled = GetGame().GetPlayerController().GetControlledEntity();
		if (!controlled)
			return;

		SCR_VoNComponent von = SCR_VoNComponent.Cast(controlled.FindComponent(SCR_VoNComponent));
		if (!von)
			return;

		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}

	//------------------------------------------------------------------------------------------------
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;

		GetGame().GetCallqueue().Call(OpenChatWrap);
	}

	//------------------------------------------------------------------------------------------------
	void OpenChatWrap()
	{
		if (m_ChatPanel && !m_ChatPanel.IsOpen())
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
	}

	//------------------------------------------------------------------------------------------------
	//! Opens the pause menu rather than leaving, since players often hit it by accident
	void Action_Exit()
	{
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}

	//------------------------------------------------------------------------------------------------
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}
