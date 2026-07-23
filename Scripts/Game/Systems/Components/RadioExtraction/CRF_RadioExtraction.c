//------------------------------------------------------------------------------------
// CRF_RadioExtraction: Lightweight lobby plant/extract interaction
// One faction can plant a bomb; the opposing faction must either defuse it or wait
// out a configurable timer to send a server extraction notification. Mirrors the
// Rush MCOM plant/defuse/detonate cycle (countdown, sounds, explosion), but every
// duration, the explosion prefab, and every sound is settable per instance.
//------------------------------------------------------------------------------------

enum CRF_ERadioExtractionSide
{
	PLANTER,    // Faction that can plant/arm the bomb
	EXTRACTOR   // Faction that can defuse the bomb and send the extraction message
}

[ComponentEditorProps(category: "Game Mode Component", description: "Lightweight lobby radio extraction interaction: one faction can plant a bomb, the other must wait out a timer to send an extraction message or defuse the bomb.")]
class CRF_RadioExtractionClass : ScriptComponentClass {}

class CRF_RadioExtraction : ScriptComponent
{
	//===================================================================================
	// ATTRIBUTES
	//===================================================================================

	// Spawn Settings
	//------------------------------------------------------------------------------------
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab spawned on the target entity; must carry an ActionsManagerComponent with the Radio Extraction actions.", category: "Radio Extraction - Spawn", params: "et")]
	ResourceName m_rBombPrefab;

	[Attribute("", UIWidgets.EditBox, "Name of the pre-placed entity in the world this prefab spawns onto.", category: "Radio Extraction - Spawn")]
	string m_sTargetEntityName;

	// Faction Settings
	//------------------------------------------------------------------------------------
	[Attribute("BLUFOR", UIWidgets.ComboBox, "The faction allowed to plant the bomb", "", category: "Radio Extraction - Factions", enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")})]
	FactionKey m_sPlanterFactionKey;

	[Attribute("OPFOR", UIWidgets.ComboBox, "The faction allowed to defuse the bomb and send the extraction message", "", category: "Radio Extraction - Factions", enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")})]
	FactionKey m_sExtractorFactionKey;

	// Timing Settings
	//------------------------------------------------------------------------------------
	[Attribute("10", UIWidgets.EditBox, "Minutes after safestart ends until the Send Extraction Message action unlocks.", category: "Radio Extraction - Timing")]
	int m_iUnlockTimeMinutes;

	[Attribute("45", UIWidgets.EditBox, "Time in seconds for the bomb's arm-to-detonation countdown.", category: "Radio Extraction - Timing")]
	int m_iBombCountdownSeconds;

	[Attribute("Extraction message sent.", UIWidgets.EditBox, "Notification text broadcast when the extraction message is sent.", category: "Radio Extraction - Timing")]
	string m_sExtractionMessageText;

	// Sound Settings — mirrors Rush's plant/defuse/tick sound setup: a central .acp
	// resource picker per sound, plus the AudioSystem event name (the "namesake")
	// defined inside that .acp, exactly as CRF_TaskHandler_PlantDefuseBomb does.
	//------------------------------------------------------------------------------------
	[Attribute("{1D6C7E5479081CAF}Sounds/Rush/planting_3D.acp", UIWidgets.ResourceNamePicker, "Sound broadcast to all clients while a player is planting the bomb.", params: "acp", category: "Radio Extraction - Sounds")]
	ResourceName m_rPlantSoundResource;

	[Attribute("RUSH_PLANTING", UIWidgets.EditBox, "AudioSystem event name for the planting sound. Must match an event defined in the sound config above.", category: "Radio Extraction - Sounds")]
	string m_sPlantSoundEvent;

	[Attribute("{1D6C7E5479081CAF}Sounds/Rush/planting_3D.acp", UIWidgets.ResourceNamePicker, "Sound broadcast to all clients while a player is defusing the bomb.", params: "acp", category: "Radio Extraction - Sounds")]
	ResourceName m_rDefuseSoundResource;

	[Attribute("RUSH_PLANTING", UIWidgets.EditBox, "AudioSystem event name for the defusing sound. Must match an event defined in the sound config above.", category: "Radio Extraction - Sounds")]
	string m_sDefuseSoundEvent;

	[Attribute("{A6BBE7DBD7C64EE6}Sounds/Rush/beep_3D.acp", UIWidgets.ResourceNamePicker, "Looping bomb ticking sound started when armed, heard by all clients.", params: "acp", category: "Radio Extraction - Sounds")]
	ResourceName m_rTickSoundResource;

	[Attribute("RUSH_BEEP", UIWidgets.EditBox, "AudioSystem event name for the ticking sound. Must match an event defined in the sound config above.", category: "Radio Extraction - Sounds")]
	string m_sTickSoundEvent;

	[Attribute("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav", UIWidgets.ResourceNamePicker, "Sound played on all clients alongside the extraction message notification.", params: "wav", category: "Radio Extraction - Sounds")]
	ResourceName m_rNotificationSound;

	// Explosion Settings — staged like Rush's MCOM destruction: a primary explosion
	// spawns immediately, secondary explosion + fire spawn 385ms later.
	//------------------------------------------------------------------------------------
	[Attribute("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et", UIWidgets.ResourceNamePicker, "Primary explosion prefab spawned immediately on detonation.", params: "et", category: "Radio Extraction - Explosion")]
	ResourceName m_rExplosionPrefabPrimary;

	[Attribute("{BCE4E0823FCFBCB7}Prefabs/Weapons/Warheads/Explosions/Explosion_AmmoRack_Large.et", UIWidgets.ResourceNamePicker, "Secondary explosion prefab spawned 385ms after detonation.", params: "et", category: "Radio Extraction - Explosion")]
	ResourceName m_rExplosionPrefabSecondary;

	[Attribute("{4BE47BA2B7E3877E}Prefabs/Systems/Fire/Wrapper_Fire_Large_Damage.et", UIWidgets.ResourceNamePicker, "Fire/smoke prefab spawned 385ms after detonation.", params: "et", category: "Radio Extraction - Explosion")]
	ResourceName m_rFirePrefab;

	//===================================================================================
	// RUNTIME / REPLICATED STATE
	//===================================================================================

	[RplProp()]
	protected bool m_bBombPlanted = false;

	[RplProp()]
	protected bool m_bCountdownActive = false;

	[RplProp()]
	protected int m_iCountdownTimeRemaining = 0;

	[RplProp()]
	protected bool m_bDetonated = false;

	[RplProp()]
	protected bool m_bExtractionUnlocked = false;

	[RplProp()]
	protected bool m_bExtractionMessageSent = false;

	[RplProp(onRplName: "ShowMessage")]
	protected string m_sMessageContent;
	protected string m_sStoredMessageContent;

	[RplProp(onRplName: "PlaySound")]
	protected string m_sSoundString;

	protected IEntity m_eBombEntity;
	protected SCR_PopUpNotification m_PopUpNotification;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;

	// Polled (not event-subscribed) safestart tracking — I dont know why but invoker wasnt working correctly, would detect safestart gone before lobby
	protected bool m_bHasSafestartStarted = false;
	protected bool m_bHasSafestartEnded = false;
	protected float m_fSafestartPollBuffer = 0;

	//===================================================================================
	// SINGLETON
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	protected static CRF_RadioExtraction m_sInstance;
	void CRF_RadioExtraction(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_RadioExtraction()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_RadioExtraction GetInstance()
	{
		return m_sInstance;
	}

	//===================================================================================
	// INITIALIZATION
	//===================================================================================

	protected int m_iSpawnRetryCount = 0;
	protected static const int SPAWN_RETRY_LIMIT = 20;
	protected static const int SPAWN_RETRY_DELAY_MS = 1000;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();

		if (!Replication.IsServer())
			return;

		SpawnBombEntity();

		// Poll safestart status once a second rather than subscribing to the invoker
		SetEventMask(owner, EntityEvent.FIXEDFRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		GetGame().GetCallqueue().Remove(CountdownTimer);
		GetGame().GetCallqueue().Remove(UnlockExtraction);
		GetGame().GetCallqueue().Remove(SpawnBombEntity);
		ClearEventMask(owner, EntityEvent.FIXEDFRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);

		if (m_fSafestartPollBuffer < 1)
		{
			m_fSafestartPollBuffer += timeSlice;
			return;
		}
		m_fSafestartPollBuffer = 0;

		if (m_bHasSafestartEnded)
			return;

		if (!m_SafestartManager)
			m_SafestartManager = CRF_SafestartManager.GetInstance();
		if (!m_SafestartManager)
			return;

		if (!m_bHasSafestartStarted)
		{
			m_bHasSafestartStarted = m_SafestartManager.GetSafestartStatus();
			return;
		}

		if (m_SafestartManager.GetSafestartStatus())
			return;

		m_bHasSafestartEnded = true;
		Print(string.Format("[CRF_RadioExtraction] Safestart ended, extraction unlocks in %1 minute(s).", m_iUnlockTimeMinutes), LogLevel.NORMAL);
		GetGame().GetCallqueue().CallLater(UnlockExtraction, m_iUnlockTimeMinutes * 60000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnBombEntity()
	{
		if (m_rBombPrefab.IsEmpty() || m_sTargetEntityName.IsEmpty())
		{
			Print("[CRF_RadioExtraction] m_rBombPrefab or m_sTargetEntityName is not set, skipping spawn.", LogLevel.WARNING);
			return;
		}

		IEntity targetEntity = GetGame().GetWorld().FindEntityByName(m_sTargetEntityName);
		if (!targetEntity)
		{
			m_iSpawnRetryCount++;
			if (m_iSpawnRetryCount > SPAWN_RETRY_LIMIT)
			{
				Print(string.Format("[CRF_RadioExtraction] Could not find target entity named '%1' after %2 attempts, giving up.", m_sTargetEntityName, SPAWN_RETRY_LIMIT), LogLevel.ERROR);
				return;
			}

			GetGame().GetCallqueue().CallLater(SpawnBombEntity, SPAWN_RETRY_DELAY_MS, false);
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		targetEntity.GetWorldTransform(spawnParams.Transform);

		SCR_TerrainHelper.OrientToTerrain(spawnParams.Transform);

		m_eBombEntity = GetGame().SpawnEntityPrefab(Resource.Load(m_rBombPrefab), GetGame().GetWorld(), spawnParams);

		if (!m_eBombEntity)
			Print(string.Format("[CRF_RadioExtraction] Failed to spawn m_rBombPrefab at target entity '%1'.", m_sTargetEntityName), LogLevel.ERROR);
	}

	//===================================================================================
	// SIDE / FACTION HELPERS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks whether the user's actual faction matches the faction assigned to the given logical side
	bool IsUserOnSide(IEntity user, CRF_ERadioExtractionSide side)
	{
		if (!user)
			return false;

		FactionKey userFactionKey;

		FactionAffiliationComponent userAffiliation = FactionAffiliationComponent.Cast(user.FindComponent(FactionAffiliationComponent));
		if (userAffiliation)
		{
			Faction userFaction = userAffiliation.GetAffiliatedFaction();
			if (userFaction)
				userFactionKey = userFaction.GetFactionKey();
		}

		if (userFactionKey.IsEmpty())
			return false;

		if (side == CRF_ERadioExtractionSide.PLANTER)
			return userFactionKey == m_sPlanterFactionKey;

		return userFactionKey == m_sExtractorFactionKey;
	}

	//===================================================================================
	// STATE ACCESSORS
	//===================================================================================

	bool IsBombPlanted() { return m_bBombPlanted; }
	bool IsCountdownActive() { return m_bCountdownActive; }
	int GetCountdownTimeRemaining() { return m_iCountdownTimeRemaining; }
	bool IsDetonated() { return m_bDetonated; }
	bool IsExtractionUnlocked() { return m_bExtractionUnlocked; }
	bool IsExtractionMessageSent() { return m_bExtractionMessageSent; }
	IEntity GetBombEntity() { return m_eBombEntity; }

	//===================================================================================
	// PLANT / DEFUSE / DETONATE
	// Called server-side only, via CRF_PlayerRplToAuthorityManager's RpcAsk_RequestRadioExtraction*
	// handlers
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Server-authoritative: arms the bomb and starts the detonation countdown
	void PlantBomb()
	{
		if (!Replication.IsServer())
			return;

		if (m_bBombPlanted || m_bDetonated)
			return;

		m_bBombPlanted = true;
		m_bCountdownActive = true;
		m_iCountdownTimeRemaining = m_iBombCountdownSeconds;

		StartTickingSound();

		GetGame().GetCallqueue().Remove(CountdownTimer);
		GetGame().GetCallqueue().CallLater(CountdownTimer, 1000, true);

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Server-authoritative: defuses the bomb and stops the countdown
	void DefuseBomb()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bBombPlanted || !m_bCountdownActive)
			return;

		m_bBombPlanted = false;
		m_bCountdownActive = false;
		m_iCountdownTimeRemaining = 0;

		GetGame().GetCallqueue().Remove(CountdownTimer);
		StopTickingSound();

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void CountdownTimer()
	{
		if (!m_bCountdownActive)
		{
			GetGame().GetCallqueue().Remove(CountdownTimer);
			return;
		}

		m_iCountdownTimeRemaining--;

		if (m_iCountdownTimeRemaining <= 0)
		{
			GetGame().GetCallqueue().Remove(CountdownTimer);
			Detonate();
			return;
		}

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Countdown reached zero without being defused: permanently blocks the extraction message
	protected void Detonate()
	{
		if (!Replication.IsServer())
			return;

		m_bBombPlanted = false;
		m_bCountdownActive = false;
		m_iCountdownTimeRemaining = 0;
		m_bDetonated = true;

		StopTickingSound();
		PlayDetonationEffects();

		m_sMessageContent = "The bomb has detonated!|10|";

		Replication.BumpMe();
		ShowMessage();
	}

	//------------------------------------------------------------------------------------------------
	//! Spawns the primary explosion immediately, then the secondary explosion + fire 385ms later
	protected void PlayDetonationEffects()
	{
		if (!m_eBombEntity)
			return;

		// Capture position into a local vector before any potential entity deletion
		vector explosionPos = m_eBombEntity.GetOrigin();

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = explosionPos;

		if (!m_rExplosionPrefabPrimary.IsEmpty())
			GetGame().SpawnEntityPrefab(Resource.Load(m_rExplosionPrefabPrimary), GetGame().GetWorld(), spawnParams);

		GetGame().GetCallqueue().CallLater(SpawnDelayedExplosionEffects, 385, false, explosionPos);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnDelayedExplosionEffects(vector explosionPos)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = explosionPos;

		if (!m_rExplosionPrefabSecondary.IsEmpty())
			GetGame().SpawnEntityPrefab(Resource.Load(m_rExplosionPrefabSecondary), GetGame().GetWorld(), spawnParams);

		if (!m_rFirePrefab.IsEmpty())
			GetGame().SpawnEntityPrefab(Resource.Load(m_rFirePrefab), GetGame().GetWorld(), spawnParams);
	}

	//===================================================================================
	// EXTRACTION MESSAGE
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	protected void UnlockExtraction()
	{
		if (!Replication.IsServer())
			return;

		m_bExtractionUnlocked = true;
		Replication.BumpMe();

		Print(string.Format("[CRF_RadioExtraction] Extraction unlocked after %1 minute(s) post-safestart.", m_iUnlockTimeMinutes), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Server-authoritative: broadcasts the configured extraction message, if allowed
	void SendExtractionMessage()
	{
		if (!Replication.IsServer())
			return;

		if (!m_bExtractionUnlocked || m_bBombPlanted || m_bDetonated || m_bExtractionMessageSent)
			return;

		m_bExtractionMessageSent = true;
		m_sMessageContent = string.Format("%1|10|", m_sExtractionMessageText);
		m_sSoundString = m_rNotificationSound;

		Replication.BumpMe();
		ShowMessage();
		PlaySound();
	}

	//===================================================================================
	// SOUND MANAGEMENT (server-side, replicated via CRF_RplBroadcastManager positional sounds)
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	void StartTickingSound()
	{
		if (!Replication.IsServer() || !m_eBombEntity || m_rTickSoundResource.IsEmpty() || m_sTickSoundEvent.IsEmpty())
			return;

		if (m_RplBroadcastManager)
			m_RplBroadcastManager.PlayPositionalSound(m_rTickSoundResource, m_sTickSoundEvent, m_eBombEntity.GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	void StopTickingSound()
	{
		if (!Replication.IsServer() || m_sTickSoundEvent.IsEmpty())
			return;

		if (m_RplBroadcastManager)
			m_RplBroadcastManager.StopPositionalSound(m_sTickSoundEvent);
	}

	//===================================================================================
	// UI MESSAGING
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Plays the replicated notification sound; called via RPL replication (mirrors Rush's PlaySound)
	protected void PlaySound()
	{
		if (!m_sSoundString.IsEmpty())
			AudioSystem.PlaySound(m_sSoundString);
	}

	//------------------------------------------------------------------------------------------------
	//! Displays the current message via popup notification; called via RPL replication
	protected void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;

		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		if (!m_PopUpNotification)
			return;

		m_sStoredMessageContent = m_sMessageContent;

		// Parse message format: "message|duration|submessage"
		array<string> messageParts = {};
		m_sMessageContent.Split("|", messageParts, false);

		if (messageParts.Count() < 2)
			return;

		string mainMessage = messageParts[0];
		float duration = messageParts[1].ToFloat();
		string subMessage;
		if (messageParts.Count() > 2)
			subMessage = messageParts[2];
		else
			subMessage = "";

		m_PopUpNotification.PopupMsg(mainMessage, duration, subMessage);
	}
}
