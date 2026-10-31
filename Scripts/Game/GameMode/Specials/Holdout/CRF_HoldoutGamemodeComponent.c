//------------------------------------------------------------------------------------
// CRF_HoldoutGamemodeComponent: "Holdout" - hold a building against waves of the undead
//
//   1. Prep (m_fPrepSeconds): the defenders get into the building and board up its entrances. The
//      ground-floor exterior doors are found when the mission starts and each gets a barricade
//      (CRF_HoldoutBarricadeComponent), empty. Boarding needs an entrenching tool; anyone who spawns
//      without one is given one.
//   2. Waves: zombie groups arrive from the spawn points around the town, a few seconds apart, each
//      wave bigger than the last - and sized for the defenders alive when it starts - and tougher (they shrug off more damage, m_fToughnessPerWave). They
//      head for the nearest defender and tear the boards off to get in. A wave is over when its last
//      zombie is dead.
//   3. Lull (m_fLullSeconds): the supply crate inside is restocked. Every m_iSupportEveryWaves waves
//      the radio can call in support once: the fallen come back and an extra resupply is dropped.
//
//   Windows: ground-floor windows are barricaded too (they start boarded, and are repaired like doors).
//   Each zombie group is sent at one barricade; once a window's boards are gone, zombies at it climb in.
//   Windows that are separate parts of the building are found by themselves; ones built into the
//   building's model can't be, so place a marker in each (m_aWindowNames). The log says what was found.
//
//   Requisition: defenders earn money for kills (m_iKillReward), for assists - hurting a zombie someone
//   else killed by at least m_fAssistMinDamage (m_iAssistReward) -, for every board they put back
//   (m_iRepairReward) and for each wave they live through (m_iWaveReward). They spend it at the supply
//   crate on the quartermaster's catalogue (m_aShop): weapons with ammunition, grenades, medical kits,
//   and upgrades for the whole defence - reinforced boards, a fourth board, a bigger resupply, a radio
//   recharge.
//
//   It ends when every defender is down. Score: waves survived.
//
// Setup for mission makers
// 1. Add this component to the gamemode entity and set the defending faction.
// 2. Name an entity next to the building to defend and set m_sBuildingAnchorName. Its entrances are
//    found by themselves (doors with a roof on one side only, at ground level); to place them by hand
//    instead, list entities standing in the doorways, facing out, as m_aEntranceNames. Windows the
//    search misses: entities at each window's sill, facing out, as m_aWindowNames.
// 3. Place a few empty entities around the area and list them as zombie spawn points.
// 4. Respawns off: support calls bring the fallen back. A spawn point for the defenders near the
//    building is needed for that.
// 5. Factions: the zombie faction (BACON_622120A5448725E3_FACTION) must be hostile to the defenders.
//------------------------------------------------------------------------------------

enum CRF_EHoldoutPhase
{
	PREP,
	WAVE,
	LULL,
	ENDED
}

//------------------------------------------------------------------------------------------------
//! An opening to barricade: where its bottom is, which way is out, its size; for a window, the floor
//! inside it where climbing zombies land
class CRF_HoldoutEntrance
{
	vector m_vFloor;
	vector m_vOutward;
	float m_fWidth;
	float m_fHeight;
	bool m_bWindow;
	vector m_vInside;
}

//------------------------------------------------------------------------------------------------
enum CRF_EHoldoutShopKind
{
	ITEMS,				//!< the items go to the buyer
	REINFORCED_BOARDS,	//!< every barricade's boards take more punishment
	EXTRA_BOARD,		//!< every barricade takes one more board
	DOUBLE_RESUPPLY,	//!< the crate gets twice as much each lull
	RADIO_RECHARGE		//!< the radio can call support again now
}

//------------------------------------------------------------------------------------------------
//! One line of the quartermaster's catalogue
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sName")]
class CRF_HoldoutShopEntry
{
	[Attribute(desc: "What it is called at the crate")]
	string m_sName;

	[Attribute("100", UIWidgets.EditBox, "Price", params: "0 100000 1")]
	int m_iPrice;

	[Attribute("0", UIWidgets.ComboBox, "What buying it does", enums: ParamEnumArray.FromEnum(CRF_EHoldoutShopKind))]
	CRF_EHoldoutShopKind m_eKind;

	[Attribute(desc: "Items given to the buyer (Items kind); list one several times for more", params: "et")]
	ref array<ResourceName> m_aItems;
}


[ComponentEditorProps(category: "Game Mode Component", description: "Holdout: defend a building against waves of zombies, boarding up its entrances.")]
class CRF_HoldoutGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_HoldoutGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction of the defenders", category: "Holdout")]
	protected string m_sDefenderFactionKey;

	[Attribute("BACON_622120A5448725E3_FACTION", UIWidgets.EditBox, "Faction of the zombies", category: "Holdout")]
	protected string m_sZombieFactionKey;

	[Attribute("", UIWidgets.EditBox, "Name of an entity next to (or in) the building to defend. The nearest building within 30 m is used.", category: "Holdout - Building")]
	protected string m_sBuildingAnchorName;

	[Attribute("", UIWidgets.Auto, "Optional: entities standing in doorways, facing out, to barricade instead of finding the entrances", category: "Holdout - Building")]
	protected ref array<string> m_aEntranceNames;

	[Attribute("", UIWidgets.Auto, "Optional: entities placed in ground-floor windows - at the sill, facing out - to barricade as windows. Added to any windows found by themselves; use it for the ones that aren't (the log says what the search found).", category: "Holdout - Building")]
	protected ref array<string> m_aWindowNames;

	[Attribute("1.2", UIWidgets.EditBox, "Width (metres) of the barricade in a hand-placed window", params: "0.4 4 0.1", category: "Holdout - Building")]
	protected float m_fPlacedWindowWidth;

	[Attribute("1.3", UIWidgets.EditBox, "Height (metres) of the barricade in a hand-placed window, up from the sill", params: "0.4 3 0.1", category: "Holdout - Building")]
	protected float m_fPlacedWindowHeight;

	[Attribute("2.2", UIWidgets.EditBox, "A door counts as ground level when its sill is at most this far (metres) above the ground outside", params: "0 10 0.1", category: "Holdout - Building")]
	protected float m_fMaxEntranceHeight;

	[Attribute("{7C1AEA0000000101}Prefabs/Props/Holdout/CRF_HoldoutBarricade.et", UIWidgets.ResourceNamePicker, "Barricade put in each entrance", params: "et", category: "Holdout - Building")]
	protected ResourceName m_sBarricadePrefab;

	[Attribute("{07114788AA3E7D4D}Prefabs/Items/Equipment/Accessories/ETool_M1910/ETool_M1910_FreeRoamBuilding_Gadget.et", UIWidgets.ResourceNamePicker, "Entrenching tool given to defenders who spawn without one. Empty = none.", params: "et", category: "Holdout - Building")]
	protected ResourceName m_sEntrenchingToolPrefab;

	[Attribute("90", UIWidgets.EditBox, "Seconds from the round going live to the first wave", params: "0 1200 1", category: "Holdout - Waves")]
	protected float m_fPrepSeconds;

	[Attribute("45", UIWidgets.EditBox, "Seconds between one wave's last zombie dying and the next wave", params: "5 600 1", category: "Holdout - Waves")]
	protected float m_fLullSeconds;

	[Attribute("", UIWidgets.EditBox, "Names of the entities zombies come from. Put them on streets around the area, out of sight of the building.", category: "Holdout - Waves")]
	protected ref array<string> m_aSpawnPointNames;

	[Attribute("60", UIWidgets.EditBox, "A spawn point is only used if every defender is at least this far from it (metres)", params: "0 2000 5", category: "Holdout - Waves")]
	protected float m_fMinSpawnDistance;

	[Attribute("", UIWidgets.ResourceNamePicker, "Group prefabs for the waves", params: "et", category: "Holdout - Waves")]
	protected ref array<ResourceName> m_aGroups;

	[Attribute("", UIWidgets.ResourceNamePicker, "Group prefabs mixed in from Large Group Wave on", params: "et", category: "Holdout - Waves")]
	protected ref array<ResourceName> m_aLargeGroups;

	[Attribute("4", UIWidgets.EditBox, "Wave from which the large groups are mixed in", params: "1 100 1", category: "Holdout - Waves")]
	protected int m_iLargeGroupWave;

	[Attribute("2", UIWidgets.EditBox, "Groups in the first wave", params: "1 100 1", category: "Holdout - Waves")]
	protected int m_iFirstWaveGroups;

	[Attribute("1", UIWidgets.EditBox, "Groups added with each wave", params: "0 20 0.1", category: "Holdout - Waves")]
	protected float m_fGroupsAddedPerWave;

	[Attribute("16", UIWidgets.EditBox, "Most groups in one wave", params: "1 200 1", category: "Holdout - Waves")]
	protected int m_iMaxGroupsPerWave;

	[Attribute("12", UIWidgets.EditBox, "Seconds between a wave's groups arriving", params: "1 120 0.5", category: "Holdout - Waves")]
	protected float m_fGroupInterval;

	[Attribute("0.5", UIWidgets.EditBox, "Seconds taken off the gap between groups each wave (never below 3)", params: "0 10 0.1", category: "Holdout - Waves")]
	protected float m_fGroupIntervalDropPerWave;

	[Attribute("40", UIWidgets.EditBox, "Most zombies alive at once for Players Per Wave Scale defenders; scaled with the defenders like the groups, up to Max Zombies Cap", params: "1 500 1", category: "Holdout - Waves")]
	protected int m_iMaxZombies;

	[Attribute("8", UIWidgets.EditBox, "Defenders the wave settings above are set for. Each wave is scaled by the defenders alive when it starts against this: twice as many, twice the groups (between Min and Max Player Scale).", params: "1 128 1", category: "Holdout - Waves")]
	protected int m_iPlayersPerWaveScale;

	[Attribute("0.5", UIWidgets.EditBox, "Smallest wave scale, however few defenders are left", params: "0.1 1 0.05", category: "Holdout - Waves")]
	protected float m_fMinPlayerScale;

	[Attribute("4", UIWidgets.EditBox, "Largest wave scale, however many defenders there are", params: "1 20 0.1", category: "Holdout - Waves")]
	protected float m_fMaxPlayerScale;

	[Attribute("110", UIWidgets.EditBox, "Hard cap on zombies alive at once, whatever the scale (server load)", params: "1 500 1", category: "Holdout - Waves")]
	protected int m_iMaxZombiesCap;

	[Attribute("0.07", UIWidgets.EditBox, "Each wave after the first, zombies take this much less of the damage dealt to them (0.07 = 7% a wave)", params: "0 0.5 0.01", category: "Holdout - Waves")]
	protected float m_fToughnessPerWave;

	[Attribute("0.35", UIWidgets.EditBox, "The least of the damage dealt that zombies ever take", params: "0.05 1 0.01", category: "Holdout - Waves")]
	protected float m_fMinZombieDamageScale;

	[Attribute("15", UIWidgets.EditBox, "Seconds between pointing every group at the nearest defender again", params: "3 120 1", category: "Holdout - Waves")]
	protected float m_fRetargetSeconds;

	[Attribute("{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et", UIWidgets.ResourceNamePicker, "Waypoint given to zombie groups at the nearest defender", params: "et", category: "Holdout - Waves")]
	protected ResourceName m_sWaypointPrefab;

	[Attribute("{7C1AE1F2A3B4C800}Sounds/Halloween/CRF_DistantSiren.wav", UIWidgets.ResourceNamePicker, "Sound everyone hears when the first wave comes. Empty = none.", params: "wav", category: "Holdout - Waves")]
	protected ResourceName m_sFirstWaveSound;

	[Attribute("{7C1AEA0000000104}Prefabs/Props/Holdout/CRF_HoldoutSupplyCrate.et", UIWidgets.ResourceNamePicker, "Supply crate, put inside the building: restocked every lull, and where the catalogue is bought (it carries the requisition actions)", params: "et", category: "Holdout - Supplies")]
	protected ResourceName m_sSupplyCratePrefab;

	[Attribute("", UIWidgets.ResourceNamePicker, "Items laid round the crate free each lull - dressings and the like; ammunition is bought (list one several times for more). Leftovers still lying there are cleared first.", params: "et", category: "Holdout - Supplies")]
	protected ref array<ResourceName> m_aSupplyItems;

	[Attribute("{7C1AEA0000000103}Prefabs/Props/Holdout/CRF_HoldoutRadio.et", UIWidgets.ResourceNamePicker, "Radio, put inside the building, that calls in support", params: "et", category: "Holdout - Supplies")]
	protected ResourceName m_sRadioPrefab;

	[Attribute("5", UIWidgets.EditBox, "The radio can call in support once after every this many waves. 0 = never.", params: "0 100 1", category: "Holdout - Supplies")]
	protected int m_iSupportEveryWaves;

	[Attribute("1", UIWidgets.CheckBox, "Advance the gamemode to the AAR when the defenders are overrun", category: "Holdout")]
	protected bool m_bEndMissionWhenOverrun;

	[Attribute("1", UIWidgets.CheckBox, "Barricade the ground-floor windows too (they start boarded). Zombies climb in through one with no boards left.", category: "Holdout - Building")]
	protected bool m_bBarricadeWindows;

	[Attribute("2.4", UIWidgets.EditBox, "A window counts as ground floor when its sill is at most this far (metres) above the ground outside", params: "0 10 0.1", category: "Holdout - Building")]
	protected float m_fMaxWindowSill;

	[Attribute("250", UIWidgets.EditBox, "Money each defender starts with", params: "0 100000 1", category: "Holdout - Requisition")]
	protected int m_iStartingMoney;

	[Attribute("50", UIWidgets.EditBox, "Money for each zombie killed", params: "0 10000 1", category: "Holdout - Requisition")]
	protected int m_iKillReward;

	[Attribute("25", UIWidgets.EditBox, "Money for an assist: every defender other than the killer who did at least Assist Min Damage to the zombie", params: "0 10000 1", category: "Holdout - Requisition")]
	protected int m_iAssistReward;

	[Attribute("10", UIWidgets.EditBox, "Damage a defender must do to a zombie (before its wave toughness) to get an assist when it dies", params: "0 1000 1", category: "Holdout - Requisition")]
	protected float m_fAssistMinDamage;

	[Attribute("20", UIWidgets.EditBox, "Money for each board put back on a barricade", params: "0 10000 1", category: "Holdout - Requisition")]
	protected int m_iRepairReward;

	[Attribute("100", UIWidgets.EditBox, "Money for every defender alive at the end of a wave", params: "0 10000 1", category: "Holdout - Requisition")]
	protected int m_iWaveReward;

	[Attribute("1.75", UIWidgets.EditBox, "Board toughness with the reinforced boards upgrade (times the barricade's)", params: "1 10 0.05", category: "Holdout - Requisition")]
	protected float m_fReinforcedBoardScale;

	[Attribute(desc: "The quartermaster's catalogue, bought at the supply crate. Empty = US Late War weapons, ammunition, grenades, medical kits and the defence upgrades. The crate prefab has a slot for each of the first 24.", category: "Holdout - Requisition")]
	protected ref array<ref CRF_HoldoutShopEntry> m_aShop;

	//===================================================================================
	// STATE
	//===================================================================================

	[RplProp()]
	protected int m_iPhase;

	[RplProp()]
	protected int m_iWave;

	[RplProp()]
	protected int m_iPhaseSecondsLeft;

	[RplProp()]
	protected bool m_bSupportReady;

	// Upgrades bought (1 << CRF_EHoldoutShopKind)
	[RplProp()]
	protected int m_iUpgrades;

	// Everyone's money, packed "playerId:money;..." - the server's m_mMoney, sent when it changes
	[RplProp(onRplName: "OnMoneyChanged")]
	protected string m_sMoney;

	// Every machine: money by player (the server's is the real one; clients unpack m_sMoney into it)
	protected ref map<int, int> m_mMoney = new map<int, int>();
	protected bool m_bMoneyDirty;

	// Server: damage done to each zombie, by player, for assists (zombie RplId -> playerId -> damage)
	protected ref map<RplId, ref map<int, float>> m_mZombieDamage = new map<RplId, ref map<int, float>>();

	// Every machine: the catalogue in use (m_aShop, or the default)
	protected ref array<ref CRF_HoldoutShopEntry> m_aShopEntries = {};

	// Client: the m_sMoney last unpacked
	protected string m_sUnpackedMoney;

	// Server
	protected bool m_bRoundLive;
	protected float m_fElapsedSeconds;
	protected IEntity m_Building;
	protected ref array<IEntity> m_aSpawnPoints = {};
	protected ref array<IEntity> m_aBarricades = {};
	protected IEntity m_SupplyCrate;
	protected IEntity m_Radio;
	protected vector m_vSupplyCentre;
	protected ref array<RplId> m_aSupplyItemIds = {};
	protected int m_iGroupsToSpawn;
	protected float m_fWaveScale = 1;	// this wave's size against Players Per Wave Scale defenders
	protected float m_fNextGroupAt;
	protected float m_fNextRetargetAt;
	protected float m_fNextDoorCheckAt;
	protected int m_iNobodyAliveSeconds;
	protected bool m_bAnnouncedFew;
	protected ref array<RplId> m_aToolGivenBodies = {};
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Zombie groups, their current waypoint, when they spawned and the barricade they were sent at
	// (index into m_aBarricades, -1 = none) - same index
	protected ref array<SCR_AIGroup> m_aGroupsAlive = {};
	protected ref array<IEntity> m_aGroupWaypoints = {};
	protected ref array<float> m_aGroupSpawnedAt = {};
	protected ref array<int> m_aGroupBreach = {};

	// Client
	protected ref CRF_SpecialsMarkerSet m_Markers = new CRF_SpecialsMarkerSet();
	protected ref CRF_HoldoutHUD m_Hud;

	// Scratch for queries
	protected ref array<IEntity> m_aFound = {};

	protected static CRF_HoldoutGamemodeComponent s_Instance;

	protected static const string LOG = "[CRF_Holdout]";
	protected static const int NOBODY_ALIVE_GRACE = 10;
	protected static const float GROUP_SPAWN_GRACE = 60;
	protected static const float MERGE_DOORS_WITHIN = 2.2;
	protected static const float ROOF_PROBE_OUT = 1.8;
	protected static const float ROOF_PROBE_UP = 14;
	protected static const float SUPPLY_ITEM_RING = 1.3;
	protected static const float DOOR_CHECK_SECONDS = 2;	// how often stuck zombies force doors (ForceDoorsOpen)
	protected static const float DOOR_FORCE_RADIUS = 1.8;	// a closed door this close to a stuck zombie is forced

	// Bacon Zombies groups, used when the mission doesn't set its own
	protected static const ResourceName BACON_SOLDIERS = "{58251EDC277CE499}622120A5448725E3/Prefabs/Groups/Group_Zombies_USSR.et";
	protected static const ResourceName BACON_HORDE = "{6AEA65C9E88BB0B2}622120A5448725E3/Prefabs/Groups/Group_Zombies_Horde.et";

	// The crate prefab has requisition actions for this many catalogue slots
	protected static const int SHOP_SLOTS = 24;

	protected static const ResourceName MARKER_ICON = "{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds";

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_HoldoutGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_HoldoutGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (m_aShop && !m_aShop.IsEmpty())
		{
			foreach (CRF_HoldoutShopEntry entry : m_aShop)
				m_aShopEntries.Insert(entry);
		}
		else
		{
			BuildDefaultShop();
		}

		if (GetGame().InPlayMode())
			SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!System.IsConsoleApp())
		{
			GetGame().GetCallqueue().CallLater(UpdateMarkers, 1000, true);

			// The wave and money panel; often enough to step aside promptly when a menu opens
			m_Hud = new CRF_HoldoutHUD();
			GetGame().GetCallqueue().CallLater(UpdateHud, 250, true);
		}

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

		m_iPhase = CRF_EHoldoutPhase.PREP;
		m_iPhaseSecondsLeft = (int)m_fPrepSeconds;
		Replication.BumpMe();

		// The building, its barricades, the crate and the radio once the world has streamed in
		GetGame().GetCallqueue().CallLater(PrepareBuilding, 3000, false);

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
			GetGame().GetCallqueue().Remove(UpdateHud);
			GetGame().GetCallqueue().Remove(PrepareBuilding);
		}

		m_Markers.Clear();
		if (m_Hud)
			m_Hud.Destroy();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive)
			StartRound();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGamemodeStateChanged()
	{
		if (m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME && (!m_SafestartManager || !m_SafestartManager.GetSafestartStatus()))
			StartRound();
	}

	//------------------------------------------------------------------------------------------------
	protected void StartRound()
	{
		if (m_bRoundLive || m_iPhase == CRF_EHoldoutPhase.ENDED)
			return;

		m_bRoundLive = true;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);

		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFactionKey, "Get inside and board it up",
			string.Format("They'll be here in %1. Board up every ground-floor door: use the barricade in each doorway with your entrenching tool. There are supplies in the crate inside.", CRF_SpecialsHelper.FormatSeconds((int)m_fPrepSeconds)), 15);
	}

	//===================================================================================
	// THE BUILDING (server)
	//===================================================================================

	protected void PrepareBuilding()
	{
		array<ref CRF_HoldoutEntrance> entrances = {};
		if (m_aEntranceNames && !m_aEntranceNames.IsEmpty())
			GetPlacedEntrances(entrances);
		else
		{
			m_Building = FindBuilding();
			if (m_Building)
				FindEntrances(m_Building, entrances);
		}

		GetPlacedWindows(entrances);

		if (entrances.IsEmpty())
		{
			Print(LOG + " No entrances to barricade - check Building Anchor Name, or list them in Entrance Names.", LogLevel.ERROR);
			return;
		}

		// Windows start boarded, doors empty (the defenders need a way in)
		CRF_HoldoutEntrance first;
		foreach (CRF_HoldoutEntrance entrance : entrances)
		{
			IEntity barricade = SpawnBarricade(entrance);
			if (barricade)
				m_aBarricades.Insert(barricade);

			if (!first && !entrance.m_bWindow)
				first = entrance;
		}

		// The crate and the radio just inside the first doors
		if (!first)
			first = entrances[0];
		m_vSupplyCentre = first.m_vFloor - first.m_vOutward * 2.6;
		m_SupplyCrate = SpawnAt(m_sSupplyCratePrefab, m_vSupplyCentre, first.m_vOutward);
		RestockSupplies(1);

		CRF_HoldoutEntrance radioEntrance = first;
		foreach (CRF_HoldoutEntrance other : entrances)
		{
			if (!other.m_bWindow && other != first)
			{
				radioEntrance = other;
				break;
			}
		}

		vector radioAt = radioEntrance.m_vFloor - radioEntrance.m_vOutward * 2.2;
		if (radioEntrance == first)
			radioAt = radioAt + Vector(first.m_vOutward[2], 0, -first.m_vOutward[0]) * 2.0;
		m_Radio = SpawnAt(m_sRadioPrefab, radioAt, radioEntrance.m_vOutward * -1);

		int windows = 0;
		foreach (CRF_HoldoutEntrance counted : entrances)
		{
			if (counted.m_bWindow)
				windows++;
		}

		Print(string.Format("%1 %2 door(s) and %3 window(s) barricaded.", LOG, entrances.Count() - windows, windows), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! The building nearest the anchor (within 30 m)
	protected IEntity FindBuilding()
	{
		IEntity anchor = CRF_SpecialsHelper.FindNamedEntity(m_sBuildingAnchorName, LOG);
		if (!anchor)
			return null;

		m_aFound.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(anchor.GetOrigin(), 30, CollectBuilding, null, EQueryEntitiesFlags.ALL);

		IEntity best;
		float bestDistance = 1e10;
		foreach (IEntity building : m_aFound)
		{
			float distance = vector.DistanceSqXZ(building.GetOrigin(), anchor.GetOrigin());
			if (distance < bestDistance)
			{
				best = building;
				bestDistance = distance;
			}
		}
		m_aFound.Clear();

		if (!best)
			Print(LOG + " No building within 30 m of " + m_sBuildingAnchorName + ".", LogLevel.ERROR);

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CollectBuilding(IEntity entity)
	{
		if (SCR_DestructibleBuildingEntity.Cast(entity) && !m_aFound.Contains(entity))
			m_aFound.Insert(entity);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Its doors that lead outside at ground level: a roof on one side only, sill near the ground. Pairs
	//! of leaves (double doors) become one entrance.
	protected void FindEntrances(notnull IEntity building, notnull array<ref CRF_HoldoutEntrance> outEntrances)
	{
		array<IEntity> leaves = {};
		CollectDoors(building, leaves, 0);

		BaseWorld world = GetGame().GetWorld();
		foreach (IEntity leaf : leaves)
		{
			vector mins, maxs;
			leaf.GetWorldBounds(mins, maxs);
			vector centre = (mins + maxs) * 0.5;

			float floorY = mins[1];
			if (floorY - world.GetSurfaceY(centre[0], centre[2]) > m_fMaxEntranceHeight)
				continue;

			// The leaf is thin across the opening: that axis is the way through
			vector localMins, localMaxs;
			leaf.GetBounds(localMins, localMaxs);
			vector transform[4];
			leaf.GetWorldTransform(transform);
			vector through = transform[2];
			float width = localMaxs[0] - localMins[0];
			if (localMaxs[0] - localMins[0] < localMaxs[2] - localMins[2])
			{
				through = transform[0];
				width = localMaxs[2] - localMins[2];
			}

			through[1] = 0;
			through.Normalize();

			bool roofAhead = IsCovered(centre + through * ROOF_PROBE_OUT, floorY);
			bool roofBehind = IsCovered(centre - through * ROOF_PROBE_OUT, floorY);
			if (roofAhead == roofBehind)
				continue; // inside the building, or open on both sides

			vector outward = through;
			if (roofAhead)
				outward = through * -1;

			vector floorPoint = Vector(centre[0], floorY, centre[2]);
			CRF_HoldoutEntrance pair = FindNearbyEntrance(outEntrances, floorPoint);
			if (pair)
			{
				// The other leaf of a double door: one opening across both
				float span = vector.DistanceXZ(pair.m_vFloor, floorPoint);
				pair.m_vFloor = (pair.m_vFloor + floorPoint) * 0.5;
				pair.m_fWidth = Math.Max(pair.m_fWidth, span + width);
				continue;
			}

			CRF_HoldoutEntrance entrance = new CRF_HoldoutEntrance();
			entrance.m_vFloor = floorPoint;
			entrance.m_vOutward = outward;
			entrance.m_fWidth = width;
			entrance.m_fHeight = Math.Clamp(maxs[1] - mins[1], 1.8, 2.6);
			outEntrances.Insert(entrance);
		}

		if (m_bBarricadeWindows)
			FindWindows(building, outEntrances);
	}

	//------------------------------------------------------------------------------------------------
	//! Its ground-floor windows: window parts (not bare glass) with the sill near the ground and a roof
	//! on one side only
	protected void FindWindows(notnull IEntity building, notnull array<ref CRF_HoldoutEntrance> outEntrances)
	{
		array<IEntity> windows = {};
		CollectWindows(building, windows, 0);

		string buildingPrefab = "?";
		if (building.GetPrefabData())
			buildingPrefab = building.GetPrefabData().GetPrefabName();

		// Some buildings have their windows in the building's own model rather than as parts: nothing to find
		if (windows.IsEmpty())
		{
			Print(string.Format("%1 No window parts in %2 - its windows are part of the building's model. Place markers in them and list them in Window Names to barricade them.", LOG, buildingPrefab), LogLevel.WARNING);
			return;
		}

		// Why windows were left out, for the log
		int tooHigh, tooSmall, notOutside;

		BaseWorld world = GetGame().GetWorld();
		foreach (IEntity window : windows)
		{
			vector mins, maxs;
			window.GetWorldBounds(mins, maxs);
			vector centre = (mins + maxs) * 0.5;
			float sill = mins[1];
			if (sill - world.GetSurfaceY(centre[0], centre[2]) > m_fMaxWindowSill)
			{
				tooHigh++;
				continue;
			}

			vector localMins, localMaxs;
			window.GetBounds(localMins, localMaxs);
			vector transform[4];
			window.GetWorldTransform(transform);
			vector through = transform[2];
			float width = localMaxs[0] - localMins[0];
			if (localMaxs[0] - localMins[0] < localMaxs[2] - localMins[2])
			{
				through = transform[0];
				width = localMaxs[2] - localMins[2];
			}

			float height = maxs[1] - mins[1];
			if (width < 0.4 || height < 0.4)
			{
				tooSmall++;
				continue;
			}

			through[1] = 0;
			through.Normalize();

			bool roofAhead = IsCovered(centre + through * ROOF_PROBE_OUT, sill);
			bool roofBehind = IsCovered(centre - through * ROOF_PROBE_OUT, sill);
			if (roofAhead == roofBehind)
			{
				notOutside++;
				continue;
			}

			vector outward = through;
			if (roofAhead)
				outward = through * -1;

			// Where a zombie coming through lands: the floor a metre inside
			vector inside = Vector(centre[0], sill + 0.3, centre[2]) - outward * 1.0;
			inside[1] = FindFloor(inside, sill - 3);

			CRF_HoldoutEntrance entrance = new CRF_HoldoutEntrance();
			entrance.m_vFloor = Vector(centre[0], sill, centre[2]);
			entrance.m_vOutward = outward;
			entrance.m_fWidth = width;
			entrance.m_fHeight = height;
			entrance.m_bWindow = true;
			entrance.m_vInside = inside;
			outEntrances.Insert(entrance);
		}

		Print(string.Format("%1 Windows in %2: %3 part(s), %4 used; left out %5 above the ground floor (Max Window Sill), %6 too small, %7 not between inside and outside.",
			LOG, buildingPrefab, windows.Count(), windows.Count() - tooHigh - tooSmall - notOutside, tooHigh, tooSmall, notOutside), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Windows placed by hand (m_aWindowNames): each marker sits at the sill, facing out
	protected void GetPlacedWindows(notnull array<ref CRF_HoldoutEntrance> outEntrances)
	{
		if (!m_aWindowNames)
			return;

		foreach (string windowName : m_aWindowNames)
		{
			IEntity marker = CRF_SpecialsHelper.FindNamedEntity(windowName, LOG);
			if (!marker)
				continue;

			vector transform[4];
			marker.GetWorldTransform(transform);
			vector outward = transform[2];
			outward[1] = 0;
			outward.Normalize();

			vector sillPoint = marker.GetOrigin();

			// Where a zombie coming through lands: the floor a metre inside
			vector inside = sillPoint + Vector(0, 0.3, 0) - outward * 1.0;
			inside[1] = FindFloor(inside, sillPoint[1] - 3);

			CRF_HoldoutEntrance entrance = new CRF_HoldoutEntrance();
			entrance.m_vFloor = sillPoint;
			entrance.m_vOutward = outward;
			entrance.m_fWidth = m_fPlacedWindowWidth;
			entrance.m_fHeight = m_fPlacedWindowHeight;
			entrance.m_bWindow = true;
			entrance.m_vInside = inside;
			outEntrances.Insert(entrance);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Window parts anywhere in the building's hierarchy - by prefab, as they have no component of their own
	protected void CollectWindows(IEntity parent, notnull array<IEntity> outWindows, int depth)
	{
		IEntity child = parent.GetChildren();
		while (child)
		{
			EntityPrefabData prefabData = child.GetPrefabData();
			string prefab;
			if (prefabData)
				prefab = prefabData.GetPrefabName();

			if (prefab.Contains("/Windows/") && !prefab.Contains("/Glass_"))
				outWindows.Insert(child);
			else if (depth < 3)
				CollectWindows(child, outWindows, depth + 1);

			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Height of the first floor below a point (down to lowest), or the point's own height if none
	protected float FindFloor(vector point, float lowest)
	{
		TraceParam trace = new TraceParam();
		trace.Start = point;
		trace.End = Vector(point[0], lowest, point[2]);
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.LayerMask = EPhysicsLayerDefs.Projectile;
		float hit = GetGame().GetWorld().TraceMove(trace, null);
		if (hit >= 1)
			return point[1];

		return point[1] + (lowest - point[1]) * hit + 0.05;
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_HoldoutEntrance FindNearbyEntrance(array<ref CRF_HoldoutEntrance> entrances, vector floorPoint)
	{
		foreach (CRF_HoldoutEntrance entrance : entrances)
		{
			if (vector.DistanceXZ(entrance.m_vFloor, floorPoint) <= MERGE_DOORS_WITHIN && Math.AbsFloat(entrance.m_vFloor[1] - floorPoint[1]) < 0.5)
				return entrance;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Door leaves anywhere in the building's hierarchy (they hang off door frames, which hang off the building)
	protected void CollectDoors(IEntity parent, notnull array<IEntity> outLeaves, int depth)
	{
		IEntity child = parent.GetChildren();
		while (child)
		{
			if (child.FindComponent(DoorComponent))
				outLeaves.Insert(child);
			else if (depth < 3)
				CollectDoors(child, outLeaves, depth + 1);

			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Something overhead (a ceiling) above this point, standing on floorY
	protected bool IsCovered(vector point, float floorY)
	{
		TraceParam trace = new TraceParam();
		trace.Start = Vector(point[0], floorY + 1.2, point[2]);
		trace.End = trace.Start + Vector(0, ROOF_PROBE_UP, 0);
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.LayerMask = EPhysicsLayerDefs.Projectile;
		return GetGame().GetWorld().TraceMove(trace, null) < 1;
	}

	//------------------------------------------------------------------------------------------------
	//! Entrances placed by hand: the entity stands in the doorway facing out
	protected void GetPlacedEntrances(notnull array<ref CRF_HoldoutEntrance> outEntrances)
	{
		foreach (string entranceName : m_aEntranceNames)
		{
			IEntity marker = CRF_SpecialsHelper.FindNamedEntity(entranceName, LOG);
			if (!marker)
				continue;

			vector transform[4];
			marker.GetWorldTransform(transform);
			vector outward = transform[2];
			outward[1] = 0;
			outward.Normalize();

			CRF_HoldoutEntrance entrance = new CRF_HoldoutEntrance();
			entrance.m_vFloor = marker.GetOrigin();
			entrance.m_vOutward = outward;
			entrance.m_fWidth = 0;
			entrance.m_fHeight = 2;
			outEntrances.Insert(entrance);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A barricade in the opening, a little inside it, facing out. Doors start empty, windows boarded.
	protected IEntity SpawnBarricade(CRF_HoldoutEntrance entrance)
	{
		IEntity barricade = SpawnAt(m_sBarricadePrefab, entrance.m_vFloor - entrance.m_vOutward * 0.2, entrance.m_vOutward);
		if (!barricade)
			return null;

		CRF_HoldoutBarricadeComponent component = CRF_HoldoutBarricadeComponent.Cast(barricade.FindComponent(CRF_HoldoutBarricadeComponent));
		if (component)
		{
			float width = entrance.m_fWidth;
			if (width <= 0)
				width = 1.2;

			int boards = 0;
			vector inside = entrance.m_vInside;
			if (entrance.m_bWindow)
			{
				boards = 99; // full
			}
			else
			{
				// Where a zombie stuck on the threshold is put: the floor a metre past the barricade
				inside = entrance.m_vFloor - entrance.m_vOutward * 1.2 + Vector(0, 1, 0);
				inside[1] = FindFloor(inside, entrance.m_vFloor[1] - 1.5);
			}

			component.Setup(width, entrance.m_fHeight, boards, entrance.m_bWindow, inside);
		}

		return barricade;
	}

	//------------------------------------------------------------------------------------------------
	//! A replicated prefab at a world position, its forward along facing (horizontal)
	protected IEntity SpawnAt(ResourceName prefab, vector position, vector facing)
	{
		if (prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print(string.Format("%1 Can't load '%2'.", LOG, prefab), LogLevel.ERROR);
			return null;
		}

		vector forward = Vector(facing[0], 0, facing[2]).Normalized();
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[0] = Vector(forward[2], 0, -forward[0]);
		spawnParams.Transform[1] = Vector(0, 1, 0);
		spawnParams.Transform[2] = forward;
		spawnParams.Transform[3] = position;
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
	}

	//===================================================================================
	// TICK (1 s, server)
	//===================================================================================

	protected void Tick()
	{
		if (m_iPhase == CRF_EHoldoutPhase.ENDED)
			return;

		m_fElapsedSeconds += 1;
		GiveEntrenchingTools();
		ReplicateMoney();

		switch (m_iPhase)
		{
			case CRF_EHoldoutPhase.PREP:
			case CRF_EHoldoutPhase.LULL:
			{
				m_iPhaseSecondsLeft--;
				Replication.BumpMe();
				if (m_iPhaseSecondsLeft == 10)
					CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFactionKey, "Ten seconds", "Get to the doors.", 4);

				if (m_iPhaseSecondsLeft <= 0)
					StartWave(m_iWave + 1);
				break;
			}

			case CRF_EHoldoutPhase.WAVE:
			{
				UpdateWave();
				break;
			}
		}

		// Everyone down
		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefenderFactionKey, alive);
		if (alive.IsEmpty())
		{
			m_iNobodyAliveSeconds++;
			if (m_iNobodyAliveSeconds >= NOBODY_ALIVE_GRACE)
				Overrun();
		}
		else
		{
			m_iNobodyAliveSeconds = 0;
		}
	}

	//===================================================================================
	// WAVES
	//===================================================================================

	protected void StartWave(int wave)
	{
		ForgetGoneZombies();

		// Sized for the defenders still standing
		array<int> defenders = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefenderFactionKey, defenders);
		m_fWaveScale = Math.Clamp(defenders.Count() / Math.Max(1, m_iPlayersPerWaveScale), m_fMinPlayerScale, m_fMaxPlayerScale);

		int baseGroups = (int)Math.Min(m_iMaxGroupsPerWave, m_iFirstWaveGroups + Math.Floor((wave - 1) * m_fGroupsAddedPerWave));

		m_iWave = wave;
		m_iPhase = CRF_EHoldoutPhase.WAVE;
		m_iPhaseSecondsLeft = 0;
		m_iGroupsToSpawn = (int)Math.Max(1, Math.Round(baseGroups * m_fWaveScale));
		m_fNextGroupAt = m_fElapsedSeconds;
		m_fNextRetargetAt = m_fElapsedSeconds + m_fRetargetSeconds;
		m_bAnnouncedFew = false;
		Replication.BumpMe();

		Print(string.Format("%1 Wave %2: %3 defender(s), scale %4, %5 group(s), at most %6 alive.", LOG, wave, defenders.Count(), m_fWaveScale, m_iGroupsToSpawn, GetZombieCap()), LogLevel.NORMAL);

		string text = "Here they come.";
		if (wave > 1)
			text = string.Format("More of them this time, and harder to put down. (%1 groups)", m_iGroupsToSpawn);
		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("WAVE %1", wave), text, 8);

		if (wave == 1 && !m_sFirstWaveSound.IsEmpty())
		{
			COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
			if (broadcastManager)
				broadcastManager.PlaySoundForEveryone(m_sFirstWaveSound);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateWave()
	{
		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefenderFactionKey, alive);

		// The wave's groups arrive one after another
		if (m_iGroupsToSpawn > 0 && m_fElapsedSeconds >= m_fNextGroupAt && !alive.IsEmpty())
		{
			if (CountZombies() < GetZombieCap() && SpawnGroup(PickGroupPrefab(), alive))
				m_iGroupsToSpawn--;

			// More defenders, more groups - arriving closer together so the wave doesn't drag
			float interval = Math.Max(3, m_fGroupInterval - (m_iWave - 1) * m_fGroupIntervalDropPerWave);
			m_fNextGroupAt = m_fElapsedSeconds + Math.Max(2, interval / Math.Sqrt(Math.Max(1, m_fWaveScale)));
		}

		if (m_fElapsedSeconds >= m_fNextRetargetAt)
		{
			m_fNextRetargetAt = m_fElapsedSeconds + m_fRetargetSeconds;
			UpdateGroups(alive);
		}

		if (m_fElapsedSeconds >= m_fNextDoorCheckAt)
		{
			m_fNextDoorCheckAt = m_fElapsedSeconds + DOOR_CHECK_SECONDS;
			ForceDoorsOpen();
		}

		int zombies = CountZombies();
		if (m_iGroupsToSpawn == 0 && zombies <= 5 && zombies > 0 && !m_bAnnouncedFew)
		{
			m_bAnnouncedFew = true;
			CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFactionKey, "Nearly there", string.Format("%1 left out there.", zombies), 4);
		}

		// Over when the last group is in and every zombie is dead
		if (m_iGroupsToSpawn == 0 && zombies == 0 && !HasGroupStillSpawning())
			EndWave();
	}

	//------------------------------------------------------------------------------------------------
	//! Most zombies alive at once this wave: Max Zombies scaled like the groups, never above the hard cap
	protected int GetZombieCap()
	{
		return (int)Math.Min(m_iMaxZombiesCap, Math.Round(m_iMaxZombies * m_fWaveScale));
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasGroupStillSpawning()
	{
		foreach (int i, SCR_AIGroup group : m_aGroupsAlive)
		{
			if (group && group.GetAgentsCount() == 0 && m_fElapsedSeconds - m_aGroupSpawnedAt[i] < GROUP_SPAWN_GRACE)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void EndWave()
	{
		m_iPhase = CRF_EHoldoutPhase.LULL;
		m_iPhaseSecondsLeft = (int)m_fLullSeconds;

		// Clean up the empty groups
		for (int i = m_aGroupsAlive.Count() - 1; i >= 0; i--)
			ForgetGroup(i, true);

		RestockSupplies(GetResupplyRounds());

		array<int> survivors = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefenderFactionKey, survivors);
		foreach (int survivor : survivors)
			AddMoney(survivor, m_iWaveReward);

		string text = string.Format("Next wave in %1. The crate's been restocked - and check the boards.", CRF_SpecialsHelper.FormatSeconds((int)m_fLullSeconds));
		if (m_iSupportEveryWaves > 0 && m_iWave % m_iSupportEveryWaves == 0)
		{
			m_bSupportReady = true;
			text += " The radio can call in support: the fallen come back, with more supplies.";
		}

		Replication.BumpMe();
		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("WAVE %1 HELD", m_iWave), text, 12);
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName PickGroupPrefab()
	{
		array<ResourceName> pool = {};
		if (m_aGroups && !m_aGroups.IsEmpty())
			pool.Copy(m_aGroups);
		else
			pool.Insert(BACON_SOLDIERS);

		if (m_iWave >= m_iLargeGroupWave)
		{
			if (m_aLargeGroups && !m_aLargeGroups.IsEmpty())
				pool.InsertAll(m_aLargeGroups);
			else
				pool.Insert(BACON_HORDE);
		}

		return pool.GetRandomElement();
	}

	//------------------------------------------------------------------------------------------------
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
		position[0] = position[0] + Math.RandomFloat(-8, 8);
		position[2] = position[2] + Math.RandomFloat(-8, 8);
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = position;

		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams));
		if (!group)
			return false;

		// Each group goes for one barricade, so windows get visitors as well as doors
		int breach = -1;
		if (!m_aBarricades.IsEmpty())
			breach = Math.RandomInt(0, m_aBarricades.Count());

		m_aGroupsAlive.Insert(group);
		m_aGroupWaypoints.Insert(null);
		m_aGroupSpawnedAt.Insert(m_fElapsedSeconds);
		m_aGroupBreach.Insert(breach);
		SendGroupAt(m_aGroupsAlive.Count() - 1, position, alive);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! A random spawn point at least m_fMinSpawnDistance from every defender; failing that the furthest
	protected IEntity PickSpawnPoint(array<int> alive)
	{
		array<IEntity> usable = {};
		IEntity furthest;
		float furthestDistance = -1;

		foreach (IEntity spawnPoint : m_aSpawnPoints)
		{
			float distance = CRF_SpecialsHelper.DistanceToNearestPlayer(spawnPoint.GetOrigin(), alive);
			if (distance >= m_fMinSpawnDistance)
				usable.Insert(spawnPoint);

			if (distance > furthestDistance)
			{
				furthest = spawnPoint;
				furthestDistance = distance;
			}
		}

		if (!usable.IsEmpty())
			return usable.GetRandomElement();

		return furthest;
	}

	//------------------------------------------------------------------------------------------------
	//! Points every group at the nearest living defender again; drops groups that are wiped out
	protected void UpdateGroups(array<int> alive)
	{
		for (int i = m_aGroupsAlive.Count() - 1; i >= 0; i--)
		{
			SCR_AIGroup group = m_aGroupsAlive[i];
			if (!group)
			{
				ForgetGroup(i, false);
				continue;
			}

			IEntity leader = group.GetLeaderEntity();
			if (!leader)
			{
				if (group.GetAgentsCount() == 0 && m_fElapsedSeconds - m_aGroupSpawnedAt[i] >= GROUP_SPAWN_GRACE)
					ForgetGroup(i, true);
				continue;
			}

			if (!alive.IsEmpty())
				SendGroupAt(i, leader.GetOrigin(), alive);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Replaces the group's waypoint: at its barricade while that still has boards to tear off (or, for a
	//! window, while most of the group is still outside it), otherwise at the living defender nearest to from
	protected void SendGroupAt(int index, vector from, array<int> alive)
	{
		SCR_AIGroup group = m_aGroupsAlive[index];
		if (!group)
			return;

		vector targetPosition;
		CRF_HoldoutBarricadeComponent breach = GetGroupBreach(index);
		if (breach && ShouldAssault(group, breach))
		{
			// Just outside it
			targetPosition = breach.GetOwner().GetOrigin() + breach.GetOutward() * 1.5;
		}
		else
		{
			int nearestId;
			CRF_SpecialsHelper.DistanceToNearestPlayer(from, alive, nearestId);
			IEntity target = GetGame().GetPlayerManager().GetPlayerControlledEntity(nearestId);
			if (!target)
				return;

			targetPosition = target.GetOrigin();
		}

		Resource resource = Resource.Load(m_sWaypointPrefab);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = targetPosition;

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

		SCR_AIGroup group = m_aGroupsAlive[index];
		if (group)
			group.RemoveWaypoint(old);

		SCR_EntityHelper.DeleteEntityAndChildren(old);
		m_aGroupWaypoints[index] = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Stops tracking a group; deleteMembers also removes the group and any zombies left in it
	protected void ForgetGroup(int index, bool deleteMembers)
	{
		RemoveGroupWaypoint(index);

		SCR_AIGroup group = m_aGroupsAlive[index];
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

		m_aGroupsAlive.Remove(index);
		m_aGroupWaypoints.Remove(index);
		m_aGroupSpawnedAt.Remove(index);
		m_aGroupBreach.Remove(index);
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_HoldoutBarricadeComponent GetGroupBreach(int index)
	{
		int breach = m_aGroupBreach[index];
		if (!m_aBarricades.IsIndexValid(breach) || !m_aBarricades[breach])
			return null;

		return CRF_HoldoutBarricadeComponent.Cast(m_aBarricades[breach].FindComponent(CRF_HoldoutBarricadeComponent));
	}

	//------------------------------------------------------------------------------------------------
	//! A barricade lost its last board: the groups sent at it get their next waypoint now (the
	//! defenders, or for a window still the window until they're through - see ShouldAssault)
	void OnBarricadeBroken(IEntity barricade)
	{
		int breach = m_aBarricades.Find(barricade);
		if (breach < 0)
			return;

		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefenderFactionKey, alive);
		if (alive.IsEmpty())
			return;

		for (int i = 0; i < m_aGroupsAlive.Count(); i++)
		{
			if (m_aGroupBreach[i] != breach || !m_aGroupsAlive[i])
				continue;

			IEntity leader = m_aGroupsAlive[i].GetLeaderEntity();
			if (leader)
				SendGroupAt(i, leader.GetOrigin(), alive);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Bacon zombies can't open doors (their door reaction is switched off) and stop dead at a closed
	//! one, hopping as they try to climb it - the building's inner doors, or an outer one that wasn't
	//! barricaded. A zombie standing still next to a closed door forces it open.
	protected void ForceDoorsOpen()
	{
		array<AIAgent> agents = {};
		foreach (SCR_AIGroup group : m_aGroupsAlive)
		{
			if (!group)
				continue;

			agents.Clear();
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				SCR_ChimeraCharacter zombie = SCR_ChimeraCharacter.Cast(agent.GetControlledEntity());
				if (!zombie)
					continue;

				// Backstop for the damage manager's UpdateConsciousness: a zombie lying unconscious dies
				CharacterControllerComponent controller = zombie.GetCharacterController();
				if (controller.GetLifeState() == ECharacterLifeState.INCAPACITATED)
				{
					SCR_CharacterDamageManagerComponent damageManager = SCR_CharacterDamageManagerComponent.Cast(zombie.GetDamageManager());
					if (damageManager)
						damageManager.Kill(damageManager.GetInstigator());
					continue;
				}

				if (controller.IsDead() || !CRF_HoldoutBarricadeComponent.IsStalled(zombie))
					continue;

				m_aFound.Clear();
				GetGame().GetWorld().QueryEntitiesBySphere(zombie.GetOrigin() + Vector(0, 1, 0), DOOR_FORCE_RADIUS, CollectDoor, null, EQueryEntitiesFlags.ALL);
				foreach (IEntity door : m_aFound)
				{
					DoorComponent doorComponent = DoorComponent.Cast(door.FindComponent(DoorComponent));
					if (doorComponent && !doorComponent.IsOpen() && !doorComponent.IsOpening())
						doorComponent.SetControlValue(1, RplId.Invalid());
				}
			}
		}

		m_aFound.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected bool CollectDoor(IEntity entity)
	{
		if (entity.FindComponent(DoorComponent) && !m_aFound.Contains(entity))
			m_aFound.Insert(entity);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Keep at the barricade: boards still on it, or a bare window most of the group hasn't come through yet
	protected bool ShouldAssault(notnull SCR_AIGroup group, notnull CRF_HoldoutBarricadeComponent breach)
	{
		if (breach.GetBoards() > 0)
			return true;

		if (!breach.IsWindow())
			return false;

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		int outside = 0;
		foreach (AIAgent agent : agents)
		{
			IEntity member = agent.GetControlledEntity();
			if (member && breach.IsOutside(member.GetOrigin()))
				outside++;
		}

		return outside * 2 > agents.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Living zombies in the wave's groups
	protected int CountZombies()
	{
		int count = 0;
		foreach (SCR_AIGroup group : m_aGroupsAlive)
		{
			if (group)
				count += group.GetAgentsCount();
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! Share of the damage dealt that a zombie takes this wave (CRF_Holdout_SCR_CharacterDamageManagerComponent)
	float GetZombieDamageScale(IEntity victim)
	{
		if (m_iWave <= 1 || !IsZombie(victim))
			return 1;

		return Math.Max(m_fMinZombieDamageScale, 1 - (m_iWave - 1) * m_fToughnessPerWave);
	}

	//------------------------------------------------------------------------------------------------
	//! A character of the zombie faction
	bool IsZombie(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		Faction faction = character.GetFaction();
		return faction && faction.GetFactionKey() == m_sZombieFactionKey;
	}

	//===================================================================================
	// SUPPLIES AND SUPPORT (server)
	//===================================================================================

	//! The crate's leftovers go, then a fresh lot is laid round it (rounds times over)
	protected void RestockSupplies(int rounds)
	{
		if (!m_SupplyCrate || !m_aSupplyItems || m_aSupplyItems.IsEmpty())
			return;

		// Only what still lies by the crate - not what is in someone's kit
		foreach (RplId itemId : m_aSupplyItemIds)
		{
			IEntity leftover = COA_EntityHelper.GetEntityFromRplId(itemId);
			if (leftover && !leftover.GetParent() && vector.DistanceXZ(leftover.GetOrigin(), m_vSupplyCentre) < 4)
				SCR_EntityHelper.DeleteEntityAndChildren(leftover);
		}
		m_aSupplyItemIds.Clear();

		int count = m_aSupplyItems.Count() * rounds;
		float startAngle = Math.RandomFloat(0, Math.PI * 2);
		for (int i = 0; i < count; i++)
		{
			ResourceName prefab = m_aSupplyItems[i % m_aSupplyItems.Count()];
			Resource resource = Resource.Load(prefab);
			if (!resource || !resource.IsValid())
				continue;

			// Rings round the crate, a little wider for each lot
			float angle = startAngle + Math.PI * 2 * i / Math.Max(1, m_aSupplyItems.Count());
			float ring = SUPPLY_ITEM_RING + 0.35 * (i / m_aSupplyItems.Count());
			vector at = m_vSupplyCentre + Vector(Math.Cos(angle) * ring, 0.15, Math.Sin(angle) * ring);

			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			Math3D.AnglesToMatrix(Vector(Math.RandomFloat(0, 360), 0, 0), spawnParams.Transform);
			spawnParams.Transform[3] = at;

			IEntity item = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
			if (!item)
				continue;

			RplComponent itemRpl = RplComponent.Cast(item.FindComponent(RplComponent));
			if (itemRpl)
				m_aSupplyItemIds.Insert(itemRpl.Id());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Defenders on foot without an entrenching tool get one, once per life
	protected void GiveEntrenchingTools()
	{
		if (m_sEntrenchingToolPrefab.IsEmpty())
			return;

		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sDefenderFactionKey, alive);
		foreach (int playerId : alive)
		{
			IEntity body = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!body)
				continue;

			// A defender's first appearance: their starting money
			if (!m_mMoney.Contains(playerId))
				AddMoney(playerId, m_iStartingMoney);

			RplComponent bodyRpl = RplComponent.Cast(body.FindComponent(RplComponent));
			if (!bodyRpl || m_aToolGivenBodies.Contains(bodyRpl.Id()))
				continue;

			// Their gear is put on a moment after they spawn; look again next tick until it has
			SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.GetGadgetManager(body);
			if (!gadgets || m_fElapsedSeconds < 2)
				continue;

			if (!gadgets.GetGadgetByType(EGadgetType.BUILDING_TOOL))
			{
				SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
				if (!inventory || !inventory.TrySpawnPrefabToStorage(m_sEntrenchingToolPrefab))
					continue;
			}

			m_aToolGivenBodies.Insert(bodyRpl.Id());
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsSupportReady()
	{
		return m_bSupportReady && m_iPhase != CRF_EHoldoutPhase.ENDED;
	}

	//------------------------------------------------------------------------------------------------
	string GetSupportNotReadyReason()
	{
		if (m_iSupportEveryWaves <= 0)
			return "No answer";

		int next = (m_iWave / m_iSupportEveryWaves + 1) * m_iSupportEveryWaves;
		return string.Format("No answer - try again after wave %1", next);
	}

	//------------------------------------------------------------------------------------------------
	//! Server (the radio): the fallen come back and an extra resupply is dropped at the crate
	void CallSupport(int callerId)
	{
		if (!IsSupportReady())
			return;

		m_bSupportReady = false;
		Replication.BumpMe();

		int revived = ReviveFallen();
		RestockSupplies(GetResupplyRounds() * 2);

		string caller = GetGame().GetPlayerManager().GetPlayerName(callerId);
		CRF_RadioTransmissionEvents.PopupToFaction("", "Support is on its way",
			string.Format("%1 got through on the radio. %2 reinforcement(s), and a double resupply at the crate.", caller, revived), 10);
	}

	//------------------------------------------------------------------------------------------------
	//! Dead defenders still connected go back in (at the defenders' spawn point)
	protected int ReviveFallen()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!slottingManager || !respawnManager)
			return 0;

		int revived = 0;
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			COA_SlotData slotData = slottingManager.GetPlayerSlotData(playerId);
			if (!slotData || !slotData.GetIsDeadSlot() || slotData.GetSlotFactionKey() != m_sDefenderFactionKey)
				continue;

			respawnManager.RespawnPlayer(playerId);
			revived++;
		}

		return revived;
	}

	//===================================================================================
	// REQUISITION
	//===================================================================================

	//! Server: a zombie a defender killed pays the killer, and everyone else who hurt it enough an assist
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);

		if (!Replication.IsServer() || m_iPhase == CRF_EHoldoutPhase.ENDED)
			return;

		SCR_ChimeraCharacter victim = SCR_ChimeraCharacter.Cast(instigatorContextData.GetVictimEntity());
		if (!victim || !IsZombie(victim))
			return;

		int killerId = instigatorContextData.GetKillerPlayerID();
		if (killerId > 0)
			AddMoney(killerId, m_iKillReward);

		RplId victimId;
		map<int, float> damageBy;
		if (!COA_ReplicationHelper.GetRplId(victim, victimId) || !m_mZombieDamage.Find(victimId, damageBy))
			return;

		m_mZombieDamage.Remove(victimId);
		foreach (int playerId, float damage : damageBy)
		{
			if (playerId != killerId && damage >= m_fAssistMinDamage)
				AddMoney(playerId, m_iAssistReward);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server (SCR_CharacterDamageManagerComponent.HijackDamageHandling): a player hurt a character;
	//! damage to zombies is added up for assists
	void OnZombieDamaged(IEntity zombie, int playerId, float damage)
	{
		if (playerId <= 0 || damage <= 0 || m_iPhase == CRF_EHoldoutPhase.ENDED || !IsZombie(zombie))
			return;

		RplId zombieId;
		if (!COA_ReplicationHelper.GetRplId(zombie, zombieId))
			return;

		map<int, float> damageBy = m_mZombieDamage.Get(zombieId);
		if (!damageBy)
		{
			damageBy = new map<int, float>();
			m_mZombieDamage.Insert(zombieId, damageBy);
		}

		float done;
		damageBy.Find(playerId, done);
		damageBy.Set(playerId, done + damage);
	}

	//------------------------------------------------------------------------------------------------
	//! Server, each wave: forget the damage to zombies that went without dying (cleaned up, stuck)
	protected void ForgetGoneZombies()
	{
		for (int i = m_mZombieDamage.Count() - 1; i >= 0; i--)
		{
			if (!COA_ReplicationHelper.GetEntityFromRplId(m_mZombieDamage.GetKey(i)))
				m_mZombieDamage.RemoveElement(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Server (CRF_HoldoutBarricadeComponent.AddBoard): a board put back pays
	void OnBoardRepaired(int playerId)
	{
		if (playerId > 0)
			AddMoney(playerId, m_iRepairReward);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: money for (or, negative, from) a player; sent to everyone with the next tick
	void AddMoney(int playerId, int amount)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;

		int money;
		m_mMoney.Find(playerId, money);
		m_mMoney.Set(playerId, Math.Max(0, money + amount));
		m_bMoneyDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine
	int GetMoney(int playerId)
	{
		UnpackMoney();

		int money;
		m_mMoney.Find(playerId, money);
		return money;
	}

	//------------------------------------------------------------------------------------------------
	//! Server, once a tick: everyone's money out if it changed
	protected void ReplicateMoney()
	{
		if (!m_bMoneyDirty)
			return;

		m_bMoneyDirty = false;
		string packed;
		foreach (int playerId, int money : m_mMoney)
			packed += string.Format("%1:%2;", playerId, money);

		m_sMoney = packed;
		Replication.BumpMe();
		OnMoneyChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Clients: m_sMoney into m_mMoney, if it changed since the last time. Done on demand as well as from
	//! OnMoneyChanged, so a client that joined mid-round has everyone's money without waiting for a change.
	protected void UnpackMoney()
	{
		if (Replication.IsServer() || m_sUnpackedMoney == m_sMoney)
			return;

		m_sUnpackedMoney = m_sMoney;
		m_mMoney.Clear();
		array<string> entries = {};
		m_sMoney.Split(";", entries, true);
		foreach (string entry : entries)
		{
			array<string> parts = {};
			entry.Split(":", parts, true);
			if (parts.Count() == 2)
				m_mMoney.Set(parts[0].ToInt(), parts[1].ToInt());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine with a player: unpack, and show the local player the change straight away (the HUD
	//! panel flashes the amount and shows the difference)
	protected void OnMoneyChanged()
	{
		UnpackMoney();

		if (!System.IsConsoleApp())
			UpdateHud();
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine with a player: the wave and money panel (CRF_HoldoutHUD)
	protected void UpdateHud()
	{
		if (m_Hud)
			m_Hud.Update(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	CRF_EHoldoutPhase GetPhase()
	{
		return m_iPhase;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated): the wave under way, or the last one held during a lull; 0 before the first
	int GetWave()
	{
		return m_iWave;
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated): seconds until the next wave, during prep and lulls
	int GetPhaseSecondsLeft()
	{
		return m_iPhaseSecondsLeft;
	}

	//------------------------------------------------------------------------------------------------
	string GetDefenderFactionKey()
	{
		return m_sDefenderFactionKey;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasUpgrade(CRF_EHoldoutShopKind kind)
	{
		return (m_iUpgrades & (1 << kind)) != 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Toughness of every board (CRF_HoldoutBarricadeComponent)
	float GetBoardHealthScale()
	{
		if (HasUpgrade(CRF_EHoldoutShopKind.REINFORCED_BOARDS))
			return m_fReinforcedBoardScale;

		return 1;
	}

	//------------------------------------------------------------------------------------------------
	//! Boards every barricade takes beyond its own (CRF_HoldoutBarricadeComponent)
	int GetExtraBoards()
	{
		if (HasUpgrade(CRF_EHoldoutShopKind.EXTRA_BOARD))
			return 1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetResupplyRounds()
	{
		if (HasUpgrade(CRF_EHoldoutShopKind.DOUBLE_RESUPPLY))
			return 2;

		return 1;
	}

	//------------------------------------------------------------------------------------------------
	CRF_HoldoutShopEntry GetShopEntry(int index)
	{
		if (!m_aShopEntries.IsIndexValid(index))
			return null;

		return m_aShopEntries[index];
	}

	//------------------------------------------------------------------------------------------------
	//! A slot shows while it has an entry, and an upgrade only until it is bought
	bool CanShowShopEntry(int index)
	{
		CRF_HoldoutShopEntry entry = GetShopEntry(index);
		if (!entry || m_iPhase == CRF_EHoldoutPhase.ENDED)
			return false;

		switch (entry.m_eKind)
		{
			case CRF_EHoldoutShopKind.ITEMS:
				return true;
			case CRF_EHoldoutShopKind.RADIO_RECHARGE:
				return m_iSupportEveryWaves > 0;
		}

		return !HasUpgrade(entry.m_eKind);
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (the action's check and the server's)
	bool CanBuy(int playerId, int index, out string reason)
	{
		CRF_HoldoutShopEntry entry = GetShopEntry(index);
		if (!entry || !CanShowShopEntry(index))
		{
			reason = "Not available";
			return false;
		}

		if (entry.m_eKind == CRF_EHoldoutShopKind.RADIO_RECHARGE && m_bSupportReady)
		{
			reason = "The radio is already charged";
			return false;
		}

		if (GetMoney(playerId) < entry.m_iPrice)
		{
			reason = string.Format("Needs %1 (you have %2)", entry.m_iPrice, GetMoney(playerId));
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! What the crate's action is called: the entry, its price and the local player's money
	string GetShopEntryLabel(int index)
	{
		CRF_HoldoutShopEntry entry = GetShopEntry(index);
		if (!entry)
			return "";

		return string.Format("%1 - %2 (you have %3)", entry.m_sName, entry.m_iPrice, GetMoney(SCR_PlayerController.GetLocalPlayerId()));
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a player buys a catalogue entry
	void Buy(int playerId, int index)
	{
		string reason;
		if (!Replication.IsServer() || playerId <= 0 || !CanBuy(playerId, index, reason))
			return;

		CRF_HoldoutShopEntry entry = GetShopEntry(index);
		AddMoney(playerId, -entry.m_iPrice);

		string buyer = GetGame().GetPlayerManager().GetPlayerName(playerId);
		switch (entry.m_eKind)
		{
			case CRF_EHoldoutShopKind.ITEMS:
			{
				GiveItems(playerId, entry.m_aItems);
				return;
			}

			case CRF_EHoldoutShopKind.RADIO_RECHARGE:
			{
				m_bSupportReady = true;
				break;
			}

			default:
			{
				m_iUpgrades = m_iUpgrades | (1 << entry.m_eKind);
				break;
			}
		}

		Replication.BumpMe();
		CRF_RadioTransmissionEvents.PopupToFaction(m_sDefenderFactionKey, entry.m_sName, string.Format("Requisitioned by %1.", buyer), 6);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: into the buyer's kit, or at their feet what doesn't fit
	protected void GiveItems(int playerId, array<ResourceName> items)
	{
		IEntity body = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!body || !items)
			return;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		foreach (int i, ResourceName prefab : items)
		{
			if (inventory && inventory.TrySpawnPrefabToStorage(prefab))
				continue;

			Resource resource = Resource.Load(prefab);
			if (!resource || !resource.IsValid())
				continue;

			float angle = Math.PI * 2 * i / Math.Max(1, items.Count());
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			spawnParams.Transform[3] = body.GetOrigin() + Vector(Math.Cos(angle) * 0.6, 0.15, Math.Sin(angle) * 0.6);
			GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_HoldoutShopEntry AddShopEntry(string name, int price, CRF_EHoldoutShopKind kind)
	{
		CRF_HoldoutShopEntry entry = new CRF_HoldoutShopEntry();
		entry.m_sName = name;
		entry.m_iPrice = price;
		entry.m_eKind = kind;
		entry.m_aItems = {};
		m_aShopEntries.Insert(entry);
		return entry;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddShopItems(notnull CRF_HoldoutShopEntry entry, ResourceName prefab, int count)
	{
		for (int i = 0; i < count; i++)
			entry.m_aItems.Insert(prefab);
	}

	//------------------------------------------------------------------------------------------------
	//! The catalogue when the mission doesn't set one: US Late War (Finest Hour) ammunition and weapons - the
	//! defenders start with a pistol and buy the rest - kit, and the upgrades
	protected void BuildDefaultShop()
	{
		ResourceName garand = "{6FF6BC1AA4BC98BE}Prefabs/Weapons/Rifles/M1_Garand/Rifle_M1.et";
		ResourceName garandClip = "{582AFEEBE91494B5}Prefabs/Weapons/Magazines/M1_Garand/Clip_762x63_M1_8rnd_Ball_M2.et";
		ResourceName carbine = "{0C95F90EAD15FC6B}Prefabs/Weapons/Rifles/M1_Carbine/Rifle_M1_Carbine.et";
		ResourceName carbineMag = "{497BEFA61452A6B5}Prefabs/Weapons/Magazines/M1_Carbine/Magazine_762x33_M1_Carbine_15rnd_Ball_M1.et";
		ResourceName greaseGun = "{E14DFCA44B4ADB90}Prefabs/Weapons/Machineguns/M3A1/Submachinegun_M3A1.et";
		ResourceName greaseGunMag = "{30572471CB07D84B}Prefabs/Weapons/Magazines/M3A1_GreaseGun/Magazine_1143x23_M3A1_30rnd_Ball_M1.et";
		ResourceName thompson = "{E59A4E6BBDE7CFBD}Prefabs/Weapons/Machineguns/M1A1/Submachinegun_M1A1.et";
		ResourceName thompsonMag = "{645FF3031157C71E}Prefabs/Weapons/Magazines/M1A1_Thompson/Magazine_1143x23_M1A1_30rnd_Ball_M1.et";
		ResourceName bar = "{C2596CA8BD1C2F08}Prefabs/Weapons/Machineguns/M1918_BAR/Machinegun_M1918.et";
		ResourceName barMag = "{1363AE4797C0E29F}Prefabs/Weapons/Magazines/M1918_BAR/Magazine_762x63_M1918_20rnd_4Ball_1Tracer.et";
		ResourceName springfield = "{4191E250FDEC2FBF}Prefabs/Weapons/Rifles/M1903A3/Rifle_M1903A4.et";
		ResourceName springfieldClip = "{228B4D35573544B0}Prefabs/Weapons/Magazines/M1903A3/Clip_762x63_M1903_5rnd_Ball_M2.et";
		ResourceName pistolMag = "{BEDAB3A48410CF6C}Prefabs/Weapons/Magazines/M1911/Magazine_1143x23_M1911_7rnd_Ball_M1.et";
		ResourceName grenade = "{9404B8186F1501E7}Prefabs/Weapons/Grenades/Mk2/Grenade_Mk2_Olive.et";
		ResourceName dressing = "{A81F501D3EF6F38E}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_US_01.et";
		ResourceName tourniquet = "{D70216B1B2889129}Prefabs/Items/Medicine/Tourniquet_01/Tourniquet_US_01.et";
		ResourceName morphine = "{0D9A5DCF89AE7AA9}Prefabs/Items/Medicine/MorphineInjection_01/MorphineInjection_01.et";

		// Ammunition: nobody gets any for free
		CRF_HoldoutShopEntry entry1 = AddShopEntry("M1911 magazines (4)", 50, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry1, pistolMag, 4);

		CRF_HoldoutShopEntry entry2 = AddShopEntry("M1 Carbine magazines (4)", 75, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry2, carbineMag, 4);

		CRF_HoldoutShopEntry entry3 = AddShopEntry("M1 Garand clips (6)", 100, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry3, garandClip, 6);

		CRF_HoldoutShopEntry entry4 = AddShopEntry("M1903 clips (6)", 75, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry4, springfieldClip, 6);

		CRF_HoldoutShopEntry entry5 = AddShopEntry("M3 grease gun magazines (3)", 100, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry5, greaseGunMag, 3);

		CRF_HoldoutShopEntry entry6 = AddShopEntry("M1A1 Thompson magazines (3)", 125, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry6, thompsonMag, 3);

		CRF_HoldoutShopEntry entry7 = AddShopEntry("M1918 BAR magazines (3)", 150, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry7, barMag, 3);

		// Weapons
		CRF_HoldoutShopEntry entry8 = AddShopEntry("M1 Carbine, 2 magazines", 750, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry8, carbine, 1);
		AddShopItems(entry8, carbineMag, 2);

		CRF_HoldoutShopEntry entry9 = AddShopEntry("M1 Garand, 2 clips", 900, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry9, garand, 1);
		AddShopItems(entry9, garandClip, 2);

		CRF_HoldoutShopEntry entry10 = AddShopEntry("M1903A4 sniper rifle, 2 clips", 1200, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry10, springfield, 1);
		AddShopItems(entry10, springfieldClip, 2);

		CRF_HoldoutShopEntry entry11 = AddShopEntry("M3 grease gun, 2 magazines", 1350, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry11, greaseGun, 1);
		AddShopItems(entry11, greaseGunMag, 2);

		CRF_HoldoutShopEntry entry12 = AddShopEntry("M1A1 Thompson, 2 magazines", 1800, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry12, thompson, 1);
		AddShopItems(entry12, thompsonMag, 2);

		CRF_HoldoutShopEntry entry13 = AddShopEntry("M1918 BAR, 2 magazines", 2700, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry13, bar, 1);
		AddShopItems(entry13, barMag, 2);

		// Kit
		CRF_HoldoutShopEntry entry14 = AddShopEntry("Three Mk 2 grenades", 150, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry14, grenade, 3);

		CRF_HoldoutShopEntry entry15 = AddShopEntry("Medical kit (4 dressings, tourniquet, morphine)", 100, CRF_EHoldoutShopKind.ITEMS);
		AddShopItems(entry15, dressing, 4);
		AddShopItems(entry15, tourniquet, 1);
		AddShopItems(entry15, morphine, 1);

		// The defence
		AddShopEntry("Upgrade: reinforced boards on every barricade", 1500, CRF_EHoldoutShopKind.REINFORCED_BOARDS);
		AddShopEntry("Upgrade: a fourth board on every barricade", 2000, CRF_EHoldoutShopKind.EXTRA_BOARD);
		AddShopEntry("Upgrade: double the medical supplies every lull", 1200, CRF_EHoldoutShopKind.DOUBLE_RESUPPLY);
		AddShopEntry("Recharge the radio (call support again)", 2500, CRF_EHoldoutShopKind.RADIO_RECHARGE);
	}

	//===================================================================================
	// END
	//===================================================================================

	protected void Overrun()
	{
		if (m_iPhase == CRF_EHoldoutPhase.ENDED)
			return;

		int held = Math.Max(0, m_iWave - 1);
		if (m_iPhase == CRF_EHoldoutPhase.LULL)
			held = m_iWave;

		m_iPhase = CRF_EHoldoutPhase.ENDED;
		Replication.BumpMe();
		GetGame().GetCallqueue().Remove(Tick);
		m_mZombieDamage.Clear();

		CRF_RadioTransmissionEvents.PopupToFaction("", "OVERRUN", string.Format("Nobody is left. You held out for %1 wave(s).", held), 15);
		Print(string.Format("%1 Overrun after %2 wave(s).", LOG, held), LogLevel.NORMAL);

		if (m_bEndMissionWhenOverrun && m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//===================================================================================
	// CLIENT: map markers
	//===================================================================================

	protected void UpdateMarkers()
	{
		// Also the client's money check: a missed OnMoneyChanged still shows the change (it only acts on one)
		if (!Replication.IsServer())
			OnMoneyChanged();

		m_Markers.Begin();

		if (m_iPhase != CRF_EHoldoutPhase.ENDED)
		{
			foreach (CRF_HoldoutBarricadeComponent barricade : CRF_HoldoutBarricadeComponent.GetAll())
			{
				int boards = barricade.GetBoards();
				int color = ARGB(255, 60, 200, 80);
				if (boards == 0)
					color = ARGB(255, 220, 40, 40);
				else if (!barricade.IsFull())
					color = ARGB(255, 240, 170, 30);

				string kind = "Door";
				if (barricade.IsWindow())
					kind = "Window";

				m_Markers.Want(barricade.GetOwner().GetOrigin(), string.Format("%1 %2/%3", kind, boards, barricade.GetMaxBoards()), MARKER_ICON, color);
			}
		}

		m_Markers.Apply();
	}
}
