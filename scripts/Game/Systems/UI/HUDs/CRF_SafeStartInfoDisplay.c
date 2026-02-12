class CRF_SafeStartInfoDisplay : SCR_InfoDisplayExtended
{
	//------------------------------------------------------------------------------------------------
	// UI widget references
	//------------------------------------------------------------------------------------------------
	protected OverlayWidget m_wSafeStartInfoPanel;
	
	// Mission Info
	protected TextWidget m_wMissionTitleValue;
	protected TextWidget m_wMissionAuthorValue;
	protected TextWidget m_wMissionTypeValue;
	protected TextWidget m_wSideRatiosValue;
	protected TextWidget m_wSafeStartHardLimitValue;
	protected TextWidget m_wMissionLengthValue;
	protected TextWidget m_wJIPAfterSafestartValue;
	protected TextWidget m_wRespawnValue;
	protected TextWidget m_wEspionageValue;
	
	// Markers
	protected TextWidget m_wBlueForceTrackerValue;
	protected TextWidget m_wUnitMapMarkersValue;
	
	// Equipment
	protected TextWidget m_wFactionNameValue;
	protected TextWidget m_wRadiosValue;
	protected TextWidget m_wMapValue;
	protected TextWidget m_wBinocularsValue;
	protected TextWidget m_wEntrenchingToolValue;
	protected TextWidget m_wNightVisionValue;
	protected TextWidget m_wFlashlightValue;
	
	//------------------------------------------------------------------------------------------------
	// Manager references
	//------------------------------------------------------------------------------------------------
	protected CRF_SafestartManager m_SafestartManager = null;
	protected CRF_Gamemode m_Gamemode = null;
	
	//------------------------------------------------------------------------------------------------
	// State variables
	//------------------------------------------------------------------------------------------------
	protected bool m_bInitialized = false;
	protected bool m_bDataLoaded = false;
	
	//------------------------------------------------------------------------------------------------
	// Override functions
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Main update method called each frame
	 */
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		// Initialize references if needed
		if (!m_bInitialized)
		{
			if (!InitializeReferences())
				return;
			m_bInitialized = true;
		}
		
		// Load mission data once, but only after player has taken a playable slot (not spectator)
		if (!m_bDataLoaded && m_Gamemode)
		{
			// Get player ID and check if they have a faction slot
			int playerId = SCR_PlayerController.GetLocalPlayerId();
			CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
			
			if (slottingManager)
			{
				Faction playerFaction = slottingManager.GetPlayerSlotFaction(playerId, true);
				
				// Only load data if player has a faction AND is not in spectator
				if (playerFaction)
				{
					IEntity playerEntity = SCR_PlayerController.GetLocalControlledEntity();
					if (playerEntity && !CRF_GamemodeManager.IsSpectator(playerEntity))
					{
						LoadMissionData();
						m_bDataLoaded = true;
					}
				}
			}
		}
		
		// Handle visibility based on safestart status
		if (m_SafestartManager && m_wSafeStartInfoPanel)
		{
			bool safestartActive = m_SafestartManager.GetSafestartStatus();
			bool hudVisible = CRF_PlayerControllerManager.GetInstance().m_bHUDVisible;
			
			m_wSafeStartInfoPanel.SetVisible(safestartActive && hudVisible);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Initializes all widget and manager references
	 */
	protected bool InitializeReferences()
	{
		// Get manager references
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		if (!m_SafestartManager || !m_Gamemode)
			return false;
		
		// Get main panel
		m_wSafeStartInfoPanel = OverlayWidget.Cast(m_wRoot.FindAnyWidget("SafeStartInfoPanel"));
		
		// Get mission info widgets
		m_wMissionTitleValue = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionTitleValue"));
		m_wMissionAuthorValue = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionAuthorValue"));
		m_wMissionTypeValue = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionTypeValue"));
		m_wSideRatiosValue = TextWidget.Cast(m_wRoot.FindAnyWidget("SideRatiosValue"));
		m_wSafeStartHardLimitValue = TextWidget.Cast(m_wRoot.FindAnyWidget("SafeStartHardLimitValue"));
		m_wMissionLengthValue = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionLengthValue"));
		m_wJIPAfterSafestartValue = TextWidget.Cast(m_wRoot.FindAnyWidget("JIPAfterSafestartValue"));
		m_wRespawnValue = TextWidget.Cast(m_wRoot.FindAnyWidget("RespawnValue"));
		m_wEspionageValue = TextWidget.Cast(m_wRoot.FindAnyWidget("EspionageValue"));
		
		// Get markers widgets
		m_wBlueForceTrackerValue = TextWidget.Cast(m_wRoot.FindAnyWidget("BlueForceTrackerValue"));
		m_wUnitMapMarkersValue = TextWidget.Cast(m_wRoot.FindAnyWidget("UnitMapMarkersValue"));
		
		// Get equipment widgets
		m_wFactionNameValue = TextWidget.Cast(m_wRoot.FindAnyWidget("FactionNameValue"));
		m_wRadiosValue = TextWidget.Cast(m_wRoot.FindAnyWidget("RadiosValue"));
		m_wMapValue = TextWidget.Cast(m_wRoot.FindAnyWidget("MapValue"));
		m_wBinocularsValue = TextWidget.Cast(m_wRoot.FindAnyWidget("BinocularsValue"));
		m_wEntrenchingToolValue = TextWidget.Cast(m_wRoot.FindAnyWidget("EntrenchingToolValue"));
		m_wNightVisionValue = TextWidget.Cast(m_wRoot.FindAnyWidget("NightVisionValue"));
		m_wFlashlightValue = TextWidget.Cast(m_wRoot.FindAnyWidget("FlashlightValue"));
		
		return AreWidgetsValid();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Validates that all required widgets are present
	 */
	protected bool AreWidgetsValid()
	{
		return m_wSafeStartInfoPanel && m_wMissionTitleValue && m_wMissionAuthorValue && 
		       m_wMissionTypeValue && m_wSideRatiosValue && m_wSafeStartHardLimitValue && 
		       m_wMissionLengthValue && m_wJIPAfterSafestartValue && m_wRespawnValue && 
		       m_wEspionageValue && m_wBlueForceTrackerValue && m_wUnitMapMarkersValue && 
		       m_wFactionNameValue && m_wRadiosValue && m_wMapValue;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Loads and displays mission data
	 */
	protected void LoadMissionData()
	{
		if (!m_Gamemode)
			return;
		
		// Load mission info
		LoadMissionInfo();
		
		// Load markers info
		LoadMarkersInfo();
		
		// Load equipment info
		LoadEquipmentInfo();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Loads basic mission information
	 */
	protected void LoadMissionInfo()
	{
		// Mission Title - Get from GetGame().GetMissionName()
		string missionTitle = "Unknown Mission";
		if (GetGame().GetMissionName())
			missionTitle = GetGame().GetMissionName();
		m_wMissionTitleValue.SetText(missionTitle);
		
		// Mission Author - Get from mission header
		string missionAuthor = "Unknown";
		SCR_MissionHeader missionHeader = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
		if (missionHeader && missionHeader.m_sAuthor && !missionHeader.m_sAuthor.IsEmpty())
			missionAuthor = missionHeader.m_sAuthor;
		m_wMissionAuthorValue.SetText(string.Format("By: %1", missionAuthor));
		
		// Mission Type - Get from mission header's game mode
		string missionType = "N/A";
		if (missionHeader && missionHeader.m_sGameMode && !missionHeader.m_sGameMode.IsEmpty())
			missionType = missionHeader.m_sGameMode;
		m_wMissionTypeValue.SetText(missionType);
		
		// Side Ratios
		string sideRatios = "N/A";
		if (m_Gamemode.m_sFactionOneKey && m_Gamemode.m_sFactionTwoKey)
		{
			string factionOneName = m_Gamemode.m_sFactionOneKey;
			string factionTwoName = m_Gamemode.m_sFactionTwoKey;
			int ratioOne = m_Gamemode.m_iFactionOneRatio;
			int ratioTwo = m_Gamemode.m_iFactionTwoRatio;
			
			sideRatios = string.Format("%1 %2:%3 %4", factionOneName, ratioOne, ratioTwo, factionTwoName);
		}
		m_wSideRatiosValue.SetText(sideRatios);
		
		// Safe Start Limit
		string safeStartLimit;
		if (m_Gamemode.m_bUseSafestartTimeLimit && m_Gamemode.m_iSafestartTimeLimit > 0)
		{
			int minutes = m_Gamemode.m_iSafestartTimeLimit;
			safeStartLimit = string.Format("Yes - %1:00", minutes.ToString(2));
		}
		else
		{
			safeStartLimit = "No";
		}
		m_wSafeStartHardLimitValue.SetText(safeStartLimit);
		
		// Mission Length
		string missionLength;
		if (m_Gamemode.m_iTimeLimitMinutes > 0)
		{
			int hours = m_Gamemode.m_iTimeLimitMinutes / 60;
			int minutes = m_Gamemode.m_iTimeLimitMinutes % 60;
			missionLength = string.Format("%1:%2:00", hours, minutes.ToString(2));
		}
		else
		{
			missionLength = "Unlimited";
		}
		m_wMissionLengthValue.SetText(missionLength);
		
		// JIP After Safestart
		string jipAfterSafestart;
		if (m_Gamemode.m_bLockUnusedSlots)
			jipAfterSafestart = "No";
		else
			jipAfterSafestart = "Yes";
		m_wJIPAfterSafestartValue.SetText(jipAfterSafestart);
		
		// Respawn
		string respawnStatus;
		if (m_Gamemode.m_bRespawnEnabled)
		{
			int seconds = m_Gamemode.m_iTimeToRespawn;
			int minutes = seconds / 60;
			int remainingSeconds = seconds % 60;
			string timeString;
			
			if (minutes > 0)
				timeString = string.Format("%1:%2", minutes, remainingSeconds.ToString(2));
			else
				timeString = string.Format("0:%1", seconds.ToString(2));
			
			// Get player's faction tickets
			int tickets = 0;
			SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (playerController)
			{
				SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
				if (factionManager)
				{
					Faction playerFaction = factionManager.GetPlayerFaction(playerController.GetPlayerId());
					if (playerFaction)
					{
						string factionKey = playerFaction.GetFactionKey();
						switch (factionKey)
						{
							case "BLUFOR":
								tickets = m_Gamemode.m_iBLUFORTickets;
								break;
							case "OPFOR":
								tickets = m_Gamemode.m_iOPFORTickets;
								break;
							case "INDFOR":
								tickets = m_Gamemode.m_iINDFORTickets;
								break;
							case "CIV":
								tickets = m_Gamemode.m_iCIVTickets;
								break;
						}
					}
				}
			}
			
			string ticketString;
			if (tickets > 0)
				ticketString = string.Format(" - %1 Tickets", tickets);
			else if (tickets == -1)
				ticketString = " - Unlimited Tickets";
			else
				ticketString = " - No Tickets";
			
			if (m_Gamemode.m_bWaveRespawn)
				respawnStatus = string.Format("Yes - Waves - %1%2", timeString, ticketString);
			else
				respawnStatus = string.Format("Yes - %1%2", timeString, ticketString);
		}
		else
		{
			respawnStatus = "Off";
		}
		m_wRespawnValue.SetText(respawnStatus);
		
		// Espionage
		string espionageStatus;
		if (m_Gamemode.m_bAllowEspionage)
			espionageStatus = "On";
		else
			espionageStatus = "Off";
		m_wEspionageValue.SetText(espionageStatus);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Loads markers configuration information
	 */
	protected void LoadMarkersInfo()
	{
		// Get player's faction for checking BFT and marker settings
		string bftStatus = "Off"; // Default
		string globalMarkersStatus = "On"; // Default to On
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (factionManager)
			{
				Faction playerFaction = factionManager.GetPlayerFaction(playerController.GetPlayerId());
				if (playerFaction)
				{
					string factionKey = playerFaction.GetFactionKey();
					
					// Blue Force Tracker - Check if BFT is enabled for player's faction
					if (m_Gamemode.IsSideBFTEnabled(factionKey))
						bftStatus = "On";
					
					// Global Markers - Inverse of shareable markers setting
					// If shareable markers are enabled for this faction, global markers are OFF
					if (m_Gamemode.DoesFactionShareMarker(factionKey))
						globalMarkersStatus = "Off";
				}
			}
		}
		
		m_wBlueForceTrackerValue.SetText(string.Format("Blue Force Tracker: %1", bftStatus));
		m_wUnitMapMarkersValue.SetText(string.Format("Global Markers: %1", globalMarkersStatus));
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Loads equipment availability information
	 */
	protected void LoadEquipmentInfo()
	{
		// Get player's faction to check gear script settings
		string factionName = "Unknown";
		string radiosValue = "None";
		
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
		{
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (factionManager)
			{
				Faction playerFaction = factionManager.GetPlayerFaction(playerController.GetPlayerId());
				if (playerFaction)
				{
					string factionKey = playerFaction.GetFactionKey();
					CRF_GearScriptContainer gearScriptSettings = null;
					
					// Get the appropriate gear script settings based on faction
					switch (factionKey)
					{
						case "BLUFOR":
							gearScriptSettings = m_Gamemode.m_BLUFORGearScriptSettings;
							break;
						case "OPFOR":
							gearScriptSettings = m_Gamemode.m_OPFORGearScriptSettings;
							break;
						case "INDFOR":
							gearScriptSettings = m_Gamemode.m_INDFORGearScriptSettings;
							break;
						case "CIV":
							gearScriptSettings = m_Gamemode.m_CIVILIANGearScriptSettings;
							break;
					}
					
					if (gearScriptSettings)
					{
						// Load the gear script config to get faction name
						ResourceName gearScriptResource = gearScriptSettings.m_rGearScript;
						if (gearScriptResource)
						{
							CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(
								BaseContainerTools.CreateInstanceFromContainer(
									BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()
								)
							);
							
							if (gearConfig && gearConfig.m_FactionName != "")
								factionName = gearConfig.m_FactionName;
						}
						
						// Determine radio configuration
						if (gearScriptSettings.m_bEnableRTORadios)
						{
							radiosValue = "RTO Radios";
						}
						else if (gearScriptSettings.m_bEnableGIRadios && gearScriptSettings.m_bEnableLeadershipRadios)
						{
							radiosValue = "SR + LR";
						}
						else if (gearScriptSettings.m_bEnableGIRadios)
						{
							radiosValue = "Short Range Only";
						}
						else if (gearScriptSettings.m_bEnableLeadershipRadios)
						{
							radiosValue = "LR Only";
						}
					}
				}
			}
		}
		
		m_wFactionNameValue.SetText(string.Format("Faction Name: %1", factionName));
		m_wRadiosValue.SetText(string.Format("Radios: %1", radiosValue));
		
		// Placeholder values for other equipment - TODO: implement actual checks
		m_wMapValue.SetText("Map: Yes");
		m_wBinocularsValue.SetText("Binoculars: Yes");
		m_wEntrenchingToolValue.SetText("Entrenching Tool: Yes");
		m_wNightVisionValue.SetText("Night Vision: None");
		m_wFlashlightValue.SetText("Flashlight: None");
	}
}
