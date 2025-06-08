class CRF_PolyZoneEffectHUD : SCR_ScriptedWidgetComponent
{
	TextWidget m_wCounter;
	float m_fTime;
	string m_sString;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wCounter = TextWidget.Cast(w.FindAnyWidget("Counter"));
	}
	
	void SetTime(float time)
	{
		m_fTime = time;
		Update(0.0);
	}
	
	void SetString(string str)
	{
		m_sString = str;
	}
	
	bool ShowVignette()
	{
		return false;
	}
	
	bool ShowScreenBlure()
	{
		return false;
	}
	
	void Update(float timeSlice)
	{
		m_fTime -= timeSlice;
		if (m_fTime < 0) m_fTime = 0;
		int miliseconds = Math.Mod(m_fTime, 1) * 100;
		int seconds = Math.Mod(m_fTime, 60);
		int minutes = (m_fTime / 60);
		m_wCounter.SetTextFormat("%1:%2.%3", minutes.ToString(2), seconds.ToString(2), miliseconds.ToString(2));
	}
}