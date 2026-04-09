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
	[RplProp()] int m_iGunsDestroyed = 0;
	[RplProp()] int m_iActiveObjective = 0;
	[RplProp()] int m_iTimeOnObjective = 61;
	//0 Relay Station
	//1 SME Church
	//2 Town south of SME
	
	void DestroyGun()
	{
		m_iGunsDestroyed++;
		Replication.BumpMe();
	}
	
	void AdvanceObjective()
	{
		m_iActiveObjective++;
		Replication.BumpMe();
	}
	
	void SetTime(int time)
	{
		m_iTimeOnObjective = time;
		Replication.BumpMe();
	}

	
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
			
			m_MenuManager.RemovePlayerFromAnyChannel(playerId, false);
		}
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));
		if (playerCharacter && playerRplComp)
		{
			playerCharacter.DisableAI();
			DeleteOldInitialEntity(playerController, playerCharacter);
			CRF_PlayerHelper.AssignFactionToPlayer(playerController, faction);
			bool requestSpawnUsed = CRF_PlayerHelper.AssignCharacterToPlayer(playerController, playerCharacter);
			
			if (!CRF_EntityHelper.IsSpectator(playerCharacter))
			{
				GetGame().GetCallqueue().CallLater(AssignPlayerToGroup, 350, false, playerId); // Need a delay here to fix nametags not showing up sometimes, 350ms is just a arbitrary value - Njpatman

				// Only notify data-collector modules manually when SetInitialMainEntity was used.
				// When RequestSpawn is used, OnPlayerSpawnFinalize_S fires automatically and
				// delegates to NotifyPlayerSpawned — calling it twice would cause duplicate
				// AddInvokers registrations and incorrect stat tracking.
				if (!requestSpawnUsed)
				{
					SCR_DataCollectorComponent dataCollector = SCR_DataCollectorComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_DataCollectorComponent));
					if (dataCollector)
						dataCollector.NotifyPlayerSpawned(playerId, playerCharacter);
				}
			}

			m_RplBroadcastManager.InitilizePlayerBroadcast(playerId, playerRplComp.Id());
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
			
			m_RplBroadcastManager.SendCharacterLoadingScreen(playerId);
			playerCharacter = SpawnPlayableCharacter(playerId, spawnPointID);
			
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
	protected CRF_PlayerCharacter SpawnPlayableCharacter(int playerId, int spawnPointID)
	{
		int slotId = m_SlottingManager.GetPlayerSlotID(playerId);
		if (slotId < 0)
			return null;
			
		ResourceName resourceName = m_SlottingManager.GetPlayerSlotResource(playerId);
		if (resourceName.IsEmpty())
			return null;
		
		CRF_SpawnPointData spawnPointData;
		
		if (spawnPointID == -1)
			spawnPointData = m_RespawnManager.FindInitalFactionSpawnpoint(m_SlottingManager.GetPlayerSlotFaction(playerId).GetFactionKey(), m_SlottingManager.GetPlayerSlotGroup(playerId));
		else
			spawnPointData = m_RespawnManager.GetSpawnPoint(spawnPointID);
		
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
	
	//------------------------------------------------------------------------------------------------
	//! Delete old initial entity if it exists (prevents ghost entities)
	//! \param[in] oldEntity The old entity to check and potentially delete
	//! \param[in] newCharacter The new character being assigned (don't delete this one)
	protected void DeleteOldInitialEntity(SCR_PlayerController playerController, IEntity newCharacter)
	{
		if (!playerController || !newCharacter)
			return;
		
		IEntity oldEntity = playerController.GetMainEntity();
		if (!oldEntity || oldEntity == newCharacter)
			return;
		
		// Check if old entity is an initial entity (spectator prefab)
		if (CRF_EntityHelper.IsSpectator(oldEntity))
			SCR_EntityHelper.DeleteEntityAndChildren(oldEntity);
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
	//! Assign player to their slotted group
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
	static CRF_GamemodeManager GetInstance()
	{
		return m_sInstance;
	}
}