class CRF_MissionHelper {

	//------------------------------------------------------------------------------------------------
	static vector GetAOCenter()
	{
		CRF_RespawnManager respawnMan = CRF_RespawnManager.GetInstance();
		//We are cooked
		if (!respawnMan)
			return "0 0 0";

		vector spawnPointLocation[4];
		array<string> facKey = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		vector registeredPosition[4] = {"0 0 0", "0 0 0", "0 0 0", "0 0 0"};

		foreach(int i, FactionKey factionKey : facKey)
		{
			respawnMan.FindSpawnPointLocation(factionKey, spawnPointLocation);
			registeredPosition[i] = spawnPointLocation[3];
		};

		return ComputeAOCenter(registeredPosition);
	};

	//------------------------------------------------------------------------------------------------
	static vector ComputeAOCenter(vector pts[4])
	{
		vector sum = "0 0 0";
		int count = 0;

		for (int i = 0; i < 4; i++)
		{
			vector p = pts[i];
			if (p[0] == 0 && p[1] == 0 && p[2] == 0)   // ignore empty
				continue;

			sum += p;
			count++;
		}

		if (count == 1)
			return pts[0];   // only one point

		if (count == 0)
			return "0 0 0";   // no data

		return sum / count;
	}

	//------------------------------------------------------------------------------------------------
	static float ComputeAORadius(vector pts[4], vector center)
	{
		float maxDist = 0;

		for (int i = 0; i < 4; i++)
		{
			vector p = pts[i];
			if (p[0] == 0 && p[1] == 0 && p[2] == 0)
				continue;

			float d = vector.Distance(center, p);
			if (d > maxDist)
				maxDist = d;
		}

		return maxDist;
	}

	//------------------------------------------------------------------------------------------------
	static string SanitizeMissionName(string fullName)
	{
	    array<string> parts = {};

	    fullName.Split(" ", parts, true);

	    // Remove the first two tokens like "CRF" and "CO50"/"COTVT55"
	    if (parts.Count() > 2)
	    {
	        string cleanName;
	        for (int i = 2; i < parts.Count(); i++)
	        {
	            if (i > 2)
	                cleanName += " ";
	            cleanName += parts[i];
	        }
			cleanName.ToUpper();
	        return cleanName;
	    }

		fullName.ToUpper();
	    return fullName; // fallback if unexpected format
	}
}