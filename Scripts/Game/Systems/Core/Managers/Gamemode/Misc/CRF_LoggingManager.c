/*
//! Logging component for COALITION games
//! Component overrides base game mode so it always runs
//!
//! Note that write files are formatted as CSV
//! for parsing by an external program
//! which splits strings via commas
//!
//! Server only
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
	private PlayerManager m_PlayerManager;
	private FactionManager m_FM;
	private SCR_FactionManager m_SFM;
	private CRF_Gamemode m_GM;
	
	// Cached references for performance
	private ArmaReforgerScripted m_Game;
	private BackendApi m_BackendApi;
	private BaseWorld m_World;
	
	// Winner tracking variables
	private FactionKey m_sWinningFaction = "";
	private string m_sWinnerMethod = ""; // "automatic", "manual", "timeout", "inconclusive"
	private bool m_bWinnerDetermined = false;
	
	// Singleton instance
	private static CRF_LoggingManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	//! Constructor
	void CRF_LoggingManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
		
		// Initialize damage tracking maps
		m_mPendingDamageWeapons = new map<string, string>();
		m_mPendingDamageTypes = new map<string, int>();
		
		// Cache frequently accessed singleton references
		m_Game = GetGame();
		if (m_Game)
		{
			m_BackendApi = m_Game.GetBackendApi();
			m_World = m_Game.GetWorld();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Singleton instance getter
	static CRF_LoggingManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Return the file handle for external use
	FileHandle GetLogFileHandle()
	{
		return m_LogFileHandle;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Component initialization
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on server in play mode
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		if (!m_Game || !m_Game.InPlayMode())
			return;
		
		m_PlayerManager = m_Game.GetPlayerManager();
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Error: Could not get PlayerManager during initialization", LogLevel.ERROR);
			return;
		}
		
		m_GM = CRF_Gamemode.GetInstance();
		if (!m_GM)
		{
			Print("[CRF_LoggingManager] Warning: Could not get CRF_Gamemode instance during initialization", LogLevel.WARNING);
			// Don't return here as some functionality might still work without the gamemode
		}
		
		// We'll use a more direct approach to track damage since we can't register events this way
		
		// Don't call UpdatePlayerCount here - wait until mission is fully loaded
		InitializeLogging();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize logging system
	private void InitializeLogging()
	{
		// Avoids NPEs on player disconnect
		if (!m_PlayerManager)
			return;
		
		// Initialize mission data
		m_sMissionName = m_Game.GetMissionName();
		m_iPlayerCount = m_PlayerManager.GetPlayerCount();
		//if (m_iPlayerCount < 9)
			//return;
		m_sPlayerCountMax = m_iPlayerCount.ToString();
		SCR_MissionHeader header = SCR_MissionHeader.Cast(m_Game.GetMissionHeader());
		m_sAuthorName = header.m_sAuthor;
		m_sMaxPlayers = header.m_iPlayerCount.ToString();
		m_sMissionDetails = header.m_sDetails;
		m_sGameMode = header.m_sGameMode;
		m_sTerrain = header.m_sTerrainName;
		m_sMissionName = m_sMissionName + " (" + m_sTerrain + ")"; // append terrain onto mission name due to constraints
		m_sPlayerCountMax = m_sPlayerCountMax + "/" + m_sMaxPlayers; // same here
		
		// Open global log file (close any stale handle first — defensive in case OnPostInit fires more than once)
		if (m_LogFileHandle)
		{
			m_LogFileHandle.Close();
			m_LogFileHandle = null;
		}
		m_LogFileHandle = FileIO.OpenFile(LOG_PATH, FileMode.APPEND);
		
		// Log mission beginning - use basic player count without faction data for now
		LogMissionEvent("beginning");
		
		// Register for gamemode state changes
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (gamemode)
			gamemode.GetOnStateChanged().Insert(OnGamemodeStateChanged);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle player connection events
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Error: PlayerManager is NULL in OnPlayerConnected", LogLevel.ERROR);
			return;
		}
		
		m_sPlayerName = m_PlayerManager.GetPlayerName(playerId);
		m_sPlayerGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("connect" + SEPARATOR + m_sPlayerName + SEPARATOR + m_sPlayerGUID);
		
		// Log late-connect if player connects during safestart (game phase, before safestart ends)
		// These players are eligible for AAR. Players who connect after safestart ends are not.
		if (m_GM && m_GM.m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
			if (safestart && safestart.GetSafestartStatus())
			{
				if (m_LogFileHandle)
					m_LogFileHandle.WriteLine("jip" + SEPARATOR + m_sPlayerName + SEPARATOR + m_sPlayerGUID);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle player disconnection events
	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		super.OnPlayerDisconnected(playerId, cause, timeout);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Error: PlayerManager is NULL in OnPlayerDisconnected", LogLevel.ERROR);
			return;
		}
		
		m_sPlayerName = m_PlayerManager.GetPlayerName(playerId);
		string disconnectGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (m_LogFileHandle)
			m_LogFileHandle.WriteLine("disconnect" + SEPARATOR + m_sPlayerName + SEPARATOR + disconnectGUID + SEPARATOR + cause);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle gamemode state changes
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
				// Log the VAAR file path before the ended event so the bot can associate it with this mission
				CRF_VAAR_GamemodeComponent vaarComponent = CRF_VAAR_GamemodeComponent.GetInstance();
				if (vaarComponent && m_LogFileHandle)
				{
					string vaarFilePath = vaarComponent.GetLogFilePath();
					if (!vaarFilePath.IsEmpty())
					{
						int slashPos = vaarFilePath.LastIndexOf("/");
						string vaarFileName = vaarFilePath.Substring(slashPos + 1, vaarFilePath.Length() - slashPos - 1);
						m_LogFileHandle.WriteLine("vaar_log" + SEPARATOR + vaarFileName);
					}
				}
				LogMissionEvent("ended");
				break;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Handle game mode end
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
	override void OnDelete(IEntity owner)
	{
		if (m_LogFileHandle)
		{
			m_LogFileHandle.Close();
			m_LogFileHandle = null;
		}
		
		if (s_Instance == this)
			s_Instance = null;
		
		super.OnDelete(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Method called from safestart to annotate game start
	void GameStarted()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		UpdatePlayerCount();
		LogMissionEvent("started"); // global log
		
		if (m_sGameMode == "SPCL" || m_sGameMode == "SPC" || m_sGameMode == "SPECIAL") // ignore specials
			return;

		Attendance(); // Attendance log and ORBAT logging
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper method to update player count
	private void UpdatePlayerCount()
	{
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Error: PlayerManager is NULL in UpdatePlayerCount", LogLevel.ERROR);
			return;
		}
		
		// Basic player count is always available
		m_iPlayerCount = m_PlayerManager.GetPlayerCount();
		m_sPlayerCountMax = m_iPlayerCount.ToString();
		m_sPlayerCountMax = m_sPlayerCountMax + "/" + m_sMaxPlayers;
		
		// Try to get faction manager - it may not be ready during early initialization
		if (!m_FM)
			m_FM = m_Game.GetFactionManager();
		
		if (!m_FM)
		{
			Print("[CRF_LoggingManager] FactionManager not yet available, using default faction counts", LogLevel.VERBOSE);
			// Set default values for faction counts
			m_iBluforCount = 0;
			m_iOpforCount = 0;
			m_iIndforCount = 0;
			m_iCivCount = 0;
		}
		else
		{
			if (!m_SFM)
				m_SFM = SCR_FactionManager.Cast(m_FM);
			
			if (!m_SFM)
			{
				Print("[CRF_LoggingManager] Warning: SCR_FactionManager cast failed, cannot update faction player counts", LogLevel.WARNING);
				// Set default values for faction counts
				m_iBluforCount = 0;
				m_iOpforCount = 0;
				m_iIndforCount = 0;
				m_iCivCount = 0;
			}
			else
			{
				Faction faction;
				
				faction = m_FM.GetFactionByKey("BLUFOR");
				if (faction)
					m_iBluforCount = m_SFM.GetFactionPlayerCount(faction);
				else
					m_iBluforCount = 0;
				
				faction = m_FM.GetFactionByKey("OPFOR");
				if (faction)
					m_iOpforCount = m_SFM.GetFactionPlayerCount(faction);
				else
					m_iOpforCount = 0;
				
				faction = m_FM.GetFactionByKey("INDFOR");
				if (faction)
					m_iIndforCount = m_SFM.GetFactionPlayerCount(faction);
				else
					m_iIndforCount = 0;
				
				faction = m_FM.GetFactionByKey("CIV");
				if (faction)
					m_iCivCount = m_SFM.GetFactionPlayerCount(faction);
				else
					m_iCivCount = 0;
			}
		}
		
		m_aSideCounts.Clear();
		m_aSideCounts.Insert(m_iBluforCount);
		m_aSideCounts.Insert(m_iOpforCount);
		m_aSideCounts.Insert(m_iIndforCount);
		m_aSideCounts.Insert(m_iCivCount);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Helper method to log mission events with consistent format
	private void LogMissionEvent(string eventType)
	{
		if (!m_LogFileHandle)
			return;
		
		// Ensure faction counts are available before logging
		if (m_aSideCounts.IsEmpty())
		{
			// Try to update player count if faction manager is now available
			UpdatePlayerCount();
		}
		
		m_LogFileHandle.WriteLine("mission" + SEPARATOR + eventType + SEPARATOR + m_sMissionName + SEPARATOR + m_sPlayerCountMax + SEPARATOR + m_sAuthorName + SEPARATOR + m_sMissionDetails + SEPARATOR + m_sGameMode + SEPARATOR + m_aSideCounts);
	}
	
	private void Attendance()
	{
		if (!m_LogFileHandle)
			return;
		
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Error: PlayerManager is NULL in Attendance", LogLevel.ERROR);
			return;
		}
		
		// Log players in attendance
		array<int> players = {};
		m_PlayerManager.GetPlayers(players);
		foreach (int player : players)
		{
			m_LogFileHandle.WriteLine("attendance," + SCR_PlayerIdentityUtils.GetPlayerIdentityId(player));
		}
		
		// ORBAT is now logged separately via OnGamemodeStateChanged when entering GAME state
	}
	
	//------------------------------------------------------------------------------------------------
	//! Logs the ORBAT (Order of Battle) for the mission
	private void LogORBAT()
	{
		if (!m_LogFileHandle)
			return;
		
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Error: PlayerManager is NULL in LogORBAT", LogLevel.ERROR);
			return;
		}
		
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
			Faction faction = m_FM.GetFactionByKey(factionKey);
			if (!faction)
				continue;
				
			string factionName = faction.GetFactionName();
			int factionPlayerCount = 0;
			if (m_SFM)
				factionPlayerCount = m_SFM.GetFactionPlayerCount(faction);
			
			// Log faction header with player count
			m_LogFileHandle.WriteLine("orbat_side" + SEPARATOR + factionKey + SEPARATOR + factionName + SEPARATOR + factionPlayerCount);
			
			// Get all groups for this faction
			array<SCR_AIGroup> factionGroups = slottingManager.GetAllGroups(factionKey);
			
			// Process each group
			foreach (SCR_AIGroup group : factionGroups)
			{
				if (!group)
					continue;
				
				// Get group name and ID - try multiple methods to get a meaningful name
				string groupName = group.GetCustomName();
				if (groupName.IsEmpty())
					groupName = group.GetCustomNameWithOriginal();
				
				// If still empty, try getting callsign (squad name)
				if (groupName.IsEmpty())
				{
					string company, platoon, squad, character, format;
					group.GetCallsigns(company, platoon, squad, character, format);
					if (!squad.IsEmpty())
						groupName = squad;
				}
				
				// Final fallback to group ID
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
					CRF_SlotData slotData = slottingManager.GetSlotData(slotId);
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
					CRF_SlotData slotData = slottingManager.GetSlotData(slotId);
					if (!slotData)
						continue;
					
					// Get player ID
					int playerId = slotData.GetSlotCurrentPlayerId();
					
					// Skip empty slots
					if (playerId <= 0)
						continue;
					
					// Get player name
					string playerName = m_PlayerManager.GetPlayerName(playerId);
					
					// Get player role
					string roleName = slotData.GetSlotName();
					
					// Get player GUID
					string playerGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
					
					// Log player role info
					m_LogFileHandle.WriteLine("orbat_player" + SEPARATOR + factionKey + SEPARATOR + 
					                          groupName + SEPARATOR + roleName + SEPARATOR + 
					                          playerName + SEPARATOR + playerGUID);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Track a weapon used on a specific player
	void TrackWeaponUsed(int victimId, string weaponName, int damageType = 0)
	{
		// Only track for real players — AI all share key "0" which causes cross-contamination
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
		
		if (!m_GM)
		{
			Print("[CRF_LoggingManager] Warning: Gamemode manager is null, cannot log player kill", LogLevel.WARNING);
			return;
		}
		
		if (m_GM.m_GamemodeState != CRF_EGamemodeState.GAME) // ignore aar deaths
			return;
		
		if (m_sGameMode == "SPCL" || m_sGameMode == "SPC" || m_sGameMode == "SPECIAL") // ignore specials
			return;
		
		if (!m_PlayerManager)
		{
			Print("[CRF_LoggingManager] Warning: PlayerManager is null, cannot log player kill", LogLevel.WARNING);
			return;
		}
		
		// Skip self-kills
		IEntity killerEntityCheck = instiContext.GetKillerEntity();
		IEntity targetEntityCheck = instiContext.GetVictimEntity();
		if (killerEntityCheck && targetEntityCheck && killerEntityCheck == targetEntityCheck)
			return;
		
		// Get victim entity and determine if it's a player
		IEntity victimEntity = instiContext.GetVictimEntity();
		int victimId = 0;
		if (victimEntity)
			victimId = m_PlayerManager.GetPlayerIdFromControlledEntity(victimEntity);
		
		if (victimId <= 0)
			victimId = instiContext.GetVictimPlayerID();
		
		// Victim info
		string sVictimFaction = "UNKNOWN";
		SCR_ChimeraCharacter victimChimera = SCR_ChimeraCharacter.Cast(victimEntity);
		if (victimChimera)
			sVictimFaction = victimChimera.GetFactionKey();
		else
			Print("[CRF_LoggingManager] Warning: Could not get victim character for faction info", LogLevel.WARNING);
		
		string sVictimGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(victimId);
		string sVictimName;
		if (victimId > 0)
			sVictimName = m_PlayerManager.GetPlayerName(victimId);
		else
			sVictimName = "AI";
		sVictimName = sVictimName + "(" + sVictimFaction + ")";
		
		// Get killer entity and determine if it's a player
		IEntity killerEntity = instiContext.GetKillerEntity();
		int killerId = 0;
		if (killerEntity)
			killerId = m_PlayerManager.GetPlayerIdFromControlledEntity(killerEntity);
		
		if (killerId <= 0)
			killerId = instiContext.GetKillerPlayerID();
		
		// Killer info
		string sKillerFaction = "UNKNOWN";
		SCR_ChimeraCharacter killerChimera = SCR_ChimeraCharacter.Cast(killerEntity);
		if (killerChimera)
			sKillerFaction = killerChimera.GetFactionKey();
		else
			Print("[CRF_LoggingManager] Warning: Could not get killer character for faction info", LogLevel.WARNING);
		
		string sKillerGUID = SCR_PlayerIdentityUtils.GetPlayerIdentityId(killerId);
		string sKillerName;
		if (killerId > 0)
			sKillerName = m_PlayerManager.GetPlayerName(killerId);
		else
			sKillerName = "AI";
		sKillerName = sKillerName + "(" + sKillerFaction + ")";
		
		// Weapon detection — prefer pre-tracked weapon, fall back to utility then inventory
		string sWeaponName = "Unknown Weapon";
		int damageType = 0;
		string victimKey = victimId.ToString();
		
		if (m_mPendingDamageWeapons.Contains(victimKey))
		{
			sWeaponName = m_mPendingDamageWeapons.Get(victimKey);
			m_mPendingDamageWeapons.Remove(victimKey);
			
			if (m_mPendingDamageTypes.Contains(victimKey))
			{
				damageType = m_mPendingDamageTypes.Get(victimKey);
				m_mPendingDamageTypes.Remove(victimKey);
			}
		}
		else
		{
			sWeaponName = CRF_DamageHelper.GetWeaponName(instiContext);
			
			if (sWeaponName == "Unknown Weapon" && killerEntity)
			{
				SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(killerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
				if (inventory)
				{
					BaseWeaponComponent weapon = inventory.GetCurrentCharacterWeapon();
					if (weapon)
						sWeaponName = weapon.GetUIInfo().GetName();
				}
			}
		}
		
		// Range — measure from the killer's character position.
		// GetKillerEntity() may return the projectile entity (bullet/rocket), not the shooter,
		// which causes bogus 10 000 m+ distances when the projectile is at world origin on impact.
		// Use the killer's controlled character when we have a valid player ID, falling back to
		// killerEntity only for AI / vehicle kills.
		if (!killerEntity)
			return;
		IEntity killerCharEntity = null;
		if (killerId > 0)
			killerCharEntity = m_PlayerManager.GetPlayerControlledEntity(killerId);
		if (!killerCharEntity)
			killerCharEntity = killerEntity; // AI or vehicle — killerEntity is already the agent
		float fRange = vector.Distance(victimEntity.GetOrigin(), killerCharEntity.GetOrigin());
		int iRangeMeters = fRange.ToString().ToInt(); // round to whole metres; avoids locale decimal-separator breaking CSV
		
		// Time
		int iTotalSeconds = m_World.GetWorldTime() / 1000;
		string sTime = SCR_FormatHelper.FormatTime(iTotalSeconds);
		
		// Log to file
		m_LogFileHandle.WriteLine("kill" + SEPARATOR + sVictimName + SEPARATOR + sVictimGUID + SEPARATOR + 
		                         sKillerName + SEPARATOR + sKillerGUID + SEPARATOR + sWeaponName + SEPARATOR + 
		                         iRangeMeters + SEPARATOR + sTime);
	}
	
	// TODO: Implement these on EH where grenade is thrown
	/*void LogGrenadeThrown()
	{
		if (!m_LogFileHandle)
			return;
		
		// TODO: Add username and guid to both loggers
		m_LogFileHandle.WriteLine("grenade" + SEPARATOR + );
	}*/
	
	//------------------------------------------------------------------------------------------------
	//! Called when player takes significant damage - can be used to track weapons that cause damage
	//! This needs to be called from damage handling events in the game
	void PlayerTookDamage(int victimId, IEntity killerEntity, int damageType)
	{
		if (victimId <= 0)
			return;
		
		// Only track if this is a player-to-player event
		int killerId = m_PlayerManager.GetPlayerIdFromControlledEntity(killerEntity);
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
		
		// Only fall back to damage type labels when no weapon was identified from inventory.
		// If inventory already gave us a real weapon name, keep it so the bot shows
		// the actual weapon (e.g. "RGO Grenade") rather than a generic type string.
		if (damageType > 0 && damageType != EDamageType.KINETIC && weaponName == "Unknown Weapon")
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
		}
		
		// Store this weapon and damage type for the victim
		TrackWeaponUsed(victimId, weaponName, damageType);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Public method to log the ORBAT at any time
	void LogCurrentORBAT()
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		if (!m_LogFileHandle)
			return;
		
		LogORBAT();
	}
	
	//------------------------------------------------------------------------------------------------
	// FACTION WINNER LOGGING SYSTEM
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Set the winning faction for this mission
	//! \param[in] factionKey The faction key that won (BLUFOR, OPFOR, INDFOR, CIV)
	//! \param[in] method The method used to determine the winner (manual, automatic, timeout, etc.)
	void SetWinningFaction(FactionKey factionKey, string method = "manual")
	{
		if (RplSession.Mode() != RplMode.Dedicated && RplSession.Mode() != RplMode.Listen)
			return;
		
		// Validate faction key
		if (factionKey.IsEmpty())
		{
			Print("[CRF_LoggingManager] Warning: Empty faction key provided to SetWinningFaction", LogLevel.WARNING);
			return;
		}
		
		// Normalize faction key to uppercase
		factionKey.ToUpper();
		
		// Validate faction key is one of the supported factions
		if (factionKey != "BLUFOR" && factionKey != "OPFOR" && factionKey != "INDFOR" && factionKey != "CIV")
		{
			Print(string.Format("[CRF_LoggingManager] Warning: Invalid faction key '%1' provided to SetWinningFaction", factionKey), LogLevel.WARNING);
			return;
		}
		
		m_sWinningFaction = factionKey;
		m_sWinnerMethod = method;
		m_bWinnerDetermined = true;
		
		// Log immediately to ensure it's captured
		LogWinner();
		
		Print(string.Format("[CRF_LoggingManager] Winner set: %1 (method: %2)", factionKey, method), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the currently determined winning faction
	//! \return FactionKey of the winning faction, empty string if not determined
	FactionKey GetWinningFaction()
	{
		return m_sWinningFaction;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if a winner has been determined
	//! \return true if winner has been set, false otherwise
	bool IsWinnerDetermined()
	{
		return m_bWinnerDetermined;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get the method used to determine the winner
	//! \return string describing how the winner was determined
	string GetWinnerMethod()
	{
		return m_sWinnerMethod;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Private method to write winner information to log file
	protected void LogWinner()
	{
		if (!m_LogFileHandle)
			return;
		
		// Format: mission_winner,FACTION,METHOD,MISSION_NAME
		string winnerLine = string.Format("mission_winner%1%2%1%3%1%4", 
			SEPARATOR, m_sWinningFaction, m_sWinnerMethod, m_sMissionName);
		
		m_LogFileHandle.WriteLine(winnerLine);
	}
}
