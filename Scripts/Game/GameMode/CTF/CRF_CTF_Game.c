//------------------------------------------------------------------------------------
// CRF_CTFGamemodeManager: Capture The Flag gamemode for the Coalition Reforger Framework.
//
// Supports two configurations via m_eGameModeType:
//  - Double:  Each faction has its own home flag (placed with CRF_CTF_FlagComponent's
//             Owning Faction set). The opposing faction must steal it from its home base
//             and carry it back to their own base to score.
//  - Neutral: A single ownerless flag (Owning Faction left empty) sits in the world.
//             Either faction may pick it up and must carry it to the OPPOSING faction's
//             base to score.
//
// Flags are placed directly in the mission world (not runtime-spawned) and self-register
// via CRF_CTF_FlagComponent's static roster. Pickup requests are funneled through
// COA_PlayerRplToAuthorityManager (client -> server RPC) for dedicated-server safety, same
// as every other CRF gamemode. Capture/return conditions are evaluated on a server-side
// timer, following the CacheHunt proximity-sweep pattern rather than physical triggers.
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Capture The Flag gamemode - Neutral (single flag) or Double (two home flags)")]
class CRF_CTFGamemodeManagerClass : SCR_BaseGameModeComponentClass
{
}

class CRF_CTFGamemodeManager : SCR_BaseGameModeComponent
{
	//===================================================================================
	// ATTRIBUTES
	//===================================================================================

	[Attribute("Double", UIWidgets.ComboBox, desc: "Neutral: one ownerless flag, carry to the enemy base to score. Double: each faction has a home flag, steal and return it to your own base to score.", enums: {ParamEnum("Neutral", "Neutral (single flag)"), ParamEnum("Double", "Double (two home flags)")})]
	protected string m_eGameModeType;

	[Attribute("BLUFOR", UIWidgets.ComboBox, desc: "First team's faction", enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")})]
	protected FactionKey m_eTeamA;

	[Attribute("OPFOR", UIWidgets.ComboBox, desc: "Second team's faction", enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")})]
	protected FactionKey m_eTeamB;

	[Attribute("ctf_teamA_base", UIWidgets.EditBox, desc: "Name of the empty entity marking Team A's capture/return point.")]
	protected string m_sTeamABaseName;

	[Attribute("ctf_teamB_base", UIWidgets.EditBox, desc: "Name of the empty entity marking Team B's capture/return point.")]
	protected string m_sTeamBBaseName;

	[Attribute("{B1A844B7FEF4BC41}PrefabsMissionMaking/Markers/ShapeMarkers/ShapeMarker_Base.et", UIWidgets.ResourceNamePicker, desc: "Marker entity spawned on each base to show the capture zone on the map. Must be a COA_ShapeMarker. Leave empty to disable.", params: "et")]
	protected ResourceName m_rBaseMarkerPrefab;

	[Attribute("0", UIWidgets.ComboBox, desc: "Shape drawn for each base's capture zone marker", enums: ParamEnumArray.FromEnum(COA_EMapShapeMarkerType))]
	protected COA_EMapShapeMarkerType m_eBaseMarkerShape;

	[Attribute("2.5", UIWidgets.Slider, desc: "Border thickness of the capture zone marker outline, in pixels", params: "0.25 12 0.25")]
	protected float m_fBaseMarkerBorderWidth;

	[Attribute("0.25 0.5 1 1", UIWidgets.ColorPicker, desc: "Colour of Team A's capture zone marker")]
	protected ref Color m_TeamAMarkerColor;

	[Attribute("1 0.25 0.25 1", UIWidgets.ColorPicker, desc: "Colour of Team B's capture zone marker")]
	protected ref Color m_TeamBMarkerColor;

	[Attribute("3", UIWidgets.EditBox, desc: "Number of captures needed to win the round.")]
	protected int m_iScoreToWin;

	[Attribute("8", UIWidgets.EditBox, desc: "Distance (m) from a base a carrier must be within to capture or return a flag.")]
	protected float m_fCaptureRadius;

	[Attribute("1", UIWidgets.CheckBox, desc: "Double mode only: a team's own flag must be at home for that team to score with the enemy flag.")]
	protected bool m_bRequireOwnFlagAtHomeToCapture;

	[Attribute("60", UIWidgets.EditBox, desc: "Seconds an untouched dropped flag waits before automatically returning home. 0 disables auto-return.")]
	protected float m_fAutoReturnSeconds;

	[Attribute("0.5", UIWidgets.EditBox, desc: "How often (seconds) capture/return/auto-return conditions are checked.")]
	protected float m_fTickInterval;

	// Capture zone visualization (only visible in Workbench editor)
	[Attribute("1", UIWidgets.CheckBox, desc: "Show a debug sphere sized to Capture Radius on each base entity. Editor only, no runtime cost.")]
	protected bool m_bEnableDebugVisuals;

	//===================================================================================
	// REPLICATED STATE
	//===================================================================================

	[RplProp()]
	protected int m_iTeamAScore = 0;

	[RplProp()]
	protected int m_iTeamBScore = 0;

	//===================================================================================
	// RUNTIME STATE
	//===================================================================================

	protected vector m_vTeamABasePos;
	protected vector m_vTeamBBasePos;
	protected bool m_bTeamABaseFound = false;
	protected bool m_bTeamBBaseFound = false;

	protected static const int MARKER_WIDGET_DELAY_MS = 100;

	protected bool m_bVictoryTriggered = false;
	protected float m_fTickAccumulator = 0;

	protected static CRF_CTFGamemodeManager m_sInstance;

	#ifdef WORKBENCH
	// Debug shape tracking (Workbench only - zero runtime overhead)
	protected ref array<ref Shape> m_aDebugShapes = {};
	#endif

	//===================================================================================
	// SINGLETON
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	void CRF_CTFGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_CTFGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_CTFGamemodeManager GetInstance()
	{
		return m_sInstance;
	}

	//===================================================================================
	// INITIALIZATION
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	override protected void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!GetGame().InPlayMode())
			return;

		SetEventMask(GetOwner(), EntityEvent.FRAME);

		IEntity teamABase = GetGame().GetWorld().FindEntityByName(m_sTeamABaseName);
		if (teamABase)
		{
			m_vTeamABasePos = teamABase.GetOrigin();
			m_bTeamABaseFound = true;
		}
		else
		{
			Print(string.Format("[CRF_CTF] WARNING: Team A base entity '%1' not found in world.", m_sTeamABaseName), LogLevel.WARNING);
		}

		IEntity teamBBase = GetGame().GetWorld().FindEntityByName(m_sTeamBBaseName);
		if (teamBBase)
		{
			m_vTeamBBasePos = teamBBase.GetOrigin();
			m_bTeamBBaseFound = true;
		}
		else
		{
			Print(string.Format("[CRF_CTF] WARNING: Team B base entity '%1' not found in world.", m_sTeamBBaseName), LogLevel.WARNING);
		}

		SpawnBaseMarkers();

		// Flag components self-register during their own OnPostInit; give them a moment
		// to finish before checking the roster matches the configured gamemode type.
		GetGame().GetCallqueue().CallLater(ValidateFlagSetup, 500, false);
	}

	//===================================================================================
	// CAPTURE ZONE MAP MARKERS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Spawns a COA_ShapeMarker on each base, sized to Capture Radius, so players can see on
	//! the map exactly where to bring the flag. COA_ShapeMarker carries no RplComponent, so it
	//! cannot be a server-spawned replicated entity - every machine spawns its own local copy
	//! here, same as Cache Hunt's attacker search area markers. A dedicated server has no map
	//! UI to show this on, so it skips spawning entirely.
	protected void SpawnBaseMarkers()
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		if (m_bTeamABaseFound)
			SpawnShapeMarker(m_vTeamABasePos, m_TeamAMarkerColor);

		if (m_bTeamBBaseFound)
			SpawnShapeMarker(m_vTeamBBasePos, m_TeamBMarkerColor);
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns and configures one capture zone marker locally. Every setter used here is
	//! local-only, which is what an unreplicated COA_ShapeMarker needs.
	//! \param[in] position World position to spawn the marker at
	//! \param[in] color Marker outline/fill colour
	protected IEntity SpawnShapeMarker(vector position, Color color)
	{
		if (m_rBaseMarkerPrefab.IsEmpty())
			return null;

		Resource markerResource = Resource.Load(m_rBaseMarkerPrefab);
		if (!markerResource || !markerResource.IsValid())
		{
			Print(string.Format("[CRF_CTF] WARNING: Base marker prefab '%1' could not be loaded.", m_rBaseMarkerPrefab), LogLevel.WARNING);
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
			Print(string.Format("[CRF_CTF] WARNING: Base marker prefab '%1' is not a COA_ShapeMarker.", m_rBaseMarkerPrefab), LogLevel.WARNING);
			SCR_EntityHelper.DeleteEntityAndChildren(markerEntity);
			return null;
		}

		// Both factions need to see both bases - to know where to score and where to defend.
		marker.m_bShowForAnyFaction = true;
		marker.m_bVisibleForEmptyFaction = true;
		marker.m_bUseWorldScale = true;
		marker.m_MarkerColor = color;

		marker.SetShapeType(m_eBaseMarkerShape);
		marker.SetShapeBorderWidth(m_fBaseMarkerBorderWidth);
		marker.SetShapeRadius(m_fCaptureRadius);

		// A manual marker only builds its widget on the map-open event it subscribes to in
		// EOnInit. Spawned while the map is already open (e.g. a JIP player), it would stay
		// invisible until the player closed and reopened the map, so build it now. Deferred a
		// beat because EOnInit has not run yet on a freshly spawned entity.
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
	//! Sanity-checks that the flags placed in the world match the configured gamemode type.
	//! Purely diagnostic - capture/pickup rules are driven per-flag by its Owning Faction
	//! attribute, so a mismatch here won't break gameplay, just warns the mission maker.
	protected void ValidateFlagSetup()
	{
		int neutralCount = 0;
		int ownedCount = 0;

		foreach (CRF_CTF_FlagComponent flag : CRF_CTF_FlagComponent.GetRegisteredFlags())
		{
			if (!flag)
				continue;

			if (flag.IsNeutral())
				neutralCount++;
			else
				ownedCount++;
		}

		if (m_eGameModeType == "Neutral" && neutralCount != 1)
			Print(string.Format("[CRF_CTF] WARNING: Neutral mode expects exactly 1 flag with no Owning Faction set, found %1.", neutralCount), LogLevel.WARNING);
		else if (m_eGameModeType == "Double" && ownedCount != 2)
			Print(string.Format("[CRF_CTF] WARNING: Double mode expects exactly 2 flags with Owning Faction set (one per team), found %1.", ownedCount), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!Replication.IsServer())
			return;

		m_fTickAccumulator += timeSlice;
		if (m_fTickAccumulator < m_fTickInterval)
			return;

		m_fTickAccumulator = 0;
		ServerTick();
	}

	//===================================================================================
	// PLAYER LIFECYCLE
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer())
			return;

		DropCarriedFlagForPlayer(instigatorContextData.GetVictimPlayerID());
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);

		if (!Replication.IsServer())
			return;

		DropCarriedFlagForPlayer(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Drops whichever flag the given player is carrying, at their current position.
	//! Server only. Used for both death and disconnect - either way the flag can no
	//! longer stay attached to that player.
	protected void DropCarriedFlagForPlayer(int playerId)
	{
		if (playerId <= 0)
			return;

		foreach (CRF_CTF_FlagComponent flag : CRF_CTF_FlagComponent.GetRegisteredFlags())
		{
			if (!flag || flag.GetCarrierId() != playerId)
				continue;

			vector dropPos = vector.Zero;
			vector dropAngles = vector.Zero;

			IEntity carrierEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (carrierEntity)
			{
				dropPos = carrierEntity.GetOrigin();
				dropAngles = carrierEntity.GetYawPitchRoll();
			}

			flag.Drop(dropPos, dropAngles);

			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			BroadcastMessage(string.Format("%1 has dropped the %2!", playerName, flag.GetDisplayName()));
		}
	}

	//===================================================================================
	// PICKUP (called from COA_PlayerRplToAuthorityManager's RpcAsk handler)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Server-authoritative pickup attempt. Re-validates every rule the action already
	//! checked client-side, since the client's view can be stale or spoofed.
	//! \param[in] playerId Player requesting the pickup
	//! \param[in] flag The flag being picked up
	void TryPickUpFlag(int playerId, CRF_CTF_FlagComponent flag)
	{
		if (!Replication.IsServer() || !flag || playerId <= 0)
			return;

		if (flag.IsCarried())
			return;

		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(playerEntity.FindComponent(SCR_DamageManagerComponent));
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			return;

		FactionKey playerFaction = GetEntityFactionKey(playerEntity);
		if (playerFaction.IsEmpty())
			return;

		FactionKey flagFaction = flag.GetOwningFaction();

		// A team cannot steal its own home flag while it is still docked at base.
		if (flag.IsAtBase() && !flagFaction.IsEmpty() && playerFaction == flagFaction)
			return;

		if (!flag.TryPickUp(playerId))
			return;

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		BroadcastMessage(string.Format("%1 has taken the %2!", playerName, flag.GetDisplayName()));
	}

	//===================================================================================
	// SERVER TICK - CAPTURE / RETURN / AUTO-RETURN
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	protected void ServerTick()
	{
		if (m_bVictoryTriggered)
			return;

		foreach (CRF_CTF_FlagComponent flag : CRF_CTF_FlagComponent.GetRegisteredFlags())
		{
			if (!flag)
				continue;

			if (flag.IsCarried())
				CheckCarriedFlag(flag);
			else if (flag.IsDropped())
				CheckAutoReturn(flag);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckAutoReturn(CRF_CTF_FlagComponent flag)
	{
		if (m_fAutoReturnSeconds <= 0)
			return;

		float elapsedMs = GetGame().GetWorld().GetWorldTime() - flag.GetDroppedAtMs();
		if (elapsedMs < m_fAutoReturnSeconds * 1000)
			return;

		flag.ReturnToBase();
		BroadcastMessage(string.Format("The %1 has returned to base.", flag.GetDisplayName()));
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckCarriedFlag(CRF_CTF_FlagComponent flag)
	{
		int carrierId = flag.GetCarrierId();
		IEntity carrierEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(carrierId);
		if (!carrierEntity)
			return;

		FactionKey carrierFaction = GetEntityFactionKey(carrierEntity);
		if (carrierFaction.IsEmpty())
			return;

		FactionKey flagFaction = flag.GetOwningFaction();
		vector carrierPos = carrierEntity.GetOrigin();
		float radiusSq = m_fCaptureRadius * m_fCaptureRadius;

		// Carrying your own team's flag: walking it back to your base just returns it, no score.
		if (!flagFaction.IsEmpty() && carrierFaction == flagFaction)
		{
			vector homePos;
			if (!TryGetBasePosition(carrierFaction, homePos))
				return;

			if (vector.DistanceSq(carrierPos, homePos) <= radiusSq)
			{
				flag.ReturnToBase();
				BroadcastMessage(string.Format("The %1 has been returned.", flag.GetDisplayName()));
			}

			return;
		}

		// Carrying an enemy flag (Double mode) or the neutral flag (Neutral mode):
		// Neutral mode scores at the OPPOSING faction's base; Double mode scores at
		// the carrier's OWN base.
		FactionKey captureBaseFaction = carrierFaction;
		if (flagFaction.IsEmpty())
			captureBaseFaction = GetOpposingTeam(carrierFaction);

		vector captureBasePos;
		if (!TryGetBasePosition(captureBaseFaction, captureBasePos))
			return;

		if (vector.DistanceSq(carrierPos, captureBasePos) > radiusSq)
			return;

		if (m_bRequireOwnFlagAtHomeToCapture && !flagFaction.IsEmpty())
		{
			CRF_CTF_FlagComponent ownFlag = CRF_CTF_FlagComponent.GetFlagForFaction(carrierFaction);
			if (ownFlag && !ownFlag.IsAtBase())
				return;
		}

		CaptureFlag(flag, carrierId, carrierFaction);
	}

	//------------------------------------------------------------------------------------------------
	//! Awards a capture, resets the flag, and checks for round victory.
	protected void CaptureFlag(CRF_CTF_FlagComponent flag, int scoringPlayerId, FactionKey scoringTeam)
	{
		if (scoringTeam == m_eTeamA)
			m_iTeamAScore++;
		else if (scoringTeam == m_eTeamB)
			m_iTeamBScore++;
		else
			return;

		Replication.BumpMe();

		flag.ReturnToBase();

		string playerName = GetGame().GetPlayerManager().GetPlayerName(scoringPlayerId);
		BroadcastMessage(string.Format("%1 captured the %2! Score: %3 - %4", playerName, flag.GetDisplayName(), m_iTeamAScore, m_iTeamBScore));

		CheckVictory(scoringTeam);
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckVictory(FactionKey scoringTeam)
	{
		if (m_bVictoryTriggered || m_iScoreToWin <= 0)
			return;

		int score = 0;
		if (scoringTeam == m_eTeamA)
			score = m_iTeamAScore;
		else if (scoringTeam == m_eTeamB)
			score = m_iTeamBScore;
		else
			return;

		if (score < m_iScoreToWin)
			return;

		m_bVictoryTriggered = true;

		BroadcastMessage(string.Format("%1 wins the round!", scoringTeam));

		CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
		if (loggingManager)
			loggingManager.SetWinningFaction(scoringTeam, "automatic");

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (gamemode)
			gamemode.AdvanceGamemodeState(true);
	}

	//===================================================================================
	// HELPERS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	protected bool TryGetBasePosition(FactionKey factionKey, out vector position)
	{
		if (factionKey == m_eTeamA && m_bTeamABaseFound)
		{
			position = m_vTeamABasePos;
			return true;
		}

		if (factionKey == m_eTeamB && m_bTeamBBaseFound)
		{
			position = m_vTeamBBasePos;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected FactionKey GetOpposingTeam(FactionKey factionKey)
	{
		if (factionKey == m_eTeamA)
			return m_eTeamB;

		return m_eTeamA;
	}

	//------------------------------------------------------------------------------------------------
	//! Resolves an entity's faction key through its faction affiliation component.
	protected static FactionKey GetEntityFactionKey(IEntity entity)
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
	protected void BroadcastMessage(string message)
	{
		Rpc(RpcDo_BroadcastMessage, message);
	}

	//===================================================================================
	// RPCs (server -> client broadcasts)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_BroadcastMessage(string message)
	{
		SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
		if (notification)
			notification.PopupMsg(message);
	}

	//===================================================================================
	// ACCESSORS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	int GetTeamAScore()
	{
		return m_iTeamAScore;
	}

	//------------------------------------------------------------------------------------------------
	int GetTeamBScore()
	{
		return m_iTeamBScore;
	}

	//------------------------------------------------------------------------------------------------
	FactionKey GetTeamA()
	{
		return m_eTeamA;
	}

	//------------------------------------------------------------------------------------------------
	FactionKey GetTeamB()
	{
		return m_eTeamB;
	}

	//===================================================================================
	// WORKBENCH DEBUG VISUALIZATION
	//===================================================================================

	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//! Draws a live wireframe sphere sized to Capture Radius on each base entity, so a mission
	//! maker can see and tune the (otherwise invisible) capture/return zone. Editor only - this
	//! whole block is compiled out of the shipped game.
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		super._WB_AfterWorldUpdate(owner, timeSlice);

		if (!m_bEnableDebugVisuals)
		{
			ClearDebugShapes();
			return;
		}

		CreateDebugShapes();
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateDebugShapes()
	{
		m_aDebugShapes.Clear();

		const ShapeFlags baseSphereFlags = ShapeFlags.TRANSP | ShapeFlags.VISIBLE | ShapeFlags.NOOUTLINE | ShapeFlags.NOZBUFFER;

		IEntity teamABase = GetGame().GetWorld().FindEntityByName(m_sTeamABaseName);
		if (teamABase)
			m_aDebugShapes.Insert(Shape.CreateSphere(ARGB(100, 0, 150, 255), baseSphereFlags, teamABase.GetOrigin(), m_fCaptureRadius));

		IEntity teamBBase = GetGame().GetWorld().FindEntityByName(m_sTeamBBaseName);
		if (teamBBase)
			m_aDebugShapes.Insert(Shape.CreateSphere(ARGB(100, 255, 50, 50), baseSphereFlags, teamBBase.GetOrigin(), m_fCaptureRadius));
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearDebugShapes()
	{
		if (m_aDebugShapes)
			m_aDebugShapes.Clear();
	}
	#endif
}
