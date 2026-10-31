//------------------------------------------------------------------------------------
// CRF_RallyMinimap: the course minimap on the rally HUD (under the standings panel)
//
// The whole course, north up, fitted to a square: the roads around it faint, the course over them
// (legs already driven dimmed, the current leg bright), the checkpoints - start green, finish red -
// a ring on the next one, and the local player's car as an arrow pointing the way it faces.
//
// Client side only. It is built from the rally_start / rally_cpN / rally_end positions every machine
// already caches for the HUD, so nothing extra is replicated.
//
// The course is traced along the roads between consecutive checkpoints. The roads around the course
// (RoadNetworkManager) become a graph - points along each road, linked in order, plus links where a
// road's end meets another road - and each leg is the shortest road route between its checkpoints.
// A leg with no sensible road route (a checkpoint off the road network, or a route far longer than
// the straight line) is drawn straight. With checkpoints far apart the shortest road may not be the
// stage road; another checkpoint in between fixes that.
//------------------------------------------------------------------------------------
class CRF_RallyMinimap
{
	// Road graph
	protected static const float ROAD_STEP = 20;          // road points are filled in to at most this far apart (m)
	protected static const float JOIN_RADIUS = 8;         // a road end this close to another road is a junction (m)
	protected static const float GRID_CELL = 10;          // spatial lookup cell (m)
	protected static const float ROAD_MARGIN = 300;       // roads are gathered this far around the checkpoints (m)
	protected static const float SNAP_RADIUS = 60;        // a checkpoint further than this from any road gets a straight leg (m)
	protected static const float DETOUR_LIMIT = 4;        // as does a road route this many times longer than the straight line
	protected static const int MAX_EXPANSIONS = 40000;    // route search budget per leg

	// View
	protected static const float VIEW_PADDING = 0.08;     // around the course, as a fraction of its size
	protected static const float MIN_VIEW_SIZE = 300;     // m
	protected static const float LAYOUT_SIZE = 284;       // MinimapCanvas's size in the layout; drawing sizes below are at that size
	protected static const float MIN_VERTEX_SPACING = 1.5; // px - closer road points are dropped

	protected CanvasWidget m_wCanvas;
	protected float m_fCanvasW;
	protected float m_fCanvasH;
	protected ref array<ref CanvasWidgetCommand> m_aCommands = {};

	// World data, built once
	protected bool m_bBuilt;
	protected ref array<vector> m_aCheckpoints = {};
	protected ref array<ref array<vector>> m_aLegs = {};
	protected ref array<ref array<vector>> m_aRoads = {};
	protected float m_fViewMinX;
	protected float m_fViewMinZ;
	protected float m_fViewSize;

	// Road graph, only while building
	protected ref array<vector> m_aNodes;
	protected ref array<int> m_aNodeRoad;
	protected ref array<ref array<int>> m_aLinks;
	protected ref map<int, ref array<int>> m_mGrid;
	protected ref array<int> m_aHeapNodes;
	protected ref array<float> m_aHeapKeys;

	// The parts of the drawing that change with every update
	protected ref array<ref LineDrawCommand> m_aLegLines = {};
	protected ref LineDrawCommand m_NextRing;
	protected ref PolygonDrawCommand m_CarShape;
	protected ref LineDrawCommand m_CarOutline;

	protected int m_iRoadColor;
	protected int m_iAheadColor;
	protected int m_iCurrentColor;
	protected int m_iDoneColor;
	protected int m_iCheckpointColor;
	protected int m_iStartColor;
	protected int m_iFinishColor;
	protected int m_iAccentColor;
	protected int m_iOutlineColor;
	protected int m_iHiddenColor;

	//------------------------------------------------------------------------------------------------
	void CRF_RallyMinimap()
	{
		// Designer colours are sRGB; widget colours are linear (Color.FromSRGBA converts)
		m_iRoadColor       = Color.FromSRGBA(58, 64, 78, 255).PackToInt();
		m_iAheadColor      = Color.FromSRGBA(196, 204, 218, 255).PackToInt();
		m_iCurrentColor    = Color.FromSRGBA(255, 255, 255, 255).PackToInt();
		m_iDoneColor       = Color.FromSRGBA(96, 104, 120, 255).PackToInt();
		m_iCheckpointColor = Color.FromSRGBA(196, 204, 218, 255).PackToInt();
		m_iStartColor      = Color.FromSRGBA(92, 196, 128, 255).PackToInt();
		m_iFinishColor     = Color.FromSRGBA(232, 80, 80, 255).PackToInt();
		m_iAccentColor     = Color.FromSRGBA(255, 229, 0, 255).PackToInt();  // the HUD accent / checkpoint arrow
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
	//! Traces the course along the roads and frames it. Once per race - the road search isn't free.
	//! \param[in] checkpoints Course checkpoints in order, start first and finish last
	//! \param[in] closedLoop Also draw the run from the finish back to the start (circuits)
	void Build(notnull array<vector> checkpoints, bool closedLoop)
	{
		m_bBuilt = true;
		m_aCheckpoints.Copy(checkpoints);
		m_aLegs.Clear();
		m_aRoads.Clear();

		int cpCount = checkpoints.Count();
		if (cpCount < 2)
			return;

		float minX = checkpoints[0][0];
		float maxX = minX;
		float minZ = checkpoints[0][2];
		float maxZ = minZ;
		foreach (vector cp : checkpoints)
		{
			minX = Math.Min(minX, cp[0]);
			maxX = Math.Max(maxX, cp[0]);
			minZ = Math.Min(minZ, cp[2]);
			maxZ = Math.Max(maxZ, cp[2]);
		}

		BuildRoadGraph(Vector(minX - ROAD_MARGIN, -1000, minZ - ROAD_MARGIN), Vector(maxX + ROAD_MARGIN, 10000, maxZ + ROAD_MARGIN));

		int legCount = cpCount - 1;
		if (closedLoop)
			legCount++;

		for (int i = 0; i < legCount; i++)
		{
			array<vector> leg = {};
			RouteLeg(checkpoints[i], checkpoints[(i + 1) % cpCount], leg);
			m_aLegs.Insert(leg);
		}

		m_aNodes = null;
		m_aNodeRoad = null;
		m_aLinks = null;
		m_mGrid = null;
		m_aHeapNodes = null;
		m_aHeapKeys = null;

		FitView();
		ClipRoadsToView();
	}

	//------------------------------------------------------------------------------------------------
	//! Redraws the dynamic parts. Call at the HUD rate.
	//! \param[in] showCar Draw the car (false for spectators and the dead)
	//! \param[in] carPos The local player's vehicle (or character) position
	//! \param[in] carYaw Its heading in degrees, clockwise from north
	//! \param[in] currentLeg The leg being driven (leg i runs from checkpoint i to i + 1), -1 before the start, past the last leg once finished
	//! \param[in] nextCP The checkpoint to ring, -1 for none
	void Update(bool showCar, vector carPos, float carYaw, int currentLeg, int nextCP)
	{
		if (!m_wCanvas || !m_bBuilt || m_aLegs.IsEmpty())
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

		foreach (int legIndex, LineDrawCommand legLine : m_aLegLines)
		{
			if (legIndex < currentLeg)
			{
				legLine.m_iColor = m_iDoneColor;
				legLine.m_fWidth = 2.5 * ui;
			}
			else if (legIndex == currentLeg)
			{
				legLine.m_iColor = m_iCurrentColor;
				legLine.m_fWidth = 3.5 * ui;
			}
			else
			{
				legLine.m_iColor = m_iAheadColor;
				legLine.m_fWidth = 2.5 * ui;
			}
		}

		m_NextRing.m_Vertices.Clear();
		if (m_aCheckpoints.IsIndexValid(nextCP))
		{
			m_NextRing.m_iColor = m_iAccentColor;
			m_wCanvas.TessellateCircle(ToPixel(m_aCheckpoints[nextCP]), 8 * ui, 20, m_NextRing.m_Vertices);
		}
		else
		{
			m_NextRing.m_iColor = m_iHiddenColor;
		}

		m_CarShape.m_Vertices.Clear();
		m_CarOutline.m_Vertices.Clear();
		if (showCar)
		{
			m_CarShape.m_iColor = m_iAccentColor;
			m_CarOutline.m_iColor = m_iOutlineColor;
			SetCarVertices(ClampToView(carPos), carYaw, ui);
		}
		else
		{
			m_CarShape.m_iColor = m_iHiddenColor;
			m_CarOutline.m_iColor = m_iHiddenColor;
		}

		m_wCanvas.SetDrawCommands(m_aCommands);
	}

	//===================================================================================
	// DRAWING
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! The static drawing (roads, course, checkpoints) for the current canvas size, plus the
	//! commands Update fills in
	protected void BuildDrawing()
	{
		m_aCommands.Clear();
		m_aLegLines.Clear();
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

		foreach (array<vector> leg : m_aLegs)
		{
			LineDrawCommand legLine = new LineDrawCommand();
			legLine.m_iColor = m_iAheadColor;
			legLine.m_fWidth = 2.5 * ui;
			legLine.m_Vertices = new array<float>();
			ToVertices(leg, legLine.m_Vertices);
			m_aCommands.Insert(legLine);
			m_aLegLines.Insert(legLine);
		}

		int lastCP = m_aCheckpoints.Count() - 1;
		foreach (int cpIndex, vector cp : m_aCheckpoints)
		{
			float radius = 3 * ui;
			int color = m_iCheckpointColor;
			if (cpIndex == 0)
			{
				radius = 5 * ui;
				color = m_iStartColor;
			}
			else if (cpIndex == lastCP)
			{
				radius = 5 * ui;
				color = m_iFinishColor;
			}

			vector centre = ToPixel(cp);

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
		}

		m_NextRing = new LineDrawCommand();
		m_NextRing.m_fWidth = 2 * ui;
		m_NextRing.m_bShouldEnclose = true;
		m_NextRing.m_Vertices = new array<float>();
		m_aCommands.Insert(m_NextRing);

		m_CarOutline = new LineDrawCommand();
		m_CarOutline.m_fWidth = 2 * ui;
		m_CarOutline.m_bShouldEnclose = true;
		m_CarOutline.m_Vertices = new array<float>();
		m_aCommands.Insert(m_CarOutline);

		m_CarShape = new PolygonDrawCommand();
		m_CarShape.m_Vertices = new array<float>();
		m_aCommands.Insert(m_CarShape);
	}

	//------------------------------------------------------------------------------------------------
	//! An arrow-head triangle at the car, pointing along its heading
	protected void SetCarVertices(vector worldPos, float yaw, float ui)
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

		array<float> vertices = m_CarShape.m_Vertices;
		vertices.Insert(tipX);
		vertices.Insert(tipY);
		vertices.Insert(backX + rightX * halfWidth);
		vertices.Insert(backY + rightY * halfWidth);
		vertices.Insert(backX - rightX * halfWidth);
		vertices.Insert(backY - rightY * halfWidth);

		m_CarOutline.m_Vertices.Copy(vertices);
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
	//! Keeps the car on the map's edge when it wanders outside the view
	protected vector ClampToView(vector world)
	{
		world[0] = Math.Clamp(world[0], m_fViewMinX, m_fViewMinX + m_fViewSize);
		world[2] = Math.Clamp(world[2], m_fViewMinZ, m_fViewMinZ + m_fViewSize);
		return world;
	}

	//------------------------------------------------------------------------------------------------
	//! A square around the course with a margin
	protected void FitView()
	{
		vector first = m_aLegs[0][0];
		float minX = first[0];
		float maxX = minX;
		float minZ = first[2];
		float maxZ = minZ;
		foreach (array<vector> leg : m_aLegs)
		{
			foreach (vector point : leg)
			{
				minX = Math.Min(minX, point[0]);
				maxX = Math.Max(maxX, point[0]);
				minZ = Math.Min(minZ, point[2]);
				maxZ = Math.Max(maxZ, point[2]);
			}
		}

		float size = Math.Max(maxX - minX, maxZ - minZ);
		size = Math.Max(MIN_VIEW_SIZE, size * (1 + 2 * VIEW_PADDING));

		m_fViewSize = size;
		m_fViewMinX = (minX + maxX) * 0.5 - size * 0.5;
		m_fViewMinZ = (minZ + maxZ) * 0.5 - size * 0.5;
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

	//===================================================================================
	// ROAD ROUTING
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Every road in the box as graph nodes (its points, filled in to ROAD_STEP apart) linked along the
	//! road, with each road end linked to the other roads it meets. Also keeps the roads for drawing.
	protected void BuildRoadGraph(vector mins, vector maxs)
	{
		m_aNodes = {};
		m_aNodeRoad = {};
		m_aLinks = {};
		m_mGrid = new map<int, ref array<int>>();

		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return;

		RoadNetworkManager roadManager = aiWorld.GetRoadNetworkManager();
		if (!roadManager)
			return;

		array<BaseRoad> roads = {};
		roadManager.GetRoadsInAABB(mins, maxs, roads);

		array<int> roadEnds = {};
		array<vector> points = {};
		foreach (BaseRoad road : roads)
		{
			if (!road)
				continue;

			points.Clear();
			road.GetPoints(points);
			if (points.Count() < 2)
				continue;

			int roadIndex = m_aRoads.Count();
			array<vector> line = {};
			int previous = -1;
			foreach (int p, vector point : points)
			{
				if (p > 0)
				{
					vector from = points[p - 1];
					int steps = (int)Math.Ceil(vector.DistanceXZ(from, point) / ROAD_STEP);
					for (int s = 1; s < steps; s++)
					{
						vector between = vector.Lerp(from, point, s / (float)steps);
						previous = AddNode(between, previous, roadIndex);
						line.Insert(between);
					}
				}

				previous = AddNode(point, previous, roadIndex);
				line.Insert(point);

				if (p == 0)
					roadEnds.Insert(previous);
			}

			roadEnds.Insert(previous);
			m_aRoads.Insert(line);
		}

		// Junctions: a road end joins every point of another road within JOIN_RADIUS
		array<int> nearby = {};
		foreach (int endNode : roadEnds)
		{
			FindNodesNear(m_aNodes[endNode], JOIN_RADIUS, nearby);
			foreach (int other : nearby)
			{
				if (m_aNodeRoad[other] == m_aNodeRoad[endNode] || m_aLinks[endNode].Contains(other))
					continue;

				m_aLinks[endNode].Insert(other);
				m_aLinks[other].Insert(endNode);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int AddNode(vector pos, int linkTo, int roadIndex)
	{
		int index = m_aNodes.Insert(pos);
		m_aNodeRoad.Insert(roadIndex);
		m_aLinks.Insert(new array<int>());

		if (linkTo >= 0)
		{
			m_aLinks[index].Insert(linkTo);
			m_aLinks[linkTo].Insert(index);
		}

		int key = GridKey((int)Math.Floor(pos[0] / GRID_CELL), (int)Math.Floor(pos[2] / GRID_CELL));
		array<int> cell = m_mGrid.Get(key);
		if (!cell)
		{
			cell = new array<int>();
			m_mGrid.Insert(key, cell);
		}
		cell.Insert(index);

		return index;
	}

	//------------------------------------------------------------------------------------------------
	protected int GridKey(int cellX, int cellZ)
	{
		return cellX * 65536 + cellZ;
	}

	//------------------------------------------------------------------------------------------------
	protected void FindNodesNear(vector pos, float radius, notnull array<int> found)
	{
		found.Clear();
		int minCellX = (int)Math.Floor((pos[0] - radius) / GRID_CELL);
		int maxCellX = (int)Math.Floor((pos[0] + radius) / GRID_CELL);
		int minCellZ = (int)Math.Floor((pos[2] - radius) / GRID_CELL);
		int maxCellZ = (int)Math.Floor((pos[2] + radius) / GRID_CELL);

		for (int cellX = minCellX; cellX <= maxCellX; cellX++)
		{
			for (int cellZ = minCellZ; cellZ <= maxCellZ; cellZ++)
			{
				array<int> cell = m_mGrid.Get(GridKey(cellX, cellZ));
				if (!cell)
					continue;

				foreach (int node : cell)
				{
					if (vector.DistanceXZ(m_aNodes[node], pos) <= radius)
						found.Insert(node);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The road node closest to pos within radius, -1 for none
	protected int FindNearestNode(vector pos, float radius)
	{
		array<int> nearby = {};
		FindNodesNear(pos, radius, nearby);

		int best = -1;
		float bestDistance = radius;
		foreach (int node : nearby)
		{
			float distance = vector.DistanceXZ(m_aNodes[node], pos);
			if (distance <= bestDistance)
			{
				best = node;
				bestDistance = distance;
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	//! One leg as world points: along the roads where there's a sensible road route, else straight
	protected void RouteLeg(vector from, vector to, notnull array<vector> leg)
	{
		leg.Clear();
		leg.Insert(from);

		int startNode = FindNearestNode(from, SNAP_RADIUS);
		int goalNode = FindNearestNode(to, SNAP_RADIUS);
		array<int> path = {};
		if (startNode >= 0 && goalNode >= 0 && FindPath(startNode, goalNode, path))
		{
			float length;
			vector previous = from;
			foreach (int node : path)
			{
				length += vector.DistanceXZ(previous, m_aNodes[node]);
				previous = m_aNodes[node];
			}
			length += vector.DistanceXZ(previous, to);

			if (length <= DETOUR_LIMIT * Math.Max(vector.DistanceXZ(from, to), 50))
			{
				foreach (int pathNode : path)
				{
					leg.Insert(m_aNodes[pathNode]);
				}
			}
		}

		leg.Insert(to);
	}

	//------------------------------------------------------------------------------------------------
	//! A* over the road graph. \param[out] path Nodes from start to goal
	protected bool FindPath(int startNode, int goalNode, notnull array<int> path)
	{
		path.Clear();
		int count = m_aNodes.Count();

		array<float> costs = {};
		array<int> cameFrom = {};
		array<bool> closed = {};
		costs.Resize(count);
		cameFrom.Resize(count);
		closed.Resize(count);
		for (int i = 0; i < count; i++)
		{
			costs[i] = float.MAX;
			cameFrom[i] = -1;
			closed[i] = false;
		}

		m_aHeapNodes = {};
		m_aHeapKeys = {};

		vector goalPos = m_aNodes[goalNode];
		costs[startNode] = 0;
		HeapPush(startNode, vector.DistanceXZ(m_aNodes[startNode], goalPos));

		int expansions;
		while (!m_aHeapNodes.IsEmpty())
		{
			int node = HeapPop();
			if (closed[node])
				continue;

			if (node == goalNode)
			{
				array<int> reversed = {};
				for (int step = goalNode; step >= 0; step = cameFrom[step])
				{
					reversed.Insert(step);
				}

				for (int r = reversed.Count() - 1; r >= 0; r--)
				{
					path.Insert(reversed[r]);
				}

				return true;
			}

			closed[node] = true;
			expansions++;
			if (expansions > MAX_EXPANSIONS)
				return false;

			vector nodePos = m_aNodes[node];
			foreach (int next : m_aLinks[node])
			{
				if (closed[next])
					continue;

				float cost = costs[node] + vector.DistanceXZ(nodePos, m_aNodes[next]);
				if (cost >= costs[next])
					continue;

				costs[next] = cost;
				cameFrom[next] = node;
				HeapPush(next, cost + vector.DistanceXZ(m_aNodes[next], goalPos));
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Binary min-heap on m_aHeapKeys
	protected void HeapPush(int node, float key)
	{
		int i = m_aHeapNodes.Insert(node);
		m_aHeapKeys.Insert(key);

		while (i > 0)
		{
			int parent = (i - 1) / 2;
			if (m_aHeapKeys[parent] <= key)
				break;

			m_aHeapNodes[i] = m_aHeapNodes[parent];
			m_aHeapKeys[i] = m_aHeapKeys[parent];
			i = parent;
		}

		m_aHeapNodes[i] = node;
		m_aHeapKeys[i] = key;
	}

	//------------------------------------------------------------------------------------------------
	protected int HeapPop()
	{
		int top = m_aHeapNodes[0];
		int last = m_aHeapNodes.Count() - 1;
		int node = m_aHeapNodes[last];
		float key = m_aHeapKeys[last];
		m_aHeapNodes.Remove(last);
		m_aHeapKeys.Remove(last);

		if (last == 0)
			return top;

		// Sift the old last element down from the root
		int i = 0;
		while (true)
		{
			int child = 2 * i + 1;
			if (child >= last)
				break;

			if (child + 1 < last && m_aHeapKeys[child + 1] < m_aHeapKeys[child])
				child++;

			if (m_aHeapKeys[child] >= key)
				break;

			m_aHeapNodes[i] = m_aHeapNodes[child];
			m_aHeapKeys[i] = m_aHeapKeys[child];
			i = child;
		}

		m_aHeapNodes[i] = node;
		m_aHeapKeys[i] = key;
		return top;
	}
}
