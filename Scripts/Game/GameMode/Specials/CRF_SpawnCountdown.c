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
	protected int m_iLastReplicatedSecond = -1;
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	protected static ResourceName INITIAL_ENTITY_PREFAB = "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et";

	int m_iBeepTimer = m_iTimer;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		CheckGameStartUI(owner, timeSlice);

		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		CheckIfGamemodeStarted(owner, timeSlice);
	}

	void CheckIfGamemodeStarted(IEntity owner, float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		if (COA_Gamemode.GetInstance().m_GamemodeState != COA_EGamemodeState.GAME)
			return;

		m_fGameStartTimer -= timeSlice;

		// Only replicate when the displayed second changes - not every frame
		int currentSecond = Math.Floor(m_fGameStartTimer);
		if (currentSecond != m_iLastReplicatedSecond)
		{
			m_iLastReplicatedSecond = currentSecond;
			Replication.BumpMe();
		}

		if (m_fGameStartTimer <= 0)
			ClearEventMask(owner, EntityEvent.FRAME);
	}

	void CheckGameStartUI(IEntity owner, float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif

		if (!GetGame().GetPlayerController())
			return;

		IEntity localEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (!localEntity)
			return;

		bool isInitialEntity = localEntity.GetPrefabData().GetPrefabName() == INITIAL_ENTITY_PREFAB;
		if (isInitialEntity)
		{
			if (m_wGameStartBase)
				GetGame().GetMenuManager().CloseMenu(m_wGameStartBase);
			return;
		}

		if (m_fGameStartTimer <= 0)
		{
			if (m_wGameStartBase)
			{
				GetGame().GetMenuManager().CloseMenu(m_wGameStartBase);
				AudioSystem.PlaySound(m_sIntroVoiceLine);
			}
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		}
		
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