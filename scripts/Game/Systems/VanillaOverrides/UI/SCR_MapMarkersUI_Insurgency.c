modded class SCR_MapMarkersUI
{
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
	
	// Insurgency gamemode reference
	protected CRF_InsurgencyGamemodeManager m_InsurgencyGamemode;
	
	//------------------------------------------------------------------------------------------------
	// Called when the map is opened
	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		
		if (!CRF_Gamemode.GetInstance())
			return;
		
		// Get insurgency gamemode reference
		m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
		
		// Update map open status
		if (!m_bIsMapOpen)
			m_bIsMapOpen = true;
		
		// Schedule marker initialization
		GetGame().GetCallqueue().Call(LoadStoredMarkers);
	}
	
	//------------------------------------------------------------------------------------------------
	// Loads and creates markers from player's stored data
	override void LoadStoredMarkers()
	{
		// Get stored marker data from player controller
		array<string> markerDataArray = CRF_PlayerControllerManager.GetInstance().GetScriptedMarkersArray();
		
		// If game is running, remove this function from call queue
		if(SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			GetGame().GetCallqueue().Remove(LoadStoredMarkers);
		
		// Return if no markers to display
		if(!markerDataArray)
			return;
			
		if(markerDataArray.IsEmpty())
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
	override protected void CleanupExistingMarkers()
	{
		foreach(Widget markerWidget, string data : m_mMarkerWidgetData)
		{
			delete markerWidget;
		}
		
		m_mMarkerWidgetData.Clear();
		m_aMarkerUpdateTimes.Clear();
		m_aCachedPositions.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	// Creates a single marker from the provided data string
	override protected void CreateMarkerFromData(string markerData)
	{
		// Parse marker data
		TStringArray markerProperties = {};
		markerData.Split("||", markerProperties, false);
		
		// Need at least 7 properties for standard format
		if (markerProperties.Count() < 7)
			return;
		
		// Extract metadata from entity name
		string entityName = markerProperties[0];
		string markerType = "";
		int markerPhase = 0;
		
		// Parse entity name for metadata (format: EntityName_TYPE_PHASEXX)
		array<string> nameParts = {};
		entityName.Split("_", nameParts, false);
		
		// Look for TYPE and PHASE markers in the name
		foreach (int i, string part : nameParts)
		{
			if (part == "CACHE")
				markerType = "CACHE";
			else if (part == "POLYZONE")
				markerType = "POLYZONE";
			else if (part.Contains("PHASE"))
			{
				// Extract phase number (e.g., "PHASE1" -> 1)
				string phaseStr = part;
				phaseStr.Replace("PHASE", "");
				markerPhase = phaseStr.ToInt();
			}
		}
		
		// Check visibility based on marker type and phase
		if (markerType != "" && !ShouldMarkerBeVisible(markerType, markerPhase, entityName))
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
	/**
	 * Check if a marker should be visible based on type, phase, and game state
	 */
	protected bool ShouldMarkerBeVisible(string markerType, int markerPhase, string entityName)
	{
		// If no marker type specified, show marker (legacy markers)
		if (markerType == "")
			return true;
		
		if (!m_InsurgencyGamemode)
			return true; // If not insurgency mode, show all markers
		
		// Get player's faction
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return false;
		
		SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction playerFaction = factionComp.GetAffiliatedFaction();
		if (!playerFaction)
			return false;
		
		FactionKey playerFactionKey = playerFaction.GetFactionKey();
		bool isAttacker = (playerFactionKey == m_InsurgencyGamemode.m_AttackingSide);
		bool isDefender = (playerFactionKey == m_InsurgencyGamemode.m_DefendingSide);
		
		// Handle cache markers (defenders only)
		if (markerType == "CACHE")
		{
			// Attackers never see cache markers
			if (isAttacker)
				return false;
			
			// Defenders only see cache markers if cache is active
			if (isDefender)
			{
				// Check if the cache entity exists and is active
				IEntity cacheEntity = GetGame().GetWorld().FindEntityByName(entityName);
				if (!cacheEntity)
					return false;
				
				CRF_InsDestructiveComponent cacheComp = CRF_InsDestructiveComponent.Cast(
					cacheEntity.FindComponent(CRF_InsDestructiveComponent));
				
				if (!cacheComp)
					return false;
				
				// Only show if cache is active and not destroyed
				return cacheComp.IsCacheActive() && !cacheComp.IsCacheDestroyed();
			}
			
			return false;
		}
		
		// Handle polyzone markers (attackers only)
		if (markerType == "POLYZONE")
		{
			// Defenders never see polyzone markers
			if (isDefender)
				return false;
			
			// Attackers only see polyzone markers when phase is active and revealed
			if (isAttacker)
			{
				int currentPhase = m_InsurgencyGamemode.GetCurrentPhase();
				
				// Zone must be for current phase
				if (markerPhase != currentPhase)
					return false;
				
				// Zone must be revealed
				return m_InsurgencyGamemode.AreCurrentPhaseZonesRevealed();
			}
			
			return false;
		}
		
		// For other marker types, show normally
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when the map is closed
	override protected void OnMapClose(MapConfiguration config)
	{
		m_bIsMapOpen = false;
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
		
		// Update each marker's position and visibility
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
			
			// Extract metadata from entity name for visibility check
			string entityName = markerProperties[0];
			string markerType = "";
			int markerPhase = 0;
			
			// Parse entity name for metadata (format: EntityName_TYPE_PHASEXX)
			array<string> nameParts = {};
			entityName.Split("_", nameParts, false);
			
			// Look for TYPE and PHASE markers in the name
			foreach (string part : nameParts)
			{
				if (part == "CACHE")
					markerType = "CACHE";
				else if (part == "POLYZONE")
					markerType = "POLYZONE";
				else if (part.Contains("PHASE"))
				{
					// Extract phase number (e.g., "PHASE1" -> 1)
					string phaseStr = part;
					phaseStr.Replace("PHASE", "");
					markerPhase = phaseStr.ToInt();
				}
			}
			
			// Check visibility for phase-based markers
			if (markerType != "")
			{
				// Get the actual entity name (strip metadata suffix)
				string actualEntityName = GetActualEntityName(entityName);
				
				if (!ShouldMarkerBeVisible(markerType, markerPhase, actualEntityName))
				{
					// Hide marker but don't delete it
					markerWidget.SetVisible(false);
					markerIndex++;
					continue;
				}
				else
				{
					// Show marker
					markerWidget.SetVisible(true);
				}
			}
			
			// Calculate marker position
			vector markerPosition = GetMarkerPosition(markerProperties, markerIndex);
			
			// Update marker position on screen
			UpdateMarkerScreenPosition(markerWidget, markerPosition);
			
			markerIndex++;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get actual entity name by stripping metadata suffix
	 * @param entityNameWithMetadata - Entity name with _TYPE_PHASEXX suffix
	 * @return Actual entity name
	 */
	protected string GetActualEntityName(string entityNameWithMetadata)
	{
		// Find first underscore followed by CACHE or POLYZONE
		int cachePos = entityNameWithMetadata.IndexOf("_CACHE");
		int polyzonePos = entityNameWithMetadata.IndexOf("_POLYZONE");
		
		int cutPos = -1;
		if (cachePos != -1)
			cutPos = cachePos;
		else if (polyzonePos != -1)
			cutPos = polyzonePos;
		
		if (cutPos != -1)
			return entityNameWithMetadata.Substring(0, cutPos);
		
		return entityNameWithMetadata;
	}
	
	//------------------------------------------------------------------------------------------------
	// Calculates marker position based on marker properties
	override protected vector GetMarkerPosition(array<string> markerProperties, int markerIndex)
	{
		vector position;
		
		// Static markers use direct position
		if(markerProperties[0] == "Static Marker") 
		{
			position = markerProperties[1].ToVector();
			return position;
		}
		
		// Extract actual entity name (strip metadata)
		string entityNameWithMetadata = markerProperties[0];
		string actualEntityName = GetActualEntityName(entityNameWithMetadata);
		
		// Entity-based markers
		IEntity entity = GetGame().GetWorld().FindEntityByName(actualEntityName);
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
	override protected void UpdateMarkerScreenPosition(Widget marker, vector worldPos)
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
	/**
	 * Force refresh markers (useful when phase changes or cache destroyed)
	 */
	void RefreshInsurgencyMarkers()
	{
		if (!m_bIsMapOpen)
			return;
		
		// Reload all markers from scratch
		LoadStoredMarkers();
	}
}