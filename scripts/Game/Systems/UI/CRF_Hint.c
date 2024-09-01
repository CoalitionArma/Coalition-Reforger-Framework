class CRF_Hint : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wText;
	protected float m_fEndTime;
	
	override void HandlerAttached(Widget w)
	{
		if (!GetGame().InPlayMode())
			return;
		
		super.HandlerAttached(w);
		m_wText = TextWidget.Cast(m_wRoot.FindAnyWidget("hintText"));
		m_wText.SetVisible(false);
	}
	
	void ShowHint(string hinttext, float duration)
	{
		m_wText.SetText(hinttext);
		m_wText.SetVisible(true);
		
		m_fEndTime = GetGame().GetWorld().GetWorldTime() + duration;
		
		// Play sound
		AudioSystem.PlaySound("{A4D15A2A486BD70A}Sounds/UI/Samples/Editor/UI_E_Notification_Default.wav");
		// Main loop to remove the hint after duration
		GetGame().GetCallqueue().CallLater(HintLoop, 1000, true);
	}
	
	void HintLoop() 
	{
		if (GetGame().GetWorld().GetWorldTime() >= m_fEndTime) {
			GetGame().GetCallqueue().Remove(HintLoop);
			m_wText.SetVisible(false);
		}
	}
}