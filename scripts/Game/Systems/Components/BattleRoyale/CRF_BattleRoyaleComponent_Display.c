/****************************************************************************************
 * CRF_BattleRoyaleComponent_Display.c
 - Contains all display logic for the Battle Royale component
 ****************************************************************************************/

class CRF_BattleRoyaleComponentDisplay : SCR_InfoDisplayExtended
{
	protected string m_sStoredString;
	protected TextWidget m_wTimer;
	protected TextWidget m_wStageText;
	protected TextWidget m_wPlayerCount;
	protected TextWidget m_wWinnerText;
	protected ImageWidget m_wBackground;
	protected BlurWidget m_wBlur;
	protected CRF_BattleRoyaleComponent m_BattleRoyaleComponent;
	
	// Cached toggle - read once at init, never re-checked
	protected bool m_bPlayerTrackingEnabled = true;
	
	// Store original sizes of elements
	protected float m_fOriginalTimerWidth = -1;
	protected float m_fOriginalTimerHeight = -1;
	protected float m_fOriginalTimerX = 0;
	protected float m_fOriginalTimerY = 0;
	protected float m_fOriginalStageX = 0;
	protected float m_fOriginalStageY = 0;
	protected float m_fOriginalBgWidth = -1;
	protected float m_fOriginalBgHeight = -1;
	protected float m_fOriginalBgX = 0;
	protected float m_fOriginalBgY = 0;
	
	// Animation variables
	protected float m_fTargetScale = 1.0;
	protected float m_fPulseIntensity = 0;
	
	// Optimization: track last timer value
	protected int m_iLastTimeLeft = -1;
	
	// Sound flags to prevent multiple plays
	protected bool m_bThirtySecondSoundPlayed = false;
	protected bool m_bFifteenSecondSoundPlayed = false;
	protected bool m_bFiveSecondSoundPlayed = false;
	
	// Smooth oscillation variables
	protected float m_fOscillationTime = 0;
	protected float m_fLastOscillationSpeed = 1.5;
	
	// Delay for timer display while resetting
	protected float m_fDisplayDelay = 0;
	protected bool m_bDelayActive = false;
	
	// Optimization: Cache displayed values to avoid unnecessary updates
	protected int m_iLastDisplayedPlayerCount = -1;
	protected bool m_bWinnersDisplayed = false;
	
	//------------------------------------------------------------------------------------------------
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
		
		// Get components if not initialized
		if (!m_BattleRoyaleComponent || !m_wTimer || !m_wBackground || !m_wStageText || !m_wPlayerCount || !m_wWinnerText) 
		{
			m_BattleRoyaleComponent = CRF_BattleRoyaleComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_BattleRoyaleComponent));
			if (!m_BattleRoyaleComponent) 
				return;
			
			// Cache player tracking toggle once - never re-read
			m_bPlayerTrackingEnabled = m_BattleRoyaleComponent.m_bEnablePlayerTracking;
			
			m_wTimer = TextWidget.Cast(m_wRoot.FindWidget("Timer"));
			m_wStageText = TextWidget.Cast(m_wRoot.FindWidget("StageText"));
			m_wPlayerCount = TextWidget.Cast(m_wRoot.FindWidget("PlayerCount"));
			m_wWinnerText = TextWidget.Cast(m_wRoot.FindWidget("WinnerText"));
			m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("Background"));
			m_wBlur = BlurWidget.Cast(m_wRoot.FindWidget("Blur"));
			
			if (!m_wTimer || !m_wStageText || !m_wBackground || !m_wPlayerCount || !m_wWinnerText) 
				return;
			
			// Store original sizes of elements
			if (m_fOriginalTimerWidth < 0)
			{
				m_fOriginalTimerWidth = 112;        
				m_fOriginalTimerHeight = 40;        
				m_fOriginalBgWidth = 128;           
				m_fOriginalBgHeight = 40;           
				m_fOriginalTimerX = -56;            
				m_fOriginalTimerY = 16;             
				m_fOriginalStageX = -72;            
				m_fOriginalStageY = 56;             
				m_fOriginalBgX = -64;               
				m_fOriginalBgY = 16;                
			}
		}
		
		// Check HUD visibility
		CRF_PlayerControllerManager pcManager = CRF_PlayerControllerManager.GetInstance();
		if (pcManager && !pcManager.m_bHUDVisible)
		{
			HideDisplay();
			m_wPlayerCount.SetOpacity(0);
			return;
		}
		
		// Check if Battle Royale is active at all
		if (!m_BattleRoyaleComponent.m_bBattleRoyaleActive)
		{
			HideDisplay();
			if (m_bPlayerTrackingEnabled)
				m_wPlayerCount.SetOpacity(0); // Only hide player count when BR is completely inactive
			// Reset delay when BR stops
			m_bDelayActive = false;
			m_fDisplayDelay = 0;
			return;
		}
		
		// Update player count and winner display only if tracking is enabled
		if (m_bPlayerTrackingEnabled)
		{
			UpdatePlayerCount();
			UpdateWinnerDisplay();
		}
		
		// Check if countdown is active (timer/stage display)
		if (!m_BattleRoyaleComponent.countDownActive)
		{
			HideDisplay();
			// Reset delay when countdown stops
			m_bDelayActive = false;
			m_fDisplayDelay = 0;
			return; // Player count already updated above, just hide timer/stage
		}
		
		// Start delay when countdown becomes active for the first time
		if (!m_bDelayActive)
		{
			m_bDelayActive = true;
			m_fDisplayDelay = 1.2; // 1.2 second delay
		}
		
		// Handle display delay for timer/stage only
		if (m_fDisplayDelay > 0)
		{
			m_fDisplayDelay -= timeSlice;
			HideDisplay();
			return; // Player count already updated above
		}

		// Update timer and stage when countdown is active and delay has passed
		UpdateStageText();
		UpdateTimer();
		
		// Only run animations when actually needed (performance optimization)
		if (m_fPulseIntensity > 0 || m_fTargetScale != 1.0)
		{
			UpdateAnimations(timeSlice);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void HideDisplay()
	{
		m_wTimer.SetOpacity(0);
		m_wStageText.SetOpacity(0);
		// Player count remains visible - don't hide it here
		// Winner text remains visible - don't hide it here (handled separately)
		m_wBackground.SetOpacity(0);
		
		// Reset sound flags when display is hidden
		m_bThirtySecondSoundPlayed = false;
		m_bFifteenSecondSoundPlayed = false;
		m_bFiveSecondSoundPlayed = false;
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateStageText()
	{
		if (!m_BattleRoyaleComponent.m_sStageText.IsEmpty())
		{
			m_wStageText.SetText(m_BattleRoyaleComponent.m_sStageText);
			m_wStageText.SetOpacity(1);
		}
		else
		{
			m_wStageText.SetOpacity(0);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdatePlayerCount()
	{
		int playerCount = m_BattleRoyaleComponent.m_iAlivePlayerCount;
		
		// Only update if count changed
		if (playerCount == m_iLastDisplayedPlayerCount)
			return;
		
		m_iLastDisplayedPlayerCount = playerCount;
		m_wPlayerCount.SetText(string.Format("Alive: %1", playerCount));
		m_wPlayerCount.SetOpacity(1);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateTimer()
	{
		// Display timer text from dedicated m_sTimerText property 
		if (!m_BattleRoyaleComponent.m_sTimerText.IsEmpty())
		{
			if (m_sStoredString != m_BattleRoyaleComponent.m_sTimerText) 
			{
				m_wTimer.SetText(m_BattleRoyaleComponent.m_sTimerText);
				m_sStoredString = m_BattleRoyaleComponent.m_sTimerText;
			}
			
			// Determine animation intensity based on timer value
			float newTargetScale = 1.0;
			float newPulseIntensity = 0;
			
			// Access the current timer value
			int timeLeft = m_BattleRoyaleComponent.m_iCurrentTimer;
			
			// Only recalculate animation parameters if timer value changed
			if (timeLeft != m_iLastTimeLeft)
			{
				m_iLastTimeLeft = timeLeft;
				
				// Sound alerts
				if (timeLeft == 30 && !m_bThirtySecondSoundPlayed)
				{
					AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
					m_bThirtySecondSoundPlayed = true;
				}
				else if (timeLeft == 15 && !m_bFifteenSecondSoundPlayed)
				{
					AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
					m_bFifteenSecondSoundPlayed = true;
				}
				else if (timeLeft == 5 && !m_bFiveSecondSoundPlayed)
				{
					AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
					m_bFiveSecondSoundPlayed = true;
				}
				
				if (timeLeft <= 30 && timeLeft >= 0)
				{
					// Reduced scale increase from 1.0 to 1.25 as we approach 0
					float progress = Math.InverseLerp(30, 0, timeLeft);
					newTargetScale = Math.Lerp(1.05, 1.25, progress);
					
					// Add pulse intensity that increases as time runs out
					float pulseProgress = Math.InverseLerp(30, 0, timeLeft);
					newPulseIntensity = Math.Lerp(0.0, 0.9, pulseProgress);
				}
				
				m_fTargetScale = newTargetScale;
				m_fPulseIntensity = newPulseIntensity;
			}
			
			m_wTimer.SetOpacity(1);
			m_wBackground.SetOpacity(1);
		} 
		else 
		{
			m_wTimer.SetOpacity(0);
			m_wBackground.SetOpacity(0);
			
			// Reset animation state when timer is hidden
			m_fTargetScale = 1.0;
			m_fPulseIntensity = 0;
			m_fOscillationTime = 0;
			m_iLastTimeLeft = -1;
			
			// Reset sound flags
			m_bThirtySecondSoundPlayed = false;
			m_bFifteenSecondSoundPlayed = false;
			m_bFiveSecondSoundPlayed = false;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateAnimations(float timeSlice)
	{
		// Update oscillation time at full framerate for smooth pulsing
		m_fOscillationTime += timeSlice;
		
		// Apply smooth pulsing effect if active
		if (m_fPulseIntensity > 0)
		{
			// Calculate oscillation speed that increases as timer approaches 0
			float speedProgress = m_fPulseIntensity;
			float oscillationSpeed = Math.Lerp(1.5, 0.3, speedProgress);
			
			// Smooth speed transition
			if (Math.AbsFloat(oscillationSpeed - m_fLastOscillationSpeed) > 0.01)
			{
				float currentPhase = Math.Mod(m_fOscillationTime * Math.PI2 / m_fLastOscillationSpeed, Math.PI2);
				m_fOscillationTime = (currentPhase * oscillationSpeed) / Math.PI2;
				m_fLastOscillationSpeed = oscillationSpeed;
			}
			
			// Calculate sine wave for smooth oscillation
			float sineWave = Math.Sin(m_fOscillationTime * Math.PI2 / oscillationSpeed);
			float normalizedWave = sineWave * 0.5 + 0.5;
			
			// Calculate glow intensity
			float glowIntensity = normalizedWave * m_fPulseIntensity;
			
			// Apply colors with smooth oscillation
			m_wTimer.SetColor(Color(0.053 + glowIntensity * 0.5, 0.578 + glowIntensity * 0.422, 0.053 + glowIntensity * 0.5, 1));
			m_wBackground.SetColor(Color(0.0894 + glowIntensity * 0.3, 0.0878 + glowIntensity * 0.4, 0.0878 + glowIntensity * 0.3, 0.333 + glowIntensity * 0.2));
			m_wTimer.SetOpacity(1.0);
			m_wBackground.SetOpacity(1.0);
		}
		else
		{
			// Reset to original colors and opacity
			m_wTimer.SetColor(Color(0.053, 0.578, 0.053, 1));
			m_wBackground.SetColor(Color(0.0894, 0.0878, 0.0878, 0.333));
			m_wTimer.SetOpacity(1.0);
			m_wBackground.SetOpacity(1.0);
			
			// Reset oscillation when not pulsing
			m_fOscillationTime = 0;
			m_fLastOscillationSpeed = 2.5;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateWinnerDisplay()
	{
		// Check if we have winners
		if (!m_BattleRoyaleComponent.m_aWinnerPlayerIds || m_BattleRoyaleComponent.m_aWinnerPlayerIds.Count() == 0)
		{
			m_wWinnerText.SetOpacity(0);
			if (m_wBlur)
				m_wBlur.SetOpacity(0);
			m_bWinnersDisplayed = false; // Reset flag when no winners
			return;
		}
		
		// Early exit: Already displayed, don't update every frame
		if (m_bWinnersDisplayed)
			return;
		
		if (m_BattleRoyaleComponent.m_bDebugEnabled)
			Print(string.Format("[CRF_BR_Display] Displaying %1 winners", m_BattleRoyaleComponent.m_aWinnerPlayerIds.Count()));
		
		// Build winner text (only once)
		string winnerText = "Winners:\n";
		
		foreach (int playerId : m_BattleRoyaleComponent.m_aWinnerPlayerIds)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!playerName.IsEmpty())
				winnerText += playerName + "  ";
		}
		
		// Update and show winner text and blur
		m_wWinnerText.SetText(winnerText);
		m_wWinnerText.SetOpacity(1);
		if (m_wBlur)
			m_wBlur.SetOpacity(1);
		m_bWinnersDisplayed = true; // Mark as displayed - won't update again
		
		if (m_BattleRoyaleComponent.m_bDebugEnabled)
			Print("[CRF_BR_Display] Winner text set and visible");
	}
}
