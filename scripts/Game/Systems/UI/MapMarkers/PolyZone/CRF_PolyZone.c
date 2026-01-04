[ComponentEditorProps(category: "GameScripted/Character", description: "Add label for observers", color: "0 0 255 255", icon: HYBRID_COMPONENT_ICON)]
class CRF_PolyZoneClass: ScriptComponentClass
{
};

[ComponentEditorProps(icon: HYBRID_COMPONENT_ICON)]
class CRF_PolyZone : ScriptComponent
{
	[Attribute("{B8793707B56B2F9F}UI/Map/PolyMapMarkerBase.layout", params: "layout")]
	protected ResourceName m_sPolyMarkerLayout;
	
	[Attribute("{E362BE45DB490A07}UI/data/Zone.edds", UIWidgets.ResourcePickerThumbnail, desc: "", params: "edds")]
	ResourceName m_mPolygonTexture;
	[Attribute("1 1 1 1", UIWidgets.ColorPicker, desc: "")]
	ref Color m_cPolygonColor;
	[Attribute("0.01", UIWidgets.Slider, desc: "", params: "0.001 4 0.01")]
	float m_fPolygonUVScale;
	
	[Attribute("{8D8EB58699FBC40B}UI/data/ZoneBorder.edds", UIWidgets.ResourcePickerThumbnail, desc: "", params: "edds")]
	ResourceName m_mPolygonTextureBorder;
	[Attribute("1 1 1 1", UIWidgets.ColorPicker, desc: "")]
	ref Color m_cPolygonBorderColor;
	[Attribute("0.1", UIWidgets.Slider, desc: "", params: "0.001 40 0.01")]
	float m_fPolygonBorderUVScale;
	[Attribute("15", UIWidgets.Slider, desc: "", params: "1 100 0.1")]
	float m_fPolygonBorderWidth;
	
	protected ref SharedItemRef m_TextureSharedItem;
	protected ref SharedItemRef m_TextureBorderSharedItem;
	ShapeEntity m_ePolylineShapeEntity;
	SCR_MapEntity m_MapEntity;
	ref array<float> m_aPolygon;
	ref array<float> m_aPolygonTrigger;
	
	[Attribute("0")]
	bool m_bIsSafestartBorder;
	
	[Attribute("0")]
	bool m_bIsForwardDeployZone;
		
	[Attribute("0")]
	bool m_bLineMode;
	
	[Attribute("0")]
	bool m_bReversed;
	
	[Attribute("")]
	ref array<FactionKey> m_aVisibleForFactions;
	[Attribute("0", UIWidgets.ComboBox, "", "", ParamEnumArray.FromEnum(CRF_EGamemodeState))]
	ref array<CRF_EGamemodeState> m_aHideOnGameModeStates;
	
	bool IsCurrentVisibility()
	{
		CRF_Gamemode gameMode = CRF_Gamemode.GetInstance();
		if (!gameMode)
			return true;
		
		if (m_aHideOnGameModeStates.Contains(gameMode.m_GamemodeState))
			return false;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return true; // Somehow manager lost, show marker
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return true; // Somehow player controller lost, show marker
		
		SCR_PlayerFactionAffiliationComponent playerFactionAffiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!playerFactionAffiliationComponent)
			return true; // Somehow player faction component lost, show marker
		
		Faction faction = playerFactionAffiliationComponent.GetAffiliatedFaction();
		FactionKey factionKey = "";
		if (faction)
			factionKey = faction.GetFactionKey();
		
		// Check is player faction in visibility list
		return m_aVisibleForFactions.Contains(factionKey);
	}
	
	override void OnPostInit(IEntity owner)
	{
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		ScriptInvokerBase<MapConfigurationInvoker> onMapOpen = m_MapEntity.GetOnMapOpen();
		ScriptInvokerBase<MapConfigurationInvoker> onMapClose = m_MapEntity.GetOnMapClose();
		
		onMapOpen.Insert(CreateMapWidget);
		onMapClose.Insert(DeleteMapWidget);
		
		m_ePolylineShapeEntity = ShapeEntity.Cast(owner);
		
		// Validate that owner is a ShapeEntity before proceeding
		if (!m_ePolylineShapeEntity)
		{
			Print(string.Format("[CRF_PolyZone] ERROR: Component attached to non-ShapeEntity! Owner: %1", owner), LogLevel.ERROR);
			return;
		}
		
		GetGame().GetCallqueue().CallLater(UpdatePolygon, 0, false);
		
		if (m_bIsSafestartBorder && Replication.IsServer() && CRF_SafestartManager.GetInstance())
			CRF_SafestartManager.GetInstance().AddSafestartZone(owner);
		
		if (m_bIsForwardDeployZone && Replication.IsServer() && CRF_GamemodeManager.GetInstance())
			CRF_GamemodeManager.GetInstance().AddForwardDeployZone(owner);
	}
	
	void UpdatePolygon()
	{
		// Check if polyline shape entity is valid
		if (!m_ePolylineShapeEntity)
		{
			Print("[CRF_PolyZone] ERROR: m_ePolylineShapeEntity is NULL. Owner entity must be a ShapeEntity!", LogLevel.ERROR);
			return;
		}
		
		array<vector> outPoints = new array<vector>();
		
		m_ePolylineShapeEntity.GetPointsPositions(outPoints);
		vector origin = GetOwner().GetOrigin();
		for (int i = 0; i < outPoints.Count(); i++)
		{
			outPoints[i] = outPoints[i] + origin;
		}
		for (int i = 0; i < outPoints.Count() - 1; i++)
		{
			if ((Math.AbsFloat(outPoints[i][0] - outPoints[i+1][0]) + Math.AbsFloat(outPoints[i][1] - outPoints[i+1][1])) < 0.1)
			{
				outPoints.RemoveOrdered(i);
				i--;
			}
		}
		/*
		while (outPoints.Count() > 1 && (outPoints[0] - outPoints[outPoints.Count() - 1]).Length() < 1)
			outPoints.Remove(0);
		*/
		m_aPolygon = new array<float>();
		m_aPolygonTrigger = new array<float>();
		
		SCR_Math2D.Get2DPolygon(outPoints, m_aPolygonTrigger);
		
		if (m_bReversed)
		{
			vector minB;
			vector maxB;
			GetGame().GetWorld().GetBoundBox(minB, maxB);
			outPoints.InsertAt(Vector(minB[0], 0, minB[2]), 0);
			outPoints.InsertAt(Vector(minB[0], 0, maxB[2]), 0);
			outPoints.InsertAt(Vector(maxB[0], 0, maxB[2]), 0);
			outPoints.InsertAt(Vector(maxB[0], 0, minB[2]), 0);
			outPoints.InsertAt(Vector(minB[0], 0, minB[2]), 0);
			outPoints.InsertAt(outPoints[5], 0);
		}
			
		SCR_Math2D.Get2DPolygon(outPoints, m_aPolygon);
	}
	
	bool IsInsidePolygon(vector position)
	{
		return Math2D.IsPointInPolygon(m_aPolygonTrigger, position[0], position[2]);
	}
	
	CanvasWidget m_wCanvasWidget;
	protected ref PolygonDrawCommand m_DrawPolygon = new PolygonDrawCommand();
	protected ref LineDrawCommand m_LinePolygon = new LineDrawCommand();
	protected ref array<ref CanvasWidgetCommand> m_MapDrawCommands = { m_DrawPolygon, m_LinePolygon };
	void CreateMapWidget(MapConfiguration mapConfig)
	{
		if (m_bLineMode)
			m_MapDrawCommands = { m_LinePolygon };
		else
			m_MapDrawCommands = { m_DrawPolygon, m_LinePolygon };
		
		if (!IsCurrentVisibility())
			return;
		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		
		// Get map frame
		Widget mapFrame = m_MapEntity.GetMapMenuRoot().FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!mapFrame) mapFrame = m_MapEntity.GetMapMenuRoot();
		if (!mapFrame) return; // Somethig gone wrong
		
		m_wCanvasWidget = CanvasWidget.Cast(GetGame().GetWorkspace().CreateWidgets(m_sPolyMarkerLayout, mapFrame));
				
		if (m_mPolygonTexture != "")
			m_TextureSharedItem = m_wCanvasWidget.LoadTexture(m_mPolygonTexture);
		else
			m_TextureSharedItem = null;
		
		if (m_mPolygonTextureBorder != "")
			m_TextureBorderSharedItem = m_wCanvasWidget.LoadTexture(m_mPolygonTextureBorder);
		else
			m_TextureBorderSharedItem = null;
		
		m_DrawPolygon.m_pTexture = m_TextureSharedItem;
		m_DrawPolygon.m_fUVScale = m_fPolygonUVScale;
		m_DrawPolygon.m_iColor = m_cPolygonColor.PackToInt();
		
		m_LinePolygon.m_pTexture = m_TextureBorderSharedItem;
		m_LinePolygon.m_UVScale = Vector(1, m_fPolygonBorderUVScale, 0.01);
		m_LinePolygon.m_iColor = m_cPolygonBorderColor.PackToInt();
		m_LinePolygon.m_fWidth = m_fPolygonBorderWidth;
		m_LinePolygon.m_bShouldEnclose = !m_bLineMode;
		
		m_wCanvasWidget.SetDrawCommands(m_MapDrawCommands);
		//GetGame().GetCallqueue().CallLater(Update, 0, true);
		SetEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	
	void DeleteMapWidget(MapConfiguration mapConfig)
	{
		//GetGame().GetCallqueue().Remove(Update);
		ClearEventMask(GetOwner(), EntityEvent.POSTFRAME);
	}
	
	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		m_DrawPolygon.m_Vertices = new array<float>();
		m_LinePolygon.m_Vertices = new array<float>();
		float screenXold, screenYold;
		for (int i = 0; i < m_aPolygon.Count(); i += 2)
		{
			float screenX, screenY;
			m_MapEntity.WorldToScreen(m_aPolygon[i], m_aPolygon[i+1], screenX, screenY, true);
			if ((Math.AbsFloat(screenXold - screenX) + Math.AbsFloat(screenYold - screenY)) < 2.1)
			{
				continue;
			}
			if (m_bReversed && (i == 0 || i == 2))
			{
				screenX += 0.1;
			}
			screenXold = screenX;
			screenYold = screenY;
						
			m_DrawPolygon.m_Vertices.Insert(screenX);
			m_DrawPolygon.m_Vertices.Insert(screenY);
			if (!m_bReversed || i > 10)
			{
				m_LinePolygon.m_Vertices.Insert(screenX);
				m_LinePolygon.m_Vertices.Insert(screenY);
			}
		}
	}
}