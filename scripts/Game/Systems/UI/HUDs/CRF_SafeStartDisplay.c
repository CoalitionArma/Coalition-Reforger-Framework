class CRF_SafeStartDisplay : SCR_InfoDisplayExtended
{
	//------------------------------------------------------------------------------------------------
	// UI widget references for timer display
	//------------------------------------------------------------------------------------------------
	protected ImageWidget m_wTimerImage;
	protected TextWidget m_wTimerDescription;
	protected TextWidget m_wTimerText;
	protected TextWidget m_wMissionStart;
	protected TextWidget m_wMissionStart2;
	
	//------------------------------------------------------------------------------------------------
	// UI widget references for faction status display
	//------------------------------------------------------------------------------------------------
	protected ImageWidget m_wFactionsBackground;
	protected OverlayWidget m_wBluforFrame;
	protected OverlayWidget m_wOpforFrame;
	protected OverlayWidget m_wIndforFrame;
	protected OverlayWidget m_wCivFrame;
	protected TextWidget m_wBluforReady;
	protected TextWidget m_wOpforReady;
	protected TextWidget m_wIndforReady;
	protected TextWidget m_wCivReady;
	protected PanelWidget m_wFactionsPanel;
	
	//------------------------------------------------------------------------------------------------
	// Manager references
	//------------------------------------------------------------------------------------------------
	protected CRF_SafestartManager m_SafestartManager = null;
	protected SCR_FactionManager m_FactionManager = null;
	
	//------------------------------------------------------------------------------------------------
	// State variables
	//------------------------------------------------------------------------------------------------
	protected float m_fCurrentOpacity = 0;
	protected bool m_bAlreadyActivated = false;

	//------------------------------------------------------------------------------------------------
	// Override functions
	//------------------------------------------------------------------------------------------------
	/**
	 * Main update method called each frame
	 * @param owner The entity that owns this display
	 * @param timeSlice Time passed since last update
	 */
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		// Initialize references if needed
		if (!AreReferencesValid()) 
		{
			InitializeReferences();
			return;
		}
		
		// Handle HUD visibility
		if (!CRF_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			HideAllUIElements();
			return;
		} 
		else 
		{
			ShowMainUIElements();
		}
		
		// Update mission start animation
		if (m_fCurrentOpacity > 0)
		{
			m_fCurrentOpacity = m_fCurrentOpacity - 0.0025;
			
			m_wMissionStart.SetOpacity(m_fCurrentOpacity);
			m_wMissionStart2.SetOpacity(m_fCurrentOpacity);
		}
		
		// Handle safestart status changes
		if (m_SafestartManager.GetSafestartStatus()) 
		{
			if (!m_bAlreadyActivated) 
			{
				StopMission();
				m_bAlreadyActivated = true;
			}
			
			UpdatePlayedFactions();
			UpdateTimer();
		}
		
		if (!m_SafestartManager.GetSafestartStatus() && m_bAlreadyActivated) 
		{
			StartMission();
			m_bAlreadyActivated = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper methods
	//------------------------------------------------------------------------------------------------
	/**
	 * Checks if all required references are initialized
	 * @return true if all references are valid, false otherwise
	 */
	protected bool AreReferencesValid()
	{
		if (!m_SafestartManager || !m_FactionManager || 
			!m_wTimerImage || !m_wTimerDescription || !m_wTimerText || 
			!m_wMissionStart || !m_wMissionStart2 || !m_wFactionsBackground || 
			!m_wBluforFrame || !m_wOpforFrame || !m_wIndforFrame || !m_wCivFrame || 
			!m_wBluforReady || !m_wOpforReady || !m_wIndforReady || !m_wCivReady || 
			!m_wFactionsPanel) 
		{
			return false;
		}
		
		return true;
	}
	
	/**
	 * Initializes all required references to managers and UI widgets
	 */
	protected void InitializeReferences()
	{
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		
		m_wTimerImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("TimerImage"));
		m_wTimerDescription = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerDescription"));
		m_wTimerText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimerText"));
		m_wMissionStart = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStart"));
		m_wMissionStart2 = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionStart2"));
		m_wFactionsBackground = ImageWidget.Cast(m_wRoot.FindAnyWidget("FactionsBackground"));
		m_wBluforFrame = OverlayWidget.Cast(m_wRoot.FindAnyWidget("BluforFrame"));
		m_wOpforFrame = OverlayWidget.Cast(m_wRoot.FindAnyWidget("OpforFrame"));
		m_wIndforFrame = OverlayWidget.Cast(m_wRoot.FindAnyWidget("IndforFrame"));
		m_wCivFrame = OverlayWidget.Cast(m_wRoot.FindAnyWidget("CivFrame"));
		m_wBluforReady = TextWidget.Cast(m_wRoot.FindAnyWidget("BluforReady"));
		m_wOpforReady = TextWidget.Cast(m_wRoot.FindAnyWidget("OpforReady"));
		m_wIndforReady = TextWidget.Cast(m_wRoot.FindAnyWidget("IndforReady"));
		m_wCivReady = TextWidget.Cast(m_wRoot.FindAnyWidget("CivReady"));
		m_wFactionsPanel = PanelWidget.Cast(m_wRoot.FindAnyWidget("FactionsPanel"));
	}
	
	/**
	 * Hides all UI elements when HUD is not visible
	 */
	protected void HideAllUIElements()
	{
		// Hide timer elements
		m_wTimerDescription.SetVisible(false);
		m_wTimerText.SetVisible(false);
		m_wTimerImage.SetVisible(false);
	
		// Hide faction elements
		m_wFactionsBackground.SetVisible(false);
		m_wFactionsPanel.SetVisible(false);
		m_wBluforFrame.SetVisible(false);
		m_wOpforFrame.SetVisible(false);
		m_wIndforFrame.SetVisible(false);
		m_wCivFrame.SetVisible(false);
		
		// Hide mission elements
		m_wMissionStart.SetVisible(false);
		m_wMissionStart2.SetVisible(false);
	}
	
	/**
	 * Shows main UI elements when HUD is visible
	 */
	protected void ShowMainUIElements()
	{
		// Show timer elements
		m_wTimerDescription.SetVisible(true);
		m_wTimerText.SetVisible(true);
		m_wTimerImage.SetVisible(true);
	
		// Show faction elements
		m_wFactionsBackground.SetVisible(true);
		m_wFactionsPanel.SetVisible(true);
		
		// Show mission elements
		m_wMissionStart.SetVisible(true);
		m_wMissionStart2.SetVisible(true);
	}
	
	/**
	 * Updates the faction status display with current ready states
	 */
	protected void UpdatePlayedFactions() 
	{
		array<string> outFactionsReady = m_SafestartManager.GetWhosReady();
		
		if (!outFactionsReady || outFactionsReady.IsEmpty()) 
		{
			return;
		}
		
		foreach (int i, string factionReady : outFactionsReady) 
		{
			int colorToSet = 0;
			
			// Set color based on faction status
			if (factionReady == "Ready")     
			{
				colorToSet = ARGB(185, 0, 190, 85);   // Green for ready
			}
			
			if (factionReady == "Not Ready") 
			{
				colorToSet = ARGB(185, 200, 65, 65);  // Red for not ready
			}
			
			if (factionReady == "N/A")       
			{
				colorToSet = ARGB(185, 135, 135, 135); // Gray for no faction
			}
		
			// Update UI for the appropriate faction index
			switch (i) 
			{
				case 0: // BLUFOR
				{
					m_wBluforReady.SetText(factionReady); 
					m_wBluforReady.SetColorInt(colorToSet); 
					
					if (factionReady != "N/A")
					{
						m_wBluforFrame.SetVisible(true);
					}
					else
					{
						m_wBluforFrame.SetVisible(false);
					}
					break;
				}
				
				case 1: // OPFOR
				{
					m_wOpforReady.SetText(factionReady);  
					m_wOpforReady.SetColorInt(colorToSet);  
					
					if (factionReady != "N/A")
					{
						m_wOpforFrame.SetVisible(true);
					}
					else
					{
						m_wOpforFrame.SetVisible(false);
					}
					break;
				}
				
				case 2: // INDFOR
				{
					m_wIndforReady.SetText(factionReady); 
					m_wIndforReady.SetColorInt(colorToSet);
					 
					if (factionReady != "N/A")
					{
						m_wIndforFrame.SetVisible(true);
					}
					else
					{
						m_wIndforFrame.SetVisible(false);
					}
					break;
				}
				
				case 3: // CIVILIAN
				{
					m_wCivReady.SetText(factionReady);    
					m_wCivReady.SetColorInt(colorToSet);    
					
					if (factionReady != "N/A")
					{
						m_wCivFrame.SetVisible(true);
					}
					else
					{
						m_wCivFrame.SetVisible(false);
					}
					break;
				}
			}
		}
	}
	
	/**
	 * Updates the timer display with current server world time or countdown timer
	 */
	protected void UpdateTimer()
	{	
		// If in countdown mode, show the countdown timer instead of world time
		if (m_SafestartManager && m_SafestartManager.GetCountdownMode())
		{
			m_wTimerText.SetText(m_SafestartManager.GetFormattedSafeStartTimeRemaining());
		}
		else
		{
			m_wTimerText.SetText(CRF_GamemodeManager.GetInstance().GetServerWorldTime());
		}
	}

	/**
	 * Activates the safestart display mode (waiting for mission start)
	 */
	protected void StopMission()
	{
		// Show timer elements
		m_wTimerDescription.SetOpacity(1);
		m_wTimerText.SetOpacity(1);
		m_wTimerImage.SetOpacity(1);

		// Show faction elements
		m_wFactionsBackground.SetOpacity(1);
		m_wFactionsPanel.SetOpacity(1);
		
		// Hide mission start notification
		m_wMissionStart.SetOpacity(0);
		m_wMissionStart2.SetOpacity(0);
		
		m_fCurrentOpacity = 0;
	}
	
	/**
	 * Activates the mission start display mode
	 */
	protected void StartMission()
	{
		CRF_PlayerControllerManager.GetInstance().DisplayTitleCard();
		// Hide timer elements
		m_wTimerDescription.SetOpacity(0);
		m_wTimerText.SetOpacity(0);
		m_wTimerImage.SetOpacity(0);
		
		// Hide faction elements
		m_wFactionsBackground.SetOpacity(0);
		m_wFactionsPanel.SetOpacity(0);
		m_wBluforFrame.SetVisible(false);
		m_wOpforFrame.SetVisible(false);
		m_wIndforFrame.SetVisible(false);
		m_wCivFrame.SetVisible(false);
			
		// Show mission start notification (will fade out)
		m_wMissionStart.SetOpacity(1);
		m_wMissionStart2.SetOpacity(1);
		
		m_fCurrentOpacity = 1;
	}
}