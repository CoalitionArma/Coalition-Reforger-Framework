modded class SCR_MapUIBaseComponent
{
	static SCR_MapEntity m_MapUnitEntity;
	static bool m_isMapOpen = false;
	
	protected ref array<Widget> m_aStoredWidgetArray = {};
	protected CRF_GameModePlayerComponent m_GameModePlayerComponent;
	
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);		
		m_aStoredWidgetArray.Clear();
		if (!m_isMapOpen)
			m_isMapOpen = true;
	}
	//------------------------------------------------------------------------------------------------
	override protected void OnMapClose(MapConfiguration config)
	{
		m_isMapOpen = false;
		m_aStoredWidgetArray.Clear();
		super.OnMapClose(config);
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
		
		m_GameModePlayerComponent = CRF_GameModePlayerComponent.GetInstance();
		if (!m_GameModePlayerComponent) 
			return;
		
		if(m_aStoredWidgetArray.Count() > 2)
		{
			delete m_aStoredWidgetArray.Get(0);
			m_aStoredWidgetArray.Remove(0);
			
			delete m_aStoredWidgetArray.Get(1);
			m_aStoredWidgetArray.Remove(1);
		}
		
		TStringArray markerArray = m_GameModePlayerComponent.GetScriptedMarkersArray();
		
		foreach(int i, string markerString : markerArray)
		{
			TStringArray markerStringArray = {};
			markerString.Split("||", markerStringArray, false);
			
			int timedelay = markerStringArray[2];
			
			if(timedelay <= 0) {
				SetMarker(i, markerStringArray[0], markerStringArray[1].ToVector(), markerStringArray[3], markerStringArray[4]);
			} else {
				
			};
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void SetMarker(int markerInt, string markerEntityName, vector markerOffset, string markerText, string markerImage) 
	{
		IEntity entity = GetGame().GetWorld().FindEntityByName(markerEntityName);
		if (!entity)
			return;
			
		vector pos = entity.GetOrigin();
		pos = pos + markerOffset;
			
		Widget widget = GetGame().GetWorkspace().CreateWidgets("{DD15734EB89D74E2}UI/layouts/Map/MapMarkerBase.layout", m_RootWidget);
		
		if (!widget)
			return;

		ImageWidget m_UnitImage = ImageWidget.Cast(widget.FindAnyWidget("MarkerIcon"));		
		if(m_UnitImage)
		{
			m_UnitImage.LoadImageTexture(0, markerImage);
			m_UnitImage.SetVisible(true);
		}
		
		TextWidget m_UnitText = TextWidget.Cast(widget.FindAnyWidget("MarkerText"));
		if(m_UnitText)
		{
			m_UnitText.SetText(markerText);
			m_UnitText.SetVisible(true);
		}
		
		if (m_aStoredWidgetArray.Get(markerInt))
		{
			delete m_aStoredWidgetArray.Get(markerInt);
			m_aStoredWidgetArray.Remove(markerInt);
		};
		m_aStoredWidgetArray.InsertAt(widget, markerInt);
		
		int screenPosX;
		int screenPosY;
		
		m_MapUnitEntity.WorldToScreen(pos[0], pos[2], screenPosX, screenPosY, true);
		
		screenPosX = GetGame().GetWorkspace().DPIUnscale(screenPosX);
		screenPosY = GetGame().GetWorkspace().DPIUnscale(screenPosY);
				
		FrameSlot.SetPos(
				widget,
				screenPosX,
				screenPosY
		);
	}
}