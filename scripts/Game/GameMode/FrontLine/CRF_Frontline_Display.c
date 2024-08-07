class CRF_Frontline_HUD : SCR_InfoDisplay
{
	protected ImageWidget m_wASite;
	protected ImageWidget m_wBSite;
	protected ImageWidget m_wCSite;
	protected ImageWidget m_wDSite;
	protected ImageWidget m_wESite;
	protected ImageWidget m_wASiteLock;
	protected ImageWidget m_wBSiteLock;
	protected ImageWidget m_wCSiteLock;
	protected ImageWidget m_wDSiteLock;
	protected ImageWidget m_wESiteLock;
	protected ProgressBarWidget m_wSiteCaptureBar;
	protected TextWidget m_wSiteCaptureText;
	
	protected float m_fStoredOpcaity;
	protected bool m_bStoredFadeInBoolean;
	protected bool m_bStoredProgressBarBoolean;

	protected CRF_GameModePlayerComponent m_GameModePlayerComponent = null;
	protected CRF_FrontlineGameModeComponent m_FrontlineGameModeComponent = null;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);
		
		if(!m_FrontlineGameModeComponent)
		{
			m_FrontlineGameModeComponent = CRF_FrontlineGameModeComponent.GetInstance();
			m_wRoot.SetOpacity(0);
			return;
		}
		
		if (!m_GameModePlayerComponent || !m_wASite || !m_wBSite || !m_wCSite || !m_wDSite || !m_wESite || !m_wASiteLock || !m_wBSiteLock || !m_wCSiteLock || !m_wDSiteLock || !m_wESiteLock || !m_wSiteCaptureBar || !m_wSiteCaptureText) {
			m_GameModePlayerComponent    = CRF_GameModePlayerComponent.GetInstance();
			m_wASite                     = ImageWidget.Cast(m_wRoot.FindWidget("ASite"));
			m_wBSite                     = ImageWidget.Cast(m_wRoot.FindWidget("BSite"));
			m_wCSite                     = ImageWidget.Cast(m_wRoot.FindWidget("CSite"));
			m_wDSite                     = ImageWidget.Cast(m_wRoot.FindWidget("DSite"));
			m_wESite                     = ImageWidget.Cast(m_wRoot.FindWidget("ESite"));
			m_wASiteLock                 = ImageWidget.Cast(m_wRoot.FindWidget("ASiteLock"));
			m_wBSiteLock                 = ImageWidget.Cast(m_wRoot.FindWidget("BSiteLock"));
			m_wCSiteLock                 = ImageWidget.Cast(m_wRoot.FindWidget("CSiteLock"));
			m_wDSiteLock                 = ImageWidget.Cast(m_wRoot.FindWidget("DSiteLock"));
			m_wESiteLock                 = ImageWidget.Cast(m_wRoot.FindWidget("ESiteLock"));
			m_wSiteCaptureBar            = ProgressBarWidget.Cast(m_wRoot.FindWidget("SiteCaptureBar"));
			m_wSiteCaptureText           = TextWidget.Cast(m_wRoot.FindWidget("SiteCaptureText"));
			return;
		};
		
		CRF_SafestartGameModeComponent safestart = CRF_SafestartGameModeComponent.GetInstance();
		if(safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			m_wRoot.SetOpacity(0);
		else
			m_wRoot.SetOpacity(1);
		
		string siteCaptureBarText = m_FrontlineGameModeComponent.m_sHudMessage;
		m_wSiteCaptureText.SetText(siteCaptureBarText);
		
		m_bStoredProgressBarBoolean = false;
		
		foreach(int i, string zoneName : m_FrontlineGameModeComponent.m_aZoneObjectNames)
		{
			string status = m_FrontlineGameModeComponent.m_aZonesStatus[i];
			string imageTexture;
			int imageColor;
			
			ImageWidget widget;
			ImageWidget lockWidget;
			
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
			
			FactionKey zonefaction = zoneStatusArray[0];
			string zoneState = zoneStatusArray[1];
			
			switch(zoneName)
			{
				case "aZone" : {
					widget = m_wASite; 
					lockWidget = m_wASiteLock; 
					break;
				};
				case "bZone" : {
					widget = m_wBSite; 
					lockWidget = m_wBSiteLock; 
					break;
				};
				case "cZone" : {
					widget = m_wCSite; 
					lockWidget = m_wCSiteLock; 
					break;
				};
				case "dZone" : {
					widget = m_wDSite; 
					lockWidget = m_wDSiteLock; 
					break;
				};
				case "eZone" : {
					widget = m_wESite; 
					lockWidget = m_wESiteLock; 
					break;
				};
			}
			
			if(!widget || !lockWidget)
				return;
			
			string nickname;
			
			switch(zonefaction)
			{
				case m_FrontlineGameModeComponent.m_BluforSide : {
					widget.SetColorInt(ARGB(255, 0, 25, 225)); 
					nickname = m_FrontlineGameModeComponent.m_sBluforSideNickname; 
					break;
				}; //Blufor
				case m_FrontlineGameModeComponent.m_OpforSide  : {
					widget.SetColorInt(ARGB(255, 225, 25, 0)); 
					nickname = m_FrontlineGameModeComponent.m_sOpforSideNickname;   
					break;
				}; //Opfor
				default : { 
					widget.SetColorInt(ARGB(255, 225, 225, 225)); 
					break; 
				}; //Uncaptured
			}
			
			if(zoneState == "Locked")
				lockWidget.SetOpacity(1);
			else
				lockWidget.SetOpacity(0);
			
			if(zoneState.ToInt() != 0)
			{
				m_wSiteCaptureBar.SetCurrent(zoneState.ToInt());
				m_bStoredProgressBarBoolean = true;
				
				if(zoneState.ToInt() > m_FrontlineGameModeComponent.m_iZoneCaptureTime)
					m_wSiteCaptureBar.SetMax(m_FrontlineGameModeComponent.m_iInitialTime); // Only other time we use the progress bar
				else
					m_wSiteCaptureBar.SetMax(m_FrontlineGameModeComponent.m_iZoneCaptureTime);
				
				switch(zonefaction)
				{
					case m_FrontlineGameModeComponent.m_BluforSide : { m_wSiteCaptureBar.SetColorInt(ARGB(255, 0, 25, 225));    break;}; //Blufor
					case m_FrontlineGameModeComponent.m_OpforSide  : { m_wSiteCaptureBar.SetColorInt(ARGB(255, 225, 25, 0));    break;}; //Opfor
					default                                        : { m_wSiteCaptureBar.SetColorInt(ARGB(255, 225, 225, 225)); break;}; //Uncaptured
				}
				
				if (m_bStoredFadeInBoolean)
				{
					m_fStoredOpcaity = m_fStoredOpcaity + 0.0125;
					if (m_fStoredOpcaity >= 1)
						m_bStoredFadeInBoolean = !m_bStoredFadeInBoolean;
				};
				
				if (!m_bStoredFadeInBoolean)
				{
					m_fStoredOpcaity = m_fStoredOpcaity - 0.0125;
					if (m_fStoredOpcaity <= 0)
						m_bStoredFadeInBoolean = !m_bStoredFadeInBoolean;
				};
				
				widget.SetOpacity(m_fStoredOpcaity);
				m_wSiteCaptureBar.SetOpacity(1);
			} else {
				widget.SetOpacity(1);
			};
		}
		
		if(!m_bStoredProgressBarBoolean) 
			m_wSiteCaptureBar.SetOpacity(0);
	}
}