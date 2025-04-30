class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

/**
 * CRF_GamemodeManager - Core gamemode component responsible for player initialization,
 * spectator handling, server time tracking, and moderator management.
 */
class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	// Resource path for spectator entity prefab
	const static ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
	// Replicated array of player IDs with moderator privileges
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	// Replicated server time string
	[RplProp()]
	protected string m_sServerWorldTime;
	
	// Component references
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	
	//-----------------------------------------------------------------------------
	// INITIALIZATION METHODS
	//-----------------------------------------------------------------------------
	
	/**
	 * Returns the singleton instance of this component
	 * @return Instance of CRF_GamemodeManager or null if not found
	 */
	static CRF_GamemodeManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
			
		return CRF_GamemodeManager.Cast(gameMode.FindComponent(CRF_GamemodeManager));
	}
	
	/**
	 * Initialize this component after creation
	 * @param owner The entity that owns this component
	 */
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		// Get all required manager references
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
	}
	
	//-----------------------------------------------------------------------------
	// SPECTATOR MANAGEMENT
	//-----------------------------------------------------------------------------
	
	/**
	 * Determines if a specific entity is a spectator
	 * @param entity The entity to check
	 * @return True if the entity is a spectator
	 */
	static bool IsSpectator(IEntity entity)
	{
		if (!entity)
			return false;
		
		return entity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE;
	}
	
	/**
	 * Checks if the local player is currently a spectator
	 * @return True if the local player is a spectator
	 */
	static bool IsSpectator()
	{
		IEntity mainEntity = SCR_PlayerController.GetLocalMainEntity();
		if (mainEntity)
		{
			if (mainEntity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
				return true;
		}
		
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (controlledEntity)
		{
			if (controlledEntity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
				return true;
		}

		return false;
	}

	/**
	 * Places a player into spectator mode
	 * @param playerId ID of the player to put in spectator mode
	 * @param entity Optional entity to position spectator camera at
	 */
	void EnterSpectator(int playerId, IEntity entity = null)
	{
		// Spawn spectator entity
		IEntity specEntity = GetGame().SpawnEntityPrefab(Resource.Load(SPECTATOR_RESOURCE), GetGame().GetWorld());
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		// Assign entity to player (with delay to ensure proper initialization)
		GetGame().GetCallqueue().CallLater(pc.SetInitialMainEntity, 250, false, specEntity);

		// Remove player from any existing group
		SCR_AIGroup currentGroup = m_GroupsManagerComponent.GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
		
		// Disable damage handling on spectator entity
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(
			specEntity.FindComponent(SCR_CharacterDamageManagerComponent)
		); 
		if (damManager)
			damManager.EnableDamageHandling(false);
		
		// Set player faction to spectator faction
		SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
			GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)
		);
		factionComp.RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));

		// Set camera position based on game state
		vector cameraPos[4];
		
		if (m_Gamemode.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			if (m_SlottingManager.IsPlayerInASlot(playerId) && entity)
			{
				// Position above dead entity if available
				entity.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			}
			else
			{
				// Use generic spawn position otherwise
				cameraPos[3] = m_Gamemode.m_vGenericSpawn[3];
			}
		}
		else
		{
			// Put high in sky if not in game
			cameraPos[3] = "0 10000 0";
		}

		// Send initialization to client
		m_RplBroadcastManager.SendSpecClientInit(playerId, cameraPos);
	}
	
	//-----------------------------------------------------------------------------
	// PLAYER INITIALIZATION
	//-----------------------------------------------------------------------------
	
	/**
	 * Initializes a player in the gamemode, either placing them in their slot or in spectator
	 * @param playerId ID of the player to initialize
	 * @param overrideLocation Optional vector to override spawn location
	 */
	void InitilizePlayer(int playerId, vector overrideLocation = vector.Zero)
	{
		// Check if player should be in spectator (not in slot or dead)
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId)) 
		{
			IEntity currentEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!IsSpectator(currentEntity))
				EnterSpectator(playerId);
			
			return;
		}

		// Get or create player character
		IEntity playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);

		if (!playerCharacter)
		{
			// Spawn new character using slot information
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			
			// Set spawn position
			if (overrideLocation != vector.Zero)
			{
				spawnParams.Transform[3] = overrideLocation;
			}
			else
			{
				m_SlottingManager.GetPlayerSlotVector(playerId, spawnParams.Transform);
			}
			
			// Create character entity
			ResourceName resourceName = m_SlottingManager.GetPlayerSlotResource(playerId);
			playerCharacter = GetGame().SpawnEntityPrefab(Resource.Load(resourceName), GetGame().GetWorld(), spawnParams);
		
			// Update slot with character information
			int slotID = m_SlottingManager.GetPlayerSlotID(playerId);
			RplComponent rplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
			
			m_SlottingManager.UpdateSlotCharacter(slotID, rplComp.Id());
			m_SlottingManager.UpdateSlotDeathState(slotID, false);
			
			// Mark as slot-spawned for internal tracking
			CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(
				playerCharacter.FindComponent(CRF_PlayableCharacter)
			);
			
			if (playableCharComp)
				playableCharComp.SetIsSlotSpawned();
		}

		// Assign character to player controller
		SCR_PlayerController playerController = SCR_PlayerController.Cast(
			GetGame().GetPlayerManager().GetPlayerController(playerId)
		);
		playerController.SetInitialMainEntity(playerCharacter);

		// Set player faction based on slot
		SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
		);
		factionComp.RequestFaction(m_SlottingManager.GetPlayerSlotFaction(playerId));

		// Add player to appropriate group
		int groupId = m_SlottingManager.GetPlayerSlotGroup(playerId).GetGroupID();
		if (groupId != -1)
		{
			m_GroupsManagerComponent.AddPlayerToGroup(groupId, playerId);
			SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
		}

		// Complete player initialization
		m_RplBroadcastManager.InitilizePlayer(playerId);
	}
	
	//-----------------------------------------------------------------------------
	// TIME MANAGEMENT
	//-----------------------------------------------------------------------------
	
	/**
	 * Returns the current server time string
	 * @return Formatted server time string
	 */
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}
	
	/**
	 * Sets the server time string and triggers replication
	 * @param input Time string to set
	 */
	void SetServerWorldTime(string input)
	{
		m_sServerWorldTime = input;
		Replication.BumpMe();
	}
	
	/**
	 * Updates the server time based on safe start time
	 */
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);
		Replication.BumpMe();
	}

	/**
	 * Updates the mission end countdown timer
	 * Removes itself from call queue when expired
	 */
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		// Check if timer expired
		if (totalSeconds == 0) 
		{
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			m_sServerWorldTime = "Mission Time Expired!";
		}

		Replication.BumpMe();
	}
	
	//-----------------------------------------------------------------------------
	// MODERATOR MANAGEMENT
	//-----------------------------------------------------------------------------
	
	/**
	 * Adds a player to the moderator list
	 * @param playerId ID of the player to make moderator
	 */
	void SetPlayerModerator(int playerId)
	{
		if (!Replication.IsServer())
			return;
		
		m_aModerators.Insert(playerId);
		Replication.BumpMe();
	}
	
	/**
	 * Checks if a specific player is a moderator
	 * @param playerId ID of the player to check
	 * @return True if the player is a moderator
	 */
	bool IsModerator(int playerId)
	{
		return m_aModerators.Contains(playerId);
	}
	
	/**
	 * Checks if the local player is a moderator
	 * @return True if the local player is a moderator
	 */
	bool IsModerator()
	{
		return m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
}
