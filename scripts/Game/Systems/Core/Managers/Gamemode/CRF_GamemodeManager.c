class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	// Spectator resource to use
	static const ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
	// Time it takes for players to Init
	static const int PLAYER_INITILIZATION_TIME = 250;
	
	[RplProp()]
	ref array<int> m_aModerators = {}; 
	
	[RplProp()]
	ref array<int> m_aDonators = {};
	
	[RplProp()]
	protected string m_sServerWorldTime;
	
	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	static ref CRF_GearScriptRolesConfig m_RolesConfig;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	
	protected static CRF_GamemodeManager m_sInstance;
	
	// NEVER EVER SPAWN AN ENT WITH A PURE 0 WORLD VECTOR OR ELSE I WILL CASTRATE YOU I STG - Njpatman
	static const vector ZERO_SPAWN_VECTOR[4] = { "1 0 0", "0 1 0", "0 0 1", "0 0 0" };
	
	ref array<IEntity> m_aDeadBodies = {};
	protected ref array<IEntity> m_aForwardDeployZones = {};
	protected ref array<ref CRF_ForwardDeployRequest> m_aForwardDeployRequests = {};
	
	void CRF_GamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)	
	{
		m_sInstance = this;
	}
	
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		
		m_aDeadBodies.Insert(instigatorContextData.GetVictimEntity());
	}
	
	void CleanUpBodies()
	{
		array<IEntity> bodiesToRemove = new array<IEntity>();
		bodiesToRemove.Reserve(m_aDeadBodies.Count()); // Pre-allocate capacity for performance
		
		foreach (IEntity body: m_aDeadBodies)
		{
			if (!body)
				continue;
			
			if (!GetGame().GetWorld().QueryEntitiesBySphere(body.GetOrigin(), 30, CleanUpBodyCallback, null))
				continue;

			bodiesToRemove.Insert(body);
		}
		
		int delay = 1;
		foreach (IEntity body: bodiesToRemove)
		{
			m_aDeadBodies.RemoveItem(body);
			//Lets not delete 100s of entities in one frame now
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 100 * delay, false, body);
			delay++;
		}
	}
	
	bool CleanUpBodyCallback(IEntity entity)
	{
		if (ChimeraCharacter.Cast(entity))
		{
			//Is this character dead
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (damageManager)
			{
				if (damageManager.GetState() == EDamageState.DESTROYED)
					return true;
				else
					return false;
			}
		}
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Get the spectator resource name
	* @param vectorToCheck vector to check
	* @return ResourceName of the spectator entity
	*/
	static bool IsValidSpawnVector(vector vectorToCheck)
	{	
		bool finalcheck = false;
		bool zeroCheck = (vector.Distance(ZERO_SPAWN_VECTOR[3], vectorToCheck) > 5);
		bool tenCheck = (vector.Distance("0 10000 0", vectorToCheck) > 5);
		bool negCheck = (vectorToCheck[1] >= 0);
		
		if (zeroCheck && tenCheck && negCheck)
			finalcheck = true;
		
		return finalcheck;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Get the spectator resource name
	* @return ResourceName of the spectator entity
	*/
	static ResourceName GetSpectatorResource()
	{
		return SPECTATOR_RESOURCE;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Get the instance of the GamemodeManager from the current game mode
	* @return Instance of the GamemodeManager, null if not found
	*/
	static CRF_GamemodeManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		// Initialize all required manager references
		InitializeManagers();
		LoadConfigurations();
	}
	
	//Needed so when we teleport players/vehicles the aren't spawning on top of each other.
	float m_fBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	    super.EOnFrame(owner, timeSlice);
	    m_fBuffer += timeSlice;
	    if (m_fBuffer > 0.1)
	    {
	        m_fBuffer = 0;
	        if (m_aForwardDeployRequests.Count() > 0)
	        {
	            CRF_ForwardDeployRequest request = m_aForwardDeployRequests.Get(0);
	            if (request)
	            {
	                PerformForwardDeploy(request.m_iPlayerId, request.m_vTransform);
	                m_aForwardDeployRequests.RemoveOrdered(0);
	            }
	        }
	        if (m_aForwardDeployRequests.Count() == 0)
	            ClearEventMask(owner, EntityEvent.FRAME);
	    }
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load necessary configurations for gearscript
	 */
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
	/**
	* Initialize all manager references needed for this component
	*/
	protected void InitializeManagers()
	{
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_GroupsManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_AdminMenuManager = CRF_AdminMenuManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	// SPECTATOR MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	* Check if a given entity is a spectator
	* @param entity Entity to check
	* @return True if entity is a spectator, false otherwise
	*/
	static bool IsSpectator(IEntity entity)
	{
		if (!entity)
			return false;
		
		return entity.GetPrefabData().GetPrefabName() == GetSpectatorResource();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if the local player is a spectator
	* @return True if local player is a spectator, false otherwise
	*/
	static bool IsSpectator()
	{
		IEntity mainEntity = SCR_PlayerController.GetLocalMainEntity();
		if (mainEntity && mainEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource())
			return true;
		
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource())
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// PLAYER INITIALIZATION
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/**
	* Initialize a player into the game either as a playable character or spectator
	* @param playerId ID of the player to initialize
	* @param spawnLocation Location to spawn the player (Use "CRF_GamemodeManager.ZERO_SPAWN_VECTOR" as the input to have players spawn at their original slot location)
	*/
	void InitilizePlayer(int playerId, vector spawnLocation[4])
	{
		if (!IsValidSpawnVector(spawnLocation[3]) && spawnLocation != ZERO_SPAWN_VECTOR)
		{
			Print(string.Format("[CRF ERROR]: %1 DOESN'T HAVE VALID SPAWN VECTOR!", playerId), LogLevel.ERROR);
			return;
		};
		
		if (playerId <= 0)
			return;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
			
		SCR_ChimeraCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			// SPECTATOR PATH: Create initial entity for spectators
			playerCharacter = CreateSpectatorEntity(CRF_GamemodeManager.ZERO_SPAWN_VECTOR);
	
			
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			
			RemovePlayerFromCurrentGroup(playerId);
			DisableDamageForSpectator(playerCharacter);
		} 
		else 
		{
			// PLAYABLE CHARACTER PATH: Skip initial entity, spawn real character directly
			// This optimization eliminates 50% of entity spawns (no temporary initial entities)
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnLocation, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
			
			// If character already existed (respawn case), clean up any old initial/spectator entity
			if (alreadyCreated)
			{
				DeleteOldInitialEntity(playerController, playerCharacter);
			}
			
			CRF_MenuManager.GetInstance().RemovePlayerFromAnyChannel(playerId, false);
		}
		
		if (playerCharacter)
		{
			AssignFactionToPlayer(playerController, faction);
			GetGame().GetCallqueue().CallLater(InitilizePlayerCharacter, CRF_GamemodeManager.PLAYER_INITILIZATION_TIME, false, playerId, playerController, playerCharacter);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign the player to the set entity
	* @param playerId ID of the player
	* @param playerController controller of the player
	* @param playerCharacter entity the player will take
	*/
	protected void InitilizePlayerCharacter(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter)
	{
		// Validate that player is still connected before proceeding
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
			
		// Validate that the character still exists
		if (!playerCharacter)
			return;
			
		AssignCharacterToPlayer(playerController, playerCharacter);
		
		// Wait a frame for the entity assignment to take effect, then verify success
		GetGame().GetCallqueue().Call(VerifyCharacterAssignment, playerId, playerController, playerCharacter);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Verify that character assignment was successful and complete initialization
	* @param playerId ID of the player
	* @param playerController controller of the player
	* @param playerCharacter entity the player should control
	*/
	protected void VerifyCharacterAssignment(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter)
	{
		// Validate that player is still connected
		if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
			return;
			
		// Check if character assignment was successful
		IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		// If player is still controlling the initial entity, retry the assignment
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == GetSpectatorResource() && (playerCharacter.GetPrefabData().GetPrefabName() != GetSpectatorResource()))
		{
			// Force reassign the character
			AssignCharacterToPlayer(playerController, playerCharacter);
			
			// Schedule another verification attempt
			GetGame().GetCallqueue().CallLater(VerifyCharacterAssignment, 100, false, playerId, playerController, playerCharacter);
			return;
		}
		
		// Assignment successful, complete initialization
		if (playerCharacter.GetPrefabData().GetPrefabName() != GetSpectatorResource())
			AssignPlayerToGroup(playerId);
		
		RplComponent playerRplComp = RplComponent.Cast(playerCharacter.FindComponent(RplComponent));

		GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast, PLAYER_INITILIZATION_TIME, false, playerId, playerRplComp.Id());
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Create a spectator entity in the world
	* @return The created spectator character
	*/
	protected SCR_ChimeraCharacter CreateSpectatorEntity(vector spawnLocation[4])
	{
		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform = spawnLocation;
		
		Resource spectatorRes = Resource.Load(GetSpectatorResource());
		return SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld(), spawnParams));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Remove player from their current group if any
	* @param playerId ID of the player to remove from group
	*/
	protected void RemovePlayerFromCurrentGroup(int playerId)
	{
		SCR_AIGroup currentGroup = m_GroupsManagerComponent.GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Disable damage handling for spectator character
	* @param character Character to disable damage for
	*/
	protected void DisableDamageForSpectator(SCR_ChimeraCharacter character)
	{
		if (!character)
			return;
			
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(character.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if (damManager)
			damManager.EnableDamageHandling(false);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Get existing character or create a new one for playable roles
	* @param playerId ID of the player
	* @param overrideLocation Optional spawn location
	* @return The character entity
	*/
	protected SCR_ChimeraCharacter GetOrCreatePlayableCharacter(int playerId, vector overrideLocation[4], out bool alreadyCreated)
	{
		alreadyCreated = true;
		SCR_ChimeraCharacter playerCharacter = m_SlottingManager.GetPlayerSlotCharacter(playerId);
		
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
			
			// Notify data collector about the spawn
			SCR_DataCollectorComponent dc = GetGame().GetDataCollector();
			if (dc)
			{
				// Use our custom notification method since we don't use the spawn request system
				dc.NotifyPlayerSpawned(playerId, playerCharacter);
			}
		}
			
		return playerCharacter;
	}

	//------------------------------------------------------------------------------------------------
	/**
	* Assign faction to player controller
	* @param playerController Player controller to assign faction to
	* @param faction Faction to assign
	*/
	protected void AssignFactionToPlayer(SCR_PlayerController playerController, Faction faction)
	{
		if (!faction || !playerController)
			return;
			
		SCR_PlayerFactionAffiliationComponent affiliationComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent)
		);
		
		if (affiliationComponent)
			affiliationComponent.RequestFaction(faction);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign character entity to player controller
	* @param playerController Player controller to assign character to
	* @param character Character to assign
	*/
	protected void AssignCharacterToPlayer(SCR_PlayerController playerController, SCR_ChimeraCharacter character)
	{
		if (!character || !playerController)
			return;
		
		// Delete the old initial entity BEFORE assigning new character
		// This prevents "ghost" entities
		DeleteOldInitialEntity(playerController, character);
		
		playerController.SetInitialMainEntity(character);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Delete old initial entity if it exists (prevents ghost entities)
	* @param playerController Player controller to check
	* @param newCharacter The new character being assigned (don't delete this one)
	*/
	static void DeleteOldInitialEntity(SCR_PlayerController playerController, IEntity newCharacter)
	{
		if (!playerController)
			return;
			
		IEntity oldEntity = playerController.GetMainEntity();
		if (!oldEntity || oldEntity == newCharacter)
			return;
		
		// Check if old entity is an initial entity (spawned at 1000m)
		string oldPrefab = oldEntity.GetPrefabData().GetPrefabName();
		if (oldPrefab == SPECTATOR_RESOURCE || oldPrefab.Contains("InitialEntity"))
		{
			// Log deletion for debugging
			Print(string.Format("[CRF] Deleting ghost initial entity for player %1 at position %2", 
				playerController.GetPlayerId(), 
				oldEntity.GetOrigin()), 
				LogLevel.VERBOSE);
			
			// Delete immediately to prevent replication
			SCR_EntityHelper.DeleteEntityAndChildren(oldEntity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign player to their slotted group
	* @param playerId ID of the player to assign
	*/
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
			groupComponent.RequestJoinGroup(groupId);
	}
	
	//------------------------------------------------------------------------------------------------
	// TIME MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	* Get the current server world time string
	* @return Formatted server time string
	*/
	string GetServerWorldTime()
	{
		return m_sServerWorldTime;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set the server world time
	* @param input Time string to set
	*/
	void SetServerWorldTime(string input)
	{
		SetServerWorldTimeSilent(input);
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set the server world time without triggering replication
	* @param input Time string to set
	*/
	protected void SetServerWorldTimeSilent(string input)
	{
		m_sServerWorldTime = input;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Update the server world time based on safestart time
	*/
	void UpdateServerWorldTime()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeSafeStartBegan - currentTime;
		int totalSeconds = (millis * 0.001);

		SetServerWorldTimeSilent(SCR_FormatHelper.FormatTime(totalSeconds));
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	/**
	* Update mission end timer and handle expiration
	*/
	void UpdateMissionEndTimer()
	{
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float millis = m_SafestartManager.m_iTimeMissionEnds - currentTime;
		int totalSeconds = (millis * 0.001);

		SetServerWorldTimeSilent(SCR_FormatHelper.FormatTime(totalSeconds));

		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			SetServerWorldTimeSilent("Mission Time Expired!");
		}

		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	// MODERATOR MANAGEMENT
	//------------------------------------------------------------------------------------------------
	
	/**
	* Set a player status
	* @param playerId ID of the player to set as moderator or donator
	*/
	void SetPlayerStatus(int playerId, string role)
	{
		if (!Replication.IsServer())
			return;
		
		if (m_aModerators.Contains(playerId) || m_aDonators.Contains(playerId))
			return;
		
		bool statusChanged = false;
		switch (role) {
			case "mod": {
				m_aModerators.Insert(playerId);
				statusChanged = true;
				break;
			}
			case "don": {
				m_aDonators.Insert(playerId);
				statusChanged = true;
				break;
			}
		}
			
		if (statusChanged && !m_bSuppressReplication)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Set multiple player statuses in a batch to optimize replication
	* @param playerStatuses Array of player status updates {playerId, role}
	*/
	void BatchSetPlayerStatus(array<ref array<string>> playerStatuses)
	{
		if (!Replication.IsServer())
			return;
		
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		foreach (ref array<string> statusUpdate : playerStatuses)
		{
			if (statusUpdate.Count() < 2)
				continue;
				
			int playerId = statusUpdate[0].ToInt();
			string role = statusUpdate[1];
			
			if (m_aModerators.Contains(playerId) || m_aDonators.Contains(playerId))
				continue;
				
			switch (role) {
				case "mod": {
					m_aModerators.Insert(playerId);
					anyChanged = true;
					break;
				}
				case "don": {
					m_aDonators.Insert(playerId);
					anyChanged = true;
					break;
				}
			}
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Batch update multiple gamemode properties to minimize replication calls
	* @param newWorldTime Optional new world time string  
	* @param playerStatuses Optional array of player status updates
	*/
	void BatchUpdateGamemodeState(string newWorldTime = "", array<ref array<string>> playerStatuses = null)
	{
		if (!Replication.IsServer())
			return;
			
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		// Update world time if provided
		if (newWorldTime != "" && newWorldTime != m_sServerWorldTime)
		{
			SetServerWorldTimeSilent(newWorldTime);
			anyChanged = true;
		}
		
		// Update player statuses if provided
		if (playerStatuses)
		{
			foreach (ref array<string> statusUpdate : playerStatuses)
			{
				if (statusUpdate.Count() < 2)
					continue;
					
				int playerId = statusUpdate[0].ToInt();
				string role = statusUpdate[1];
				
				if (m_aModerators.Contains(playerId) || m_aDonators.Contains(playerId))
					continue;
					
				switch (role) {
					case "mod": {
						m_aModerators.Insert(playerId);
						anyChanged = true;
						break;
					}
					case "don": {
						m_aDonators.Insert(playerId);
						anyChanged = true;
						break;
					}
				}
			}
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if a given player is a moderator
	* @param playerId ID of the player to check
	* @return True if player is a moderator, false otherwise
	*/
	bool IsModerator(int playerId)
	{
		return m_aModerators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a moderator
	* @return True if local player is a moderator, false otherwise
	*/
	bool IsModerator()
	{
		return m_aModerators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if local player is a donator
	* NOTE: Not used in game mode. Added for future uses.
	* @return True if local player is a donator, false otherwise
	*/
	bool IsDonator()
	{
		return m_aDonators.Contains(SCR_PlayerController.GetLocalPlayerId());
	}
	
	bool IsDonator(int playerId)
	{
		return m_aDonators.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	void AddForwardDeployZone(IEntity entity)
	{
		m_aForwardDeployZones.Insert(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	array<IEntity> GetForwardDeployZones()
	{
		return m_aForwardDeployZones;
	}
	
		
	//------------------------------------------------------------------------------------------------
	void DeleteAllForwardDeployZones()
	{
		foreach(IEntity zone : m_aForwardDeployZones)
		{
			if(zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
		
		m_aForwardDeployZones.Clear();
	}
	
	void CreateForwardDeployRequest(int playerId, vector transform)
	{
		ref CRF_ForwardDeployRequest request = new CRF_ForwardDeployRequest();
		request.m_iPlayerId = playerId;
		request.m_vTransform = transform;
		m_aForwardDeployRequests.Insert(request);
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	void PerformForwardDeploy(int playerId, vector transform)
	{
		vector initialSpawnLocation;
		SCR_WorldTools.FindEmptyTerrainPosition(initialSpawnLocation, transform, 10);
		vector params[4];
		params[3] = initialSpawnLocation;
		SCR_TerrainHelper.OrientToTerrain(params, GetGame().GetWorld(), true);
		vector finalSpawnLocation;
		SCR_TerrainHelper.SnapToGeometry(finalSpawnLocation, params[3], null);
		params[3] = finalSpawnLocation;
		SCR_Global.TeleportPlayer(playerId, finalSpawnLocation, SCR_EPlayerTeleportedReason.NONE);
		CRF_RplBroadcastManager.GetInstance().BroadcastVehiclePosUpdate(finalSpawnLocation, playerId);
	}
}

class CRF_ForwardDeployRequest
{
	int m_iPlayerId;
	vector m_vTransform;
}