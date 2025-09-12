/*
* Logging component for COALITION games
* Component overrides base game mode so it always runs
*
* Note that write files are formatted as CSV
* for parsing by an external program
* which splits strings via commas
*
* Server only
*/
[ComponentEditorProps(category: "CRF Logging Component", description: "Handles logging for server events")]
class CRF_LoggingManagerClass: SCR_BaseGameModeComponentClass
{
	// Class definition for the component class
}

class CRF_LoggingManager: SCR_BaseGameModeComponent
{    
	// Constants
	const string SEPARATOR = ",";
	const string LOG_PATH = "$profile:COAServerLog.txt";
	
	// Mission and player data
	private string m_sMissionName;
	private string m_sPlayerName;
	private string m_sAuthorName;
	private string m_sMaxPlayers;
	private string m_sMissionDetails;
	private string m_sGameMode;
	private string m_sPlayerGUID;
	private string m_sTerrain;
	
	// Kill data
	string m_sKillerName;
	string m_sKillerGUID;
	string m_sKillerFaction;
	string m_sVictimName;
	string m_sVictimGUID;
	string m_sVictimFaction;
	string m_sTime;
	string m_sWeaponName;
	float m_fRange;
	float m_fTotalTime;
	int m_iTotalSeconds;
	
	// Kill tracking for more accurate weapon logging
	private ref map<string, string> m_mPendingDamageWeapons;
	
	// Damage type tracking for kills
	private ref map<string, int> m_mPendingDamageTypes;
	
	// Player counts
	private int m_iPlayerCount;
	private string m_sPlayerCountMax;
	private int m_iBluforCount;
	private int m_iOpforCount;
	private int m_iIndforCount;
	private int m_iCivCount;
	private ref array<int> m_aSideCounts = new array<int>;
	
	// File handling and faction management
	private ref FileHandle m_LogFileHandle;
	private SCR_FactionManager m_FactionManager;
	private BaseWeaponManagerComponent m_WMC;
	private SCR_ChimeraCharacter m_PlayerChimera;
	private PlayerManager m_PlayerManager;
	private SCR_CharacterInventoryStorageComponent m_Inventory;
	private BaseWeaponComponent m_BWC;
	private FactionManager m_FM;
	private SCR_FactionManager m_SFM;
	private Faction m_Faction;
	private CRF_Gamemode m_GM;
	
	// Singleton instance
	private static CRF_LoggingManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	// Constructor
	void CRF_LoggingManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
		
		// Initialize damage tracking maps
		m_mPendingDamageWeapons = new map<string, string>();
		m_mPendingDamageTypes = new map<string, int>();
	}
	
	//------------------------------------------------------------------------------------------------
	// Singleton instance getter
	static CRF_LoggingManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	// Return the file handle for external use
	FileHandle GetLogFileHandle()
	{
		return m_LogFileHandle;
	}
	
	//------------------------------------------------------------------------------------------------
	// Component initialization
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server in play mode
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		if (!GetGame().InPlayMode())
			return;
		
		m_PlayerManager = GetGame().GetPlayerManager();
		m_GM = CRF_Gamemode.GetInstance();
		
		// We'll use a more direct approach to track damage since we can't register events this way
		
		UpdatePlayerCount();
		InitializeLogging();
	}
	
	//------------------------------------------------------------------------------------------------
	// Initialize logging system
	private void InitializeLogging()
	{
		// Initialize mission data
		m_sMissionName = GetGame().GetMissionName();
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		//if (m_iPlayerCount < 9)
			//return;
		m_sPlayerCountMax = m_iPlayerCount.ToString();
		SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		m_sAuthorName = header.m_sAuthor;
		m_sMaxPlayers = header.m_iPlayerCount.ToString();
		m_sMissionDetails = header.m_sDetails;
		m_sGameMode = header.m_sGameMode;
		m_sTerrain = header.m_sTerrainName;
		m_sMissionName = m_sMissionName + " (" + m_sTerrain + ")"; // append terrain onto mission name due to constraints
		m_sPlayerCountMax = m_sPlayerCountMax + "/" + m_sMaxPlayers; // same here
		
		// Open global log file
		m_LogFileHandle = FileIO.OpenFile(LOG_PATH, FileMode.APPEND);
		
		// Log mission beginning
		LogMissionEvent("beginning");
		
		// Register for gamemode state changes
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode)
			gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle player connection events
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_sPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		m_sPlayerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
		
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("connect" + SEPARATOR + m_sPlayerName + SEPARATOR + m_sPlayerGUID);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle player disconnection events
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_sPlayerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("disconnect" + SEPARATOR + m_sPlayerName + SEPARATOR + cause);
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle gamemode state changes
	void OnGamemodeStateChanged()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		UpdatePlayerCount();
		
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return;
		
		switch (gamemode.m_GamemodeState)
		{
			case CRF_EGamemodeState.SLOTTING:
			{
				LogMissionEvent("slotting");
				break;
			}
			case CRF_EGamemodeState.GAME:
			{
				LogMissionEvent("safestart");
				// Only log ORBAT at game start, not during slotting, as slots may still be changing
				if (m_sGameMode != "SPCL" && m_sGameMode != "SPC" && m_sGameMode != "SPECIAL")
					LogORBAT();
				break;
			}
			case CRF_EGamemodeState.AAR:
			{
				LogMissionEvent("ended");
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Handle game mode end
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		// Close file handle to prevent memory leaks
		if (m_LogFileHandle)
		{
			m_LogFileHandle.Close();
			m_LogFileHandle = null;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Method called from safestart to annotate game start
	void GameStarted()
	{
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		UpdatePlayerCount();
		LogMissionEvent("started"); // global log
		
		if (m_sGameMode == "SPCL" || m_sGameMode == "SPC" || m_sGameMode == "SPECIAL") // ignore specials
			return;

		Attendance(); // Attendance log and ORBAT logging
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to update player count
	private void UpdatePlayerCount()
	{
		m_iPlayerCount = GetGame().GetPlayerManager().GetPlayerCount();
		m_sPlayerCountMax = m_iPlayerCount.ToString();
		m_sPlayerCountMax = m_sPlayerCountMax + "/" + m_sMaxPlayers;
		
		m_FM = GetGame().GetFactionManager();
		m_SFM = SCR_FactionManager.Cast(m_FM);
		m_Faction = m_FM.GetFactionByKey("BLUFOR");
		m_iBluforCount = m_SFM.GetFactionPlayerCount(m_Faction);
		m_Faction = m_FM.GetFactionByKey("OPFOR");
		m_iOpforCount = m_SFM.GetFactionPlayerCount(m_Faction);
		m_Faction = m_FM.GetFactionByKey("INDFOR");
		m_iIndforCount = m_SFM.GetFactionPlayerCount(m_Faction);
		m_Faction = m_FM.GetFactionByKey("CIV");
		m_iCivCount = m_SFM.GetFactionPlayerCount(m_Faction);
		
		if (m_aSideCounts.IsEmpty()) {
			m_aSideCounts.Insert(m_iBluforCount);
			m_aSideCounts.Insert(m_iOpforCount);
			m_aSideCounts.Insert(m_iIndforCount);
			m_aSideCounts.Insert(m_iCivCount);
		} else {
			m_aSideCounts.Clear();
			m_aSideCounts.Insert(m_iBluforCount);
			m_aSideCounts.Insert(m_iOpforCount);
			m_aSideCounts.Insert(m_iIndforCount);
			m_aSideCounts.Insert(m_iCivCount);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper method to log mission events with consistent format
	private void LogMissionEvent(string eventType)
	{
		if (!m_LogFileHandle)
			return;
		
		m_LogFileHandle.WriteLine("mission" + SEPARATOR + eventType + SEPARATOR + m_sMissionName + SEPARATOR + m_sPlayerCountMax + SEPARATOR + m_sAuthorName + SEPARATOR + m_sMissionDetails + SEPARATOR + m_sGameMode + SEPARATOR + m_aSideCounts);
	}
	
	private void Attendance()
	{
		if (!m_LogFileHandle)
			return;
		
		// Log players in attendance
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int player : players)
		{
			m_LogFileHandle.WriteLine("attendance," + GetGame().GetBackendApi().GetPlayerIdentityId(player));
		}
		
		// ORBAT is now logged separately via OnGamemodeStateChanged when entering GAME state
	}
	
	//------------------------------------------------------------------------------------------------
	// Logs the ORBAT (Order of Battle) for the mission
	private void LogORBAT()
	{
		if (!m_LogFileHandle)
			return;
		
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;
		
		// Get all factions
		array<FactionKey> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		
		// Process each faction for detailed ORBAT
		foreach (FactionKey factionKey : factionKeys)
		{
			// Skip if faction not used in mission
			if (!slottingManager.IsFactionValid(factionKey))
				continue;
			
			// Get faction name and player count
			Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
			if (!faction)
				continue;
				
			string factionName = faction.GetFactionName();
			SCR_FactionManager scrFM = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			int factionPlayerCount = scrFM.GetFactionPlayerCount(faction);
			
			// Log faction header with player count
			m_LogFileHandle.WriteLine("orbat_side" + SEPARATOR + factionKey + SEPARATOR + factionName + SEPARATOR + factionPlayerCount);
			
			// Get all groups for this faction
			array<SCR_AIGroup> factionGroups = slottingManager.GetAllGroups(factionKey);
			
			// Process each group
			foreach (SCR_AIGroup group : factionGroups)
			{
				if (!group)
					continue;
				
				// Get group name and ID
				string groupName = group.GetName();
				if (groupName.IsEmpty())
					groupName = "Group " + group.GetGroupID().ToString();
				
				RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
				if (!groupRplComp)
					continue;
				
				RplId groupId = groupRplComp.Id();
				
				// Count players in this group
				int groupPlayerCount = 0;
				array<int> slotsInGroup = slottingManager.GetAllSlotIDsForGroup(groupId);
				foreach (int slotId : slotsInGroup)
				{
					CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
					if (slotData && slotData.GetSlotCurrentPlayerId() > 0)
						groupPlayerCount++;
				}
				
				// Skip empty groups
				if (groupPlayerCount == 0)
					continue;
				
				// Log group header with player count
				m_LogFileHandle.WriteLine("orbat_group" + SEPARATOR + factionKey + SEPARATOR + groupName + SEPARATOR + groupPlayerCount);
				
				// Process each slot
				foreach (int slotId : slotsInGroup)
				{
					CRF_SlotDataContainer slotData = slottingManager.GetSlotData(slotId);
					if (!slotData)
						continue;
					
					// Get player ID
					int playerId = slotData.GetSlotCurrentPlayerId();
					
					// Skip empty slots
					if (playerId <= 0)
						continue;
					
					// Get player name
					string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
					
					// Get player role
					string roleName = slotData.GetSlotName();
					
					// Get player GUID
					string playerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(playerId);
					
					// Log player role info
					m_LogFileHandle.WriteLine("orbat_player" + SEPARATOR + factionKey + SEPARATOR + 
					                          groupName + SEPARATOR + roleName + SEPARATOR + 
					                          playerName + SEPARATOR + playerGUID);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Track a weapon used on a specific player
	void TrackWeaponUsed(int victimId, string weaponName, int damageType = 0)
	{
		if (victimId <= 0 || weaponName.IsEmpty())
			return;
		
		string victimKey = victimId.ToString();
		m_mPendingDamageWeapons.Set(victimKey, weaponName);
		
		// Also track the damage type if provided
		if (damageType > 0)
		{
			m_mPendingDamageTypes.Set(victimKey, damageType);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Logs player death and kill data to file
	void LogPlayerKill(SCR_InstigatorContextData instiContext)
	{
		if (!m_LogFileHandle)
			return;
		
		if (m_GM.m_GamemodeState != CRF_EGamemodeState.GAME) // ignore aar deaths
			return;
		
		if (m_sGameMode == "SPCL" || m_sGameMode == "SPC" || m_sGameMode == "SPECIAL") // ignore specials
			return;
		
		int victimId = instiContext.GetVictimPlayerID();
		
		// Victim info
		m_PlayerChimera = SCR_ChimeraCharacter.Cast(m_PlayerManager.GetPlayerControlledEntity(victimId));
		m_sVictimFaction = m_PlayerChimera.GetFactionKey();
		m_sVictimGUID = GetGame().GetBackendApi().GetPlayerIdentityId(victimId);
		if (victimId > 0) // if it's a player 
			m_sVictimName = GetGame().GetPlayerManager().GetPlayerName(victimId);
		else 
			m_sVictimName = "AI";
		m_sVictimName = m_sVictimName + "(" + m_sVictimFaction + ")"; // we append the faction here due to compiler constraints
		
		// Killer info
		int killerId = instiContext.GetKillerPlayerID();
		m_PlayerChimera = SCR_ChimeraCharacter.Cast(m_PlayerManager.GetPlayerControlledEntity(killerId));
		m_sKillerFaction = m_PlayerChimera.GetFactionKey();
		m_sKillerGUID = GetGame().GetBackendApi().GetPlayerIdentityId(killerId);
		if (killerId > 0) // if it's a player and ignore aar killings
			m_sKillerName = GetGame().GetPlayerManager().GetPlayerName(killerId);
		else
			m_sKillerName = "AI";
		m_sKillerName = m_sKillerName + "(" + m_sKillerFaction + ")"; // we append the faction here due to compiler constraints
		
		// Default weapon name and damage type
		m_sWeaponName = "Unknown Weapon";
		int damageType = 0;
		
		// First, check if we have tracked a weapon for this victim
		string victimKey = victimId.ToString();
		if (m_mPendingDamageWeapons.Contains(victimKey))
		{
			m_sWeaponName = m_mPendingDamageWeapons.Get(victimKey);
			m_mPendingDamageWeapons.Remove(victimKey); // Clear the tracking data
			
			// Also get the damage type if available
			if (m_mPendingDamageTypes.Contains(victimKey))
			{
				damageType = m_mPendingDamageTypes.Get(victimKey);
				m_mPendingDamageTypes.Remove(victimKey); // Clear the tracking data
			}
		}
		else
		{
			// If no tracked weapon, use utility to determine the weapon
			m_sWeaponName = CRF_DamageUtility.GetWeaponName(instiContext);
			
			// If we still don't have a weapon name, try the killer's current weapon as a last resort
			if (m_sWeaponName == "Unknown Weapon")
			{
				IEntity killerEntity = instiContext.GetKillerEntity();
				if (killerEntity)
				{
					m_Inventory = SCR_CharacterInventoryStorageComponent.Cast(killerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
					if (m_Inventory)
					{
						m_BWC = m_Inventory.GetCurrentCharacterWeapon();
						if (m_BWC)
							m_sWeaponName = m_BWC.GetUIInfo().GetName();
					}
				}
			}
		}
		
		// Range
		m_fRange = vector.Distance(instiContext.GetVictimEntity().GetOrigin(),instiContext.GetKillerEntity().GetOrigin());
		
		// Time
		m_fTotalTime = GetGame().GetWorld().GetWorldTime();
  		m_iTotalSeconds = (m_fTotalTime / 1000);
		m_sTime = SCR_FormatHelper.FormatTime(m_iTotalSeconds);
		
		// Append damage type to weapon name if available
		if (damageType > 0)
		{
			string damageTypeStr = CRF_DamageUtility.GetDamageTypeString(damageType);
			m_sWeaponName = m_sWeaponName + " (" + damageTypeStr + ")";
		}
		
		// Log to file
		m_LogFileHandle.WriteLine("kill" + SEPARATOR + m_sVictimName + SEPARATOR + m_sVictimGUID + SEPARATOR + 
		                         m_sKillerName + SEPARATOR + m_sKillerGUID + SEPARATOR + m_sWeaponName + SEPARATOR + 
		                         m_fRange + SEPARATOR + m_sTime);
	}
	
	// TODO: Implement these on EH where grenade is thrown
	/*void LogGrenadeThrown()
	{
		if (!m_LogFileHandle)
			return;
		
		// TODO: Add username and guid to both loggers
		m_LogFileHandle.WriteLine("grenade" + SEPARATOR + );
	}*/
	
	void LogShots()
	{
		// This should be pulled from the datacollector IMO, once per player, at end of game (enterAAR()).
	}
	
	//------------------------------------------------------------------------------------------------
	// Called when player takes significant damage - can be used to track weapons that cause damage
	// This needs to be called from damage handling events in the game
	void PlayerTookDamage(int victimId, IEntity killerEntity, int damageType)
	{
		if (victimId <= 0)
			return;
		
		// Only track if this is a player-to-player event
		int killerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(killerEntity);
		if (killerId <= 0)
			return;
			
		// Get the weapon information
		string weaponName = "Unknown Weapon";
		
		// Try to determine weapon from killer entity
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(killerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (inventory)
		{
			BaseWeaponComponent weapon = inventory.GetCurrentCharacterWeapon();
			if (weapon)
			{
				weaponName = weapon.GetUIInfo().GetName();
			}
		}
		
		// Handle specific damage types if provided
		if (damageType > 0)
		{
			if (damageType == EDamageType.BLEEDING)
			{
				weaponName = "Bleeding";
			}
			else if (damageType == EDamageType.EXPLOSIVE)
			{
				weaponName = "Explosion";
			}
			else if (damageType == EDamageType.FRAGMENTATION || damageType == EDamageType.PROCESSED_FRAGMENTATION)
			{
				weaponName = "Fragmentation";
			}
			else if (damageType == EDamageType.FIRE || damageType == EDamageType.INCENDIARY)
			{
				weaponName = "Fire";
			}
			else if (damageType == EDamageType.COLLISION)
			{
				weaponName = "Collision";
			}
			else if (damageType == EDamageType.MELEE)
			{
				weaponName = "Melee";
			}
			else
			{
				// Use the damage type string from utility class
				weaponName = CRF_DamageUtility.GetDamageTypeString(damageType);
			}
		}
		
		// Store this weapon and damage type for the victim
		TrackWeaponUsed(victimId, weaponName, damageType);
	}
	
	//------------------------------------------------------------------------------------------------
	// Public method to log the ORBAT at any time
	void LogCurrentORBAT()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		if (!m_LogFileHandle)
			return;
		
		LogORBAT();
	}
}