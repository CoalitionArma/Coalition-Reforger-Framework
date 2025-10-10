
class CRF_GameTimerDisplay : SCR_InfoDisplayExtended
{
	//-------------------------------------------------------------------------
	// Widget References
	//-------------------------------------------------------------------------
	// Main timer elements
	protected SCR_MapEntity m_MapEntity;
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;
	
	// Ticket display for faction one (BLUFOR)
	protected ImageWidget m_wTicketOneImage; // Missing semicolon fixed
	protected TextWidget m_wTicketOneText; // Missing semicolon fixed
	protected TextWidget m_wTicketOneNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketOneBackground; // Missing semicolon fixed
	
	// Ticket display for faction two (OPFOR)
	protected ImageWidget m_wTicketTwoImage; // Missing semicolon fixed
	protected TextWidget m_wTicketTwoText; // Missing semicolon fixed
	protected TextWidget m_wTicketTwoNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketTwoBackground; // Missing semicolon fixed
	
	// Ticket display for faction three (INDFOR)
	protected ImageWidget m_wTicketThreeImage; // Missing semicolon fixed
	protected TextWidget m_wTicketThreeText; // Missing semicolon fixed
	protected TextWidget m_wTicketThreeNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketThreeBackground; // Missing semicolon fixed
	
	// Ticket display for faction four (CIV)
	protected ImageWidget m_wTicketFourImage; // Missing semicolon fixed
	protected TextWidget m_wTicketFourText; // Missing semicolon fixed
	protected TextWidget m_wTicketFourNumber; // Missing semicolon fixed
	protected ImageWidget m_wTicketFourBackground; // Missing semicolon fixed

	// Game state references
	protected CRF_SafestartManager m_SafestartManager;
	protected string m_sStoredServerWorldTime;
	protected string m_sServerWorldTime;
	protected SCR_PopUpNotification m_PopUpNotification = null;
	
	protected bool m_bUpdateTimer = false;
	
	//-------------------------------------------------------------------------
	// Initialization
	//-------------------------------------------------------------------------
	override protected void DisplayInit(IEntity owner)
	{
		super.DisplayInit(owner);
		// We really dont want this running on the server
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		// Set up periodic timer update every second
		m_bUpdateTimer = true;

		// Get notification system reference
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
	}
	
	/**
	 * Updates the game timer display UI elements.
	 * Called on each frame update to refresh the timer visualization.
	 * 
	 * @param owner The entity that owns this display component
	 * @param timeSlice The time elapsed since the last update in seconds
	 * @override Overrides the base class implementation to provide game timer specific display logic
	 */
	float m_fUpdateBuffer = 0;
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		// Only fire if in-game
		if (!GetGame().GetWorld().GetWorldTime() || !SCR_PlayerController.GetLocalControlledEntity())
			return;
		
		if (m_fUpdateBuffer >= 1)
		{
			if (m_bUpdateTimer)
				UpdateTimer();
			m_fUpdateBuffer = 0;
		}
		m_fUpdateBuffer += timeSlice;
		
		// Initialize references if they don't exist
		// This handles respawn support and first-time initialization
		// Check if we're in-game and already initialized
		if (!m_SafestartManager || !m_wTimer || !m_wBackground || !m_MapEntity) 
		{
			// Get game system references
			m_SafestartManager = CRF_SafestartManager.GetInstance();
			m_MapEntity = SCR_MapEntity.GetMapInstance();
			
			// Find and cast main timer widgets
			m_wTimer = TextWidget.Cast(m_wRoot.FindWidget("timeLeftTimer"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("timeLeftBackground"));
			
			// Find and cast faction one ticket widgets
			m_wTicketOneImage = ImageWidget.Cast(m_wRoot.FindWidget("TicketOneImage"));
			m_wTicketOneText = TextWidget.Cast(m_wRoot.FindWidget("TicketOneText"));
			m_wTicketOneNumber = TextWidget.Cast(m_wRoot.FindWidget("TicketOneNumber"));
			m_wTicketOneBackground = ImageWidget.Cast(m_wRoot.FindWidget("TicketOneBackground"));
			
			// Find and cast faction two ticket widgets
			m_wTicketTwoImage = ImageWidget.Cast(m_wRoot.FindWidget("TicketTwoImage"));
			m_wTicketTwoText = TextWidget.Cast(m_wRoot.FindWidget("TicketTwoText"));
			m_wTicketTwoNumber = TextWidget.Cast(m_wRoot.FindWidget("TicketTwoNumber"));
			m_wTicketTwoBackground = ImageWidget.Cast(m_wRoot.FindWidget("TicketTwoBackground"));
			
			// Find and cast faction three ticket widgets
			m_wTicketThreeImage = ImageWidget.Cast(m_wRoot.FindWidget("TicketThreeImage"));
			m_wTicketThreeText = TextWidget.Cast(m_wRoot.FindWidget("TicketThreeText"));
			m_wTicketThreeNumber = TextWidget.Cast(m_wRoot.FindWidget("TicketThreeNumber"));
			m_wTicketThreeBackground = ImageWidget.Cast(m_wRoot.FindWidget("TicketThreeBackground"));
			
			// Find and cast faction four ticket widgets
			m_wTicketFourImage = ImageWidget.Cast(m_wRoot.FindWidget("TicketFourImage"));
			m_wTicketFourText = TextWidget.Cast(m_wRoot.FindWidget("TicketFourText"));
			m_wTicketFourNumber = TextWidget.Cast(m_wRoot.FindWidget("TicketFourNumber"));
			m_wTicketFourBackground = ImageWidget.Cast(m_wRoot.FindWidget("TicketFourBackground"));
			
			return;
		}
	}
	
	//-------------------------------------------------------------------------
	// Timer Update - Called every second
	//-------------------------------------------------------------------------
	void UpdateTimer()
	{	
		// Skip update if essential components are missing or player is spectating
		if (!m_SafestartManager || !m_wTimer || !m_wBackground || !m_MapEntity || 
			!SCR_PlayerController.GetLocalControlledEntity() || 
			CRF_GamemodeManager.IsSpectator()) 
		{
			return;
		}
		
		// Handle HUD visibility toggle
		if(!CRF_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			// Hide all timer elements when HUD is disabled
			SetTimerVisibility(false);
			return;
		} 
		else 
		{
			// Show timer elements when HUD is enabled
			SetTimerVisibility(true);
		}
		
		// Handle ticket display when game is active and safestart is disabled
		if (CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME && 
			!CRF_SafestartManager.GetInstance().GetSafestartStatus())
		{
			UpdateTicketDisplay();
		}
		
		// Get current mission time
		m_sServerWorldTime = CRF_GamemodeManager.GetInstance().GetServerWorldTime();
		
		// Handle invalid time or end of mission
		if (m_sServerWorldTime == "N/A") 
		{
			m_bUpdateTimer = false;
			return;
		}
		
		// Skip update if in safestart, time is empty, or hasn't changed
		if (m_SafestartManager.GetSafestartStatus() || 
			m_sServerWorldTime.IsEmpty() || 
			m_sStoredServerWorldTime == m_sServerWorldTime) 
		{
			return;
		}
		
		// Store time for comparison in next update
		m_sStoredServerWorldTime = m_sServerWorldTime;
		
		// Handle time warnings (15min, 5min, end)
		HandleTimeWarnings();
		
		// Format and display time remaining
		UpdateTimeDisplay();
	}
	
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------
	
	/**
	* Sets visibility of timer and ticket UI elements
	* @param isVisible - whether elements should be visible
	*/
	protected void SetTimerVisibility(bool isVisible)
	{
		// Set main timer visibility
		m_wTimer.SetVisible(isVisible);
		m_wBackground.SetVisible(isVisible);
		
		// Set ticket one visibility
		m_wTicketOneImage.SetVisible(isVisible);
		m_wTicketOneText.SetVisible(isVisible);
		m_wTicketOneNumber.SetVisible(isVisible);
		m_wTicketOneBackground.SetVisible(isVisible);
	}
	
	/**
	* Updates the ticket display based on player faction or admin status
	*/
	protected void UpdateTicketDisplay()
	{
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		
		// Different display for admins vs regular players
		if (!SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
		{
			// For regular players - only show their faction's tickets
			string faction = SCR_FactionManager.SGetLocalPlayerFaction().GetFactionKey();
			
			// Skip ticket display for spectators
			if (faction != "SPEC") 
			{
				// Set color based on player's faction
				m_wTicketOneText.SetColor(factionManager.GetFactionByKey(faction).GetFactionColor());
				m_wTicketOneNumber.SetColor(factionManager.GetFactionByKey(faction).GetFactionColor());
				m_wTicketOneImage.SetColor(factionManager.GetFactionByKey(faction).GetFactionColor());
				
				// Display appropriate ticket count based on faction
				UpdateFactionTickets(faction, m_wTicketOneNumber);
			}
		} else {
			// For admins - show all factions' tickets
			
			// BLUFOR tickets (position one)
			m_wTicketOneText.SetColor(factionManager.GetFactionByKey("BLUFOR").GetFactionColor());
			m_wTicketOneNumber.SetColor(factionManager.GetFactionByKey("BLUFOR").GetFactionColor());
			m_wTicketOneImage.SetColor(factionManager.GetFactionByKey("BLUFOR").GetFactionColor());
			UpdateFactionTickets("BLUFOR", m_wTicketOneNumber);
			
			// OPFOR tickets (position two)
			m_wTicketTwoText.SetColor(factionManager.GetFactionByKey("OPFOR").GetFactionColor());
			m_wTicketTwoNumber.SetColor(factionManager.GetFactionByKey("OPFOR").GetFactionColor());
			m_wTicketTwoImage.SetColor(factionManager.GetFactionByKey("OPFOR").GetFactionColor());
			UpdateFactionTickets("OPFOR", m_wTicketTwoNumber);
			
			// INDFOR tickets (position three)
			m_wTicketThreeText.SetColor(factionManager.GetFactionByKey("INDFOR").GetFactionColor());
			m_wTicketThreeNumber.SetColor(factionManager.GetFactionByKey("INDFOR").GetFactionColor());
			m_wTicketThreeImage.SetColor(factionManager.GetFactionByKey("INDFOR").GetFactionColor());
			UpdateFactionTickets("INDFOR", m_wTicketThreeNumber);
			
			// CIV tickets (position four)
			m_wTicketFourText.SetColor(factionManager.GetFactionByKey("CIV").GetFactionColor());
			m_wTicketFourNumber.SetColor(factionManager.GetFactionByKey("CIV").GetFactionColor());
			m_wTicketFourImage.SetColor(factionManager.GetFactionByKey("CIV").GetFactionColor());
			UpdateFactionTickets("CIV", m_wTicketFourNumber);
		}
	}
	
	/**
	* Updates ticket display for a specific faction
	* @param faction - faction key ("BLUFOR", "OPFOR", "INDFOR", "CIV")
	* @param ticketWidget - the text widget to update
	*/
	protected void UpdateFactionTickets(string faction, TextWidget ticketWidget)
	{
		int tickets = -1;
		
		// Get ticket count from respawn manager
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (respawnManager)
			tickets = respawnManager.GetFactionTickets(faction);
		
		// Display "INF" for infinite tickets (-1) or the actual count
		if (tickets == -1)
			ticketWidget.SetText("INF");
		else
			ticketWidget.SetText(tickets.ToString());
	}
	
	/**
	* Handles time warnings at specific thresholds
	*/
	protected void HandleTimeWarnings()
	{
		// Play sound and show notification at specific time thresholds
		if (m_sServerWorldTime == "00:15:00" || 
			m_sServerWorldTime == "00:05:00" || 
			m_sServerWorldTime == "Mission Time Expired!") 
		{
			// Play warning sound
			AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
			
			// Show appropriate message based on time
			if (m_sServerWorldTime == "00:15:00") 
			{
				m_PopUpNotification.PopupMsg("Mission Ends In 15 Minutes!", 10);
			}
			else if (m_sServerWorldTime == "00:05:00") 
			{
				m_PopUpNotification.PopupMsg("Mission Ends In 5 Minutes!", 10);
			}
			else if (m_sServerWorldTime == "Mission Time Expired!") 
			{
				m_bUpdateTimer = false;
				m_PopUpNotification.PopupMsg(m_sServerWorldTime, 10);
				m_wTimer.SetText(m_sServerWorldTime);
				return;
			}
		}
	}
	
	/**
	* Updates the time display including formatting and visibility
	*/
	protected void UpdateTimeDisplay()
	{
		// Split time string into components
		array<string> timeParts = {};
		m_sServerWorldTime.Split(":", timeParts, false);
		
		// Determine visibility based on time remaining and map status
		bool shouldHideDisplay = (m_SafestartManager.GetSafestartStatus() || 
								((timeParts[0] != "00" || timeParts[1].ToInt() >= 5) && 
								(!m_MapEntity || !m_MapEntity.IsOpen())));
		
		// Set opacity based on visibility requirement
		if (shouldHideDisplay) 
		{
			SetWidgetOpacity(0);
			return;
		}
		else 
		{
			// Show timer
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
			
			// Determine if tickets should be shown
			CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
			bool hasAnyTickets = false;
			if (respawnManager)
			{
				hasAnyTickets = (respawnManager.GetFactionTickets("BLUFOR") > -1 || 
								respawnManager.GetFactionTickets("OPFOR") > -1 || 
								respawnManager.GetFactionTickets("INDFOR") > -1 || 
								respawnManager.GetFactionTickets("CIV") > -1);
			}
			
			if (hasAnyTickets)
			{
				// Show player faction tickets
				m_wTicketOneImage.SetOpacity(1);
				m_wTicketOneText.SetOpacity(1);
				m_wTicketOneNumber.SetOpacity(1);
				m_wTicketOneBackground.SetOpacity(1);
				
				// For admins, show all faction tickets
				if (SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()))
				{
					m_wTicketTwoImage.SetOpacity(1);
					m_wTicketTwoText.SetOpacity(1);
					m_wTicketTwoNumber.SetOpacity(1);
					m_wTicketTwoBackground.SetOpacity(1);
					
					m_wTicketThreeImage.SetOpacity(1);
					m_wTicketThreeText.SetOpacity(1);
					m_wTicketThreeNumber.SetOpacity(1);
					m_wTicketThreeBackground.SetOpacity(1);				
					
					m_wTicketFourImage.SetOpacity(1);
					m_wTicketFourText.SetOpacity(1);
					m_wTicketFourNumber.SetOpacity(1);
					m_wTicketFourBackground.SetOpacity(1);
				}
			}
		}
		
		// Format time display (drop the hour part if it's 00)
		string displayTime = m_sServerWorldTime;
		if (timeParts[0] == "00")
		{
			displayTime = string.Format("%1:%2", timeParts[1], timeParts[2]);
		}
		
		m_wTimer.SetText("Mission End: " + displayTime);
		
		// Set color based on time remaining
		if (timeParts[0] == "00" && timeParts[1].ToInt() < 5)
		{
			// Less than 5 minutes - red
			m_wTimer.SetColorInt(ARGB(255, 200, 65, 65));
		}
		else if (timeParts[0] == "00" && timeParts[1].ToInt() < 15)
		{
			// Less than 15 minutes - yellow
			m_wTimer.SetColorInt(ARGB(255, 230, 230, 0));
		}
		else
		{
			// Normal - light gray
			m_wTimer.SetColorInt(ARGB(255, 215, 215, 215));
		}
	}
	
	/**
	* Sets opacity of all widgets to the specified value
	* @param opacity - opacity value (0-1)
	*/
	protected void SetWidgetOpacity(float opacity)
	{
		// Main timer elements
		m_wTimer.SetOpacity(opacity);
		m_wBackground.SetOpacity(opacity);
		
		// Faction one ticket elements
		m_wTicketOneImage.SetOpacity(opacity);
		m_wTicketOneText.SetOpacity(opacity);
		m_wTicketOneNumber.SetOpacity(opacity);
		m_wTicketOneBackground.SetOpacity(opacity);
		
		// Faction two ticket elements
		m_wTicketTwoImage.SetOpacity(opacity);
		m_wTicketTwoText.SetOpacity(opacity);
		m_wTicketTwoNumber.SetOpacity(opacity);
		m_wTicketTwoBackground.SetOpacity(opacity);
		
		// Faction three ticket elements
		m_wTicketThreeImage.SetOpacity(opacity);
		m_wTicketThreeText.SetOpacity(opacity);
		m_wTicketThreeNumber.SetOpacity(opacity);
		m_wTicketThreeBackground.SetOpacity(opacity);
		
		// Faction four ticket elements
		m_wTicketFourImage.SetOpacity(opacity);
		m_wTicketFourText.SetOpacity(opacity);
		m_wTicketFourNumber.SetOpacity(opacity);
		m_wTicketFourBackground.SetOpacity(opacity);
	}
}

