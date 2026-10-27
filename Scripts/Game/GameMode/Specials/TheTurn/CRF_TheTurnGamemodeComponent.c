//------------------------------------------------------------------------------------
// CRF_TheTurnGamemodeComponent: "The Turn" - a TvT that turns into a zombie outbreak, on purpose
//
//   1. Normal TvT: the mission plays as written. Radio chatter hints that something is wrong.
//   2. Outbreak (m_fOutbreakMinutes): zombie AI groups (Bacon Zombies by default) come in waves from the
//      spawn points around the area, each wave bigger than the last, heading for the nearest players.
//   3. Extraction (m_fExtractionCallMinutes after the outbreak): one helicopter is called to a landing
//      zone everyone is told about. It has too few seats for everyone, and both sides want them.
//      Players who reach the LZ while it waits take a seat (they're out of the round and spectate)
//      until it's full or it leaves.
//
//   Win: the side with more players aboard. Equal (or nobody) is a draw.
//
// Setup for mission makers
// 1. Build a normal TvT, but without objective components that end the mission on their own.
// 2. Add this component to the gamemode entity and set both faction keys.
// 3. Place a few empty entities around the edge of the area and list their names as zombie spawn points.
// 4. Place and name the landing zone (a helipad works) and set m_sLandingZoneName.
// 5. Factions: make the zombie faction (BACON_622120A5448725E3_FACTION) hostile to both sides.
// 6. Respawns: zombies make long fights deadly; the Lobby's respawn settings still apply.
//------------------------------------------------------------------------------------

enum CRF_ETheTurnPhase
{
	NORMAL,
	OUTBREAK,
	INBOUND,	//!< extraction called, on its way
	BOARDING,	//!< helicopter at the LZ
	ENDED
}

//------------------------------------------------------------------------------------------------
//! A radio message at a set time
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sTitle")]
class CRF_TheTurnMessage
{
	[Attribute("10", UIWidgets.Auto, "Minutes into the round (after safestart).")]
	float m_fMinute;

	[Attribute("", UIWidgets.EditBox, "Faction that hears it. Empty = everyone.")]
	string m_sFactionKey;

	[Attribute("", UIWidgets.EditBox, "Popup title.")]
	string m_sTitle;

	[Attribute("", UIWidgets.EditBoxMultiline, "Popup text.")]
	string m_sText;
}

[ComponentEditorProps(category: "Game Mode Component", description: "The Turn: a TvT that becomes a zombie outbreak, ending in one helicopter extraction with too few seats.")]
class CRF_TheTurnGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_TheTurnGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("BLUFOR", UIWidgets.EditBox, "First side.", category: "The Turn - Factions")]
	protected string m_sFactionOneKey;

	[Attribute("OPFOR", UIWidgets.EditBox, "Second side.", category: "The Turn - Factions")]
	protected string m_sFactionTwoKey;

	[Attribute("25", UIWidgets.Auto, "Minutes of normal TvT before the outbreak.", category: "The Turn - Story")]
	protected float m_fOutbreakMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Send the built-in radio chatter before and at the outbreak (riots up north, a patrol lost, then the outbreak itself).", category: "The Turn - Story")]
	protected bool m_bDefaultChatter;

	[Attribute(desc: "Extra radio messages at set times.", category: "The Turn - Story")]
	protected ref array<ref CRF_TheTurnMessage> m_aMessages;

	[Attribute("", UIWidgets.Auto, "Names of the entities zombies spawn at. Place them around the edge of the area.", category: "The Turn - Zombies")]
	protected ref array<string> m_aSpawnPointNames;

	[Attribute("", UIWidgets.ResourceNamePicker, "Group prefabs for the early waves. Empty = Bacon Zombies civilians.", params: "et", category: "The Turn - Zombies")]
	protected ref array<ResourceName> m_aEarlyGroups;

	[Attribute("", UIWidgets.ResourceNamePicker, "Group prefabs mixed in from Late Wave on. Empty = Bacon Zombies soldiers and hordes.", params: "et", category: "The Turn - Zombies")]
	protected ref array<ResourceName> m_aLateGroups;

	[Attribute("4", UIWidgets.Auto, "Wave from which the late groups are mixed in.", category: "The Turn - Zombies")]
	protected int m_iLateWave;

	[Attribute("90", UIWidgets.Auto, "Seconds between waves.", category: "The Turn - Zombies")]
	protected float m_fWaveSeconds;

	[Attribute("2", UIWidgets.Auto, "Groups in the first wave.", category: "The Turn - Zombies")]
	protected int m_iFirstWaveGroups;

	[Attribute("1", UIWidgets.Auto, "Groups added with each wave.", category: "The Turn - Zombies")]
	protected float m_fGroupsAddedPerWave;

	[Attribute("8", UIWidgets.Auto, "Most groups in one wave.", category: "The Turn - Zombies")]
	protected int m_iMaxGroupsPerWave;

	[Attribute("90", UIWidgets.Auto, "Most zombies alive at once (server load). No new groups spawn above this.", category: "The Turn - Zombies")]
	protected int m_iMaxZombies;

	[Attribute("200", UIWidgets.Auto, "A spawn point is only used if the nearest player is at least this far (metres).", category: "The Turn - Zombies")]
	protected float m_fMinSpawnDistance;

	[Attribute("900", UIWidgets.Auto, "Spawn points closer than this to a player are preferred (metres).", category: "The Turn - Zombies")]
	protected float m_fMaxSpawnDistance;

	[Attribute("30", UIWidgets.Auto, "Seconds between pointing every group at the nearest player again.", category: "The Turn - Zombies")]
	protected float m_fRetargetSeconds;

	[Attribute("1500", UIWidgets.Auto, "Groups further than this from every player are removed (metres).", category: "The Turn - Zombies")]
	protected float m_fDespawnDistance;

	[Attribute("{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et", UIWidgets.ResourceNamePicker, "Waypoint given to zombie groups at the nearest player.", params: "et", category: "The Turn - Zombies")]
	protected ResourceName m_sWaypointPrefab;

	[Attribute("15", UIWidgets.Auto, "Minutes after the outbreak that extraction is called.", category: "The Turn - Extraction")]
	protected float m_fExtractionCallMinutes;

	[Attribute("", UIWidgets.EditBox, "Name of the landing zone entity.", category: "The Turn - Extraction")]
	protected string m_sLandingZoneName;

	[Attribute("240", UIWidgets.Auto, "Seconds from the call until the helicopter lands.", category: "The Turn - Extraction")]
	protected int m_iInboundSeconds;

	[Attribute("120", UIWidgets.Auto, "Seconds the helicopter waits at the LZ.", category: "The Turn - Extraction")]
	protected int m_iBoardingSeconds;

	[Attribute("12", UIWidgets.Auto, "Seats on the helicopter. It leaves as soon as they're taken.", category: "The Turn - Extraction")]
	protected int m_iSeats;

	[Attribute("15", UIWidgets.Auto, "Metres from the LZ a player has to reach to take a seat.", category: "The Turn - Extraction")]
	protected float m_fBoardingRadius;

	[Attribute("{70BAEEFC2D3FEE64}Prefabs/Vehicles/Helicopters/UH1H/UH1H.et", UIWidgets.ResourceNamePicker, "Helicopter shown at the LZ while it waits (nobody can fly it - anyone reaching it takes a seat). Empty = none.", params: "et", category: "The Turn - Extraction")]
	protected ResourceName m_sHelicopterPrefab;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the helicopter leaves.", category: "The Turn - Victory")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker icon for the LZ.", params: "edds", category: "The Turn - Map")]
	protected ResourceName m_sMarkerIcon;

	//===================================================================================
	// STATE
	//===================================================================================

	[RplProp()]
	protected int m_iPhase;

	//! Seconds left while the helicopter is inbound or waiting
	[RplProp()]
	protected int m_iPhaseSecondsLeft;

	[RplProp()]
	protected int m_iSeatsLeft;

	// Server
	protected bool m_bRoundLive;
	protected bool m_bEnded;
	protected float m_fElapsedSeconds;
	protected ref array<ref CRF_TheTurnMessage> m_aSchedule = {};
	protected ref array<bool> m_aSent = {};
	protected ref array<IEntity> m_aSpawnPoints = {};
	protected IEntity m_LandingZone;
	protected IEntity m_Helicopter;
	protected int m_iWave;
	protected float m_fNextWaveAt;
	protected float m_fNextRetargetAt;
	protected int m_iNobodyAliveSeconds;
	protected ref map<string, int> m_mAboard = new map<string, int>();
	protected ref array<string> m_aAboardNames = {};
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Zombie groups, their current waypoint and when they spawned (same index)
	protected ref array<SCR_AIGroup> m_aGroups = {};
	protected ref array<IEntity> m_aGroupWaypoints = {};
	protected ref array<float> m_aGroupSpawnedAt = {};

	// Seconds a group gets to spawn its members before an empty one is dropped
	protected static const float GROUP_SPAWN_GRACE = 60;

	// Client
	protected ref CRF_SpecialsMarkerSet m_Markers = new CRF_SpecialsMarkerSet();

	protected static CRF_TheTurnGamemodeComponent s_Instance;

	protected static const string LOG = "[CRF_TheTurn]";
	protected static const int NOBODY_ALIVE_GRACE = 15;

	// Bacon Zombies groups, used when the mission doesn't set its own
	protected static const ResourceName BACON_CIVILIANS = "{67E76BA71804B965}622120A5448725E3/Prefabs/Groups/Group_Zombies_CIV.et";
	protected static const ResourceName BACON_CIVILIANS_LARGE = "{813B0F968868FA48}622120A5448725E3/Prefabs/Groups/Group_Zombies_CIV_Large.et";
	protected static const ResourceName BACON_SOLDIERS = "{58251EDC277CE499}622120A5448725E3/Prefabs/Groups/Group_Zombies_USSR.et";
	protected static const ResourceName BACON_HORDE = "{6AEA65C9E88BB0B2}622120A5448725E3/Prefabs/Groups/Group_Zombies_Horde.et";

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_TheTurnGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_TheTurnGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (GetGame().InPlayMode())
			SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// The LZ marker on every machine with a player
		if (!System.IsConsoleApp())
			GetGame().GetCallqueue().CallLater(UpdateMarkers, 1000, true);

		if (!Replication.IsServer())
			return;

		if (m_aSpawnPointNames)
		{
			foreach (string spawnPointName : m_aSpawnPointNames)
			{
				IEntity spawnPoint = CRF_SpecialsHelper.FindNamedEntity(spawnPointName, LOG);
				if (spawnPoint)
					m_aSpawnPoints.Insert(spawnPoint);
			}
		}

		if (m_aSpawnPoints.IsEmpty())
			Print(LOG + " No zombie spawn points - list the names of entities around the area in Spawn Point Names.", LogLevel.ERROR);

		m_LandingZone = CRF_SpecialsHelper.FindNamedEntity(m_sLandingZoneName, LOG);
		if (!m_LandingZone)
			Print(LOG + " No landing zone - set Landing Zone Name, or nobody can be extracted.", LogLevel.ERROR);

		BuildSchedule();

		m_iPhase = CRF_ETheTurnPhase.NORMAL;
		m_iSeatsLeft = m_iSeats;
		Replication.BumpMe();

		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChanged);

		// Fallback for missions without safestart
		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(UpdateMarkers);
		}

		m_Markers.Clear();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! The mission's messages plus, if enabled, the built-in chatter leading up to the outbreak
	protected void BuildSchedule()
	{
		if (m_bDefaultChatter)
		{
			AddMessage(m_fOutbreakMinutes * 0.4, "Radio chatter",
				"Command: unconfirmed reports of riots in the villages to the north. Civilians attacking each other. Disregard and continue your mission.");
			AddMessage(m_fOutbreakMinutes * 0.7, "Lost contact",
				"Command has lost contact with a patrol in your area. Their last transmission was screaming.");
			AddMessage(m_fOutbreakMinutes * 0.9, "Something is wrong",
				"The dogs have stopped barking. Keep your eyes open.");
		}

		if (m_aMessages)
		{
			foreach (CRF_TheTurnMessage message : m_aMessages)
			{
				if (message)
					m_aSchedule.Insert(message);
			}
		}

		foreach (CRF_TheTurnMessage message : m_aSchedule)
			m_aSent.Insert(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void AddMessage(float minute, string title, string text)
	{
		CRF_TheTurnMessage message = new CRF_TheTurnMessage();
		message.m_fMinute = minute;
		message.m_sTitle = title;
		message.m_sText = text;
		m_aSchedule.Insert(message);
	}

	//===================================================================================
	// ROUND START
	//===================================================================================

	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive)
			StartRound();
	}

	protected void OnGamemodeStateChanged()
	{
		if (m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME && (!m_SafestartManager || !m_SafestartManager.GetSafestartStatus()))
			StartRound();
	}

	protected void StartRound()
	{
		if (m_bRoundLive || m_bEnded)
			return;

		m_bRoundLive = true;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);
	}

	//===================================================================================
	// TICK (1 s, server)
	//===================================================================================

	protected void Tick()
	{
		if (m_bEnded)
			return;

		m_fElapsedSeconds += 1;
		SendScheduledMessages();

		if (m_iPhase == CRF_ETheTurnPhase.NORMAL)
		{
			if (m_fElapsedSeconds >= m_fOutbreakMinutes * 60)
				StartOutbreak();
			return;
		}

		// From the outbreak on: waves, and groups steered towards the living
		if (m_fElapsedSeconds >= m_fNextWaveAt)
		{
			m_fNextWaveAt = m_fElapsedSeconds + m_fWaveSeconds;
			SpawnWave();
		}

		if (m_fElapsedSeconds >= m_fNextRetargetAt)
		{
			m_fNextRetargetAt = m_fElapsedSeconds + m_fRetargetSeconds;
			UpdateGroups();
		}

		switch (m_iPhase)
		{
			case CRF_ETheTurnPhase.OUTBREAK:
			{
				if (m_fElapsedSeconds >= (m_fOutbreakMinutes + m_fExtractionCallMinutes) * 60)
					CallExtraction();
				break;
			}

			case CRF_ETheTurnPhase.INBOUND:
			{
				m_iPhaseSecondsLeft--;
				if (m_iPhaseSecondsLeft == 60)
					CRF_RadioTransmissionEvents.PopupToFaction("", "Helicopter one minute out", string.Format("%1 seats. Get to the landing zone.", m_iSeatsLeft), 6);

				if (m_iPhaseSecondsLeft <= 0)
					Land();

				Replication.BumpMe();
				break;
			}

			case CRF_ETheTurnPhase.BOARDING:
			{
				Board();
				m_iPhaseSecondsLeft--;
				Replication.BumpMe();

				if (m_iSeatsLeft <= 0)
				{
					Depart("The helicopter is full.");
					return;
				}

				if (m_iPhaseSecondsLeft <= 0)
				{
					Depart("The helicopter couldn't wait any longer.");
					return;
				}
				break;
			}
		}

		// Everyone dead
		array<int> alive = {};
		GetAlivePlayingPlayers(alive);
		if (alive.IsEmpty())
		{
			m_iNobodyAliveSeconds++;
			if (m_iNobodyAliveSeconds >= NOBODY_ALIVE_GRACE)
				Depart("Nobody is left alive.");
		}
		else
		{
			m_iNobodyAliveSeconds = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void SendScheduledMessages()
	{
		foreach (int i, CRF_TheTurnMessage message : m_aSchedule)
		{
			if (m_aSent[i] || m_fElapsedSeconds < message.m_fMinute * 60)
				continue;

			m_aSent[i] = true;
			CRF_RadioTransmissionEvents.PopupToFaction(message.m_sFactionKey, message.m_sTitle, message.m_sText, 10);
		}
	}

	//===================================================================================
	// OUTBREAK AND WAVES
	//===================================================================================

	protected void StartOutbreak()
	{
		m_iPhase = CRF_ETheTurnPhase.OUTBREAK;
		Replication.BumpMe();

		m_fNextWaveAt = m_fElapsedSeconds;
		m_fNextRetargetAt = m_fElapsedSeconds + m_fRetargetSeconds;

		CRF_RadioTransmissionEvents.PopupToFaction("", "OUTBREAK",
			string.Format("They're coming from every direction. The fight doesn't matter any more - stay alive. A helicopter will be called in %1 minutes, and it won't have room for everyone.", Math.Round(m_fExtractionCallMinutes)), 15);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnWave()
	{
		array<int> alive = {};
		GetAlivePlayingPlayers(alive);
		if (alive.IsEmpty() || m_aSpawnPoints.IsEmpty())
			return;

		m_iWave++;
		int groups = (int)Math.Min(m_iMaxGroupsPerWave, m_iFirstWaveGroups + Math.Floor((m_iWave - 1) * m_fGroupsAddedPerWave));
		int spawned = 0;
		for (int i = 0; i < groups; i++)
		{
			if (CountZombies() >= m_iMaxZombies)
				break;

			if (SpawnGroup(PickGroupPrefab(), alive))
				spawned++;
		}

		Print(string.Format("%1 Wave %2: %3 group(s), %4 zombies alive before it.", LOG, m_iWave, spawned, CountZombies()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName PickGroupPrefab()
	{
		array<ResourceName> pool = {};
		if (m_aEarlyGroups && !m_aEarlyGroups.IsEmpty())
			pool.Copy(m_aEarlyGroups);
		else
		{
			pool.Insert(BACON_CIVILIANS);
			pool.Insert(BACON_CIVILIANS_LARGE);
		}

		if (m_iWave >= m_iLateWave)
		{
			if (m_aLateGroups && !m_aLateGroups.IsEmpty())
				pool.InsertAll(m_aLateGroups);
			else
			{
				pool.Insert(BACON_SOLDIERS);
				pool.Insert(BACON_HORDE);
			}
		}

		return pool.GetRandomElement();
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns a group at a spawn point a sensible distance from the players and sends it at the nearest one
	protected bool SpawnGroup(ResourceName prefab, array<int> alive)
	{
		IEntity spawnPoint = PickSpawnPoint(alive);
		if (!spawnPoint)
			return false;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("%1 Can't load zombie group '%2'.", LOG, prefab), LogLevel.ERROR);
			return false;
		}

		vector position = spawnPoint.GetOrigin();
		position[0] = position[0] + Math.RandomFloat(-20, 20);
		position[2] = position[2] + Math.RandomFloat(-20, 20);
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = position;

		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams));
		if (!group)
			return false;

		m_aGroups.Insert(group);
		m_aGroupWaypoints.Insert(null);
		m_aGroupSpawnedAt.Insert(m_fElapsedSeconds);
		SendGroupAt(m_aGroups.Count() - 1, position, alive);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A random spawn point between the min and max distance from the nearest player; failing that the
	//! closest one that is still beyond the min distance
	protected IEntity PickSpawnPoint(array<int> alive)
	{
		array<IEntity> preferred = {};
		IEntity fallback;
		float fallbackDistance = 1e10;

		foreach (IEntity spawnPoint : m_aSpawnPoints)
		{
			float distance = CRF_SpecialsHelper.DistanceToNearestPlayer(spawnPoint.GetOrigin(), alive);
			if (distance < m_fMinSpawnDistance)
				continue;

			if (distance <= m_fMaxSpawnDistance)
				preferred.Insert(spawnPoint);
			else if (distance < fallbackDistance)
			{
				fallback = spawnPoint;
				fallbackDistance = distance;
			}
		}

		if (!preferred.IsEmpty())
			return preferred.GetRandomElement();

		return fallback;
	}

	//------------------------------------------------------------------------------------------------
	//! Points every group at the nearest living player again; removes dead groups and ones nobody is near
	protected void UpdateGroups()
	{
		array<int> alive = {};
		GetAlivePlayingPlayers(alive);

		for (int i = m_aGroups.Count() - 1; i >= 0; i--)
		{
			SCR_AIGroup group = m_aGroups[i];
			IEntity leader;
			if (group)
				leader = group.GetLeaderEntity();

			if (!group)
			{
				ForgetGroup(i, false);
				continue;
			}

			if (!leader)
			{
				// Still spawning its members, or wiped out
				if (group.GetAgentsCount() == 0 && m_fElapsedSeconds - m_aGroupSpawnedAt[i] >= GROUP_SPAWN_GRACE)
					ForgetGroup(i, true);
				continue;
			}

			if (alive.IsEmpty())
				continue;

			if (CRF_SpecialsHelper.DistanceToNearestPlayer(leader.GetOrigin(), alive) > m_fDespawnDistance)
			{
				ForgetGroup(i, true);
				continue;
			}

			SendGroupAt(i, leader.GetOrigin(), alive);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Replaces the group's waypoint with one at the living player nearest to from
	protected void SendGroupAt(int index, vector from, array<int> alive)
	{
		SCR_AIGroup group = m_aGroups[index];
		int nearestId;
		CRF_SpecialsHelper.DistanceToNearestPlayer(from, alive, nearestId);
		IEntity target = GetGame().GetPlayerManager().GetPlayerControlledEntity(nearestId);
		if (!group || !target)
			return;

		Resource resource = Resource.Load(m_sWaypointPrefab);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = target.GetOrigin();

		AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams));
		if (!waypoint)
			return;

		RemoveGroupWaypoint(index);
		group.AddWaypoint(waypoint);
		m_aGroupWaypoints[index] = waypoint;
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveGroupWaypoint(int index)
	{
		AIWaypoint old = AIWaypoint.Cast(m_aGroupWaypoints[index]);
		if (!old)
			return;

		SCR_AIGroup group = m_aGroups[index];
		if (group)
			group.RemoveWaypoint(old);

		SCR_EntityHelper.DeleteEntityAndChildren(old);
		m_aGroupWaypoints[index] = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Stops tracking a group; deleteMembers also removes the group and its zombies
	protected void ForgetGroup(int index, bool deleteMembers)
	{
		RemoveGroupWaypoint(index);

		SCR_AIGroup group = m_aGroups[index];
		if (group && deleteMembers)
		{
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (agent && agent.GetControlledEntity())
					SCR_EntityHelper.DeleteEntityAndChildren(agent.GetControlledEntity());
			}

			SCR_EntityHelper.DeleteEntityAndChildren(group);
		}

		m_aGroups.Remove(index);
		m_aGroupWaypoints.Remove(index);
		m_aGroupSpawnedAt.Remove(index);
	}

	//------------------------------------------------------------------------------------------------
	protected int CountZombies()
	{
		int count = 0;
		foreach (SCR_AIGroup group : m_aGroups)
		{
			if (group)
				count += group.GetAgentsCount();
		}
		return count;
	}

	//===================================================================================
	// EXTRACTION
	//===================================================================================

	protected void CallExtraction()
	{
		m_iPhase = CRF_ETheTurnPhase.INBOUND;
		m_iPhaseSecondsLeft = m_iInboundSeconds;
		Replication.BumpMe();

		string grid = "";
		if (m_LandingZone)
			grid = " Landing zone: grid " + CRF_RadioTransmissionEvents.GetGrid100(m_LandingZone.GetOrigin()) + ".";

		CRF_RadioTransmissionEvents.PopupToFaction("", "A helicopter is coming",
			string.Format("It lands in %1 with %2 seats - not enough for everyone.%3 Whoever gets aboard gets out.", CRF_SpecialsHelper.FormatSeconds(m_iInboundSeconds), m_iSeats, grid), 15);
	}

	//------------------------------------------------------------------------------------------------
	protected void Land()
	{
		m_iPhase = CRF_ETheTurnPhase.BOARDING;
		m_iPhaseSecondsLeft = m_iBoardingSeconds;
		Replication.BumpMe();

		// Whoever is already waiting on the pad boards first, so nobody is standing where the helicopter appears
		Board();

		if (m_LandingZone && !m_sHelicopterPrefab.IsEmpty() && m_iSeatsLeft > 0)
		{
			Resource resource = Resource.Load(m_sHelicopterPrefab);
			if (resource && resource.IsValid())
			{
				EntitySpawnParams spawnParams = new EntitySpawnParams();
				spawnParams.TransformMode = ETransformMode.WORLD;
				m_LandingZone.GetWorldTransform(spawnParams.Transform);
				vector position = spawnParams.Transform[3];
				position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
				spawnParams.Transform[3] = position;
				m_Helicopter = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
			}
		}

		CRF_RadioTransmissionEvents.PopupToFaction("", "The helicopter is down",
			string.Format("%1 seats. It leaves in %2 or when it's full.", m_iSeatsLeft, CRF_SpecialsHelper.FormatSeconds(m_iBoardingSeconds)), 8);
	}

	//------------------------------------------------------------------------------------------------
	//! Players at the LZ take the free seats, nearest first
	protected void Board()
	{
		if (!m_LandingZone || m_iSeatsLeft <= 0)
			return;

		vector lz = m_LandingZone.GetOrigin();
		array<int> alive = {};
		GetAlivePlayingPlayers(alive);

		array<int> boarding = {};
		array<float> distances = {};
		foreach (int playerId : alive)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity)
				continue;

			float distance = vector.DistanceXZ(entity.GetOrigin(), lz);
			if (distance > m_fBoardingRadius)
				continue;

			// Insertion by distance
			int at = 0;
			while (at < distances.Count() && distances[at] <= distance)
				at++;
			boarding.InsertAt(playerId, at);
			distances.InsertAt(distance, at);
		}

		foreach (int playerId : boarding)
		{
			if (m_iSeatsLeft <= 0)
				break;

			string factionKey = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!CRF_SpecialsHelper.ExtractPlayer(playerId))
				continue;

			m_iSeatsLeft--;
			int aboard;
			m_mAboard.Find(factionKey, aboard);
			m_mAboard.Set(factionKey, aboard + 1);
			m_aAboardNames.Insert(playerName);

			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "You're aboard", "You made it out. Watch the rest from here.", 8);
			CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("%1 is aboard (%2)", playerName, CRF_SpecialsHelper.GetFactionName(factionKey)),
				string.Format("%1 seat(s) left.", m_iSeatsLeft), 4);
		}

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! The helicopter leaves (or there's nobody left to take): the side with more aboard wins
	protected void Depart(string reason)
	{
		if (m_bEnded)
			return;

		m_bEnded = true;
		m_iPhase = CRF_ETheTurnPhase.ENDED;
		Replication.BumpMe();
		GetGame().GetCallqueue().Remove(Tick);

		if (m_Helicopter)
			SCR_EntityHelper.DeleteEntityAndChildren(m_Helicopter);

		int one;
		int two;
		m_mAboard.Find(m_sFactionOneKey, one);
		m_mAboard.Find(m_sFactionTwoKey, two);

		string winner = "";
		string title = "NOBODY WINS";
		if (one > two)
			winner = m_sFactionOneKey;
		else if (two > one)
			winner = m_sFactionTwoKey;

		if (!winner.IsEmpty())
			title = CRF_SpecialsHelper.GetFactionName(winner) + " WINS";

		string score = string.Format("%1 aboard: %2 %3, %4 %5.", reason, CRF_SpecialsHelper.GetFactionName(m_sFactionOneKey), one, CRF_SpecialsHelper.GetFactionName(m_sFactionTwoKey), two);
		CRF_RadioTransmissionEvents.PopupToFaction("", title, score, 15);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager && !winner.IsEmpty())
			loggingManager.SetWinningFaction(winner, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//===================================================================================
	// CLIENT: the LZ marker
	//===================================================================================

	protected void UpdateMarkers()
	{
		m_Markers.Begin();

		IEntity lz = GetGame().GetWorld().FindEntityByName(m_sLandingZoneName);
		if (lz && (m_iPhase == CRF_ETheTurnPhase.INBOUND || m_iPhase == CRF_ETheTurnPhase.BOARDING))
		{
			string label = string.Format("Extraction - lands in %1, %2 seats", CRF_SpecialsHelper.FormatSeconds(m_iPhaseSecondsLeft), m_iSeatsLeft);
			if (m_iPhase == CRF_ETheTurnPhase.BOARDING)
				label = string.Format("Extraction - %1 seats, leaves in %2", m_iSeatsLeft, CRF_SpecialsHelper.FormatSeconds(m_iPhaseSecondsLeft));

			m_Markers.Want(lz.GetOrigin(), label, m_sMarkerIcon, ARGB(255, 255, 165, 0));
		}

		m_Markers.Apply();
	}

	//===================================================================================
	// HELPERS
	//===================================================================================

	//! Living players on either side
	protected void GetAlivePlayingPlayers(notnull array<int> outPlayers)
	{
		array<int> two = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sFactionOneKey, outPlayers);
		CRF_SpecialsHelper.GetAlivePlayers(m_sFactionTwoKey, two);
		outPlayers.InsertAll(two);
	}
}
