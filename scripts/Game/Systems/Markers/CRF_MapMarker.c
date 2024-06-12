class CRF_MapMarkerComponent: SCR_MapUIBaseComponent
{
	const string XX_MISSION_LAYOUT = "{DD15734EB89D74E2}UI/layouts/Map/MapMarkerBase.layout";
	static SCR_MapEntity m_MapUnitEntity;
	static bool m_isMapOpen = false;
	static ref Widget m_widget;
	
	// constructor vars
	string m_sMarkerName;
	IEntity m_eObjectiveEnt;
	string m_sMarkerText;
	int m_iUpdateRate;
	
	void CRF_MapMarkerComponent(string markerName, string objectiveName, string markertext, int markerUpdate)
	{
		m_sMarkerName = markerName;
		m_eObjectiveEnt = GetGame().GetWorld().FindEntityByName(objectiveName);
		m_sMarkerText = markertext;
		m_iUpdateRate = markerUpdate;
		PrintFormat("CRF %1\n%2\n%3\n%4",m_sMarkerName,m_eObjectiveEnt,m_sMarkerText,m_iUpdateRate);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		Print("CRF Map Open");
		
		if (!m_isMapOpen)
		{
			CreateMarker();
			m_isMapOpen = true;
		}
	}
	//------------------------------------------------------------------------------------------------
	override protected void OnMapClose(MapConfiguration config)
	{
		m_isMapOpen = false;
		super.OnMapClose(config);
		
		// Delete marker when closed
	}
	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);

		if (!m_isMapOpen)
			return;
		
		m_MapUnitEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapUnitEntity) 
			return;

		int screenPosX;
		int screenPosY;		
			
		vector pos = m_eObjectiveEnt.GetOrigin();
		PrintFormat("CRF Pos: %1",pos);
		
		m_MapUnitEntity.WorldToScreen(pos[0], pos[2], screenPosX, screenPosY, true);
		ImageWidget m_UnitImage = ImageWidget.Cast(m_widget.FindAnyWidget("Image"));
		
		screenPosX = GetGame().GetWorkspace().DPIUnscale(screenPosX);
		screenPosY = GetGame().GetWorkspace().DPIUnscale(screenPosY);
				
		FrameSlot.SetPos(
				m_widget, 
				screenPosX,
				screenPosY
		);
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateMarker()
	{
		m_widget = GetGame().GetWorkspace().CreateWidgets(XX_MISSION_LAYOUT, m_RootWidget);
		
		if (!m_widget) 
			return;

		ImageWidget m_UnitImage = ImageWidget.Cast(m_widget.FindAnyWidget("MarkerIcon"));		
		if(m_UnitImage)
		{
			m_UnitImage.LoadImageTexture(0, m_sMarkerName);
			//m_UnitImage.SetColor(Color.FromRGBA(255, 0, 0, 255));
			m_UnitImage.SetVisible(true);
		}
		
		TextWidget m_UnitText = TextWidget.Cast(m_widget.FindAnyWidget("MarkerText"));
		if(m_UnitText)
		{
			m_UnitText.SetText(m_sMarkerText);
			//m_UnitText.SetColor(Color.FromRGBA(0, 255, 0, 255));
			m_UnitText.SetVisible(true);
		}		
	}
}