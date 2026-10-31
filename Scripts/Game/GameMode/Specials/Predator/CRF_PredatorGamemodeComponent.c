//------------------------------------------------------------------------------------
// CRF_PredatorGamemodeComponent: a hunter on its own faction, stalking everyone else
//
// Like the Slasher's killers, the predator never tires, is hard to put down, and its body vanishes soon
// after it dies so nobody takes its gear. On top of that:
//
//   Cloak [O, CRF_PredatorCloak]: hides the predator from everyone else, except anyone within a few
//     metres who sees through it. It drains while on and recharges while off, drops when the charge runs
//     out, and fails for a few seconds when the predator is hit.
//   Thermal vision [;, CRF_PredatorThermal]: the predator's view switches to a heat palette. Local to the
//     predator - nothing goes over the network.
//
// Everyone else is told when something is watching them, and the predator's kills are announced.
// It works alongside any objective mode (see CRF_DynamicObjectiveGamemodeComponent).
//
// Multiplayer
//   - The server owns everything that matters: charge, hit lockout, who's cloaked, damage, stamina rules,
//     body removal. The predator's client only asks to toggle the cloak (an RPC on its player controller,
//     rate-limited on the server).
//   - Who's cloaked is one replicated list (m_aCloakedIds), sent when it changes; players who join late get
//     it with the gamemode entity.
//   - Every machine with a player (clients, a listen-server host, Workbench) hides the cloaked predators'
//     characters for itself, every frame, tracked by player so respawns and deleted bodies are handled.
//     The server never hides anything, so hit detection is untouched. Spectators always see the predator.
//   - Thermal vision is local to the predator's machine. Popups to a listen-server host's own player are
//     shown directly, since an owner RPC from the server doesn't reach it.
//
// Setup for mission makers
// 1. Add this component to the gamemode entity; set the predator faction.
// 2. A slot on that faction with Configs/Gearscripts/Standard/Specials/Predator/COA_GS_Predator.conf.
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "Predator: a hunter on its own faction that can cloak and see in thermal, never tires and is hard to kill.")]
class CRF_PredatorGamemodeComponentClass : SCR_BaseGameModeComponentClass {}

class CRF_PredatorGamemodeComponent : SCR_BaseGameModeComponent
{
	//===================================================================================
	// EDITOR ATTRIBUTES
	//===================================================================================

	[Attribute("INDFOR", UIWidgets.EditBox, "Faction of the predator.", category: "Predator")]
	protected string m_sPredatorFaction;

	[Attribute("1", UIWidgets.CheckBox, "Running and sprinting don't drain the predator's stamina.", category: "Predator")]
	protected bool m_bTireless;

	[Attribute("0.4", UIWidgets.Slider, "Share of incoming damage the predator takes (0.4 = two and a half times as hard to kill).", params: "0.05 1 0.05", category: "Predator")]
	protected float m_fDamageTaken;

	[Attribute("5", UIWidgets.Auto, "Seconds after the predator dies before its body vanishes, taking its gear with it. 0 = bodies stay.", category: "Predator")]
	protected float m_fBodyRemoveDelay;

	[Attribute("25", UIWidgets.Auto, "Other players within this many metres of a predator are told something is watching them. 0 = off.", category: "Predator")]
	protected float m_fPresenceRadius;

	[Attribute("1", UIWidgets.CheckBox, "Tell everyone when the predator takes someone, and when it goes down.", category: "Predator")]
	protected bool m_bAnnounceKills;

	[Attribute("1", UIWidgets.CheckBox, "The predator can cloak.", category: "Predator - Cloak")]
	protected bool m_bCloakEnabled;

	[Attribute("75", UIWidgets.Auto, "Seconds a full charge lasts.", category: "Predator - Cloak")]
	protected float m_fCloakSeconds;

	[Attribute("120", UIWidgets.Auto, "Seconds to recharge from empty to full while the cloak is off.", category: "Predator - Cloak")]
	protected float m_fCloakRechargeSeconds;

	[Attribute("25", UIWidgets.Slider, "Charge (percent) needed to engage the cloak.", params: "0 100 5", category: "Predator - Cloak")]
	protected int m_iCloakMinimumPercent;

	[Attribute("6", UIWidgets.Auto, "Seconds the cloak fails after the predator is hit.", category: "Predator - Cloak")]
	protected float m_fCloakHitLockout;

	[Attribute("4", UIWidgets.Auto, "Players this close (metres) see through the cloak. 0 = nobody does.", category: "Predator - Cloak")]
	protected float m_fCloakRevealDistance;

	[Attribute("1", UIWidgets.CheckBox, "The predator can switch to thermal vision.", category: "Predator - Thermal")]
	protected bool m_bThermalEnabled;

	[Attribute("{6EB54B42992241F1}UI/Materials/Predator/CRF_PredatorThermal.emat", UIWidgets.ResourceNamePicker, "ThermalImaging post-process material for the predator's view.", params: "emat", category: "Predator - Thermal")]
	protected ResourceName m_sThermalMaterial;

	//===================================================================================
	// STATE
	//===================================================================================

	//! The only replicated state: living predators that are cloaked, by player. Sent when it changes; players
	//! who join late get it with the gamemode entity.
	[RplProp()]
	protected ref array<int> m_aCloakedIds = {};

	// Server, by player
	protected bool m_bRoundLive;
	protected bool m_bEnded;
	protected float m_fElapsedSeconds;
	protected ref map<int, float> m_mCharge = new map<int, float>();		//!< 0-1
	protected ref set<int> m_CloakedPlayers = new set<int>();
	protected ref map<int, float> m_mLockedUntil = new map<int, float>();	//!< m_fElapsedSeconds
	protected ref map<int, float> m_mLastToggle = new map<int, float>();	//!< world time, ms
	protected ref set<int> m_WarnedLowCharge = new set<int>();
	protected ref map<int, float> m_mLastPresence = new map<int, float>();
	protected int m_iTrophies;
	protected COA_Gamemode m_Gamemode;
	protected COA_SafestartManager m_SafestartManager;

	// Client (every machine with a player)
	protected ref map<int, ref CRF_PredatorHidden> m_mHidden = new map<int, ref CRF_PredatorHidden>();
	protected ref CRF_PredatorHidden m_OwnMask;		//!< our own mask, hidden while we look out from behind it
	protected bool m_bThermalOn;
	protected int m_iThermalCamera = -1;

	protected static CRF_PredatorGamemodeComponent s_Instance;

	protected static const string LOG = "[CRF_Predator]";
	protected static const float PRESENCE_COOLDOWN = 30;
	protected static const int LOW_CHARGE_PERCENT = 20;
	protected static const float TOGGLE_COOLDOWN_MS = 500;	//!< so mashing the key can't flicker the predator for everyone
	protected static const int THERMAL_PRIORITY = 11;		//!< the same slot the thermal sights in our mods use
	protected static const float MASK_HIDE_DISTANCE = 0.45;	//!< camera this close to the head (metres): first person, or aiming in third


	//===================================================================================
	// LIFECYCLE
	//===================================================================================

	void CRF_PredatorGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_PredatorGamemodeComponent GetInstance()
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

		// The cloak, thermal vision and the keys on every machine with a player (clients, a listen-server host,
		// Workbench). Every frame, so a cloaked predator that streams in or respawns is hidden before it's drawn.
		if (!System.IsConsoleApp())
		{
			SetEventMask(owner, EntityEvent.FRAME);

			InputManager inputManager = GetGame().GetInputManager();
			if (inputManager)
			{
				inputManager.AddActionListener("CRF_PredatorCloak", EActionTrigger.DOWN, OnCloakPressed);
				inputManager.AddActionListener("CRF_PredatorThermal", EActionTrigger.DOWN, OnThermalPressed);
			}
		}

		if (!Replication.IsServer())
			return;

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
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		IEntity localEntity = SCR_PlayerController.GetLocalControlledEntity();
		UpdateCloakVisibility(localEntity);
		UpdateOwnMask(localEntity);
		UpdateThermal(localEntity);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(Tick);
			GetGame().GetCallqueue().Remove(RemoveBody);

			InputManager inputManager = GetGame().GetInputManager();
			if (inputManager)
			{
				inputManager.RemoveActionListener("CRF_PredatorCloak", EActionTrigger.DOWN, OnCloakPressed);
				inputManager.RemoveActionListener("CRF_PredatorThermal", EActionTrigger.DOWN, OnThermalPressed);
			}
		}

		// Leave nothing hidden and no effect on the camera behind
		RevealAll();
		UpdateOwnMask(null);
		m_bThermalOn = false;
		UpdateThermal(null);

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

	//===================================================================================
	// ROUND START (server)
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
		if (m_bRoundLive || m_bEnded)
			return;

		m_bRoundLive = true;
		GetGame().GetCallqueue().CallLater(Tick, 1000, true);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			SendBriefing(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendBriefing(int playerId)
	{
		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) == m_sPredatorFaction)
		{
			Notify(playerId, "The hunt begins",
				"Cloak with [O], thermal vision with [;]. Getting hit breaks the cloak. Take your trophies.", 12);
		}
		else
		{
			Notify(playerId, "You are not alone",
				"Something is hunting both sides. It can turn invisible - you'll only see it up close.", 10);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		// A new body starts uncloaked with a full charge
		if (CRF_RadioTransmissionEvents.GetPlayerFactionKey(playerId) == m_sPredatorFaction)
		{
			m_mCharge.Set(playerId, 1);
			m_CloakedPlayers.RemoveItem(playerId);
			m_WarnedLowCharge.RemoveItem(playerId);
			Sync();
		}

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

		array<int> predators = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sPredatorFaction, predators);

		foreach (int playerId : predators)
			UpdateCharge(playerId);

		WarnPrey(predators);
		Sync();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateCharge(int playerId)
	{
		float charge = GetCharge(playerId);
		if (m_CloakedPlayers.Contains(playerId))
		{
			charge -= 1 / Math.Max(1, m_fCloakSeconds);
			if (charge <= 0)
			{
				charge = 0;
				SetCloaked(playerId, false);
				Notify(playerId, "Cloak depleted", "Recharging.", 4);
			}
			else if (charge * 100 <= LOW_CHARGE_PERCENT && !m_WarnedLowCharge.Contains(playerId))
			{
				m_WarnedLowCharge.Insert(playerId);
				Notify(playerId, "Cloak failing", string.Format("%1%% charge left.", (int)Math.Round(charge * 100)), 3);
			}
		}
		else
		{
			charge = Math.Min(1, charge + 1 / Math.Max(1, m_fCloakRechargeSeconds));
		}

		m_mCharge.Set(playerId, charge);
	}

	//------------------------------------------------------------------------------------------------
	//! "Something is watching you" for anyone close to a predator, now and then
	protected void WarnPrey(array<int> predators)
	{
		if (m_fPresenceRadius <= 0 || predators.IsEmpty())
			return;

		array<int> everyone = {};
		CRF_SpecialsHelper.GetAlivePlayers("", everyone);
		foreach (int playerId : everyone)
		{
			if (predators.Contains(playerId))
				continue;

			IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!entity || CRF_SpecialsHelper.DistanceToNearestPlayer(entity.GetOrigin(), predators) > m_fPresenceRadius)
				continue;

			float last;
			if (m_mLastPresence.Find(playerId, last) && m_fElapsedSeconds - last < PRESENCE_COOLDOWN)
				continue;

			m_mLastPresence.Set(playerId, m_fElapsedSeconds);
			Notify(playerId, "Something is watching you", "The jungle has gone quiet.", 3);
		}
	}

	//===================================================================================
	// CLOAK (server)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! A predator pressed the cloak key (SCR_PlayerController.CRF_RequestPredatorCloak). Everything is decided
	//! here: the client only asks.
	void ToggleCloak(int playerId)
	{
		if (!Replication.IsServer() || !IsPredator(CRF_RadioTransmissionEvents.GetAlivePlayerEntity(playerId)))
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		float lastToggle;
		if (m_mLastToggle.Find(playerId, lastToggle) && now - lastToggle < TOGGLE_COOLDOWN_MS)
			return;

		m_mLastToggle.Set(playerId, now);

		if (m_CloakedPlayers.Contains(playerId))
		{
			SetCloaked(playerId, false);
			Notify(playerId, "Cloak off", string.Format("%1%% charge.", GetChargePercent(playerId)), 2);
			Sync();
			return;
		}

		if (!m_bCloakEnabled || !m_bRoundLive)
		{
			Notify(playerId, "Cloak offline", "It comes online when the round starts.", 3);
			return;
		}

		float lockedUntil;
		if (m_mLockedUntil.Find(playerId, lockedUntil) && m_fElapsedSeconds < lockedUntil)
		{
			Notify(playerId, "Cloak disrupted", string.Format("Back in %1 s.", (int)Math.Ceil(lockedUntil - m_fElapsedSeconds)), 2);
			return;
		}

		int percent = GetChargePercent(playerId);
		if (percent < m_iCloakMinimumPercent)
		{
			Notify(playerId, "Not enough charge", string.Format("%1%% - the cloak needs %2%%.", percent, m_iCloakMinimumPercent), 2);
			return;
		}

		SetCloaked(playerId, true);
		Notify(playerId, "Cloak engaged", string.Format("%1%% charge.", percent), 2);
		Sync();
	}

	//------------------------------------------------------------------------------------------------
	protected void SetCloaked(int playerId, bool cloaked)
	{
		if (cloaked)
		{
			m_CloakedPlayers.Insert(playerId);
			m_WarnedLowCharge.RemoveItem(playerId);
		}
		else
		{
			m_CloakedPlayers.RemoveItem(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A hit on a predator breaks the cloak for a while (CRF_Predator_SCR_CharacterDamageManagerComponent)
	void OnPredatorHit(IEntity character)
	{
		if (!Replication.IsServer() || !IsPredator(character))
			return;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
		if (playerId <= 0)
			return;

		m_mLockedUntil.Set(playerId, m_fElapsedSeconds + m_fCloakHitLockout);
		if (!m_CloakedPlayers.Contains(playerId))
			return;

		SetCloaked(playerId, false);
		Notify(playerId, "Cloak disrupted", string.Format("You've been hit. Back in %1 s.", (int)Math.Ceil(m_fCloakHitLockout)), 3);
		Sync();
	}

	//------------------------------------------------------------------------------------------------
	//! The replicated list of cloaked predators, sent only when it changes. The dead don't stay cloaked.
	protected void Sync()
	{
		array<int> predators = {};
		CRF_SpecialsHelper.GetAlivePlayers(m_sPredatorFaction, predators);

		for (int i = m_CloakedPlayers.Count() - 1; i >= 0; i--)
		{
			if (!predators.Contains(m_CloakedPlayers.Get(i)))
				m_CloakedPlayers.Remove(i);
		}

		array<int> cloaked = {};
		foreach (int playerId : predators)
		{
			if (m_CloakedPlayers.Contains(playerId))
				cloaked.Insert(playerId);
		}

		if (SameIds(cloaked, m_aCloakedIds))
			return;

		m_aCloakedIds.Copy(cloaked);
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected bool SameIds(array<int> a, array<int> b)
	{
		if (a.Count() != b.Count())
			return false;

		foreach (int i, int value : a)
		{
			if (b[i] != value)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected float GetCharge(int playerId)
	{
		float charge;
		if (!m_mCharge.Find(playerId, charge))
		{
			charge = 1;
			m_mCharge.Set(playerId, charge);
		}

		return charge;
	}

	//------------------------------------------------------------------------------------------------
	protected int GetChargePercent(int playerId)
	{
		return (int)Math.Round(GetCharge(playerId) * 100);
	}

	//===================================================================================
	// DEATHS (server)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer())
			return;

		IEntity victim = instigatorContextData.GetVictimEntity();
		int victimId = instigatorContextData.GetVictimPlayerID();
		int killerId = instigatorContextData.GetKillerPlayerID();
		PlayerManager playerManager = GetGame().GetPlayerManager();

		if (IsPredator(victim))
		{
			// Uncloaked straight away, so everyone sees it fall
			m_CloakedPlayers.RemoveItem(victimId);
			Sync();

			if (m_bAnnounceKills)
			{
				string by = "";
				if (killerId > 0 && killerId != victimId)
					by = playerManager.GetPlayerName(killerId) + " brought it down.";
				NotifyAll("The hunter is down", by, 8);
			}

			// Its body vanishes after a moment, taking its gear with it (the deletion replicates)
			RplComponent bodyRpl = RplComponent.Cast(victim.FindComponent(RplComponent));
			if (bodyRpl && m_fBodyRemoveDelay > 0)
				GetGame().GetCallqueue().CallLater(RemoveBody, m_fBodyRemoveDelay * 1000, false, bodyRpl.Id());

			return;
		}

		if (!m_bAnnounceKills || killerId <= 0 || CRF_RadioTransmissionEvents.GetPlayerFactionKey(killerId) != m_sPredatorFaction)
			return;

		m_iTrophies++;
		NotifyAll(playerManager.GetPlayerName(victimId) + " was taken", string.Format("The hunter has %1 trophies.", m_iTrophies), 6);
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveBody(RplId bodyId)
	{
		IEntity body = COA_EntityHelper.GetEntityFromRplId(bodyId);
		if (body)
			SCR_EntityHelper.DeleteEntityAndChildren(body);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a popup for one player. Our own player on a server we're playing on (listen server, Workbench)
	//! gets it directly - an owner RPC from the server doesn't reach the server's own player.
	protected void Notify(int playerId, string text, string text2, float duration)
	{
		if (RplSession.Mode() != RplMode.Dedicated && playerId == SCR_PlayerController.GetLocalPlayerId())
		{
			SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
			if (notification)
				notification.PopupMsg(text, duration, text2);
			return;
		}

		CRF_RadioTransmissionEvents.PopupToPlayer(playerId, text, text2, duration);
	}

	//------------------------------------------------------------------------------------------------
	protected void NotifyAll(string text, string text2, float duration)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
			Notify(playerId, text, text2, duration);
	}

	//===================================================================================
	// CLIENT (every frame): hiding the cloaked, and thermal vision for the predator
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Every cloaked predator is hidden here - except our own character, from spectators, and from anyone close
	//! enough to see through the cloak. Tracked by player, so a respawn (a new body) or a deleted body is handled.
	//! Gear the predator picks up while cloaked arrives visible, so the hiding is redone every frame.
	protected void UpdateCloakVisibility(IEntity localEntity)
	{
		map<int, IEntity> wanted = new map<int, IEntity>();

		// Spectating, dead or between bodies: see everything
		if (localEntity && !COA_EntityHelper.IsSpectator(localEntity))
		{
			vector viewer = localEntity.GetOrigin();
			CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
			if (camera)
				viewer = camera.GetOrigin();

			foreach (int playerId : m_aCloakedIds)
			{
				IEntity predator = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
				if (!predator || predator == localEntity)
					continue;

				if (m_fCloakRevealDistance > 0 && vector.Distance(predator.GetOrigin(), viewer) <= m_fCloakRevealDistance)
					continue;

				wanted.Set(playerId, predator);
			}
		}

		// Show again what's no longer cloaked, and the old body of a predator that has a new one
		array<int> known = {};
		foreach (int hiddenId, CRF_PredatorHidden hidden : m_mHidden)
			known.Insert(hiddenId);

		foreach (int previousId : known)
		{
			IEntity wantedEntity;
			CRF_PredatorHidden previous = m_mHidden.Get(previousId);
			if (!wanted.Find(previousId, wantedEntity) || !previous || previous.m_Character != wantedEntity)
			{
				if (previous)
					previous.Reveal();
				m_mHidden.Remove(previousId);
			}
		}

		foreach (int cloakedId, IEntity cloakedEntity : wanted)
		{
			CRF_PredatorHidden record = m_mHidden.Get(cloakedId);
			if (!record)
			{
				record = new CRF_PredatorHidden(cloakedEntity);
				m_mHidden.Set(cloakedId, record);
			}

			record.Hide();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RevealAll()
	{
		foreach (int hiddenId, CRF_PredatorHidden hidden : m_mHidden)
		{
			if (hidden)
				hidden.Reveal();
		}

		m_mHidden.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! The predator's mask (and the night vision fitted to it) sits right in front of the first-person camera.
	//! Vanilla fades headgear when the camera gets close to the head, but that relies on the headgear's
	//! material, and this one doesn't fade - so on the predator's own machine the mask is hidden while the
	//! camera is at the head, and comes back in third person. Nobody else's view of it changes.
	protected void UpdateOwnMask(IEntity localEntity)
	{
		IEntity mask;
		bool hide;

		ChimeraCharacter character = ChimeraCharacter.Cast(localEntity);
		if (character && IsPredator(character) && character.GetCharacterController() && !character.GetCharacterController().IsDead())
		{
			EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(character.FindComponent(EquipedLoadoutStorageComponent));
			if (loadout)
				mask = loadout.GetClothFromArea(LoadoutHeadCoverArea);

			CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
			if (mask != null && camera != null)
				hide = vector.Distance(camera.GetOrigin(), GetHeadPosition(character)) <= MASK_HIDE_DISTANCE;
		}

		if (m_OwnMask && (!hide || m_OwnMask.m_Character != mask))
		{
			m_OwnMask.Reveal();
			m_OwnMask = null;
		}

		if (!hide)
			return;

		if (!m_OwnMask)
			m_OwnMask = new CRF_PredatorHidden(mask);

		m_OwnMask.Hide();
	}

	//------------------------------------------------------------------------------------------------
	//! World position of the head bone, as vanilla's head-visibility check measures it
	protected vector GetHeadPosition(ChimeraCharacter character)
	{
		Animation animation = character.GetAnimation();
		int headBone = animation.GetBoneIndex("Head");
		if (headBone < 0)
			return character.GetOrigin();

		vector headMat[4];
		animation.GetBoneMatrix(headBone, headMat);

		vector charMat[4];
		character.GetWorldTransform(charMat);
		Math3D.MatrixMultiply4(charMat, headMat, headMat);
		return headMat[3];
	}

	//------------------------------------------------------------------------------------------------
	//! The heat palette on whichever camera the predator is looking through (it changes for sights and
	//! vehicles), cleared from the old one. Only on the predator's own machine.
	protected void UpdateThermal(IEntity localEntity)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		bool predatorAlive = IsPredator(localEntity) && CRF_RadioTransmissionEvents.GetAlivePlayerEntity(SCR_PlayerController.GetLocalPlayerId()) == localEntity;
		if (!predatorAlive)
			m_bThermalOn = false;

		bool wanted = m_bThermalOn && m_bThermalEnabled && !m_sThermalMaterial.IsEmpty();
		int camera = world.GetCurrentCameraId();

		if (m_iThermalCamera >= 0 && (!wanted || camera != m_iThermalCamera))
		{
			world.SetCameraPostProcessEffect(m_iThermalCamera, THERMAL_PRIORITY, PostProcessEffectType.ThermalImaging, string.Empty);
			m_iThermalCamera = -1;
		}

		if (wanted && m_iThermalCamera != camera)
		{
			world.SetCameraPostProcessEffect(camera, THERMAL_PRIORITY, PostProcessEffectType.ThermalImaging, m_sThermalMaterial);
			m_iThermalCamera = camera;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The client only asks; the server decides (CRF_RequestPredatorCloak -> ToggleCloak)
	protected void OnCloakPressed(float value, EActionTrigger reason)
	{
		if (!IsPredator(SCR_PlayerController.GetLocalControlledEntity()))
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.CRF_RequestPredatorCloak();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnThermalPressed(float value, EActionTrigger reason)
	{
		if (!m_bThermalEnabled || !IsPredator(SCR_PlayerController.GetLocalControlledEntity()))
			return;

		m_bThermalOn = !m_bThermalOn;
		UpdateThermal(SCR_PlayerController.GetLocalControlledEntity());

		SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
		if (!notification)
			return;

		if (m_bThermalOn)
			notification.PopupMsg("Thermal vision on", 2);
		else
			notification.PopupMsg("Thermal vision off", 2);
	}

	//===================================================================================
	// HOOKS (stamina and damage overrides)
	//===================================================================================

	//! A character on the predator's faction (replicated, so the same answer on every machine)
	bool IsPredator(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character)
			return false;

		Faction faction = character.GetFaction();
		return faction && faction.GetFactionKey() == m_sPredatorFaction;
	}

	//------------------------------------------------------------------------------------------------
	//! Everywhere (the stamina hook runs on the character's owner)
	bool IsTirelessPredator(IEntity character)
	{
		return m_bTireless && IsPredator(character);
	}

	//------------------------------------------------------------------------------------------------
	//! Multiplier for damage dealt to this entity (CRF_Predator_SCR_CharacterDamageManagerComponent)
	float GetDamageScale(IEntity entity)
	{
		if (IsPredator(entity))
			return m_fDamageTaken;

		return 1;
	}
}

//------------------------------------------------------------------------------------------------
//! Client: a predator character this machine has hidden, and the parts it hid - only those come back, so items
//! the game keeps hidden itself (like those in storage) stay hidden
class CRF_PredatorHidden
{
	IEntity m_Character;
	protected ref array<IEntity> m_aParts = {};

	//------------------------------------------------------------------------------------------------
	void CRF_PredatorHidden(IEntity character)
	{
		m_Character = character;
	}

	//------------------------------------------------------------------------------------------------
	void Hide()
	{
		if (m_Character)
			HideTree(m_Character);
	}

	//------------------------------------------------------------------------------------------------
	protected void HideTree(IEntity entity)
	{
		if (entity.GetFlags() & EntityFlags.VISIBLE)
		{
			entity.ClearFlags(EntityFlags.VISIBLE, false);
			m_aParts.Insert(entity);
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			HideTree(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	void Reveal()
	{
		foreach (IEntity part : m_aParts)
		{
			if (part)
				part.SetFlags(EntityFlags.VISIBLE, false);
		}

		m_aParts.Clear();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	//! The local predator wants its cloak toggled (CRF_PredatorGamemodeComponent.OnCloakPressed). Our own
	//! player on a server we're playing on is handled directly; a client asks the server.
	void CRF_RequestPredatorCloak()
	{
		if (Replication.IsServer())
		{
			RpcAsk_CRF_TogglePredatorCloak();
			return;
		}

		Rpc(RpcAsk_CRF_TogglePredatorCloak);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CRF_TogglePredatorCloak()
	{
		CRF_PredatorGamemodeComponent predator = CRF_PredatorGamemodeComponent.GetInstance();
		if (predator)
			predator.ToggleCloak(GetPlayerId());
	}
}
