class CRF_ShapeMarkerClass : CRF_ManualMarkerClass
{
}

enum CRF_EMapShapeMarkerType
{
	CIRCLE,
	SQUARE
}

// Pre-placeable map marker that renders as a circle or square with independent X/Y size.
class CRF_ShapeMarker : CRF_ManualMarker
{
	protected const float DEG_TO_RAD = 0.01745329252;
	protected const ResourceName SHAPE_DRAW_LAYOUT = "{B8793707B56B2F9F}UI/Map/PolyMapMarkerBase.layout";

	// Fallback image-based implementation (toggle via m_bUseDrawApproach).
	protected const ResourceName SHAPE_CIRCLE_IMAGESET = "{F3A9B47F55BE8D2B}UI/Images/Atlas/PS_Atlas_x64.imageset";
	protected const string SHAPE_CIRCLE_QUAD = "Circle";
	protected const ResourceName SHAPE_SQUARE_IMAGESET = "{8479B3B5347DF5CF}UI/Imagesets/MilitarySymbol/ID_D.imageset";
	protected const string SHAPE_SQUARE_QUAD = "Generic_Select";

	protected CanvasWidget m_wShapeCanvas;
	protected ref LineDrawCommand m_DrawShapeBorder = new LineDrawCommand();
	protected ref array<ref CanvasWidgetCommand> m_aShapeDrawCommands = { m_DrawShapeBorder };

	[Attribute("0", UIWidgets.ComboBox, "Shape type", "", ParamEnumArray.FromEnum(CRF_EMapShapeMarkerType), category: "Shape Marker")]
	protected CRF_EMapShapeMarkerType m_eShapeType;

	[Attribute("100", UIWidgets.Slider, "Marker size on X axis", "1 5000 1", category: "Shape Marker")]
	protected float m_fWorldSizeX;

	[Attribute("100", UIWidgets.Slider, "Marker size on Y axis", "1 5000 1", category: "Shape Marker")]
	protected float m_fWorldSizeY;

	[Attribute("1", UIWidgets.CheckBox, "Use draw approach (disable to revert to icon implementation)", category: "Shape Marker")]
	protected bool m_bUseDrawApproach;

	[Attribute("2.0", UIWidgets.Slider, "Border width (pixels)", "0.25 12 0.05", category: "Shape Marker")]
	protected float m_fShapeBorderWidth;

	[Attribute("0", UIWidgets.CheckBox, "Scale border width with marker size", category: "Shape Marker")]
	protected bool m_bScaleBorderWithMarkerSize;

	[Attribute("100", UIWidgets.Slider, "Reference marker size for border scaling", "10 1000 1", category: "Shape Marker")]
	protected float m_fBorderScaleReferenceSize;

	[Attribute("64", UIWidgets.Slider, "Circle segments", "12 180 1", category: "Shape Marker")]
	protected int m_iCircleSegments;

	override protected void EOnInit(IEntity owner)
	{
		if (m_bUseDrawApproach)
			m_sMarkerPrefab = SHAPE_DRAW_LAYOUT;

		ApplyShapeVisual();
		super.EOnInit(owner);
	}

	override void CreateMapWidget(MapConfiguration mapConfig)
	{
		if (!m_bUseDrawApproach)
		{
			ApplyShapeVisual();
			super.CreateMapWidget(mapConfig);
			return;
		}

		if (m_wRoot)
			return;

		if (!IsCurrentFactionVisibility())
			return;

		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame)
			mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame)
			return;

		m_wRoot = GetGame().GetWorkspace().CreateWidgets(SHAPE_DRAW_LAYOUT, mapFrame);
		if (!m_wRoot)
			return;

		m_wRoot.SetZOrder(m_iZOrder);
		m_wShapeCanvas = CanvasWidget.Cast(m_wRoot);
		if (!m_wShapeCanvas)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
			return;
		}

		m_DrawShapeBorder.m_pTexture = null;
		m_DrawShapeBorder.m_bShouldEnclose = true;
		m_DrawShapeBorder.m_UVScale = Vector(1, 1, 0.01);

		m_wShapeCanvas.SetDrawCommands(m_aShapeDrawCommands);
		SetEventMask(EntityEvent.POSTFRAME);
	}

	override void DeleteMapWidget(MapConfiguration mapConfig)
	{
		if (!m_bUseDrawApproach)
		{
			super.DeleteMapWidget(mapConfig);
			return;
		}

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wShapeCanvas = null;
		ClearEventMask(EntityEvent.POSTFRAME);
	}

	override protected void EOnPostFrame(IEntity owner, float timeSlice)
	{
		if (!m_bUseDrawApproach)
		{
			if (!m_wRoot || !m_hManualMarkerComponent)
				return;

			float fallbackX;
			float fallbackY;
			float fallbackSizeX;
			float fallbackSizeY;
			if (!GetShapeScreenMetrics(fallbackX, fallbackY, fallbackSizeX, fallbackSizeY))
				return;

			m_hManualMarkerComponent.SetSlot(fallbackX, fallbackY, fallbackSizeX, fallbackSizeY, GetYawPitchRoll()[0]);
			return;
		}

		if (!m_wRoot || !m_wShapeCanvas)
			return;

		float centerX;
		float centerY;
		float sizeX;
		float sizeY;
		if (!GetShapeScreenMetrics(centerX, centerY, sizeX, sizeY))
			return;

		UpdateShapeDrawCommands(centerX, centerY, sizeX, sizeY, GetYawPitchRoll()[0]);
	}

	protected bool GetShapeScreenMetrics(out float centerX, out float centerY, out float sizeX, out float sizeY)
	{
		bool useDrawCoordinates = m_bUseDrawApproach;

		float worldX;
		float worldY;
		float screenX;
		float screenY;
		float screenXEndX;
		float screenYEndX;
		float screenXEndY;
		float screenYEndY;

		vector worldPosition = GetOrigin();
		worldX = worldPosition[0];
		worldY = worldPosition[2];

		m_MapEntity.WorldToScreen(worldX, worldY, screenX, screenY, true);
		m_MapEntity.WorldToScreen(worldX + m_fWorldSizeX, worldY, screenXEndX, screenYEndX, true);
		m_MapEntity.WorldToScreen(worldX, worldY + m_fWorldSizeY, screenXEndY, screenYEndY, true);

		if (useDrawCoordinates)
		{
			centerX = screenX;
			centerY = screenY;
		}
		else
		{
			centerX = GetGame().GetWorkspace().DPIUnscale(screenX);
			centerY = GetGame().GetWorkspace().DPIUnscale(screenY);
		}

		sizeX = Math.AbsFloat(m_fWorldSizeX);
		sizeY = Math.AbsFloat(m_fWorldSizeY);
		if (m_bUseWorldScale)
		{
			if (useDrawCoordinates)
			{
				sizeX = Math.AbsFloat(screenXEndX - screenX);
				sizeY = Math.AbsFloat(screenY - screenYEndY);
			}
			else
			{
				sizeX = Math.AbsFloat(GetGame().GetWorkspace().DPIUnscale(screenXEndX - screenX));
				sizeY = Math.AbsFloat(GetGame().GetWorkspace().DPIUnscale(screenY - screenYEndY));
			}
		}

		if (sizeX < 1.0)
			sizeX = 1.0;
		if (sizeY < 1.0)
			sizeY = 1.0;

		return true;
	}

	protected void UpdateShapeDrawCommands(float centerX, float centerY, float sizeX, float sizeY, float yawDeg)
	{
		array<float> vertices = new array<float>();

		float halfX = sizeX * 0.5;
		float halfY = sizeY * 0.5;
		float yawRad = yawDeg * DEG_TO_RAD;
		float cosYaw = Math.Cos(yawRad);
		float sinYaw = Math.Sin(yawRad);

		if (m_eShapeType == CRF_EMapShapeMarkerType.SQUARE)
		{
			InsertRotatedVertex(vertices, centerX, centerY, -halfX, -halfY, cosYaw, sinYaw);
			InsertRotatedVertex(vertices, centerX, centerY,  halfX, -halfY, cosYaw, sinYaw);
			InsertRotatedVertex(vertices, centerX, centerY,  halfX,  halfY, cosYaw, sinYaw);
			InsertRotatedVertex(vertices, centerX, centerY, -halfX,  halfY, cosYaw, sinYaw);
		}
		else
		{
			int segments = Math.ClampInt(m_iCircleSegments, 12, 180);
			float step = Math.PI2 / segments;

			for (int i = 0; i < segments; i++)
			{
				float angle = i * step;
				float localX = Math.Cos(angle) * halfX;
				float localY = Math.Sin(angle) * halfY;
				InsertRotatedVertex(vertices, centerX, centerY, localX, localY, cosYaw, sinYaw);
			}
		}

		m_DrawShapeBorder.m_Vertices = vertices;
		m_DrawShapeBorder.m_iColor = m_MarkerColor.PackToInt();
		m_DrawShapeBorder.m_fWidth = GetBorderWidth(sizeX, sizeY);
	}

	protected float GetBorderWidth(float sizeX, float sizeY)
	{
		float borderWidth = m_fShapeBorderWidth;
		if (!m_bScaleBorderWithMarkerSize)
			return borderWidth;

		float referenceSize = Math.Max(1.0, m_fBorderScaleReferenceSize);
		float markerSize = Math.Max(sizeX, sizeY);
		borderWidth *= markerSize / referenceSize;

		return Math.Max(0.05, borderWidth);
	}

	protected void InsertRotatedVertex(notnull array<float> vertices, float centerX, float centerY, float localX, float localY, float cosYaw, float sinYaw)
	{
		float rotatedX = localX * cosYaw - localY * sinYaw;
		float rotatedY = localX * sinYaw + localY * cosYaw;

		vertices.Insert(centerX + rotatedX);
		vertices.Insert(centerY + rotatedY);
	}

	protected void ApplyShapeVisual()
	{
		switch (m_eShapeType)
		{
			case CRF_EMapShapeMarkerType.SQUARE:
				m_sImageSet = SHAPE_SQUARE_IMAGESET;
				m_sImageSetGlow = "";
				m_sQuadName = SHAPE_SQUARE_QUAD;
				break;

			case CRF_EMapShapeMarkerType.CIRCLE:
			default:
				m_sImageSet = SHAPE_CIRCLE_IMAGESET;
				m_sImageSetGlow = "";
				m_sQuadName = SHAPE_CIRCLE_QUAD;
				break;
		}
	}
}
