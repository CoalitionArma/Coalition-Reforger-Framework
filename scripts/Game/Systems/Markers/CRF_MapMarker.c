modded class SCR_MapUIBaseComponent
{
	static SCR_MapEntity m_MapUnitEntity;
	static bool m_isMapOpen = false;
	
	protected ref array<Widget> m_aStoredWidgetArray = {};
	protected ref array<float> m_aStoredTimeSliceArray = {};
	protected ref array<vector> m_aStoredPosArray = {};
	protected CRF_GameModePlayerComponent m_GameModePlayerComponent;
	
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		if (!m_isMapOpen)
			m_isMapOpen = true;
	}
	//------------------------------------------------------------------------------------------------
	override protected void OnMapClose(MapConfiguration config)
	{
		m_isMapOpen = false;
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
		
		TStringArray markerArray = m_GameModePlayerComponent.GetScriptedMarkersArray();
		
		if(markerArray.Count() != m_aStoredWidgetArray.Count())
		{
			foreach(int i, Widget marker : m_aStoredWidgetArray)
			{
				delete marker;
				m_aStoredWidgetArray.Remove(i);
			};
		};
		
		foreach(int i, string markerString : markerArray)
		{
			TStringArray markerStringArray = {};
			markerString.Split("||", markerStringArray, false);
			
			SetMarker(i, markerStringArray[0], markerStringArray[1].ToVector(), markerStringArray[2].ToFloat(), markerStringArray[3], markerStringArray[4]);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void SetMarker(int markerInt, string markerEntityName, vector markerOffset, float timeDelay, string markerText, string markerImage) 
	{
		vector pos;
		
		if(markerEntityName != "Static Marker") {
			IEntity entity = GetGame().GetWorld().FindEntityByName(markerEntityName);
			if (!entity)
				return;
			
			if(timeDelay > 0) {
				float ts = GetGame().GetWorld().GetWorldTime();
				
				if (!m_aStoredTimeSliceArray.IsIndexValid(markerInt) || ts >= m_aStoredTimeSliceArray.Get(markerInt)) {
					float finalTimeDelay = ts + (timeDelay * 1000);
					
					if (m_aStoredTimeSliceArray.IsIndexValid(markerInt))
						m_aStoredTimeSliceArray.Set(markerInt, finalTimeDelay);
					else
						m_aStoredTimeSliceArray.Insert(finalTimeDelay);
					
					pos = entity.GetOrigin() + markerOffset;
					
					if (m_aStoredPosArray.IsIndexValid(markerInt))
						m_aStoredPosArray.Set(markerInt, pos);
					else
						m_aStoredPosArray.Insert(pos);
					
				} else {
					if (m_aStoredPosArray.IsIndexValid(markerInt))
						pos = m_aStoredPosArray.Get(markerInt);
					else
						pos = entity.GetOrigin() + markerOffset;
				};
			} else {
				pos = entity.GetOrigin() + markerOffset;
			};
		} else {
			pos = markerOffset;
		};
			
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
		
		if (m_aStoredWidgetArray.IsIndexValid(markerInt))
		{
			delete m_aStoredWidgetArray.Get(markerInt);
			m_aStoredWidgetArray.Set(markerInt, widget);
		} else {
			m_aStoredWidgetArray.Insert(widget);
		}
		
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