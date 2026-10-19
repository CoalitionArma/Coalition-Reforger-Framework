modded enum ChimeraMenuPreset
{
	CRF_ReplayBrowser,
	CRF_ReplayHUD
}

//------------------------------------------------------------------------------------------------
//! The replay's controls: timeline, play/pause, ±30 s, speed, follow a soldier, and the kill feed.
//! The spectator camera flies underneath it (see the modded COA_SpectatorCamera in CRF_ReplayLauncher).
class CRF_ReplayHUDMenu : ChimeraMenuBase
{
	protected static const string CONTEXT = "CRF_ReplayContext";
	protected static const int KILL_LINES = 6;
	protected static const float KILL_FEED_SECONDS = 25;
	protected static const float SEEK_STEP = 30;

	protected Widget m_wRoot;
	protected InputManager m_InputManager;

	protected TextWidget m_wTitle;
	protected TextWidget m_wStatus;
	protected TextWidget m_wTime;
	protected TextWidget m_wSpeed;
	protected TextWidget m_wFollow;
	protected TextWidget m_wPlayText;
	protected Widget m_wTimeline;
	protected Widget m_wTimelineFill;
	protected Widget m_wControls;
	protected ref array<TextWidget> m_aKillLines = {};

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_wRoot = GetRootWidget();
		m_InputManager = GetGame().GetInputManager();

		m_wTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		m_wStatus = TextWidget.Cast(m_wRoot.FindAnyWidget("Status"));
		m_wTime = TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText"));
		m_wSpeed = TextWidget.Cast(m_wRoot.FindAnyWidget("SpeedText"));
		m_wFollow = TextWidget.Cast(m_wRoot.FindAnyWidget("FollowText"));
		m_wTimeline = m_wRoot.FindAnyWidget("TimelineTrack");
		m_wTimelineFill = m_wRoot.FindAnyWidget("TimelineFill");
		m_wControls = m_wRoot.FindAnyWidget("Controls");

		Widget playFrame = m_wRoot.FindAnyWidget("PlayButtonFrame");
		if (playFrame)
			m_wPlayText = TextWidget.Cast(playFrame.FindAnyWidget("ButtonText"));

		for (int i = 0; i < KILL_LINES; i++)
		{
			TextWidget line = TextWidget.Cast(m_wRoot.FindAnyWidget("Kill" + i));
			if (line)
				m_aKillLines.Insert(line);
		}

		GetButtonClick(m_wRoot, "ExitButton").Insert(OnExit);
		GetButtonClick(m_wRoot, "BackButton").Insert(OnSeekBack);
		GetButtonClick(m_wRoot, "PlayButton").Insert(OnPlayPause);
		GetButtonClick(m_wRoot, "ForwardButton").Insert(OnSeekForward);
		GetButtonClick(m_wRoot, "SlowerButton").Insert(OnSlower);
		GetButtonClick(m_wRoot, "FasterButton").Insert(OnFaster);
		GetButtonClick(m_wRoot, "PrevButton").Insert(OnPrevious);
		GetButtonClick(m_wRoot, "NextButton").Insert(OnNext);
		GetButtonClick(m_wRoot, "FreeCamButton").Insert(OnFreeCam);
		GetButtonClick(m_wRoot, "TimelineButton").Insert(OnTimelineClicked);

		m_InputManager.AddActionListener("CRF_ReplayPlayPause", EActionTrigger.DOWN, OnPlayPause);
		m_InputManager.AddActionListener("CRF_ReplaySeekBack", EActionTrigger.DOWN, OnSeekBack);
		m_InputManager.AddActionListener("CRF_ReplaySeekForward", EActionTrigger.DOWN, OnSeekForward);
		m_InputManager.AddActionListener("CRF_ReplaySlower", EActionTrigger.DOWN, OnSlower);
		m_InputManager.AddActionListener("CRF_ReplayFaster", EActionTrigger.DOWN, OnFaster);
		m_InputManager.AddActionListener("CRF_ReplayPrevUnit", EActionTrigger.DOWN, OnPrevious);
		m_InputManager.AddActionListener("CRF_ReplayNextUnit", EActionTrigger.DOWN, OnNext);
		m_InputManager.AddActionListener("CRF_ReplayFreeCam", EActionTrigger.DOWN, OnFreeCam);
		m_InputManager.AddActionListener("CRF_ReplayToggleHUD", EActionTrigger.DOWN, OnToggleControls);
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnExit);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_InputManager)
		{
			m_InputManager.RemoveActionListener("CRF_ReplayPlayPause", EActionTrigger.DOWN, OnPlayPause);
			m_InputManager.RemoveActionListener("CRF_ReplaySeekBack", EActionTrigger.DOWN, OnSeekBack);
			m_InputManager.RemoveActionListener("CRF_ReplaySeekForward", EActionTrigger.DOWN, OnSeekForward);
			m_InputManager.RemoveActionListener("CRF_ReplaySlower", EActionTrigger.DOWN, OnSlower);
			m_InputManager.RemoveActionListener("CRF_ReplayFaster", EActionTrigger.DOWN, OnFaster);
			m_InputManager.RemoveActionListener("CRF_ReplayPrevUnit", EActionTrigger.DOWN, OnPrevious);
			m_InputManager.RemoveActionListener("CRF_ReplayNextUnit", EActionTrigger.DOWN, OnNext);
			m_InputManager.RemoveActionListener("CRF_ReplayFreeCam", EActionTrigger.DOWN, OnFreeCam);
			m_InputManager.RemoveActionListener("CRF_ReplayToggleHUD", EActionTrigger.DOWN, OnToggleControls);
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnExit);
		}

		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		m_InputManager.ActivateContext(CONTEXT);

		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (!player)
			return;

		SetText(m_wTitle, player.GetTitle());
		SetText(m_wStatus, player.GetStatus());
		SetText(m_wTime, string.Format("%1 / %2", CRF_Replay.Clock(player.GetTimeSeconds()), CRF_Replay.Clock(player.GetDurationSeconds())));
		SetText(m_wSpeed, string.Format("%1x", player.GetSpeed()));

		string followed = player.GetFollowedName();
		if (followed.IsEmpty())
			SetText(m_wFollow, "FREE CAMERA");
		else
			SetText(m_wFollow, followed);

		if (player.IsPlaying())
			SetText(m_wPlayText, "PAUSE");
		else
			SetText(m_wPlayText, "PLAY");

		UpdateTimeline(player);
		UpdateKillFeed(player);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTimeline(CRF_ReplayPlayer player)
	{
		if (!m_wTimeline || !m_wTimelineFill)
			return;

		float duration = player.GetDurationSeconds();
		float progress;
		if (duration > 0)
			progress = Math.Clamp(player.GetTimeSeconds() / duration, 0, 1);

		FrameSlot.SetAnchorMax(m_wTimelineFill, progress, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateKillFeed(CRF_ReplayPlayer player)
	{
		array<CRF_ReplayKillEntry> kills = {};
		player.GetRecentKills(KILL_FEED_SECONDS, kills);
		foreach (int i, TextWidget line : m_aKillLines)
		{
			if (i >= kills.Count())
			{
				line.SetVisible(false);
				continue;
			}

			CRF_ReplayKill kill = kills[i].m_Kill;
			string text;
			if (kill.kn.IsEmpty() || kill.ki == kill.ti)
				text = kill.tn;
			else
				text = string.Format("%1  [%2]  %3", kill.kn, ShortWeapon(kill.w), kill.tn);

			if (kill.d > 0)
				text += string.Format("  %1 m", kill.d);

			line.SetText(text);
			line.SetVisible(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected string ShortWeapon(string weapon)
	{
		if (weapon.IsEmpty())
			return "—";

		return weapon;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetText(TextWidget widget, string text)
	{
		if (widget)
			widget.SetText(text);
	}

	// ── Controls ─────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void OnPlayPause()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.SetPlaying(!player.IsPlaying());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSeekBack()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.SeekBy(-SEEK_STEP);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSeekForward()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.SeekBy(SEEK_STEP);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSlower()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.ChangeSpeed(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFaster()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.ChangeSpeed(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPrevious()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.FollowNext(-1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNext()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.FollowNext(1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFreeCam()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.Unfollow();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnToggleControls()
	{
		if (m_wControls)
			m_wControls.SetVisible(!m_wControls.IsVisible());
	}

	//------------------------------------------------------------------------------------------------
	//! Jump to where the timeline was clicked
	protected void OnTimelineClicked()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (!player || !m_wTimeline)
			return;

		float x, y, width, height;
		m_wTimeline.GetScreenPos(x, y);
		m_wTimeline.GetScreenSize(width, height);
		if (width <= 0)
			return;

		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		float fraction = Math.Clamp((mouseX - x) / width, 0, 1);
		player.SeekSeconds(fraction * player.GetDurationSeconds());
	}

	//------------------------------------------------------------------------------------------------
	protected void OnExit()
	{
		Close();
		CRF_ReplayLauncher.Exit();
	}

	//------------------------------------------------------------------------------------------------
	//! Click event of a named button under parent (a throwaway invoker when it's missing)
	static ScriptInvoker GetButtonClick(Widget parent, string name)
	{
		Widget button;
		if (parent)
			button = parent.FindAnyWidget(name);

		SCR_ButtonTextComponent component;
		if (button)
			component = SCR_ButtonTextComponent.Cast(button.FindHandler(SCR_ButtonTextComponent));

		if (!component)
			return new ScriptInvoker();

		return component.m_OnClicked;
	}
}
