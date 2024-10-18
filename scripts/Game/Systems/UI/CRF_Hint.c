class CRF_Hint : SCR_ScriptedWidgetComponent
{
	protected Widget m_wMainWidget;
	protected TextWidget m_wText;
	protected ImageWidget m_wBG;
	protected float m_fEndTime;
	
	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;
		
		m_wMainWidget = w;
		
		super.HandlerAttached(m_wMainWidget);
		m_wText = TextWidget.Cast(m_wMainWidget.FindAnyWidget("hintText"));
		m_wBG = ImageWidget.Cast(m_wMainWidget.FindAnyWidget("hintBG"));
	}
	
	void ShowHint(string hinttext, float duration)
	{
		if(!m_wMainWidget)
			return;
		
		m_wText.SetText(hinttext);
		m_wText.SetOpacity(1);
		m_wBG.SetOpacity(0.5);
		
		m_fEndTime = GetGame().GetWorld().GetWorldTime() + duration;
		// Play sound
		AudioSystem.PlaySound("{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav");
		
		// Main loop to remove the hint after duration
		GetGame().GetCallqueue().CallLater(HintLoop, 1000, true);
	}
	
	void HintLoop() 
	{
		if(!m_wMainWidget)
		{
			GetGame().GetCallqueue().Remove(HintLoop);
			return;
		}
		
		if (GetGame().GetWorld().GetWorldTime() >= m_fEndTime) {
			GetGame().GetCallqueue().Remove(HintLoop);
			GetGame().GetCallqueue().CallLater(FadeAndDeleteHintLoop, 0, true);
		}
	}
	
	void FadeAndDeleteHintLoop() 
	{
		if(!m_wMainWidget)
		{
			GetGame().GetCallqueue().Remove(FadeAndDeleteHintLoop);
			return;
		}
		
		float opacity = m_wMainWidget.GetOpacity();
		if (opacity > 0) {
			m_wMainWidget.SetOpacity(opacity - 0.015);
		} else {
			GetGame().GetCallqueue().Remove(FadeAndDeleteHintLoop);
			delete m_wMainWidget;
		}
	}

}