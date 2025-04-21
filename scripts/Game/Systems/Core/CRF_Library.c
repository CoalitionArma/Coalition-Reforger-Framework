class CRF_LibraryClass : ScriptComponentClass {}

class CRF_Library : ScriptComponent
{
	//------------------------------------------------------------------------------------------------
	/*!
	Check if given player is an moderator.
	\param playerId ID of queried player
	\return True when player with given ID is an moderator.
	*/
	static bool IsModerator(int playerId)
	{
		//return GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	Check if local player is an moderator.
	\return True when local player is a moderator.
	*/
	static bool IsModerator()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		//return GetGame().GetPlayerManager().HasPlayerRole(playerId, EPlayerRole.COALITION_MODERATOR);
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool IsSquadLeaderRole(IEntity entity)
	{
		ref TStringArray roles = {"_COY_P","_PL_P","_MO_P","_SL_P","_VehLead_P","_IndirectLead_P","_LogiLead_P"};
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!prefab.Contains("CRF_GS_"))
			return false;

		string role = PrefabToRole(prefab);

		return roles.Contains(role);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsTeamLeaderRole(IEntity entity)
	{
		ref TStringArray roles = {"_TL_P"};
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!prefab.Contains("CRF_GS_"))
			return false;

		string role = PrefabToRole(prefab);

		return roles.Contains(role);
	}

	//------------------------------------------------------------------------------------------------
	static string PrefabToRole(ResourceName prefab)
	{
		array<string> value = {};
		prefab.Split("_", value, true);

		string role = "_" + value[3] + "_" + value[4];

		role.Split(".", value, true);
		role = value[0];

		return role;
	}
};