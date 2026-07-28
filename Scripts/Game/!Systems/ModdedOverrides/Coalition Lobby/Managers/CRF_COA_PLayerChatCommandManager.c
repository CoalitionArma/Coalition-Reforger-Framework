modded class COA_PlayerChatCommandManager
{
	//------------------------------------------------------------------------------------------------
	//! Registers chat commands for admin messaging
	override void AddMsgAction()
	{
        super.AddMsgAction();

		SCR_ChatPanelManager chatPanelManager = SCR_ChatPanelManager.GetInstance();
		
		// CURRENTLY BROKEN IN 1.7 - NEEDS UPDATING CRF_PersistanceManager
		// ChatCommandInvoker invoker6 = chatPanelManager.GetCommandInvoker("save");
		// invoker6.Insert(SaveMission_Callback);		
		
		ChatCommandInvoker invoker7 = chatPanelManager.GetCommandInvoker("bug");
		invoker7.Insert(ReportBug);

		ChatCommandInvoker invoker9 = chatPanelManager.GetCommandInvoker("forwarddeploy");
		invoker9.Insert(ReopenForwardDeployMenu);

		ChatCommandInvoker invoker10 = chatPanelManager.GetCommandInvoker("fd");
		invoker10.Insert(ReopenForwardDeployMenu);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 FORWARD DEPLOY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Reopens the JIP forward deploy menu (e.g. if the player closed it earlier via the back action).
	//! Usage: /forwarddeploy or /fd
	void ReopenForwardDeployMenu(SCR_ChatPanel panel, string data)
	{
		SCR_ChatComponent chatComponent;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc)
			chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
		if (!gamemode || !safestartManager || gamemode.m_bLockUnusedSlots || safestartManager.GetSafestartStatus())
		{
			if (chatComponent)
				chatComponent.ShowMessage("Forward deploy is not available right now.");
			return;
		}

		IEntity controlled = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlled || COA_EntityHelper.IsSpectator(controlled))
		{
			if (chatComponent)
				chatComponent.ShowMessage("You need to be playing a character to forward deploy.");
			return;
		}

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_JIPForwardDeployMenu);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 BUG REPORT METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Reports bugs to github
	//! \param[in] panel - Chat panel
	//! \param[in] data - Message content
	void ReportBug(SCR_ChatPanel panel, string data)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(pc.FindComponent(SCR_ChatComponent));
		if (!chatComponent)
			return;
		
		int playerID = GetGame().GetPlayerController().GetPlayerId();
		
		if (!data.Length() > 0)
		{
			chatComponent.ShowMessage("You need to include your bug report after /bug");
			return;
		}	
		
		chatComponent.ShowMessage(string.Format("Bug Report Sent: \"%1\"", data));
		m_PlayerRplToAuthorityManager.ReportBug(data, playerID);
	}

	//------------------------------------------------------------------------------------------------
	//! Callback for advancing gamemode state with optional faction winner parameter
	//! Usage: /aar [faction]
	//! Examples: /aar, /aar blufor, /aar blu, /aar opfor, /aar opf, /aar indfor, /aar ind, /aar civ
	override void Advance_Callback(SCR_ChatPanel panel, string data)
	{
		// Check if admin privileges are required
		if (!SCR_Global.IsAdmin())
		{
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("You need admin privileges to use the /aar command.");
			}
			return;
		}
		
		// Parse faction parameter if provided
		if (data && data.Length() > 0)
		{
			// Clean up the input - remove extra spaces and convert to uppercase
			data.Trim();
			data.ToUpper();
			
			// Map short faction names to full names
			FactionKey winningFaction = "";
			switch (data)
			{
				case "BLUFOR":
				case "BLU":
				case "B":
				case "BLUE":
					winningFaction = "BLUFOR";
					break;
					
				case "OPFOR":
				case "OPF":
				case "O":
				case "RED":
					winningFaction = "OPFOR";
					break;
					
				case "INDFOR":
				case "IND":
				case "I":
				case "INDEPENDENT":
				case "GREEN":
					winningFaction = "INDFOR";
					break;
					
				case "CIV":
				case "C":
				case "CIVILIAN":
					winningFaction = "CIV";
					break;
					
				default:
					// Invalid faction specified
					if (panel)
					{
						SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
						if (chatComponent)
						{
							string validOptions = "Valid faction options: blufor (blu), opfor (opf), indfor (ind), civ";
							chatComponent.ShowMessage(string.Format("Invalid faction '%1'. %2", data, validOptions));
						}
					}
					return;
			}
			
			// Set the winning faction in the logging manager
			CRF_LoggingManager loggingManager = CRF_LoggingManager.GetInstance();
			if (loggingManager)
			{
				// Show confirmation message
				if (panel)
				{
					SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
					if (chatComponent)
						chatComponent.ShowMessage(string.Format("Winner set to %1. Advancing to AAR...", winningFaction));
				}
			}
			
			// Advance to AAR state — winning faction is passed to the server RPC so
			// SetWinningFaction is called on the authority where the log file handle exists.
			m_PlayerRplToAuthorityManager.RequestAdvanceGamemodeState(true, winningFaction);
		}
		else
		{
			// No faction specified - show current usage
			if (panel)
			{
				SCR_ChatComponent chatComponent = SCR_ChatComponent.Cast(GetGame().GetPlayerController().FindComponent(SCR_ChatComponent));
				if (chatComponent)
					chatComponent.ShowMessage("Usage: /aar [faction] - Examples: /aar blufor, /aar opfor, /aar indfor, /aar civ");
					
			}
			return;
		}
	}
}