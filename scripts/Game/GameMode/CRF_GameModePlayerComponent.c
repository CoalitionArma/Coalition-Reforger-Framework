[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GameModePlayerComponentClass: ScriptComponentClass
{
	
}

class CRF_GameModePlayerComponent: ScriptComponent
{	
  ref array<string> m_aScriptedMarkers = {};
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	static CRF_GameModePlayerComponent GetInstance()
	{
		if (GetGame().GetPlayerController())
			return CRF_GameModePlayerComponent.Cast(GetGame().GetPlayerController().FindComponent(CRF_GameModePlayerComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------

	// Functions for ready up replication
	
	//------------------------------------------------------------------------------------------------
	
	void Owner_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{	
		Rpc(RpcAsk_ToggleBombPlanted, sitePlanted, togglePlanted);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_ToggleBombPlanted(string sitePlanted, bool togglePlanted)
	{
		CRF_SearchAndDestroyGameModeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_SearchAndDestroyGameModeComponent)).ToggleBombPlanted(sitePlanted, togglePlanted);
	}
	
	//------------------------------------------------------------------------------------------------

	// Functions for scripted markers
	
	//------------------------------------------------------------------------------------------------
	
	TStringArray GetScriptedMarkersArray()
	{
		return m_aScriptedMarkers;
	}
	
	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Adds a scripted marker on the users map which will follow the specified entity
	//! \param[in] markerEntityName is the name of the entity the marker will track.
	//! \param[in] markerOffset is the offset from the marker entity. (This can also be the vector pos for a static marker, simply set the "markerEntityName" param to "Static Marker").
	//! \param[in] timeDelay is the delay between marker updates.
	//! \param[in] markerImage is the image that will be displayed on the map.
	//! \param[in] markerText is the text that will be displayed on the map just under the image.
	void AddScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5", markerEntityName, markerOffset.ToString(), timeDelay.ToString(), markerText, markerImage));
	}

	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Removes the scripted marker from the users map, must have all params be the same params that were initially put in the AddScriptedMarkers function
	void RemoveScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage)
	{
		m_aScriptedMarkers.RemoveItem(string.Format("%1||%2||%3||%4||%5", markerEntityName, markerOffset.ToString(), timeDelay.ToString(), markerText, markerImage));
	}
}