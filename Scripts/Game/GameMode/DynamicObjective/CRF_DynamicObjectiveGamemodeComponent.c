//------------------------------------------------------------------------------------
// CRF_DynamicObjectiveGamemodeComponent: one objective that keeps moving
//
// One site is live at a time, picked at random from the sites the mission maker placed. A faction holds
// it while it has living players inside the radius and no other competing faction does - both there
// means it's contested. The holder scores a point every second.
//
//   Securing: a faction that has held the site for Secure Seconds in total takes the bonus, and the
//             objective moves on.
//   Moving:   a site nobody secures moves on by itself after Site Minutes, with a warning first.
//   The next site is marked straight away but only goes live Relocate Seconds later, so both sides
//   can race for it.
//
//   Victory: first to Score To Win. At the time limit the leader wins; a tie plays on to the
//   Lobby's own time limit.
//
// Sites are empty named entities in the world. Only their position on the map matters (distances
// ignore height); the mode spawns Site Prefab on the ground at the live site.
//
// Setup for mission makers
// 1. Add this component to the gamemode entity; list the competing factions.
// 2. Place and name the sites, and list the names (and, in the same order, what to call them).
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Dynamic objective: one site at a time scores for whoever holds it alone, and it moves when secured or after a while.")]
class CRF_DynamicObjectiveGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_DynamicObjectiveGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("", UIWidgets.Auto, "Factions competing for the objective (two or more). Players of other factions neither hold nor contest it.", category: "Dynamic Objective - Factions")]
	protected ref array<string> m_aFactions;

	[Attribute("", UIWidgets.Auto, "Names of the site entities placed in the world.", category: "Dynamic Objective - Sites")]
	protected ref array<string> m_aSiteNames;

	[Attribute("", UIWidgets.Auto, "What to call each site, in the same order as the names. Missing ones are \"Site 1\", \"Site 2\"...", category: "Dynamic Objective - Sites")]
	protected ref array<string> m_aSiteLabels;

	[Attribute("40", UIWidgets.Auto, "Metres (on the map, ignoring height) from the site a player has to be to hold it.", category: "Dynamic Objective - Sites")]
	protected float m_fRadius;

	[Attribute("{8B4B8BF44162B24E}Prefabs/Compositions/Slotted/SlotFlatSmall/Antenna_S_US_01.et", UIWidgets.ResourceNamePicker, "Prop spawned on the ground at the site, removed when the objective moves on. Empty = none.", params: "et", category: "Dynamic Objective - Sites")]
	protected ResourceName m_sSitePrefab;

	[Attribute("8", UIWidgets.Auto, "Minutes a site stays live when nobody secures it.", category: "Dynamic Objective - Timing")]
	protected float m_fSiteMinutes;

	[Attribute("60", UIWidgets.Auto, "Seconds of warning before a site nobody secured moves on.", category: "Dynamic Objective - Timing")]
	protected int m_iMoveWarningSeconds;

	[Attribute("45", UIWidgets.Auto, "Seconds between a site moving on and the next one going live. 0 = straight away.", category: "Dynamic Objective - Timing")]
	protected int m_iRelocateSeconds;

	[Attribute("150", UIWidgets.Auto, "Seconds a faction has to hold a site in total to secure it.", category: "Dynamic Objective - Scoring")]
	protected int m_iSecureSeconds;

	[Attribute("60", UIWidgets.Auto, "Bonus points for securing a site.", category: "Dynamic Objective - Scoring")]
	protected int m_iSecureBonus;

	[Attribute("600", UIWidgets.Auto, "Points a faction needs to win.", category: "Dynamic Objective - Victory")]
	protected int m_iScoreToWin;

	[Attribute("55", UIWidgets.Auto, "Minutes into the round the leader wins on points. 0 = no time limit.", category: "Dynamic Objective - Victory")]
	protected int m_iTimeLimitMinutes;

	[Attribute("1", UIWidgets.CheckBox, "Record the winner and advance the gamemode to the AAR when the round ends.", category: "Dynamic Objective - Victory")]
	protected bool m_bEndMissionOnVictory;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourcePickerThumbnail, "Map marker icon for the site.", params: "edds", category: "Dynamic Objective - Map")]
	protected ResourceName m_sMarkerIcon;

	//===================================================================================
	// STATE
	//===================================================================================

	[RplProp()]
	protected bool m_bRoundLive;

	//! Current site (live, or on its way), -1 before the sites are found
	[RplProp()]
	protected int m_iSite = -1;

	//! The site scores (false while the next one is on its way)
	[RplProp()]
	protected bool m_bSiteLive;

	//! Index into m_aFactions, NOBODY (-1) or CONTESTED (-2)
	[RplProp()]
	protected int m_iHolder = -1;

	//! Seconds until the site moves on, or until the next one goes live
	[RplProp()]
	protected int m_iSecondsLeft;

	//! Per faction
	[RplProp()]
	protected ref array<int> m_aScores = {};

	//! Per faction: seconds held on the current site
	[RplProp()]
	protected ref array<int> m_aHeld = {};

	// Server
	protected bool m_bEnded;
	protected float m_fElapsedSeconds;
	protected ref array<IEntity> m_aSites = {};
	protected IEntity m_SiteProp;
	protected int m_iLastAnnouncedHolder = -1;
	protected float m_fLastHolderAnnouncement = -1000;
	protected ref array<int> m_aMilestones = {};		//!< per faction: quarters of Score To Win announced
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Client
	protected ref CRF_SpecialsMarkerSet m_Markers = new CRF_SpecialsMarkerSet();

	protected static CRF_DynamicObjectiveGamemodeComponent s_Instance;

	protected static const string LOG = "[CRF_DynamicObjective]";
	protected static const int NOBODY = -1;
	protected static const int CONTESTED = -2;
	protected static const float HOLDER_ANNOUNCE_COOLDOWN = 20;

	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_DynamicObjectiveGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_DynamicObjectiveGamemodeComponent GetInstance()
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

		// Markers on every machine with a player
		if (!System.IsConsoleApp())
			GetGame().GetCallqueue().CallLater(ClientUpdate, 1000, true);

		if (!Replication.IsServer())
			return;

		if (m_aSiteNames)
		{
			foreach (string siteName : m_aSiteNames)
			{
				IEntity site = CRF_SpecialsHelper.FindNamedEntity(siteName, LOG);
				if (site)
					m_aSites.Insert(site);
			}
		}

		if (m_aSites.IsEmpty())
		{
			Print(LOG + " No sites - list the site entities in Site Names.", LogLevel.ERROR);
			return;
		}

		if (!m_aFactions || m_aFactions.Count() < 2)
			Print(LOG + " Fewer than two factions - list the competing factions.", LogLevel.ERROR);

		if (m_aFactions)
		{
			for (int i = 0; i < m_aFactions.Count(); i++)
			{
				m_aScores.Insert(0);
				m_aHeld.Insert(0);
				m_aMilestones.Insert(0);
			}
		}

		// The first site is marked during safestart so both sides can plan for it
		m_iSite = Math.RandomInt(0, m_aSites.Count());
		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(SpawnSiteProp, 2000, false);

		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafestartManager = COA_SafestartManager.GetInstance();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChanged);

		// Fallback for missions without safestart
		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Insert(OnPlayerSpawned);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(ClientUpdate);
			GetGame().GetCallqueue().Remove(SpawnSiteProp);
		}

		m_Markers.Clear();

		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
			m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChanged);

		if (m_Gamemode && m_Gamemode.GetOnStateChanged())
			m_Gamemode.GetOnStateChanged().Remove(OnGamemodeStateChanged);

		if (m_Gamemode)
			m_Gamemode.GetOnPlayerSpawned().Remove(OnPlayerSpawned);

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the prop for the current site, replacing the last one
	protected void SpawnSiteProp()
	{
		if (m_SiteProp)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_SiteProp);
			m_SiteProp = null;
		}

		IEntity site = GetSite(m_iSite);
		if (site)
			m_SiteProp = CRF_SpecialsHelper.SpawnOnGround(m_sSitePrefab, site.GetOrigin(), site.GetYawPitchRoll()[0]);
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
		if (m_bRoundLive || m_bEnded || m_aSites.IsEmpty())
			return;

		m_bRoundLive = true;
		GoLive();
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			SendBriefing(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendBriefing(int playerId)
	{
		if (GetFactionIndex(CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId)) < 0)
			return;

		CRF_RadioTransmissionEvents.PopupToPlayer(playerId, "Hold the objective",
			string.Format("Be the only side at %1 to score. Hold it for %2 to secure it. It moves - first to %3 points wins.",
				GetSiteLabel(m_iSite), CRF_SpecialsHelper.FormatSeconds(m_iSecureSeconds), m_iScoreToWin), 12);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		if (m_bRoundLive && !m_bEnded)
			SendBriefing(playerId);
	}

	//===================================================================================
	// TICK (1 s, server)
	//===================================================================================

	protected void Tick()
	{
		if (m_bEnded)
			return;

		m_fElapsedSeconds += 1;

		if (m_bSiteLive)
			UpdateSite();
		else
			UpdateRelocation();

		Replication.BumpMe();

		if (!m_bEnded && m_iTimeLimitMinutes > 0 && m_fElapsedSeconds >= m_iTimeLimitMinutes * 60)
			EndOnPoints();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateSite()
	{
		int holder = FindHolder();
		if (holder != m_iHolder)
		{
			m_iHolder = holder;
			AnnounceHolder();
		}

		if (m_iHolder >= 0)
		{
			m_aScores[m_iHolder] = m_aScores[m_iHolder] + 1;
			m_aHeld[m_iHolder] = m_aHeld[m_iHolder] + 1;
			AnnounceMilestone(m_iHolder);

			if (m_aScores[m_iHolder] >= m_iScoreToWin)
			{
				Victory(m_iHolder, string.Format("%1 reached %2 points.", GetFactionName(m_iHolder), m_iScoreToWin));
				return;
			}

			if (m_aHeld[m_iHolder] >= m_iSecureSeconds)
			{
				Secure(m_iHolder);
				return;
			}
		}

		m_iSecondsLeft--;
		if (m_iSecondsLeft == m_iMoveWarningSeconds)
		{
			CRF_RadioTransmissionEvents.PopupToFaction("", GetSiteLabel(m_iSite) + " is moving",
				string.Format("Nobody has secured it. It moves in %1.", CRF_SpecialsHelper.FormatSeconds(m_iMoveWarningSeconds)), 6);
		}

		if (m_iSecondsLeft <= 0)
			MoveOn(GetSiteLabel(m_iSite) + " has moved", "Nobody secured it.");
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRelocation()
	{
		m_iSecondsLeft--;
		if (m_iSecondsLeft <= 0)
			GoLive();
	}

	//------------------------------------------------------------------------------------------------
	//! The one competing faction with living players at the site, NOBODY or CONTESTED
	protected int FindHolder()
	{
		IEntity site = GetSite(m_iSite);
		if (!site || !m_aFactions)
			return NOBODY;

		int holder = NOBODY;
		array<int> players = {};
		foreach (int i, string factionKey : m_aFactions)
		{
			CRF_SpecialsHelper.GetAlivePlayers(factionKey, players);
			if (CountNear(site.GetOrigin(), m_fRadius, players) == 0)
				continue;

			if (holder != NOBODY)
				return CONTESTED;

			holder = i;
		}

		return holder;
	}

	//------------------------------------------------------------------------------------------------
	//! "X holds the site" / "contested", now and then rather than every time someone steps in or out
	protected void AnnounceHolder()
	{
		if (m_iHolder == NOBODY || m_iHolder == m_iLastAnnouncedHolder)
			return;

		if (m_fElapsedSeconds - m_fLastHolderAnnouncement < HOLDER_ANNOUNCE_COOLDOWN)
			return;

		m_iLastAnnouncedHolder = m_iHolder;
		m_fLastHolderAnnouncement = m_fElapsedSeconds;

		if (m_iHolder == CONTESTED)
			CRF_RadioTransmissionEvents.PopupToFaction("", GetSiteLabel(m_iSite) + " is contested", "Nobody scores while both sides are there.", 4);
		else
			CRF_RadioTransmissionEvents.PopupToFaction("", GetFactionName(m_iHolder) + " holds " + GetSiteLabel(m_iSite), GetScoreLine(), 4);
	}

	//------------------------------------------------------------------------------------------------
	//! A quarter, half and three quarters of the way to winning, once each
	protected void AnnounceMilestone(int faction)
	{
		int quarter = m_aScores[faction] * 4 / Math.Max(1, m_iScoreToWin);
		if (quarter <= m_aMilestones[faction] || quarter >= 4)
			return;

		m_aMilestones[faction] = quarter;
		CRF_RadioTransmissionEvents.PopupToFaction("", string.Format("%1 is %2%% of the way", GetFactionName(faction), quarter * 25), GetScoreLine(), 6);
	}

	//------------------------------------------------------------------------------------------------
	protected void Secure(int faction)
	{
		m_aScores[faction] = m_aScores[faction] + m_iSecureBonus;
		string label = GetSiteLabel(m_iSite);

		if (m_aScores[faction] >= m_iScoreToWin)
		{
			Victory(faction, string.Format("%1 secured %2 and reached %3 points.", GetFactionName(faction), label, m_iScoreToWin));
			return;
		}

		MoveOn(GetFactionName(faction) + " secured " + label, string.Format("+%1 points.", m_iSecureBonus));
	}

	//------------------------------------------------------------------------------------------------
	//! The objective moves to another site: marked now, live after Relocate Seconds
	protected void MoveOn(string title, string reason)
	{
		int previous = m_iSite;
		if (m_aSites.Count() > 1)
		{
			while (m_iSite == previous)
				m_iSite = Math.RandomInt(0, m_aSites.Count());
		}

		for (int i = 0; i < m_aHeld.Count(); i++)
			m_aHeld[i] = 0;

		m_iHolder = NOBODY;
		m_iLastAnnouncedHolder = NOBODY;
		m_bSiteLive = false;
		m_iSecondsLeft = m_iRelocateSeconds;
		SpawnSiteProp();

		IEntity site = GetSite(m_iSite);
		string where = "";
		if (site)
			where = ", grid " + CRF_RadioTransmissionEvents.GetGrid100(site.GetOrigin());

		CRF_RadioTransmissionEvents.PopupToFaction("", title,
			string.Format("%1 Next: %2%3, live in %4. %5", reason, GetSiteLabel(m_iSite), where, CRF_SpecialsHelper.FormatSeconds(m_iRelocateSeconds), GetScoreLine()), 10);

		if (m_iRelocateSeconds <= 0)
			GoLive();
	}

	//------------------------------------------------------------------------------------------------
	protected void GoLive()
	{
		m_bSiteLive = true;
		m_iSecondsLeft = (int)Math.Max(1, m_fSiteMinutes * 60);
		Replication.BumpMe();

		IEntity site = GetSite(m_iSite);
		string where = "";
		if (site)
			where = "Grid " + CRF_RadioTransmissionEvents.GetGrid100(site.GetOrigin()) + ". ";

		CRF_RadioTransmissionEvents.PopupToFaction("", GetSiteLabel(m_iSite) + " is live",
			string.Format("%1Hold it alone to score.", where), 6);
	}

	//===================================================================================
	// VICTORY
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	protected void EndOnPoints()
	{
		int leader = NOBODY;
		bool tied;
		foreach (int i, int score : m_aScores)
		{
			if (leader == NOBODY || score > m_aScores[leader])
			{
				leader = i;
				tied = false;
			}
			else if (score == m_aScores[leader])
			{
				tied = true;
			}
		}

		if (leader == NOBODY)
			return;

		// A tie goes to sudden death: the next point wins (or the Lobby's own time limit ends it)
		if (tied)
		{
			m_iTimeLimitMinutes = 0;
			m_iScoreToWin = Math.Min(m_iScoreToWin, m_aScores[leader] + 1);
			CRF_RadioTransmissionEvents.PopupToFaction("", "Time is up - it's a tie", "Sudden death: the next point wins.", 8);
			return;
		}

		Victory(leader, "Time is up. " + GetScoreLine());
	}

	//------------------------------------------------------------------------------------------------
	protected void Victory(int faction, string reason)
	{
		if (m_bEnded)
			return;

		m_bEnded = true;
		m_bRoundLive = false;
		m_bSiteLive = false;
		Replication.BumpMe();
		GetGame().GetCallqueue().Remove(Tick);

		string winner = GetFactionName(faction);
		winner.ToUpper();
		CRF_RadioTransmissionEvents.PopupToFaction("", winner + " WINS", reason, 12);

		if (!m_bEndMissionOnVictory)
			return;

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(m_aFactions[faction], "automatic");

		if (m_Gamemode)
			m_Gamemode.AdvanceGamemodeState(true);
	}

	//===================================================================================
	// CLIENT (1 s): the site on the map
	//===================================================================================

	protected void ClientUpdate()
	{
		m_Markers.Begin();

		string localFaction = CRF_SpecialsHelper.GetLocalFactionKey();
		IEntity site = GetGame().GetWorld().FindEntityByName(GetSiteName(m_iSite));
		if (site && !m_bEnded && !localFaction.IsEmpty())
			m_Markers.Want(site.GetOrigin(), GetSiteLabel(m_iSite) + " - " + GetStateText(), m_sMarkerIcon, GetStateColor(localFaction));

		m_Markers.Apply();
	}

	//------------------------------------------------------------------------------------------------
	//! Changes about once a minute rather than every second, so the marker isn't redrawn constantly
	protected string GetStateText()
	{
		if (!m_bRoundLive)
			return "first objective";

		if (!m_bSiteLive)
			return "live in " + CRF_SpecialsHelper.FormatSeconds(m_iSecondsLeft);

		string holder = "neutral";
		if (m_iHolder == CONTESTED)
			holder = "contested";
		else if (m_iHolder >= 0)
			holder = "held by " + GetFactionName(m_iHolder);

		int minutes = (int)Math.Ceil(m_iSecondsLeft / 60.0);
		return string.Format("%1 - moves in %2 min", holder, minutes);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetStateColor(string localFaction)
	{
		if (m_iHolder == CONTESTED)
			return ARGB(255, 230, 140, 40);

		if (m_iHolder >= 0)
		{
			if (m_aFactions[m_iHolder] == localFaction)
				return ARGB(255, 80, 200, 120);

			return ARGB(255, 220, 60, 60);
		}

		return ARGB(255, 235, 235, 235);
	}

	//===================================================================================
	// HELPERS
	//===================================================================================

	protected IEntity GetSite(int index)
	{
		if (!m_aSites.IsIndexValid(index))
			return null;

		return m_aSites[index];
	}

	//------------------------------------------------------------------------------------------------
	//! By name, so clients (which don't keep m_aSites) find it too. Sites missing from the world were
	//! dropped on the server, so the index counts only the ones found - the same on every machine.
	protected string GetSiteName(int index)
	{
		if (!m_aSiteNames)
			return "";

		int found = -1;
		foreach (string siteName : m_aSiteNames)
		{
			if (!GetGame().GetWorld().FindEntityByName(siteName))
				continue;

			found++;
			if (found == index)
				return siteName;
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	protected string GetSiteLabel(int index)
	{
		string siteName = GetSiteName(index);
		if (m_aSiteLabels && m_aSiteNames)
		{
			int nameIndex = m_aSiteNames.Find(siteName);
			if (m_aSiteLabels.IsIndexValid(nameIndex) && !m_aSiteLabels[nameIndex].IsEmpty())
				return m_aSiteLabels[nameIndex];
		}

		return string.Format("Site %1", index + 1);
	}

	//------------------------------------------------------------------------------------------------
	protected int GetFactionIndex(string factionKey)
	{
		if (!m_aFactions || factionKey.IsEmpty())
			return -1;

		return m_aFactions.Find(factionKey);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetFactionName(int index)
	{
		if (!m_aFactions || !m_aFactions.IsIndexValid(index))
			return "";

		return CRF_SpecialsHelper.GetFactionName(m_aFactions[index]);
	}

	//------------------------------------------------------------------------------------------------
	//! "BLUFOR 120 - OPFOR 95"
	protected string GetScoreLine()
	{
		array<string> parts = {};
		foreach (int i, int score : m_aScores)
			parts.Insert(string.Format("%1 %2", GetFactionName(i), score));

		return SCR_StringHelper.Join(" - ", parts);
	}

	//------------------------------------------------------------------------------------------------
	protected int CountNear(vector position, float radius, array<int> playerIds)
	{
		int count = 0;
		foreach (int playerId : playerIds)
		{
			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (entity && vector.DistanceXZ(entity.GetOrigin(), position) <= radius)
				count++;
		}
		return count;
	}
}
