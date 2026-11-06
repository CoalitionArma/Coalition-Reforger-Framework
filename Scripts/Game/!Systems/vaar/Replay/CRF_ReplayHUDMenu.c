modded enum ChimeraMenuPreset
{
	CRF_ReplayBrowser,
	CRF_ReplayHUD
}

//------------------------------------------------------------------------------------------------
//! The replay's controls: timeline, play/pause, ±30 s, speed, follow a soldier, the kill feed, and the
//! spectator's tools - name tags and markers (CRF_ReplayOverlay), camera modes, last kill, NVG, names,
//! teleport. The spectator camera flies underneath it (see the modded COA_SpectatorCamera in CRF_ReplayLauncher).
class CRF_ReplayHUDMenu : ChimeraMenuBase
{
	protected static const string CONTEXT = "CRF_ReplayContext";
	protected static const int KILL_LINES = 6;
	protected static const float KILL_FEED_SECONDS = 25;
	protected static const float SEEK_STEP = 30;
	protected static const int TOAST_MS = 2000;

	protected static const ResourceName NVG_HDR = "{511CD467ED159EA2}Assets/Materials/Nightvision/NVG_Spectator_HDR.emat";
	protected static const ResourceName DEFAULT_HDR = "{9DEECCABE8357209}Common/Postprocess/HDR.emat";
	protected static const ResourceName TICK_LAYOUT = "{6A7E2B0000000004}UI/layouts/Menus/Replay/CRF_ReplayTick.layout";
	protected static const int TIMELINE_TICK_SLOTS = 1000;	// at most one kill tick per thousandth of the timeline

	// Kill feed text (sRGB, rich text colour tags): recorded side 1 BLUFOR, 2 OPFOR, 3 INDFOR, 4 CIV
	protected static const ref array<string> SIDE_RGBA = {"220,226,236,255", "96,150,240,255", "232,96,96,255", "92,196,128,255", "176,120,220,255"};
	protected static const string MUTED_RGBA = "120,128,148,255";

	// Camera mode selector fills (linear, the layout's accent and field colours)
	protected static ref Color s_SelectedFill = new Color(0.5841, 0.0369, 0.0369, 1);
	protected static ref Color s_Fill = new Color(0.011, 0.0137, 0.0212, 1);

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
	protected Widget m_wTitlePanel;
	protected Widget m_wKillFeed;
	protected Widget m_wTimelineHead;
	protected Widget m_wTimelineButton;
	protected Widget m_wPreview;
	protected TextWidget m_wPreviewText;
	protected ref array<Widget> m_aKillRows = {};
	protected ref array<RichTextWidget> m_aKillLines = {};

	protected ref CRF_ReplayOverlay m_Overlay;
	protected ref COA_MenuPolish m_UIPolish;

	// Map (the spectator's, CRF_ReplayMapOverlay)
	protected static CRF_ReplayHUDMenu s_Instance;
	protected FrameWidget m_wMapFrame;
	protected Widget m_wMapPins;
	protected SCR_MapEntity m_MapEntity;
	protected ref CRF_ReplayMapOverlay m_MapOverlay;
	protected bool m_bMapOpen;
	protected bool m_bTicksBuilt;
	protected int m_iShownCameraMode = -1;
	protected bool m_bHideUI;
	protected bool m_bNVG;
	protected string m_sToast;
	protected int m_iToastUntil;

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
		m_wTitlePanel = m_wRoot.FindAnyWidget("TitlePanel");
		m_wKillFeed = m_wRoot.FindAnyWidget("KillFeed");
		m_wTimelineHead = m_wRoot.FindAnyWidget("TimelineHead");
		m_wTimelineButton = m_wRoot.FindAnyWidget("TimelineButton");
		m_wPreview = m_wRoot.FindAnyWidget("TimelinePreview");
		m_wPreviewText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimelinePreviewText"));
		m_wMapFrame = FrameWidget.Cast(m_wRoot.FindAnyWidget("MapFrame"));
		m_wMapPins = m_wRoot.FindAnyWidget("MapPins");
		s_Instance = this;

		Widget playFrame = m_wRoot.FindAnyWidget("PlayButtonFrame");
		if (playFrame)
			m_wPlayText = TextWidget.Cast(playFrame.FindAnyWidget("ButtonText"));

		for (int i = 0; i < KILL_LINES; i++)
		{
			Widget killRow = m_wRoot.FindAnyWidget("Kill" + i + "Row");
			RichTextWidget line = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Kill" + i));
			if (killRow && line)
			{
				m_aKillRows.Insert(killRow);
				m_aKillLines.Insert(line);
			}
		}

		Widget markers = m_wRoot.FindAnyWidget("WorldMarkers");
		if (!markers)
			markers = m_wRoot;

		m_Overlay = new CRF_ReplayOverlay(markers);
		SetupKeyHints();

		// Hover / pressed states and a quick fade-in, as in the Lobby's menus; the name tags and markers have their own
		array<string> ownHover = {"WorldMarkers", "MapFrame", "MapPins"};
		m_UIPolish = new COA_MenuPolish(m_wRoot, true, ownHover);

		GetButtonClick(m_wRoot, "CamHelmetButton").Insert(OnCameraHelmet);
		GetButtonClick(m_wRoot, "CamEyeButton").Insert(OnCameraEye);
		GetButtonClick(m_wRoot, "CamOrbitButton").Insert(OnCameraOrbit);

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
		m_InputManager.AddActionListener("CRF_ReplayFreeCam", EActionTrigger.DOWN, OnFreeCam);
		m_InputManager.AddActionListener("CRF_ReplayToggleHUD", EActionTrigger.DOWN, OnToggleUI);
		m_InputManager.AddActionListener("CRF_ReplayNVG", EActionTrigger.DOWN, OnToggleNVG);
		m_InputManager.AddActionListener("COA_SpecPrevPlayer", EActionTrigger.DOWN, OnPrevious);
		m_InputManager.AddActionListener("COA_SpecNextPlayer", EActionTrigger.DOWN, OnNext);
		m_InputManager.AddActionListener("COA_SpecToggleCamMode", EActionTrigger.DOWN, OnCameraMode);
		m_InputManager.AddActionListener("COA_SpecToggleNames", EActionTrigger.DOWN, OnToggleNames);
		m_InputManager.AddActionListener("COA_SpecKillTeleport", EActionTrigger.DOWN, OnLastKill);
		m_InputManager.AddActionListener("ManualCameraTeleport", EActionTrigger.DOWN, OnFreeCam);
		m_InputManager.AddActionListener("GadgetMap", EActionTrigger.DOWN, OnToggleMap);
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
			m_InputManager.RemoveActionListener("CRF_ReplayFreeCam", EActionTrigger.DOWN, OnFreeCam);
			m_InputManager.RemoveActionListener("CRF_ReplayToggleHUD", EActionTrigger.DOWN, OnToggleUI);
			m_InputManager.RemoveActionListener("CRF_ReplayNVG", EActionTrigger.DOWN, OnToggleNVG);
			m_InputManager.RemoveActionListener("COA_SpecPrevPlayer", EActionTrigger.DOWN, OnPrevious);
			m_InputManager.RemoveActionListener("COA_SpecNextPlayer", EActionTrigger.DOWN, OnNext);
			m_InputManager.RemoveActionListener("COA_SpecToggleCamMode", EActionTrigger.DOWN, OnCameraMode);
			m_InputManager.RemoveActionListener("COA_SpecToggleNames", EActionTrigger.DOWN, OnToggleNames);
			m_InputManager.RemoveActionListener("COA_SpecKillTeleport", EActionTrigger.DOWN, OnLastKill);
			m_InputManager.RemoveActionListener("ManualCameraTeleport", EActionTrigger.DOWN, OnFreeCam);
			m_InputManager.RemoveActionListener("GadgetMap", EActionTrigger.DOWN, OnToggleMap);
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnExit);
		}

		if (m_bMapOpen)
			CloseMap();

		if (s_Instance == this)
			s_Instance = null;

		if (m_bNVG)
			SetNVG(false);

		if (m_UIPolish)
			m_UIPolish.Cleanup();

		m_Overlay = null;
		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		m_InputManager.ActivateContext(CONTEXT);
		if (m_UIPolish)
			m_UIPolish.Update(tDelta);

		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (!player)
			return;

		if (!m_bTicksBuilt && !player.IsLoading())
			BuildTimelineTicks(player);

		UpdateCameraButtons(player);
		UpdateTimelinePreview(player);

		if (m_bMapOpen)
		{
			// The map pans with the camera keys - they mustn't let go of the soldier being followed
			m_InputManager.ActivateContext("MapContext");
			if (m_MapOverlay)
				m_MapOverlay.Update(tDelta);
		}
		else
		{
			ReleaseOnManualControl(player);
			if (m_Overlay)
				m_Overlay.Update(player);
		}

		SetText(m_wTitle, player.GetTitle());
		string status = player.GetStatus();
		if (status.IsEmpty() && System.GetTickCount() < m_iToastUntil)
			status = m_sToast;

		SetText(m_wStatus, status);
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
	//! As in the spectator: flying the camera yourself stops following (looking around only does in first
	//! person - RMB orbits the soldier in the third-person camera)
	protected void ReleaseOnManualControl(CRF_ReplayPlayer player)
	{
		if (player.GetFollowedId() == 0)
			return;

		bool manual = m_InputManager.GetActionValue("ManualCameraMoveLateral") != 0
			|| m_InputManager.GetActionValue("ManualCameraMoveVertical") != 0
			|| m_InputManager.GetActionValue("ManualCameraMoveLongitudinal") != 0
			|| (player.GetCameraMode() != CRF_EReplayCameraMode.ORBIT && m_InputManager.GetActionValue("ManualCameraRotate") != 0);

		if (manual)
			player.Unfollow();
	}

	//------------------------------------------------------------------------------------------------
	//! Bottom strip: the shortcuts with the player's own key glyphs, as on the spectator's hint bar
	protected void SetupKeyHints()
	{
		string separator = "      ";
		RichTextWidget playback = RichTextWidget.Cast(m_wRoot.FindAnyWidget("Keys"));
		if (playback)
		{
			string text = "<action name='CRF_ReplayPlayPause'/> Play / pause";
			text += separator + "<action name='CRF_ReplaySeekBack'/><action name='CRF_ReplaySeekForward'/> 30 s";
			text += separator + "<action name='CRF_ReplaySlower'/><action name='CRF_ReplayFaster'/> Speed";
			text += separator + "<action name='CRF_ReplayToggleHUD'/> Hide UI";
			text += separator + "<action name='" + UIConstants.MENU_ACTION_BACK + "'/> Exit";
			playback.SetText(text);
		}

		RichTextWidget camera = RichTextWidget.Cast(m_wRoot.FindAnyWidget("CameraKeys"));
		if (camera)
		{
			string cameraText = "<action name='COA_SpecPrevPlayer'/><action name='COA_SpecNextPlayer'/> Switch player";
			cameraText += separator + "<action name='COA_SpecToggleCamMode'/> Camera";
			cameraText += separator + "<action name='CRF_ReplayFreeCam'/> Free camera";
			cameraText += separator + "<action name='COA_SpecKillTeleport'/> Last kill";
			cameraText += separator + "<action name='CRF_ReplayNVG'/> NVG";
			cameraText += separator + "<action name='COA_SpecToggleNames'/> Names";
			cameraText += separator + "<action name='ManualCameraTeleport'/> Teleport";
			cameraText += separator + "<action name='GadgetMap'/> Map";
			camera.SetText(cameraText);
		}
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

		// The track's anchors are its vertical middle
		FrameSlot.SetAnchorMax(m_wTimelineFill, progress, 0.5);
		if (m_wTimelineHead)
		{
			FrameSlot.SetAnchorMin(m_wTimelineHead, progress, 0.5);
			FrameSlot.SetAnchorMax(m_wTimelineHead, progress, 0.5);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A tick on the timeline at every kill, once the recording has been read
	protected void BuildTimelineTicks(CRF_ReplayPlayer player)
	{
		m_bTicksBuilt = true;
		Widget ticks = m_wRoot.FindAnyWidget("TimelineTicks");
		int last = player.GetLastFrame();
		if (!ticks || last <= 0)
			return;

		array<int> frames = {};
		player.GetKillFrames(frames);
		set<int> placed = new set<int>();
		foreach (int frame : frames)
		{
			float fraction = Math.Clamp(frame / (float)last, 0, 1);
			int slotIndex = Math.Round(fraction * TIMELINE_TICK_SLOTS);
			if (placed.Contains(slotIndex))
				continue;

			placed.Insert(slotIndex);
			Widget tick = GetGame().GetWorkspace().CreateWidgets(TICK_LAYOUT, ticks);
			if (!tick)
				continue;

			FrameSlot.SetAnchorMin(tick, fraction, 0);
			FrameSlot.SetAnchorMax(tick, fraction, 0);
			FrameSlot.SetPos(tick, -1, 0);
			FrameSlot.SetSize(tick, 2, 14);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The time under the cursor, above the timeline
	protected void UpdateTimelinePreview(CRF_ReplayPlayer player)
	{
		if (!m_wPreview)
			return;

		bool hovered = !m_bHideUI && m_wTimelineButton && m_wTimeline && WidgetManager.GetWidgetUnderCursor() == m_wTimelineButton;
		m_wPreview.SetVisible(hovered);
		if (!hovered)
			return;

		float x, y, width, height;
		m_wTimeline.GetScreenPos(x, y);
		m_wTimeline.GetScreenSize(width, height);
		if (width <= 0)
			return;

		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		float fraction = Math.Clamp((mouseX - x) / width, 0, 1);
		SetText(m_wPreviewText, CRF_Replay.Clock(fraction * player.GetDurationSeconds()));

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		FrameSlot.SetPos(m_wPreview, workspace.DPIUnscale(x + fraction * width) - 32, workspace.DPIUnscale(y) - 30);
	}

	//------------------------------------------------------------------------------------------------
	//! The camera selector shows the mode in use - through the hover effect, since it's usually just been clicked
	protected void UpdateCameraButtons(CRF_ReplayPlayer player)
	{
		int mode = player.GetCameraMode();
		if (mode == m_iShownCameraMode)
			return;

		m_iShownCameraMode = mode;
		array<string> names = {"CamHelmetButton", "CamEyeButton", "CamOrbitButton"};
		array<CRF_EReplayCameraMode> modes = {CRF_EReplayCameraMode.HELMET, CRF_EReplayCameraMode.EYE, CRF_EReplayCameraMode.ORBIT};
		foreach (int i, string name : names)
		{
			Widget button = m_wRoot.FindAnyWidget(name);
			Widget background = m_wRoot.FindAnyWidget(name + "BG");
			if (!background)
				continue;

			if (modes[i] == mode)
				COA_UIHoverEffect.SetTargetColor(button, background, s_SelectedFill);
			else
				COA_UIHoverEffect.SetTargetColor(button, background, s_Fill);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Killer and victim in their side's colour, the weapon and distance muted
	protected void UpdateKillFeed(CRF_ReplayPlayer player)
	{
		array<CRF_ReplayKillEntry> kills = {};
		player.GetRecentKills(KILL_FEED_SECONDS, kills);
		foreach (int i, RichTextWidget line : m_aKillLines)
		{
			Widget killRow = m_aKillRows[i];
			if (i >= kills.Count())
			{
				killRow.SetVisible(false);
				continue;
			}

			CRF_ReplayKill kill = kills[i].m_Kill;
			string text;
			if (kill.kn.IsEmpty() || kill.kn == kill.tn)
				text = Colored(kill.tn, SideColor(kill.tf));
			else
				text = string.Format("%1  %2  %3", Colored(kill.kn, SideColor(kill.kf)), Colored(ShortWeapon(kill.w), MUTED_RGBA), Colored(kill.tn, SideColor(kill.tf)));

			if (kill.d > 0)
				text += "  " + Colored(string.Format("%1 m", kill.d), MUTED_RGBA);

			line.SetText(text);
			killRow.SetVisible(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string SideColor(int side)
	{
		if (side < 0 || side >= SIDE_RGBA.Count())
			side = 0;

		return SIDE_RGBA[side];
	}

	//------------------------------------------------------------------------------------------------
	protected static string Colored(string text, string rgba)
	{
		return "<color rgba='" + rgba + "'>" + text + "</color>";
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

	//------------------------------------------------------------------------------------------------
	//! A line under the title for a moment (the spectator's popup)
	protected void Toast(string message)
	{
		m_sToast = message;
		m_iToastUntil = System.GetTickCount() + TOAST_MS;
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
	//! Also the teleport key: the camera's own teleport component moves it, following would pull it back
	protected void OnFreeCam()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.Unfollow();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCameraMode()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (!player)
			return;

		switch (player.CycleCameraMode())
		{
			case CRF_EReplayCameraMode.HELMET: Toast("Camera: helmet cam"); break;
			case CRF_EReplayCameraMode.EYE: Toast("Camera: eye cam"); break;
			default: Toast("Camera: orbit");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCameraHelmet()
	{
		SetCameraMode(CRF_EReplayCameraMode.HELMET);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCameraEye()
	{
		SetCameraMode(CRF_EReplayCameraMode.EYE);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCameraOrbit()
	{
		SetCameraMode(CRF_EReplayCameraMode.ORBIT);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetCameraMode(CRF_EReplayCameraMode mode)
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.SetCameraMode(mode);
	}

	//------------------------------------------------------------------------------------------------
	//! Player names: when close -> always shown -> hidden (shared with the spectator)
	protected void OnToggleNames()
	{
		COA_ESpecNameMode mode = COA_SpectatorLabelIconCharacter.CycleNameMode();
		if (mode == COA_ESpecNameMode.ALWAYS)
			Toast("Player names: always shown");
		else if (mode == COA_ESpecNameMode.HIDDEN)
			Toast("Player names: hidden");
		else
			Toast("Player names: when close");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLastKill()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (!player)
			return;

		vector position;
		if (!player.GetLastKillPosition(position))
		{
			Toast("No kill to go to yet");
			return;
		}

		player.MoveCameraTo(position);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnToggleNVG()
	{
		m_bNVG = !m_bNVG;
		SetNVG(m_bNVG);
	}

	//------------------------------------------------------------------------------------------------
	//! The spectator's night vision post-process on the current camera
	protected static void SetNVG(bool on)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		int cameraId = world.GetCurrentCameraId();
		if (on)
		{
			world.SetCameraPostProcessEffect(cameraId, 16, PostProcessEffectType.HDR, NVG_HDR);
			world.SetCameraHDRBrightness(cameraId, 0.4);
		}
		else
		{
			world.SetCameraPostProcessEffect(cameraId, 16, PostProcessEffectType.HDR, DEFAULT_HDR);
			world.SetCameraHDRBrightness(cameraId, -1);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Everything off for a clean view, and back
	protected void OnToggleUI()
	{
		m_bHideUI = !m_bHideUI;
		array<Widget> panels = {m_wControls, m_wTitlePanel, m_wKillFeed};
		foreach (Widget widget : panels)
		{
			if (widget)
				widget.SetVisible(!m_bHideUI);
		}

		if (m_Overlay)
			m_Overlay.SetVisible(!m_bHideUI && !m_bMapOpen);
	}

	// ── Map ──────────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void OnToggleMap()
	{
		if (m_bMapOpen)
			CloseMap();
		else
			OpenMap();
	}

	//------------------------------------------------------------------------------------------------
	//! The spectator's map: the game mode's editor map config, full screen, every soldier on it
	protected void OpenMap()
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity || !m_wMapFrame)
		{
			Toast("No map in this world");
			return;
		}

		SCR_MapConfigComponent configComponent;
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			configComponent = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));

		if (!configComponent)
		{
			Print("[CRF_Replay] The replay game mode has no SCR_MapConfigComponent - no map", LogLevel.ERROR);
			Toast("No map for this replay");
			return;
		}

		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
			camera.SetInputEnabled(false);

		m_wMapFrame.SetVisible(true);
		Widget toolMenu = m_wMapFrame.FindAnyWidget("ToolMenuHoriz");
		if (toolMenu)
			SCR_WidgetHelper.RemoveAllChildren(toolMenu);

		MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, configComponent.GetEditorMapConfig(), m_wMapFrame);
		m_MapEntity.OpenMap(mapConfig);
		m_bMapOpen = true;

		if (m_wMapPins)
		{
			m_wMapPins.SetVisible(true);
			if (!m_MapOverlay)
				m_MapOverlay = new CRF_ReplayMapOverlay();

			m_MapOverlay.Open(m_wMapPins, m_MapEntity);
		}

		// The name tags would float over the map
		if (m_Overlay)
			m_Overlay.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseMap()
	{
		m_bMapOpen = false;
		if (m_MapOverlay)
			m_MapOverlay.Close();

		if (m_wMapPins)
			m_wMapPins.SetVisible(false);

		CameraManager cameraManager = GetGame().GetCameraManager();
		SCR_ManualCamera camera;
		if (cameraManager)
			camera = SCR_ManualCamera.Cast(cameraManager.CurrentCamera());
		if (camera)
			camera.SetInputEnabled(true);

		if (m_wMapFrame)
			m_wMapFrame.SetVisible(false);

		if (m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();

		if (m_Overlay)
			m_Overlay.SetVisible(!m_bHideUI);
	}

	//------------------------------------------------------------------------------------------------
	//! A soldier (or a vehicle's crew) clicked on the map: close it and follow them
	static void FollowFromMap(int characterId)
	{
		if (s_Instance)
			s_Instance.CloseMap();

		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.Follow(characterId);
	}

	//------------------------------------------------------------------------------------------------
	//! A body clicked on the map: close it and fly the free camera there
	static void MoveCameraFromMap(vector position)
	{
		if (s_Instance)
			s_Instance.CloseMap();

		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.MoveCameraTo(position);
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
		// Back closes the map first, like the spectator's
		if (m_bMapOpen)
		{
			CloseMap();
			return;
		}

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
