class CRF_SpawnCountDownClass: SCR_BaseGameModeComponentClass
{
}

class CRF_SpawnCountDown: SCR_BaseGameModeComponent
{
	[Attribute("10")] int m_iTimer;
	[Attribute("{299DFE1A76794D78}Sounds/GunGame/timeBeep.wav")] ResourceName m_sTimerBeep;
	[Attribute("{60994C0146B8931A}Sounds/GunGame/patmanParasite.wav")] ResourceName m_sIntroVoiceLine;
	[RplProp()] float m_fGameStartTimer = m_iTimer;
	MenuBase m_wGameStartBase;
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	int m_iBeepTimer = m_iTimer;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		CheckGameStartUI(timeSlice);
		
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif
		
		CheckIfGamemodeStarted(timeSlice);
	}
	
	void CheckIfGamemodeStarted(float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif
		
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
			return;

		m_fGameStartTimer -= timeSlice;
		Replication.BumpMe();
	}
	
	void CheckGameStartUI(float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif
		
		if (!GetGame().GetPlayerController())
			return;
		
		if (!SCR_PlayerController.GetLocalControlledEntity())
			return;
		
		if (SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" && m_wGameStartBase)
		{
			GetGame().GetMenuManager().CloseMenu(m_wGameStartBase);
			return;
		}
		
		if (SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" )
			return;
		
		if (m_fGameStartTimer <= 0 && m_wGameStartBase)
		{
			GetGame().GetMenuManager().CloseMenu(m_wGameStartBase);
			AudioSystem.PlaySound(m_sIntroVoiceLine);
			return;
		}
		
		if (m_fGameStartTimer <= 0)
			return;
		
		if (!m_wGameStartBase)
		{
			m_wGameStartBase = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_GungameStart);
		}
		
		if (Math.Ceil(m_fGameStartTimer) < m_iBeepTimer)
		{
			m_iBeepTimer = Math.Ceil(m_fGameStartTimer);
			AudioSystem.PlaySound(m_sTimerBeep);
		}
		
		TextWidget.Cast(m_wGameStartBase.GetRootWidget().FindWidget("Timer")).SetText(Math.Round(m_fGameStartTimer).ToString());
		if (m_fGameStartTimer < 3)
			BlurWidget.Cast(m_wGameStartBase.GetRootWidget().FindWidget("Blur")).SetIntensity(m_fGameStartTimer/3);
	}
}