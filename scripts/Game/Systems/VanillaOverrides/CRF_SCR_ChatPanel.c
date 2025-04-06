modded class SCR_ChatPanel : SCR_ScriptedWidgetComponent
{
    void SetAlwaysVisible(bool state)
    {
        m_bAlwaysVisible = state;
    }
	
	void ForceShowFullHistory()
	{
	    array<ref SCR_ChatMessage> messages = SCR_ChatPanelManager.GetInstance().GetMessages();
	
	    int messageCount = messages.Count();
	    if (messageCount == 0) return;
	
	    m_iMessageLineCount = Math.Min(messageCount, m_aMessageLines.Count());
	    m_iHistoryId = messageCount - 1;
	    m_bHistoryMode = true;
	
	    UpdateChatMessages();
	}
	
	void ExpandMessageLines(int desiredLineCount)
	{
	    if (!m_Widgets || !m_Widgets.m_MessageHistory)
	    	return;
	
	    while (m_aMessageLines.Count() < desiredLineCount)
	    {
	        Widget lineWidget = GetGame().GetWorkspace().CreateWidgets(m_sChatMessageLineLayout, m_Widgets.m_MessageHistory);
	        SCR_ChatMessageLineComponent comp = SCR_ChatMessageLineComponent.Cast(lineWidget.FindHandler(SCR_ChatMessageLineComponent));
	
	        if (comp)
	        {
	            comp.SetEmptyMessage();
	            m_aMessageLines.Insert(comp);
	        }
	    }
	
	    m_iMessageLineCount = desiredLineCount;
	}
}