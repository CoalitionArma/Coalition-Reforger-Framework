class CRF_GamemodeManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_GamemodeManager : SCR_BaseGameModeComponent
{
	const static ResourceName SPECTATOR_RESOURCE = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";
	
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
		
		return entity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Check if the local player is a spectator
	* @return True if local player is a spectator, false otherwise
	*/
	static bool IsSpectator()
	{
		IEntity mainEntity = SCR_PlayerController.GetLocalMainEntity();
		if (mainEntity && mainEntity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;
		
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (controlledEntity && controlledEntity.GetPrefabData().GetPrefabName() == SPECTATOR_RESOURCE)
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// PLAYER INITIALIZATION
	//------------------------------------------------------------------------------------------------
	
	/**
	* Initialize a player into the game either as a playable character or spectator
	* @param playerId ID of the player to initialize
	* @param overrideLocation Optional location to spawn the player
	*/
	void InitilizePlayer(int playerId, vector overrideLocation = vector.Zero)
	{
		if (playerId <= 0)
			return;
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!playerController)
			return;
			
		SCR_ChimeraCharacter playerCharacter = null;
		Faction faction = null;
		bool alreadyCreated;
		bool isSpectator;
		
		// Determine if player should be spectator or playable character
		if (!m_SlottingManager.IsPlayerInASlot(playerId) || m_SlottingManager.IsPlayerConsideredDead(playerId))
		{
			playerCharacter = CreateSpectatorEntity();
			faction = GetGame().GetFactionManager().GetFactionByKey("SPEC");
			isSpectator = true;
			
			RemovePlayerFromCurrentGroup(playerId);
			DisableDamageForSpectator(playerCharacter);
		} else {
			playerCharacter = GetOrCreatePlayableCharacter(playerId, overrideLocation, alreadyCreated);
			faction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		}
		
		if (playerCharacter)
		{
			AssignFactionToPlayer(playerController, faction);
			GetGame().GetCallqueue().CallLater(InitilizePlayerCharacter, 500, false, playerId, playerController, playerCharacter, isSpectator);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Assign the player to the set entity
	* @param playerId ID of the player
	* @param playerController controller of the player
	* @param playerCharacter entity the player will take
	* @param isSpectator to pass along to the players client
	*/
	protected void InitilizePlayerCharacter(int playerId, SCR_PlayerController playerController, SCR_ChimeraCharacter playerCharacter, bool isSpectator)
	{
		AssignCharacterToPlayer(playerController, playerCharacter);
		
		if (!isSpectator)
			AssignPlayerToGroup(playerId);

		CRF_RplBroadcastManager.GetInstance().InitilizePlayerBroadcast(playerId, isSpectator);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Create a spectator entity in the world
	* @return The created spectator character
	*/
	protected SCR_ChimeraCharacter CreateSpectatorEntity()
	{
		Resource spectatorRes = Resource.Load(SPECTATOR_RESOURCE);
		return SCR_ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(spectatorRes, GetGame().GetWorld()));
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
	protected SCR_ChimeraCharacter GetOrCreatePlayableCharacter(int playerId, vector overrideLocation, out bool alreadyCreated)
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
