modded enum ChimeraMenuPreset : ScriptMenuPresetEnum
{
	CRF_RespawnMenu
}


class CRF_RespawnMenu: ChimeraMenuBase
{
	protected Widget m_wRoot;
	protected SCR_MapEntity m_MapEntity;
	protected SCR_ChatPanel m_ChatPanel;
	
	void UpdateTimer()
	{	
		TextWidget timerWidget = TextWidget.Cast(GetRootWidget().FindAnyWidget("timerCountDown"));
		timerWidget.SetText(SCR_FormatHelper.FormatTime(CRF_Gamemode.GetInstance().m_iRespawnTimer));
	}
	
	override void OnMenuOpen()
	{
		GetGame().GetCallqueue().CallLater(UpdateTimer, 500, true);

		//Loads the map
		if (m_MapEntity)
		{	
			GetGame().GetCallqueue().CallLater(OpenMap, 0); 
		}
		
		
		//INPUT MANAGERS
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		
		//Loads the chat
		Widget wChatPanel = GetRootWidget().FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		//MORE INPUT MANAGERS
		GetGame().GetInputManager().AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
	}
	
	override void OnMenuInit()
	{		
		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		
	}
	
	override void OnMenuUpdate(float tDelta)
	{
	    if (m_MapEntity)
	        GetGame().GetInputManager().ActivateContext("MapContext");
	}
	
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(UpdateTimer);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
		GetGame().GetInputManager().RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetGame().GetInputManager().RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		
		if (m_MapEntity)
			m_MapEntity.CloseMap();
	}
	
	void OpenMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWrap, 0); // Need two frames
	}
	
	void OpenMapWrap()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;
		
		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChange, 0);
	}
	
	void OpenMapWrapZoomChange()
	{
		// What i do with my life...
		GetGame().GetCallqueue().CallLater(OpenMapWrapZoomChangeWrap, 0); // Another two frames
	}
	
	void OpenMapWrapZoomChangeWrap()
	{
		m_MapEntity.ZoomOut();
	}
	
	void Action_VONon()
	{
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetTalking(true, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetTransmitRadio(GetVoNTransiver());
		von.SetCommMethod(ECommMethod.SQUAD_RADIO);
		von.SetCapture(true);
	}
	
	//From RL
	RadioTransceiver GetVoNTransiver()
	{
		IEntity entity = GetGame().GetPlayerController().GetControlledEntity();
		ref array<IEntity> items = {};
		SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent)).GetItems(items);
		IEntity radioEntity;
		foreach(IEntity item: items)
		{
			if(item.FindComponent(BaseRadioComponent))
				radioEntity = item;
		}
		BaseRadioComponent radio = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		radio.SetPower(true);
		RadioTransceiver transiver = RadioTransceiver.Cast(radio.GetTransceiver(0));
		transiver.SetFrequency(1);
		return transiver;
	}
	
	
	void LobbyVoNDisableDelayed()
	{
		SCR_VoNComponent von = SCR_VoNComponent.Cast(GetGame().GetPlayerController().GetControlledEntity().FindComponent(SCR_VoNComponent));
		von.SetCommMethod(ECommMethod.DIRECT);
		von.SetCapture(false);
	}
	
	//From reforger lobby <3
	void Action_VONOff()
	{
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).SetTalking(false, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().CallLater(LobbyVoNDisableDelayed, 400);
	}
	
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Frame delay
		GetGame().GetCallqueue().CallLater(OpenChatWrap, 5);
	}
	
	void OpenChatWrap()
	{
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager.GetInstance().OpenChatPanel(m_ChatPanel);
		}
	}
	
	//From reforger lobby <3
	void Action_Exit()
	{
		// For some strange reason players all the time accidentally exit game, maybe jus open pause menu
		//GameStateTransitions.RequestGameplayEndTransition();  
		//Close();
		GetGame().GetCallqueue().CallLater(OpenPauseMenuWrap, 0); //  Else menu auto close itself
	}
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}