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
// list). Redrawn every frame.
//------------------------------------------------------------------------------------
class CRF_TankBattleMinimap
{
	protected static const float VIEW_PADDING = 0.12;	// around the points and spawns, as a fraction of their spread
	protected static const float MIN_VIEW_SIZE = 300;	// m
	protected static const float LAYOUT_SIZE = 242;		// MinimapCanvas's height in the layout; sizes below are at that size
	protected static const float GUN_LINE_LENGTH = 22;	// px at layout size

	protected CanvasWidget m_wCanvas;
	protected float m_fCanvasW;
	protected float m_fCanvasH;
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};

	protected bool m_bBuilt;
	protected float m_fViewMinX;
	protected float m_fViewMinZ;
	protected float m_fViewSize;

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
	}

	//------------------------------------------------------------------------------------------------
	//! The canvas to draw on (null while the HUD is down)
	void SetCanvas(CanvasWidget canvas)
	{
		m_wCanvas = canvas;
		m_fCanvasW = 0;
		m_fCanvasH = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Every frame while the HUD is up
	void Update(notnull CRF_TankBattleGamemodeComponent mode)
	{
		if (!m_wCanvas)
			return;

		if (!m_bBuilt && !FitView(mode))
			return;

		float w, h;
		m_wCanvas.GetScreenSize(w, h);
		if (w <= 0 || h <= 0)
			return; // not laid out yet

		m_fCanvasW = w;
		m_fCanvasH = h;
		float ui = m_fCanvasH / LAYOUT_SIZE;

		int localSide = mode.GetLocalSide();
		int viewSide = localSide;
		if (viewSide < 0)
			viewSide = 0;

		m_aCommands.Clear();
		DrawImage(mode);
		DrawPoints(mode, viewSide, ui);
		DrawTanks(mode, localSide, viewSide, ui);
		m_wCanvas.SetDrawCommands(m_aCommands);
	}

	//===================================================================================
	// DRAWING
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! The satellite image under the view square, cropped by UV
	protected void DrawImage(notnull CRF_TankBattleGamemodeComponent mode)
	{
		if (!m_bImageTried)
			m_bImageTried = LoadImage(mode);

		if (!m_Image)
			return;

		float side = m_fViewSize * PixelsPerMetre();
		ImageDrawCommand image = new ImageDrawCommand();
		image.m_pTexture = m_Image;
		image.m_iColor = m_iImageColor;
		image.m_Position = ToPixel(Vector(m_fViewMinX, 0, m_fViewMinZ + m_fViewSize));	// top left = north west
		image.m_Size = Vector(side, side, 0);
		image.m_fUV[0] = m_fImageU0;
		image.m_fUV[1] = m_fImageV0;
		image.m_fUV[2] = m_fImageU1;
		image.m_fUV[3] = m_fImageV1;
		m_aCommands.Insert(image);
	}

	//------------------------------------------------------------------------------------------------
	//! The image and the part of it under the view. The image covers the map's area, north up (V 0 = north).
	//! eturn false to try again next frame (the map entity isn't there yet)
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
	protected void DrawPoints(notnull CRF_TankBattleGamemodeComponent mode, int viewSide, float ui)
	{
		float pixelsPerMetre = PixelsPerMetre();
		for (int i = 0; i < mode.GetPointCount(); i++)
		{
			IEntity pointEntity = mode.GetPointEntity(i);
			if (!pointEntity)
				continue;

			vector centre = ToPixel(pointEntity.GetOrigin());
			float radius = Math.Max(mode.GetPointRadius(i) * pixelsPerMetre, 6 * ui);

			int owner = mode.GetPointOwner(i);
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
			int capturing = mode.GetPointCapturing(i);
			if (capturing == CRF_TankBattleGamemodeComponent.CONTESTED)
				ringColor = m_iContestedColor;
			else if (capturing == viewSide)
				ringColor = m_iOwnColor;
			else if (capturing >= 0)
				ringColor = m_iEnemyColor;

			PolygonDrawCommand fill = new PolygonDrawCommand();
			fill.m_iColor = fillColor;
			fill.m_Vertices = new array<float>();
			m_wCanvas.TessellateCircle(centre, radius, 32, fill.m_Vertices);
			m_aCommands.Insert(fill);

			LineDrawCommand ring = new LineDrawCommand();
			ring.m_iColor = ringColor;
			ring.m_fWidth = 2 * ui;
			ring.m_bShouldEnclose = true;
			ring.m_Vertices = new array<float>();
			m_wCanvas.TessellateCircle(centre, radius, 32, ring.m_Vertices);
			m_aCommands.Insert(ring);

			// The letter, centred on the zone (the text is placed by its top-left corner)
			float size = 16 * ui;
			TextDrawCommand label = new TextDrawCommand();
			label.m_iColor = m_iLabelColor;
			label.m_sText = mode.GetPointName(i);
			label.m_fSize = size;
			label.m_Position = Vector(centre[0] - size * 0.3, centre[1] - size * 0.55, 0);
			m_aCommands.Insert(label);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Friendly tanks as dots (spectators: every tank, in its side's colour), the local tank as an arrow
	protected void DrawTanks(notnull CRF_TankBattleGamemodeComponent mode, int localSide, int viewSide, float ui)
	{
		Vehicle localTank = mode.GetLocalTank();

		array<Vehicle> tanks = {};
		array<int> sides = {};
		mode.GetTanks(tanks, sides);
		foreach (int i, Vehicle tank : tanks)
		{
			if (!tank || tank == localTank)
				continue;

			if (localSide >= 0 && sides[i] != localSide)
				continue;

			int color = m_iOwnColor;
			if (sides[i] != viewSide)
				color = m_iEnemyColor;

			InsertDot(ClampToView(tank.GetOrigin()), 4.5 * ui, color, ui);
		}

		if (!localTank)
			return;

		vector position = ClampToView(localTank.GetOrigin());
		vector gunDirection = mode.GetLocalGunDirection();
		if (gunDirection != vector.Zero)
		{
			vector centre = ToPixel(position);
			float length = GUN_LINE_LENGTH * ui;
			vector flat = Vector(gunDirection[0], 0, gunDirection[2]).Normalized();

			LineDrawCommand gun = new LineDrawCommand();
			gun.m_iColor = m_iSelfColor;
			gun.m_fWidth = 2 * ui;
			gun.m_Vertices = new array<float>();
			gun.m_Vertices.Insert(centre[0]);
			gun.m_Vertices.Insert(centre[1]);
			gun.m_Vertices.Insert(centre[0] + flat[0] * length);
			gun.m_Vertices.Insert(centre[1] - flat[2] * length);
			m_aCommands.Insert(gun);
		}

		InsertArrow(position, localTank.GetAngles()[1], ui);
	}

	//------------------------------------------------------------------------------------------------
	protected void InsertDot(vector world, float radius, int color, float ui)
	{
		vector centre = ToPixel(world);

		PolygonDrawCommand outline = new PolygonDrawCommand();
		outline.m_iColor = m_iOutlineColor;
		outline.m_Vertices = new array<float>();
		m_wCanvas.TessellateCircle(centre, radius + 1.25 * ui, 16, outline.m_Vertices);
		m_aCommands.Insert(outline);

		PolygonDrawCommand dot = new PolygonDrawCommand();
		dot.m_iColor = color;
		dot.m_Vertices = new array<float>();
		m_wCanvas.TessellateCircle(centre, radius, 16, dot.m_Vertices);
		m_aCommands.Insert(dot);
	}

	//------------------------------------------------------------------------------------------------
	//! An arrow-head triangle pointing along yaw (degrees, clockwise from north)
	protected void InsertArrow(vector world, float yaw, float ui)
	{
		vector centre = ToPixel(world);
		float rad = yaw * Math.DEG2RAD;

		// Canvas y points south, so north (yaw 0) is up the screen
		float forwardX = Math.Sin(rad);
		float forwardY = -Math.Cos(rad);
		float rightX = -forwardY;
		float rightY = forwardX;

		float length = 10 * ui;
		float halfWidth = 7 * ui;

		float tipX = centre[0] + forwardX * length;
		float tipY = centre[1] + forwardY * length;
		float backX = centre[0] - forwardX * length * 0.6;
		float backY = centre[1] - forwardY * length * 0.6;

		array<float> vertices = new array<float>();
		vertices.Insert(tipX);
		vertices.Insert(tipY);
		vertices.Insert(backX + rightX * halfWidth);
		vertices.Insert(backY + rightY * halfWidth);
		vertices.Insert(backX - rightX * halfWidth);
		vertices.Insert(backY - rightY * halfWidth);

		LineDrawCommand outline = new LineDrawCommand();
		outline.m_iColor = m_iOutlineColor;
		outline.m_fWidth = 2.5 * ui;
		outline.m_bShouldEnclose = true;
		outline.m_Vertices = new array<float>();
		outline.m_Vertices.Copy(vertices);
		m_aCommands.Insert(outline);

		PolygonDrawCommand shape = new PolygonDrawCommand();
		shape.m_iColor = m_iSelfColor;
		shape.m_Vertices = vertices;
		m_aCommands.Insert(shape);
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
		m_bBuilt = true;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected float PixelsPerMetre()
	{
		return Math.Min(m_fCanvasW, m_fCanvasH) / m_fViewSize;
	}

	//------------------------------------------------------------------------------------------------
	//! World position to canvas pixels: the view square fitted and centred in the canvas, north up
	protected vector ToPixel(vector world)
	{
		float pixelsPerMetre = PixelsPerMetre();
		float offsetX = (m_fCanvasW - m_fViewSize * pixelsPerMetre) * 0.5;
		float offsetY = (m_fCanvasH - m_fViewSize * pixelsPerMetre) * 0.5;

		return Vector(offsetX + (world[0] - m_fViewMinX) * pixelsPerMetre, offsetY + (m_fViewMinZ + m_fViewSize - world[2]) * pixelsPerMetre, 0);
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
