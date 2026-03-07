class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Time it takes for players to Init
	static const int PLAYER_INITILIZATION_TIME = 250;
	
	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	static ref CRF_GearScriptRolesConfig m_RolesConfig;
	
	protected CRF_SlottingManager m_SlottingManager;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		// Initialize all required manager references
		InitializeManagers();
		LoadConfigurations();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Load necessary configurations for gearscript
	protected void LoadConfigurations()
	{
		ResourceName rolesConfigPath;
		if (!CVON_VONGameModeComponent.GetInstance())
			  rolesConfigPath = "{4388548E9F600148}Configs/Gearscripts/CRF_Global_Roles_Config.conf";
		else
			rolesConfigPath = "{F04F02DBFC65553E}Configs/Gearscripts/Additional Configs/CRF_CVON_Global_Roles_Config.conf";
		
		m_RolesConfig = CRF_GearScriptRolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(rolesConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GearScriptRolesConfig RolesConfig()
	{
		return m_RolesConfig;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize all manager references needed for this component
	protected void InitializeManagers()
	{
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initialize a player into the game either as a playable character or spectator
	//! \param[in] playerId ID of the player to initialize
	//! \param[in] spawnLocation Location to spawn the player (Use "CRF_EntityHelper.ZERO_SPAWN_VECTOR" as the input to have players spawn at their original slot location)
	void InitilizePlayer(int playerId, vector spawnLocation[4])
	{
		if (!CRF_EntityHelper.IsValidSpawnVector(spawnLocation[3]) && spawnLocation != CRF_EntityHelper.ZERO_SPAWN_VECTOR)
		{
			Print(string.Format("[CRF ERROR]: %1 DOESN'T HAVE VALID SPAWN VECTOR!", playerId), LogLevel.ERROR);
			return;
		};
		
		if (playerId <= 0)
			return;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
			
		CRF_PlayerCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			// SPECTATOR PATH: Create initial entity for spectators
			playerCharacter = CreateSpectatorEntity(CRF_EntityHelper.ZERO_SPAWN_VECTOR);
	
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			
			CRF_PlayerHelper.RemovePlayerFromCurrentGroup(playerId);
		} 
		else 
		{
			// PLAYABLE CHARACTER PATH: Skip initial entity, spawn real character directly
			// This optimization eliminates 50% of entity spawns (no temporary initial entities)
			
			// CRITICAL: Capture old entity BEFORE spawning new one
			// GetOrCreatePlayableCharacter() calls RequestSpawn() which immediately assigns the new entity
			// After that, GetMainEntity() will return the NEW entity, not the old spectator
			IEntity oldEntityToDelete = playerController.GetMainEntity();
			
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnLocation, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			// ALWAYS clean up old spectator/initial entities when assigning a new playable character
			// This prevents ghost spectator entities from accumulating when transitioning from spectator mode
			// DeleteOldInitialEntity has built-in safety checks to avoid deleting wrong entities
			DeleteOldInitialEntity(oldEntityToDelete, playerCharacter);
			
			CRF_MenuManager.GetInstance().RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (playerCharacter)
		{
			playerCharacter.DisableAI();
			CRF_PlayerHelper.AssignFactionToPlayer(playerController, faction);
			GetGame().GetCallqueue().CallLater(InitilizePlayerCharacter, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME, false, playerId, playerController, playerCharacter);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	//! Assign the player to the set entity
	//! \param[in] playerId ID of the player
	//! \param[in] playerController controller of the player
	//! \param[in] playerCharacter entity the player will take
	protected void InitilizePlayerCharacter(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter)
	{
		// Validate that player is still connected before proceeding
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
			
		// Validate that the character still exists
		if (!playerCharacter)
			return;
		
		// Delete the old initial entity BEFORE assigning new character
		// This prevents "ghost" entities
		DeleteOldInitialEntity(playerController, playerCharacter);
			
		CRF_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);
		
		// Wait a frame for the entity assignment to take effect, then verify success
		GetGame().GetCallqueue().Call(VerifyCharacterAssignment, playerId, playerController, playerCharacter);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Verify that character assignment was successful and complete initialization
	//! \param[in] playerId ID of the player
	//! \param[in] playerController controller of the player
	//! \param[in] playerCharacter entity the player should control
	protected void VerifyCharacterAssignment(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter)
	{
		// Validate that player is still connected
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
			
		// Check if character assignment was successful
		IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		// If player is still controlling the initial entity, retry the assignment
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == CRF_EntityHelper.GetSpectatorResource() && (playerCharacter.GetPrefabData().GetPrefabName() != CRF_EntityHelper.GetSpectatorResource()))
		{
			// Delete the old initial entity BEFORE assigning new character
			// This prevents "ghost" entities
			DeleteOldInitialEntity(playerController, playerCharacter);
			
			// Force reassign the character
			CRF_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);
			
			// Schedule another verification attempt
			GetGame().GetCallqueue().CallLater(VerifyCharacterAssignment, 100, false, playerId, playerController, playerCharacter);
			return;
		}
		
		// Assignment successful, complete initialization
		if (playerCharacter.GetPrefabData().GetPrefabName() != CRF_EntityHelper.GetSpectatorResource())
			m_SlottingManager.AssignPlayerToGroup(playerId);
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));

		GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast, PLAYER_INITILIZATION_TIME, false, playerId, playerRplComp.Id());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get existing character or create a new one for playable roles
	//! \param[in] playerId ID of the player
	//! \param[in] overrideLocation Optional spawn location
	//! \return The character entity
	protected CRF_PlayerCharacter GetOrCreatePlayableCharacter(int playerId, vector overrideLocation[4], out bool alreadyCreated)
	{
		alreadyCreated = true;
		CRF_PlayerCharacter playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		
		if (!playerCharacter || playerCharacter.GetCharacterController().IsDead())
		{
			alreadyCreated = false;
			
			CRF_RplBroadcastManager.GetInstance().SendCharacterLoadingScreen(playerId);
			playerCharacter = m_SlottingManager.SpawnPlayableEntity(playerId, overrideLocation);
			
			if (!playerCharacter)
			{
				Print(string.Format("[CRF_GamemodeManager] ERROR: Failed to spawn character for player %1", playerId), LogLevel.ERROR);
				return null;
			}
		}
		
		// Always notify the data collector when a player is initialized into a character,
		// whether newly spawned or reusing an existing entity (e.g. first initialization at game start).
		// This ensures distance tracking modules attach their CompartmentEntered/Left invokers.
		SCR_DataCollectorComponent dc = GetGame().GetDataCollector();
		if (dc)
			dc.NotifyPlayerSpawned(playerId, playerCharacter);
			
		return playerCharacter;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SPECTATOR ENTITY HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Create a spectator entity in the world
	//! \return The created spectator character
	protected CRF_PlayerCharacter CreateSpectatorEntity(vector spawnLocation[4])
	{
		CRF_PlayerCharacter spec = CRF_PlayerCharacter.Cast(playerController.GetMainEntity());
		if (spec && CRF_EntityHelper.IsSpectator(spec))
		{
			if (!CRF_DamageHelper.CheckIfEntityAlive(spec))
				SCR_EntityHelper.DeleteEntityAndChildren(spec);
			else
				return spec;
		}
		
		Resource spectatorRes = Resource.Load(CRF_EntityHelper.GetSpectatorResource());
		CRF_PlayerCharacter spec = CRF_PlayerCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld(), CRF_EntityHelper.CreateSpawnParams(spawnLocation)));
		
		return spec;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Delete old initial entity if it exists (prevents ghost entities)
	//! \param[in] oldEntity The old entity to check and potentially delete
	//! \param[in] newCharacter The new character being assigned (don't delete this one)
	static void DeleteOldInitialEntity(IEntity oldEntity, IEntity newCharacter)
	{
		if (!oldEntity || oldEntity == newCharacter)
			return;
		
		// Check if old entity is an initial entity (spectator prefab)
		if (CRF_EntityHelper.IsSpectator(oldEntity))
		{
			// Log deletion for debugging
			int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(oldEntity);
			Print(string.Format("[CRF] Deleting ghost spectator entity for player %1 at position %2", 
				playerId, 
				oldEntity.GetOrigin()), 
				LogLevel.VERBOSE);
			
			// Delete immediately to prevent replication
			SCR_EntityHelper.DeleteEntityAndChildren(oldEntity);
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_GamemodeManager m_sInstance;
	void CRF_GamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeManager GetInstance()
	{
		return m_sInstance;
	}
}