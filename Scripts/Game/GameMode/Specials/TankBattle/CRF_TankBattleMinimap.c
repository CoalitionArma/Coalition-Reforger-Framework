//------------------------------------------------------------------------------------
// CRF_TankBattleMinimap: the battlefield minimap on the Tank Battle HUD (bottom right)
//
// After CRF_PayloadMinimap. North up, fitted once to the capture points and the tank spawns. The
// capture zones are drawn to scale, filled in the colour of the side holding them with their letter
// on top, outlined in the colour of the side taking them (yellow while contested). Friendly tanks
// are dots; the local tank is a white arrow along its hull with a short line where its gun points.
// Enemies aren't shown. Under it all, the map's satellite image cropped to the view (the world map's
// "Satellite background image", which covers the terrain: SCR_MapEntity Offset/Size).
//
// Client only, from what CRF_TankBattleGamemodeComponent already replicates (point state, the tank
// list, the server's friendly positions).
//
// Cost: the draw commands are built once and then edited in place (the canvas keeps a pointer to
// m_aCommands, so nothing is allocated per frame).
//  - When the canvas changes size: the image and the zones' geometry.
//  - Every REFRESH_MS: the zones' colours and the friendly dots (the only loops; the server's
//    positions arrive once a second, and at this map's scale a tank moves about half a pixel in
//    REFRESH_MS).
//  - Every frame: the local tank's arrow and gun line, a fixed handful of vertices, and only when
//    they have moved.
//------------------------------------------------------------------------------------
class CRF_TankBattleMinimap
{
	protected static const float VIEW_PADDING = 0.12;	// around the points and spawns, as a fraction of their spread
	protected static const float MIN_VIEW_SIZE = 300;	// m
	protected static const float LAYOUT_SIZE = 242;		// MinimapCanvas's height in the layout; sizes below are at that size
	protected static const float GUN_LINE_LENGTH = 22;	// px at layout size
	protected static const float DOT_RADIUS = 4.5;		// px at layout size
	protected static const int REFRESH_MS = 200;		// zones' colours and friendly dots
	protected static const int ZONE_SEGMENTS = 32;
	protected static const int DOT_SEGMENTS = 16;
	protected static const float SELF_MOVE_PX = 0.25;	// the arrow is redrawn once it has moved this far...
	protected static const float SELF_TURN = 0.01;		// ...or its hull or gun direction has changed this much (about half a degree)

	protected CanvasWidget m_wCanvas;
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};	//!< what the canvas draws: it keeps a pointer to this array

	// The view square, fitted once
	protected bool m_bViewFitted;
	protected float m_fViewMinX;
	protected float m_fViewMinZ;
	protected float m_fViewSize;

	// World to canvas pixels, recomputed when the canvas changes size
	protected bool m_bLayoutDirty = true;
	protected float m_fCanvasW;
	protected float m_fCanvasH;
	protected float m_fPixelsPerMetre;
	protected float m_fOffsetX;
	protected float m_fOffsetY;
	protected float m_fUi = 1;

	// The commands, kept and edited in place
	protected ref ImageDrawCommand m_ImageCommand;
	protected ref array<int> m_aZonePoints = {};			//!< the point index of each zone below
	protected ref array<ref PolygonDrawCommand> m_aZoneFills = {};
	protected ref array<ref LineDrawCommand> m_aZoneRings = {};
	protected ref array<ref TextDrawCommand> m_aZoneLabels = {};
	protected ref array<ref PolygonDrawCommand> m_aDotOutlines = {};	//!< a pool: the first m_iDotsShown are drawn
	protected ref array<ref PolygonDrawCommand> m_aDotFills = {};
	protected int m_iDotsShown = -1;
	protected ref LineDrawCommand m_GunLine;
	protected ref LineDrawCommand m_ArrowOutline;
	protected ref PolygonDrawCommand m_ArrowFill;			//!< shares m_ArrowOutline's vertices

	// Timing and the arrow as last drawn
	protected int m_iRefreshAtMs;
	protected int m_iLastUpdateMs = -100000;
	protected bool m_bSelfShown;
	protected bool m_bGunShown;
	protected float m_fSelfX;
	protected float m_fSelfY;
	protected float m_fSelfForwardX;
	protected float m_fSelfForwardY;
	protected float m_fGunX;
	protected float m_fGunZ;

	// Scratch for GetTankPositions, reused
	protected ref array<vector> m_aTankPositions = {};
	protected ref array<int> m_aTankSides = {};

	protected int m_iOwnColor;
	protected int m_iEnemyColor;
	protected int m_iNeutralColor;
	protected int m_iOwnFill;
	protected int m_iEnemyFill;
	protected int m_iNeutralFill;
	protected int m_iContestedColor;
	protected int m_iSelfColor;
	protected int m_iOutlineColor;
	protected int m_iLabelColor;
	protected int m_iImageColor;
	protected static const int HIDDEN_COLOR = 0;	// fully transparent

	// The satellite image, cropped to the view
	protected ref SharedItemRef m_Image;
	protected bool m_bImageTried;
	protected float m_fImageU0;
	protected float m_fImageV0;
	protected float m_fImageU1;
	protected float m_fImageV1;

	//------------------------------------------------------------------------------------------------
	void CRF_TankBattleMinimap()
	{
		// Designer colours are sRGB; widget colours are linear (Color.FromSRGBA converts)
		m_iOwnColor       = Color.FromSRGBA(60, 140, 255, 255).PackToInt();
		m_iEnemyColor     = Color.FromSRGBA(225, 45, 35, 255).PackToInt();
		m_iNeutralColor   = Color.FromSRGBA(170, 176, 190, 255).PackToInt();
		m_iOwnFill        = Color.FromSRGBA(60, 140, 255, 90).PackToInt();
		m_iEnemyFill      = Color.FromSRGBA(225, 45, 35, 90).PackToInt();
		m_iNeutralFill    = Color.FromSRGBA(120, 126, 140, 70).PackToInt();
		m_iContestedColor = Color.FromSRGBA(255, 200, 90, 255).PackToInt();
		m_iSelfColor      = Color.FromSRGBA(255, 255, 255, 255).PackToInt();
		m_iOutlineColor   = Color.FromSRGBA(11, 12, 16, 255).PackToInt();
		m_iLabelColor     = Color.FromSRGBA(240, 244, 250, 255).PackToInt();
		m_iImageColor     = Color.FromSRGBA(185, 185, 185, 255).PackToInt();	// dimmed, so the zones and tanks stand out

		// Fixed vertex arrays (zeroes), written in place by UpdateSelf
		m_GunLine = new LineDrawCommand();
		m_GunLine.m_Vertices = new array<float>();
		m_GunLine.m_Vertices.Resize(4);

		m_ArrowOutline = new LineDrawCommand();
		m_ArrowOutline.m_bShouldEnclose = true;
		m_ArrowOutline.m_Vertices = new array<float>();
		m_ArrowOutline.m_Vertices.Resize(6);

		m_ArrowFill = new PolygonDrawCommand();
		m_ArrowFill.m_Vertices = m_ArrowOutline.m_Vertices;

		HideSelf();
	}

	//------------------------------------------------------------------------------------------------
	//! The canvas to draw on (null while the HUD is down)
	void SetCanvas(CanvasWidget canvas)
	{
		m_wCanvas = canvas;
		m_fCanvasW = 0;
		m_fCanvasH = 0;
		m_bLayoutDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame while the minimap is up (see the header for what each step costs)
	void Update(notnull CRF_TankBattleGamemodeComponent mode)
	{
		if (!m_wCanvas)
			return;

		if (!m_bViewFitted && !FitView(mode))
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float w, h;
		m_wCanvas.GetScreenSize(w, h);
		if (w <= 0 || h <= 0)
			return; // not laid out yet

		if (w != m_fCanvasW || h != m_fCanvasH)
		{
			m_fCanvasW = w;
			m_fCanvasH = h;
			m_bLayoutDirty = true;
		}

		// Shown again after being hidden: refresh at once rather than show where the tanks were
		int now = world.GetWorldTime();
		if (now - m_iLastUpdateMs > REFRESH_MS * 2)
			m_iRefreshAtMs = now;

		m_iLastUpdateMs = now;

		bool rebuildList;
		bool changed;
		if (m_bLayoutDirty)
		{
			BuildLayout(mode);
			m_iRefreshAtMs = now;
			rebuildList = true;
		}

		if (now >= m_iRefreshAtMs)
		{
			m_iRefreshAtMs = now + REFRESH_MS;
			if (Refresh(mode))
				rebuildList = true;

			changed = true;
		}

		if (UpdateSelf(mode, rebuildList))
			changed = true;

		if (rebuildList)
			AssembleCommands();
		else if (changed)
			m_wCanvas.SetDrawCommands(m_aCommands);
	}

	//===================================================================================
	// BUILDING: when the canvas changes size
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	protected void BuildLayout(notnull CRF_TankBattleGamemodeComponent mode)
	{
		m_bLayoutDirty = false;
		m_fPixelsPerMetre = Math.Min(m_fCanvasW, m_fCanvasH) / m_fViewSize;
		m_fOffsetX = (m_fCanvasW - m_fViewSize * m_fPixelsPerMetre) * 0.5;
		m_fOffsetY = (m_fCanvasH - m_fViewSize * m_fPixelsPerMetre) * 0.5;
		m_fUi = m_fCanvasH / LAYOUT_SIZE;

		BuildImage(mode);
		BuildZones(mode);

		m_GunLine.m_fWidth = 2 * m_fUi;
		m_ArrowOutline.m_fWidth = 2.5 * m_fUi;
	}

	//------------------------------------------------------------------------------------------------
	//! The satellite image under the view square, cropped by UV
	protected void BuildImage(notnull CRF_TankBattleGamemodeComponent mode)
	{
		if (!m_bImageTried)
			m_bImageTried = LoadImage(mode);

		if (!m_Image)
		{
			m_ImageCommand = null;
			return;
		}

		if (!m_ImageCommand)
			m_ImageCommand = new ImageDrawCommand();

		float side = m_fViewSize * m_fPixelsPerMetre;
		m_ImageCommand.m_pTexture = m_Image;
		m_ImageCommand.m_iColor = m_iImageColor;
		m_ImageCommand.m_Position = ToPixel(Vector(m_fViewMinX, 0, m_fViewMinZ + m_fViewSize));	// top left = north west
		m_ImageCommand.m_Size = Vector(side, side, 0);
		m_ImageCommand.m_fUV[0] = m_fImageU0;
		m_ImageCommand.m_fUV[1] = m_fImageV0;
		m_ImageCommand.m_fUV[2] = m_fImageU1;
		m_ImageCommand.m_fUV[3] = m_fImageV1;
	}

	//------------------------------------------------------------------------------------------------
	//! The image and the part of it under the view. The image covers the map's area, north up (V 0 = north).
	//! returns false to try again later (the map entity isn't there yet)
	protected bool LoadImage(notnull CRF_TankBattleGamemodeComponent mode)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return false;

		ResourceName resource = mode.GetMinimapImage();
		if (resource.IsEmpty())
		{
			// Maps that set the image on their placed map entity (Chernarus Minus) rather than its prefab
			if (mode.ShowMinimapSatellite())
				Print("[CRF_TankBattle] No minimap satellite image: this map's map entity prefab has none. Set Minimap Image on the mode to the map's satellite .edds.", LogLevel.WARNING);

			return true;
		}

		vector offset = mapEntity.Offset();
		vector size = mapEntity.Size();
		if (size[0] <= 0 || size[2] <= 0)
			return true;

		m_Image = CanvasWidget.LoadTexture(resource);
		if (!m_Image)
		{
			Print(string.Format("[CRF_TankBattle] Minimap image '%1' didn't load.", resource), LogLevel.WARNING);
			return true;
		}

		m_fImageU0 = (m_fViewMinX - offset[0]) / size[0];
		m_fImageU1 = (m_fViewMinX + m_fViewSize - offset[0]) / size[0];
		m_fImageV0 = 1 - (m_fViewMinZ + m_fViewSize - offset[2]) / size[2];
		m_fImageV1 = 1 - (m_fViewMinZ - offset[2]) / size[2];
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Each zone's circle and letter; their colours are set by Refresh
	protected void BuildZones(notnull CRF_TankBattleGamemodeComponent mode)
	{
		m_aZonePoints.Clear();
		m_aZoneFills.Clear();
		m_aZoneRings.Clear();
		m_aZoneLabels.Clear();

		for (int i = 0; i < mode.GetPointCount(); i++)
		{
			IEntity pointEntity = mode.GetPointEntity(i);
			if (!pointEntity)
				continue;

			vector centre = ToPixel(pointEntity.GetOrigin());
			float radius = Math.Max(mode.GetPointRadius(i) * m_fPixelsPerMetre, 6 * m_fUi);

			PolygonDrawCommand fill = new PolygonDrawCommand();
			fill.m_Vertices = new array<float>();
			m_wCanvas.TessellateCircle(centre, radius, ZONE_SEGMENTS, fill.m_Vertices);

			LineDrawCommand ring = new LineDrawCommand();
			ring.m_fWidth = 2 * m_fUi;
			ring.m_bShouldEnclose = true;
			ring.m_Vertices = fill.m_Vertices;

			// The letter, centred on the zone (the text is placed by its top-left corner)
			float size = 16 * m_fUi;
			TextDrawCommand label = new TextDrawCommand();
			label.m_iColor = m_iLabelColor;
			label.m_sText = mode.GetPointName(i);
			label.m_fSize = size;
			label.m_Position = Vector(centre[0] - size * 0.3, centre[1] - size * 0.55, 0);

			m_aZonePoints.Insert(i);
			m_aZoneFills.Insert(fill);
			m_aZoneRings.Insert(ring);
			m_aZoneLabels.Insert(label);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The canvas's list, grouped by command type so the canvas can batch them: image, zone fills, zone
	//! rings, letters, dot outlines, dots, the gun line, the arrow
	protected void AssembleCommands()
	{
		m_aCommands.Clear();
		if (m_ImageCommand)
			m_aCommands.Insert(m_ImageCommand);

		foreach (PolygonDrawCommand fill : m_aZoneFills)
		{
			m_aCommands.Insert(fill);
		}

		foreach (LineDrawCommand ring : m_aZoneRings)
		{
			m_aCommands.Insert(ring);
		}

		foreach (TextDrawCommand label : m_aZoneLabels)
		{
			m_aCommands.Insert(label);
		}

		for (int i = 0; i < m_iDotsShown; i++)
		{
			m_aCommands.Insert(m_aDotOutlines[i]);
		}

		for (int i = 0; i < m_iDotsShown; i++)
		{
			m_aCommands.Insert(m_aDotFills[i]);
		}

		m_aCommands.Insert(m_GunLine);
		m_aCommands.Insert(m_ArrowOutline);
		m_aCommands.Insert(m_ArrowFill);
		m_wCanvas.SetDrawCommands(m_aCommands);
	}

	//===================================================================================
	// REFRESH: every REFRESH_MS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Zone colours and friendly dots, in place. returns true if the list needs reassembling (the
	//! number of dots changed)
	protected bool Refresh(notnull CRF_TankBattleGamemodeComponent mode)
	{
		// Pieces that weren't there at the last build (the map entity, a point not streamed in yet)
		if (!m_bImageTried || m_aZonePoints.Count() < mode.GetPointCount())
			m_bLayoutDirty = true;

		int localSide = mode.GetLocalSide();
		int viewSide = localSide;
		if (viewSide < 0)
			viewSide = 0;

		RefreshZones(mode, viewSide);
		return RefreshDots(mode, localSide, viewSide);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshZones(notnull CRF_TankBattleGamemodeComponent mode, int viewSide)
	{
		foreach (int z, int point : m_aZonePoints)
		{
			int owner = mode.GetPointOwner(point);
			int fillColor = m_iNeutralFill;
			int ringColor = m_iNeutralColor;
			if (owner == viewSide)
			{
				fillColor = m_iOwnFill;
				ringColor = m_iOwnColor;
			}
			else if (owner >= 0)
			{
				fillColor = m_iEnemyFill;
				ringColor = m_iEnemyColor;
			}

			// The ring shows who is taking it
			int capturing = mode.GetPointCapturing(point);
			if (capturing == CRF_TankBattleGamemodeComponent.CONTESTED)
				ringColor = m_iContestedColor;
			else if (capturing == viewSide)
				ringColor = m_iOwnColor;
			else if (capturing >= 0)
				ringColor = m_iEnemyColor;

			m_aZoneFills[z].m_iColor = fillColor;
			m_aZoneRings[z].m_iColor = ringColor;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Friendly tanks as dots (spectators: every tank, in its side's colour). Positions, not entities: a
	//! tank outside this client's network view distance isn't streamed in, so its position is the
	//! server's (GetTankPositions). returns true if the number of dots changed
	protected bool RefreshDots(notnull CRF_TankBattleGamemodeComponent mode, int localSide, int viewSide)
	{
		m_aTankPositions.Clear();
		m_aTankSides.Clear();
		mode.GetTankPositions(m_aTankPositions, m_aTankSides, mode.GetLocalTank());

		float radius = DOT_RADIUS * m_fUi;
		float outlineRadius = radius + 1.25 * m_fUi;
		int shown;
		foreach (int i, vector tankPosition : m_aTankPositions)
		{
			int side = m_aTankSides[i];
			if (localSide >= 0 && side != localSide)
				continue;

			int color = m_iOwnColor;
			if (side != viewSide)
				color = m_iEnemyColor;

			if (shown >= m_aDotFills.Count())
			{
				PolygonDrawCommand newOutline = new PolygonDrawCommand();
				newOutline.m_iColor = m_iOutlineColor;
				newOutline.m_Vertices = new array<float>();
				m_aDotOutlines.Insert(newOutline);

				PolygonDrawCommand newFill = new PolygonDrawCommand();
				newFill.m_Vertices = new array<float>();
				m_aDotFills.Insert(newFill);
			}

			vector centre = ToPixel(ClampToView(tankPosition));
			PolygonDrawCommand outline = m_aDotOutlines[shown];
			outline.m_Vertices.Clear();
			m_wCanvas.TessellateCircle(centre, outlineRadius, DOT_SEGMENTS, outline.m_Vertices);

			PolygonDrawCommand fill = m_aDotFills[shown];
			fill.m_iColor = color;
			fill.m_Vertices.Clear();
			m_wCanvas.TessellateCircle(centre, radius, DOT_SEGMENTS, fill.m_Vertices);
			shown++;
		}

		if (shown == m_iDotsShown)
			return false;

		m_iDotsShown = shown;
		return true;
	}

	//===================================================================================
	// THE LOCAL TANK: every frame, no loops
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! The arrow along the local tank's hull and the line along its gun, written into their fixed
	//! vertices. returns true if anything changed (force: redraw even if it hasn't moved)
	protected bool UpdateSelf(notnull CRF_TankBattleGamemodeComponent mode, bool force)
	{
		Vehicle localTank = mode.GetLocalTank();
		if (!localTank)
		{
			if (!m_bSelfShown)
				return false;

			HideSelf();
			return true;
		}

		vector centre = ToPixel(ClampToView(localTank.GetOrigin()));
		float rad = localTank.GetAngles()[1] * Math.DEG2RAD;

		// Canvas y points south, so north (yaw 0) is up the screen
		float forwardX = Math.Sin(rad);
		float forwardY = -Math.Cos(rad);

		vector gunDirection = mode.GetLocalGunDirection();
		bool gunShown = gunDirection[0] != 0 || gunDirection[2] != 0;
		float gunX;
		float gunZ;
		if (gunShown)
		{
			float flat = Math.Sqrt(gunDirection[0] * gunDirection[0] + gunDirection[2] * gunDirection[2]);
			gunX = gunDirection[0] / flat;
			gunZ = gunDirection[2] / flat;
		}

		if (!force && m_bSelfShown && gunShown == m_bGunShown
			&& Math.AbsFloat(centre[0] - m_fSelfX) < SELF_MOVE_PX && Math.AbsFloat(centre[1] - m_fSelfY) < SELF_MOVE_PX
			&& Math.AbsFloat(forwardX - m_fSelfForwardX) < SELF_TURN && Math.AbsFloat(forwardY - m_fSelfForwardY) < SELF_TURN
			&& Math.AbsFloat(gunX - m_fGunX) < SELF_TURN && Math.AbsFloat(gunZ - m_fGunZ) < SELF_TURN)
			return false;

		m_bSelfShown = true;
		m_bGunShown = gunShown;
		m_fSelfX = centre[0];
		m_fSelfY = centre[1];
		m_fSelfForwardX = forwardX;
		m_fSelfForwardY = forwardY;
		m_fGunX = gunX;
		m_fGunZ = gunZ;

		// The gun line
		array<float> gun = m_GunLine.m_Vertices;
		float length = GUN_LINE_LENGTH * m_fUi;
		gun[0] = centre[0];
		gun[1] = centre[1];
		gun[2] = centre[0] + gunX * length;
		gun[3] = centre[1] - gunZ * length;
		if (gunShown)
			m_GunLine.m_iColor = m_iSelfColor;
		else
			m_GunLine.m_iColor = HIDDEN_COLOR;

		// The arrow head: a triangle pointing along the hull
		float rightX = -forwardY;
		float rightY = forwardX;
		float arrowLength = 10 * m_fUi;
		float halfWidth = 7 * m_fUi;
		float backX = centre[0] - forwardX * arrowLength * 0.6;
		float backY = centre[1] - forwardY * arrowLength * 0.6;

		array<float> arrow = m_ArrowOutline.m_Vertices;
		arrow[0] = centre[0] + forwardX * arrowLength;
		arrow[1] = centre[1] + forwardY * arrowLength;
		arrow[2] = backX + rightX * halfWidth;
		arrow[3] = backY + rightY * halfWidth;
		arrow[4] = backX - rightX * halfWidth;
		arrow[5] = backY - rightY * halfWidth;
		m_ArrowOutline.m_iColor = m_iOutlineColor;
		m_ArrowFill.m_iColor = m_iSelfColor;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void HideSelf()
	{
		m_bSelfShown = false;
		m_GunLine.m_iColor = HIDDEN_COLOR;
		m_ArrowOutline.m_iColor = HIDDEN_COLOR;
		m_ArrowFill.m_iColor = HIDDEN_COLOR;
	}

	//===================================================================================
	// VIEW
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! A square around the capture zones and the tank spawns. False until the points are found.
	protected bool FitView(notnull CRF_TankBattleGamemodeComponent mode)
	{
		array<vector> positions = {};
		array<float> radii = {};
		for (int i = 0; i < mode.GetPointCount(); i++)
		{
			IEntity pointEntity = mode.GetPointEntity(i);
			if (!pointEntity)
				continue;

			positions.Insert(pointEntity.GetOrigin());
			radii.Insert(mode.GetPointRadius(i));
		}

		if (positions.IsEmpty())
			return false;

		mode.GetSpawnPositions(positions);
		while (radii.Count() < positions.Count())
		{
			radii.Insert(15);
		}

		vector first = positions[0];
		float minX = first[0];
		float maxX = minX;
		float minZ = first[2];
		float maxZ = minZ;
		foreach (int i, vector position : positions)
		{
			minX = Math.Min(minX, position[0] - radii[i]);
			maxX = Math.Max(maxX, position[0] + radii[i]);
			minZ = Math.Min(minZ, position[2] - radii[i]);
			maxZ = Math.Max(maxZ, position[2] + radii[i]);
		}

		float size = Math.Max(maxX - minX, maxZ - minZ);
		size = Math.Max(MIN_VIEW_SIZE, size * (1 + 2 * VIEW_PADDING));

		m_fViewSize = size;
		m_fViewMinX = (minX + maxX) * 0.5 - size * 0.5;
		m_fViewMinZ = (minZ + maxZ) * 0.5 - size * 0.5;
		m_bViewFitted = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! World position to canvas pixels: the view square fitted and centred in the canvas, north up
	protected vector ToPixel(vector world)
	{
		return Vector(m_fOffsetX + (world[0] - m_fViewMinX) * m_fPixelsPerMetre, m_fOffsetY + (m_fViewMinZ + m_fViewSize - world[2]) * m_fPixelsPerMetre, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps a tank on the map's edge when it is outside the view (the canvas doesn't clip)
	protected vector ClampToView(vector world)
	{
		world[0] = Math.Clamp(world[0], m_fViewMinX, m_fViewMinX + m_fViewSize);
		world[2] = Math.Clamp(world[2], m_fViewMinZ, m_fViewMinZ + m_fViewSize);
		return world;
	}
}
