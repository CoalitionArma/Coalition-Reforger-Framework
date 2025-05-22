class CRF_AdminMenuManagerClass : ScriptComponentClass {}

class CRF_TicketMessageData
{
	string sender;
	string msg;
	string timestamp;
}

class CRF_AdminActionLog
{
	string timestamp;
	string action; // Description of the action
}

class CRF_Ticket
{
	int ticketID; // ID of the player who requested help
	ref array<ref CRF_TicketMessageData> messages; // Array of messages from the player since they asked for help
	
	void AddMessage(string sender, string msg)
	{
		CRF_TicketMessageData message = new CRF_TicketMessageData;
		message.sender = sender;
		message.msg = msg;
		message.timestamp = CRF_AdminMenuManager.GetFormattedTimestamp();
		messages.Insert(message);
	}
}

class CRF_AdminMenuManager : ScriptComponent
{
	// Map of player tickets
	private ref map<int, ref CRF_Ticket> m_mTickets = new map<int, ref CRF_Ticket>();
	
	// Array of admin actions
	private ref array<ref CRF_AdminActionLog> m_mAdminActions = new array<ref CRF_AdminActionLog>();
	
	//------------------------------------------------------------------------------------------------
	/**
	* Returns the instance of CRF_AdminMenuManager from the current game mode
	* @return CRF_AdminMenuManager instance or null if not found
	*/
	static CRF_AdminMenuManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
		
		return CRF_AdminMenuManager.Cast(gameMode.FindComponent(CRF_AdminMenuManager));
	}
	
	/**
	* Creates a formatted timestamp string from current system time
	* @return Formatted timestamp in HH:MM:SS format
	*/
	static string GetFormattedTimestamp()
	{
		int hour = 0;
		int minute = 0;
		int second = 0;
		
		// Get local time
		System.GetHourMinuteSecond(hour, minute, second);
		
		// Format with leading zeros
		return string.Format("%02d:%02d:%02d", hour, minute, second);
	}
	
	/**
	* Logs an admin action to the array
	* @param data Description of the action
	*/
	void LogAdminAction(string data)
	{
		// Create new log
		CRF_AdminActionLog log = new CRF_AdminActionLog();
		log.action = data;
		log.timestamp = GetFormattedTimestamp();
		m_mAdminActions.Insert(log);
		
		// Refresh Lists if admin menu is open
		RefreshLists();
	}
	
	/**
	* Returns list of actions taken by admins this mission
	* @return Array of admin action logs
	*/
	array<ref CRF_AdminActionLog> GetAdminActionLogs()
	{
		return m_mAdminActions;
	}

	/**
	* Creates new ticket or adds new message to existing ticket
	* @param ticketID ID of the ticket (ID of the player that initially opened the ticket)
	* @param senderID ID of the player sending the message
	* @param data Text in the message
	*/
	void NewTicketMessage(int ticketID, int senderID, string data)
	{
		// Get name of player sending the admin message
		string sender = GetGame().GetPlayerManager().GetPlayerName(senderID);
		if (!sender)
			return;
		
		CRF_Ticket ticket;
		
		// Check if player already has ticket open
		if (!m_mTickets.Contains(ticketID))
		{
			// Creates new ticket
			ticket = new CRF_Ticket();
			ticket.ticketID = ticketID;
			ticket.messages = {};
			m_mTickets.Set(ticketID, ticket);
		}
		else 
		{
			// Grab the existing ticket
			m_mTickets.Find(ticketID, ticket);
		}
		
		// Add the new message to the ticket
		ticket.AddMessage(sender, data);
		
		// Refresh Lists if admin menu is open
		RefreshLists();
	}
	
	/**
	* Returns a list of current open tickets
	* @return Array of player IDs with open tickets
	*/
	array<int> GetOpenTickets()
	{
		array<int> playerIDs = {};
		
		// Gather list of playerIDs for tickets that are open
		foreach (int id, ref CRF_Ticket ticket : m_mTickets)
		{
			playerIDs.Insert(ticket.ticketID);
		}
		
		return playerIDs;
	}
	
	/**
	* Returns list of messages from a ticket
	* @param playerID ID of the player whose ticket messages to retrieve
	* @return Array of ticket messages or null if no ticket found
	*/
	array<ref CRF_TicketMessageData> GetTicketMessages(int playerID)
	{
		CRF_Ticket ticket;
		
		// Get the messages from the player's ID
		if (m_mTickets.Find(playerID, ticket))
			return ticket.messages;
		
		return null;
	}
	
	/**
	* Close a ticket
	* @param ticketID ID of the ticket to close
	*/
	void CloseTicket(int ticketID)
	{		
		// Remove the ticket from the map
		m_mTickets.Remove(ticketID);
		
		// Refresh Lists if admin menu is open
		RefreshLists();
	}
	
	/**
	* Refreshes the lists on new message and new log being added
	*/
	void RefreshLists()
	{		
		// Check if the top menu is the admin menu
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (!topMenu)
			return;
			
		if (!topMenu.IsInherited(CRF_AdminMenu))
			return;
			
		// Repopulate menu components
		CRF_AdminMenu adminMenu = CRF_AdminMenu.Cast(topMenu);
		if (adminMenu.GetCurrentOpenTab() == "Tickets")
		{
			adminMenu.PopulateTicketMessages();
			adminMenu.PopulateOpenTicketList();
		}
		
		adminMenu.PopulateAdminActionsList();
	}

	/**
	* Check if a ticket exists for the given ID
	* @param ticketID ID of the ticket to check
	* @return True if ticket exists, false otherwise
	*/
	bool TicketExists(int ticketID)
	{		
		return m_mTickets.Contains(ticketID);
	}
}
