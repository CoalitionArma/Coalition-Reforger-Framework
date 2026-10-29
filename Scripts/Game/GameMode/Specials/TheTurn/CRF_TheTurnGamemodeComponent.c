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
//   Dead become civilians (m_bDeadBecomeCivilians): a player killed on either side comes back once as a
//   civilian at the civilian spawn point. Civilians draw the zombies like anyone else but can't board,
//   and a civilian who dies spectates. Their way out is a refugee truck (m_sRefugeePointName) that waits
//   from the extraction call until the helicopter leaves; only civilians see it on the map.
//
//   Caches: weapon arsenals and supply crates (fuel, medical) at named spots, each a question mark on
//   everyone's map.
//
// Setup for mission makers
// 1. Build a normal TvT, but without objective components that end the mission on their own.
// 2. Add this component to the gamemode entity and set both faction keys.
// 3. Place a few empty entities around the edge of the area and list their names as zombie spawn points.
// 4. Place and name the landing zone (a helipad works) and set m_sLandingZoneName.
// 5. Factions: make the zombie faction (BACON_622120A5448725E3_FACTION) hostile to both sides.
// 6. Respawns: turn the Lobby's respawns off when the dead become civilians, and place a civilian spawn
//    point (CIVILIAN_Spawn.et) and set a civilian gearscript. Otherwise the Lobby's respawn settings apply.
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

	[Attribute("{7C1AE1F2A3B4C800}Sounds/Halloween/CRF_DistantSiren.wav", UIWidgets.ResourceNamePicker, "Sound file everyone hears at the outbreak and when the helicopter is called - a far-off civil defence siren by default. Empty = none.", params: "wav", category: "The Turn - Story")]
	protected ResourceName m_sSirenSound;

	[Attribute("", UIWidgets.Auto, "Names of entities where an abandoned civilian vehicle is left, nearly out of fuel. Only their position on the map matters: the vehicle is put on the ground at a clear spot within 15 m.", category: "The Turn - Civilian Vehicles")]
	protected ref array<string> m_aCivilianVehicleSpotNames;

	[Attribute("", UIWidgets.ResourceNamePicker, "Vehicles to pick from at random (list one several times to make it more common). Empty = vanilla civilian cars, vans, UAZs and the odd Ural.", params: "et", category: "The Turn - Civilian Vehicles")]
	protected ref array<ResourceName> m_aCivilianVehiclePrefabs;

	[Attribute("0.03", UIWidgets.Slider, "Least fuel a civilian vehicle starts with (share of a full tank).", params: "0 1 0.01", category: "The Turn - Civilian Vehicles")]
	protected float m_fCivilianFuelMin;

	[Attribute("0.12", UIWidgets.Slider, "Most fuel a civilian vehicle starts with (share of a full tank).", params: "0 1 0.01", category: "The Turn - Civilian Vehicles")]
	protected float m_fCivilianFuelMax;

	[Attribute("1", UIWidgets.CheckBox, "Players killed on either side come back once as civilians at the civilian spawn point. Civilians can't board the helicopter, and a civilian who dies spectates. Turn the Lobby's respawns off.", category: "The Turn - Dead Become Civilians")]
	protected bool m_bDeadBecomeCivilians;

	[Attribute("CIV", UIWidgets.EditBox, "Faction the dead come back as. Needs a spawn point and a gearscript.", category: "The Turn - Dead Become Civilians")]
	protected string m_sCivilianFactionKey;

	[Attribute("10", UIWidgets.Auto, "Seconds after death before the player comes back as a civilian.", category: "The Turn - Dead Become Civilians")]
	protected float m_fCivilianDelaySeconds;

	[Attribute("You died, but you're not out. You're a civilian now, with no seat on the helicopter. Stay alive: when the helicopter is called, a refugee truck comes for civilians and it'll be on your map. Die again and you spectate.", UIWidgets.EditBoxMultiline, "Popup a player gets on coming back as a civilian.", category: "The Turn - Dead Become Civilians")]
	protected string m_sCivilianText;

	[Attribute("", UIWidgets.EditBox, "Name of the entity the refugee truck waits at (the civilians' way out). It's there from the extraction call until the helicopter leaves; civilians who reach it are out of the round. Empty = no truck.", category: "The Turn - Dead Become Civilians")]
	protected string m_sRefugeePointName;

	[Attribute("{EEAC3A94929A2B55}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport_CIV_randomized.et", UIWidgets.ResourceNamePicker, "Truck shown at the refugee point (its tank is emptied, so nobody drives off in it). Empty = none.", params: "et", category: "The Turn - Dead Become Civilians")]
	protected ResourceName m_sRefugeeTruckPrefab;

	[Attribute("15", UIWidgets.Auto, "Metres from the refugee point a civilian has to reach to get on the truck.", category: "The Turn - Dead Become Civilians")]
	protected float m_fRefugeeRadius;

	[Attribute("", UIWidgets.Auto, "Names of entities where a weapon arsenal is put down. Only their position on the map matters.", category: "The Turn - Caches")]
	protected ref array<string> m_aArsenalSpotNames;

	[Attribute("{3324FCADDE3D4D31}Prefabs/Compositions/SupplyArsenal_NoSupply/SupplyArsenalNoSupply_CIV.et", UIWidgets.ResourceNamePicker, "Arsenal put down at each arsenal spot. The CRF supply arsenal gives every player their own side's gearscript (civilians: the civilian gearscript and its extra supply arsenal items).", params: "et", category: "The Turn - Caches")]
	protected ResourceName m_sArsenalPrefab;

	[Attribute("", UIWidgets.Auto, "Names of entities where a supply cache (a crate with fuel and medical supplies round it) is put down.", category: "The Turn - Caches")]
	protected ref array<string> m_aSupplySpotNames;

	[Attribute("{B3283803059E2750}Prefabs/Props/Crates/CrateWooden_01_plain.et", UIWidgets.ResourceNamePicker, "Crate at the centre of each supply cache. Empty = just the items.", params: "et", category: "The Turn - Caches")]
	protected ResourceName m_sSupplyCratePrefab;

	[Attribute("", UIWidgets.ResourceNamePicker, "Items laid round each supply crate (list one several times for more). Fuel cans start full. Empty = two jerrycans, bandages, tourniquets, morphine and saline.", params: "et", category: "The Turn - Caches")]
	protected ref array<ResourceName> m_aSupplyItems;

	[Attribute("1", UIWidgets.CheckBox, "Mark every arsenal and supply cache on everyone's map with a question mark.", category: "The Turn - Caches")]
	protected bool m_bMarkCaches;

	[Attribute("{7606192F8A35EADE}UI/Textures/Tasks/TaskIcons/64/Icon_Task_Unknown.edds", UIWidgets.ResourcePickerThumbnail, "Map marker icon for the caches.", params: "edds", category: "The Turn - Caches")]
	protected ResourceName m_sCacheMarkerIcon;

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

	// Playable groups the dead are put in as civilians
	protected ref array<SCR_AIGroup> m_aCivilianGroups = {};
	protected IEntity m_RefugeePoint;
	protected IEntity m_RefugeeTruck;
	protected int m_iCiviliansOut;

	// Seconds a group gets to spawn its members before an empty one is dropped
	protected static const float GROUP_SPAWN_GRACE = 60;

	protected static const COA_EGearRole CIVILIAN_ROLE = COA_EGearRole.RIFLEMAN;
	protected static const int CIVILIANS_PER_GROUP = 16;

	// Supply cache contents, used when the mission doesn't list its own
	protected static const ResourceName SUPPLY_JERRYCAN = "{12D5AD21E383B768}Prefabs/Items/Fuel/Jerrycan_01/Jerrycan_01_item.et";
	protected static const ResourceName SUPPLY_BANDAGE = "{C3F1FA1E2EC2B345}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_USSR_01.et";
	protected static const ResourceName SUPPLY_TOURNIQUET = "{80E75A71C29190DB}Prefabs/Items/Medicine/Tourniquet_01/Tourniquet_USSR_01.et";
	protected static const ResourceName SUPPLY_MORPHINE = "{0D9A5DCF89AE7AA9}Prefabs/Items/Medicine/MorphineInjection_01/MorphineInjection_01.et";
	protected static const ResourceName SUPPLY_SALINE = "{527D7C5D2E476BDC}Prefabs/Items/Medicine/SalineBag_01/SalineBag_USSR_01.et";

	// Supply items lie this far from the crate's centre
	protected static const float SUPPLY_ITEM_RING = 1.4;

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

	// Vanilla civilian vehicles (randomized colours), used when the mission doesn't list its own.
	// Repeats make the small cars more common than the vans, and the Ural rare.
	protected static const ResourceName CIV_S105 = "{128253A267BE9424}Prefabs/Vehicles/Wheeled/S105/S105_randomized.et";
	protected static const ResourceName CIV_S1203_TRANSPORT = "{49C909AFD66E90A1}Prefabs/Vehicles/Wheeled/S1203/S1203_transport_randomized.et";
	protected static const ResourceName CIV_S1203_CARGO = "{5729759DD092B0F7}Prefabs/Vehicles/Wheeled/S1203/S1203_cargo_randomized.et";
	protected static const ResourceName CIV_UAZ452 = "{C7CE8044B5E301F5}Prefabs/Vehicles/Wheeled/UAZ452/UAZ452_transport_CIV_randomized.et";
	protected static const ResourceName CIV_UAZ469_OPEN = "{1B0D46B41C50873D}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_uncovered_CIV_Randomized.et";
	protected static const ResourceName CIV_UAZ469_COVERED = "{57A441224AC02CF3}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_covered_CIV_Randomized.et";
	protected static const ResourceName CIV_URAL = "{EEAC3A94929A2B55}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport_CIV_randomized.et";

	// Vehicles are dropped this far above the ground so they settle onto their wheels
	protected static const float VEHICLE_SPAWN_LIFT = 0.5;

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

		if (m_bDeadBecomeCivilians && !m_sRefugeePointName.IsEmpty())
			m_RefugeePoint = CRF_SpecialsHelper.FindNamedEntity(m_sRefugeePointName, LOG);

		// Abandoned civilian vehicles and the caches, once the world has settled
		GetGame().GetCallqueue().CallLater(SpawnCivilianVehicles, 2000, false);
		GetGame().GetCallqueue().CallLater(SpawnCaches, 2000, false);

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
			GetGame().GetCallqueue().Remove(SpawnCivilianVehicles);
			GetGame().GetCallqueue().Remove(SetLowFuel);
			GetGame().GetCallqueue().Remove(SpawnCaches);
			GetGame().GetCallqueue().Remove(SetFuelPercentage);
			GetGame().GetCallqueue().Remove(BecomeCivilian);
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
	//! Server: an abandoned civilian vehicle at every listed spot, on the ground and nearly out of fuel.
	//! Vehicles are replicated, so every client gets them (late joiners included).
	protected void SpawnCivilianVehicles()
	{
		if (!m_aCivilianVehicleSpotNames || m_aCivilianVehicleSpotNames.IsEmpty())
			return;

		array<ResourceName> pool = {};
		if (m_aCivilianVehiclePrefabs && !m_aCivilianVehiclePrefabs.IsEmpty())
			pool.Copy(m_aCivilianVehiclePrefabs);
		else
		{
			for (int car = 0; car < 3; car++)
				pool.Insert(CIV_S105);
			for (int pair = 0; pair < 2; pair++)
			{
				pool.Insert(CIV_S1203_TRANSPORT);
				pool.Insert(CIV_UAZ452);
				pool.Insert(CIV_UAZ469_OPEN);
			}
			pool.Insert(CIV_S1203_CARGO);
			pool.Insert(CIV_UAZ469_COVERED);
			pool.Insert(CIV_URAL);
		}

		int spawned = 0;
		foreach (string spotName : m_aCivilianVehicleSpotNames)
		{
			IEntity spot = CRF_SpecialsHelper.FindNamedEntity(spotName, LOG);
			if (!spot)
				continue;

			IEntity vehicle = CRF_SpecialsHelper.SpawnOnGround(pool.GetRandomElement(), spot.GetOrigin(), Math.RandomFloat(0, 360), 15, 3.5, VEHICLE_SPAWN_LIFT);
			if (!vehicle)
				continue;

			// After a moment, once the vehicle's fuel system has set itself up
			GetGame().GetCallqueue().CallLater(SetLowFuel, 1000, false, vehicle);
			spawned++;
		}

		Print(string.Format("%1 %2 abandoned civilian vehicle(s) placed.", LOG, spawned), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetLowFuel(IEntity vehicle)
	{
		if (!vehicle)
			return;

		SCR_FuelManagerComponent fuelManager = SCR_FuelManagerComponent.Cast(vehicle.FindComponent(SCR_FuelManagerComponent));
		if (fuelManager)
			fuelManager.SetTotalFuelPercentage(Math.RandomFloat(m_fCivilianFuelMin, Math.Max(m_fCivilianFuelMin, m_fCivilianFuelMax)));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetFuelPercentage(IEntity entity, float fuel)
	{
		if (!entity)
			return;

		SCR_FuelManagerComponent fuelManager = SCR_FuelManagerComponent.Cast(entity.FindComponent(SCR_FuelManagerComponent));
		if (fuelManager)
			fuelManager.SetTotalFuelPercentage(fuel);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a weapon arsenal at every arsenal spot and a supply cache at every supply spot, on the
	//! ground. All of it is replicated; the map markers come from the spot entities on each client.
	protected void SpawnCaches()
	{
		int arsenals = 0;
		if (m_aArsenalSpotNames && !m_sArsenalPrefab.IsEmpty())
		{
			foreach (string arsenalSpotName : m_aArsenalSpotNames)
			{
				IEntity arsenalSpot = CRF_SpecialsHelper.FindNamedEntity(arsenalSpotName, LOG);
				if (arsenalSpot && CRF_SpecialsHelper.SpawnOnGround(m_sArsenalPrefab, arsenalSpot.GetOrigin(), Math.RandomFloat(0, 360), 12, 3))
					arsenals++;
			}
		}

		array<ResourceName> items = {};
		if (m_aSupplyItems && !m_aSupplyItems.IsEmpty())
			items.Copy(m_aSupplyItems);
		else
		{
			items.Insert(SUPPLY_JERRYCAN);
			items.Insert(SUPPLY_JERRYCAN);
			for (int bandage = 0; bandage < 4; bandage++)
				items.Insert(SUPPLY_BANDAGE);
			items.Insert(SUPPLY_TOURNIQUET);
			items.Insert(SUPPLY_TOURNIQUET);
			items.Insert(SUPPLY_MORPHINE);
			items.Insert(SUPPLY_SALINE);
		}

		int caches = 0;
		if (m_aSupplySpotNames)
		{
			foreach (string supplySpotName : m_aSupplySpotNames)
			{
				IEntity supplySpot = CRF_SpecialsHelper.FindNamedEntity(supplySpotName, LOG);
				if (supplySpot && SpawnSupplyCache(supplySpot.GetOrigin(), items))
					caches++;
			}
		}

		Print(string.Format("%1 %2 arsenal(s) and %3 supply cache(s) placed.", LOG, arsenals, caches), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! A crate on the ground with the items lying in a ring round it
	protected bool SpawnSupplyCache(vector position, array<ResourceName> items)
	{
		vector centre = position;
		if (!m_sSupplyCratePrefab.IsEmpty())
		{
			IEntity crate = CRF_SpecialsHelper.SpawnOnGround(m_sSupplyCratePrefab, position, Math.RandomFloat(0, 360), 10, 2);
			if (!crate)
				return false;

			centre = crate.GetOrigin();
		}

		BaseWorld world = GetGame().GetWorld();
		float startAngle = Math.RandomFloat(0, Math.PI * 2);
		foreach (int i, ResourceName item : items)
		{
			Resource resource = Resource.Load(item);
			if (!resource || !resource.IsValid())
			{
				Print(string.Format("%1 Can't load supply item '%2'.", LOG, item), LogLevel.ERROR);
				continue;
			}

			float angle = startAngle + Math.PI * 2 * i / items.Count();
			vector at = centre + Vector(Math.Cos(angle) * SUPPLY_ITEM_RING, 0, Math.Sin(angle) * SUPPLY_ITEM_RING);
			at[1] = world.GetSurfaceY(at[0], at[2]) + 0.15;

			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			Math3D.AnglesToMatrix(Vector(Math.RandomFloat(0, 360), 0, 0), spawnParams.Transform);
			spawnParams.Transform[3] = at;

			IEntity spawned = GetGame().SpawnEntityPrefab(resource, world, spawnParams);

			// Fuel cans come part-full; these are full, once their fuel system has set itself up
			if (spawned && spawned.FindComponent(SCR_FuelManagerComponent))
				GetGame().GetCallqueue().CallLater(SetFuelPercentage, 1000, false, spawned, 1.0);
		}

		return true;
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

		// The refugee truck waits from the extraction call until the helicopter leaves
		if (m_iPhase == CRF_ETheTurnPhase.INBOUND || m_iPhase == CRF_ETheTurnPhase.BOARDING)
			BoardRefugees();

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

		// Everyone dead (civilians keep the round going for their truck)
		array<int> alive = {};
		GetZombieTargets(alive);
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
		SoundSiren();
	}

	//------------------------------------------------------------------------------------------------
	//! A far-off siren everyone hears
	protected void SoundSiren()
	{
		if (m_sSirenSound.IsEmpty())
			return;

		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.PlaySoundForEveryone(m_sSirenSound);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnWave()
	{
		array<int> alive = {};
		GetZombieTargets(alive);
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
		GetZombieTargets(alive);

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
		SoundSiren();
		SendRefugeeTruck();
	}

	//------------------------------------------------------------------------------------------------
	//! The civilians' way out: a truck at the refugee point, told only to them
	protected void SendRefugeeTruck()
	{
		if (!m_bDeadBecomeCivilians || !m_RefugeePoint)
			return;

		if (!m_sRefugeeTruckPrefab.IsEmpty())
		{
			m_RefugeeTruck = CRF_SpecialsHelper.SpawnOnGround(m_sRefugeeTruckPrefab, m_RefugeePoint.GetOrigin(), m_RefugeePoint.GetYawPitchRoll()[0], 15, 4, VEHICLE_SPAWN_LIFT);
			if (m_RefugeeTruck)
				GetGame().GetCallqueue().CallLater(SetFuelPercentage, 1000, false, m_RefugeeTruck, 0.0);
		}

		string grid = CRF_RadioTransmissionEvents.GetGrid100(m_RefugeePoint.GetOrigin());
		CRF_RadioTransmissionEvents.PopupToFaction(m_sCivilianFactionKey, "A refugee truck is waiting",
			string.Format("Civilians: a truck is waiting at grid %1, and it's marked on your map. It leaves when the helicopter does. Get on it and you're out.", grid), 15);
	}

	//------------------------------------------------------------------------------------------------
	//! Civilians at the refugee point get on the truck (out of the round, like the helicopter)
	protected void BoardRefugees()
	{
		if (!m_bDeadBecomeCivilians || !m_RefugeePoint)
			return;

		vector point = m_RefugeePoint.GetOrigin();
		if (m_RefugeeTruck)
			point = m_RefugeeTruck.GetOrigin();

		array<int> civilians = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sCivilianFactionKey, civilians);
		foreach (int playerId : civilians)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity || vector.DistanceXZ(entity.GetOrigin(), point) > m_fRefugeeRadius)
				continue;

			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!CRF_SpecialsHelper.ExtractPlayer(playerId))
				continue;

			m_iCiviliansOut++;
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "You're on the truck", "You made it out. Watch the rest from here.", 8);
			CRF_RadioTransmissionEvents.PopupToFaction(m_sCivilianFactionKey, string.Format("%1 is on the refugee truck", playerName), "", 4);
		}
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

		if (m_RefugeeTruck)
			SCR_EntityHelper.DeleteEntityAndChildren(m_RefugeeTruck);

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
		if (m_RefugeePoint)
			score += string.Format(" Civilians on the refugee truck: %1.", m_iCiviliansOut);
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
	// DEAD BECOME CIVILIANS (server)
	//===================================================================================

	//! Runs before the Lobby's own death handling (which marks the slot dead and puts the player in
	//! spectator), so the switch to civilian waits until that is done
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer() || !m_bDeadBecomeCivilians || m_bEnded)
			return;

		int playerId = instigatorContextData.GetVictimPlayerID();
		if (playerId <= 0)
			return;

		// Only soldiers come back; a civilian who dies stays in spectator
		string factionKey = CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId);
		if (factionKey != m_sFactionOneKey && factionKey != m_sFactionTwoKey)
			return;

		GetGame().GetCallqueue().CallLater(BecomeCivilian, (int)(Math.Max(m_fCivilianDelaySeconds, 1) * 1000), false, playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Moves a dead soldier's slot to the civilian faction and spawns them at its spawn point. The slot
	//! itself changes side, so the player keeps one slot and reconnects into it as a civilian.
	protected void BecomeCivilian(int playerId)
	{
		if (m_bEnded || !GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (!slottingManager || !respawnManager || !broadcastManager)
			return;

		int slotId = slottingManager.GetPlayerSlotID(playerId);
		COA_SlotData slotData = slottingManager.GetSlotData(slotId);

		// Still a dead soldier (not moved or revived by an admin meanwhile)
		if (!slotData || !slotData.GetIsDeadSlot())
			return;

		FactionKey factionKey = slotData.GetSlotFactionKey();
		if (factionKey != m_sFactionOneKey && factionKey != m_sFactionTwoKey)
			return;

		if (respawnManager.GetFactionSpawnpoints(m_sCivilianFactionKey).IsEmpty())
		{
			Print(string.Format("%1 No spawn point for '%2' - the dead stay in spectator. Place a CIVILIAN_Spawn.", LOG, m_sCivilianFactionKey), LogLevel.ERROR);
			return;
		}

		SCR_AIGroup group = GetCivilianGroup();
		if (!group)
			return;

		RplComponent groupRpl = RplComponent.Cast(group.FindComponent(RplComponent));
		if (!groupRpl)
			return;

		slotData.SetSlotFactionKey(m_sCivilianFactionKey);
		slotData.SetSlotCurrentGroup(groupRpl.Id());
		broadcastManager.UpdateSlotData(slotData);

		// Role isn't part of the full slot update; everyone gets the civilian gearscript's one role
		slottingManager.UpdateSlotRole(slotId, CIVILIAN_ROLE);

		// Marks the slot alive and spawns them at the civilian spawn point
		respawnManager.RespawnPlayer(playerId);

		CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "You're a civilian", m_sCivilianText, 12);
		Print(string.Format("%1 %2 (%3) came back as a civilian.", LOG, GetGame().GetPlayerManager().GetPlayerName(playerId), factionKey), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! A civilian playable group with room, created when they're all full
	protected SCR_AIGroup GetCivilianGroup()
	{
		foreach (SCR_AIGroup group : m_aCivilianGroups)
		{
			if (group && group.GetPlayerCount() < CIVILIANS_PER_GROUP)
				return group;
		}

		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(m_sCivilianFactionKey));
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!faction || !groupsManager)
		{
			Print(string.Format("%1 No faction '%2' - the dead stay in spectator.", LOG, m_sCivilianFactionKey), LogLevel.ERROR);
			return null;
		}

		SCR_AIGroup group = groupsManager.CreateNewPlayableGroup(faction);
		if (!group)
			return null;

		group.SetCanDeleteIfNoPlayer(false);
		group.SetDeleteWhenEmpty(false);
		group.SetMaxMembers(CIVILIANS_PER_GROUP);
		group.SetCustomName("Civilians", 0);
		m_aCivilianGroups.Insert(group);
		return group;
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

		// Caches: a question mark for everyone, all round
		if (m_bMarkCaches)
		{
			WantCacheMarkers(m_aArsenalSpotNames);
			WantCacheMarkers(m_aSupplySpotNames);
		}

		// The refugee truck: civilians only, while it waits
		if (m_bDeadBecomeCivilians && !m_sRefugeePointName.IsEmpty() && (m_iPhase == CRF_ETheTurnPhase.INBOUND || m_iPhase == CRF_ETheTurnPhase.BOARDING)
			&& CRF_SpecialsHelper.GetLocalFactionKey() == m_sCivilianFactionKey)
		{
			IEntity refugeePoint = GetGame().GetWorld().FindEntityByName(m_sRefugeePointName);
			if (refugeePoint)
				m_Markers.Want(refugeePoint.GetOrigin(), "Refugee truck - leaves with the helicopter", m_sMarkerIcon, ARGB(255, 80, 200, 120));
		}

		m_Markers.Apply();
	}

	//------------------------------------------------------------------------------------------------
	protected void WantCacheMarkers(array<string> spotNames)
	{
		if (!spotNames)
			return;

		foreach (string spotName : spotNames)
		{
			IEntity spot = GetGame().GetWorld().FindEntityByName(spotName);
			if (spot)
				m_Markers.Want(spot.GetOrigin(), "", m_sCacheMarkerIcon, ARGB(255, 150, 60, 210));
		}
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

	//! Living players the zombies spawn away from and hunt: both sides and the civilians
	protected void GetZombieTargets(notnull array<int> outPlayers)
	{
		GetAlivePlayingPlayers(outPlayers);
		if (!m_bDeadBecomeCivilians)
			return;

		array<int> civilians = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sCivilianFactionKey, civilians);
		outPlayers.InsertAll(civilians);
	}
}
