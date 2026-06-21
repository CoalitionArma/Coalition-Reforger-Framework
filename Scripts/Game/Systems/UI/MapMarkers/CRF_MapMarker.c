modded class SCR_MapMarkersUI
{
	protected const int CRF_DOUBLE_CLICK_MS = 350;
	protected const float CRF_DOUBLE_CLICK_WORLD_TOLERANCE = 12.5;

	protected CRF_PlayerScriptedMarkerManager m_PlayerScriptedMarkerManager;
	protected float m_fLastMapSelectTime = -1;
	protected float m_fLastMapClickWorldX;
	protected float m_fLastMapClickWorldY;

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
		ResetDoubleClickState();
		
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
		if (markerProperties.Count() < 7)
			return;
		
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
		ResetDoubleClickState();
		
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
			
			if(!markerProperties || markerProperties.Count() < 7)
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

	//------------------------------------------------------------------------------------------------
	// Disable quick radial marker menu opening. Marker placement is handled by double-left-click.
	override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
	{
		return;
	}

	//------------------------------------------------------------------------------------------------
	// Double-left-click on empty map opens marker placement dialog at click position.
	override protected void OnInputMapSelect(float value, EActionTrigger reason)
	{
		super.OnInputMapSelect(value, reason);

		if (m_MarkerEditRoot)
		{
			ResetDoubleClickState();
			return;
		}

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager || !inputManager.IsUsingMouseAndKeyboard())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		if (!m_CursorModule)
			return;

		if ((m_CursorModule.GetCursorState() & SCR_MapCursorModule.STATE_POPUP_RESTRICTED) != 0)
			return;

		if ((m_CursorModule.GetCursorState() & EMapCursorState.CS_PAN) != 0)
			return;

		if (IsMarkerUnderCursor())
		{
			ResetDoubleClickState();
			return;
		}

		float worldX, worldY;
		mapEntity.GetMapCursorWorldPosition(worldX, worldY);

		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float now = world.GetWorldTime();
		bool isDoubleClick = false;

		if (m_fLastMapSelectTime >= 0)
		{
			float deltaTime = now - m_fLastMapSelectTime;
			float dx = worldX - m_fLastMapClickWorldX;
			float dy = worldY - m_fLastMapClickWorldY;
			float distSq = dx * dx + dy * dy;
			float toleranceSq = CRF_DOUBLE_CLICK_WORLD_TOLERANCE * CRF_DOUBLE_CLICK_WORLD_TOLERANCE;

			isDoubleClick = deltaTime <= CRF_DOUBLE_CLICK_MS && distSq <= toleranceSq;
		}

		if (!isDoubleClick)
		{
			m_fLastMapSelectTime = now;
			m_fLastMapClickWorldX = worldX;
			m_fLastMapClickWorldY = worldY;
			return;
		}

		ResetDoubleClickState();

		float screenX, screenY;
		mapEntity.WorldToScreen(worldX, worldY, screenX, screenY);
		mapEntity.PanSmooth(screenX, screenY, 0.05);

		if (!m_PlacedMarkerConfig)
		{
			if (!m_MarkerMgr)
				return;

			SCR_MapMarkerConfig markerConfig = m_MarkerMgr.GetMarkerConfig();
			if (!markerConfig)
				return;

			m_PlacedMarkerConfig = SCR_MapMarkerEntryPlaced.Cast(markerConfig.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
			if (!m_PlacedMarkerConfig)
				return;
		}

		CreateMarkerEditDialog();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMarkerUnderCursor()
	{
		array<Widget> widgets = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		foreach (Widget widget : widgets)
		{
			SCR_MapMarkerWidgetComponent markerComp = SCR_MapMarkerWidgetComponent.Cast(widget.FindHandler(SCR_MapMarkerWidgetComponent));
			if (markerComp)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void ResetDoubleClickState()
	{
		m_fLastMapSelectTime = -1;
		m_fLastMapClickWorldX = 0;
		m_fLastMapClickWorldY = 0;
	}
}
