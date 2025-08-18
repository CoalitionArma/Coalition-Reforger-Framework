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
	
	static ref CRF_GearScriptRolesConfig m_RolesConfig;
	
	protected CRF_Gamemode m_Gamemode;
	protected CRF_SlottingManager m_SlottingManager;
	protected CRF_SafestartManager m_SafestartManager;
	protected SCR_GroupsManagerComponent m_GroupsManagerComponent;
	protected CRF_AdminMenuManager m_AdminMenuManager;
	
	// NEVER EVER SPAWN AN ENT WITH A PURE 0 WORLD VECTOR OR ELSE I WILL CASTRATE YOU I STG - Njpatman
	static const vector ZERO_SPAWN_VECTOR[4] = { "1 0 0", "0 1 0", "0 0 1", "0 0 0" };
	
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
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		
		return CRF_GamemodeManager.Cast(gameMode.FindComponent(CRF_GamemodeManager));
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		// Initialize all required manager references
		InitializeManagers();
		LoadConfigurations();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Load necessary configurations for gearscript
	 */
	protected void LoadConfigurations()
	{
		const ResourceName rolesConfigPath = "{4388548E9F600148}Configs/Gearscripts/CRF_Global_Roles_Config.conf";
		
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
			playerCharacter = CreateSpectatorEntity(spawnLocation);
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			
			RemovePlayerFromCurrentGroup(playerId);
			DisableDamageForSpectator(playerCharacter);
		} else {
			playerCharacter = GetOrCreatePlayableCharacter(playerId, spawnLocation, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
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
			// Run datacollector for stats
			SCR_DataCollectorComponent dc = GetGame().GetDataCollector();
			dc.OnPlayerSpawned(playerId, playerCharacter);
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
			
		playerController.SetInitialMainEntity(character);
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
		m_sServerWorldTime = input;
		Replication.BumpMe();
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

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);
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

		m_sServerWorldTime = SCR_FormatHelper.FormatTime(totalSeconds);

		if (totalSeconds == 0) {
			GetGame().GetCallqueue().Remove(UpdateMissionEndTimer);
			m_sServerWorldTime = "Mission Time Expired!";
		}

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
		
		switch (role) {
			case "mod": {
				m_aModerators.Insert(playerId);
				break;
			}
			case "don": {
				m_aDonators.Insert(playerId);
				break;
			}
		}
			
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
}
