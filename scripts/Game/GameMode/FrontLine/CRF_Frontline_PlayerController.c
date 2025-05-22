modded class CRF_PlayerControllerManager
{
	/**
	 * Update map markers for objectives in Frontline gamemode
	 * @param zoneStatus - Array of zone status strings
	 * @param zoneObjectNames - Array of zone object names
	 * @param bluforSide - Blue force faction key
	 * @param opforSide - Opposing force faction key
	 */
	void UpdateMapMarkers(array<string> zoneStatus, array<string> zoneObjectNames, FactionKey bluforSide, FactionKey opforSide)
	{
		RemoveALLScriptedMarkers();

		foreach (int i, string zoneName : zoneObjectNames)
		{
			string status = zoneStatus[i];
			string imageTexture;
			int imageColor;

			// Parse zone status
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);

			string zoneLocked = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];

			// Select image based on zone index
			switch (i)
			{
				case 0: {imageTexture = "{21A2A457BD0E42C1}UI\Objectives\A.edds"; break; };
				case 1: {imageTexture = "{7F4A8D140283CCCE}UI\Objectives\B.edds"; break; };
				case 2: {imageTexture = "{8B42CA8C0F5EA4BA}UI\Objectives\C.edds"; break; };
				case 3: {imageTexture = "{C29ADF937D98D0D0}UI\Objectives\D.edds"; break; };
				case 4: {imageTexture = "{3692980B7045B8A4}UI\Objectives\E.edds"; break; };
			}

			// Add lock marker if zone is locked
			if (zoneLocked == "Locked")
				AddScriptedMarker(zoneName, "0 0 0", 0, "", "{91427B7866707601}UI\Objectives\lock.edds", 50, ARGB(255, 142, 142, 142));

			// Set color based on controlling faction
			switch (zoneFactionStored)
			{
				case bluforSide: {imageColor = ARGB(255, 0, 25, 225); break; }; // Blufor
				case opforSide: {imageColor = ARGB(255, 225, 25, 0); break; }; // Opfor
				default: {imageColor = ARGB(255, 225, 225, 225); break; }; // Uncaptured
			}

			// Add zone marker
			AddScriptedMarker(zoneName, "0 0 0", 0, "", imageTexture, 45, imageColor);
		}
	}
}