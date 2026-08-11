//------------------------------------------------------------------------------------
// CRF_CacheHuntGamemodeManager: Cache Hunt gamemode implementation for the Coalition
// Reforger Framework.
//
// Attackers must find and destroy up to five hidden ammunition caches. Defenders know
// exactly where the caches are, can rearm from them, and can fast-travel between their
// main spawn flag and each cache via CRF flag poles.
//
// Features:
// - Up to 5 caches, either placed by name or randomised around a centre entity
// - Attackers see large world-scaled search circles that are randomly offset from the
//   real cache position, so the cache is inside the circle but never at its centre
// - Defenders see exact cache markers on their map
// - Auto-spawned teleport flag poles, one per cache, linked to the defender home flag
// - Teleport actions disable while enemies are near either end of the trip
// - Caches restock ammunition pulled from the defending faction's assigned gearscript
// - Destroying a cache triggers a respawn wave for both sides
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
//! Which weapon classes in the defending gearscript contribute their magazines to the
//! caches. A launcher's magazine array holds its rockets or missiles - an Igla round is
//! a magazine like any other - so pulling every array in the gearscript hands out
//! unlimited AT and AA. Hence the split, with the heavy classes off by default.
enum CRF_ECacheHuntAmmoClass
{
	SMALL_ARMS			= 1 << 0,	//!< Rifles, carbines, pistols, sniper rifles
	SUPPORT				= 1 << 1,	//!< AR, MMG, HMG
	GRENADE_LAUNCHER	= 1 << 2,	//!< Rifle-mounted UGL rounds
	ANTI_TANK			= 1 << 3,	//!< AT, MAT, HAT rockets
	ANTI_AIR			= 1 << 4,	//!< AA missiles
	CUSTOM_ROLES		= 1 << 5,	//!< Whatever the gearscript's custom roles carry
}

//------------------------------------------------------------------------------------
//! Per-cache server-side bookkeeping. One instance is created for every spawned cache
//! so each can own its damage-state callback binding.
class CRF_CacheHuntCacheData
{
	int m_iIndex;										// 0-based cache index
	IEntity m_Cache;									// The spawned cache entity
	IEntity m_Flag;										// Auto-spawned teleport flag pole
	vector m_vPosition;									// Cache world position
	vector m_vSearchOffset;								// Cache -> search area centre, at the starting radius
	bool m_bDestroyed;
	bool m_bDetonating;									// A demolition fuse is already running

	protected SCR_DamageManagerComponent m_DamageManager;

	//------------------------------------------------------------------------------------------------
	//! Hooks the cache's damage manager so destruction is reported back to the manager.
	//! \return True when a damage manager was found and bound
	bool BindDamageManager()
	{
		if (!m_Cache)
			return false;

		m_DamageManager = SCR_DamageManagerComponent.Cast(m_Cache.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
			return false;

		m_DamageManager.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void UnbindDamageManager()
	{
		if (m_DamageManager)
			m_DamageManager.GetOnDamageStateChanged().Remove(OnDamageStateChanged);

		m_DamageManager = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDamageStateChanged(EDamageState state)
	{
		if (state != EDamageState.DESTROYED)
			return;

		CRF_CacheHuntGamemodeManager manager = CRF_CacheHuntGamemodeManager.GetInstance();
		if (manager)
			manager.OnCacheDestroyed(m_iIndex);
	}
}

//------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Cache Hunt: attackers hunt and destroy up to five hidden caches that defenders rearm and fast-travel from")]
class CRF_CacheHuntGamemodeManagerClass: SCR_BaseGameModeComponentClass {}

class CRF_CacheHuntGamemodeManager: SCR_BaseGameModeComponent
{
	//===================================================================================
	// ATTRIBUTES
	//===================================================================================

	// Faction Settings
	//------------------------------------------------------------------------------------
	[Attribute("BLUFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")}, desc: "The side hunting and destroying the caches", category: "Cache Hunt - Factions")]
	FactionKey m_AttackingSide;

	[Attribute("OPFOR", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")}, desc: "The side defending the caches. This side sees exact cache markers, rearms from caches and can use the teleport flags", category: "Cache Hunt - Factions")]
	FactionKey m_DefendingSide;

	// Cache Settings
	//------------------------------------------------------------------------------------
	[Attribute("{66F430BAD8FD6BB7}Prefabs/Props/Military/Arsenal/AmmoBoxes/CRF_CacheHunt_Cache.et", UIWidgets.ResourceNamePicker, desc: "Prefab spawned as a cache. Must carry a damage manager (SCR_DestructionMultiPhaseComponent or SCR_DamageManagerComponent), an RplComponent, and an SCR_ArsenalComponent when rearm is enabled", params: "et", category: "Cache Hunt - Caches")]
	ResourceName m_rCachePrefab;

	[Attribute("", UIWidgets.Auto, desc: "Names of entities marking where caches spawn. Used when 'Randomize Caches' is off. Maximum of 5 entries, extras are ignored", category: "Cache Hunt - Caches")]
	ref array<string> m_aCacheSpawnPointNames;

	[Attribute("0", UIWidgets.CheckBox, desc: "Scatter caches randomly around a centre entity instead of using named spawn points", category: "Cache Hunt - Caches")]
	bool m_bRandomizeCaches;

	[Attribute("CacheHunt_Center", UIWidgets.EditBox, desc: "Name of the entity caches are randomised around. Only used when 'Randomize Caches' is on", category: "Cache Hunt - Caches")]
	string m_sRandomCenterEntityName;

	[Attribute("3", UIWidgets.Slider, desc: "How many caches to scatter when randomising", params: "1 5 1", category: "Cache Hunt - Caches")]
	int m_iRandomCacheCount;

	[Attribute("100", UIWidgets.Slider, desc: "Minimum distance from the centre entity a randomised cache can spawn (metres)", params: "0 2000 10", category: "Cache Hunt - Caches")]
	float m_fRandomMinRadius;

	[Attribute("600", UIWidgets.Slider, desc: "Maximum distance from the centre entity a randomised cache can spawn (metres)", params: "10 4000 10", category: "Cache Hunt - Caches")]
	float m_fRandomMaxRadius;

	[Attribute("150", UIWidgets.Slider, desc: "Minimum distance between two randomised caches (metres)", params: "0 2000 10", category: "Cache Hunt - Caches")]
	float m_fRandomMinSeparation;

	[Attribute("6", UIWidgets.Slider, desc: "Fuse between the Destroy Cache action finishing and the cache going up, giving the attacker time to get clear (seconds). Does not affect caches killed with explosives", params: "0 60 1", category: "Cache Hunt - Caches")]
	float m_fDestroyFuseSeconds;

	// Attacker Search Markers
	//------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, desc: "Give attackers a large search area marker per cache", category: "Cache Hunt - Attacker Markers")]
	bool m_bEnableSearchMarkers;

	[Attribute("{B1A844B7FEF4BC41}PrefabsMissionMaking/Markers/ShapeMarkers/ShapeMarker_Base.et", UIWidgets.ResourceNamePicker, desc: "Marker entity spawned as the attacker search area. Must be a COA_ShapeMarker", params: "et", category: "Cache Hunt - Attacker Markers")]
	ResourceName m_rSearchMarkerPrefab;

	[Attribute("200", UIWidgets.Slider, desc: "Radius of the attacker search area in metres. The cache is always somewhere inside this shape", params: "25 1500 25", category: "Cache Hunt - Attacker Markers")]
	float m_fSearchAreaRadius;

	[Attribute("0.75", UIWidgets.Slider, desc: "How far the shape's centre is offset from the real cache, as a fraction of the search radius. 0 centres the shape exactly on the cache", params: "0 0.95 0.05", category: "Cache Hunt - Attacker Markers")]
	float m_fSearchOffsetFactor;

	[Attribute("0", UIWidgets.ComboBox, desc: "Shape drawn as the search area", enums: ParamEnumArray.FromEnum(COA_EMapShapeMarkerType), category: "Cache Hunt - Attacker Markers")]
	COA_EMapShapeMarkerType m_eSearchMarkerShape;

	[Attribute("2.5", UIWidgets.Slider, desc: "Border thickness of the search area outline, in pixels", params: "0.25 12 0.25", category: "Cache Hunt - Attacker Markers")]
	float m_fSearchMarkerBorderWidth;

	[Attribute("1 0.25 0.25 1", UIWidgets.ColorPicker, desc: "Colour of the attacker search area outline", category: "Cache Hunt - Attacker Markers")]
	ref Color m_SearchMarkerColor;

	[Attribute("1", UIWidgets.CheckBox, desc: "Narrow the search areas in steps as the mission runs on, so a stalled hunt still ends. Needs a mission time limit set in the COA gamemode", category: "Cache Hunt - Search Narrowing")]
	bool m_bEnableSearchNarrowing;

	[Attribute("4", UIWidgets.Slider, desc: "How many steps the search areas narrow in. Each step is announced to the attackers", params: "2 10 1", category: "Cache Hunt - Search Narrowing")]
	int m_iSearchNarrowSteps;

	[Attribute("0.25", UIWidgets.Slider, desc: "Final search radius as a fraction of the starting radius. 0.25 turns a 200m circle into a 50m one, which is still a real building-to-building search", params: "0.05 1 0.05", category: "Cache Hunt - Search Narrowing")]
	float m_fSearchMinRadiusFactor;

	[Attribute("0.75", UIWidgets.Slider, desc: "Fraction of the mission by which the search areas have fully narrowed. 0.75 leaves the last quarter of the mission at the tightest search area", params: "0.2 1 0.05", category: "Cache Hunt - Search Narrowing")]
	float m_fSearchNarrowCompleteAt;

	// Defender Markers
	//------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, desc: "Show defenders an exact map marker on every surviving cache", category: "Cache Hunt - Defender Markers")]
	bool m_bEnableDefenderMarkers;

	[Attribute("{233A8BC0520B1B8B}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Grenades.edds", UIWidgets.ResourcePickerThumbnail, desc: "Icon used for the defender cache markers", params: "edds", category: "Cache Hunt - Defender Markers")]
	ResourceName m_rDefenderMarkerIcon;

	// Teleport Flags
	//------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, desc: "Spawn a teleport flag pole at every cache and link it to the defender home flag", category: "Cache Hunt - Teleport")]
	bool m_bEnableTeleportFlags;

	[Attribute("{5CA9E48B71D3F2A0}Prefabs/Structures/CacheHunt/CRF_CacheHunt_FlagPole.et", UIWidgets.ResourceNamePicker, desc: "Flag pole prefab auto-spawned at each cache. Must carry CRF_CacheHunt_FlagComponent", params: "et", category: "Cache Hunt - Teleport")]
	ResourceName m_rFlagPolePrefab;

	[Attribute("CacheHunt_HomeFlag", UIWidgets.EditBox, desc: "Name of the flag pole placed at the defender main spawn. Place the same CacheHunt flag pole prefab there and give it this name", category: "Cache Hunt - Teleport")]
	string m_sDefenderHomeFlagName;

	[Attribute("5", UIWidgets.Slider, desc: "Distance from the cache the teleport flag is spawned (metres)", params: "2 30 1", category: "Cache Hunt - Teleport")]
	float m_fFlagSpawnDistance;

	[Attribute("150", UIWidgets.Slider, desc: "Enemies within this radius of either flag disable the teleport (metres)", params: "10 1000 10", category: "Cache Hunt - Teleport")]
	float m_fEnemyProximityRadius;

	[Attribute("2", UIWidgets.Slider, desc: "How often the enemy proximity check runs (seconds)", params: "1 15 1", category: "Cache Hunt - Teleport")]
	float m_fFlagCheckInterval;

	// Rearm
	//------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, desc: "Fill each cache's arsenal with ammunition pulled from the defending faction's assigned gearscript", category: "Cache Hunt - Rearm")]
	bool m_bEnableCacheRearm;

	[Attribute("0", UIWidgets.CheckBox, desc: "Charge supplies for ammunition taken from a cache. The per-item price comes from the faction's entity catalog, not from this gamemode - see the guide", category: "Cache Hunt - Rearm")]
	bool m_bAmmoCostsSupplies;

	[Attribute("39", UIWidgets.Flags, desc: "Which weapon classes from the defending gearscript stock the caches. Anti-tank and anti-air are off by default: a launcher's magazines are its rockets, so including them gives every defender unlimited AT and AA rounds", enums: ParamEnumArray.FromEnum(CRF_ECacheHuntAmmoClass), category: "Cache Hunt - Rearm")]
	CRF_ECacheHuntAmmoClass m_eAmmoClasses;

	// Misc
	//------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, desc: "Trigger a respawn wave for both sides whenever a cache is destroyed", category: "Cache Hunt - Misc")]
	bool m_bRespawnOnCacheDestroyed;

	[Attribute("2000", UIWidgets.EditBox, desc: "Delay before caches are spawned, giving the world time to finish loading (milliseconds)", category: "Cache Hunt - Misc")]
	int m_iInitDelayMs;

	//===================================================================================
	// RUNTIME STATE
	//===================================================================================

	//! Positions of every cache that is still standing. Replicated so clients can draw
	//! defender markers without needing entity references.
	[RplProp(onRplName: "OnActiveCachesReplicated")]
	protected ref array<vector> m_aActiveCachePositions = {};

	//! Original 0-based index of each surviving cache, in step with the positions above.
	//! Replicated separately because the position list compacts as caches die - without
	//! this, destroying cache A would relabel cache B as A on every defender's map.
	[RplProp(onRplName: "OnActiveCachesReplicated")]
	protected ref array<int> m_aActiveCacheIndices = {};

	//! Centres of the attacker search areas, one per surviving cache. Decided on the server
	//! so every attacker sees the same shape in the same place, then spawned locally by each
	//! client - shape markers carry no RplComponent and cannot be replicated entities.
	[RplProp(onRplName: "OnActiveCachesReplicated")]
	protected ref array<vector> m_aSearchMarkerPositions = {};

	//! Current search radius as a fraction of the starting radius. Drops in steps as the
	//! mission runs on. Replicated because each client sizes its own shape markers.
	[RplProp(onRplName: "OnActiveCachesReplicated")]
	protected float m_fSearchRadiusFactor = 1.0;

	//! How many narrowing steps have already been applied
	protected int m_iSearchNarrowStepsDone = 0;

	[RplProp()]
	protected int m_iCachesDestroyed = 0;

	[RplProp()]
	protected int m_iCacheTotal = 0;

	//! Server-only cache bookkeeping
	protected ref array<ref CRF_CacheHuntCacheData> m_aCaches = {};

	//! Cached magazine list pulled from the defending faction's gearscript
	protected ref array<ResourceName> m_aDefenderAmmunition;

	//! Client-only: scripted markers currently on the local player's map. Position and label
	//! are tracked in step because RemoveScriptedMarker matches on every field it was added
	//! with, so a marker has to be removed using the exact label it went in with.
	protected ref array<string> m_aLocalDefenderMarkerPositions = {};
	protected ref array<string> m_aLocalDefenderMarkerLabels = {};

	//! Client-only: locally spawned attacker search shapes, and the centres they sit on
	protected ref array<IEntity> m_aLocalSearchMarkers = {};
	protected ref array<vector> m_aLocalSearchMarkerPositions = {};

	//! Radius factor the local markers were built at. Tracked separately from position
	//! because with Search Offset Factor at 0 the centres never move, so a narrowing step
	//! would change the size without changing a single position.
	protected float m_fLocalSearchMarkerFactor = 1.0;

	protected bool m_bMarkerPollRunning = false;

	protected bool m_bInitialised = false;
	protected bool m_bAllCachesDestroyed = false;

	protected COA_RplBroadcastManager m_RplBroadcastManager;
	protected COA_RespawnManager m_RespawnManager;

	protected static CRF_CacheHuntGamemodeManager m_sInstance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	void CRF_CacheHuntGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_CacheHuntGamemodeManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_CacheHuntGamemodeManager GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!GetGame().InPlayMode())
			return;

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(InitialiseCaches, m_iInitDelayMs, false);

		// Markers are drawn by any machine with a local player, which includes a listen
		// host and Workbench play. Replication.IsClient() is false on the host, so gating
		// on it would leave the host with no markers at all.
		if (RplSession.Mode() != RplMode.Dedicated)
			StartLocalMarkerPoll();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(InitialiseCaches);
			GetGame().GetCallqueue().Remove(UpdateFlagProximity);
			GetGame().GetCallqueue().Remove(ApplyCacheArsenalDeferred);
			GetGame().GetCallqueue().Remove(DetonateCache);
			GetGame().GetCallqueue().Remove(UpdateSearchNarrowing);
			GetGame().GetCallqueue().Remove(AssignFlagCacheIndex);
			GetGame().GetCallqueue().Remove(UpdateLocalMarkers);
			GetGame().GetCallqueue().Remove(EnsureMarkerWidget);
		}

		ClearAllDefenderMarkers();
		ClearAllSearchMarkers();

		foreach (CRF_CacheHuntCacheData cache : m_aCaches)
		{
			if (cache)
				cache.UnbindDamageManager();
		}
		m_aCaches.Clear();

		if (m_sInstance == this)
			m_sInstance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected COA_RplBroadcastManager GetBroadcastManager()
	{
		if (!m_RplBroadcastManager)
			m_RplBroadcastManager = COA_RplBroadcastManager.GetInstance();

		return m_RplBroadcastManager;
	}

	//------------------------------------------------------------------------------------------------
	protected COA_RespawnManager GetRespawnManager()
	{
		if (!m_RespawnManager)
			m_RespawnManager = COA_RespawnManager.GetInstance();

		return m_RespawnManager;
	}

	//===================================================================================
	// CACHE PLACEMENT (SERVER)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Resolves cache positions, spawns the caches, their markers and their teleport flags.
	protected void InitialiseCaches()
	{
		if (m_bInitialised)
			return;

		m_bInitialised = true;

		array<vector> positions = {};

		if (m_bRandomizeCaches)
			BuildRandomisedPositions(positions);
		else
			BuildNamedPositions(positions);

		if (positions.IsEmpty())
		{
			Print("[CRF_CacheHunt] No cache positions could be resolved. Check the spawn point names or the randomisation centre entity.", LogLevel.ERROR);
			return;
		}

		foreach (int index, vector position : positions)
		{
			SpawnCache(index, position);
		}

		m_iCacheTotal = m_aCaches.Count();
		RebuildActiveCachePositions();

		Print(string.Format("[CRF_CacheHunt] Spawned %1 cache(s). Attackers: %2, Defenders: %3.", m_iCacheTotal, m_AttackingSide, m_DefendingSide), LogLevel.NORMAL);

		if (m_bEnableTeleportFlags)
		{
			ResolveHomeFlag();
			GetGame().GetCallqueue().CallLater(UpdateFlagProximity, (int)(m_fFlagCheckInterval * 1000), true);
		}

		if (m_bEnableSearchNarrowing && m_bEnableSearchMarkers)
			GetGame().GetCallqueue().CallLater(UpdateSearchNarrowing, NARROW_CHECK_INTERVAL_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Collects positions from the mission maker's named spawn point entities.
	protected void BuildNamedPositions(notnull array<vector> positions)
	{
		if (!m_aCacheSpawnPointNames)
			return;

		foreach (string entityName : m_aCacheSpawnPointNames)
		{
			if (positions.Count() >= MAX_CACHES)
				break;

			if (entityName.IsEmpty())
				continue;

			IEntity spawnPoint = GetGame().GetWorld().FindEntityByName(entityName);
			if (!spawnPoint)
			{
				Print(string.Format("[CRF_CacheHunt] Cache spawn point '%1' was not found in the world.", entityName), LogLevel.WARNING);
				continue;
			}

			positions.Insert(spawnPoint.GetOrigin());
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Scatters cache positions around the configured centre entity, honouring the
	//! minimum separation where possible.
	protected void BuildRandomisedPositions(notnull array<vector> positions)
	{
		IEntity centre = GetGame().GetWorld().FindEntityByName(m_sRandomCenterEntityName);
		if (!centre)
		{
			Print(string.Format("[CRF_CacheHunt] Randomisation centre entity '%1' was not found in the world.", m_sRandomCenterEntityName), LogLevel.ERROR);
			return;
		}

		vector centreOrigin = centre.GetOrigin();
		int wanted = Math.Clamp(m_iRandomCacheCount, 1, MAX_CACHES);
		float minRadius = Math.Min(m_fRandomMinRadius, m_fRandomMaxRadius);
		float maxRadius = Math.Max(m_fRandomMinRadius, m_fRandomMaxRadius);

		for (int i = 0; i < wanted; i++)
		{
			vector candidate;
			bool placed = false;

			for (int attempt = 0; attempt < RANDOM_PLACEMENT_ATTEMPTS; attempt++)
			{
				float angle = Math.RandomFloat(0, Math.PI2);
				float distance = Math.RandomFloat(minRadius, maxRadius);

				candidate = centreOrigin;
				candidate[0] = centreOrigin[0] + Math.Cos(angle) * distance;
				candidate[2] = centreOrigin[2] + Math.Sin(angle) * distance;
				candidate[1] = SCR_TerrainHelper.GetTerrainY(candidate);

				if (!IsFarEnoughFromOthers(candidate, positions))
					continue;

				vector settled;
				if (SCR_WorldTools.FindEmptyTerrainPosition(settled, candidate, CACHE_CLEARANCE_RADIUS))
					candidate = settled;

				placed = true;
				break;
			}

			if (!placed)
			{
				Print(string.Format("[CRF_CacheHunt] Could not find a spot for randomised cache %1 that respects the minimum separation. Placing it anyway.", i + 1), LogLevel.WARNING);
			}

			positions.Insert(candidate);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsFarEnoughFromOthers(vector candidate, notnull array<vector> existing)
	{
		if (m_fRandomMinSeparation <= 0)
			return true;

		float minSeparationSq = m_fRandomMinSeparation * m_fRandomMinSeparation;
		foreach (vector other : existing)
		{
			if (vector.DistanceSq(candidate, other) < minSeparationSq)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns one cache plus its attacker search marker and teleport flag.
	protected void SpawnCache(int index, vector position)
	{
		Resource cacheResource = Resource.Load(m_rCachePrefab);
		if (!cacheResource || !cacheResource.IsValid())
		{
			Print(string.Format("[CRF_CacheHunt] Cache prefab '%1' could not be loaded.", m_rCachePrefab), LogLevel.ERROR);
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = position;
		SCR_TerrainHelper.OrientToTerrain(spawnParams.Transform);

		IEntity cacheEntity = GetGame().SpawnEntityPrefab(cacheResource, GetGame().GetWorld(), spawnParams);
		if (!cacheEntity)
		{
			Print(string.Format("[CRF_CacheHunt] Failed to spawn cache %1 at %2.", index + 1, position), LogLevel.ERROR);
			return;
		}

		CRF_CacheHuntCacheData data = new CRF_CacheHuntCacheData();
		data.m_iIndex = index;
		data.m_Cache = cacheEntity;
		data.m_vPosition = cacheEntity.GetOrigin();

		if (!data.BindDamageManager())
		{
			Print(string.Format("[CRF_CacheHunt] Cache prefab '%1' has no SCR_DamageManagerComponent. It can never be destroyed, so the objective will not complete.", m_rCachePrefab), LogLevel.ERROR);
		}

		data.m_vSearchOffset = ComputeSearchOffset();

		m_aCaches.Insert(data);

		if (m_bEnableTeleportFlags)
			data.m_Flag = SpawnCacheFlag(index, data.m_vPosition);

		// Give the freshly spawned cache a moment to finish initialising its arsenal.
		if (m_bEnableCacheRearm)
			GetGame().GetCallqueue().CallLater(ApplyCacheArsenalDeferred, ARSENAL_SETUP_DELAY_MS, false, index);
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyCacheArsenalDeferred(int index)
	{
		CRF_CacheHuntCacheData data = GetCacheData(index);
		if (data && !data.m_bDestroyed)
			ApplyCacheArsenal(data);
	}

	//------------------------------------------------------------------------------------------------
	//! Picks where a cache's search area is centred. The centre is pushed a random distance
	//! away from the cache so the cache sits inside the shape but never at the middle.
	//! Decided once on the server so every attacker searches the same ground.
	protected vector ComputeSearchOffset()
	{
		float maxOffset = m_fSearchAreaRadius * Math.Clamp(m_fSearchOffsetFactor, 0, 0.95);
		float angle = Math.RandomFloat(0, Math.PI2);
		float offset = Math.RandomFloat(0, maxOffset);

		return Vector(Math.Cos(angle) * offset, 0, Math.Sin(angle) * offset);
	}

	//------------------------------------------------------------------------------------------------
	//! Where a cache's search area sits at the current radius.
	//!
	//! The offset scales with the radius rather than staying put. That is not cosmetic: the
	//! centre is deliberately offset by up to 95% of the radius, so a circle that shrank
	//! around a fixed centre would eventually stop containing its own cache and send the
	//! attackers to search ground the cache provably is not on. Scaling both together keeps
	//! the cache in the same relative spot inside the shape, so it stays inside at every size.
	protected vector ComputeSearchCenter(notnull CRF_CacheHuntCacheData data)
	{
		vector center = data.m_vPosition + (data.m_vSearchOffset * m_fSearchRadiusFactor);
		center[1] = SCR_TerrainHelper.GetTerrainY(center);

		return center;
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns the teleport flag pole that serves a given cache.
	protected IEntity SpawnCacheFlag(int index, vector cachePosition)
	{
		Resource flagResource = Resource.Load(m_rFlagPolePrefab);
		if (!flagResource || !flagResource.IsValid())
		{
			Print(string.Format("[CRF_CacheHunt] Flag pole prefab '%1' could not be loaded.", m_rFlagPolePrefab), LogLevel.ERROR);
			return null;
		}

		// Offset the flag from the cache so it does not clip through it.
		float angle = Math.RandomFloat(0, Math.PI2);
		vector flagPosition = cachePosition;
		flagPosition[0] = cachePosition[0] + Math.Cos(angle) * m_fFlagSpawnDistance;
		flagPosition[2] = cachePosition[2] + Math.Sin(angle) * m_fFlagSpawnDistance;

		vector settled;
		if (SCR_WorldTools.FindEmptyTerrainPosition(settled, flagPosition, FLAG_CLEARANCE_RADIUS))
			flagPosition = settled;
		else
			flagPosition[1] = SCR_TerrainHelper.GetTerrainY(flagPosition);

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = flagPosition;
		SCR_TerrainHelper.OrientToTerrain(spawnParams.Transform);

		IEntity flagEntity = GetGame().SpawnEntityPrefab(flagResource, GetGame().GetWorld(), spawnParams);
		if (!flagEntity)
		{
			Print(string.Format("[CRF_CacheHunt] Failed to spawn the teleport flag for cache %1.", index + 1), LogLevel.ERROR);
			return null;
		}

		// The cache index is an RplProp, so wait for the flag's RplId before assigning it.
		AssignFlagCacheIndex(flagEntity, index, 0);

		return flagEntity;
	}

	//------------------------------------------------------------------------------------------------
	//! Tags a spawned flag with the cache it serves, retrying until its RplComponent is ready.
	protected void AssignFlagCacheIndex(IEntity flagEntity, int index, int attempt)
	{
		if (!flagEntity)
			return;

		RplComponent rplComponent = RplComponent.Cast(flagEntity.FindComponent(RplComponent));
		if (rplComponent && rplComponent.Id() == RplId.Invalid())
		{
			if (attempt + 1 >= MAX_RPL_WAIT_ATTEMPTS)
			{
				Print(string.Format("[CRF_CacheHunt] Teleport flag for cache %1 never got a valid RplId; assigning its index anyway.", index + 1), LogLevel.WARNING);
			}
			else
			{
				GetGame().GetCallqueue().CallLater(AssignFlagCacheIndex, RPL_WAIT_INTERVAL_MS, false, flagEntity, index, attempt + 1);
				return;
			}
		}

		CRF_CacheHunt_FlagComponent flagComponent = CRF_CacheHunt_FlagComponent.Cast(flagEntity.FindComponent(CRF_CacheHunt_FlagComponent));
		if (!flagComponent)
		{
			Print(string.Format("[CRF_CacheHunt] Flag pole prefab '%1' is missing CRF_CacheHunt_FlagComponent, so it cannot be used for teleporting.", m_rFlagPolePrefab), LogLevel.ERROR);
			return;
		}

		flagComponent.SetCacheIndex(index);
	}

	//------------------------------------------------------------------------------------------------
	//! Finds the mission maker's home flag and tags it as the defender spawn end of the link.
	protected void ResolveHomeFlag()
	{
		if (m_sDefenderHomeFlagName.IsEmpty())
		{
			Print("[CRF_CacheHunt] No defender home flag name configured. Teleporting will be one-way from the caches only.", LogLevel.WARNING);
			return;
		}

		IEntity homeFlag = GetGame().GetWorld().FindEntityByName(m_sDefenderHomeFlagName);
		if (!homeFlag)
		{
			Print(string.Format("[CRF_CacheHunt] Defender home flag '%1' was not found in the world.", m_sDefenderHomeFlagName), LogLevel.WARNING);
			return;
		}

		CRF_CacheHunt_FlagComponent flagComponent = CRF_CacheHunt_FlagComponent.Cast(homeFlag.FindComponent(CRF_CacheHunt_FlagComponent));
		if (!flagComponent)
		{
			Print(string.Format("[CRF_CacheHunt] Defender home flag '%1' is missing CRF_CacheHunt_FlagComponent.", m_sDefenderHomeFlagName), LogLevel.ERROR);
			return;
		}

		flagComponent.SetCacheIndex(HOME_FLAG_INDEX);
	}

	//===================================================================================
	// CACHE DESTRUCTION (SERVER)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called by CRF_CacheHuntCacheData when a cache reaches EDamageState.DESTROYED.
	//! \param[in] index 0-based index of the destroyed cache
	void OnCacheDestroyed(int index)
	{
		if (!Replication.IsServer())
			return;

		CRF_CacheHuntCacheData data = GetCacheData(index);
		if (!data || data.m_bDestroyed)
			return;

		data.m_bDestroyed = true;
		data.UnbindDamageManager();
		m_iCachesDestroyed++;

		// The flag this cache fed goes with it. The attackers' search area disappears too,
		// but that happens client-side off the replicated position list below.
		if (data.m_Flag)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(data.m_Flag);
			data.m_Flag = null;
		}

		RebuildActiveCachePositions();

		int remaining = m_iCacheTotal - m_iCachesDestroyed;
		COA_RplBroadcastManager broadcastManager = GetBroadcastManager();

		if (remaining > 0)
		{
			if (broadcastManager)
			{
				broadcastManager.PopUpNotification(15,
					string.Format("Cache %1 destroyed!", GetCacheLabel(index)),
					string.Format("%1 of %2 caches destroyed - %3 remaining", m_iCachesDestroyed, m_iCacheTotal, remaining));
			}
		}
		else if (!m_bAllCachesDestroyed)
		{
			m_bAllCachesDestroyed = true;

			if (broadcastManager)
			{
				broadcastManager.PopUpNotification(20,
					"All caches destroyed!",
					string.Format("%1 wins the cache hunt", m_AttackingSide));
			}
		}

		Print(string.Format("[CRF_CacheHunt] Cache %1 destroyed. %2/%3 down.", GetCacheLabel(index), m_iCachesDestroyed, m_iCacheTotal), LogLevel.NORMAL);

		if (m_bRespawnOnCacheDestroyed)
			RespawnBothSides();

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side handler for an attacker's Destroy Cache action, routed in from
	//! COA_PlayerRplToAuthorityManager. Everything the client sent is re-checked here.
	//! \param[in] cache Cache entity the client asked to destroy
	//! \param[in] playerId Player making the request
	void RequestDestroyCache(IEntity cache, int playerId)
	{
		if (!Replication.IsServer() || !cache)
			return;

		CRF_CacheHuntCacheData data = GetCacheDataForEntity(cache);
		if (!data || data.m_bDestroyed)
			return;

		// Re-check the faction server-side; a client could ask for anything
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;

		if (GetEntityFactionKey(playerEntity) != m_AttackingSide)
		{
			Print(string.Format("[CRF_CacheHunt] Rejected a Destroy Cache request from player %1 - not on the attacking side.", playerId), LogLevel.WARNING);
			return;
		}

		// And re-check they are actually stood at the cache, so the RplId cannot be
		// replayed from across the map
		if (vector.DistanceSq(playerEntity.GetOrigin(), cache.GetOrigin()) > DESTROY_RANGE * DESTROY_RANGE)
		{
			Print(string.Format("[CRF_CacheHunt] Rejected a Destroy Cache request from player %1 - too far from the cache.", playerId), LogLevel.WARNING);
			return;
		}

		StartCacheDetonation(data);
	}

	//------------------------------------------------------------------------------------------------
	//! Lights the fuse on a cache. The cache stays standing and still counts as alive until
	//! the fuse runs out, which gives whoever set the charge time to get clear of the blast.
	protected void StartCacheDetonation(notnull CRF_CacheHuntCacheData data)
	{
		if (data.m_bDestroyed || data.m_bDetonating)
			return;

		data.m_bDetonating = true;

		if (m_fDestroyFuseSeconds <= 0)
		{
			DetonateCache(data.m_iIndex);
			return;
		}

		GetGame().GetCallqueue().CallLater(DetonateCache, (int)(m_fDestroyFuseSeconds * 1000), false, data.m_iIndex);
	}

	//------------------------------------------------------------------------------------------------
	//! Blows a cache up and books it in. Safe to call alongside the damage-manager path -
	//! OnCacheDestroyed ignores a cache that is already down.
	protected void DetonateCache(int index)
	{
		CRF_CacheHuntCacheData data = GetCacheData(index);
		if (!data || data.m_bDestroyed)
			return;

		IEntity cache = data.m_Cache;

		if (cache)
		{
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			spawnParams.Transform[3] = cache.GetOrigin();

			GetGame().SpawnEntityPrefab(Resource.Load(DESTRUCTION_EFFECT), GetGame().GetWorld(), spawnParams);
		}

		// Book the kill before deleting, so the bookkeeping still has the entity to work with
		OnCacheDestroyed(index);

		if (cache)
		{
			data.m_Cache = null;
			SCR_EntityHelper.DeleteEntityAndChildren(cache);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_CacheHuntCacheData GetCacheDataForEntity(IEntity cache)
	{
		foreach (CRF_CacheHuntCacheData data : m_aCaches)
		{
			if (data && data.m_Cache == cache)
				return data;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Sends both sides back to the spawn flag they came from by triggering a respawn wave.
	protected void RespawnBothSides()
	{
		COA_RespawnManager respawnManager = GetRespawnManager();
		if (!respawnManager)
			return;

		respawnManager.RespawnSide(m_AttackingSide);
		respawnManager.RespawnSide(m_DefendingSide);
	}

	//===================================================================================
	// SEARCH NARROWING (SERVER)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Steps the search areas down as the mission runs on, so a hunt that has stalled still
	//! resolves. Narrowing is deliberately stepped rather than continuous: a circle that
	//! creeps inward frame by frame just reads as players misremembering where it was, while
	//! a step with an announcement behind it reads as fresh intel.
	protected void UpdateSearchNarrowing()
	{
		if (!m_bEnableSearchNarrowing || m_bAllCachesDestroyed)
			return;

		// Nothing narrows during safestart. The mission clock only starts when safestart
		// ends, so this is belt and braces on top of GetMissionProgress() returning -1
		// while m_iTimeMissionEnds is still unset - but it makes the intent explicit and
		// covers a safestart that gets re-entered.
		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		if (safestartManager && safestartManager.GetSafestartStatus())
			return;

		float progress = GetMissionProgress();
		if (progress < 0)
			return;	// No time limit set, so there is no mission clock to narrow against

		int steps = Math.ClampInt(m_iSearchNarrowSteps, 2, 10);
		float completeAt = Math.Clamp(m_fSearchNarrowCompleteAt, 0.2, 1.0);

		// How many steps should have happened by now
		int wantedSteps = Math.ClampInt((int)Math.Floor((progress / completeAt) * steps), 0, steps);

		if (wantedSteps <= m_iSearchNarrowStepsDone)
			return;

		m_iSearchNarrowStepsDone = wantedSteps;

		float minFactor = Math.Clamp(m_fSearchMinRadiusFactor, 0.05, 1.0);
		m_fSearchRadiusFactor = Math.Lerp(1.0, minFactor, (float)wantedSteps / steps);

		// Recentre and resize every surviving search area, then tell the attackers why
		RebuildActiveCachePositions();
		AnnounceSearchNarrowing(wantedSteps, steps);

		Print(string.Format("[CRF_CacheHunt] Search areas narrowed to step %1/%2 (%3%% of starting radius) at %4%% mission progress.",
			wantedSteps, steps, (int)(m_fSearchRadiusFactor * 100), (int)(progress * 100)), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Announced to both sides on purpose. The attackers need to know their circles moved,
	//! and the defenders knowing the net is closing is the tension that makes a stalled
	//! round interesting rather than just slow. Neither side learns anything secret - the
	//! defenders already know exactly where their caches are.
	protected void AnnounceSearchNarrowing(int step, int totalSteps)
	{
		COA_RplBroadcastManager broadcastManager = GetBroadcastManager();
		if (!broadcastManager)
			return;

		string body;
		if (step >= totalSteps)
			body = string.Format("Search areas are now at their tightest - %1%% of their original size", (int)(m_fSearchRadiusFactor * 100));
		else
			body = string.Format("Search areas narrowed - step %1 of %2", step, totalSteps);

		broadcastManager.PopUpNotification(12, "Intel update", body);
	}

	//------------------------------------------------------------------------------------------------
	//! \return How far through the mission we are, 0..1, or -1 when there is no time limit
	protected float GetMissionProgress()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GameTimerManager timerManager = COA_GameTimerManager.GetInstance();
		if (!gamemode || !timerManager)
			return -1;

		if (gamemode.m_iTimeLimitMinutes <= 0 || timerManager.m_iTimeMissionEnds <= 0)
			return -1;	// Untimed mission, or safestart has not started the clock yet

		int durationMs = gamemode.m_iTimeLimitMinutes * 60000;
		int startTime = timerManager.m_iTimeMissionEnds - durationMs;
		int elapsed = GetGame().GetWorld().GetWorldTime() - startTime;

		return Math.Clamp((float)elapsed / durationMs, 0, 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Refreshes the replicated position lists the clients draw their markers from:
	//! exact cache positions for defenders, offset search centres for attackers.
	protected void RebuildActiveCachePositions()
	{
		m_aActiveCachePositions.Clear();
		m_aActiveCacheIndices.Clear();
		m_aSearchMarkerPositions.Clear();

		foreach (CRF_CacheHuntCacheData cache : m_aCaches)
		{
			if (!cache || cache.m_bDestroyed)
				continue;

			m_aActiveCachePositions.Insert(cache.m_vPosition);
			m_aActiveCacheIndices.Insert(cache.m_iIndex);
			m_aSearchMarkerPositions.Insert(ComputeSearchCenter(cache));
		}

		Replication.BumpMe();
	}

	//===================================================================================
	// TELEPORT FLAG PROXIMITY (SERVER)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Marks every registered flag as blocked or clear based on nearby attacking players.
	//! Runs on the server only; clients read the replicated result off the flag component.
	protected void UpdateFlagProximity()
	{
		array<CRF_CacheHunt_FlagComponent> flags = CRF_CacheHunt_FlagComponent.GetRegisteredFlags();
		if (flags.IsEmpty())
			return;

		array<vector> enemyPositions = {};
		CollectFactionPlayerPositions(m_AttackingSide, enemyPositions);

		float radiusSq = m_fEnemyProximityRadius * m_fEnemyProximityRadius;

		foreach (CRF_CacheHunt_FlagComponent flag : flags)
		{
			if (!flag || !flag.GetOwner())
				continue;

			vector flagOrigin = flag.GetOwner().GetOrigin();
			bool blocked = false;

			foreach (vector enemyPosition : enemyPositions)
			{
				if (vector.DistanceSq(flagOrigin, enemyPosition) <= radiusSq)
				{
					blocked = true;
					break;
				}
			}

			flag.SetEnemiesNear(blocked);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Gathers the world positions of every living player on the given faction.
	protected void CollectFactionPlayerPositions(FactionKey factionKey, notnull array<vector> positions)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		array<int> playerIds = {};
		playerManager.GetAllPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;

			// Skip the dead so corpses do not keep a flag locked down.
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(playerEntity.FindComponent(SCR_DamageManagerComponent));
			if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
				continue;

			if (GetEntityFactionKey(playerEntity) != factionKey)
				continue;

			positions.Insert(playerEntity.GetOrigin());
		}
	}

	//===================================================================================
	// REARM (SERVER)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Replaces a cache's arsenal item list with the defending faction's ammunition.
	//!
	//! An SCR_ArsenalComponent serves a virtual list, not the physical contents of the box's
	//! storage - spawning magazines into the storage does not make them appear in the arsenal
	//! UI. So the list itself is what has to be built.
	protected void ApplyCacheArsenal(notnull CRF_CacheHuntCacheData data)
	{
		if (!data.m_Cache)
		{
			Print(string.Format("[CRF_CacheHunt] Cache %1 has no entity by the time its arsenal was due to be filled.", data.m_iIndex + 1), LogLevel.WARNING);
			return;
		}

		array<ResourceName> ammunition = GetDefenderAmmunition();
		if (ammunition.IsEmpty())
		{
			Print(string.Format("[CRF_CacheHunt] No ammunition resolved from the '%1' gearscript, so cache %2's arsenal was left as authored.", m_DefendingSide, data.m_iIndex + 1), LogLevel.WARNING);
			return;
		}

		SCR_ArsenalComponent arsenal = SCR_ArsenalComponent.Cast(data.m_Cache.FindComponent(SCR_ArsenalComponent));
		if (!arsenal)
		{
			Print(string.Format("[CRF_CacheHunt] Cache prefab '%1' has no SCR_ArsenalComponent, so defenders have nothing to rearm from. Disable 'Enable Cache Rearm' or use a cache prefab with an arsenal.", m_rCachePrefab), LogLevel.ERROR);
			return;
		}

		// Reuse the list the prefab authored where possible, so anything the arsenal has
		// already cached off that object stays pointed at the same instance.
		SCR_ArsenalItemListConfig itemList = arsenal.GetOverwriteArsenalConfig();
		if (!itemList)
		{
			itemList = new SCR_ArsenalItemListConfig();
			arsenal.CRF_SetOverwriteArsenalConfig(itemList);
		}

		int before = itemList.CRF_GetItemCount();

		// EQUIPMENT and AMMUNITION are what the arsenal filters ammunition on. Both must be
		// present in the cache prefab's Supported Item Types / Modes flags or the entries
		// are dropped - the shipped prefab's 493046 / 94 include both.
		//
		// Costs come from the faction's entity catalog rather than from a gamemode attribute.
		// The purchase path reads the catalog directly, but the arsenal UI displays whatever
		// the entry itself carries - so copying the catalog price onto each entry is what
		// keeps the advertised price and the charged price the same number.
		itemList.CRF_ReplaceWithStandaloneItems(ammunition, BuildSupplyCosts(ammunition),
			SCR_EArsenalItemType.EQUIPMENT, SCR_EArsenalItemMode.AMMUNITION);

		ApplySupplyUsage(data.m_Cache);

		// Rebuild the served list and tell clients, now that the entries have changed
		arsenal.RefreshArsenal();

		Print(string.Format("[CRF_CacheHunt] Cache %1 arsenal list rewritten: %2 authored entries -> %3 gearscript ammunition entries.",
			data.m_iIndex + 1, before, itemList.CRF_GetItemCount()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Looks up each magazine's supply cost in the faction entity catalogs, the same source
	//! CRF_VehicleGearscriptManager prices truck resupply from. Anything with no catalog
	//! entry stays at 0, which is also what vanilla would charge for it.
	//! \param[in] ammunition Magazine prefabs to price
	//! \return Costs parallel to ammunition
	protected array<int> BuildSupplyCosts(notnull array<ResourceName> ammunition)
	{
		array<int> costs = {};
		costs.Reserve(ammunition.Count());

		for (int i = 0; i < ammunition.Count(); i++)
		{
			costs.Insert(0);
		}

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!catalogManager || !factionManager)
			return costs;

		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);

		foreach (Faction faction : factions)
		{
			SCR_EntityCatalog catalog = catalogManager.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, faction.GetFactionKey(), false);
			if (!catalog)
				continue;

			for (int i = 0; i < ammunition.Count(); i++)
			{
				SCR_EntityCatalogEntry entry = catalog.GetEntryWithPrefab(ammunition[i]);
				if (!entry)
					continue;

				SCR_ArsenalItem data = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
				if (!data)
					continue;

				costs.Set(i, data.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false));
			}
		}

		return costs;
	}

	//------------------------------------------------------------------------------------------------
	//! Turns supply charging on or off for a cache.
	//!
	//! An arsenal only charges at all when its owner's SCR_ResourceComponent has SUPPLIES
	//! enabled (SCR_ArsenalComponent.IsArsenalUsingSupplies). The shipped cache prefab
	//! ships with SUPPLIES in its disabled list, so without this every take is free no
	//! matter what the arsenal entries say.
	protected void ApplySupplyUsage(notnull IEntity cache)
	{
		SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.FindResourceComponent(cache);
		if (!resourceComponent)
		{
			if (m_bAmmoCostsSupplies)
				Print(string.Format("[CRF_CacheHunt] Cache prefab '%1' has no SCR_ResourceComponent, so ammunition cannot cost supplies.", m_rCachePrefab), LogLevel.WARNING);

			return;
		}

		resourceComponent.SetResourceTypeEnabled(m_bAmmoCostsSupplies, EResourceType.SUPPLIES);

		// The setter is a no-op when the prefab disallows changing this resource type, and
		// it fails silently, so confirm rather than assume.
		if (resourceComponent.IsResourceTypeEnabled(EResourceType.SUPPLIES) != m_bAmmoCostsSupplies)
		{
			Print(string.Format("[CRF_CacheHunt] Could not set supply usage to %1 on cache prefab '%2'. Its SCR_ResourceComponent disallows changing SUPPLIES - fix it in the prefab instead.",
				m_bAmmoCostsSupplies, m_rCachePrefab), LogLevel.WARNING);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Builds (and caches) the ammunition list for the defending faction by walking every
	//! magazine array in that faction's gearscript config.
	protected array<ResourceName> GetDefenderAmmunition()
	{
		// Only treat the list as cached once it actually resolved. Caching an empty result
		// would permanently wedge the caches empty if the gearscript was not ready yet.
		if (m_aDefenderAmmunition && !m_aDefenderAmmunition.IsEmpty())
			return m_aDefenderAmmunition;

		m_aDefenderAmmunition = {};

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_GearscriptManager gearscriptManager = COA_GearscriptManager.GetInstance();
		if (!gamemode)
		{
			Print("[CRF_CacheHunt] COA_Gamemode.GetInstance() is null - cannot resolve the defending gearscript.", LogLevel.ERROR);
			return m_aDefenderAmmunition;
		}

		if (!gearscriptManager)
		{
			Print("[CRF_CacheHunt] COA_GearscriptManager.GetInstance() is null - cannot load the defending gearscript.", LogLevel.ERROR);
			return m_aDefenderAmmunition;
		}

		// Resolve the gearscript the same way CRF_VehicleGearscriptManager.SetTruckGear does:
		// straight off the faction's container. GetGearScriptResource() returns the separate
		// replicated "current" field instead, which is not the same value and is populated
		// later in startup.
		ResourceName gearScriptResource;
		COA_GearScriptContainer gsContainer = gamemode.GetGearScriptSettings(m_DefendingSide);
		if (gsContainer)
			gearScriptResource = gsContainer.m_rGearScript;

		if (gearScriptResource.IsEmpty())
			gearScriptResource = gamemode.GetGearScriptResource(m_DefendingSide);

		if (gearScriptResource.IsEmpty())
		{
			Print(string.Format("[CRF_CacheHunt] No gearscript assigned to the defending faction '%1' (container found: %2). Caches cannot be stocked.", m_DefendingSide, gsContainer != null), LogLevel.WARNING);
			return m_aDefenderAmmunition;
		}

		Print(string.Format("[CRF_CacheHunt] Defending faction '%1' resolved to gearscript '%2'.", m_DefendingSide, gearScriptResource), LogLevel.NORMAL);

		COA_GearScriptConfig config = gearscriptManager.LoadGearScriptConfig(gearScriptResource);
		if (!config)
		{
			Print(string.Format("[CRF_CacheHunt] Gearscript '%1' could not be loaded.", gearScriptResource), LogLevel.WARNING);
			return m_aDefenderAmmunition;
		}

		if (m_eAmmoClasses & CRF_ECacheHuntAmmoClass.SMALL_ARMS)
		{
			CollectMagazinesFromWeapons(config.m_Rifles);
			CollectMagazinesFromWeapons(config.m_Carbines);
			CollectMagazinesFromWeapons(config.m_Pistols);

			if (config.m_SNIPER)
				CollectMagazines(config.m_SNIPER.m_MagazineArray);
		}

		if (m_eAmmoClasses & CRF_ECacheHuntAmmoClass.GRENADE_LAUNCHER)
			CollectMagazinesFromWeapons(config.m_RifleUGLs);

		if (m_eAmmoClasses & CRF_ECacheHuntAmmoClass.SUPPORT)
		{
			CollectSpecMagazines(config.m_AR);
			CollectSpecMagazines(config.m_MMG);
			CollectSpecMagazines(config.m_HMG);
		}

		// A launcher's magazine array is its rockets, so these two are what put Iglas and
		// RPG rounds in a cache. Off by default.
		if (m_eAmmoClasses & CRF_ECacheHuntAmmoClass.ANTI_TANK)
		{
			CollectSpecMagazines(config.m_AT);
			CollectSpecMagazines(config.m_MAT);
			CollectSpecMagazines(config.m_HAT);
		}

		if (m_eAmmoClasses & CRF_ECacheHuntAmmoClass.ANTI_AIR)
			CollectSpecMagazines(config.m_AA);

		if ((m_eAmmoClasses & CRF_ECacheHuntAmmoClass.CUSTOM_ROLES) && config.m_RolesToSetCustomSettings)
		{
			foreach (COA_Role_Custom_Gear customGear : config.m_RolesToSetCustomSettings)
			{
				if (!customGear)
					continue;

				CollectMagazinesFromWeapons(customGear.m_PrimaryWeapon);
				CollectMagazinesFromWeapons(customGear.m_SecondaryWeapon);
				CollectMagazinesFromWeapons(customGear.m_Pistols);
			}
		}

		if (m_aDefenderAmmunition.IsEmpty())
		{
			// The config parsed but every magazine array in it was empty, which usually means
			// the wrong gearscript is assigned to this faction rather than a code fault.
			Print(string.Format("[CRF_CacheHunt] Gearscript '%1' parsed but contains no magazines. Rifles=%2 UGLs=%3 Carbines=%4 Pistols=%5 Sniper=%6 CustomRoles=%7",
				gearScriptResource,
				CountOrZero(config.m_Rifles),
				CountOrZero(config.m_RifleUGLs),
				CountOrZero(config.m_Carbines),
				CountOrZero(config.m_Pistols),
				config.m_SNIPER != null,
				CountOrZeroRoles(config.m_RolesToSetCustomSettings)), LogLevel.WARNING);
		}
		else
		{
			Print(string.Format("[CRF_CacheHunt] Cache ammunition list built from '%1': %2 magazine type(s). First entry: %3",
				gearScriptResource, m_aDefenderAmmunition.Count(), m_aDefenderAmmunition[0]), LogLevel.NORMAL);
		}

		return m_aDefenderAmmunition;
	}

	//------------------------------------------------------------------------------------------------
	protected int CountOrZero(array<ref COA_Weapon_Class> weapons)
	{
		if (!weapons)
			return 0;

		return weapons.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected int CountOrZeroRoles(array<ref COA_Role_Custom_Gear> roles)
	{
		if (!roles)
			return 0;

		return roles.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectMagazinesFromWeapons(array<ref COA_Weapon_Class> weapons)
	{
		if (!weapons)
			return;

		foreach (COA_Weapon_Class weapon : weapons)
		{
			if (weapon)
				CollectMagazines(weapon.m_MagazineArray);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectSpecMagazines(COA_Spec_Weapon_Class weapon)
	{
		if (!weapon)
			return;

		if (!weapon.m_MagazineArray)
			return;

		foreach (COA_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
		{
			if (magazine && !magazine.m_Magazine.IsEmpty() && !m_aDefenderAmmunition.Contains(magazine.m_Magazine))
				m_aDefenderAmmunition.Insert(magazine.m_Magazine);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectMagazines(array<ref COA_Magazine_Class> magazines)
	{
		if (!magazines)
			return;

		foreach (COA_Magazine_Class magazine : magazines)
		{
			if (magazine && !magazine.m_Magazine.IsEmpty() && !m_aDefenderAmmunition.Contains(magazine.m_Magazine))
				m_aDefenderAmmunition.Insert(magazine.m_Magazine);
		}
	}

	//===================================================================================
	// MAP MARKERS (CLIENT)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Both marker sets are driven from a poll rather than replication alone, because the
	//! local player's faction can change between slotting and respawns.
	protected void StartLocalMarkerPoll()
	{
		if (m_bMarkerPollRunning)
			return;

		m_bMarkerPollRunning = true;
		GetGame().GetCallqueue().CallLater(UpdateLocalMarkers, MARKER_POLL_INTERVAL_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnActiveCachesReplicated()
	{
		UpdateLocalMarkers();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateLocalMarkers()
	{
		UpdateDefenderMarkers();
		UpdateSearchMarkers();
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns and removes the attacker search shapes on this machine.
	//!
	//! COA_ShapeMarker carries no RplComponent, so it cannot be a server-spawned replicated
	//! entity - each client spawns its own copy instead. The server still decides where the
	//! shapes sit (see ComputeSearchCenter), so every attacker searches the same ground.
	//! Because only attackers ever spawn one, no faction visibility filtering is needed.
	protected void UpdateSearchMarkers()
	{
		bool shouldShow = m_bEnableSearchMarkers && IsLocalPlayerAttacker();

		// A narrowing step resizes every shape, so rebuild them all rather than trying to
		// resize in place - it happens a handful of times a mission at most.
		if (m_fLocalSearchMarkerFactor != m_fSearchRadiusFactor)
		{
			m_fLocalSearchMarkerFactor = m_fSearchRadiusFactor;
			ClearAllSearchMarkers();
		}

		array<vector> wanted = {};
		if (shouldShow)
		{
			foreach (vector position : m_aSearchMarkerPositions)
				wanted.Insert(position);
		}

		// Drop shapes whose cache is gone, or all of them if the local player is no longer attacking
		for (int i = m_aLocalSearchMarkerPositions.Count() - 1; i >= 0; i--)
		{
			if (wanted.Contains(m_aLocalSearchMarkerPositions[i]))
				continue;

			if (m_aLocalSearchMarkers.IsIndexValid(i))
			{
				IEntity marker = m_aLocalSearchMarkers[i];
				if (marker)
					SCR_EntityHelper.DeleteEntityAndChildren(marker);

				m_aLocalSearchMarkers.Remove(i);
			}

			m_aLocalSearchMarkerPositions.Remove(i);
		}

		// Spawn shapes that are missing
		foreach (vector position : wanted)
		{
			if (m_aLocalSearchMarkerPositions.Contains(position))
				continue;

			IEntity marker = SpawnLocalSearchMarker(position);
			if (!marker)
				continue;

			m_aLocalSearchMarkers.Insert(marker);
			m_aLocalSearchMarkerPositions.Insert(position);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns one search shape locally and configures it. Every setter used here is
	//! local-only, which is what an unreplicated marker needs.
	protected IEntity SpawnLocalSearchMarker(vector position)
	{
		Resource markerResource = Resource.Load(m_rSearchMarkerPrefab);
		if (!markerResource || !markerResource.IsValid())
		{
			Print(string.Format("[CRF_CacheHunt] Search marker prefab '%1' could not be loaded.", m_rSearchMarkerPrefab), LogLevel.ERROR);
			return null;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = position;

		IEntity markerEntity = GetGame().SpawnEntityPrefab(markerResource, GetGame().GetWorld(), spawnParams);
		if (!markerEntity)
			return null;

		COA_ShapeMarker marker = COA_ShapeMarker.Cast(markerEntity);
		if (!marker)
		{
			Print(string.Format("[CRF_CacheHunt] Search marker prefab '%1' is not a COA_ShapeMarker.", m_rSearchMarkerPrefab), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(markerEntity);
			return null;
		}

		// Only the local attacker has this entity at all, so it can safely show for any faction
		marker.m_bShowForAnyFaction = true;
		marker.m_bVisibleForEmptyFaction = true;
		marker.m_bUseWorldScale = true;
		marker.m_MarkerColor = m_SearchMarkerColor;

		marker.SetShapeType(m_eSearchMarkerShape);
		marker.SetShapeBorderWidth(m_fSearchMarkerBorderWidth);
		// COA_ShapeMarker draws the shape centred on the entity and sized to its full
		// extent, so the world size is the circle's diameter rather than its radius.
		// The factor is replicated, so every attacker draws the same size.
		marker.SetShapeRadius(m_fSearchAreaRadius * m_fSearchRadiusFactor);

		// A manual marker only builds its widget on the map-open event it subscribes to in
		// EOnInit. Spawned while the map is already open, it would stay invisible until the
		// player closed and reopened the map, so build it now. Deferred a beat because
		// EOnInit has not run yet on a freshly spawned entity.
		GetGame().GetCallqueue().CallLater(EnsureMarkerWidget, MARKER_WIDGET_DELAY_MS, false, markerEntity);

		return markerEntity;
	}

	//------------------------------------------------------------------------------------------------
	//! Builds a spawned marker's map widget if the local player already has the map open.
	//! COA_ManualMarker.CreateMapWidget is idempotent, so a redundant call is harmless.
	protected void EnsureMarkerWidget(IEntity markerEntity)
	{
		COA_ManualMarker marker = COA_ManualMarker.Cast(markerEntity);
		if (!marker)
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		marker.CreateMapWidget(mapEntity.GetMapConfig());
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearAllSearchMarkers()
	{
		foreach (IEntity marker : m_aLocalSearchMarkers)
		{
			if (marker)
				SCR_EntityHelper.DeleteEntityAndChildren(marker);
		}

		m_aLocalSearchMarkers.Clear();
		m_aLocalSearchMarkerPositions.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the local player's cache markers. Only the defending side sees them, and
	//! only surviving caches are drawn.
	protected void UpdateDefenderMarkers()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		bool shouldShow = m_bEnableDefenderMarkers && IsLocalPlayerDefender();

		array<string> wantedPositions = {};
		array<string> wantedLabels = {};

		if (shouldShow)
		{
			foreach (int i, vector position : m_aActiveCachePositions)
			{
				int cacheIndex = -1;
				if (m_aActiveCacheIndices.IsIndexValid(i))
					cacheIndex = m_aActiveCacheIndices[i];

				wantedPositions.Insert(string.Format("%1 %2 %3", position[0], position[1], position[2]));
				wantedLabels.Insert(GetCacheDisplayName(cacheIndex));
			}
		}

		// Drop markers that no longer belong on the map
		for (int i = m_aLocalDefenderMarkerPositions.Count() - 1; i >= 0; i--)
		{
			if (IsMarkerWanted(m_aLocalDefenderMarkerPositions[i], m_aLocalDefenderMarkerLabels[i], wantedPositions, wantedLabels))
				continue;

			markerManager.RemoveScriptedMarker("Static Marker", m_aLocalDefenderMarkerPositions[i], DEFENDER_MARKER_UPDATE_DELAY,
				m_aLocalDefenderMarkerLabels[i], m_rDefenderMarkerIcon, DEFENDER_MARKER_Z_ORDER, DEFENDER_MARKER_COLOR);

			m_aLocalDefenderMarkerPositions.Remove(i);
			m_aLocalDefenderMarkerLabels.Remove(i);
		}

		// Add markers that are missing
		foreach (int i, string position : wantedPositions)
		{
			string label = wantedLabels[i];
			if (IsMarkerWanted(position, label, m_aLocalDefenderMarkerPositions, m_aLocalDefenderMarkerLabels))
				continue;

			markerManager.AddScriptedMarker("Static Marker", position, DEFENDER_MARKER_UPDATE_DELAY,
				label, m_rDefenderMarkerIcon, DEFENDER_MARKER_Z_ORDER, DEFENDER_MARKER_COLOR);

			m_aLocalDefenderMarkerPositions.Insert(position);
			m_aLocalDefenderMarkerLabels.Insert(label);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return True when the position/label pair appears in the given parallel arrays
	protected bool IsMarkerWanted(string position, string label, notnull array<string> positions, notnull array<string> labels)
	{
		foreach (int i, string candidate : positions)
		{
			if (candidate == position && labels.IsIndexValid(i) && labels[i] == label)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearAllDefenderMarkers()
	{
		COA_PlayerScriptedMarkerManager markerManager = COA_PlayerScriptedMarkerManager.GetInstance();
		if (markerManager)
		{
			foreach (int i, string position : m_aLocalDefenderMarkerPositions)
			{
				if (!m_aLocalDefenderMarkerLabels.IsIndexValid(i))
					continue;

				markerManager.RemoveScriptedMarker("Static Marker", position, DEFENDER_MARKER_UPDATE_DELAY,
					m_aLocalDefenderMarkerLabels[i], m_rDefenderMarkerIcon, DEFENDER_MARKER_Z_ORDER, DEFENDER_MARKER_COLOR);
			}
		}

		m_aLocalDefenderMarkerPositions.Clear();
		m_aLocalDefenderMarkerLabels.Clear();
	}

	//===================================================================================
	// PUBLIC API
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! \return True when the given entity belongs to the defending side
	bool IsDefender(IEntity entity)
	{
		if (!entity)
			return false;

		return GetEntityFactionKey(entity) == m_DefendingSide;
	}

	//------------------------------------------------------------------------------------------------
	//! \return True when the given entity belongs to the attacking side
	bool IsAttacker(IEntity entity)
	{
		if (!entity)
			return false;

		return GetEntityFactionKey(entity) == m_AttackingSide;
	}

	//------------------------------------------------------------------------------------------------
	//! \return True when the local player is slotted on the defending side
	bool IsLocalPlayerDefender()
	{
		return GetLocalPlayerFactionKey() == m_DefendingSide;
	}

	//------------------------------------------------------------------------------------------------
	//! \return True when the local player is slotted on the attacking side
	bool IsLocalPlayerAttacker()
	{
		return GetLocalPlayerFactionKey() == m_AttackingSide;
	}

	//------------------------------------------------------------------------------------------------
	//! \return The local player's faction key, or an empty string when unslotted
	protected FactionKey GetLocalPlayerFactionKey()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return "";

		Faction localFaction = factionManager.GetLocalPlayerFaction();
		if (!localFaction)
			return "";

		return localFaction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves an entity's faction key through its faction affiliation component.
	static FactionKey GetEntityFactionKey(IEntity entity)
	{
		if (!entity)
			return "";

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return "";

		Faction faction = affiliation.GetAffiliatedFaction();
		if (!faction)
			return "";

		return faction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	//! Human-readable label for a cache index, e.g. 0 -> "A".
	static string GetCacheLabel(int index)
	{
		switch (index)
		{
			case 0: return "A";
			case 1: return "B";
			case 2: return "C";
			case 3: return "D";
			case 4: return "E";
		}

		return (index + 1).ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! The name a cache goes by everywhere players can see it - the defenders' map markers
	//! and the teleport actions both use this, so a defender reading "Cache B" on the map
	//! knows it is the same cache as "Teleport to Cache B" on the flag.
	//! \param[in] index 0-based cache index
	static string GetCacheDisplayName(int index)
	{
		return string.Format("Cache %1", GetCacheLabel(index));
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_CacheHuntCacheData GetCacheData(int index)
	{
		foreach (CRF_CacheHuntCacheData cache : m_aCaches)
		{
			if (cache && cache.m_iIndex == index)
				return cache;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	FactionKey GetAttackingSide() { return m_AttackingSide; }
	FactionKey GetDefendingSide() { return m_DefendingSide; }
	bool AreTeleportFlagsEnabled() { return m_bEnableTeleportFlags; }
	float GetEnemyProximityRadius() { return m_fEnemyProximityRadius; }
	int GetCacheTotal() { return m_iCacheTotal; }
	int GetCachesDestroyed() { return m_iCachesDestroyed; }
	int GetCachesRemaining() { return m_iCacheTotal - m_iCachesDestroyed; }

	//===================================================================================
	// CONSTANTS
	//===================================================================================

	static const int MAX_CACHES							= 5;
	static const int HOME_FLAG_INDEX					= -1;
	protected static const int RANDOM_PLACEMENT_ATTEMPTS	= 40;
	protected static const int MAX_RPL_WAIT_ATTEMPTS	= 20;
	protected static const int RPL_WAIT_INTERVAL_MS		= 100;
	protected static const int ARSENAL_SETUP_DELAY_MS	= 1000;
	protected static const float DESTROY_RANGE			= 8;
	protected static const ResourceName DESTRUCTION_EFFECT = "{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et";
	protected static const float CACHE_CLEARANCE_RADIUS	= 3;
	protected static const float FLAG_CLEARANCE_RADIUS	= 3;
	protected static const int MARKER_POLL_INTERVAL_MS	= 2000;
	protected static const int MARKER_WIDGET_DELAY_MS	= 100;
	protected static const int NARROW_CHECK_INTERVAL_MS	= 15000;
	protected static const int DEFENDER_MARKER_UPDATE_DELAY	= 1000;
	protected static const int DEFENDER_MARKER_Z_ORDER	= 500;
	protected static const int DEFENDER_MARKER_COLOR	= ARGB(255, 80, 200, 120);
}
