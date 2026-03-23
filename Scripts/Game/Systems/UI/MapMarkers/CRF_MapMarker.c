modded class SCR_MapMarkersUI
{
	protected CRF_PlayerScriptedMarkerManager m_PlayerScriptedMarkerManager;
	// Map entity reference
	static SCR_MapEntity m_MapEntity;
	// Flag to track if the map is currently open
	static bool m_bIsMapOpen = false;
	
	// Array to store time values for marker updates
	protected ref array<float> m_aMarkerUpdateTimes = {};
	// Array to store cached positions for markers
	protected ref array<vector> m_aCachedPositions = {};
	// Map storing marker widgets and their associated data
	protected ref map<Widget, string> m_mMarkerWidgetData = new map<Widget, string>;
	
	//------------------------------------------------------------------------------------------------
	// Called when the map is opened
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		
		m_PlayerScriptedMarkerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		
		if (!CRF_Gamemode.GetInstance() || !m_PlayerScriptedMarkerManager)
			return;
		
		// Update map open status
		if (!m_bIsMapOpen)
			m_bIsMapOpen = true;
		
		// Schedule marker initialization
		GetGame().GetCallqueue().Call(LoadStoredMarkers);
		m_PlayerScriptedMarkerManager.GetOnMarkerUpdate().Insert(LoadStoredMarkers);
	}
	
	//------------------------------------------------------------------------------------------------
	// Loads and creates markers from player's stored data
	void LoadStoredMarkers()
	{
		// Do not create widgets when the map is not open
		if (!m_bIsMapOpen)
			return;
		
		// Get stored marker data from player controller
		array<string> markerDataArray = m_PlayerScriptedMarkerManager.GetScriptedMarkersArray();
		
		// Return if no markers to display
		if(!markerDataArray || markerDataArray.IsEmpty())
			return;
		
		// Clean up any existing markers
		CleanupExistingMarkers();
		
		// Create each marker from the stored data
		foreach(int index, string markerData : markerDataArray)
		{	
			CreateMarkerFromData(markerData);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Cleans up existing marker widgets
	protected void CleanupExistingMarkers()
	{
		foreach(Widget markerWidget, string data : m_mMarkerWidgetData)
		{
			delete markerWidget;
		}
		
		m_mMarkerWidgetData.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	// Creates a single marker from the provided data string
	protected void CreateMarkerFromData(string markerData)
	{
		// Parse marker data
		TStringArray markerProperties = {};
		markerData.Split("||", markerProperties, false);
		
		// Create marker widget
		Widget markerWidget = GetGame().GetWorkspace().CreateWidgets("{DD15734EB89D74E2}UI/layouts/Map/MapMarkerBase.layout", m_RootWidget);
		
		if (!markerWidget)
			return;
			
		// Set z-order from data
		markerWidget.SetZOrder(markerProperties[5].ToInt());

		// Setup marker icon
		ImageWidget markerIcon = ImageWidget.Cast(markerWidget.FindAnyWidget("MarkerIcon"));		
		if(markerIcon)
		{
			markerIcon.LoadImageTexture(0, markerProperties[4]);
			markerIcon.SetColorInt(markerProperties[6].ToInt());
			markerIcon.SetVisible(true);
		}
	
		// Setup marker text
		TextWidget markerText = TextWidget.Cast(markerWidget.FindAnyWidget("MarkerText"));
		if(markerText)
		{
			markerText.SetText(markerProperties[3]);
			markerText.SetVisible(true);
		}
	
		// Store widget with its data
		m_mMarkerWidgetData.Set(markerWidget, markerData);
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when the map is closed
	override protected void OnMapClose(MapConfiguration config)
	{
		m_bIsMapOpen = false;
		
		// Remove the marker update listener so updates don't create widgets after map close
		if (m_PlayerScriptedMarkerManager)
			m_PlayerScriptedMarkerManager.GetOnMarkerUpdate().Remove(LoadStoredMarkers);
		
		// Cancel any pending deferred LoadStoredMarkers call
		GetGame().GetCallqueue().Remove(LoadStoredMarkers);
		
		// Clean up marker widgets before parent is destroyed
		CleanupExistingMarkers();
		
		super.OnMapClose(config);
	}
	
	//------------------------------------------------------------------------------------------------
	// Updates marker positions on the map
	override void Update(float timeSlice)
	{
		super.Update(timeSlice);

		// Skip update if map is closed
		if (!m_bIsMapOpen)
			return;
		
		// Get map entity
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity) 
			return;
		
		int markerIndex = 0;
		
		// Update each marker's position
		foreach(Widget markerWidget, string markerData : m_mMarkerWidgetData)
		{
			// Skip invalid markers
			if(!markerWidget)
			{
				markerIndex++;
				continue;
			}
				
			// Parse marker data
			TStringArray markerProperties = {};
			markerData.Split("||", markerProperties, false);
			
			if(!markerProperties || markerProperties.IsEmpty())
			{
				markerIndex++;
				continue;
			}
			
			// Calculate marker position
			vector markerPosition = GetMarkerPosition(markerProperties, markerIndex);
			
			// Update marker position on screen
			UpdateMarkerScreenPosition(markerWidget, markerPosition);
			
			markerIndex++;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Calculates marker position based on marker properties
	protected vector GetMarkerPosition(array<string> markerProperties, int markerIndex)
	{
		vector position;
		
		// Static markers use direct position
		if(markerProperties[0] == "Static Marker") 
		{
			position = markerProperties[1].ToVector();
			return position;
		}
		
		// Entity-based markers
		IEntity entity = GetGame().GetWorld().FindEntityByName(markerProperties[0]);
		if (!entity)
			return vector.Zero;
		
		float updateInterval = markerProperties[2].ToFloat();
		vector offset = markerProperties[1].ToVector();
		
		// If marker has an update interval, handle caching logic
		if(updateInterval > 0) 
		{
			float currentTime = GetGame().GetWorld().GetWorldTime();
			
			// Check if it's time to update the cached position
			if (!m_aMarkerUpdateTimes.IsIndexValid(markerIndex) || currentTime >= m_aMarkerUpdateTimes.Get(markerIndex)) 
			{
				float nextUpdateTime = currentTime + (updateInterval * 1000);
				
				// Store next update time
				if (m_aMarkerUpdateTimes.IsIndexValid(markerIndex))
					m_aMarkerUpdateTimes.Set(markerIndex, nextUpdateTime);
				else
					m_aMarkerUpdateTimes.Insert(nextUpdateTime);
				
				// Calculate and cache new position
				position = entity.GetOrigin() + offset;
				
				if (m_aCachedPositions.IsIndexValid(markerIndex))
					m_aCachedPositions.Set(markerIndex, position);
				else
					m_aCachedPositions.Insert(position);
			} 
			else 
			{
				// Use cached position if available
				if (m_aCachedPositions.IsIndexValid(markerIndex))
					position = m_aCachedPositions.Get(markerIndex);
				else
					position = entity.GetOrigin() + offset;
			}
		} 
		else 
		{
			// No update interval, get current position
			position = entity.GetOrigin() + offset;
		}
		
		return position;
	}
	
	//------------------------------------------------------------------------------------------------
	// Updates marker widget position on screen based on world position
	protected void UpdateMarkerScreenPosition(Widget marker, vector worldPos)
	{
		int screenX;
		int screenY;
		
		// Convert world position to screen coordinates
		m_MapEntity.WorldToScreen(worldPos[0], worldPos[2], screenX, screenY, true);
		
		// Apply DPI scaling
		screenX = GetGame().GetWorkspace().DPIUnscale(screenX);
		screenY = GetGame().GetWorkspace().DPIUnscale(screenY);
			
		// Set widget position
		FrameSlot.SetPos(marker, screenX, screenY);
	}
}