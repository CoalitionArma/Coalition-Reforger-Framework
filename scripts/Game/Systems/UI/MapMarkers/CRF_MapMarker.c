modded class SCR_MapMarkersUI
{
	static SCR_MapEntity m_MapUnitEntity;
	static bool m_isMapOpen = false;
	
	protected ref array<float> m_aStoredTimeSliceArray = {};
	protected ref array<vector> m_aStoredPosArray = {};
	protected ref map<Widget, string> m_mStoredWidgetSettingsMap = new map<Widget, string>;
	
	//------------------------------------------------------------------------------------------------
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		if (!m_isMapOpen)
			m_isMapOpen = true;
		
		GetGame().GetCallqueue().CallLater(AddInitialMarkers, 0, true);
	}
	
	//------------------------------------------------------------------------------------------------
	void AddInitialMarkers()
	{
		array<string> storedMarkerArray = CRF_ClientComponent.GetInstance().GetScriptedMarkersArray();
		
		if(SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			GetGame().GetCallqueue().Remove(AddInitialMarkers);
		
		if(!storedMarkerArray || storedMarkerArray.IsEmpty())
			return;
		
		GetGame().GetCallqueue().Remove(AddInitialMarkers);
		
		foreach(Widget marker, string s : m_mStoredWidgetSettingsMap)
			delete marker;
		
		m_mStoredWidgetSettingsMap.Clear();
		
		foreach(int i, string markerString : storedMarkerArray)
		{	
			TStringArray markerStringArray = {};
			markerString.Split("||", markerStringArray, false);
			
			Widget widget = GetGame().GetWorkspace().CreateWidgets("{DD15734EB89D74E2}UI/layouts/Map/MapMarkerBase.layout", m_RootWidget);
			widget.SetZOrder(markerStringArray[5].ToInt());
		
			if (!widget)
				return;

			ImageWidget m_UnitImage = ImageWidget.Cast(widget.FindAnyWidget("MarkerIcon"));		
			if(m_UnitImage)
			{
				m_UnitImage.LoadImageTexture(0, markerStringArray[4]);
				m_UnitImage.SetColorInt(markerStringArray[6].ToInt());
				m_UnitImage.SetVisible(true);
			};
		
			TextWidget m_UnitText = TextWidget.Cast(widget.FindAnyWidget("MarkerText"));
			if(m_UnitText)
			{
				m_UnitText.SetText(markerStringArray[3]);
				m_UnitText.SetVisible(true);
			};
		
			m_mStoredWidgetSettingsMap.Set(widget, markerString);
		};
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
		
		int i = 0;
		
		foreach(Widget marker, string markerString : m_mStoredWidgetSettingsMap)
		{
			vector pos;
			
			TStringArray markerStringArray = {};
			markerString.Split("||", markerStringArray, false);
			
			if(!markerStringArray || markerStringArray.IsEmpty() || !marker)
				continue;
			
			if(markerStringArray[0] != "Static Marker") {
				IEntity entity = GetGame().GetWorld().FindEntityByName(markerStringArray[0]);
				if (!entity)
					continue;
			
				if(markerStringArray[2].ToFloat() > 0) {
					float ts = GetGame().GetWorld().GetWorldTime();
				
					if (!m_aStoredTimeSliceArray.IsIndexValid(i) || ts >= m_aStoredTimeSliceArray.Get(i)) {
						float finalTimeDelay = ts + (markerStringArray[2].ToFloat() * 1000);
					
						if (m_aStoredTimeSliceArray.IsIndexValid(i))
							m_aStoredTimeSliceArray.Set(i, finalTimeDelay);
						else
							m_aStoredTimeSliceArray.Insert(finalTimeDelay);
					
						pos = entity.GetOrigin() + markerStringArray[1].ToVector();
					
						if (m_aStoredPosArray.IsIndexValid(i))
							m_aStoredPosArray.Set(i, pos);
						else
							m_aStoredPosArray.Insert(pos);
					} else {
						if (m_aStoredPosArray.IsIndexValid(i))
							pos = m_aStoredPosArray.Get(i);
						else
							pos = entity.GetOrigin() + markerStringArray[1].ToVector();
					};
				} else {
					pos = entity.GetOrigin() + markerStringArray[1].ToVector();
				};
			} else {
				pos = markerStringArray[1].ToVector();
			};
			
			int screenPosX;
			int screenPosY;
		
			m_MapUnitEntity.WorldToScreen(pos[0], pos[2], screenPosX, screenPosY, true);
		
			screenPosX = GetGame().GetWorkspace().DPIUnscale(screenPosX);
			screenPosY = GetGame().GetWorkspace().DPIUnscale(screenPosY);
				
			FrameSlot.SetPos(
				marker,
				screenPosX,
				screenPosY
			);
			
			i = i + 1;
		};
	}
}