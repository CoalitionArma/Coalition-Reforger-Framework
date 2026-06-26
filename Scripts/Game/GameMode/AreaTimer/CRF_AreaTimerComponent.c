// CRF_AreaTimerComponent.c
//
// Place this ScriptComponent on a GenericEntity prefab in the mission world.
// The faction that maintains majority presence inside the capture radius will
// tick down a countdown; when it reaches zero they win (optionally triggering
// the game win condition).
//
// All client replication is routed through CRF_RplBroadcastManager so that
// this component works correctly in multiplayer without needing a replicated
// entity of its own.
//
// SETUP:
//   1. In Workbench, open or create a GenericEntity prefab.
//   2. Add CRF_AreaTimerComponent to it.
//   3. Configure the attributes (ZoneLabel, CaptureRadius, CountdownSeconds, …).
//   4. Place the prefab in your mission and position it on the objective.

enum CRF_EAreaTimerState
{
	IDLE      = 0,	// No faction has majority (or tie)
	CAPTURING = 1,	// One faction is counting down
	WIN       = 2	// Countdown reached zero – a winner was decided
}

[ComponentEditorProps(category: "CRF | Modular", description: "Area majority timer. Place on an entity in the world – the faction with majority alive presence counts down to win.")]
class CRF_AreaTimerComponentClass : ScriptComponentClass {}

class CRF_AreaTimerComponent : ScriptComponent
{
	//---------------------------------------------------------------------------------------------
	// Editor Attributes
	//---------------------------------------------------------------------------------------------

	[Attribute("Alpha Point", UIWidgets.EditBox, "Display name shown on the HUD countdown panel.", category: "Area Timer")]
	string m_sZoneLabel;

	[Attribute("50", UIWidgets.EditBox, "Radius (metres) scanned for players each tick.", category: "Area Timer")]
	float m_fCaptureRadius;

	[Attribute("120", UIWidgets.EditBox, "Seconds the dominant faction must hold the area to win.", category: "Area Timer")]
	int m_iCountdownSeconds;

	[Attribute("1", UIWidgets.EditBox, "Minimum players of the dominant faction required to start the countdown.", category: "Area Timer")]
	int m_iMinPlayersToCapture;

	[Attribute("1", UIWidgets.CheckBox, "Reset countdown to full when the controlling faction loses majority. If false the countdown pauses instead.", category: "Area Timer")]
	bool m_bResetOnLostControl;

	[Attribute("1", UIWidgets.CheckBox, "Call CRF_LoggingManager.SetWinningFaction when the countdown completes.", category: "Area Timer")]
	bool m_bWinCondition;

	//---------------------------------------------------------------------------------------------
	// Server-side state  (not replicated – broadcast via CRF_RplBroadcastManager)
	//---------------------------------------------------------------------------------------------

	protected CRF_EAreaTimerState m_eState;
	protected int    m_iCountdownRemaining;
	protected string m_sControllingFaction;

	//---------------------------------------------------------------------------------------------
	// Private
	//---------------------------------------------------------------------------------------------

	protected IEntity m_Owner;
	protected ref array<SCR_ChimeraCharacter> m_aPlayersInZone = new array<SCR_ChimeraCharacter>();

	//---------------------------------------------------------------------------------------------
	// Singleton
	//---------------------------------------------------------------------------------------------

	protected static CRF_AreaTimerComponent m_sInstance;

	//------------------------------------------------------------------------------------------------
	void CRF_AreaTimerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!m_sInstance)
			m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_AreaTimerComponent GetInstance()
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
		m_eState              = CRF_EAreaTimerState.IDLE;
		m_iCountdownRemaining = m_iCountdownSeconds;
		m_sControllingFaction = "";

		if (!Replication.IsServer())
			return;

		// Poll every second on the server.
		GetGame().GetCallqueue().CallLater(UpdateAreaTimer, 1000, true);
	}

	//---------------------------------------------------------------------------------------------
	// Server-side Tick  (called via CallLater every 1000 ms)
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void UpdateAreaTimer()
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
		if (m_eState == CRF_EAreaTimerState.WIN)
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
		bool   contested       = false;

		for (int i = 0; i < factionKeys.Count(); i++)
		{
			if (factionCounts[i] > dominantCount)
			{
				dominantFaction = factionKeys[i];
				dominantCount   = factionCounts[i];
				contested       = false;
			}
			else if (factionCounts[i] == dominantCount && dominantCount > 0)
			{
				contested = true;
			}
		}

		// Majority: one faction has more than all others combined, and meets the minimum.
		bool hasMajority = !contested
			&& dominantCount >= m_iMinPlayersToCapture
			&& dominantCount > (totalInZone - dominantCount);

		//--- Handle no-majority case ---
		if (!hasMajority)
		{
			if (m_eState == CRF_EAreaTimerState.CAPTURING)
			{
				m_eState = CRF_EAreaTimerState.IDLE;
				if (m_bResetOnLostControl)
				{
					m_iCountdownRemaining = m_iCountdownSeconds;
					m_sControllingFaction = "";
				}
			}
			BroadcastState();
			return;
		}

		//--- Faction change: new captor takes over ---
		if (dominantFaction != m_sControllingFaction)
		{
			m_sControllingFaction = dominantFaction;
			if (m_bResetOnLostControl)
				m_iCountdownRemaining = m_iCountdownSeconds;
		}

		m_eState = CRF_EAreaTimerState.CAPTURING;

		//--- Tick down ---
		m_iCountdownRemaining--;

		if (m_iCountdownRemaining <= 0)
		{
			m_iCountdownRemaining = 0;
			m_eState              = CRF_EAreaTimerState.WIN;
			BroadcastState();
			OnWin();
			return;
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
			bm.BroadcastAreaTimerUpdate(m_eState, m_iCountdownRemaining, m_sControllingFaction, m_sZoneLabel);
	}

	//---------------------------------------------------------------------------------------------
	// Win Handling
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void OnWin()
	{
		// Stop the poll loop
		GetGame().GetCallqueue().Remove(UpdateAreaTimer);

		// Notify all clients via the broadcast manager
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.BroadcastAreaTimerWin(m_sControllingFaction, m_sZoneLabel);

		// Optional game-level win condition
		if (m_bWinCondition)
		{
			CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
			if (loggingManager)
				loggingManager.SetWinningFaction(m_sControllingFaction, "automatic");
		}
	}
}
