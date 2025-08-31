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
		
		// Enable history mode
		m_bHistoryMode = true;
		
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
}