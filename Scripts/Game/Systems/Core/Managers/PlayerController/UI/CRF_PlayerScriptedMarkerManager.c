class CRF_PlayerScriptedMarkerManagerClass : ScriptComponentClass {}

class CRF_PlayerScriptedMarkerManager : ScriptComponent
{	
	// Map and Markers
	ref array<string> m_aScriptedMarkers = {};  // Custom map markers

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MARKER MAP MANAGEMENT
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Returns the array of scripted markers
	//! \return TStringArray - Array of scripted marker data strings
	TStringArray GetScriptedMarkersArray()
	{
		return m_aScriptedMarkers;
	}

	//------------------------------------------------------------------------------------------------
	//! Adds a scripted marker on the user's map
	//! \param[in] markerEntityName - Name of entity to track (or "Static Marker" for static position)
	//! \param[in] markerOffset - Position offset from entity or static position
	//! \param[in] timeDelay - Update frequency for entity tracking
	//! \param[in] markerText - Text displayed on map
	//! \param[in] markerImage - Image resource path
	//! \param[in] zOrder - Display order/priority
	//! \param[in] markerColor - ARGB color value
	void AddScriptedMarker(string markerEntityName, string markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset, 
			timeDelay.ToString(), 
			markerText, 
			markerImage, 
			zOrder.ToString(), 
			markerColor.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	//! Removes a specific scripted marker
	//! \param[in] markerEntityName - Name of entity tracked
	//! \param[in] markerOffset - Position offset
	//! \param[in] timeDelay - Update frequency
	//! \param[in] markerText - Text displayed
	//! \param[in] markerImage - Image resource path
	//! \param[in] zOrder - Display order/priority
	//! \param[in] markerColor - ARGB color value
	void RemoveScriptedMarker(string markerEntityName, string markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.RemoveItemOrdered(string.Format("%1||%2||%3||%4||%5||%6||%7", 
			markerEntityName, 
			markerOffset, 
			timeDelay.ToString(), 
			markerText, 
			markerImage, 
			zOrder.ToString(), 
			markerColor.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	//! Removes all scripted markers
	void RemoveALLScriptedMarkers()
	{
		m_aScriptedMarkers.Clear();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerScriptedMarkerManager m_sInstance;
	void CRF_PlayerScriptedMarkerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_PlayerScriptedMarkerManager GetInstance()
	{
		return m_sInstance;
	}
}