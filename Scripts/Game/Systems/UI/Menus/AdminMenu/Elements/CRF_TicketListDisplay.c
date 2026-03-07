/**
 * Ticket List Display Component
 * 
 * A UI component that displays open admin help tickets with message threads.
 * Shows tickets as "TicketID:PlayerName" format and can display message history.
 * Used in the admin ticket management system to track and respond to player requests.
 * 
 * Note: This is different from CRF_TicketDisplay which shows faction ticket counters.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the ticket list widget
 * // In your menu class, get a reference to the component:
 * CRF_TicketListDisplay m_TicketList;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("TicketListBox0"));
 * m_TicketList = CRF_TicketListDisplay.Cast(listRoot.FindHandler(CRF_TicketListDisplay));
 * 
 * // Populate with open tickets:
 * array<int> openTickets = {...};
 * m_TicketList.PopulateTickets(openTickets);
 * 
 * // Get selected ticket ID:
 * int ticketId = m_TicketList.GetSelectedTicketId();
 * 
 * // Populate messages for selected ticket:
 * array<ref CRF_TicketMessageData> messages = {...};
 * m_TicketList.PopulateMessages(messages);
 * \endcode
 */
class CRF_TicketListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying tickets
	protected SCR_ListBoxComponent m_TicketListBox;
	
	//! List box component for displaying messages
	protected SCR_ListBoxComponent m_MessageListBox;
	
	//! Player manager reference
	protected PlayerManager m_PlayerManager;
	
	//! Currently selected ticket ID
	protected int m_iSelectedTicketId = -1;
	
	//! Cached list of ticket IDs (synchronized with list order)
	protected ref array<int> m_aTicketIds = {};
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the ticket list box component
		m_TicketListBox = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_TicketListBox)
		{
			Print("CRF_TicketListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
		
		// Cache manager references
		m_PlayerManager = GetGame().GetPlayerManager();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the message list box component for displaying ticket messages
	 * \param messageListBox The list box component for messages
	 */
	void SetMessageListBox(SCR_ListBoxComponent messageListBox)
	{
		m_MessageListBox = messageListBox;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with open tickets
	 * Tickets are displayed as "TicketID:PlayerName" format
	 * \param ticketIds Array of player IDs with open tickets
	 */
	void PopulateTickets(array<int> ticketIds)
	{
		if (!m_TicketListBox || !m_PlayerManager)
			return;
		
		// Clear existing entries
		m_TicketListBox.Clear();
		m_aTicketIds.Clear();
		
		// Add each ticket to the list
		foreach (int playerId : ticketIds)
		{
			if (!m_PlayerManager.IsPlayerConnected(playerId))
				continue;
			
			string playerName = m_PlayerManager.GetPlayerName(playerId);
			string displayText = string.Format("%1:%2", playerId, playerName);
			
			m_TicketListBox.AddItem(displayText);
			m_aTicketIds.Insert(playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the message list with ticket message thread
	 * \param messages Array of ticket message data
	 */
	void PopulateMessages(array<ref CRF_TicketMessageData> messages)
	{
		if (!m_MessageListBox || !messages)
			return;
		
		// Clear old messages
		m_MessageListBox.Clear();
		
		// Format and add messages to the list
		foreach (int i, ref CRF_TicketMessageData message : messages)
		{
			string displayText = string.Format("%1 - %2: %3", message.timestamp, message.sender, message.msg);
			m_MessageListBox.AddItem(displayText);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the ticket ID from the currently selected list item
	 * \return Ticket ID (player ID), or -1 if nothing selected
	 */
	int GetSelectedTicketId()
	{
		if (!m_TicketListBox)
			return -1;
		
		int selectedIndex = m_TicketListBox.GetSelectedItem();
		if (selectedIndex < 0)
		{
			// Return stored ticket if nothing currently selected
			return m_iSelectedTicketId;
		}
		
		// Get ticket ID from selected item text
		Widget itemWidget = m_TicketListBox.GetElementComponent(selectedIndex).GetRootWidget();
		TextWidget textWidget = TextWidget.Cast(itemWidget.FindAnyWidget("Text"));
		if (!textWidget)
			return -1;
		
		string ticketText = textWidget.GetText();
		
		// Extract ID from "ID:Name" format
		array<string> parts = {};
		ticketText.Split(":", parts, true);
		
		if (parts.Count() == 0)
			return -1;
		
		int ticketId = parts[0].ToInt();
		m_iSelectedTicketId = ticketId;
		
		return ticketId;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the player name from the currently selected ticket
	 * \return Player name, or empty string if nothing selected
	 */
	string GetSelectedPlayerName()
	{
		if (!m_TicketListBox)
			return string.Empty;
		
		int selectedIndex = m_TicketListBox.GetSelectedItem();
		if (selectedIndex < 0)
			return string.Empty;
		
		Widget itemWidget = m_TicketListBox.GetElementComponent(selectedIndex).GetRootWidget();
		TextWidget textWidget = TextWidget.Cast(itemWidget.FindAnyWidget("Text"));
		if (!textWidget)
			return string.Empty;
		
		string ticketText = textWidget.GetText();
		
		// Extract name from "ID:Name" format
		array<string> parts = {};
		ticketText.Split(":", parts, true);
		
		if (parts.Count() < 2)
			return string.Empty;
		
		return parts[1];
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the selected list index
	 * \return List index, or -1 if nothing selected
	 */
	int GetSelectedIndex()
	{
		if (!m_TicketListBox)
			return -1;
		
		return m_TicketListBox.GetSelectedItem();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the currently selected ticket ID for restoration after refresh
	 * \param ticketId The ticket ID to store
	 */
	void SetSelectedTicketId(int ticketId)
	{
		m_iSelectedTicketId = ticketId;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Checks if there are any open tickets
	 * \return True if tickets exist, false otherwise
	 */
	bool HasTickets()
	{
		return m_aTicketIds.Count() > 0;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the count of open tickets
	 * \return Number of open tickets
	 */
	int GetTicketCount()
	{
		return m_aTicketIds.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears all tickets from the list
	 */
	void ClearTickets()
	{
		if (m_TicketListBox)
			m_TicketListBox.Clear();
		
		m_aTicketIds.Clear();
		m_iSelectedTicketId = -1;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears all messages from the message list
	 */
	void ClearMessages()
	{
		if (m_MessageListBox)
			m_MessageListBox.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears both ticket and message lists
	 */
	void Clear()
	{
		ClearTickets();
		ClearMessages();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the underlying ticket list box component
	 * \return The SCR_ListBoxComponent, or null if not initialized
	 */
	SCR_ListBoxComponent GetTicketListBox()
	{
		return m_TicketListBox;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the message list box component
	 * \return The SCR_ListBoxComponent, or null if not set
	 */
	SCR_ListBoxComponent GetMessageListBox()
	{
		return m_MessageListBox;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the cached array of ticket IDs
	 * \return Array of player IDs with open tickets
	 */
	array<int> GetTicketIds()
	{
		return m_aTicketIds;
	}
}
