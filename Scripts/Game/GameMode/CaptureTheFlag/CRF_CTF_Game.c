//------------------------------------------------------------------------------------
// CRF_CTFGamemodeManager: Capture the Flag gamemode
//
// OVERVIEW:
//   Two opposing sides fight over a single shared flag placed in the world.
//   The flag's position is shown on the map in real time. Either side may pick it up
//   and carry it to their own faction's designated drop zone to score a capture.
//   When the flag carrier dies the flag drops at their position. First faction to
//   reach the configured capture count wins the round.
//
// MISSION EDITOR SETUP:
//   1. Add CRF_CTFGamemodeManager as a component on your GameMode entity.
//   2. Place a prop/entity in the world named "ctf_flag" (or set m_sFlagEntityName).
//      - Add ActionsManagerComponent with a UserActionContext.
//      - Add CRF_CTF_PickupFlagAction and CRF_CTF_DropFlagAction as additionalActions.
//   3. Place two marker/prop entities:
//      - "ctf_blufor_dropzone"  — BLUFOR capture point  (configure m_sBluforDropzoneName)
//      - "ctf_opfor_dropzone"   — OPFOR capture point   (configure m_sOpforDropzoneName)
//   4. Adjust faction keys, capture radius, hold time, and win conditions as desired.
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Capture the Flag (CTF) — Fight over a shared flag and bring it to your capture zone to win.")]
class CRF_CTFGamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_CTFGamemodeManager : SCR_BaseGameModeComponent
{
	//===================================================================================
	// ATTRIBUTES
	//===================================================================================

	// --- Faction Settings ---
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key for side 1.", category: "Faction Settings")]
	string m_sBluforFactionKey;

	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key for side 2.", category: "Faction Settings")]
	string m_sOpforFactionKey;

	// --- Entity Setup ---
	[Attribute("ctf_flag", UIWidgets.EditBox, "Name of the flag entity placed in the world.\nAdd CRF_CTF_PickupFlagAction and CRF_CTF_DropFlagAction to its ActionsManagerComponent.", category: "Entity Setup")]
	string m_sFlagEntityName;

	[Attribute("ctf_blufor_dropzone", UIWidgets.EditBox, "Name of BLUFOR's capture drop zone entity in the world.", category: "Entity Setup")]
	string m_sBluforDropzoneName;

	[Attribute("ctf_opfor_dropzone", UIWidgets.EditBox, "Name of OPFOR's capture drop zone entity in the world.", category: "Entity Setup")]
	string m_sOpforDropzoneName;

	// --- Gameplay Settings ---
	[Attribute("1", UIWidgets.EditBox, "Number of successful flag captures required to win the round.", category: "Gameplay")]
	int m_iCapturesToWin;

	[Attribute("5.0", UIWidgets.EditBox, "Radius in metres of each capture drop zone. Holder must be within this distance.", category: "Gameplay")]
	float m_fCaptureRadius;

	[Attribute("3.0", UIWidgets.EditBox, "Seconds the flag holder must remain inside the drop zone to score a capture.", category: "Gameplay")]
	float m_fCaptureHoldTime;

	// --- Map Marker Settings ---
	[Attribute("false", UIWidgets.CheckBox, "Hide the flag map marker from all players.", category: "Map Markers")]
	bool m_bHideMapMarker;

	[Attribute("Flag", UIWidgets.EditBox, "Text label shown on the map for the flag.", category: "Map Markers")]
	string m_sFlagMarkerText;

	[Attribute("{428583D4284BC412}UI/Textures/Editor/EditableEntities/Waypoints/EditableEntity_Waypoint_SearchAndDestroy.edds", UIWidgets.ResourceNamePicker, "Icon used on the map for the flag marker.", params: "edds", category: "Map Markers")]
	ResourceName m_sFlagMarkerIcon;

	// --- Messages ---
	[Attribute("BLUFOR has captured the flag and won!", UIWidgets.EditBox, "Victory message broadcast when BLUFOR wins.", category: "Messages")]
	string m_sBluforWinMessage;

	[Attribute("OPFOR has captured the flag and won!", UIWidgets.EditBox, "Victory message broadcast when OPFOR wins.", category: "Messages")]
	string m_sOpforWinMessage;

	[Attribute("has picked up the flag!", UIWidgets.EditBox, "Text appended after [FACTION] PlayerName when the flag is picked up.", category: "Messages")]
	string m_sFlagPickupMessage;

	[Attribute("dropped the flag!", UIWidgets.EditBox, "Text appended after [FACTION] PlayerName when the flag is dropped.", category: "Messages")]
	string m_sFlagDroppedMessage;

	//===================================================================================
	// REPLICATED STATE
	//===================================================================================

	// Player ID of the current flag holder. -1 means the flag is unclaimed.
	[RplProp(onRplName: "OnFlagStateChanged")]
	protected int m_iFlagHolderPlayerId = -1;

	// Faction key of the current flag holder (empty when unclaimed).
	[RplProp(onRplName: "OnFlagStateChanged")]
	protected string m_sFlagHolderFaction = "";

	// BLUFOR capture score.
	[RplProp(onRplName: "OnScoreChanged")]
	protected int m_iBluforCaptures = 0;

	// OPFOR capture score.
	[RplProp(onRplName: "OnScoreChanged")]
	protected int m_iOpforCaptures = 0;

	/// General notification message: replicated change triggers the popup on all clients.
	[RplProp(onRplName: "OnMessageReceived")]
	protected string m_sMessageContent = "";
	protected string m_sStoredMessageContent = "";

	/// Static flag position — set when the flag is NOT being carried (spawn reset or drop).
	/// Received by all machines including JIP via RplProp; OnFlagPositionChanged applies it
	/// to the local entity. During carrying every machine independently follows the carrier
	/// in EOnFixedFrame instead, so this value is not bumped per-frame.
	[RplProp(onRplName: "OnFlagPositionChanged")]
	protected vector m_vFlagWorldPos;

	//===================================================================================
	// SERVER-ONLY RUNTIME VARIABLES
	//===================================================================================

	protected IEntity m_FlagEntity;
	protected IEntity m_BluforDropzone;
	protected IEntity m_OpforDropzone;

	protected vector m_vFlagSpawnPosition;
	protected bool   m_bGameOver         = false;
	protected bool   m_bEntitiesFound    = false;

	// Two-stage safestart tracking
	protected bool m_bHasSafestartBegun  = false;
	protected bool m_bGameInit           = false;

	// Accumulates time the holder has spent inside the drop zone each check cycle.
	protected float m_fCaptureProgress = 0.0;

	//===================================================================================
	// CLIENT / ALL-MACHINES RUNTIME VARIABLES
	//===================================================================================

	protected bool m_bMapMarkerAdded = false;

	//===================================================================================
	// SERVER-ONLY: DROP ZONE STATIC MARKERS
	//===================================================================================

	protected ref array<ref SCR_MapMarkerBase> m_aDropzoneMarkers = {};

	//===================================================================================
	// FRAME TIMING
	//===================================================================================

	float m_fUpdateBuffer = 0;

	//===================================================================================
	// CONSTANTS
	//===================================================================================

	static const int   FLAG_MARKER_SIZE  = 50;
	static const int   FLAG_MARKER_COLOR = ARGB(255, 255, 215, 0); // Gold

	//===================================================================================
	// INITIALIZATION
	//===================================================================================

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		SetEventMask(owner, EntityEvent.FIXEDFRAME);
	}

	override protected void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!GetGame().InPlayMode())
			return;

		// All machines need a local reference to the flag entity for position callbacks
		FindWorldEntities();
	}

	//! Locate and cache the flag and drop zone entities.
	//! Runs on ALL machines — every client needs m_FlagEntity to move it locally in EOnFixedFrame.
	protected void FindWorldEntities()
	{
		m_FlagEntity = GetGame().GetWorld().FindEntityByName(m_sFlagEntityName);
		if (!m_FlagEntity)
			Print(string.Format("[CRF_CTF] ERROR: Flag entity '%1' not found. Place an entity with this name in the mission.", m_sFlagEntityName), LogLevel.ERROR);

		if (!Replication.IsServer())
		{
			// Race-condition guard: if OnFlagPositionChanged fired before we cached the entity, apply it now.
			OnFlagPositionChanged();
			return;
		}

		if (m_FlagEntity)
		{
			m_vFlagSpawnPosition = m_FlagEntity.GetOrigin();
			m_vFlagWorldPos      = m_vFlagSpawnPosition;
		}

		m_BluforDropzone = GetGame().GetWorld().FindEntityByName(m_sBluforDropzoneName);
		if (!m_BluforDropzone)
			Print(string.Format("[CRF_CTF] ERROR: BLUFOR drop zone entity '%1' not found.", m_sBluforDropzoneName), LogLevel.ERROR);

		m_OpforDropzone = GetGame().GetWorld().FindEntityByName(m_sOpforDropzoneName);
		if (!m_OpforDropzone)
			Print(string.Format("[CRF_CTF] ERROR: OPFOR drop zone entity '%1' not found.", m_sOpforDropzoneName), LogLevel.ERROR);

		m_bEntitiesFound = (m_FlagEntity != null && m_BluforDropzone != null && m_OpforDropzone != null);
		if (!m_bEntitiesFound)
			Print("[CRF_CTF] WARNING: One or more required world entities are missing — CTF will not function correctly.", LogLevel.WARNING);
	}

	//===================================================================================
	// MAIN UPDATE LOOP
	//===================================================================================

	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);

		// ---- Map marker — all machines ----
		if (!m_bHideMapMarker && !m_bMapMarkerAdded)
			TryAddMapMarker();

		// ---- Flag following — ALL machines independently ----
		// Every machine already knows who holds the flag (m_iFlagHolderPlayerId is replicated).
		// Each machine queries the carrier's entity locally and moves its own copy of the flag.
		// No per-frame network traffic needed — each client runs this identically.
		if (m_bGameInit && m_FlagEntity && m_iFlagHolderPlayerId != -1)
		{
			IEntity holderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_iFlagHolderPlayerId);
			if (holderEntity)
			{
				vector pos = holderEntity.GetOrigin();
				pos[1] = pos[1] + 1.5;
				m_FlagEntity.SetOrigin(pos);
			}
		}

		// ---- Server-only tick ----
		if (!Replication.IsServer())
			return;

		m_fUpdateBuffer += timeSlice;
		if (m_fUpdateBuffer < 0.25)
			return;

		float deltaTime    = m_fUpdateBuffer;
		m_fUpdateBuffer    = 0;

		// Two-stage safestart check (mirrors HVT pattern):
		// If safestart is never configured, fall straight through to GameInit.
		if (!m_bHasSafestartBegun)
		{
			if (CRF_SafestartManager.GetInstance().GetSafestartStatus())
			{
				m_bHasSafestartBegun = true;
				return;
			}
			// Safestart never became active — skip straight to game init
		}

		if (!m_bGameInit)
		{
			// If safestart WAS seen active, wait for it to end before init
			if (m_bHasSafestartBegun && CRF_SafestartManager.GetInstance().GetSafestartStatus())
				return;

			GameInit();
			return;
		}

		if (m_bGameOver || !m_bEntitiesFound)
			return;

		// Drop the flag if the carrier has been killed or disconnected
		CheckHolderAlive();

		// Award a capture if the carrier reaches their drop zone
		CheckCaptureZone(deltaTime);
	}

	//! One-time server initialisation called the first frame after safestart ends.
	protected void GameInit()
	{
		m_bGameInit = true;
		m_vFlagWorldPos = m_vFlagSpawnPosition;
		Replication.BumpMe();
		Print(string.Format("[CRF_CTF] Game initialised. Flag spawn: %1. Captures to win: %2.", m_vFlagSpawnPosition.ToString(), m_iCapturesToWin));
		SpawnDropzoneMarkers();
	}

	//===================================================================================
	// FLAG MANAGEMENT  (server only)
	//===================================================================================

	//! Server: Award the flag to the requesting player.
	void PickupFlag(int playerId)
	{
		if (!m_bGameInit || m_bGameOver || !m_bEntitiesFound)
			return;

		if (m_iFlagHolderPlayerId != -1)
		{
			Print(string.Format("[CRF_CTF] PickupFlag rejected: flag already held by player %1.", m_iFlagHolderPlayerId));
			return;
		}

		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;

		Faction faction = factionManager.GetPlayerFaction(playerId);
		if (!faction)
			return;

		m_iFlagHolderPlayerId = playerId;
		m_sFlagHolderFaction  = faction.GetFactionKey();
		m_fCaptureProgress    = 0.0;

		string playerName   = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_sMessageContent   = string.Format("[%1] %2 %3", m_sFlagHolderFaction, playerName, m_sFlagPickupMessage);
		Replication.BumpMe();
		OnMessageReceived();

		Print(string.Format("[CRF_CTF] Flag picked up by player %1 (%2), faction: %3.", playerId, playerName, m_sFlagHolderFaction));
	}

	//! Server: Drop the flag at its current world position.
	//! \param broadcastDrop  Whether to show the "flag dropped" notification to all players.
	void DropFlag(bool broadcastDrop = true)
	{
		if (m_iFlagHolderPlayerId == -1)
			return;

		int    prevHolder  = m_iFlagHolderPlayerId;
		string prevFaction = m_sFlagHolderFaction;
		string playerName  = GetGame().GetPlayerManager().GetPlayerName(prevHolder);

		m_iFlagHolderPlayerId = -1;
		m_sFlagHolderFaction  = "";
		m_fCaptureProgress    = 0.0;
		// Record the current entity position as the drop point for JIP clients
		if (m_FlagEntity)
			m_vFlagWorldPos = m_FlagEntity.GetOrigin();

		if (broadcastDrop)
		{
			m_sMessageContent = string.Format("[%1] %2 %3", prevFaction, playerName, m_sFlagDroppedMessage);
			Replication.BumpMe();
			OnMessageReceived();
		}

		Print(string.Format("[CRF_CTF] Flag dropped. Previous holder: %1 (%2).", playerName, prevFaction));
	}

	//! Reset the flag entity to its original spawn position.
	protected void ResetFlagToSpawn()
	{
		m_iFlagHolderPlayerId = -1;
		m_sFlagHolderFaction  = "";
		m_fCaptureProgress    = 0.0;
		m_vFlagWorldPos       = m_vFlagSpawnPosition;

		if (m_FlagEntity)
			m_FlagEntity.SetOrigin(m_vFlagSpawnPosition);

		Replication.BumpMe();
	}

	//===================================================================================
	// HOLDER ALIVE CHECK  (server only)
	//===================================================================================

	//! Drop the flag if the carrier has been killed or disconnected.
	protected void CheckHolderAlive()
	{
		if (m_iFlagHolderPlayerId == -1)
			return;

		IEntity holderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_iFlagHolderPlayerId);
		if (!holderEntity)
		{
			DropFlag(true);
			return;
		}

		SCR_CharacterDamageManagerComponent dmgManager = SCR_CharacterDamageManagerComponent.Cast(
			holderEntity.FindComponent(SCR_CharacterDamageManagerComponent));

		if (dmgManager && dmgManager.GetState() == EDamageState.DESTROYED)
			DropFlag(true);
	}

	//===================================================================================
	// CAPTURE ZONE CHECK  (server only)
	//===================================================================================

	//! Accumulate time the carrier spends inside their faction's drop zone.
	//! When the hold time threshold is met, award the capture.
	protected void CheckCaptureZone(float deltaTime)
	{
		if (m_iFlagHolderPlayerId == -1)
		{
			m_fCaptureProgress = 0.0;
			return;
		}

		IEntity holderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_iFlagHolderPlayerId);
		if (!holderEntity)
			return;

		// Each faction delivers to their OWN drop zone
		IEntity dropzone = null;
		if (m_sFlagHolderFaction == m_sBluforFactionKey)
			dropzone = m_BluforDropzone;
		else if (m_sFlagHolderFaction == m_sOpforFactionKey)
			dropzone = m_OpforDropzone;

		if (!dropzone)
			return;

		float dist = vector.Distance(holderEntity.GetOrigin(), dropzone.GetOrigin());

		if (dist <= m_fCaptureRadius)
		{
			m_fCaptureProgress += deltaTime;

			if (m_fCaptureProgress >= m_fCaptureHoldTime)
				ProcessCapture();
		}
		else
		{
			m_fCaptureProgress = 0.0;
		}
	}

	//! Award a score, reset the flag, and check the win condition.
	protected void ProcessCapture()
	{
		string capturingFaction = m_sFlagHolderFaction;
		string capturingPlayer  = GetGame().GetPlayerManager().GetPlayerName(m_iFlagHolderPlayerId);

		if (capturingFaction == m_sBluforFactionKey)
			m_iBluforCaptures++;
		else if (capturingFaction == m_sOpforFactionKey)
			m_iOpforCaptures++;

		Print(string.Format("[CRF_CTF] Capture! %1 (%2). Score: BLUFOR %3 — OPFOR %4.",
			capturingPlayer, capturingFaction, m_iBluforCaptures, m_iOpforCaptures));

		// Reset flag before broadcasting so the map marker jumps back to spawn
		ResetFlagToSpawn();

		Replication.BumpMe();
		OnScoreChanged();

		// Check victory condition
		if (m_iBluforCaptures >= m_iCapturesToWin)
		{
			TriggerVictory(m_sBluforFactionKey);
		}
		else if (m_iOpforCaptures >= m_iCapturesToWin)
		{
			TriggerVictory(m_sOpforFactionKey);
		}
		else
		{
			// Intermediate capture — notify all players of current score
			m_sMessageContent = string.Format("[%1] %2 captured the flag!  Score: BLUFOR %3 — OPFOR %4",
				capturingFaction, capturingPlayer, m_iBluforCaptures, m_iOpforCaptures);
			Replication.BumpMe();
			OnMessageReceived();
		}
	}

	//! Broadcast the victory message and mark the game as over.
	protected void TriggerVictory(string winningFaction)
	{
		m_bGameOver = true;
		ClearDropzoneMarkers();

		if (winningFaction == m_sBluforFactionKey)
			m_sMessageContent = m_sBluforWinMessage;
		else
			m_sMessageContent = m_sOpforWinMessage;
		Replication.BumpMe();
		OnMessageReceived();

		Print(string.Format("[CRF_CTF] Victory declared for faction: %1", winningFaction));
	}

	//===================================================================================
	// MAP MARKERS  (all machines — each local player)
	//===================================================================================

	//! Attempt to register the flag marker with the local player's marker manager.
	//! Called every frame until successful, so JIP players are handled automatically.
	protected void TryAddMapMarker()
	{
		CRF_PlayerScriptedMarkerManager markerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		// timeDelay = 0 means the marker position is refreshed every time the map is opened,
		// giving near-real-time tracking as the flag entity moves with its carrier.
		markerManager.AddScriptedMarker(
			m_sFlagEntityName,
			"0 0 0",
			0,
			m_sFlagMarkerText,
			m_sFlagMarkerIcon,
			FLAG_MARKER_SIZE,
			FLAG_MARKER_COLOR
		);

		m_bMapMarkerAdded = true;
	}

	//! Spawn shared circle markers at each drop zone position (server only).
	protected void SpawnDropzoneMarkers()
	{
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMan)
			return;

		if (m_BluforDropzone)
		{
			vector bluforPos = m_BluforDropzone.GetOrigin();
			SCR_MapMarkerBase bluforMarker = new SCR_MapMarkerBase();
			bluforMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			bluforMarker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.CIRCLE);
			bluforMarker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR);
			bluforMarker.SetCustomText("BLUFOR Drop Zone");
			bluforMarker.SetWorldPos(bluforPos[0], bluforPos[2]);
			bluforMarker.m_bIsShared = true;
			markerMan.InsertStaticMarker(bluforMarker, false, true);
			m_aDropzoneMarkers.Insert(bluforMarker);
		}

		if (m_OpforDropzone)
		{
			vector opforPos = m_OpforDropzone.GetOrigin();
			SCR_MapMarkerBase opforMarker = new SCR_MapMarkerBase();
			opforMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			opforMarker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.CIRCLE);
			opforMarker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.OPFOR);
			opforMarker.SetCustomText("OPFOR Drop Zone");
			opforMarker.SetWorldPos(opforPos[0], opforPos[2]);
			opforMarker.m_bIsShared = true;
			markerMan.InsertStaticMarker(opforMarker, false, true);
			m_aDropzoneMarkers.Insert(opforMarker);
		}
	}

	//! Remove the drop zone circle markers (server only, called on game over).
	protected void ClearDropzoneMarkers()
	{
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		foreach (SCR_MapMarkerBase marker : m_aDropzoneMarkers)
		{
			if (markerMan)
				markerMan.RemoveStaticMarker(marker);
		}
		m_aDropzoneMarkers.Clear();
	}

	//! Remove the flag marker from the local player's map (called on game over).
	protected void RemoveMapMarker()
	{
		if (!m_bMapMarkerAdded)
			return;

		CRF_PlayerScriptedMarkerManager markerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!markerManager)
			return;

		markerManager.RemoveScriptedMarker(
			m_sFlagEntityName,
			"0 0 0",
			0,
			m_sFlagMarkerText,
			m_sFlagMarkerIcon,
			FLAG_MARKER_SIZE,
			FLAG_MARKER_COLOR
		);

		m_bMapMarkerAdded = false;
	}

	//===================================================================================
	// REPLICATION CALLBACKS  (called on all machines when RplProp values change)
	//===================================================================================

	//! Called on all machines when m_vFlagWorldPos changes.
	//! Applies the static (non-carried) position to the local flag entity.
	//! During carrying, EOnFixedFrame tracks the carrier independently on every machine.
	protected void OnFlagPositionChanged()
	{
		// Don't apply the static position while the flag is actively being carried —
		// EOnFixedFrame handles that independently on all machines.
		if (m_iFlagHolderPlayerId != -1)
			return;
		if (m_FlagEntity)
			m_FlagEntity.SetOrigin(m_vFlagWorldPos);
	}

	//! Called when m_iFlagHolderPlayerId or m_sFlagHolderFaction changes.
	protected void OnFlagStateChanged()
	{
		// Placeholder — extend for custom HUD elements, sounds, etc.
	}

	//! Called when either score value changes.
	protected void OnScoreChanged()
	{
		SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
		if (notification)
			notification.PopupMsg(string.Format("Score — BLUFOR: %1  |  OPFOR: %2  (first to %3)", m_iBluforCaptures, m_iOpforCaptures, m_iCapturesToWin));
	}

	//! Called when m_sMessageContent changes — shows a popup on every client.
	protected void OnMessageReceived()
	{
		if (m_sMessageContent == m_sStoredMessageContent || m_sMessageContent.IsEmpty())
			return;

		m_sStoredMessageContent = m_sMessageContent;

		SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
		if (notification)
			notification.PopupMsg(m_sMessageContent);

		// Remove the map marker once the game is declared over
		if (m_bGameOver)
			RemoveMapMarker();
	}

	//===================================================================================
	// PUBLIC API
	//===================================================================================

	//! Returns true if the flag is currently held by any player.
	bool IsFlagHeld()
	{
		return m_iFlagHolderPlayerId != -1;
	}

	//! Returns the player ID of the current flag holder, or -1 if unclaimed.
	int GetFlagHolderPlayerId()
	{
		return m_iFlagHolderPlayerId;
	}

	//! Returns the faction key of the current flag holder (empty string if unclaimed).
	string GetFlagHolderFaction()
	{
		return m_sFlagHolderFaction;
	}

	//! Returns the BLUFOR capture count.
	int GetBluforCaptures()
	{
		return m_iBluforCaptures;
	}

	//! Returns the OPFOR capture count.
	int GetOpforCaptures()
	{
		return m_iOpforCaptures;
	}

	//===================================================================================
	// SINGLETON
	//===================================================================================

	protected static CRF_CTFGamemodeManager m_sInstance;

	void CRF_CTFGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	static CRF_CTFGamemodeManager GetInstance()
	{
		return m_sInstance;
	}
}
