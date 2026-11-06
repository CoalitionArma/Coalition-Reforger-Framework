//------------------------------------------------------------------------------------
// CRF_PayloadMinimap: the route minimap on the payload HUD (top right)
//
// Copied from CRF_RallyMinimap. The whole route, north up, fitted to a square: the roads around it
// faint, the route over them (orange behind the truck, grey ahead of it), the start in green, the
// finish in red, the checkpoints (dimmed once reached, a ring on the next one), the truck, and the
// local player as an arrow pointing the way they face.
//
// Client side only, built from the route every machine already reads from the world, and the truck's
// distance along it that CRF_PayloadGamemodeComponent replicates. Nothing extra goes over the network.
// The route is the mission's payload_route shape, so unlike the rally there are no roads to search.
//------------------------------------------------------------------------------------
class CRF_PayloadMinimap
{
	protected static const float ROAD_MARGIN = 300;       // roads are gathered this far around the route (m)
	protected static const float VIEW_PADDING = 0.08;     // around the route, as a fraction of its size
	protected static const float MIN_VIEW_SIZE = 300;     // m
	protected static const float LAYOUT_SIZE = 284;       // MinimapCanvas's size in the layout; drawing sizes below are at that size
	protected static const float MIN_VERTEX_SPACING = 1.5; // px - closer points are dropped

	protected CanvasWidget m_wCanvas;
	protected float m_fCanvasW;
	protected float m_fCanvasH;
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};

	// World data, built once
	protected bool m_bBuilt;
	protected ref array<vector> m_aRoute = {};
	protected ref array<float> m_aRouteDistances = {};
	protected float m_fRouteLength;
	protected ref array<vector> m_aCheckpoints = {};
	protected ref array<ref array<vector>> m_aRoads = {};
	protected float m_fViewMinX;
	protected float m_fViewMinZ;
	protected float m_fViewSize;

	// The parts of the drawing that change with every update
	protected ref LineDrawCommand m_DoneLine;
	protected ref LineDrawCommand m_AheadLine;
	protected ref array<ref PolygonDrawCommand> m_aCheckpointDots = {};
	protected ref LineDrawCommand m_NextRing;
	protected ref PolygonDrawCommand m_TruckShape;
	protected ref LineDrawCommand m_TruckOutline;
	protected ref PolygonDrawCommand m_PlayerShape;
	protected ref LineDrawCommand m_PlayerOutline;

	protected int m_iRoadColor;
	protected int m_iAheadColor;
	protected int m_iDoneColor;
	protected int m_iCheckpointColor;
	protected int m_iReachedColor;
	protected int m_iStartColor;
	protected int m_iFinishColor;
	protected int m_iTruckColor;
	protected int m_iPlayerColor;
	protected int m_iOutlineColor;
	protected int m_iHiddenColor;

	//------------------------------------------------------------------------------------------------
	void CRF_PayloadMinimap()
	{
		// Designer colours are sRGB; widget colours are linear (Color.FromSRGBA converts)
		m_iRoadColor       = Color.FromSRGBA(58, 64, 78, 255).PackToInt();
		m_iAheadColor      = Color.FromSRGBA(150, 158, 174, 255).PackToInt();
		m_iDoneColor       = Color.FromSRGBA(255, 165, 0, 255).PackToInt();    // the truck's map marker colour
		m_iCheckpointColor = Color.FromSRGBA(232, 236, 244, 255).PackToInt();
		m_iReachedColor    = Color.FromSRGBA(96, 104, 120, 255).PackToInt();
		m_iStartColor      = Color.FromSRGBA(92, 196, 128, 255).PackToInt();
		m_iFinishColor     = Color.FromSRGBA(232, 80, 80, 255).PackToInt();
		m_iTruckColor      = Color.FromSRGBA(255, 165, 0, 255).PackToInt();
		m_iPlayerColor     = Color.FromSRGBA(255, 255, 255, 255).PackToInt();
		m_iOutlineColor    = Color.FromSRGBA(11, 12, 16, 255).PackToInt();
		m_iHiddenColor     = Color.FromSRGBA(0, 0, 0, 0).PackToInt();
	}

	//------------------------------------------------------------------------------------------------
	bool IsBuilt()
	{
		return m_bBuilt;
	}

	//------------------------------------------------------------------------------------------------
	//! The canvas to draw on (null while the HUD is down). The drawing is redone for the new canvas.
	void SetCanvas(CanvasWidget canvas)
	{
		m_wCanvas = canvas;
		m_fCanvasW = 0;
		m_fCanvasH = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Frames the route and gathers the roads around it. Once per mission.
	//! \param[in] route The route's points in world space, start first
	//! \param[in] routeDistances How far along the route each point is
	//! \param[in] checkpointDistances How far along the route each checkpoint is, in order
	void Build(notnull array<vector> route, notnull array<float> routeDistances, notnull array<float> checkpointDistances)
	{
		m_bBuilt = true;
		m_aRoute.Copy(route);
		m_aRouteDistances.Copy(routeDistances);
		m_aCheckpoints.Clear();
		m_aRoads.Clear();

		if (m_aRoute.Count() < 2 || m_aRouteDistances.Count() != m_aRoute.Count())
		{
			m_aRoute.Clear();
			return;
		}

		m_fRouteLength = m_aRouteDistances[m_aRouteDistances.Count() - 1];
		foreach (float distance : checkpointDistances)
		{
			m_aCheckpoints.Insert(GetRoutePoint(distance));
		}

		FitView();
		GatherRoads();
		ClipRoadsToView();
	}

	//------------------------------------------------------------------------------------------------
	//! Redraws the dynamic parts. Call at the HUD rate.
	//! \param[in] truckDistance How far along the route the truck is
	//! \param[in] checkpointsReached How many checkpoints it has reached
	//! \param[in] showPlayer Draw the local player (false for spectators and the dead)
	//! \param[in] playerPos The local player's vehicle (or character) position
	//! \param[in] playerYaw Its heading in degrees, clockwise from north
	void Update(float truckDistance, int checkpointsReached, bool showPlayer, vector playerPos, float playerYaw)
	{
		if (!m_wCanvas || !m_bBuilt || m_aRoute.IsEmpty())
			return;

		float w, h;
		m_wCanvas.GetScreenSize(w, h);
		if (w <= 0 || h <= 0)
			return; // not laid out yet

		if (w != m_fCanvasW || h != m_fCanvasH)
		{
			m_fCanvasW = w;
			m_fCanvasH = h;
			BuildDrawing();
		}

		float ui = m_fCanvasH / LAYOUT_SIZE;
		truckDistance = Math.Clamp(truckDistance, 0, m_fRouteLength);
		vector truckPos = GetRoutePoint(truckDistance);

		// The route split at the truck: the stretch behind it is how far it has come
		m_DoneLine.m_Vertices.Clear();
		m_AheadLine.m_Vertices.Clear();
		array<vector> done = {};
		array<vector> ahead = {};
		done.Insert(m_aRoute[0]);
		foreach (int i, vector point : m_aRoute)
		{
			if (i == 0)
				continue;

			if (m_aRouteDistances[i] < truckDistance)
				done.Insert(point);
			else
				ahead.Insert(point);
		}
		done.Insert(truckPos);
		ahead.InsertAt(truckPos, 0);
		ToVertices(done, m_DoneLine.m_Vertices);
		ToVertices(ahead, m_AheadLine.m_Vertices);
		m_DoneLine.m_fWidth = 3.5 * ui;
		m_AheadLine.m_fWidth = 2.5 * ui;

		foreach (int cpIndex, PolygonDrawCommand dot : m_aCheckpointDots)
		{
			if (cpIndex < checkpointsReached)
				dot.m_iColor = m_iReachedColor;
			else
				dot.m_iColor = m_iCheckpointColor;
		}

		m_NextRing.m_Vertices.Clear();
		if (m_aCheckpoints.IsIndexValid(checkpointsReached))
		{
			m_NextRing.m_iColor = m_iTruckColor;
			m_wCanvas.TessellateCircle(ToPixel(m_aCheckpoints[checkpointsReached]), 8 * ui, 20, m_NextRing.m_Vertices);
		}
		else
		{
			m_NextRing.m_iColor = m_iHiddenColor;
		}

		SetTruckVertices(truckPos, ui);

		m_PlayerShape.m_Vertices.Clear();
		m_PlayerOutline.m_Vertices.Clear();
		if (showPlayer)
		{
			m_PlayerShape.m_iColor = m_iPlayerColor;
			m_PlayerOutline.m_iColor = m_iOutlineColor;
			SetPlayerVertices(ClampToView(playerPos), playerYaw, ui);
		}
		else
		{
			m_PlayerShape.m_iColor = m_iHiddenColor;
			m_PlayerOutline.m_iColor = m_iHiddenColor;
		}

		m_wCanvas.SetDrawCommands(m_aCommands);
	}

	//===================================================================================
	// DRAWING
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! The static drawing (roads, start, finish, checkpoint outlines) for the current canvas size, plus
	//! the commands Update fills in
	protected void BuildDrawing()
	{
		m_aCommands.Clear();
		m_aCheckpointDots.Clear();
		float ui = m_fCanvasH / LAYOUT_SIZE;

		foreach (array<vector> road : m_aRoads)
		{
			LineDrawCommand roadLine = new LineDrawCommand();
			roadLine.m_iColor = m_iRoadColor;
			roadLine.m_fWidth = 1.5 * ui;
			roadLine.m_Vertices = new array<float>();
			ToVertices(road, roadLine.m_Vertices);
			m_aCommands.Insert(roadLine);
		}

		m_AheadLine = new LineDrawCommand();
		m_AheadLine.m_iColor = m_iAheadColor;
		m_AheadLine.m_Vertices = new array<float>();
		m_aCommands.Insert(m_AheadLine);

		m_DoneLine = new LineDrawCommand();
		m_DoneLine.m_iColor = m_iDoneColor;
		m_DoneLine.m_Vertices = new array<float>();
		m_aCommands.Insert(m_DoneLine);

		InsertDot(m_aRoute[0], 5 * ui, m_iStartColor, ui);
		InsertDot(m_aRoute[m_aRoute.Count() - 1], 5 * ui, m_iFinishColor, ui);
		foreach (vector cp : m_aCheckpoints)
		{
			m_aCheckpointDots.Insert(InsertDot(cp, 3.5 * ui, m_iCheckpointColor, ui));
		}

		m_NextRing = new LineDrawCommand();
		m_NextRing.m_fWidth = 2 * ui;
		m_NextRing.m_bShouldEnclose = true;
		m_NextRing.m_Vertices = new array<float>();
		m_aCommands.Insert(m_NextRing);

		m_TruckOutline = new LineDrawCommand();
		m_TruckOutline.m_iColor = m_iOutlineColor;
		m_TruckOutline.m_fWidth = 2 * ui;
		m_TruckOutline.m_bShouldEnclose = true;
		m_TruckOutline.m_Vertices = new array<float>();
		m_aCommands.Insert(m_TruckOutline);

		m_TruckShape = new PolygonDrawCommand();
		m_TruckShape.m_iColor = m_iTruckColor;
		m_TruckShape.m_Vertices = new array<float>();
		m_aCommands.Insert(m_TruckShape);

		m_PlayerOutline = new LineDrawCommand();
		m_PlayerOutline.m_fWidth = 2 * ui;
		m_PlayerOutline.m_bShouldEnclose = true;
		m_PlayerOutline.m_Vertices = new array<float>();
		m_aCommands.Insert(m_PlayerOutline);

		m_PlayerShape = new PolygonDrawCommand();
		m_PlayerShape.m_Vertices = new array<float>();
		m_aCommands.Insert(m_PlayerShape);
	}

	//------------------------------------------------------------------------------------------------
	//! A filled dot with a dark outline. \return the fill, for recolouring
	protected PolygonDrawCommand InsertDot(vector world, float radius, int color, float ui)
	{
		vector centre = ToPixel(world);

		LineDrawCommand dotOutline = new LineDrawCommand();
		dotOutline.m_iColor = m_iOutlineColor;
		dotOutline.m_fWidth = 1.5 * ui;
		dotOutline.m_bShouldEnclose = true;
		dotOutline.m_Vertices = new array<float>();
		m_wCanvas.TessellateCircle(centre, radius + 0.75 * ui, 16, dotOutline.m_Vertices);
		m_aCommands.Insert(dotOutline);

		PolygonDrawCommand dot = new PolygonDrawCommand();
		dot.m_iColor = color;
		dot.m_Vertices = new array<float>();
		m_wCanvas.TessellateCircle(centre, radius, 16, dot.m_Vertices);
		m_aCommands.Insert(dot);

		return dot;
	}

	//------------------------------------------------------------------------------------------------
	//! A square on the truck
	protected void SetTruckVertices(vector worldPos, float ui)
	{
		m_TruckShape.m_Vertices.Clear();
		m_TruckOutline.m_Vertices.Clear();

		vector centre = ToPixel(worldPos);
		float half = 6 * ui;

		array<float> vertices = m_TruckShape.m_Vertices;
		vertices.Insert(centre[0] - half);
		vertices.Insert(centre[1] - half);
		vertices.Insert(centre[0] + half);
		vertices.Insert(centre[1] - half);
		vertices.Insert(centre[0] + half);
		vertices.Insert(centre[1] + half);
		vertices.Insert(centre[0] - half);
		vertices.Insert(centre[1] + half);

		m_TruckOutline.m_Vertices.Copy(vertices);
	}

	//------------------------------------------------------------------------------------------------
	//! An arrow-head triangle at the player, pointing along their heading
	protected void SetPlayerVertices(vector worldPos, float yaw, float ui)
	{
		vector centre = ToPixel(worldPos);
		float rad = yaw * Math.DEG2RAD;

		// Canvas y points south, so north (yaw 0) is up the screen
		float forwardX = Math.Sin(rad);
		float forwardY = -Math.Cos(rad);
		float rightX = -forwardY;
		float rightY = forwardX;

		float length = 9 * ui;
		float halfWidth = 6 * ui;

		float tipX = centre[0] + forwardX * length;
		float tipY = centre[1] + forwardY * length;
		float backX = centre[0] - forwardX * length * 0.6;
		float backY = centre[1] - forwardY * length * 0.6;

		array<float> vertices = m_PlayerShape.m_Vertices;
		vertices.Insert(tipX);
		vertices.Insert(tipY);
		vertices.Insert(backX + rightX * halfWidth);
		vertices.Insert(backY + rightY * halfWidth);
		vertices.Insert(backX - rightX * halfWidth);
		vertices.Insert(backY - rightY * halfWidth);

		m_PlayerOutline.m_Vertices.Copy(vertices);
	}

	//------------------------------------------------------------------------------------------------
	//! The point this far along the route
	protected vector GetRoutePoint(float distance)
	{
		int last = m_aRoute.Count() - 1;
		if (distance <= 0)
			return m_aRoute[0];

		if (distance >= m_fRouteLength)
			return m_aRoute[last];

		for (int i = 1; i <= last; i++)
		{
			if (m_aRouteDistances[i] < distance)
				continue;

			float length = m_aRouteDistances[i] - m_aRouteDistances[i - 1];
			if (length <= 0)
				return m_aRoute[i];

			return vector.Lerp(m_aRoute[i - 1], m_aRoute[i], (distance - m_aRouteDistances[i - 1]) / length);
		}

		return m_aRoute[last];
	}

	//------------------------------------------------------------------------------------------------
	//! World position to canvas pixels: the view square fitted and centred in the canvas, north up
	protected vector ToPixel(vector world)
	{
		float pixelsPerMetre = Math.Min(m_fCanvasW, m_fCanvasH) / m_fViewSize;
		float offsetX = (m_fCanvasW - m_fViewSize * pixelsPerMetre) * 0.5;
		float offsetY = (m_fCanvasH - m_fViewSize * pixelsPerMetre) * 0.5;

		return Vector(offsetX + (world[0] - m_fViewMinX) * pixelsPerMetre, offsetY + (m_fViewMinZ + m_fViewSize - world[2]) * pixelsPerMetre, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! A world polyline as canvas vertices, dropping points too close together to see
	protected void ToVertices(notnull array<vector> points, notnull array<float> vertices)
	{
		int last = points.Count() - 1;
		float lastX, lastY;
		foreach (int i, vector point : points)
		{
			vector pixel = ToPixel(point);
			if (i > 0 && i < last && Math.AbsFloat(pixel[0] - lastX) + Math.AbsFloat(pixel[1] - lastY) < MIN_VERTEX_SPACING)
				continue;

			vertices.Insert(pixel[0]);
			vertices.Insert(pixel[1]);
			lastX = pixel[0];
			lastY = pixel[1];
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps the player on the map's edge when they wander outside the view
	protected vector ClampToView(vector world)
	{
		world[0] = Math.Clamp(world[0], m_fViewMinX, m_fViewMinX + m_fViewSize);
		world[2] = Math.Clamp(world[2], m_fViewMinZ, m_fViewMinZ + m_fViewSize);
		return world;
	}

	//------------------------------------------------------------------------------------------------
	//! A square around the route with a margin
	protected void FitView()
	{
		vector first = m_aRoute[0];
		float minX = first[0];
		float maxX = minX;
		float minZ = first[2];
		float maxZ = minZ;
		foreach (vector point : m_aRoute)
		{
			minX = Math.Min(minX, point[0]);
			maxX = Math.Max(maxX, point[0]);
			minZ = Math.Min(minZ, point[2]);
			maxZ = Math.Max(maxZ, point[2]);
		}

		float size = Math.Max(maxX - minX, maxZ - minZ);
		size = Math.Max(MIN_VIEW_SIZE, size * (1 + 2 * VIEW_PADDING));

		m_fViewSize = size;
		m_fViewMinX = (minX + maxX) * 0.5 - size * 0.5;
		m_fViewMinZ = (minZ + maxZ) * 0.5 - size * 0.5;
	}

	//------------------------------------------------------------------------------------------------
	//! The roads around the view, for the background
	protected void GatherRoads()
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		RoadNetworkManager roadManager = aiWorld.GetRoadNetworkManager();
		if (!roadManager)
			return;

		vector mins = Vector(m_fViewMinX - ROAD_MARGIN, -1000, m_fViewMinZ - ROAD_MARGIN);
		vector maxs = Vector(m_fViewMinX + m_fViewSize + ROAD_MARGIN, 10000, m_fViewMinZ + m_fViewSize + ROAD_MARGIN);

		array<BaseRoad> roads = {};
		roadManager.GetRoadsInAABB(mins, maxs, roads);
		foreach (BaseRoad road : roads)
		{
			if (!road)
				continue;

			array<vector> points = {};
			road.GetPoints(points);
			if (points.Count() >= 2)
				m_aRoads.Insert(points);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Cuts the background roads to the view square - the canvas doesn't clip what is drawn outside it
	protected void ClipRoadsToView()
	{
		array<ref array<vector>> clipped = {};
		foreach (array<vector> road : m_aRoads)
		{
			array<vector> piece;
			for (int i = 1; i < road.Count(); i++)
			{
				vector a, b;
				if (!ClipSegment(road[i - 1], road[i], a, b))
				{
					piece = null;
					continue;
				}

				// A new piece where the road comes (back) into view
				if (!piece || a != road[i - 1])
				{
					piece = new array<vector>();
					piece.Insert(a);
					clipped.Insert(piece);
				}

				piece.Insert(b);

				// The road leaves the view here
				if (b != road[i])
					piece = null;
			}
		}

		m_aRoads = clipped;
	}

	//------------------------------------------------------------------------------------------------
	//! Liang-Barsky: the part of segment a-b inside the view square (on x/z)
	protected bool ClipSegment(vector a, vector b, out vector clippedA, out vector clippedB)
	{
		float t0 = 0;
		float t1 = 1;
		float dx = b[0] - a[0];
		float dz = b[2] - a[2];

		if (!ClipEdge(-dx, a[0] - m_fViewMinX, t0, t1))
			return false;
		if (!ClipEdge(dx, m_fViewMinX + m_fViewSize - a[0], t0, t1))
			return false;
		if (!ClipEdge(-dz, a[2] - m_fViewMinZ, t0, t1))
			return false;
		if (!ClipEdge(dz, m_fViewMinZ + m_fViewSize - a[2], t0, t1))
			return false;

		clippedA = a;
		clippedB = b;
		if (t0 > 0)
			clippedA = vector.Lerp(a, b, t0);
		if (t1 < 1)
			clippedB = vector.Lerp(a, b, t1);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ClipEdge(float p, float q, inout float t0, inout float t1)
	{
		if (p == 0)
			return q >= 0;

		float r = q / p;
		if (p < 0)
		{
			if (r > t1)
				return false;
			if (r > t0)
				t0 = r;
		}
		else
		{
			if (r < t0)
				return false;
			if (r < t1)
				t1 = r;
		}

		return true;
	}
}
