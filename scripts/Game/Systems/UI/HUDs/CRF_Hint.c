class CRF_Hint : SCR_ScriptedWidgetComponent
{
	//------------------------------------------------------------------------------------------------
	// Widget references
	//------------------------------------------------------------------------------------------------
	protected Widget m_wMainWidget;          // Main container widget
	protected TextWidget m_wText;            // Text widget that displays the hint message
	protected ImageWidget m_wBG;             // Background image widget
	protected float m_fEndTime;              // Time when the hint should disappear
	
	//------------------------------------------------------------------------------------------------
	// Called when this component is attached to a widget
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		// Only process in play mode, not in editor
		if (!GetGame().InPlayMode())
			return;
		
		m_wMainWidget = w;
		
		super.HandlerAttached(m_wMainWidget);
		
		// Find and store references to child widgets
		m_wText = TextWidget.Cast(m_wMainWidget.FindAnyWidget("hintText"));
		m_wBG = ImageWidget.Cast(m_wMainWidget.FindAnyWidget("hintBG"));
	}
	
	//------------------------------------------------------------------------------------------------
	// Display a hint message for a specified duration
	// @param hinttext - The text message to display
	// @param duration - How long to display the hint in seconds
	//------------------------------------------------------------------------------------------------
	void ShowHint(string hinttext, float duration)
	{
		// Safety check - ensure widget is available
		if (!m_wMainWidget)
			return;
		
		// Cancel any existing hint timers
		GetGame().GetCallqueue().Remove(HintLoop);
		GetGame().GetCallqueue().Remove(FadeAndDeleteHintLoop);
		
		// Reset opacity in case it was fading
		m_wMainWidget.SetOpacity(1);
		
		// Set text content and make widgets visible
		m_wText.SetText(hinttext);
		m_wText.SetOpacity(1);
		m_wBG.SetOpacity(0.5);
		
		// Calculate when the hint should end
		m_fEndTime = GetGame().GetWorld().GetWorldTime() + duration;
		
		// Play notification sound
		AudioSystem.PlaySound("{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav");
		
		// Start monitoring hint duration with periodic checks
		GetGame().GetCallqueue().CallLater(HintLoop, 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------
	// Check if hint duration has expired
	// Called repeatedly until hint duration is reached
	//------------------------------------------------------------------------------------------------
	void HintLoop() 
	{
		// Check if widget still exists
		if (!m_wMainWidget)
		{
			GetGame().GetCallqueue().Remove(HintLoop);
			return;
		}
		
		// Check if hint duration has expired
		if (GetGame().GetWorld().GetWorldTime() >= m_fEndTime) 
		{
			// Stop the duration check and start the fade out process
			GetGame().GetCallqueue().Remove(HintLoop);
			GetGame().GetCallqueue().CallLater(FadeAndDeleteHintLoop, 0, true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Gradually fade out the hint and clean up resources
	// Called repeatedly until the hint is fully transparent
	//------------------------------------------------------------------------------------------------
	void FadeAndDeleteHintLoop() 
	{
		// Check if widget still exists
		if (!m_wMainWidget)
		{
			GetGame().GetCallqueue().Remove(FadeAndDeleteHintLoop);
			return;
		}
		
		float opacity = m_wMainWidget.GetOpacity();
		
		// If widget is still visible, reduce opacity gradually
		if (opacity > 0) 
		{
			m_wMainWidget.SetOpacity(opacity - 0.015);
		} 
		else 
		{
			// Widget is fully transparent, clean up resources
			GetGame().GetCallqueue().Remove(FadeAndDeleteHintLoop);
			delete m_wMainWidget;
			delete this;
		}
	}
}