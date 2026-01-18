modded class SCR_ChatPanel : SCR_ScriptedWidgetComponent
{
	//--------------------------------------------------------------------------
	// Sets whether the chat panel should always be visible regardless of activity
	//--------------------------------------------------------------------------
	void SetAlwaysVisible(bool state)
	{
		m_bAlwaysVisible = state;
	}
	
	//--------------------------------------------------------------------------
	// Forces the chat panel to display full message history
	// Shows as many messages as possible up to the maximum line count
	//--------------------------------------------------------------------------
	void ForceShowFullHistory()
	{
		// Get all available chat messages from the manager
		array<ref SCR_ChatMessage> messages = SCR_ChatPanelManager.GetInstance().GetMessages();
		
		// If there are no messages, nothing to display
		int messageCount = messages.Count();
		if (messageCount == 0)
		{
			return;
		}
		
		// Set the message line count to either the total message count
		// or the maximum number of available lines, whichever is smaller
		m_iMessageLineCount = Math.Min(messageCount, m_aMessageLines.Count());
		
		// Set the history index to the most recent message
		m_iHistoryId = messageCount - 1;
		
		// Refresh the displayed messages
		UpdateChatMessages();
	}
	
	//--------------------------------------------------------------------------
	// Expands the number of message lines in the chat panel to the desired count
	// Creates new line widgets as needed
	//--------------------------------------------------------------------------
	void ExpandMessageLines(int desiredLineCount)
	{
		// Validate that required widgets exist
		if (!m_Widgets)
		{
			return;
		}
		
		if (!m_Widgets.m_MessageHistory)
		{
			return;
		}
		
		// Create additional message line widgets until reaching the desired count
		while (m_aMessageLines.Count() < desiredLineCount)
		{
			// Create a new line widget using the predefined layout
			Widget lineWidget = GetGame().GetWorkspace().CreateWidgets(m_sChatMessageLineLayout, m_Widgets.m_MessageHistory);
			
			// Find and cast the handler to access the line component
			SCR_ChatMessageLineComponent comp = SCR_ChatMessageLineComponent.Cast(lineWidget.FindHandler(SCR_ChatMessageLineComponent));
			
			// If component was found successfully, initialize it and add to the lines array
			if (comp)
			{
				comp.SetEmptyMessage();
				m_aMessageLines.Insert(comp);
			}
		}
		
		// Update the message line count to the new desired count
		m_iMessageLineCount = desiredLineCount;
	}
	
	//--------------------------------------------------------------------------
	// Disable chat messages except for tickets, rcon commands and messages
	// from admins/mods
	//--------------------------------------------------------------------------
	override protected void SendMessage()
	{
		SCR_ChatPanelManager mgr = SCR_ChatPanelManager.GetInstance();

		SCR_ChatComponent chatComponent = GetChatComponent();
		
		int senderID = SCR_PlayerController.GetLocalPlayerId();

		if (!chatComponent || !m_ActiveChannel)
			return;

		string message;
		if (m_Widgets.m_MessageEditBox)
			message = m_Widgets.m_MessageEditBox.GetText();
		else
			return;

		// Check if we want to send some command
		string cmd = this.GetCommand(message);

		// If there's a command, we don't want to really send the message
		// Note: by now the channel tag or player name have been already removed from the message,
		// so there is no command here any more
		if (!cmd.IsEmpty())
		{
			// Notify the chat panel mgr, pass the message with removed command
			message = message.Substring(cmd.Length() + 1, message.Length() - cmd.Length() - 1);
			message.TrimInPlace();

			mgr.Internal_OnChatCommand(this, cmd, message);

			return;
		}

		// Check if chat is disabled, a rcon command or a message from staff
		if (!CRF_Gamemode.GetInstance().m_bDisableChat || message.StartsWith("#") || SCR_Global.IsAdmin(senderID) || CRF_GamemodeManager.GetInstance().IsModerator(senderID))
		{
			if (!m_ActiveChannel.IsAvailable(chatComponent))
			{
				SCR_ChatMessageStyle style = this.GetChannelStyle(m_ActiveChannel);
				SCR_ChatPanelManager.GetInstance().ShowHelpMessage(STR_CHANNEL_DISABLED);
			}
			else
			{
				if (PrivateMessageChannel.Cast(m_ActiveChannel))
				{
					// Get whisper receiver ID
					int playerId = this.GetPlayerIdByName(cmd);
	
					if (playerId != -1)
					{
						// Remove player name from the message
						message = message.Substring(1, message.Length() - 1);
	
						chatComponent.SendPrivateMessage(message, playerId);
					}
				}
				else
				{
					int channelId = GetChannelId(m_ActiveChannel);
					chatComponent.SendMessage(message, channelId);
				}
			}
		}
		else
		{
			chatComponent.ShowMessage("Chat disabled. Use /a if you need help.");
		}
	}
}