//------------------------------------------------------------------------------------
// CRF_EvacuationGamemodeComponent: the defenders have to get their supplies out before the site falls
//
// The evacuating side holds a site with a stock of supplies, a truckload at a time. A cargo truck parked
// in the loading bay loads one truckload while no attackers are in the site; driven to the extraction
// point and held there, it unloads, and the load counts as evacuated. The empty truck can go back for
// more. A loaded truck that's destroyed loses its load for good.
//
// The evacuating side wins when enough loads are out and then enough of its own players have reached the
// extraction point too (Require Withdrawal; off, the last load wins). The attacking side wins by taking the site (inside
// it with no evacuators for long enough), by destroying loads until there aren't enough left, or when
// the clock runs out. A side with nobody left alive loses.
//
// Every machine draws the site and extraction markers and the HUD panel from a few replicated numbers.
// The trucks are ordinary vehicles: place them with vehicle spawners (or the vehicle depot) and list
// their prefabs in m_aCargoPrefabs.
//
// Setup for mission makers (full guide: !Docs/MissionMaking/Gamemodes/CRF_EVACUATION_MISSION_MAKERS_GUIDE.md):
//   1. Add this component to the gamemode entity.
//   2. Place named entities: "evac_site" (middle of the site), "evac_load" (the loading bay, on open
//      ground a truck can park on) and "evac_extract" (the extraction point).
//   3. Give the evacuating side cargo trucks and list their prefabs in m_aCargoPrefabs.
//------------------------------------------------------------------------------------

enum CRF_EEvacuationState
{
	WAITING,		//!< the round isn't live yet
	IDLE,			//!< site held, nothing loading (no truck in the bay)
	LOADING,		//!< a truck in the bay is loading
	CONTESTED,		//!< attackers in the site: loading stops
	CAPTURING,		//!< attackers in the site and no evacuators: it's falling
	EMPTY,			//!< nothing left at the site to load
	EXTRACTING,		//!< the loads are out: the evacuating side has to get its own people out
	ENDED
}

//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Evacuation: the defenders truck their supplies out of the site, the attackers take it first.")]
class CRF_EvacuationGamemodeComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_EvacuationGamemodeComponent : SCR_BaseGameModeComponent
{
	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the side getting its supplies out", category: "Evacuation - Factions")]
	protected string m_sEvacuatingFaction;

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the side taking the site", category: "Evacuation - Factions")]
	protected string m_sAttackingFaction;

	[Attribute("evac_site", UIWidgets.EditBox, "Name of an entity in the middle of the site", category: "Evacuation - Site")]
	protected string m_sSiteName;

	[Attribute("150", UIWidgets.EditBox, "Metres from the site entity that count as the site", params: "20 1000 1", category: "Evacuation - Site")]
	protected float m_fSiteRadius;

	[Attribute("90", UIWidgets.EditBox, "Seconds attackers must hold the site with no evacuators in it to take it. It climbs while they do, holds while both sides are in it and falls back while no attackers are.", params: "10 1800 1", category: "Evacuation - Site")]
	protected float m_fCaptureSeconds;

	[Attribute("1", UIWidgets.CheckBox, "Any living attacker in the site stops the loading", category: "Evacuation - Site")]
	protected bool m_bAttackersStopLoading;

	[Attribute("evac_load", UIWidgets.EditBox, "Name of an entity at the loading bay. Put it on open ground a truck can park on, inside the site.", category: "Evacuation - Loading")]
	protected string m_sLoadingBayName;

	[Attribute("15", UIWidgets.EditBox, "Metres from the loading bay entity a truck has to be to load", params: "3 100 1", category: "Evacuation - Loading")]
	protected float m_fLoadingRadius;

	[Attribute("8", UIWidgets.EditBox, "Truckloads at the site when the round starts", params: "1 100 1", category: "Evacuation - Loading")]
	protected int m_iTotalLoads;

	[Attribute("5", UIWidgets.EditBox, "Truckloads that have to reach the extraction point for the evacuating side to win", params: "1 100 1", category: "Evacuation - Loading")]
	protected int m_iLoadsToWin;

	[Attribute("60", UIWidgets.EditBox, "Seconds a truck takes to load. It loads one truck at a time, standing still (or near enough) in the bay.", params: "5 1200 1", category: "Evacuation - Loading")]
	protected float m_fLoadSeconds;

	[Attribute("2", UIWidgets.EditBox, "Fastest a truck can be moving, in metres per second, and still load or unload", params: "0 20 0.1", category: "Evacuation - Loading")]
	protected float m_fMaxWorkSpeed;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Prefabs of the trucks that can carry a load. Empty: any vehicle can.", params: "et", category: "Evacuation - Loading")]
	protected ref array<ResourceName> m_aCargoPrefabs;

	[Attribute("evac_extract", UIWidgets.EditBox, "Name of an entity at the extraction point", category: "Evacuation - Extraction")]
	protected string m_sExtractionName;

	[Attribute("30", UIWidgets.EditBox, "Metres from the extraction entity a loaded truck has to be to unload", params: "5 300 1", category: "Evacuation - Extraction")]
	protected float m_fExtractionRadius;

	[Attribute("15", UIWidgets.EditBox, "Seconds a loaded truck takes to unload at the extraction point, with someone from the evacuating side in it and no attackers", params: "0 600 1", category: "Evacuation - Extraction")]
	protected float m_fUnloadSeconds;

	[Attribute("1", UIWidgets.CheckBox, "Once enough loads are out, the evacuating side has to pull out too: it wins only when enough of its players reach the extraction point. Off: the last load out wins.", category: "Evacuation - Extraction")]
	protected bool m_bRequireWithdrawal;

	[Attribute("0.5", UIWidgets.EditBox, "Share of the evacuating side's players still alive when the last load gets out who must then reach the extraction point (at least one)", params: "0.05 1 0.05", category: "Evacuation - Extraction")]
	protected float m_fWithdrawShare;

	[Attribute("50", UIWidgets.EditBox, "Metres from the extraction entity a player has to come, on foot or in a vehicle, to get out. They leave the round alive and spectate.", params: "5 500 1", category: "Evacuation - Extraction")]
	protected float m_fWithdrawRadius;

	[Attribute("3600", UIWidgets.EditBox, "Seconds on the clock when the round goes live. When it runs out the attackers win.", params: "60 14400 1", category: "Evacuation - Ending")]
	protected int m_iStartSeconds;

	[Attribute("1", UIWidgets.CheckBox, "A side with players slotted and nobody left alive loses", category: "Evacuation - Ending")]
	protected bool m_bWipeLoses;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round is decided. Off: the result is announced and an admin ends the mission.", category: "Evacuation - Ending")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker for the site and the extraction point", "edds", category: "Evacuation - Map")]
	protected ResourceName m_sMarkerIcon;

	[Attribute("{9EBE212C91B36BBE}UI/Textures/Editor/EditableEntities/Vehicles/EditableEntity_Vehicle_Truck.edds", UIWidgets.ResourcePickerThumbnail, "Map marker for the loading bay", "edds", category: "Evacuation - Map")]
	protected ResourceName m_sLoadingMarkerIcon;

	// Replicated: all a client needs for the panel and the mission info
	[RplProp()]
	protected int m_iState;
	[RplProp()]
	protected int m_iStock;				//!< loads still at the site
	[RplProp()]
	protected int m_iInTransit;			//!< loads on trucks
	[RplProp()]
	protected int m_iDelivered;
	[RplProp()]
	protected int m_iLost;
	[RplProp()]
	protected int m_iSecondsLeft;
	[RplProp()]
	protected int m_iCapturePercent;
	[RplProp()]
	protected int m_iLoadPercent;
	[RplProp()]
	protected int m_iUnloadPercent;		//!< the furthest-along truck unloading, -1 if none
	[RplProp()]
	protected bool m_bEvacuatorsWon;
	[RplProp()]
	protected int m_iWithdrawn;			//!< evacuators who got out after the loads
	[RplProp()]
	protected int m_iWithdrawNeeded;

	// Every machine
	protected ref CRF_SpecialsMarkerSet m_Markers;
	protected ref CRF_EvacuationHUD m_Hud;

	// Server
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_Safestart;
	protected IEntity m_Site;
	protected IEntity m_LoadingBay;
	protected IEntity m_Extraction;
	protected bool m_bRoundLive;
	protected bool m_bEndRequested;
	protected float m_fCaptureProgress;	//!< seconds of the m_fCaptureSeconds needed
	protected float m_fClockCarry;
	protected Vehicle m_LoadingTruck;
	protected float m_fLoadProgress;
	protected ref array<Vehicle> m_aLoadedTrucks = {};
	protected ref array<float> m_aUnloadProgress = {};	//!< matches m_aLoadedTrucks
	protected ref array<IEntity> m_aQueryResults = {};
	protected string m_sLastReplicated;

	protected static const int TICK_MS = 500;
	protected static const float TICK_SECONDS = 0.5;
	protected static const string LOG = "[CRF_Evacuation]";

	protected static CRF_EvacuationGamemodeComponent s_Instance;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_EvacuationGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	void ~CRF_EvacuationGamemodeComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}

	static CRF_EvacuationGamemodeComponent GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		if (!System.IsConsoleApp())
		{
			m_Markers = new CRF_SpecialsMarkerSet();
			m_Hud = new CRF_EvacuationHUD();
			GetGame().GetCallqueue().CallLater(UpdateMarkers, 1000, true);
			GetGame().GetCallqueue().CallLater(UpdateHud, 250, true);
		}

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(HookGamemode, 1000, false);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(HookGamemode);
			GetGame().GetCallqueue().Remove(UpdateMarkers);
			GetGame().GetCallqueue().Remove(UpdateHud);
		}

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Safestart && m_Safestart.m_OnSafeStartChange)
			m_Safestart.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Markers)
			m_Markers.Clear();

		if (m_Hud)
			m_Hud.Destroy();

		super.OnDelete(owner);
	}

	protected void HookGamemode()
	{
		m_Gamemode = COA_Gamemode.GetInstance();
		if (!m_Gamemode)
		{
			Print(LOG + " No COA_Gamemode: the evacuation is not running.", LogLevel.WARNING);
			return;
		}

		m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		// The clock starts when safestart ends, not when it begins
		m_Safestart = COA_SafestartManager.GetInstance();
		if (m_Safestart && m_Safestart.m_OnSafeStartChange)
			m_Safestart.m_OnSafeStartChange.Insert(OnSafestartChanged);

		if (m_iLoadsToWin > m_iTotalLoads)
		{
			Print(string.Format("%1 Loads To Win (%2) is more than the site holds (%3): capped.", LOG, m_iLoadsToWin, m_iTotalLoads), LogLevel.WARNING);
			m_iLoadsToWin = m_iTotalLoads;
		}

		m_iStock = m_iTotalLoads;
		m_iSecondsLeft = m_iStartSeconds;
		m_iLoadPercent = -1;
		m_iUnloadPercent = -1;
		Replication.BumpMe();

		OnGamemodeStateChanged();
	}

	protected void OnSafestartChanged(bool safestartActive)
	{
		if (!safestartActive && m_Gamemode && m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
			StartRound();
	}

	protected void OnGamemodeStateChanged()
	{
		if (!m_Gamemode)
			return;

		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME)
		{
			// Missions with safestart start on OnSafestartChanged
			if (!m_bRoundLive && (!m_Safestart || m_Safestart.IsRoundLive()))
				StartRound();
			return;
		}

		// The mission ended, whoever decided it
		if (m_bRoundLive && m_iState != CRF_EEvacuationState.ENDED)
		{
			GetGame().GetCallqueue().Remove(Tick);
			m_iState = CRF_EEvacuationState.ENDED;
			Replication.BumpMe();
		}
	}

	protected void StartRound()
	{
		if (m_bRoundLive)
			return;

		m_Site = CRF_SpecialsHelper.FindNamedEntity(m_sSiteName, LOG);
		m_LoadingBay = CRF_SpecialsHelper.FindNamedEntity(m_sLoadingBayName, LOG);
		m_Extraction = CRF_SpecialsHelper.FindNamedEntity(m_sExtractionName, LOG);
		if (!m_Site || !m_LoadingBay || !m_Extraction)
		{
			Print(LOG + " The round went live without its site, loading bay or extraction entity: the evacuation is not running.", LogLevel.ERROR);
			return;
		}

		m_bRoundLive = true;
		m_iState = CRF_EEvacuationState.IDLE;
		Replication.BumpMe();

		CRF_RadioTransmissionEvents.PopupToFaction(m_sEvacuatingFaction, "EVACUATE THE SUPPLIES", string.Format("Load trucks at the site and get %1 loads to the extraction point. Keep the enemy out of the site.", m_iLoadsToWin), 10);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, "STOP THE EVACUATION", "Take the site, or destroy their loaded trucks before they reach the extraction point.", 10);

		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
	}

	//===================================================================================
	// SERVER: THE ROUND
	//===================================================================================

	protected void Tick()
	{
		if (m_bEndRequested)
			return;

		// The loads are out: all that counts now is people reaching the extraction point
		if (m_iState == CRF_EEvacuationState.EXTRACTING)
		{
			TickWithdrawal();
			if (m_bEndRequested)
				return;

			UpdateClock();
			ReplicateIfChanged();
			return;
		}

		int attackersInSite = CountInRadius(m_sAttackingFaction, m_Site.GetOrigin(), m_fSiteRadius);
		int evacuatorsInSite = CountInRadius(m_sEvacuatingFaction, m_Site.GetOrigin(), m_fSiteRadius);

		UpdateCapture(attackersInSite, evacuatorsInSite);
		if (m_bEndRequested)
			return;

		CheckLoadedTrucks();
		if (m_bEndRequested)
			return;

		bool loadingBlocked = m_bAttackersStopLoading && attackersInSite > 0;
		UpdateLoading(loadingBlocked);

		UpdateState(attackersInSite, evacuatorsInSite, loadingBlocked);
		CheckOutcome();
		if (m_bEndRequested)
			return;

		UpdateClock();
		ReplicateIfChanged();
	}

	//! Living players of a faction within radius of a point (map distance)
	protected int CountInRadius(string factionKey, vector center, float radius)
	{
		array<int> players = {};
		CRF_SpecialsHelper.GetAlivePlayers(factionKey, players);

		int count;
		foreach (int playerId : players)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (entity && vector.DistanceXZ(entity.GetOrigin(), center) <= radius)
				count++;
		}

		return count;
	}

	protected void UpdateCapture(int attackers, int evacuators)
	{
		if (attackers > 0 && evacuators == 0)
			m_fCaptureProgress += TICK_SECONDS;
		else if (attackers == 0)
			m_fCaptureProgress = Math.Max(0, m_fCaptureProgress - TICK_SECONDS);

		m_iCapturePercent = Math.Round(Math.Clamp(m_fCaptureProgress / m_fCaptureSeconds, 0, 1) * 100);

		if (m_fCaptureProgress >= m_fCaptureSeconds)
			EndRound(false, "THE SITE HAS FALLEN", string.Format("%1 took the site with %2 of %3 loads out.", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction), m_iDelivered, m_iLoadsToWin));
	}

	//! Loaded trucks: lost if destroyed, unloaded if held at the extraction point
	protected void CheckLoadedTrucks()
	{
		vector extraction = m_Extraction.GetOrigin();
		float best = -1;

		for (int i = m_aLoadedTrucks.Count() - 1; i >= 0; i--)
		{
			Vehicle truck = m_aLoadedTrucks[i];
			if (!truck || IsDestroyed(truck))
			{
				m_aLoadedTrucks.Remove(i);
				m_aUnloadProgress.Remove(i);
				m_iInTransit = m_aLoadedTrucks.Count();
				m_iLost++;
				CRF_RadioTransmissionEvents.PopupToFaction("", "A LOADED TRUCK WAS DESTROYED", string.Format("That load is gone. %1 out, %2 still needed, %3 left to get out.", m_iDelivered, m_iLoadsToWin - m_iDelivered, m_iStock + m_iInTransit), 8);
				continue;
			}

			bool canUnload = vector.DistanceXZ(truck.GetOrigin(), extraction) <= m_fExtractionRadius && IsSlow(truck) && HasCrew(truck, m_sEvacuatingFaction) && !HasCrew(truck, m_sAttackingFaction);
			if (!canUnload)
			{
				m_aUnloadProgress[i] = 0;
				continue;
			}

			m_aUnloadProgress[i] = m_aUnloadProgress[i] + TICK_SECONDS;
			if (m_aUnloadProgress[i] < m_fUnloadSeconds)
			{
				best = Math.Max(best, m_aUnloadProgress[i] / Math.Max(m_fUnloadSeconds, 0.1));
				continue;
			}

			m_aLoadedTrucks.Remove(i);
			m_aUnloadProgress.Remove(i);
			m_iInTransit = m_aLoadedTrucks.Count();
			m_iDelivered++;

			if (m_iDelivered < m_iLoadsToWin)
			{
				CRF_RadioTransmissionEvents.PopupToFaction(m_sEvacuatingFaction, "LOAD EVACUATED", string.Format("%1 of %2 out. Get the truck back to the site for another.", m_iDelivered, m_iLoadsToWin), 8);
				CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, "A LOAD GOT OUT", string.Format("They have %1 of the %2 they need.", m_iDelivered, m_iLoadsToWin), 8);
			}
		}

		if (best < 0)
			m_iUnloadPercent = -1;
		else
			m_iUnloadPercent = Math.Round(best * 100);
	}

	//! One truck at a time loads in the bay, while the site isn't contested and there's something to load
	protected void UpdateLoading(bool blocked)
	{
		if (m_iStock <= 0 || blocked)
		{
			// A contested bay keeps the truck and its progress; it carries on when the site is clear
			if (m_iStock <= 0)
				ResetLoading();
			return;
		}

		if (!CanLoad(m_LoadingTruck))
		{
			ResetLoading();
			m_LoadingTruck = FindTruckToLoad();
			if (!m_LoadingTruck)
				return;
		}

		m_fLoadProgress += TICK_SECONDS;
		m_iLoadPercent = Math.Round(Math.Clamp(m_fLoadProgress / m_fLoadSeconds, 0, 1) * 100);
		if (m_fLoadProgress < m_fLoadSeconds)
			return;

		m_aLoadedTrucks.Insert(m_LoadingTruck);
		m_aUnloadProgress.Insert(0);
		m_iInTransit = m_aLoadedTrucks.Count();
		m_iStock--;
		ResetLoading();

		string next = "Take it to the extraction point.";
		if (m_iStock > 0)
			next = string.Format("Take it to the extraction point. %1 more at the site.", m_iStock);

		CRF_RadioTransmissionEvents.PopupToFaction(m_sEvacuatingFaction, "TRUCK LOADED", next, 8);
	}

	protected void ResetLoading()
	{
		m_LoadingTruck = null;
		m_fLoadProgress = 0;
		m_iLoadPercent = -1;
	}

	//! A cargo truck in the bay, not loaded, intact, standing still, with no attackers in it
	protected bool CanLoad(Vehicle truck)
	{
		if (!truck || IsDestroyed(truck) || m_aLoadedTrucks.Contains(truck) || !IsCargoTruck(truck))
			return false;

		if (vector.DistanceXZ(truck.GetOrigin(), m_LoadingBay.GetOrigin()) > m_fLoadingRadius)
			return false;

		return IsSlow(truck) && !HasCrew(truck, m_sAttackingFaction);
	}

	protected Vehicle FindTruckToLoad()
	{
		m_aQueryResults.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(m_LoadingBay.GetOrigin(), m_fLoadingRadius + 5, AddQueryResult, null, EQueryEntitiesFlags.DYNAMIC);

		Vehicle nearest;
		float nearestDistance = float.MAX;
		foreach (IEntity entity : m_aQueryResults)
		{
			Vehicle truck = Vehicle.Cast(entity);
			if (!CanLoad(truck))
				continue;

			float distance = vector.DistanceXZ(truck.GetOrigin(), m_LoadingBay.GetOrigin());
			if (distance < nearestDistance)
			{
				nearestDistance = distance;
				nearest = truck;
			}
		}

		m_aQueryResults.Clear();
		return nearest;
	}

	protected bool AddQueryResult(IEntity entity)
	{
		if (Vehicle.Cast(entity))
			m_aQueryResults.Insert(entity);

		return true;
	}

	protected bool IsCargoTruck(notnull IEntity entity)
	{
		if (!m_aCargoPrefabs || m_aCargoPrefabs.IsEmpty())
			return true;

		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
			return false;

		string prefab = prefabData.GetPrefabName();
		foreach (ResourceName cargo : m_aCargoPrefabs)
		{
			// Compare the GUIDs: a moved or renamed prefab keeps its GUID
			string cargoName = cargo;
			if (prefab.Length() >= 18 && cargoName.Length() >= 18 && prefab.Substring(0, 18) == cargoName.Substring(0, 18))
				return true;
		}

		return false;
	}

	protected bool IsDestroyed(notnull IEntity entity)
	{
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
		if (!damageManager)
			return false;

		return damageManager.GetState() == EDamageState.DESTROYED;
	}

	protected bool IsSlow(notnull IEntity entity)
	{
		Physics physics = entity.GetPhysics();
		if (!physics)
			return true;

		return physics.GetVelocity().Length() <= m_fMaxWorkSpeed;
	}

	//! Someone alive from factionKey sitting in the vehicle
	protected bool HasCrew(notnull IEntity vehicle, string factionKey)
	{
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
			return false;

		array<IEntity> occupants = {};
		compartments.GetOccupants(occupants);
		foreach (IEntity occupant : occupants)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(occupant);
			if (!character || character.GetFactionKey() != factionKey)
				continue;

			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller && !controller.IsDead())
				return true;
		}

		return false;
	}

	protected void UpdateState(int attackers, int evacuators, bool loadingBlocked)
	{
		if (attackers > 0 && evacuators == 0)
			m_iState = CRF_EEvacuationState.CAPTURING;
		else if (loadingBlocked)
			m_iState = CRF_EEvacuationState.CONTESTED;
		else if (m_iStock <= 0)
			m_iState = CRF_EEvacuationState.EMPTY;
		else if (m_LoadingTruck)
			m_iState = CRF_EEvacuationState.LOADING;
		else
			m_iState = CRF_EEvacuationState.IDLE;
	}

	protected void CheckOutcome()
	{
		if (m_iDelivered >= m_iLoadsToWin)
		{
			if (m_bRequireWithdrawal)
			{
				BeginWithdrawal();
				return;
			}

			EndRound(true, "EVACUATION COMPLETE", string.Format("%1 got %2 loads out of the site.", CRF_SpecialsHelper.GetFactionName(m_sEvacuatingFaction), m_iDelivered));
			return;
		}

		if (m_iDelivered + m_iInTransit + m_iStock < m_iLoadsToWin)
		{
			EndRound(false, "THE EVACUATION FAILED", string.Format("Too much was destroyed: %1 out, and only %2 left that could still get out. %3 needed.", m_iDelivered, m_iInTransit + m_iStock, m_iLoadsToWin));
			return;
		}

		if (!m_bWipeLoses)
			return;

		if (IsWipedOut(m_sEvacuatingFaction))
			EndRound(false, "THE EVACUATION FAILED", string.Format("%1 has nobody left alive.", CRF_SpecialsHelper.GetFactionName(m_sEvacuatingFaction)));
		else if (IsWipedOut(m_sAttackingFaction))
			EndRound(true, "THE ATTACK FAILED", string.Format("%1 has nobody left alive.", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction)));
	}

	//! The last load is out: now the evacuating side has to get out itself
	protected void BeginWithdrawal()
	{
		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sEvacuatingFaction, alive);
		m_iWithdrawNeeded = Math.Max(1, Math.Ceil(alive.Count() * m_fWithdrawShare));
		m_iWithdrawn = 0;

		m_iState = CRF_EEvacuationState.EXTRACTING;
		ResetLoading();
		m_fCaptureProgress = 0;
		m_iCapturePercent = 0;
		m_iUnloadPercent = -1;
		Replication.BumpMe();

		CRF_RadioTransmissionEvents.PopupToFaction(m_sEvacuatingFaction, "THE LOADS ARE OUT", string.Format("Now get yourselves out: %1 of you need to reach the extraction point.", m_iWithdrawNeeded), 10);
		CRF_RadioTransmissionEvents.PopupToFaction(m_sAttackingFaction, "THEY'RE PULLING OUT", string.Format("The supplies are gone. If %1 of them reach the extraction point, they win.", m_iWithdrawNeeded), 10);
	}

	//! Evacuators reaching the extraction point leave the round alive; enough of them and the evacuation is complete
	protected void TickWithdrawal()
	{
		vector point = m_Extraction.GetOrigin();
		array<int> alive = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sEvacuatingFaction, alive);
		foreach (int playerId : alive)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity || vector.DistanceXZ(entity.GetOrigin(), point) > m_fWithdrawRadius)
				continue;

			if (!CRF_SpecialsHelper.ExtractPlayer(playerId))
				continue;

			m_iWithdrawn++;
			CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "YOU GOT OUT", string.Format("%1 of the %2 needed are out.", m_iWithdrawn, m_iWithdrawNeeded), 8);
		}

		if (m_iWithdrawn >= m_iWithdrawNeeded)
		{
			EndRound(true, "EVACUATION COMPLETE", string.Format("%1 got %2 loads out of the site, and %3 of their own.", CRF_SpecialsHelper.GetFactionName(m_sEvacuatingFaction), m_iDelivered, m_iWithdrawn));
			return;
		}

		// Extracted players count as dead to the Lobby, so the wipe check can't be used for this side now
		CRF_SpecialsHelper.GetAlivePlayers(m_sEvacuatingFaction, alive);
		if (m_iWithdrawn + alive.Count() < m_iWithdrawNeeded)
		{
			EndRound(false, "THE WITHDRAWAL FAILED", string.Format("The supplies got out, but only %1 of the %2 %3 needed can still reach the extraction point.", m_iWithdrawn + alive.Count(), m_iWithdrawNeeded, CRF_SpecialsHelper.GetFactionName(m_sEvacuatingFaction)));
			return;
		}

		if (m_bWipeLoses && IsWipedOut(m_sAttackingFaction))
			EndRound(true, "THE ATTACK FAILED", string.Format("%1 has nobody left alive.", CRF_SpecialsHelper.GetFactionName(m_sAttackingFaction)));
	}

	//! A side with players slotted and none of them alive
	protected bool IsWipedOut(string factionKey)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return false;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		bool anyPlayers;
		foreach (int playerId : playerIds)
		{
			if (!slottingManager.IsPlayerInASlot(playerId))
				continue;

			Faction faction = slottingManager.GetPlayerSlotFaction(playerId, true);
			if (!faction || faction.GetFactionKey() != factionKey)
				continue;

			if (!slottingManager.IsPlayerConsideredDead(playerId))
				return false;

			anyPlayers = true;
		}

		return anyPlayers;
	}

	protected void UpdateClock()
	{
		m_fClockCarry += TICK_SECONDS;
		if (m_fClockCarry < 1)
			return;

		m_fClockCarry -= 1;
		if (m_iSecondsLeft > 0)
			m_iSecondsLeft--;

		if (m_iSecondsLeft <= 0 && m_iState == CRF_EEvacuationState.EXTRACTING)
			EndRound(false, "OUT OF TIME", string.Format("The supplies got out, but only %1 of the %2 %3 needed reached the extraction point in time.", m_iWithdrawn, m_iWithdrawNeeded, CRF_SpecialsHelper.GetFactionName(m_sEvacuatingFaction)));
		else if (m_iSecondsLeft <= 0)
			EndRound(false, "OUT OF TIME", string.Format("%1 got %2 of the %3 loads out before the clock ran out.", CRF_SpecialsHelper.GetFactionName(m_sEvacuatingFaction), m_iDelivered, m_iLoadsToWin));
	}

	//! Only send when something a client shows has changed
	protected void ReplicateIfChanged()
	{
		string snapshot = string.Format("%1 %2 %3 %4 %5 %6 %7 %8 %9", m_iState, m_iStock, m_iInTransit, m_iDelivered, m_iLost, m_iSecondsLeft, m_iCapturePercent, m_iLoadPercent, m_iUnloadPercent);
		snapshot += string.Format(" %1 %2", m_iWithdrawn, m_iWithdrawNeeded);
		if (snapshot == m_sLastReplicated)
			return;

		m_sLastReplicated = snapshot;
		Replication.BumpMe();
	}

	protected void EndRound(bool evacuatorsWon, string title, string text)
	{
		if (m_bEndRequested)
			return;

		m_bEndRequested = true;
		GetGame().GetCallqueue().Remove(Tick);

		m_iState = CRF_EEvacuationState.ENDED;
		m_bEvacuatorsWon = evacuatorsWon;
		m_iLoadPercent = -1;
		m_iUnloadPercent = -1;
		Replication.BumpMe();

		CRF_RadioTransmissionEvents.PopupToFaction("", title, text, 10);

		if (!m_bEndMissionOnVictory)
			return;

		string winner = m_sAttackingFaction;
		if (evacuatorsWon)
			winner = m_sEvacuatingFaction;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(winner, "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//===================================================================================
	// EVERY MACHINE: MAP AND HUD
	//===================================================================================

	protected void UpdateMarkers()
	{
		if (!m_Markers)
			return;

		BaseWorld world = GetGame().GetWorld();
		IEntity site = world.FindEntityByName(m_sSiteName);
		IEntity bay = world.FindEntityByName(m_sLoadingBayName);
		IEntity extraction = world.FindEntityByName(m_sExtractionName);

		m_Markers.Begin();

		if (site)
		{
			int siteColor = ARGB(255, 255, 165, 0);
			if (m_iState == CRF_EEvacuationState.CAPTURING)
				siteColor = ARGB(255, 225, 25, 0);
			m_Markers.Want(Flat(site.GetOrigin()), "Evacuation site", m_sMarkerIcon, siteColor);
		}

		// The bay matters to the side loading at it
		if (bay && CRF_SpecialsHelper.GetLocalFactionKey() == m_sEvacuatingFaction)
			m_Markers.Want(Flat(bay.GetOrigin()), "Loading bay", m_sLoadingMarkerIcon, ARGB(255, 255, 255, 255));

		if (extraction)
			m_Markers.Want(Flat(extraction.GetOrigin()), "Extraction", m_sMarkerIcon, ARGB(255, 60, 200, 60));

		m_Markers.Apply();
	}

	protected vector Flat(vector position)
	{
		return Vector(Math.Round(position[0]), 0, Math.Round(position[2]));
	}

	protected void UpdateHud()
	{
		if (m_Hud)
			m_Hud.Update(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Any machine (replicated)
	CRF_EEvacuationState GetEvacuationState()
	{
		return m_iState;
	}

	int GetStock()
	{
		return m_iStock;
	}

	int GetInTransit()
	{
		return m_iInTransit;
	}

	int GetDelivered()
	{
		return m_iDelivered;
	}

	int GetLost()
	{
		return m_iLost;
	}

	//! Evacuators who reached the extraction point after the loads, and how many have to
	int GetWithdrawn()
	{
		return m_iWithdrawn;
	}

	int GetWithdrawNeeded()
	{
		return m_iWithdrawNeeded;
	}

	int GetLoadsToWin()
	{
		return m_iLoadsToWin;
	}

	int GetSecondsLeft()
	{
		return m_iSecondsLeft;
	}

	//! 0-100, how far the attackers are to taking the site
	int GetCapturePercent()
	{
		return m_iCapturePercent;
	}

	//! 0-100, the truck loading in the bay; -1 if none is
	int GetLoadPercent()
	{
		return m_iLoadPercent;
	}

	//! 0-100, the furthest-along truck unloading at the extraction point; -1 if none is
	int GetUnloadPercent()
	{
		return m_iUnloadPercent;
	}

	bool DidEvacuatorsWin()
	{
		return m_bEvacuatorsWon;
	}

	string GetEvacuatingFactionKey()
	{
		return m_sEvacuatingFaction;
	}

	string GetAttackingFactionKey()
	{
		return m_sAttackingFaction;
	}

	string GetExtractionName()
	{
		return m_sExtractionName;
	}

	string GetSiteName()
	{
		return m_sSiteName;
	}

	//------------------------------------------------------------------------------------------------
	//! Pause menu mission info (CRF_MissionInfo hook), from replicated values
	override void CRF_AppendMissionInfo(notnull array<string> lines)
	{
		if (m_iState == CRF_EEvacuationState.WAITING)
		{
			lines.Insert("Evacuation: starts when the round goes live");
			return;
		}

		if (m_iState == CRF_EEvacuationState.ENDED)
		{
			lines.Insert(string.Format("Evacuation: round over, %1 of %2 loads out", m_iDelivered, m_iLoadsToWin));
			return;
		}

		if (m_iState == CRF_EEvacuationState.EXTRACTING)
			lines.Insert(string.Format("Evacuation: the loads are out; %1 of %2 needed have reached the extraction point", m_iWithdrawn, m_iWithdrawNeeded));
		else
			lines.Insert(string.Format("Evacuation: %1 of %2 loads out, %3 on the road, %4 at the site, %5 lost", m_iDelivered, m_iLoadsToWin, m_iInTransit, m_iStock, m_iLost));
		lines.Insert(string.Format("Evacuation: %1 left", SCR_FormatHelper.FormatTime(m_iSecondsLeft)));
	}
}
