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

	[Attribute("PropHuntHunterSpawn", UIWidgets.EditBox, "Name of the world entity that Hunters are teleported to at the end of each round. Place an empty entity in the mission with this name at the desired respawn point.")]
	string m_sHunterReturnSpawnName;

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

	// Client-side visual prop entities, keyed by player ID.
	// World prop prefabs don't have RplComponent so server-spawned entities are NOT
	// replicated to clients automatically. Each client spawns its own local copy via
	// RpcDo_ClientSpawnProp and tracks it here. The server uses m_mPlayerToPropEntity.
	protected ref map<int, IEntity> m_mClientPropEntities = new map<int, IEntity>();

	// Prevents re-entrant round-end logic
	protected bool m_bRoundEndPending = false;

	// Throttle timer for prop-position broadcast RPCs (10 Hz = 0.1 s).
	protected float m_fPropSyncTimer = 0;

	//------------------------------------------------------------
	// Client-side UI widgets — null on server, set only on the
	// local client that owns the feature.
	//------------------------------------------------------------
	// Full-screen blackout overlay shown to Hunters during grace.
	protected Widget m_wPropHuntBlackout;
	// Hunter penalty health bar.
	protected Widget m_wHunterHealthBar;
	// Layout resource for the hunter health bar HUD panel.
	protected static const ResourceName PH_HP_LAYOUT = "{AF1B0032C3D4E500}UI/layouts/HUD/PropHunt/CRF_PropHuntHunterHealthBar.layout";

	// Fully-opaque black overlay layout used for hunter screen blackout.
	protected static const ResourceName PH_BLACKOUT_LAYOUT = "{AF1B0036C3D4E500}UI/layouts/HUD/PropHunt/CRF_PropHuntBlackout.layout";

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

		if (m_bPhaseTimerActive)
		{
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

		// Prop position sync — broadcast position of every active prop to all clients.
		// SetWorldTransform on an entity with SimulationState.NONE does not replicate
		// on its own, so we push the update explicitly via RPC at 10 Hz.
		if ((m_ePhase == CRF_EPropHuntPhase.GRACE || m_ePhase == CRF_EPropHuntPhase.HUNT) && !m_mPlayerToPropEntity.IsEmpty())
		{
			m_fPropSyncTimer -= timeSlice;
			if (m_fPropSyncTimer <= 0)
			{
				m_fPropSyncTimer = 0.1; // ~10 updates per second per prop
				foreach (int propPlayerId, IEntity propEnt : m_mPlayerToPropEntity)
				{
					if (!propEnt)
						continue;
					IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(propPlayerId);
					if (!character)
						continue;
					vector pos = character.GetOrigin();
					float yaw = character.GetYawPitchRoll()[0];
					// Update server-authoritative transform. World prop prefabs lack
					// RplComponent so their position is NOT replicated automatically;
					// clients maintain their own visual copies (see RpcDo_ClientSpawnProp)
					// and are updated by the broadcast RPC below.
					propEnt.SetOrigin(pos);
					propEnt.SetYawPitchRoll(Vector(yaw, 0, 0));
					#ifdef WORKBENCH
					RpcDo_SyncPropTransform(propPlayerId, pos, yaw);
					#else
					Rpc(RpcDo_SyncPropTransform, propPlayerId, pos, yaw);
					#endif
				}
			}
		}
	}

	//------------------------------------------------------------
	// RpcDo_SyncPropTransform — unreliable broadcast at 10 Hz.
	// Moves and rotates each client's local visual prop copy.
	// Uses playerId (not RplId) because world prop prefabs lack
	// RplComponent and are not replicated by the engine.
	// In Workbench the server entity in m_mPlayerToPropEntity is used
	// directly (single process, no replication layer).
	//------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void RpcDo_SyncPropTransform(int playerId, vector pos, float yaw)
	{
		IEntity propEnt;
		#ifdef WORKBENCH
		propEnt = m_mPlayerToPropEntity.Get(playerId);
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		propEnt = m_mClientPropEntities.Get(playerId);
		#endif
		if (!propEnt)
			return;
		propEnt.SetOrigin(pos);
		propEnt.SetYawPitchRoll(Vector(yaw, 0, 0));
	}

	//------------------------------------------------------------
	// RpcDo_ClientSpawnProp — reliable broadcast; tells every client
	// to spawn a local visual copy of the prop entity for the given
	// player. Called from HandleTransformRequest on the server.
	//------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ClientSpawnProp(int playerId, ResourceName prefab, vector pos, float yaw)
	{
		#ifndef WORKBENCH
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif

		// Remove any existing client entity for this player (re-transform case).
		if (m_mClientPropEntities.Contains(playerId))
		{
			IEntity old = m_mClientPropEntities.Get(playerId);
			m_mClientPropEntities.Remove(playerId);
			if (old)
				SCR_EntityHelper.DeleteEntityAndChildren(old);
		}

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), sp.Transform);
		sp.Transform[3] = pos;

		IEntity propEnt = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), sp);
		if (!propEnt)
			return;

		Physics phys = propEnt.GetPhysics();
		if (phys)
			phys.ChangeSimulationState(SimulationState.NONE);

		m_mClientPropEntities.Set(playerId, propEnt);
	}

	//------------------------------------------------------------
	// RpcDo_ClientRemoveProp — reliable broadcast; tells every client
	// to delete the local visual prop copy for the given player.
	// Called when a prop player dies or switches disguise.
	//------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ClientRemoveProp(int playerId)
	{
		#ifndef WORKBENCH
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif

		IEntity propEnt = m_mClientPropEntities.Get(playerId);
		m_mClientPropEntities.Remove(playerId);
		if (propEnt)
			SCR_EntityHelper.DeleteEntityAndChildren(propEnt);
	}

	//------------------------------------------------------------
	// RpcDo_ClientClearAllProps — reliable broadcast; tells every
	// client to delete all local visual prop copies. Called at
	// round start (CleanupPropEntities).
	//------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ClientClearAllProps()
	{
		#ifndef WORKBENCH
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif

		foreach (int pid, IEntity propEnt : m_mClientPropEntities)
		{
			if (propEnt)
				SCR_EntityHelper.DeleteEntityAndChildren(propEnt);
		}
		m_mClientPropEntities.Clear();
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
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (respawnManager)
			respawnManager.RespawnAllSides();

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

		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMgr)
			return;

		array<int> playerIds = {};
		pm.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			Faction playerFaction = factionMgr.GetPlayerFaction(playerId);
			if (!playerFaction)
			{
				Print(string.Format("[PropHunt] CollectPlayerLists: playerId=%1 has no faction — skipping.", playerId), LogLevel.WARNING);
				continue;
			}

			string factionKey = playerFaction.GetFactionKey();
			Print(string.Format("[PropHunt] CollectPlayerLists: playerId=%1 faction=%2", playerId, factionKey), LogLevel.NORMAL);

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

		Print(string.Format("[PropHunt] CollectPlayerLists done: %1 props, %2 hunters.", m_aAliveProps.Count(), m_aAliveHunters.Count()), LogLevel.NORMAL);
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
		Print(string.Format("[PropHunt] StartGracePhase called. Grace period = %1s, Hunt limit = %2s.", m_iGracePeriodSeconds, m_iHuntTimeLimitSeconds), LogLevel.NORMAL);

		// Re-collect at grace start to ensure the lists are current regardless of
		// spawn timing or warmup duration. The CallLater in BeginRound handles the
		// common case; this call is the authoritative snapshot at phase transition.
		CollectPlayerLists();

		SetPhase(CRF_EPropHuntPhase.GRACE, m_iGracePeriodSeconds);
		BroadcastMessage(string.Format("PROPS: You have %1 seconds to hide! Press [F] near any object to disguise yourself. Hunters — stand by.", m_iGracePeriodSeconds));

		// Lock Hunters during grace period and black out their screens so they
		// cannot see where Props are hiding.
		foreach (int hunterId : m_aAliveHunters)
		{
			Print(string.Format("[PropHunt] StartGracePhase: freezing/blacking out hunterId=%1 (localPlayerId=%2)", hunterId, SCR_PlayerController.GetLocalPlayerId()), LogLevel.NORMAL);
			SetHunterFrozen(hunterId, true);
			BlackoutHunter(hunterId, true);
		}

		// Enable the F-key transform prompt for all Prop players.
		foreach (int propId : m_aAliveProps)
			EnablePropTransform(propId, true);

		// Workbench fallback: if no factions were assigned (common in solo preview
		// sessions) treat the local player as a Prop so the F-key path can be tested.
		#ifdef WORKBENCH
		if (m_aAliveProps.IsEmpty())
		{
			Print("[PropHunt] WORKBENCH: no props found via faction — enabling transform for local player as fallback.", LogLevel.WARNING);
			array<int> wbPlayers = {};
			GetGame().GetPlayerManager().GetPlayers(wbPlayers);
			foreach (int wbId : wbPlayers)
				EnablePropTransform(wbId, true);
		}
		#endif
	}

	protected void StartHuntPhase()
	{
		Print("[PropHunt] StartHuntPhase called — grace period has ended.", LogLevel.NORMAL);

		SetPhase(CRF_EPropHuntPhase.HUNT, m_iHuntTimeLimitSeconds);
		BroadcastMessage("HUNTERS: Begin! Every shot costs HP. Kill a prop to restore to 100. Reach 0 and you're out!");

		// Unlock Hunters, lift their screen blackout, show their health bar,
		// and register the OnProjectileShot event handler so every shot costs HP.
		foreach (int hunterId : m_aAliveHunters)
		{
			BlackoutHunter(hunterId, false);
			SetHunterFrozen(hunterId, false);
			SendHunterHealthHint(hunterId, m_iHunterMaxHealth, false);
			RegisterHunterShotEH(hunterId);
		}

		// Disable the transform input for all Props (whether transformed or not).
		foreach (int propId : m_aAliveProps)
			EnablePropTransform(propId, false);

		// Freeze transformed props now that hunting begins — props should stay still.
		// First do a guaranteed position snap so every client has the prop at the
		// correct location before movement is locked (guards against any drift that
		// accumulated during grace or late-joining clients).
		foreach (int propPlayerId, IEntity propEnt : m_mPlayerToPropEntity)
		{
			if (!propEnt)
				continue;
			IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(propPlayerId);
			if (!character)
				continue;
			vector snapPos = character.GetOrigin();
			float snapYaw = character.GetYawPitchRoll()[0];
			propEnt.SetOrigin(snapPos);
			propEnt.SetYawPitchRoll(Vector(snapYaw, 0, 0));
			#ifndef WORKBENCH
			Rpc(RpcDo_SyncPropTransform, propPlayerId, snapPos, snapYaw);
			#endif
			SetHunterFrozen(propPlayerId, true);
		}
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

		// Unregister shot event handlers from all surviving hunters.
		// NOTE: HP bars are intentionally kept visible during the inter-round pause so hunters
		// can see their restored HP. They are hidden in StartNextRoundOrEndGame instead.
		foreach (int hunterId : m_aAliveHunters)
			UnregisterHunterShotEH(hunterId);

		// Teleport surviving hunters back to their side's spawn point so they are
		// ready in position when the next round's warmup begins.
		TeleportHuntersToSpawn();

		// Move to inter-round pause
		SetPhase(CRF_EPropHuntPhase.ROUNDEND, m_iInterRoundPauseSeconds);
	}

	//------------------------------------------------------------
	// TeleportHuntersToSpawn — moves all surviving hunters to the
	// world entity named m_sHunterReturnSpawnName.
	// Called server-side at round end. Applies the teleport both
	// on the server (authoritative) and via Broadcast RPC so all
	// clients get an immediate visual update (same pattern as GunGame).
	//------------------------------------------------------------
	protected void TeleportHuntersToSpawn()
	{
		if (m_sHunterReturnSpawnName.IsEmpty())
			return;

		IEntity spawnEnt = GetGame().GetWorld().FindEntityByName(m_sHunterReturnSpawnName);
		if (!spawnEnt)
		{
			Print(string.Format("[PropHunt] TeleportHuntersToSpawn: spawn entity '%1' not found in world.", m_sHunterReturnSpawnName), LogLevel.WARNING);
			return;
		}

		vector spawnPos = spawnEnt.GetOrigin();

		foreach (int hunterId : m_aAliveHunters)
		{
			// Server-authoritative move.
			SCR_Global.TeleportPlayer(hunterId, spawnPos);
			// Broadcast so all clients see the character at the new position immediately.
			#ifdef WORKBENCH
			RpcDo_TeleportPlayer(hunterId, spawnPos);
			#else
			Rpc(RpcDo_TeleportPlayer, hunterId, spawnPos);
			#endif
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_TeleportPlayer(int playerId, vector pos)
	{
		SCR_Global.TeleportPlayer(playerId, pos);
	}

	//------------------------------------------------------------
	// After inter-round pause: advance round counter or end game
	//------------------------------------------------------------
	protected void StartNextRoundOrEndGame()
	{
		// Hide HP bars now that the inter-round pause has elapsed.
		// m_aAliveHunters still reflects last round's survivors at this point.
		foreach (int hunterId : m_aAliveHunters)
			HideHunterHealthBar(hunterId);

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

		// Determine victim faction via the FactionManager, which is the
		// authoritative source in CRF regardless of character entity state.
		SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMgr)
			return;

		Faction victimFactionObj = factionMgr.GetPlayerFaction(victimId);
		if (!victimFactionObj)
			return;

		string victimFaction = victimFactionObj.GetFactionKey();

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

				// Remove the client-side visual copy on all clients.
				#ifndef WORKBENCH
				Rpc(RpcDo_ClientRemoveProp, victimId);
				#endif
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
			HideHunterHealthBar(victimId);
			UnregisterHunterShotEH(victimId);

			if (m_aAliveHunters.IsEmpty())
				EndRound(m_sPropsTeamKey);
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
		// Only send a text hint for notable events (HP restored on kill, or HP depleted).
		// Normal per-shot decrements are shown silently via the health bar widget only.
		if (restored || health <= 0)
		{
			CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
			if (bm)
			{
				string msg;
				if (restored)
					msg = string.Format("PROP FOUND! HP restored to %1.", m_iHunterMaxHealth);
				else
					msg = "Hunter HP: 0 — YOU ARE DOWN!";

				bm.SendHint(msg, hunterId);
			}
		}

		// Always update the visual health bar on the Hunter's HUD.
		UpdateHunterHealthBar(hunterId, health);
	}

	//------------------------------------------------------------
	// BlackoutHunter — broadcasts a screen-blackout command.
	// The broadcast reaches all clients; only the target hunter
	// creates or destroys the overlay widget.
	//------------------------------------------------------------
	protected void BlackoutHunter(int hunterId, bool enable)
	{
		#ifdef WORKBENCH
		RpcDo_SetBlackout(hunterId, enable);
		#else
		Rpc(RpcDo_SetBlackout, hunterId, enable);
		#endif
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetBlackout(int hunterId, bool enable)
	{
		#ifndef WORKBENCH
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;
		#endif

		if (enable)
		{
			if (m_wPropHuntBlackout)
				return;

			// Use GetWorkspace().CreateWidgets() — works in both Workbench and multiplayer.
			// GetHUDManager() is unavailable in Workbench.
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (!workspace)
				return;

			m_wPropHuntBlackout = workspace.CreateWidgets(PH_BLACKOUT_LAYOUT);
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
		#ifdef WORKBENCH
		RpcDo_ShowHunterHP(hunterId, current, m_iHunterMaxHealth);
		#else
		Rpc(RpcDo_ShowHunterHP, hunterId, current, m_iHunterMaxHealth);
		#endif
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowHunterHP(int hunterId, int current, int max)
	{
		#ifndef WORKBENCH
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;
		#endif

		// Secondary guard: only show to players actually on the hunters faction.
		// Use SCR_FactionManager as the authoritative source — the character's
		// FactionAffiliationComponent may reflect the prefab default, not the slot.
		SCR_FactionManager localFactionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (localFactionMgr)
		{
			Faction localFaction = localFactionMgr.GetPlayerFaction(hunterId);
			if (localFaction && localFaction.GetFactionKey() != m_sHuntersTeamKey)
				return;
		}

		if (!m_wHunterHealthBar)
		{
			WorkspaceWidget workspace = GetGame().GetWorkspace();
			if (!workspace)
				return;

			m_wHunterHealthBar = workspace.CreateWidgets(PH_HP_LAYOUT);
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
		#ifdef WORKBENCH
		RpcDo_HideHunterHP(hunterId);
		#else
		Rpc(RpcDo_HideHunterHP, hunterId);
		#endif
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_HideHunterHP(int hunterId)
	{
		#ifndef WORKBENCH
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;
		#endif

		if (m_wHunterHealthBar)
		{
			m_wHunterHealthBar.RemoveFromHierarchy();
			m_wHunterHealthBar = null;
		}
	}

	//------------------------------------------------------------
	// RegisterHunterShotEH / UnregisterHunterShotEH
	// Attaches (or detaches) the OnProjectileShot script event handler
	// to a hunter's character entity. Fires server-side for every
	// projectile created by that character, including misses.
	//------------------------------------------------------------
	protected void RegisterHunterShotEH(int hunterId)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(hunterId);
		if (!character)
			return;

		EventHandlerManagerComponent eh = EventHandlerManagerComponent.Cast(
			character.FindComponent(EventHandlerManagerComponent)
		);
		if (eh)
			eh.RegisterScriptHandler("OnProjectileShot", this, OnHunterProjectileShot);
	}

	protected void UnregisterHunterShotEH(int hunterId)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(hunterId);
		if (!character)
			return;

		EventHandlerManagerComponent eh = EventHandlerManagerComponent.Cast(
			character.FindComponent(EventHandlerManagerComponent)
		);
		if (eh)
			eh.RemoveScriptHandler("OnProjectileShot", this, OnHunterProjectileShot);
	}

	// Callback: fires on the server each time a registered hunter fires a projectile.
	protected void OnHunterProjectileShot(int playerId, BaseWeaponComponent weapon, IEntity projectile)
	{
		ApplyHunterShotPenalty(playerId);
	}

	//------------------------------------------------------------
	// Hunter frozen/unfrozen — sends an RPC to the hunter's client
	// so SetDisableMovementControls is applied locally. This mirrors
	// vanilla SCR_BaseGameMode.SetLocalControls which runs client-side.
	// Calling it only server-side does not block client input processing.
	//------------------------------------------------------------
	protected void SetHunterFrozen(int hunterId, bool frozen)
	{
		#ifdef WORKBENCH
		RpcDo_SetHunterMovementFrozen(hunterId, frozen);
		#else
		Rpc(RpcDo_SetHunterMovementFrozen, hunterId, frozen);
		#endif
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetHunterMovementFrozen(int hunterId, bool frozen)
	{
		#ifndef WORKBENCH
		if (SCR_PlayerController.GetLocalPlayerId() != hunterId)
			return;
		#endif

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		IEntity controlledEntity = pc.GetControlledEntity();
		if (!controlledEntity)
			return;

		SCR_CharacterControllerComponent charCtrl = SCR_CharacterControllerComponent.Cast(
			controlledEntity.FindComponent(SCR_CharacterControllerComponent)
		);
		if (!charCtrl)
			return;

		charCtrl.SetDisableMovementControls(frozen);
		charCtrl.SetDisableWeaponControls(frozen);
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
	// SetPropCharacterVisible — broadcasts a VISIBLE flag change
	// for a prop player's character entity to all clients.
	// Entity flag changes from a Server-only RPC are NOT replicated
	// automatically, so every machine must apply them explicitly.
	//------------------------------------------------------------
	void SetPropCharacterVisible(int playerId, bool visible)
	{
		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		RplId charRplId = Replication.FindId(character);

		#ifdef WORKBENCH
		RpcDo_SetPropCharacterVisible(charRplId, visible);
		#else
		Rpc(RpcDo_SetPropCharacterVisible, charRplId, visible);
		#endif
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPropCharacterVisible(RplId charRplId, bool visible)
	{
		IEntity character = IEntity.Cast(Replication.FindItem(charRplId));
		if (!character)
			return;

		if (visible)
			character.SetFlags(EntityFlags.VISIBLE, true);
		else
			character.ClearFlags(EntityFlags.VISIBLE, true);
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

	//! True if the alive-props list is empty (used by Workbench fallback).
	bool NoPropsAssigned()
	{
		return m_aAliveProps.IsEmpty();
	}

	//------------------------------------------------------------
	// HandleTransformRequest — called server-side from
	// CRF_PlayerRplToOwnerManager.RpcDo_RequestPropTransform, which
	// is declared in the non-modded base class so its RPC registration
	// is always reliable on dedicated servers.
	// In Workbench the modded class calls this directly (no network).
	// playerId is resolved by GetOwner() in the RPC, so no spoofing.
	//------------------------------------------------------------
	void HandleTransformRequest(int playerId, ResourceName prefab)
	{
		Print("[PropHunt] HandleTransformRequest called.", LogLevel.NORMAL);

		if (!IsGracePhaseActive() && !IsHuntPhaseActive())
		{
			Print("[PropHunt] HandleTransformRequest: REJECTED — wrong phase.", LogLevel.WARNING);
			return;
		}

		#ifdef WORKBENCH
		// In Workbench, GetLocalPlayerId() returns 0; fall back to the first available player.
		if (playerId <= 0)
		{
			PlayerManager wbPm = GetGame().GetPlayerManager();
			if (wbPm)
			{
				array<int> wbIds = {};
				wbPm.GetPlayers(wbIds);
				if (!wbIds.IsEmpty())
					playerId = wbIds[0];
			}
		}
		Print(string.Format("[PropHunt] HandleTransformRequest: Workbench resolved playerId=%1", playerId), LogLevel.NORMAL);
		#endif

		if (playerId <= 0)
		{
			Print("[PropHunt] HandleTransformRequest: REJECTED — could not resolve playerId.", LogLevel.WARNING);
			return;
		}

		// Validate: must be a living Prop team player.
		// Workbench fallback: if no factions were assigned the props list is empty;
		// allow any player so the transform path can be tested.
		#ifdef WORKBENCH
		if (!IsValidPropPlayer(playerId) && !NoPropsAssigned())
			return;
		#else
		if (!IsValidPropPlayer(playerId))
			return;
		#endif

		if (!prefab)
			return;

		IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!character)
			return;

		// If already transformed, delete the previous prop so the player
		// can switch disguise during the grace phase.
		if (IsPlayerTransformed(playerId))
			ClearPlayerProp(playerId);

		// Capture the character's full world transform so the spawned prop
		// appears exactly where the player is standing.
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		character.GetWorldTransform(spawnParams.Transform);

		// Hide the character on ALL clients — flag changes from a server-side
		// call do not replicate automatically; the gamemode broadcasts explicitly.
		// TRACEABLE is intentionally kept so bullet raycasts still hit the
		// character's capsule for prop-kill detection.
		SetPropCharacterVisible(playerId, false);

		// Do NOT freeze the character here. During grace the prop (invisible
		// character) can still move; EOnFrame sync moves the visible prop entity
		// to follow at 10 Hz. Movement is frozen when the hunt phase begins.

		// Spawn the prop entity at the character's position.
		IEntity propEnt = GetGame().SpawnEntityPrefab(Resource.Load(prefab), GetGame().GetWorld(), spawnParams);
		if (!propEnt)
		{
			// Spawn failed — restore character visibility.
			SetPropCharacterVisible(playerId, true);
			return;
		}

		// Disable physics on the prop — it is a purely visual stand-in.
		// The hidden character provides the collision capsule. Without this
		// the prop's physics mesh would push the character away.
		Physics propPhys = propEnt.GetPhysics();
		if (propPhys)
			propPhys.ChangeSimulationState(SimulationState.NONE);

		// Register the player↔entity pair (server-side).
		SetPlayerTransformed(playerId, propEnt);

		// Tell every client to spawn their own local visual copy.
		// World prop prefabs lack RplComponent, so the server-spawned entity is not
		// replicated automatically — each client must maintain its own copy.
		#ifndef WORKBENCH
		vector spawnPos = spawnParams.Transform[3];
		float spawnYaw = character.GetYawPitchRoll()[0];
		Rpc(RpcDo_ClientSpawnProp, playerId, prefab, spawnPos, spawnYaw);
		#endif

		// Notify the player.
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.SendHint("You are DISGUISED! Move around during grace, then hold still once the hunt begins.", playerId);
	}

	//! Removes and deletes the existing prop entity for this player so they can
	//! re-transform into a different prop during the grace phase.
	void ClearPlayerProp(int playerId)
	{
		IEntity oldProp = m_mPlayerToPropEntity.Get(playerId);
		m_mPlayerToPropEntity.Remove(playerId);
		if (oldProp)
			SCR_EntityHelper.DeleteEntityAndChildren(oldProp);

		// Remove the client-side visual copy on all clients.
		#ifndef WORKBENCH
		Rpc(RpcDo_ClientRemoveProp, playerId);
		#endif
	}

	//! Returns the prop player ID whose hidden character entity matches charEntity,
	//! or -1 if the entity is not a currently-transformed prop player.
	//! Used by SCR_DamageManagerComponent to detect bullet hits on the invisible character.
	int GetPropPlayerForCharacter(IEntity charEntity)
	{
		if (!charEntity)
			return -1;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return -1;
		// Only scan the (small) alive-props list, not all players.
		foreach (int pid : m_aAliveProps)
		{
			if (IsPlayerTransformed(pid) && pm.GetPlayerControlledEntity(pid) == charEntity)
				return pid;
		}
		return -1;
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
		#ifdef WORKBENCH
		RpcDo_SetPropTransformEnabled(propId, enable);
		#else
		Rpc(RpcDo_SetPropTransformEnabled, propId, enable);
		#endif
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_SetPropTransformEnabled(int propId, bool enable)
	{
		#ifndef WORKBENCH
		if (SCR_PlayerController.GetLocalPlayerId() != propId)
			return;
		#endif

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
			// Restore character visibility on all clients before respawn.
			SetPropCharacterVisible(playerId, true);

			// Unfreeze movement in case the player was frozen as a transformed prop.
			IEntity character = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (character)
			{
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

		// Remove all client-side visual copies on all clients.
		#ifndef WORKBENCH
		Rpc(RpcDo_ClientClearAllProps);
		#endif
	}
}
