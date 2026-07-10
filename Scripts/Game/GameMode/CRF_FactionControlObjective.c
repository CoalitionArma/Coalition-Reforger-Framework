// CRF_FactionControlObjective.c
//
// Place this ScriptComponent on a GenericEntity prefab in the mission world.
// Unlike CRF_AreaTimerComponent (which races a single countdown to zero),
// this component tracks the *state* of control over an area and fires a
// message at each meaningful transition:
//
//   1. STARTED_CAPTURING - a faction achieves majority presence for the first time
//   2. HALFWAY           - the controlling faction has held it for half the timer
//   3. CONTESTED         - multiple factions are present with no clear majority
//   4. WON               - the controlling faction held it for the full duration
//
// All client replication is routed through CRF_RplBroadcastManager, same as
// CRF_AreaTimerComponent, so no replicated entity is required.
//
// SETUP:
//   1. In Workbench, open or create a GenericEntity prefab.
//   2. Add CRF_FactionControlObjective to it.
//   3. Configure the attributes (ZoneLabel, CaptureRadius, CountdownSeconds, …).
//   4. Place the prefab in your mission and position it on the objective.

enum CRF_EFactionControlState
{
	IDLE       = 0,	// No faction has majority presence (empty or below minimum)
	CONTROLLED = 1,	// One faction holds majority and is counting down
	CONTESTED  = 2,	// Multiple factions present, no faction has majority
	WON        = 3	// Countdown reached zero - controlling faction won the point
}

enum CRF_EFactionControlEvent
{
	STARTED_CAPTURING = 0,
	HALFWAY           = 1,
	CONTESTED         = 2,
	WON               = 3
}

[ComponentEditorProps(category: "CRF | Modular", description: "Faction control objective. Tracks majority control of an area and broadcasts messages when capture starts, reaches halfway, becomes contested, or is won.")]
class CRF_FactionControlObjectiveClass : ScriptComponentClass {}

class CRF_FactionControlObjective : ScriptComponent
{
	//---------------------------------------------------------------------------------------------
	// Editor Attributes
	//---------------------------------------------------------------------------------------------

	[Attribute("Objective Alpha", UIWidgets.EditBox, "Display name shown in HUD/notification messages.", category: "Faction Control")]
	string m_sZoneLabel;

	[Attribute("50", UIWidgets.EditBox, "Radius (metres) scanned for players each tick.", category: "Faction Control")]
	float m_fCaptureRadius;

	[Attribute("120", UIWidgets.EditBox, "Seconds the dominant faction must hold the area to win.", category: "Faction Control")]
	int m_iCountdownSeconds;

	[Attribute("1", UIWidgets.EditBox, "Minimum players of the dominant faction required to start capturing.", category: "Faction Control")]
	int m_iMinPlayersToCapture;

	[Attribute("1", UIWidgets.CheckBox, "Reset countdown to full when the controlling faction loses majority (including going contested). If false the countdown pauses instead.", category: "Faction Control")]
	bool m_bResetOnLostControl;

	[Attribute("1", UIWidgets.CheckBox, "Call CRF_LoggingManager.SetWinningFaction when the countdown completes.", category: "Faction Control")]
	bool m_bWinCondition;

	//---------------------------------------------------------------------------------------------
	// Server-side state  (not replicated – broadcast via CRF_RplBroadcastManager)
	//---------------------------------------------------------------------------------------------

	protected CRF_EFactionControlState m_eState;
	protected int    m_iCountdownRemaining;
	protected string m_sControllingFaction;
	protected bool   m_bHalfwaySent;

	//---------------------------------------------------------------------------------------------
	// Private
	//---------------------------------------------------------------------------------------------

	protected IEntity m_Owner;
	protected ref array<SCR_ChimeraCharacter> m_aPlayersInZone = new array<SCR_ChimeraCharacter>();

	//---------------------------------------------------------------------------------------------
	// Singleton
	//---------------------------------------------------------------------------------------------

	protected static CRF_FactionControlObjective m_sInstance;

	//------------------------------------------------------------------------------------------------
	void CRF_FactionControlObjective(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!m_sInstance)
			m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_FactionControlObjective()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_FactionControlObjective GetInstance()
	{
		return m_sInstance;
	}

	//---------------------------------------------------------------------------------------------
	// Lifecycle
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Owner = owner;
		m_eState              = CRF_EFactionControlState.IDLE;
		m_iCountdownRemaining = m_iCountdownSeconds;
		m_sControllingFaction = "";
		m_bHalfwaySent        = false;

		if (!Replication.IsServer())
			return;

		// Poll every second on the server.
		GetGame().GetCallqueue().CallLater(UpdateControlState, 1000, true);
	}

	//---------------------------------------------------------------------------------------------
	// Server-side Tick  (called via CallLater every 1000 ms)
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void UpdateControlState()
	{
		if (!m_Owner)
			return;

		// Gate: safestart active
		CRF_SafestartManager safestartManager = CRF_SafestartManager.GetInstance();
		if (!safestartManager || safestartManager.GetSafestartStatus())
			return;

		// Gate: game mode not running
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode || !gameMode.IsRunning())
			return;

		// Gate: already won
		if (m_eState == CRF_EFactionControlState.WON)
			return;

		//--- Collect alive, player-controlled characters inside the radius ---
		m_aPlayersInZone.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(
			m_Owner.GetOrigin(),
			m_fCaptureRadius,
			ProcessEntity,
			null,
			EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT
		);

		//--- Tally per faction ---
		array<string> factionKeys   = new array<string>();
		array<int>    factionCounts = new array<int>();
		int totalInZone = 0;

		foreach (SCR_ChimeraCharacter ch : m_aPlayersInZone)
		{
			string key = ch.GetFactionKey();
			if (key == "" || key == "SPEC")
				continue;

			int idx = factionKeys.Find(key);
			if (idx == -1)
			{
				factionKeys.Insert(key);
				factionCounts.Insert(1);
			}
			else
			{
				factionCounts[idx] = factionCounts[idx] + 1;
			}
			totalInZone = totalInZone + 1;
		}

		//--- Determine dominant faction ---
		string dominantFaction = "";
		int    dominantCount   = 0;
		bool   tied            = false;

		for (int i = 0; i < factionKeys.Count(); i++)
		{
			if (factionCounts[i] > dominantCount)
			{
				dominantFaction = factionKeys[i];
				dominantCount   = factionCounts[i];
				tied            = false;
			}
			else if (factionCounts[i] == dominantCount && dominantCount > 0)
			{
				tied = true;
			}
		}

		// Majority: one faction has more than all others combined, and meets the minimum.
		bool hasMajority = !tied
			&& dominantCount >= m_iMinPlayersToCapture
			&& dominantCount > (totalInZone - dominantCount);

		// Contested: more than one faction present, but nobody holds majority.
		bool isContested = !hasMajority && factionKeys.Count() > 1;

		//--- Branch on the new situation ---
		if (hasMajority)
			HandleMajority(dominantFaction);
		else if (isContested)
			HandleContested();
		else
			HandleIdle();
	}

	//---------------------------------------------------------------------------------------------
	// State handlers
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void HandleMajority(string dominantFaction)
	{
		bool isNewCapture = (m_eState != CRF_EFactionControlState.CONTROLLED) || (dominantFaction != m_sControllingFaction);

		if (isNewCapture)
		{
			m_sControllingFaction = dominantFaction;
			m_eState              = CRF_EFactionControlState.CONTROLLED;
			m_bHalfwaySent        = false;

			if (m_bResetOnLostControl || m_iCountdownRemaining <= 0)
				m_iCountdownRemaining = m_iCountdownSeconds;

			BroadcastEvent(CRF_EFactionControlEvent.STARTED_CAPTURING, m_sControllingFaction);
			BroadcastState();
			return;
		}

		// Same faction continuing to hold - tick down.
		m_iCountdownRemaining--;

		if (!m_bHalfwaySent && m_iCountdownRemaining <= (m_iCountdownSeconds / 2))
		{
			m_bHalfwaySent = true;
			BroadcastEvent(CRF_EFactionControlEvent.HALFWAY, m_sControllingFaction);
		}

		if (m_iCountdownRemaining <= 0)
		{
			m_iCountdownRemaining = 0;
			m_eState              = CRF_EFactionControlState.WON;
			BroadcastState();
			OnWin();
			return;
		}

		BroadcastState();
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleContested()
	{
		bool wasAlreadyContested = (m_eState == CRF_EFactionControlState.CONTESTED);

		m_eState = CRF_EFactionControlState.CONTESTED;

		if (m_bResetOnLostControl)
		{
			m_iCountdownRemaining = m_iCountdownSeconds;
			m_bHalfwaySent        = false;
		}

		if (!wasAlreadyContested)
			BroadcastEvent(CRF_EFactionControlEvent.CONTESTED, m_sControllingFaction);

		BroadcastState();
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleIdle()
	{
		if (m_eState == CRF_EFactionControlState.IDLE)
			return;

		m_eState = CRF_EFactionControlState.IDLE;

		if (m_bResetOnLostControl)
		{
			m_iCountdownRemaining = m_iCountdownSeconds;
			m_sControllingFaction = "";
			m_bHalfwaySent        = false;
		}

		BroadcastState();
	}

	//---------------------------------------------------------------------------------------------
	// Query callback – keeps only living, player-controlled characters
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected bool ProcessEntity(IEntity ent)
	{
		SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(ent);
		if (!ch)
			return true;

		// Must be player-controlled (not AI)
		if (!GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(ch))
			return true;

		// Skip dead players
		SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.Cast(ch.FindComponent(SCR_DamageManagerComponent));
		if (dmg && dmg.GetState() == EDamageState.DESTROYED)
			return true;

		m_aPlayersInZone.Insert(ch);
		return true;
	}

	//---------------------------------------------------------------------------------------------
	// Broadcast helpers
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BroadcastState()
	{
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.BroadcastFactionControlUpdate(m_eState, m_iCountdownRemaining, m_sControllingFaction, m_sZoneLabel);
	}

	//------------------------------------------------------------------------------------------------
	protected void BroadcastEvent(CRF_EFactionControlEvent evt, string faction)
	{
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.BroadcastFactionControlEvent(evt, faction, m_sZoneLabel);
	}

	//---------------------------------------------------------------------------------------------
	// Win Handling
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void OnWin()
	{
		// Stop the poll loop
		GetGame().GetCallqueue().Remove(UpdateControlState);

		BroadcastEvent(CRF_EFactionControlEvent.WON, m_sControllingFaction);

		// Optional game-level win condition
		if (m_bWinCondition)
		{
			CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
			if (loggingManager)
				loggingManager.SetWinningFaction(m_sControllingFaction, "automatic");
		}
	}
}