class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	// Time it takes for players to Init
	static const int PLAYER_INITILIZATION_TIME = 250;
	
	static ref CRF_RolesConfig m_RolesConfig;
	
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
		
		m_RolesConfig = CRF_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
			BaseContainerTools.LoadContainer(rolesConfigPath).GetResource().ToBaseContainer()));
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_RolesConfig RolesConfig()
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
	//! \param[in] spawnPointID the ID of the spawn point we want to spawn this player at (either set manually with the respawn screen or automatic if -1;
	void InitilizePlayer(int playerId, int spawnPointID = -1)
	{	
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
			playerCharacter = GetOrCreateSpectatorEntity(playerId, playerController);
	
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			
			CRF_PlayerHelper.RemovePlayerFromCurrentGroup(playerId);
		} else {
			// PLAYABLE CHARACTER PATH: Skip initial entity, spawn real character directly
			IEntity oldEntityToDelete = playerController.GetMainEntity();
			
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnPointID, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			CRF_MenuManager.GetInstance().RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (playerCharacter)
		{
			playerCharacter.DisableAI();
			DeleteOldInitialEntity(playerController, playerCharacter);
			CRF_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);
			CRF_PlayerHelper.AssignFactionToPlayer(playerController, faction);
			
			if (!CRF_EntityHelper.IsSpectator(playerCharacter))
				m_SlottingManager.AssignPlayerToGroup(playerId);
			
			RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
			if (playerRplComp)
				GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast, PLAYER_INITILIZATION_TIME, false, playerId, playerRplComp.Id());
		};
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER CHARACTER HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Get existing character or create a new one for playable roles
	//! \param[in] playerId ID of the player
	//! \param[in] spawnPointID Optional spawn location
	//! \return The character entity
	protected CRF_PlayerCharacter GetOrCreatePlayableCharacter(int playerId, int spawnPointID, out bool alreadyCreated)
	{
		alreadyCreated = true;
		CRF_PlayerCharacter playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		
		if (!playerCharacter || playerCharacter.GetCharacterController().IsDead())
		{
			alreadyCreated = false;
			
			CRF_RplBroadcastManager.GetInstance().SendCharacterLoadingScreen(playerId);
			playerCharacter = m_SlottingManager.SpawnPlayableEntity(playerId, spawnPointID);
			
			if (!playerCharacter)
			{
				Print(string.Format("[CRF_GamemodeManager] ERROR: Failed to spawn character for player %1", playerId), LogLevel.ERROR);
				return null;
			}
		}
			
		return playerCharacter;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SPECTATOR ENTITY HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Create a spectator entity in the world
	//! \param[in] playerId ID of the player
	//! \return The created spectator character
	protected CRF_PlayerCharacter GetOrCreateSpectatorEntity(int playerId, SCR_PlayerController playerController)
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
		EntitySpawnParams spawnParams = CRF_EntityHelper.CreateSpawnParams(CRF_Gamemode.GetInstance().GetGenericSpawn());
		spec = CRF_PlayerCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld(), spawnParams));
		
		if (!spec)
			return null;
		
		return spec;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Delete old initial entity if it exists (prevents ghost entities)
	//! \param[in] playerController The player controller
	//! \param[in] newCharacter The new character being assigned (don't delete this one)
	static void DeleteOldInitialEntity(SCR_PlayerController playerController, IEntity newCharacter)
	{
		if (!playerController || !newCharacter)
			return;
			
		IEntity oldEntity = playerController.GetMainEntity();
		DeleteOldInitialEntity(oldEntity, newCharacter);
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