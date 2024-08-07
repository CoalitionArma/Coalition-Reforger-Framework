[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GameModePlayerComponentClass: ScriptComponentClass
{
	
}

class CRF_GameModePlayerComponent: ScriptComponent
{	
  ref array<string> m_aScriptedMarkers = new array<string>;
	
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

	// Functions for Search and Destroy
	
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

	// Functions for Frontline
	
	//------------------------------------------------------------------------------------------------
	
	void UpdateMapMarkers(array<string> zoneStatus, array<string> zoneObjectNames, FactionKey bluforSide, FactionKey opforSide)
	{	
		RemoveALLScriptedMarkers();
		
		foreach(int i, string zoneName : zoneObjectNames)
		{
			string status = zoneStatus[i];
			string imageTexture;
			int imageColor;
			
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
			
			string zoneLocked = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];
			
			switch(i)
			{
				case 0 : {imageTexture = "{E5AC2ABE05771CA1}UI\Objectives\A.edds"; break;};
				case 1 : {imageTexture = "{BB4403FDBAFA92AE}UI\Objectives\B.edds"; break;};
				case 2 : {imageTexture = "{4F4C4465B727FADA}UI\Objectives\C.edds"; break;};
				case 3 : {imageTexture = "{0694517AC5E18EB0}UI\Objectives\D.edds"; break;};
				case 4 : {imageTexture = "{F29C16E2C83CE6C4}UI\Objectives\E.edds"; break;};
			}
			
			if(zoneLocked == "Locked")
				AddScriptedMarker(zoneName, "0 0 0", 0, "", "{91427B7866707601}UI\Objectives\lock.edds", 50, ARGB(255, 142, 142, 142));
			
			switch(zoneFactionStored)
			{
				case bluforSide : {imageColor = ARGB(255, 0, 25, 225);     break;};  //Blufor
				case opforSide  : {imageColor = ARGB(255, 225, 25, 0);     break;};  //Opfor
				default         : {imageColor = ARGB(255, 225, 225, 225);  break;};  //Uncaptured
			}
			
			AddScriptedMarker(zoneName, "0 0 0", 0, "", imageTexture, 45, imageColor);
		}
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
	//! \param[in] markerText is the text that will be displayed on the map just under the image.
	//! \param[in] markerImage is the image that will be displayed on the map.
	//! \param[in] markerColor is the color the image will be.
	void AddScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.Insert(string.Format("%1||%2||%3||%4||%5||%6||%7", markerEntityName, markerOffset.ToString(), timeDelay.ToString(), markerText, markerImage, zOrder.ToString(), markerColor.ToString()));
	}

	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Removes the scripted marker from the users map, must have all params be the same params that were initially put in the AddScriptedMarkers function
	void RemoveScriptedMarker(string markerEntityName, vector markerOffset, int timeDelay, string markerText, string markerImage, int zOrder, int markerColor)
	{
		m_aScriptedMarkers.RemoveItemOrdered(string.Format("%1||%2||%3||%4||%5||%6||%7", markerEntityName, markerOffset.ToString(), timeDelay.ToString(), markerText, markerImage, zOrder.ToString(), markerColor.ToString()));
	}
	
	//------------------------------------------------------------------------------------------------
	//! !LOCAL! Removes all scripted markers from the users maps
	void RemoveALLScriptedMarkers()
	{
		m_aScriptedMarkers.Clear();
	}
}