class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	protected ref CRF_ResourceCache m_ResourceCache;
	
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	protected CRF_RplBroadcastManager m_RplBroadcastManager;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_MenuManager m_MenuManager;
	protected CRF_Gamemode m_Gamemode;
	
	protected const int STATS_TRACKING_INIT_RETRY_DELAY_MS = 250;
	protected const int STATS_TRACKING_INIT_MAX_RETRIES = 20;

	protected const int GROUP_ASSIGN_RETRY_DELAY_MS = 100;
	protected const int GROUP_ASSIGN_MAX_RETRIES = 30;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if (RplSession.Mode() != RplMode.Client)
		{
			// Initialize all required manager references
			InitializeManagers();
		
			m_ResourceCache = new CRF_ResourceCache;
			GetGame().GetCallqueue().Call(m_ResourceCache.PreLoadCharacterResources);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize all manager references needed for this component
	protected void InitializeManagers()
	{
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_RplBroadcastManager = CRF_RplBroadcastManager.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_RespawnManager = CRF_RespawnManager.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		m_Gamemode = CRF_Gamemode.GetInstance();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INITIALIZATION
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initialize a player into the game either as a playable character or spectator
	//! \param[in] playerId ID of the player to initialize
	//! \param[in] spawnPointID the ID of the spawn point we want to spawn this player at (either set manually with the respawn screen or automatic if -1;
	//! \param[in] entityRplID the rplID of the entity  we want to spawn this player at (either set manually or automatic if invalid rpl id;
	bool InitilizePlayer(int playerId, int spawnPointID = -1, RplId entityRplID = RplId.Invalid())
	{	
		if (playerId <= 0)
			return true;
		
		if (!EnsureManagersReady())
			return false;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return false;
			
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
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnPointID, entityRplID, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			m_MenuManager.RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (!playerCharacter)
			return false;
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (!playerRplComp)
			return false;
		
		if (playerCharacter && playerRplComp)
		{
			playerCharacter.DisableAI();
			CRF_PlayerHelper.AssignFactionToPlayer(playerController, faction);
			CRF_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);
			
			if (!CRF_EntityHelper.IsSpectator(playerCharacter))
			{
				// Group affiliation drives nametag visibility, but SCR_PlayerControllerGroupComponent
				// isn't always resolvable immediately after SetInitialMainEntity (component/replication
				// init order). Retry until it's ready instead of guessing a fixed delay.
				ScheduleAssignPlayerToGroup(playerId, playerRplComp.Id(), 0);

				// Notify the CRF-native stats manager so it begins tracking this player.
				// Retry briefly in case component init/replication order delays availability.
				TryNotifyStatsManager(playerId, playerRplComp.Id(), 0);
			}
			else
			{
				//Sends the player the respawn screen if they reconnect while dead
				if (m_SlottingManager.IsPlayerInASlot(playerId) && m_SlottingManager.IsPlayerConsideredDead(playerId) && m_RespawnManager.CanPlayerRespawn(playerCharacter, faction.GetFactionKey(), playerId))
					m_RplBroadcastManager.SendRespawnScreen(playerId);
			}

			m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerRplComp.Id());
		};

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Resolve delayed stats-manager availability and delayed replicated-entity availability.
	protected void TryNotifyStatsManager(int playerId, RplId playerEntityRplId, int attempt)
	{
		if (playerId <= 0 || playerEntityRplId == RplId.Invalid())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			return;

		CRF_ServerStatsManager statsManager = CRF_ServerStatsManager.GetInstance();
		IEntity playerEntity = null;

		Managed replicatedItem = Replication.FindItem(playerEntityRplId);
		if (replicatedItem)
		{
			RplComponent playerRplComp = RplComponent.Cast(replicatedItem);
			if (playerRplComp)
				playerEntity = playerRplComp.GetEntity();
		}

		if (statsManager && playerEntity)
		{
			if (playerManager.GetPlayerControlledEntity(playerId) != playerEntity)
				return;

			statsManager.NotifyPlayerSpawned(playerId, playerEntity);
			return;
		}

		if (attempt + 1 >= STATS_TRACKING_INIT_MAX_RETRIES)
		{
			Print(string.Format("[CRF_GamemodeManager] WARNING: Failed to initialize stats tracking for player %1 after %2 attempts", playerId, STATS_TRACKING_INIT_MAX_RETRIES), LogLevel.WARNING);
			return;
		}

		GetGame().GetCallqueue().CallLater(TryNotifyStatsManager, STATS_TRACKING_INIT_RETRY_DELAY_MS, false, playerId, playerEntityRplId, attempt + 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Re-acquire manager references if init ordering delayed singleton availability.
	protected bool EnsureManagersReady()
	{
		if (!m_RplBroadcastManager || !m_SlottingManager || !m_RespawnManager || !m_MenuManager || !m_Gamemode)
			InitializeManagers();

		return m_RplBroadcastManager && m_SlottingManager && m_RespawnManager && m_MenuManager && m_Gamemode;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER CHARACTER HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Get existing character or create a new one for playable roles
	//! \param[in] playerId ID of the player
	//! \param[in] spawnPointID Optional spawn location
	//! \return The character entity
	protected CRF_PlayerCharacter GetOrCreatePlayableCharacter(int playerId, int spawnPointID, RplId entityRplID, out bool alreadyCreated)
	{
		alreadyCreated = true;
		CRF_PlayerCharacter playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		
		if (!playerCharacter || playerCharacter.GetCharacterController().IsDead())
		{
			alreadyCreated = false;
			
			m_RplBroadcastManager.SendCharacterLoadingScreen(playerId);
			playerCharacter = SpawnPlayableCharacter(playerId, spawnPointID, entityRplID);
			
			if (!playerCharacter)
				Print(string.Format("[CRF_GamemodeManager] ERROR: Failed to spawn character for player %1", playerId), LogLevel.ERROR);
		}
			
		return playerCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create a new character for player
	//! \param[in] playerId ID of the player
	//! \param[in] spawnPointID spawn location
	//! \return The character entity
	protected CRF_PlayerCharacter SpawnPlayableCharacter(int playerId, int spawnPointID, RplId EntityRplID)
	{
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		if (slotId < 0)
			return null;
			
		ResourceName resourceName = m_SlottingManager.GetPlayerSlotResource(playerId);
		if (resourceName.IsEmpty())
			return null;
		
		CRF_SpawnPointData spawnPointData;
		
		if (spawnPointID == -1 && EntityRplID == RplId.Invalid())
			spawnPointData = m_RespawnManager.FindInitalFactionSpawnpoint(m_SlottingManager.GetPlayerSlotFaction(playerId).GetFactionKey(), m_SlottingManager.GetPlayerSlotGroup(playerId));
		else if (EntityRplID == RplId.Invalid())
			spawnPointData = m_RespawnManager.GetSpawnPoint(spawnPointID);
		else
		{
			spawnPointData = new CRF_SpawnPointData();
			spawnPointData.SetSpawnPointEntity(EntityRplID);
		}

		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		GetSafeSpawnTransform(spawnPointData, spawnParams.Transform);
		
		CRF_PlayerCharacter playerCharacter = CRF_PlayerCharacter.Cast(
			GetGame().SpawnEntityPrefab(m_ResourceCache.GetCachedResource(resourceName), GetGame().GetWorld(), spawnParams)
		);
		
		if (!playerCharacter)
			return null;
		
		// Update character faction
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(playerCharacter.FindComponent(FactionAffiliationComponent));
		facComp.SetAffiliatedFaction(m_SlottingManager.GetPlayerSlotFaction(playerId));
	
		// Update slot data
		RplComponent charRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (charRplComp)
			m_SlottingManager.UpdateSlotCharacter(slotId, charRplComp.Id());
		
		return playerCharacter;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SPECTATOR CHARACTER HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Create a spectator entity in the world
	//! \param[in] playerId ID of the player
	//! \return The created spectator character
	protected CRF_PlayerCharacter GetOrCreateSpectatorEntity(int playerId, SCR_PlayerController playerController)
	{
		CRF_PlayerCharacter playerSpectator = CRF_PlayerCharacter.Cast(playerController.GetMainEntity());
		
		if (playerSpectator && CRF_EntityHelper.IsSpectator(playerSpectator))
		{
			if (!CRF_DamageHelper.CheckIfEntityAlive(playerSpectator))
				SCR_EntityHelper.DeleteEntityAndChildren(playerSpectator);
			else
				return playerSpectator;
		}
		
		Resource spectatorRes = Resource.Load(CRF_EntityHelper.GetSpectatorResource());
		EntitySpawnParams spawnParams = CRF_EntityHelper.CreateSpawnParams(m_Gamemode.GetGenericSpawn());
		
		playerSpectator = CRF_PlayerCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld(), spawnParams));
		
		if (!playerSpectator)
			Print(string.Format("[CRF_GamemodeManager] ERROR: Failed to spawn spectator for player %1", playerId), LogLevel.ERROR);
		
		return playerSpectator;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLAYER INIT HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Utilize spawn point information to get positional data for spawning a character entity
	//! \param[in] spawnPointData data of the spawn point we are spawning at
	//! \param[out] trasnformOut spawn location
	protected void GetSafeSpawnTransform(CRF_SpawnPointData spawnPointData, out vector trasnformOut[4])
	{
		vector baseTransform[4];
		IEntity spawnPointEnt = CRF_EntityHelper.GetEntityFromRplId(spawnPointData.GetSpawnPointEntity());
		if (spawnPointEnt)
			spawnPointEnt.GetWorldTransform(baseTransform);
		
		// Add random offset to prevent exact position overlap
		float angle = Math.RandomFloat01() * Math.PI2;
		float dist = Math.RandomFloat01() * spawnPointData.GetSpawnPointRadius();
		vector offset = Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist);
		
		baseTransform[3] = baseTransform[3] + offset;
		
		vector surface;
		// Snap to terrain geometry
		if (spawnPointData.GetIfSpawnPointSafetyCheck())
			SCR_TerrainHelper.SnapToGeometry(surface, baseTransform[3], {}, GetGame().GetWorld());
		
		if (surface != vector.Zero && spawnPointData.GetIfSpawnPointConformsToTerrain())
		{
			baseTransform[3] = surface;
			SCR_TerrainHelper.OrientToTerrain(baseTransform);
		}
		
		trasnformOut = baseTransform;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Poll until group affiliation can actually be assigned, then do it exactly once.
	//! Group affiliation is what drives nametag visibility, but the group/component readiness
	//! can lag behind character possession, so this retries instead of assuming a fixed delay
	//! is always enough. Kept separate from AssignPlayerToGroup() itself so overrides of that
	//! method (see CRF_CSI_ColorTeam.c) only fire once, on success, rather than once per retry.
	//! \param[in] playerId ID of the player to assign
	//! \param[in] playerEntityRplId RplId of the character this assignment was issued for, so a
	//!            stale retry (player died/respawned again before this resolved) doesn't fire late
	//! \param[in] attempt current retry count
	protected void ScheduleAssignPlayerToGroup(int playerId, RplId playerEntityRplId, int attempt)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !playerManager.IsPlayerConnected(playerId))
			return;

		// If the player already moved on to a different character (e.g. respawned again
		// before this resolved), let that newer InitilizePlayer call own the group assignment.
		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (!controlledEntity)
			return;

		RplComponent controlledRplComp = RplComponent.Cast(controlledEntity.FindComponent(RplComponent));
		if (!controlledRplComp || controlledRplComp.Id() != playerEntityRplId)
			return;

		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		int groupId = -1;
		if (group)
			groupId = group.GetGroupID();

		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);

		if (!group || groupId == -1 || !groupComponent)
		{
			if (attempt + 1 >= GROUP_ASSIGN_MAX_RETRIES)
			{
				Print(string.Format("[CRF_GamemodeManager] WARNING: Failed to assign player %1 to group after %2 attempts (nametags may not display)", playerId, GROUP_ASSIGN_MAX_RETRIES), LogLevel.WARNING);
				return;
			}

			GetGame().GetCallqueue().CallLater(ScheduleAssignPlayerToGroup, GROUP_ASSIGN_RETRY_DELAY_MS, false, playerId, playerEntityRplId, attempt + 1);
			return;
		}

		AssignPlayerToGroup(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Assign player to their slotted group. Only called once dependencies are confirmed ready
	//! (see ScheduleAssignPlayerToGroup).
	//! \param[in] playerId ID of the player to assign
	protected void AssignPlayerToGroup(int playerId)
	{
		SCR_AIGroup group = m_SlottingManager.GetPlayerSlotGroup(playerId);
		if (!group)
			return;

		int groupId = group.GetGroupID();
		if (groupId == -1)
			return;

		m_GroupsManagerComponent.AddPlayerToGroup(groupId, playerId);

		SCR_PlayerControllerGroupComponent groupComponent = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId);
		if (groupComponent)
			groupComponent.RPC_AskJoinGroup(groupId);
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
	void ~CRF_GamemodeManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_GamemodeManager GetInstance()
	{
		return m_sInstance;
	}
}