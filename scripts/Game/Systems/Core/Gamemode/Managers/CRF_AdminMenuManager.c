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
		int Hour = 0;
		int Minute = 0;
		int Second = 0;
		
		// Get local time
		System.GetHourMinuteSecond(Hour, Minute, Second);
		
		// Add message to ticket array
		CRF_TicketMessageData message = new CRF_TicketMessageData;
		message.sender = sender;
		message.msg = msg;
		message.timestamp = string.Format("%1:%2:%3", Hour, Minute, Second);
		messages.Insert(message);
	}
}

class CRF_AdminMenuManager : ScriptComponent
{
	// Map of player tickets
	private ref map<int, ref CRF_Ticket> m_mTickets = new map<int, ref CRF_Ticket>;
	
	// Array of admin actions
	private ref array<ref CRF_AdminActionLog> m_mAdminActions = new array<ref CRF_AdminActionLog>;
	
	//------------------------------------------------------------------------------------------------
	static CRF_AdminMenuManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_AdminMenuManager.Cast(gameMode.FindComponent(CRF_AdminMenuManager));
		else
			return null;
	}
	
	/*
	* Logs a admin action to the array
	* @param Description of the action
	*/
	void LogAdminAction(string data)
	{
		int Hour = 0;
		int Minute = 0;
		int Second = 0;
		
		// Get local time
		System.GetHourMinuteSecond(Hour, Minute, Second);
		
		// Create new log
		CRF_AdminActionLog log = new CRF_AdminActionLog;
		log.action = data;
		log.timestamp = string.Format("%1:%2:%3", Hour, Minute, Second);
		m_mAdminActions.Insert(log);
		
		// Refresh Lists if admin menu is open
		RefreshLists()
	}
	
	/*
	* Returns list of actions taken by admins this mission
	*/
	array<ref CRF_AdminActionLog> GetAdminActionLogs()
	{
		return m_mAdminActions;
	}

	/*
	* Creates new ticket or adds new message to exsisting ticket
	* @param ID of the ticket (ID of the player that initially opened the ticket)
	* @param ID of the player sending the message
	* @param Text in the message
	*/
	void NewTicketMessage(int ticketID, int senderID, string data)
	{
		CRF_Ticket ticket;
		
		// Get name of player sending the admin message
		string sender = GetGame().GetPlayerManager().GetPlayerName(senderID);
		if (!sender)
			return;
		
		// false if player already has ticket open
		if (!m_mTickets.Contains(ticketID))
		{
			// Creates new ticket
			ticket = new CRF_Ticket;
			ticket.ticketID = ticketID;
			ticket.messages = {};
			m_mTickets.Set(ticketID, ticket);
		}
		else 
		{
			// Grab the exsisting ticket
			m_mTickets.Find(ticketID, ticket)
		}
		
		// Add the new message to the ticket
		ticket.AddMessage(sender, data);
		
		// Refresh Lists if admin menu is open
		RefreshLists()
	}
	
	/*
	* Returns a list of current open tickets
	*/
	array<int> GetOpenTickets()
	{
		array<int> playerIDs = {};
		
		// Gather list of playerIDs for tickets that are open
		foreach (int i, ref CRF_Ticket ticket : m_mTickets)
		{
			playerIDs.Insert(ticket.ticketID);
		}
		
		return playerIDs;
	}
	
	/*
	* Returns list of messages from a ticket
	* @param ID of the player sending the message
	*/
	array<ref CRF_TicketMessageData> GetTicketMessages(int playerID)
	{
		CRF_Ticket ticket;
		
		// Get the messages from the players ID
		if (m_mTickets.Find(playerID, ticket))
			return ticket.messages;
		
		return null;
	}
	
	/*
	* Close a ticket
	* @param ID of the ticket to close
	*/
	void CloseTicket(int ticketID)
	{		
		// Get ID of admin closing the ticket
		int adminID = GetGame().GetPlayerController().GetPlayerId();
		
		// Remove the ticket for the array
		m_mTickets.Remove(ticketID);
		
		// Refresh Lists if admin menu is open
		RefreshLists();
	}
	
	/*
	* Refreshes the lists on new message and new log being added
	*/
	void RefreshLists()
	{		
		// Check if the top menu is the admin menu
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu != null)
		{
			if (topMenu.IsInherited(CRF_AdminMenu))
			{
				// Repopulate Ticket List, Ticket Messages and AdminActions 
				CRF_AdminMenu adminMenu = CRF_AdminMenu.Cast(topMenu);
				if (adminMenu.GetCurrentOpenTab() == "Tickets")
				{
					adminMenu.PopulateTicketMessages();
					adminMenu.PopulateOpenTicketList();
				}
				adminMenu.PopulateAdminActionsList();
			}
		}
	}

	/*
	* Check if ticket is new
	*/
	bool isNewTicket(int ticketID)
	{		
		return m_mTickets.Contains(ticketID);
	}
}
