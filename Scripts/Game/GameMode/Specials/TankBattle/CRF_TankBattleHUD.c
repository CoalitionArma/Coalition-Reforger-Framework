//------------------------------------------------------------------------------------
// CRF_TankBattleHUD: the panel at the top of the screen - one even row with each side and its score
// either side of the two bars ("BLUFOR 120 [bar] [bar] OPFOR 85"), a box per capture point coloured
// by who holds it, and a short status tag ("CAPTURING B 45%", "BLUFOR WINS") only when there is
// something to say. The minimap (bottom right, CRF_TankBattleMinimap) and a crosshair where the
// gun points (third person) are drawn every frame (UpdateFrame), and so is a 3D marker over each
// capture point: its letter in the owner's colour, a bar with the capture progress in the colour of
// the side it's swinging to, and a line saying who is taking it (or the distance). A marker fades
// almost out while the gun is aimed into its zone or near it on screen, so it doesn't hide targets
// on the objective. Per frame the markers are only placed, faded and pulsed; what they say is set
// twice a second with the rest of the panel.
//
// The local side is always on the left in blue, the enemy on the right in red (spectators: the first
// side on the left). Styled like CRF_PayloadHUD. Client only, built from what
// CRF_TankBattleGamemodeComponent already replicates. Out of the way while a menu is open.
//------------------------------------------------------------------------------------
class CRF_TankBattleHUD
{
	protected static const ResourceName LAYOUT = "{7C2E5A91B3D04F60}UI/layouts/HUD/TankBattle/CRF_TankBattleHUD.layout";
	protected static const int MAX_POINTS = 5;

	protected static const ref Color OWN_COLOR     = Color.FromSRGBA(60, 140, 255, 255);
	protected static const ref Color ENEMY_COLOR   = Color.FromSRGBA(225, 45, 35, 255);
	protected static const ref Color NEUTRAL_COLOR = Color.FromSRGBA(70, 74, 84, 230);
	protected static const ref Color DETAIL_COLOR  = Color.FromSRGBA(200, 210, 225, 255);
	protected static const ref Color ALERT_COLOR   = Color.FromSRGBA(255, 200, 90, 255);

	protected Widget m_wRoot;
	protected TextWidget m_wSide0;
	protected TextWidget m_wSide1;
	protected ProgressBarWidget m_wBar0;
	protected ProgressBarWidget m_wBar1;
	protected TextWidget m_wDetail;
	protected ref array<Widget> m_aPointWidgets = {};
	protected ref array<ImageWidget> m_aPointBacks = {};
	protected ref array<TextWidget> m_aPointTexts = {};
	protected Widget m_wMinimapPanel;
	protected ref CRF_TankBattleMinimap m_Minimap;	//!< kept when the HUD goes down, so its view is only fitted once
	protected Widget m_wCrosshair;
	protected ref array<Widget> m_aMarkers = {};
	protected ref array<ImageWidget> m_aMarkerBacks = {};
	protected ref array<TextWidget> m_aMarkerTexts = {};
	protected ref array<ProgressBarWidget> m_aMarkerBars = {};
	protected ref array<TextWidget> m_aMarkerInfos = {};
	protected ref array<ref Color> m_aMarkerColors = {};	//!< each box's colour (UpdateMarkerDetails)
	protected ref array<bool> m_aMarkerPulsing = {};		//!< the point is being taken: the box pulses
	protected ref array<float> m_aMarkerFade = {};			//!< each marker's opacity now
	protected ref Color m_aMarkerPulseColor = new Color(1, 1, 1, 1);	//!< reused for the pulse, no per-frame allocation
	protected int m_iMarkerFrameMs;

	protected static const float MARKER_HALF_WIDTH = 48;	//!< half the MarkerN frame's width in the layout
	protected static const float MARKER_HALF_HEIGHT = 33;	//!< half its height
	protected static const float MARKER_HEIGHT = 10;		//!< m above the point's centre
	protected static const float MARKER_FADE_NEAR = 30;		//!< px between the gun's aim and the marker's box: faded out within this...
	protected static const float MARKER_FADE_FAR = 150;		//!< ...back to full opacity beyond this
	protected static const float MARKER_FADE_MIN = 0.12;	//!< opacity while faded: still there, barely
	protected static const float MARKER_FADE_SPEED = 10;	//!< how quickly it fades (per second)

	protected static const float CROSSHAIR_HALF = 18;		//!< half the Crosshair frame's size in the layout

	//------------------------------------------------------------------------------------------------
	void Update(CRF_TankBattleGamemodeComponent mode)
	{
		if (!mode || mode.SideCount() != 2 || !ShouldShow())
		{
			if (m_wRoot)
				m_wRoot.SetVisible(false);
			return;
		}

		if (!m_wRoot && !Create())
			return;

		m_wRoot.SetVisible(true);

		int left = mode.GetLocalSide();
		if (left < 0)
			left = 0;
		int right = 1 - left;

		m_wSide0.SetText(GetSideName(mode, left) + " " + mode.GetScore(left).ToString());
		m_wSide1.SetText(GetSideName(mode, right) + " " + mode.GetScore(right).ToString());

		m_wBar0.SetMax(mode.GetScoreLimit());
		m_wBar0.SetCurrent(mode.GetScore(left));
		m_wBar0.SetColor(OWN_COLOR);
		m_wBar1.SetMax(mode.GetScoreLimit());
		m_wBar1.SetCurrent(mode.GetScore(right));
		m_wBar1.SetColor(ENEMY_COLOR);

		UpdatePoints(mode, left);
		UpdateDetail(mode, left);
		UpdateMarkerDetails(mode, left);

		// Only once the player sits in the tank they picked: not on foot, in the tank menu, dead or spectating
		if (m_wMinimapPanel)
			m_wMinimapPanel.SetVisible(mode.ShowMinimap() && mode.GetLocalTank() != null);
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame: the minimap and the crosshair
	void UpdateFrame(CRF_TankBattleGamemodeComponent mode)
	{
		if (!mode || !m_wRoot || !m_wRoot.IsVisible())
			return;

		if (m_Minimap && m_wMinimapPanel && m_wMinimapPanel.IsVisible())
			m_Minimap.Update(mode);

		UpdateCrosshair(mode);
		UpdateMarkers(mode);
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame: each capture point's marker placed over it (it has to follow the camera), faded
	//! while the gun is aimed at or near it so it doesn't hide targets on the objective, and its box
	//! pulsing while the point is being taken. What it says is set twice a second (UpdateMarkerDetails).
	protected void UpdateMarkers(notnull CRF_TankBattleGamemodeComponent mode)
	{
		BaseWorld world = GetGame().GetWorld();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!world || !workspace)
			return;

		int now = world.GetWorldTime();
		float step = Math.Clamp((now - m_iMarkerFrameMs) * 0.001 * MARKER_FADE_SPEED, 0, 1);
		m_iMarkerFrameMs = now;

		// Where the gun is aimed: on screen, and on the ground (once a frame, shared with the crosshair)
		vector aimScreen;
		vector aimWorld;
		bool aiming = GetGunAim(mode, workspace, world, aimScreen, aimWorld);

		// The box pulses while the point is being taken
		float pulse = 0.55 + 0.45 * Math.Sin(now * 0.008);

		int count = mode.GetPointCount();
		foreach (int i, Widget marker : m_aMarkers)
		{
			IEntity pointEntity;
			if (i < count)
				pointEntity = mode.GetPointEntity(i);

			if (!pointEntity)
			{
				marker.SetVisible(false);
				continue;
			}

			vector origin = pointEntity.GetOrigin();
			vector screen = workspace.ProjWorldToScreen(origin + Vector(0, MARKER_HEIGHT, 0), world);
			if (screen[2] <= 0)
			{
				marker.SetVisible(false);
				continue;
			}

			marker.SetVisible(true);
			FrameSlot.SetPos(marker, screen[0] - MARKER_HALF_WIDTH, screen[1]);

			// Faded while the aim is on the zone (on the ground) or close to the marker (on screen)
			float target = 1;
			if (aiming)
			{
				float dx = Math.Max(Math.AbsFloat(aimScreen[0] - screen[0]) - MARKER_HALF_WIDTH, 0);
				float dy = Math.Max(Math.AbsFloat(aimScreen[1] - (screen[1] + MARKER_HALF_HEIGHT)) - MARKER_HALF_HEIGHT, 0);
				float screenDistance = Math.Sqrt(dx * dx + dy * dy);
				target = Math.Clamp((screenDistance - MARKER_FADE_NEAR) / (MARKER_FADE_FAR - MARKER_FADE_NEAR), 0, 1);

				float groundX = aimWorld[0] - origin[0];
				float groundZ = aimWorld[2] - origin[2];
				float radius = mode.GetPointRadius(i);
				if (groundX * groundX + groundZ * groundZ <= radius * radius)
					target = 0;

				target = MARKER_FADE_MIN + (1 - MARKER_FADE_MIN) * target;
			}

			float fade = m_aMarkerFade[i];
			fade += (target - fade) * step;
			m_aMarkerFade[i] = fade;
			marker.SetOpacity(fade);

			if (m_aMarkerPulsing[i])
			{
				Color back = m_aMarkerColors[i];
				m_aMarkerPulseColor.SetR(back.R());
				m_aMarkerPulseColor.SetG(back.G());
				m_aMarkerPulseColor.SetB(back.B());
				m_aMarkerPulseColor.SetA(back.A() * pulse);
				m_aMarkerBacks[i].SetColor(m_aMarkerPulseColor);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Where the local gun is aimed, on screen and in the world. Third person and through the sight.
	protected bool GetGunAim(notnull CRF_TankBattleGamemodeComponent mode, notnull WorkspaceWidget workspace, notnull BaseWorld world, out vector screen, out vector hit)
	{
		vector origin;
		vector direction;
		float distance;
		if (!mode.GetLocalTank() || !mode.GetLocalGunRay(origin, direction, distance))
			return false;

		hit = origin + direction * distance;
		screen = workspace.ProjWorldToScreen(hit, world);
		return screen[2] > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Twice a second: what each marker says and its colours (the per-frame part only places, fades
	//! and pulses it)
	protected void UpdateMarkerDetails(notnull CRF_TankBattleGamemodeComponent mode, int left)
	{
		// From the camera when there is one, so the distance matches the view
		vector eye;
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (cameraManager && cameraManager.CurrentCamera())
			eye = cameraManager.CurrentCamera().GetOrigin();
		else if (SCR_PlayerController.GetLocalControlledEntity())
			eye = SCR_PlayerController.GetLocalControlledEntity().GetOrigin();

		int count = Math.Min(mode.GetPointCount(), m_aMarkers.Count());
		for (int i = 0; i < count; i++)
		{
			IEntity pointEntity = mode.GetPointEntity(i);
			if (pointEntity)
				UpdateMarker(mode, i, left, vector.Distance(eye, pointEntity.GetOrigin()));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateMarker(notnull CRF_TankBattleGamemodeComponent mode, int i, int left, float distance)
	{
		int owner = mode.GetPointOwner(i);
		int capturing = mode.GetPointCapturing(i);

		// -100 (side 1 holds it) to 100 (side 0): towards the local side positive
		int progress = mode.GetPointProgress(i);
		if (left == 1)
			progress = -progress;

		m_aMarkerTexts[i].SetText(mode.GetPointName(i));

		// The box's colour; while the point is being taken UpdateMarkers pulses its alpha every frame
		Color back = GetSideColor(owner, left);
		bool taking = capturing >= 0 && capturing != CRF_TankBattleGamemodeComponent.CONTESTED && !(capturing == owner && Math.AbsInt(progress) >= 100);
		m_aMarkerColors[i] = back;
		m_aMarkerPulsing[i] = taking || capturing == CRF_TankBattleGamemodeComponent.CONTESTED;
		m_aMarkerBacks[i].SetColor(back);

		// The bar: how far the point has swung, in the colour of the side it's swinging to
		ProgressBarWidget bar = m_aMarkerBars[i];
		bar.SetMax(100);
		bar.SetCurrent(Math.AbsInt(progress));
		if (progress >= 0)
			bar.SetColor(OWN_COLOR);
		else
			bar.SetColor(ENEMY_COLOR);

		TextWidget info = m_aMarkerInfos[i];
		if (capturing == CRF_TankBattleGamemodeComponent.CONTESTED)
		{
			info.SetText("CONTESTED");
			info.SetColor(ALERT_COLOR);
		}
		else if (taking && capturing == left)
		{
			info.SetText("CAPTURING");
			info.SetColor(OWN_COLOR);
		}
		else if (taking)
		{
			info.SetText("ENEMY CAPTURING");
			info.SetColor(ENEMY_COLOR);
		}
		else
		{
			int metres = (int)distance;
			info.SetText(metres.ToString() + " m");
			info.SetColor(DETAIL_COLOR);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Where the gun would hit, on screen (CRF_TankBattleGamemodeComponent.GetLocalGunRay). Third person only,
	//! the sight has its own reticle.
	protected void UpdateCrosshair(notnull CRF_TankBattleGamemodeComponent mode)
	{
		if (!m_wCrosshair)
			return;

		vector screen;
		bool show = mode.ShowCrosshair() && GetGunScreenPoint(mode, screen);
		m_wCrosshair.SetVisible(show);
		if (show)
			FrameSlot.SetPos(m_wCrosshair, screen[0] - CROSSHAIR_HALF, screen[1] - CROSSHAIR_HALF);
	}

	//------------------------------------------------------------------------------------------------
	protected bool GetGunScreenPoint(notnull CRF_TankBattleGamemodeComponent mode, out vector screen)
	{
		Vehicle tank = mode.GetLocalTank();
		TurretControllerComponent turret = mode.GetLocalTurret();
		if (!tank || !turret || !turret.GetTurretComponent() || turret.IsWeaponADS())
			return false;

		IEntity local = SCR_PlayerController.GetLocalControlledEntity();
		if (!local)
			return false;

		SCR_CharacterCameraHandlerComponent cameraHandler = SCR_CharacterCameraHandlerComponent.Cast(local.FindComponent(SCR_CharacterCameraHandlerComponent));
		if (!cameraHandler || !cameraHandler.IsInThirdPerson())
			return false;

		vector origin;
		vector direction;
		float distance;
		if (!mode.GetLocalGunRay(origin, direction, distance))
			return false;

		BaseWorld world = GetGame().GetWorld();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!world || !workspace)
			return false;

		vector hit = origin + direction * distance;
		screen = workspace.ProjWorldToScreen(hit, world);
		return screen[2] > 0;
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wMinimapPanel = null;
		m_wCrosshair = null;
		m_aMarkers.Clear();
		m_aMarkerBacks.Clear();
		m_aMarkerTexts.Clear();
		m_aMarkerBars.Clear();
		m_aMarkerInfos.Clear();
		m_aMarkerColors.Clear();
		m_aMarkerPulsing.Clear();
		m_aMarkerFade.Clear();
		if (m_Minimap)
			m_Minimap.SetCanvas(null);

		m_aPointWidgets.Clear();
		m_aPointBacks.Clear();
		m_aPointTexts.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected string GetSideName(notnull CRF_TankBattleGamemodeComponent mode, int side)
	{
		return CRF_SpecialsHelper.GetFactionName(mode.GetSide(side).m_sFactionKey);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePoints(notnull CRF_TankBattleGamemodeComponent mode, int left)
	{
		int count = mode.GetPointCount();
		foreach (int i, Widget pointWidget : m_aPointWidgets)
		{
			bool shown = i < count;
			pointWidget.SetVisible(shown);
			if (!shown)
				continue;

			m_aPointTexts[i].SetText(mode.GetPointName(i));
			m_aPointBacks[i].SetColor(GetSideColor(mode.GetPointOwner(i), left));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetSideColor(int side, int left)
	{
		if (side < 0)
			return NEUTRAL_COLOR;

		if (side == left)
			return OWN_COLOR;

		return ENEMY_COLOR;
	}

	//------------------------------------------------------------------------------------------------
	//! A short tag, hidden when there's nothing to say
	protected void UpdateDetail(notnull CRF_TankBattleGamemodeComponent mode, int left)
	{
		bool alert;
		string detail;

		CRF_ETankBattleState state = mode.GetBattleState();
		if (state == CRF_ETankBattleState.WAITING)
		{
			detail = "WAITING FOR ROUND START";
		}
		else if (state == CRF_ETankBattleState.ENDED)
		{
			int winner = mode.GetWinnerSide();
			if (winner >= 0)
				detail = GetSideName(mode, winner) + " WINS";
			else
				detail = "ROUND OVER";

			alert = true;
		}
		else
		{
			detail = GetPointDetail(mode, left);
			alert = true;
		}

		m_wDetail.SetVisible(!detail.IsEmpty());
		m_wDetail.SetText(detail);
		if (alert)
			m_wDetail.SetColor(ALERT_COLOR);
		else
			m_wDetail.SetColor(DETAIL_COLOR);
	}

	//------------------------------------------------------------------------------------------------
	//! What the point the local tank sits in is doing ("CAPTURING B 45%"), empty outside a point or
	//! while the side already holds it
	protected string GetPointDetail(notnull CRF_TankBattleGamemodeComponent mode, int left)
	{
		Vehicle tank = mode.GetLocalTank();
		if (!tank)
			return "";

		int point = mode.GetPointAt(tank.GetOrigin());
		if (point < 0)
			return "";

		string name = mode.GetPointName(point);
		if (mode.GetPointCapturing(point) == CRF_TankBattleGamemodeComponent.CONTESTED)
			return "CONTESTED " + name;

		if (mode.GetPointOwner(point) == left)
			return "";

		// -100 (the enemy holds it) to 100 (we do)
		int progress = mode.GetPointProgress(point);
		if (left == 1)
			progress = -progress;

		if (progress < 0)
		{
			int neutralised = 100 + progress;
			return "NEUTRALISING " + name + " " + neutralised.ToString() + "%";
		}

		return "CAPTURING " + name + " " + progress.ToString() + "%";
	}

	//------------------------------------------------------------------------------------------------
	//! Slotted players (alive or spectating), with no menu in front of the game other than the spectator's
	protected bool ShouldShow()
	{
		if (CRF_SpecialsHelper.GetLocalFactionKey().IsEmpty())
			return false;

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		return topMenu == null || COA_SpectatorMenu.Cast(topMenu) != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Create()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		m_wRoot = workspace.CreateWidgets(LAYOUT);
		if (!m_wRoot)
			return false;

		m_wSide0 = TextWidget.Cast(m_wRoot.FindAnyWidget("Side0"));
		m_wSide1 = TextWidget.Cast(m_wRoot.FindAnyWidget("Side1"));
		m_wBar0 = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("Bar0"));
		m_wBar1 = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("Bar1"));
		m_wDetail = TextWidget.Cast(m_wRoot.FindAnyWidget("Detail"));
		if (!m_wSide0 || !m_wSide1 || !m_wBar0 || !m_wBar1 || !m_wDetail)
		{
			Destroy();
			return false;
		}

		m_wCrosshair = m_wRoot.FindAnyWidget("Crosshair");
		if (m_wCrosshair)
			m_wCrosshair.SetVisible(false);

		for (int m = 0; m < MAX_POINTS; m++)
		{
			string prefix = "Marker" + m.ToString();
			Widget marker = m_wRoot.FindAnyWidget(prefix);
			ImageWidget markerBack = ImageWidget.Cast(m_wRoot.FindAnyWidget(prefix + "Back"));
			TextWidget markerText = TextWidget.Cast(m_wRoot.FindAnyWidget(prefix + "Text"));
			ProgressBarWidget markerBar = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget(prefix + "Bar"));
			TextWidget markerInfo = TextWidget.Cast(m_wRoot.FindAnyWidget(prefix + "Info"));
			if (!marker || !markerBack || !markerText || !markerBar || !markerInfo)
				break;

			marker.SetVisible(false);
			m_aMarkers.Insert(marker);
			m_aMarkerBacks.Insert(markerBack);
			m_aMarkerTexts.Insert(markerText);
			m_aMarkerBars.Insert(markerBar);
			m_aMarkerInfos.Insert(markerInfo);
			m_aMarkerColors.Insert(NEUTRAL_COLOR);
			m_aMarkerPulsing.Insert(false);
			m_aMarkerFade.Insert(1);
		}

		m_wMinimapPanel = m_wRoot.FindAnyWidget("MinimapPanel");
		if (m_wMinimapPanel)
		{
			if (!m_Minimap)
				m_Minimap = new CRF_TankBattleMinimap();

			m_Minimap.SetCanvas(CanvasWidget.Cast(m_wMinimapPanel.FindAnyWidget("MinimapCanvas")));
		}

		for (int i = 0; i < MAX_POINTS; i++)
		{
			Widget pointWidget = m_wRoot.FindAnyWidget("Point" + i.ToString());
			ImageWidget back = ImageWidget.Cast(m_wRoot.FindAnyWidget("Point" + i.ToString() + "Back"));
			TextWidget label = TextWidget.Cast(m_wRoot.FindAnyWidget("Point" + i.ToString() + "Text"));
			if (!pointWidget || !back || !label)
				break;

			m_aPointWidgets.Insert(pointWidget);
			m_aPointBacks.Insert(back);
			m_aPointTexts.Insert(label);
		}

		return true;
	}
}
