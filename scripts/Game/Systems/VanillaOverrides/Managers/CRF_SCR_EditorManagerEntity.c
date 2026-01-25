modded class SCR_EditorManagerEntity
{
	//----------------------------------------------------------------
	// Determines if the editor can be opened based on current mode and permissions
	//----------------------------------------------------------------
	override bool CanOpen()
	{
		if (!CRF_Gamemode.GetInstance())
		{
			return super.CanOpen();;
		}

		// If in building mode, limit editor capabilities and allow opening
		if (GetCurrentMode() == EEditorMode.BUILDING)
		{
			SetIsLimited(true);
			return true;
		}
		
		// If in photo mode and player is spectator or moderator, allow full editor capabilities
		if ((GetCurrentMode() == EEditorMode.PHOTO) && (CRF_GamemodeManager.IsSpectator() || CRF_GamemodeManager.GetInstance().IsModerator()))
		{
			SetIsLimited(false);
			return true;
		}
		
		// If in admin mode and player is admin or moderator, allow full editor capabilities
		if ((GetCurrentMode() == EEditorMode.ADMIN) && (SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()) || CRF_GamemodeManager.GetInstance().IsModerator()))
		{
			SetIsLimited(false);
			return true;
		}
		
		// If not admin or moderator, limit editor capabilities
		if (!SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()) && !CRF_GamemodeManager.GetInstance().IsModerator())
		{
			SetIsLimited(true);
		}
		
		// No modes available
		if (m_Modes.IsEmpty())
			return false;
		
		// Determine if editor can be opened based on permissions
		if (IsLimited())
			return m_CanOpen == m_CanOpenSum;
		else
			return m_CanOpen | EEditorCanOpen.ALIVE == m_CanOpenSum;
	}
	
	//----------------------------------------------------------------
	// Sets whether the editor functionality should be limited
	// @param input - true to limit functionality, false for full access
	//----------------------------------------------------------------
	void SetIsLimited(bool input)
	{
		m_bIsLimited = input;
	}

	//----------------------------------------------------------------
	// Controls server-side toggling of the editor
	//----------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override protected void ToggleServer(bool open)
	{
		// Use default behavior if no CRF_Gamemode is active
		if (!CRF_Gamemode.GetInstance())
		{
			super.ToggleServer(open);
			return;
		}

		// Return if no state change or if in client mode
		if (m_bIsOpened == open || RplSession.Mode() == RplMode.Client) 
			return;
			
		// Check if editor can be closed
		if (!open && !CanClose())
		{
			SCR_NotificationsComponent.SendToPlayer(GetPlayerID(), ENotification.EDITOR_CANNOT_CLOSE);
			return;
		}
		
		// Set editor state
		m_bIsOpened = open;
		Rpc(ToggleOwner, m_bIsOpened);
		
		// Handle editor being opened
		if (m_bIsOpened)
		{
			if (m_CurrentModeEntity)
				m_CurrentModeEntity.ActivateModeServer();
			
			Event_OnOpenedServer.Invoke();
		}
		// Handle editor being closed
		else
		{
			if (m_CurrentModeEntity)
				m_CurrentModeEntity.DeactivateModeServer();
			
			Event_OnClosedServer.Invoke();
		}
	}
	
	//----------------------------------------------------------------
	// Handles events related to editor state changes
	//----------------------------------------------------------------
	override void StartEvents(EEditorEventOperation type = EEditorEventOperation.NONE)
	{
		// Call base implementation
		super.StartEvents(type);
		
		// Skip custom behavior if no CRF_Gamemode is active
		if (!CRF_Gamemode.GetInstance())
			return;
		
		// Handle editor opening - close all CRF menus
		if (type == EEditorEventOperation.OPEN)
		{	
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PreviewMenu);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SlottingMenu);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_SpectatorMenu);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_AARMenu);
		}
		// Handle editor closing - reopen appropriate UI
		else if (type == EEditorEventOperation.CLOSE)
		{	
			if (CRF_Gamemode.GetInstance())
			{
				// Schedule UI reopening after a short delay
				GetGame().GetCallqueue().Call(OpenUI);
			}
		}
	}
	
	//----------------------------------------------------------------
	// Opens the appropriate UI menu based on the current gamemode state
	//----------------------------------------------------------------
	void OpenUI()
	{	
		// Get the active gamemode instance
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		
		// Verify gamemode exists and check player permissions
		if (gamemode)
		{
			// Check if local entity is a playable character
			if (SCR_PlayerController.GetLocalControlledEntity().FindComponent(CRF_PlayableCharacter))
			{
				CRF_PlayableCharacter playableChar = CRF_PlayableCharacter.Cast(
					SCR_PlayerController.GetLocalControlledEntity().FindComponent(CRF_PlayableCharacter));
					
				// Return if player is not spectating
				if (!CRF_GamemodeManager.IsSpectator())
					return;
			}
			// Return if not a playable character and not spectating
			else if (!CRF_GamemodeManager.IsSpectator())
				return;
		}

		// Ensure local controlled entity exists
		if (SCR_PlayerController.GetLocalControlledEntity() == null)
			return;
			
		// Open appropriate menu based on gamemode state
		switch (CRF_Gamemode.GetInstance().m_GamemodeState)
		{
			case CRF_EGamemodeState.BRIEFING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);
				break;
			}
			case CRF_EGamemodeState.SLOTTING: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);
				break;
			}
			case CRF_EGamemodeState.GAME: 
			{
				// Verify entities exist
				if (!SCR_PlayerController.GetLocalMainEntity() || !SCR_PlayerController.GetLocalControlledEntity())
					return;
				
				// Initialize spectator camera if player is spectating
				bool isSpectator = CRF_GamemodeManager.IsSpectator(SCR_PlayerController.GetLocalMainEntity());
				bool isSameEntity = SCR_PlayerController.GetLocalControlledEntity() == SCR_PlayerController.GetLocalMainEntity();
				
				if (isSpectator && isSameEntity)
					GetGame().GetCallqueue().CallLater(CRF_RplToAuthorityManager.GetInstance().RequestInitilizePlayer, 500, false, SCR_PlayerController.GetLocalPlayerId());
				
				break;
			}
			case CRF_EGamemodeState.AAR: 
			{
				GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_AARMenu);
				break;
			}
		}
	}
}