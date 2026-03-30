// CRF_PropHunt.c
//
// Prop Hunt gamemode component for the Coalition Reforger Framework.
//
// Roles:
//   BLUFOR  = Props   — spawned first; must disguise themselves as world objects.
//   OPFOR   = Hunters — released after m_iGracePeriodSeconds; must eliminate all Props.
//
// Round flow (server-authoritative):
//   1. GAME state begins. Props get m_iGracePeriodSeconds to transform. Hunters are frozen.
//   2. After the grace period Hunters are released. Each shot a Hunter fires subtracts
//      m_iHunterShotPenalty from their CUSTOM penalty health bar (default 5 HP / shot).
//      Real bullet damage is NOT applied to the Hunter — only the penalty bar decreases.
//      When their penalty bar hits 0 they are killed and sent to spectator.
//      If a Hunter successfully eliminates a Prop their penalty bar resets to 100.
//   3. The round ends when EITHER:
//        a. All Props have been eliminated (Hunters win), or
//        b. The 10-minute hunt timer expires (Props win).
//   4. A brief inter-round pause (m_iInterRoundPauseSeconds) is shown on all clients, then
//      every slotted player is re-spawned and the next round starts.
//   5. After m_iTotalRounds rounds the game ends and the overall winner is announced via
//      popup.
//
// Hunter health HUD:
//   The current penalty health is shown to each Hunter via the in-game hint channel
//   (SCR_HintUIInfo / SendHint). The hint updates on every shot and on a prop kill.
//   No layout file is required — the hint panel is provided by vanilla Reforger UI.
//
// Setup for mission makers:
//   1. Add this component to your CRF_Gamemode entity.
//   2. Set BLUFOR faction key to the Props side and OPFOR to the Hunters side in the
//      gamemode faction settings (defaults: "BLUFOR" / "OPFOR").
//   3. Tune the attributes below as desired.

//--------------------------------------------------------------
// Phase enum — propagated to clients via RplProp so HUD/UIs
// can react to state changes.
//--------------------------------------------------------------
enum CRF_EPropHuntPhase
{
	WARMUP,      // Waiting for the round to begin (brief delay after respawn)
	GRACE,       // Props are hiding; Hunters are frozen
	HUNT,        // Active hunt phase
	ROUNDEND,    // Scoring / inter-round pause
	GAMEEND      // All rounds done
}

//--------------------------------------------------------------
// Class / component declaration
//--------------------------------------------------------------
class CRF_PropHuntGamemodeClass : SCR_BaseGameModeComponentClass {}

[ComponentEditorProps(category: "Game Mode Component", description: "Prop Hunt special gamemode. Props hide, Hunters seek. Five rounds, hunter fire costs HP.")]
class CRF_PropHuntGamemode : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------
	// Configurable attributes
	//------------------------------------------------------------

	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the Props team.")]
	string m_sPropsTeamKey;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the Hunters team.")]
	string m_sHuntersTeamKey;

	[Attribute("5", UIWidgets.EditBox, "Total number of rounds to play.")]
	int m_iTotalRounds;

	[Attribute("30", UIWidgets.EditBox, "Grace period in seconds during which Props hide and Hunters are frozen.")]
	int m_iGracePeriodSeconds;

	[Attribute("600", UIWidgets.EditBox, "Hunt phase time limit in seconds (default 600 = 10 minutes). Props win if they survive.")]
	int m_iHuntTimeLimitSeconds;

	[Attribute("100", UIWidgets.EditBox, "Starting penalty health for each Hunter. Displayed on their HUD. Not connected to real HP.")]
	int m_iHunterMaxHealth;

	[Attribute("5", UIWidgets.EditBox, "Penalty health subtracted from a Hunter for every shot they fire.")]
	int m_iHunterShotPenalty;

	[Attribute("10", UIWidgets.EditBox, "Seconds between round end and the next round starting.")]
	int m_iInterRoundPauseSeconds;

	[Attribute("3", UIWidgets.EditBox, "Warmup seconds before the grace period begins (gives clients time to load in).")]
	int m_iWarmupSeconds;

	//------------------------------------------------------------
	// Replicated state — clients read these to drive HUD display
	//------------------------------------------------------------

	[RplProp(onRplName: "OnPhaseChanged")]
	CRF_EPropHuntPhase m_ePhase = CRF_EPropHuntPhase.WARMUP;

	[RplProp()]
	int m_iCurrentRound = 1;

	[RplProp()]
	float m_fPhaseTimer = 0;   // counts DOWN; clients display this as time remaining

	[RplProp()]
	int m_iPropsWins   = 0;

	[RplProp()]
	int m_iHuntersWins = 0;

	//------------------------------------------------------------
	// Server-only runtime state
	//------------------------------------------------------------

	protected bool m_bPhaseTimerActive = false;

	// Tracks which props are still alive this round (player IDs)
	protected ref array<int> m_aAliveProps = {};

	// Tracks which hunters are alive this round (player IDs)
	protected ref array<int> m_aAliveHunters = {};

	// Custom penalty health bar for each Hunter. NOT tied to real character HP.
	// Server-side only; sent to each Hunter's client via SendHint whenever it changes.
	protected ref map<int, int> m_mHunterHealth = new map<int, int>();

	// Maps Prop player IDs to their spawned prop entity. Server-side only.
	// A player appears in this map only after they have successfully transformed.
	protected ref map<int, IEntity> m_mPlayerToPropEntity = new map<int, IEntity>();

	// Prevents re-entrant round-end logic
	protected bool m_bRoundEndPending = false;

	//------------------------------------------------------------
	// Client-side UI widgets — null on server, set only on the
	// local client that owns the feature.
	//------------------------------------------------------------
	// Full-screen blackout overlay shown to Hunters during grace.
	protected Widget m_wPropHuntBlackout;
	// Hunter penalty health bar.
	protected Widget m_wHunterHealthBar;
	// Layout resource for the hunter health bar HUD panel.
	protected static const ResourceName PH_HP_LAYOUT = "UI/Layouts/HUD/PropHunt/CRF_PropHuntHunterHealthBar.layout";

	// Singleton reference
	protected static CRF_PropHuntGamemode m_sInstance;

	//------------------------------------------------------------
	// Singleton accessor
	//------------------------------------------------------------
	static CRF_PropHuntGamemode GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------
	// Lifecycle
	//------------------------------------------------------------
	void ~CRF_PropHuntGamemode()
	{
		m_sInstance = null;
	}

	//------------------------------------------------------------
	// Component lifecycle
	//------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_sInstance = this;
		SetEventMask(owner, EntityEvent.FRAME);
	}

	//------------------------------------------------------------
	// EOnFrame — server-side timer tick
	//------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		if (!m_bPhaseTimerActive)
			return;

		m_fPhaseTimer -= timeSlice;

		// Replicate timer roughly once per second to save bandwidth
		int prevSec = Math.Floor(m_fPhaseTimer + timeSlice);
		int curSec  = Math.Floor(m_fPhaseTimer);
		if (curSec != prevSec)
			Replication.BumpMe();

		if (m_fPhaseTimer <= 0)
		{
			m_fPhaseTimer = 0;
			m_bPhaseTimerActive = false;
			Replication.BumpMe();
			OnPhaseTimerExpired();
		}
	}

	//------------------------------------------------------------
	// Phase timer expired handler
	//------------------------------------------------------------
	protected void OnPhaseTimerExpired()
	{
		switch (m_ePhase)
		{
			case CRF_EPropHuntPhase.WARMUP:
				StartGracePhase();
				break;

			case CRF_EPropHuntPhase.GRACE:
				StartHuntPhase();
				break;

			case CRF_EPropHuntPhase.HUNT:
				// Time ran out — Props survived; Props win this round
				EndRound(m_sPropsTeamKey);
				break;

			case CRF_EPropHuntPhase.ROUNDEND:
				StartNextRoundOrEndGame();
				break;
		}
	}

	//------------------------------------------------------------
	// Game start — called externally by OnGameModeStart or via
	// the existing CRF_Gamemode flow when this component is present
	//------------------------------------------------------------
	override void OnGameModeStart()
	{
		super.OnGameModeStart();

		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		m_iCurrentRound  = 1;
		m_iPropsWins     = 0;
		m_iHuntersWins   = 0;
		Replication.BumpMe();

		BeginRound();
	}

	//------------------------------------------------------------
	// Round initialisation — respawn all players, collect lists,
	// then start the warmup countdown
	//------------------------------------------------------------
	protected void BeginRound()
	{
		m_bRoundEndPending = false;

		// Delete any leftover prop entities from the previous round before respawning.
		CleanupPropEntities();

		m_aAliveProps.Clear();
		m_aAliveHunters.Clear();
		m_mHunterHealth.Clear();

		// Respawn every slotted player
		RespawnAllPlayers();

		// Collect player lists after spawns kick off
		// (give a short delay so controller assignments propagate)
		GetGame().GetCallqueue().CallLater(CollectPlayerLists, 1500, false);

		// Announce round start
		string msg = string.Format("--- Round %1 of %2 ---", m_iCurrentRound, m_iTotalRounds);
		BroadcastMessage(msg);

		// Begin warmup timer
		SetPhase(CRF_EPropHuntPhase.WARMUP, m_iWarmupSeconds);
	}

	//------------------------------------------------------------
	// Collect which players belong to each team at round start
	//------------------------------------------------------------
	protected void CollectPlayerLists()
	{
		m_aAliveProps.Clear();
		m_aAliveHunters.Clear();

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		array<int> playerIds = {};
		pm.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity character = pm.GetPlayerControlledEntity(playerId);
			if (!character)
				continue;

			FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(character.FindComponent(FactionAffiliationComponent));
			if (!facComp || !facComp.GetAffiliatedFaction())
				continue;

			string factionKey = facComp.GetAffiliatedFaction().GetFactionKey();

			if (factionKey == m_sPropsTeamKey)
			{
				m_aAliveProps.Insert(playerId);
			}
			else if (factionKey == m_sHuntersTeamKey)
			{
				m_aAliveHunters.Insert(playerId);
				m_mHunterHealth.Set(playerId, m_iHunterMaxHealth);
			}
		}
	}

	//------------------------------------------------------------
	// Phase helpers
	//------------------------------------------------------------
	protected void SetPhase(CRF_EPropHuntPhase phase, float durationSeconds)
	{
		m_ePhase            = phase;
		m_fPhaseTimer       = durationSeconds;
		m_bPhaseTimerActive = (durationSeconds > 0);
		Replication.BumpMe();
	}

	protected void StartGracePhase()
	{
		SetPhase(CRF_EPropHuntPhase.GRACE, m_iGracePeriodSeconds);
		BroadcastMessage(string.Format("PROPS: You have %1 seconds to hide! Press [F] near any object to disguise yourself. Hunters — stand by.", m_iGracePeriodSeconds));

		// Lock Hunters during grace period and black out their screens so they
		// cannot see where Props are hiding.
		foreach (int hunterId : m_aAliveHunters)
		{
			SetHunterFrozen(hunterId, true);
			BlackoutHunter(hunterId, true);
		}

		// Enable the F-key transform prompt for all Prop players.
		foreach (int propId : m_aAliveProps)
			EnablePropTransform(propId, true);
	}

	protected void StartHuntPhase()
	{
		SetPhase(CRF_EPropHuntPhase.HUNT, m_iHuntTimeLimitSeconds);
		BroadcastMessage("HUNTERS: Begin! Every shot costs HP. Kill a prop to restore to 100. Reach 0 and you're out!");

		// Unlock Hunters, lift their screen blackout, and show their health bar
		foreach (int hunterId : m_aAliveHunters)
		{
			BlackoutHunter(hunterId, false);
			SetHunterFrozen(hunterId, false);
			SendHunterHealthHint(hunterId, m_iHunterMaxHealth, false);
		}

		// Disable the transform input for all Props (whether transformed or not).
		foreach (int propId : m_aAliveProps)
			EnablePropTransform(propId, false);
	}

	//------------------------------------------------------------
	// Round end logic
	//------------------------------------------------------------
	// \param[in] winningFactionKey faction key of the winners,
	//            empty string means the round was declared a draw.
	protected void EndRound(string winningFactionKey)
	{
		if (m_bRoundEndPending)
			return;

		m_bRoundEndPending = true;
		m_bPhaseTimerActive = false;

		// Award win
		if (winningFactionKey == m_sPropsTeamKey)
		{
			m_iPropsWins++;
			BroadcastMessage("PROPS WIN this round!");
		}
		else if (winningFactionKey == m_sHuntersTeamKey)
		{
			m_iHuntersWins++;
			BroadcastMessage("HUNTERS WIN this round!");
		}
		else
		{
			BroadcastMessage("Round ended as a draw.");
		}

		Replication.BumpMe();

		// Remove the health bar HUD from all surviving hunters' screens.
		foreach (int hunterId : m_aAliveHunters)
			HideHunterHealthBar(hunterId);

		// Move to inter-round pause
		SetPhase(CRF_EPropHuntPhase.ROUNDEND, m_iInterRoundPauseSeconds);
	}

	//------------------------------------------------------------
	// After inter-round pause: advance round counter or end game
	//------------------------------------------------------------
	protected void StartNextRoundOrEndGame()
	{
		if (m_iCurrentRound >= m_iTotalRounds)
		{
			EndGame();
			return;
		}

		m_iCurrentRound++;
		Replication.BumpMe();
		BeginRound();
	}

	//------------------------------------------------------------
	// Final game end
	//------------------------------------------------------------
	protected void EndGame()
	{
		SetPhase(CRF_EPropHuntPhase.GAMEEND, 0);

		string winner;
		if (m_iPropsWins > m_iHuntersWins)
			winner = string.Format("PROPS win the match! (%1 - %2)", m_iPropsWins, m_iHuntersWins);
		else if (m_iHuntersWins > m_iPropsWins)
			winner = string.Format("HUNTERS win the match! (%1 - %2)", m_iHuntersWins, m_iPropsWins);
		else
			winner = string.Format("The match is a DRAW! (%1 - %2)", m_iPropsWins, m_iHuntersWins);

		BroadcastMessage(winner);

		#ifdef WORKBENCH
		RpcDo_ShowGameEnd(winner);
		#else
		Rpc(RpcDo_ShowGameEnd, winner);
		#endif
	}

	//------------------------------------------------------------
	// Game over broadcast sent to every client
	//------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowGameEnd(string resultText)
	{
		SCR_PopUpNotification notify = SCR_PopUpNotification.GetInstance();
		if (notify)
			notify.PopupMsg(resultText, 10.0, "Prop Hunt Over");
	}

	//------------------------------------------------------------
	// OnControllableDestroyed — tracks prop/hunter deaths and
	// handles prop-kill health restore for the shooter
	//------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);

		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		if (m_ePhase != CRF_EPropHuntPhase.HUNT)
			return;

		int victimId = instigatorContextData.GetVictimPlayerID();
		if (victimId <= 0)
			return;

		// Determine victim faction
		IEntity victimEnt = GetGame().GetPlayerManager().GetPlayerControlledEntity(victimId);
		if (!victimEnt)
			return;

		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(victimEnt.FindComponent(FactionAffiliationComponent));
		if (!facComp || !facComp.GetAffiliatedFaction())
			return;

		string victimFaction = facComp.GetAffiliatedFaction().GetFactionKey();

		if (victimFaction == m_sPropsTeamKey)
		{
			m_aAliveProps.RemoveItem(victimId);

			// If this prop had transformed, delete their prop entity.
			if (m_mPlayerToPropEntity.Contains(victimId))
			{
				IEntity propEnt = m_mPlayerToPropEntity.Get(victimId);
				m_mPlayerToPropEntity.Remove(victimId);
				if (propEnt)
					SCR_EntityHelper.DeleteEntityAndChildren(propEnt);
			}

			// Restore the killer's penalty health bar
			int killerId = instigatorContextData.GetInstigator().GetInstigatorPlayerID();
			if (killerId > 0 && m_mHunterHealth.Contains(killerId))
			{
				m_mHunterHealth.Set(killerId, m_iHunterMaxHealth);
				SendHunterHealthHint(killerId, m_iHunterMaxHealth, true);
			}

			if (m_aAliveProps.IsEmpty())
				EndRound(m_sHuntersTeamKey);
			else
				BroadcastMessage(string.Format("%1 props remaining.", m_aAliveProps.Count()));
		}
		else if (victimFaction == m_sHuntersTeamKey)
		{
			m_aAliveHunters.RemoveItem(victimId);
		}
	}

	//------------------------------------------------------------
	// ApplyHunterShotPenalty — called from the damage override
	// when a Hunter fires. Decrements the custom penalty health
	// bar. Does NOT apply any real damage to the Hunter per-shot.
	// When the bar reaches 0 the Hunter is killed via lethal damage
	// (which triggers the normal CRF death → spectator flow).
	//------------------------------------------------------------
	void ApplyHunterShotPenalty(int shooterPlayerId)
	{
		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		if (m_ePhase != CRF_EPropHuntPhase.HUNT)
			return;

		if (!m_mHunterHealth.Contains(shooterPlayerId))
			return;

		int currentHealth = m_mHunterHealth.Get(shooterPlayerId);

		// Guard against the lethal-damage loop: once health is 0 the kill
		// damage has already been applied — do not re-enter.
		if (currentHealth <= 0)
			return;

		currentHealth = currentHealth - m_iHunterShotPenalty;
		if (currentHealth < 0)
			currentHealth = 0;

		m_mHunterHealth.Set(shooterPlayerId, currentHealth);

		// Update the Hunter's on-screen hint (no layout file needed)
		SendHunterHealthHint(shooterPlayerId, currentHealth, false);

		if (currentHealth <= 0)
		{
			// Kill the Hunter's character so the normal CRF OnPlayerKilled
			// flow fires and they are moved to spectator.
			// The instigator entity (hunter character itself) has no player ID
			// so GetInstigatorPlayerID() returns <= 0 in OnDamage — the penalty
			// guard at the top of this function prevents a re-entry loop.
			IEntity shooter = GetGame().GetPlayerManager().GetPlayerControlledEntity(shooterPlayerId);
			if (shooter)
			{
				SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.GetDamageManager(shooter);
				if (dmg)
				{
					HitZone hz = dmg.GetDefaultHitZone();
					if (hz)
						hz.HandleDamage(9999, EDamageType.TRUE, shooter);
				}
			}
		}
	}

	//------------------------------------------------------------
	// SendHunterHealthHint — pushes the penalty health bar value
	// to a single Hunter's screen via the hint channel AND updates
	// the persistent visual health bar HUD widget.
	//------------------------------------------------------------
	protected void SendHunterHealthHint(int hunterId, int health, bool restored)
	{
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
		{
			string msg;
			if (restored)
				msg = string.Format("HP RESTORED — Hunter HP: %1 / %2", health, m_iHunterMaxHealth);
			else if (health <= 0)
				msg = "Hunter HP: 0 — YOU ARE DOWN!";
			else
				msg = string.Format("Hunter HP: %1 / %2", health, m_iHunterMaxHealth);

			bm.SendHint(msg, hunterId);
		}

		// Also update (or create) the visual health bar on the Hunter's HUD.
		UpdateHunterHealthBar(hunterId, health);
	}

	//------------------------------------------------------------
	// BlackoutHunter — broadcasts a screen-blackout command.
	// The broadcast reaches all clients; only the target hunter
	// creates or destroys the overlay widget.
	//------------------------------------------------------------
	protected void BlackoutHunter(int hunterId, bool enable)
	{
		Rpc(RpcDo_SetBlackout, hunterId, enable);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetBlackout(int hunterId, bool enable)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;

		if (enable)
		{
			if (m_wPropHuntBlackout)
				return;

			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (!workspace)
				return;

			m_wPropHuntBlackout = workspace.CreateWidget(
				WidgetType.ImageWidgetTypeID,
				WidgetFlags.VISIBLE | WidgetFlags.BLEND | WidgetFlags.STRETCH | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS,
				new Color(0, 0, 0, 1),
				999
			);
		}
		else
		{
			if (m_wPropHuntBlackout)
			{
				m_wPropHuntBlackout.RemoveFromHierarchy();
				m_wPropHuntBlackout = null;
			}
		}
	}

	//------------------------------------------------------------
	// UpdateHunterHealthBar — broadcasts current/max values.
	// Only the target hunter updates their bar widget.
	//------------------------------------------------------------
	protected void UpdateHunterHealthBar(int hunterId, int current)
	{
		Rpc(RpcDo_ShowHunterHP, hunterId, current, m_iHunterMaxHealth);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowHunterHP(int hunterId, int current, int max)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;

		// Secondary guard: only show to players actually on the hunters faction.
		// Prevents edge cases where faction keys are misconfigured or lists are stale.
		IEntity localEnt = SCR_PlayerController.GetLocalControlledEntity();
		if (localEnt)
		{
			FactionAffiliationComponent localFac = FactionAffiliationComponent.Cast(localEnt.FindComponent(FactionAffiliationComponent));
			if (localFac && localFac.GetAffiliatedFaction() && localFac.GetAffiliatedFaction().GetFactionKey() != m_sHuntersTeamKey)
				return;
		}

		if (!m_wHunterHealthBar)
		{
			if (!GetGame().GetHUDManager())
				return;

			m_wHunterHealthBar = GetGame().GetHUDManager().CreateLayout(PH_HP_LAYOUT, EHudLayers.LOW, 0);
			if (!m_wHunterHealthBar)
				return;
		}

		ProgressBarWidget bar = ProgressBarWidget.Cast(m_wHunterHealthBar.FindAnyWidget("HealthBar"));
		if (bar)
		{
			bar.SetMax(max);
			bar.SetCurrent(current);
		}

		TextWidget txt = TextWidget.Cast(m_wHunterHealthBar.FindAnyWidget("HPText"));
		if (txt)
			txt.SetText(string.Format("%1 / %2", current, max));
	}

	//------------------------------------------------------------
	// HideHunterHealthBar — broadcasts removal of the health bar.
	// Only the target hunter removes their widget.
	//------------------------------------------------------------
	protected void HideHunterHealthBar(int hunterId)
	{
		Rpc(RpcDo_HideHunterHP, hunterId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_HideHunterHP(int hunterId)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;

		if (m_wHunterHealthBar)
		{
			m_wHunterHealthBar.RemoveFromHierarchy();
			m_wHunterHealthBar = null;
		}
	}

	//------------------------------------------------------------
	// Hunter frozen/unfrozen — holster weapon and lock movement
	// while frozen, unholster and restore on unfreeze
	//------------------------------------------------------------
	protected void SetHunterFrozen(int hunterId, bool frozen)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(hunterId);
		if (!character)
			return;

		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(
			character.FindComponent(SCR_CharacterControllerComponent)
		);
		if (!charCtrl)
			return;

		charCtrl.SetDisableMovementControls(frozen);
	}

	//------------------------------------------------------------
	// Respawn all currently slotted players
	// Reuses the CRF_GamemodeManager pattern
	//------------------------------------------------------------
	protected void RespawnAllPlayers()
	{
		CRF_GamemodeManager gamemodeManager = CRF_GamemodeManager.GetInstance();
		if (!gamemodeManager)
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;

		array<int> playerIds = {};
		pm.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
			gamemodeManager.InitilizePlayer(playerId);
	}

	//------------------------------------------------------------
	// Convenience: broadcast a popup message to all clients
	//------------------------------------------------------------
	protected void BroadcastMessage(string msg)
	{
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.BroadcastMessage(msg);
	}

	//------------------------------------------------------------
	// RplProp callback — fires on every client when m_ePhase changes
	//------------------------------------------------------------
	protected void OnPhaseChanged()
	{
		// Clients can hook into phase changes here for HUD/audio feedback.
		// Nothing needed at baseline — extend this for custom UIs.
	}

	//------------------------------------------------------------
	// IsHunterFrozen — queried by the damage override
	//------------------------------------------------------------
	bool IsGracePhaseActive()
	{
		return m_ePhase == CRF_EPropHuntPhase.GRACE;
	}

	bool IsHuntPhaseActive()
	{
		return m_ePhase == CRF_EPropHuntPhase.HUNT;
	}

	string GetPropsTeamKey()   { return m_sPropsTeamKey; }
	string GetHuntersTeamKey() { return m_sHuntersTeamKey; }

	//------------------------------------------------------------
	// Prop transformation API — called by the modded
	// CRF_PlayerRplToOwnerManager when a Prop player transforms.
	//------------------------------------------------------------

	//! Register a player as transformed into a world prop entity.
	void SetPlayerTransformed(int playerId, IEntity propEnt)
	{
		if (!propEnt)
			return;
		m_mPlayerToPropEntity.Set(playerId, propEnt);
	}

	//! Returns the player ID whose disguise entity matches ent, or -1 if none.
	//! O(n) linear scan — n is the number of Props (typically < 30).
	int GetPlayerForPropEntity(IEntity ent)
	{
		if (!ent)
			return -1;
		foreach (int playerId, IEntity propEnt : m_mPlayerToPropEntity)
		{
			if (propEnt == ent)
				return playerId;
		}
		return -1;
	}

	//! True if this player has already transformed into a prop.
	bool IsPlayerTransformed(int playerId)
	{
		return m_mPlayerToPropEntity.Contains(playerId);
	}

	//! True if playerId is on the Props team and still alive this round.
	bool IsValidPropPlayer(int playerId)
	{
		return m_aAliveProps.Contains(playerId);
	}

	//------------------------------------------------------------
	// KillPropPlayerByProxy — called from the damage override when
	// a Hunter's shot hits a registered prop entity.
	// Applies lethal TRUE damage to the hidden character so the
	// normal CRF death→spectator flow fires via OnControllableDestroyed.
	// (OnControllableDestroyed then removes the player from m_aAliveProps
	// and deletes the prop entity from m_mPlayerToPropEntity.)
	//------------------------------------------------------------
	// killerEntity — the hunter's character entity; used as the instigator so that
	// OnControllableDestroyed can identify the hunter and restore their HP to max.
	void KillPropPlayerByProxy(int playerId, IEntity killerEntity = null)
	{
		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		SCR_DamageManagerComponent dmg = SCR_DamageManagerComponent.GetDamageManager(character);
		if (!dmg)
			return;

		// Use the hunter entity as instigator so OnControllableDestroyed sees the
		// correct killer for HP restore. Fall back to character self-instigator if
		// no killer was supplied (e.g. called from ApplyHunterShotPenalty kill).
		IEntity instigator;
		if (killerEntity)
			instigator = killerEntity;
		else
			instigator = character;

		HitZone hz = dmg.GetDefaultHitZone();
		if (hz)
			hz.HandleDamage(9999, EDamageType.TRUE, instigator);
	}

	//------------------------------------------------------------
	// EnablePropTransform — sends the transform enable/disable
	// RPC to the given Prop player's client. Uses a Broadcast RPC
	// on this component (guaranteed to work in non-modded class),
	// with a player-ID filter so only the target client acts.
	//------------------------------------------------------------
	protected void EnablePropTransform(int propId, bool enable)
	{
		Rpc(RpcDo_SetPropTransformEnabled, propId, enable);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPropTransformEnabled(int propId, bool enable)
	{
		if (SCR_PlayerController.GetLocalPlayerId() != propId)
			return;

		CRF_PlayerRplToOwnerManager mgr = CRF_PlayerRplToOwnerManager.GetInstance();
		if (mgr)
			mgr.ApplyPropTransformEnabled(enable);
	}

	//------------------------------------------------------------
	// CleanupPropEntities — deletes all spawned prop entities and
	// restores any hidden characters before the next round begins.
	// Called at the start of BeginRound.
	//------------------------------------------------------------
	protected void CleanupPropEntities()
	{
		foreach (int playerId, IEntity propEnt : m_mPlayerToPropEntity)
		{
			// Restore the hidden character (will be respawned shortly anyway,
			// but restoring flags avoids orphaned invisible entities).
			IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (character)
			{
				character.SetFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
				SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(
					character.FindComponent(SCR_CharacterControllerComponent)
				);
				if (charCtrl)
					charCtrl.SetDisableMovementControls(false);
			}

			if (propEnt)
				SCR_EntityHelper.DeleteEntityAndChildren(propEnt);
		}
		m_mPlayerToPropEntity.Clear();
	}
}
