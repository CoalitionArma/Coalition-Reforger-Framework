//------------------------------------------------------------------------------------
// Shared helpers for the special modes (The Turn, Slasher): living players, getting a player out of the
// round alive (extraction), named entities and a client-side set of map markers.
// Popups and faction lookups come from CRF_RadioTransmissionEvents.
//------------------------------------------------------------------------------------
class CRF_SpecialsHelper
{
	//------------------------------------------------------------------------------------------------
	//! Slotted players on factionKey ("" = any faction) with a living, non-spectator body
	static void GetAlivePlayers(string factionKey, notnull array<int> outPlayers)
	{
		outPlayers.Clear();

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			string playerFaction = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			if (playerFaction.IsEmpty() || playerFaction == "SPEC")
				continue;

			if (!factionKey.IsEmpty() && playerFaction != factionKey)
				continue;

			IEntity entity = CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId);
			if (!entity || COA_EntityHelper.IsSpectator(entity))
				continue;

			outPlayers.Insert(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server: takes a player out of the round alive (they escaped). Their slot counts as dead, so they
	//! can't respawn, the body is removed and the Lobby puts them in spectator.
	static bool ExtractPlayer(int playerId)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!slottingManager || !gamemode)
			return false;

		int slotId = slottingManager.GetPlayerSlotID(playerId);
		if (slotId < 0)
			return false;

		COA_SlotData slotData = slottingManager.GetSlotData(slotId);
		if (!slotData)
			return false;

		// Dead first, so nothing hands them a fresh body when the old one goes
		slottingManager.UpdateSlotDeathState(slotId, true);
		slottingManager.CleanupCharacterFromSlot(slotData);
		gamemode.QueuePlayerInitialization(playerId, false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A named world entity, with a warning naming the mode if it's missing
	static IEntity FindNamedEntity(string entityName, string logPrefix)
	{
		if (entityName.IsEmpty())
			return null;

		IEntity entity = GetGame().GetWorld().FindEntityByName(entityName);
		if (!entity)
			Print(string.Format("%1 No entity named '%2' in the world.", logPrefix, entityName), LogLevel.WARNING);

		return entity;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: spawns a prop on the ground at a clear spot near position, so props placed by a mode never
	//! float or sink whatever height their anchor was given. Replicated if the prefab has an RplComponent.
	static IEntity SpawnOnGround(ResourceName prefab, vector position, float yaw, float searchRadius = 6, float clearRadius = 1.5)
	{
		if (prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("[CRF_Specials] Can't load prop '%1'.", prefab), LogLevel.ERROR);
			return null;
		}

		vector spot;
		if (!SCR_WorldTools.FindEmptyTerrainPosition(spot, position, searchRadius, clearRadius, 2))
			spot = position;
		spot[1] = GetGame().GetWorld().GetSurfaceY(spot[0], spot[2]);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), spawnParams.Transform);
		spawnParams.Transform[3] = spot;
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	//------------------------------------------------------------------------------------------------
	//! Map distance (ignoring height) from position to the nearest of the given players, and which one (-1 if none)
	static float DistanceToNearestPlayer(vector position, notnull array<int> playerIds, out int nearestId = -1)
	{
		float best = 1e10;
		nearestId = -1;
		foreach (int playerId : playerIds)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity)
				continue;

			float distance = vector.DistanceXZ(position, entity.GetOrigin());
			if (distance < best)
			{
				best = distance;
				nearestId = playerId;
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	//! 45 -> "0:45"
	static string FormatSeconds(int seconds)
	{
		if (seconds < 0)
			seconds = 0;

		int remainder = seconds % 60;
		return string.Format("%1:%2", seconds / 60, remainder.ToString(2));
	}

	//------------------------------------------------------------------------------------------------
	static string GetFactionName(string factionKey)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (factionManager)
		{
			Faction faction = factionManager.GetFactionByKey(factionKey);
			if (faction && !faction.GetFactionName().IsEmpty())
				return faction.GetFactionName();
		}

		return factionKey;
	}

	//------------------------------------------------------------------------------------------------
	//! The local player's slotted faction, "" if they aren't slotted
	static string GetLocalFactionKey()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return "";

		Faction faction = slottingManager.GetPlayerSlotFaction(SCR_PlayerController.GetLocalPlayerId(), true);
		if (!faction)
			return "";

		return faction.GetFactionKey();
	}
}

//------------------------------------------------------------------------------------------------
//! Client: a set of static scripted map markers kept in step with what a mode wants shown.
//! Each refresh: Begin(), Want() every marker that should be up, then Apply() - markers no longer
//! wanted come down, new ones go up, unchanged ones are left alone.
class CRF_SpecialsMarkerSet
{
	protected ref array<string> m_aShown = {};
	protected ref array<string> m_aWanted = {};

	protected static const string MARKER_ENTITY = "Static Marker";
	protected static const string SEPARATOR = "||";

	//------------------------------------------------------------------------------------------------
	void Begin()
	{
		m_aWanted.Clear();
	}

	//------------------------------------------------------------------------------------------------
	void Want(vector position, string label, ResourceName icon, int color)
	{
		string positionText = string.Format("%1 %2 %3", position[0], position[1], position[2]);
		m_aWanted.Insert(positionText + SEPARATOR + label + SEPARATOR + icon + SEPARATOR + color.ToString());
	}

	//------------------------------------------------------------------------------------------------
	void Apply()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		for (int i = m_aShown.Count() - 1; i >= 0; i--)
		{
			if (m_aWanted.Contains(m_aShown[i]))
				continue;

			Send(markerManager, m_aShown[i], false);
			m_aShown.Remove(i);
		}

		foreach (string key : m_aWanted)
		{
			if (m_aShown.Contains(key))
				continue;

			Send(markerManager, key, true);
			m_aShown.Insert(key);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		m_aWanted.Clear();
		Apply();
		m_aShown.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected void Send(COA_PlayerScriptedMarkerManager markerManager, string key, bool add)
	{
		array<string> parts = {};
		key.Split(SEPARATOR, parts, false);
		if (parts.Count() < 4)
			return;

		if (add)
			markerManager.AddScriptedMarker(MARKER_ENTITY, parts[0], 0, parts[1], parts[2], 50, parts[3].ToInt());
		else
			markerManager.RemoveScriptedMarker(MARKER_ENTITY, parts[0], 0, parts[1], parts[2], 50, parts[3].ToInt());
	}
}
