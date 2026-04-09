class CRF_CapturePointDisplay : SCR_InfoDisplayExtended
{
	bool m_bShow = false;
	float  m_fBLUFORRatio = 0;
	float  m_fOPFORRatio = 0;
	ProgressBarWidget m_BLUFORProgress;
	ProgressBarWidget m_OPFORProgress;
	Widget m_FrontIcon;
	float m_fDisplayedBLUFORRatio = 0.0;
	float m_fDisplayedOPFORRatio = 0.0;
	float m_fDisplayedFrontX = 28.0;
	float m_fLerpSpeed = 5.0;
	CRF_GamemodeManager m_GamemodeManager;
	Widget m_PanelWidget;
	TextWidget m_TimeText;
	
	override protected void DisplayInit(IEntity owner)
	{
		super.DisplayInit(owner);
		m_BLUFORProgress = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("USBAR"));
		m_OPFORProgress = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("GERBAR"));
		m_FrontIcon = m_wRoot.FindAnyWidget("Image4");
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);
		
		if (!m_GamemodeManager)
			m_GamemodeManager = CRF_GamemodeManager.GetInstance();
		
		if (m_bShow)
			m_wRoot.SetVisible(true);
		else
			m_wRoot.SetVisible(false);
		
		if (!m_BLUFORProgress)
			m_BLUFORProgress = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("USBAR"));
		
		if (!m_OPFORProgress)
			m_OPFORProgress = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("GERBAR"));
		
		if (!m_FrontIcon)
			m_FrontIcon = m_wRoot.FindAnyWidget("Image4");
		
		if (!m_PanelWidget)
			m_PanelWidget = m_wRoot.FindAnyWidget("TimePanel");
		
		if (!m_TimeText)
			m_TimeText = TextWidget.Cast(m_wRoot.FindAnyWidget("TimeText"));
		
		// Smooth the displayed values toward the actual values
		float t = Math.Clamp(timeSlice * m_fLerpSpeed, 0.0, 1.0);
		
		m_fDisplayedBLUFORRatio = Math.Lerp(m_fDisplayedBLUFORRatio, m_fBLUFORRatio, t);
		m_fDisplayedOPFORRatio = Math.Lerp(m_fDisplayedOPFORRatio, m_fOPFORRatio, t);
		
		float targetX = 694.0 * (m_fDisplayedBLUFORRatio / 100.0) + 28.0;
		m_fDisplayedFrontX = Math.Lerp(m_fDisplayedFrontX, targetX, t);
		
		FrameSlot.SetPosX(m_FrontIcon, m_fDisplayedFrontX);
		
		m_BLUFORProgress.SetCurrent(m_fDisplayedBLUFORRatio);
		m_OPFORProgress.SetCurrent(m_fDisplayedOPFORRatio);
		
		
		if (m_GamemodeManager.m_iTimeOnObjective < 61)
		{
			m_PanelWidget.SetVisible(true);
			m_TimeText.SetText(m_GamemodeManager.m_iTimeOnObjective.ToString());
		}
		else
			m_PanelWidget.SetVisible(false);
	}
}