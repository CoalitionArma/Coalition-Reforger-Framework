class CRF_MenuManagerClass : ScriptComponentClass {}

class CRF_MenuManager : ScriptComponent
{	
	[RplProp()]
	ref array<string> m_aVONChannels = {"Deafen|", "Global|"};

	[RplProp()]
	ref array<int> m_aPlayersRegistedVON = {};

	[RplProp()]
	int m_iChannelChanges = 0;
	
	ref array<int> m_aPlayersTalking = {};
	
	//------------------------------------------------------------------------------------------------
	static CRF_MenuManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_MenuManager.Cast(gameMode.FindComponent(CRF_MenuManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetChannel(int index, string inputString, bool channelCreation)
	{
		// Update the channel in the array
		m_aVONChannels.Set(index, inputString);
		
		// If this is not a channel creation operation, perform cleanup
		if (!channelCreation)
		{
			// Check all channels for any that need to be removed
			for (int i = 0; i < m_aVONChannels.Count(); i++)
			{
				string channel = m_aVONChannels[i];
				array<string> channelSplit = {};
				
				// Split the channel string to separate name from player IDs
				channel.Split("|", channelSplit, true);
				
				// If channel has no players and is not a default channel (index > 1)
				if (channelSplit.Count() == 1 && i > 1)
				{
					m_aVONChannels.RemoveOrdered(i);
					i--; // Adjust index since we removed an element
				}
			}
		}
		
		// Increment change counter for tracking modifications
		m_iChannelChanges++;
		
		// Notify replication system of the change
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlayerInAnyChannel(int playerId, out int channelId)
	{
		channelId = -1;
		
		for (int i = 0; i < m_aVONChannels.Count(); i++)
		{
			string channel = m_aVONChannels[i];
			array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			
			// Skip channels with no player data
			if (channelSplit.Count() <= 1)
				continue;
			
			array<string> players = {};
			channelSplit.Get(1).Split(",", players, true);
			
			for (int j = 0; j < players.Count(); j++)
			{
				string player = players[j];
				
				if (player == "")
					continue;
					
				if (player.ToInt() == playerId)
				{
					channelId = i;
					return true;
				}
			}
		}
		
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void AddPlayerToChannel(int playerId, int channelIndex, bool channelCreation)
	{
		// Check if player is already in a channel and remove them if necessary
		int currentChannelIndex;
		if (IsPlayerInAnyChannel(playerId, currentChannelIndex))
		{
			RemovePlayerFromAnyChannel(playerId, channelCreation);
		}
		
		// Split the channel string into parts
		array<string> channelSplit = {};
		m_aVONChannels.Get(channelIndex).Split("|", channelSplit, true);
		
		// Get the current players in the channel
		array<string> players = {};
		if (channelSplit.Count() > 1)
		{
			channelSplit.Get(1).Split(",", players, true);
		}
		
		// Add the player to the channel
		players.Insert(playerId.ToString());
		
		// Update the channel string
		if (channelSplit.Count() > 1)
		{
			channelSplit.Set(1, SCR_StringHelper.Join(",", players));
		}
		else
		{
			channelSplit.Insert(SCR_StringHelper.Join(",", players));
		}
		
		// Update the channel in the list
		SetChannel(channelIndex, SCR_StringHelper.Join("|", channelSplit), channelCreation);
	}

	//------------------------------------------------------------------------------------------------
	void RemovePlayerFromAnyChannel(int playerId, bool channelCreation)
	{
		// Find which channel the player is in
		int channelIndex;
		if (!IsPlayerInAnyChannel(playerId, channelIndex))
			return;
		
		// Get the channel string and split it
		array<string> channelSplit = {};
		m_aVONChannels.Get(channelIndex).Split("|", channelSplit, true);
		
		// Get the players in the channel
		array<string> players = {};
		if (channelSplit.Count() > 1)
		{
			channelSplit.Get(1).Split(",", players, true);
		}
		
		// Remove the player from the list
		int playerIndex = players.Find(playerId.ToString());
		if (playerIndex >= 0)
		{
			players.RemoveOrdered(playerIndex);
		}
		
		// Update the channel string
		if (channelSplit.Count() > 1)
		{
			channelSplit.Set(1, SCR_StringHelper.Join(",", players));
		}
		else
		{
			channelSplit.Insert(SCR_StringHelper.Join(",", players));
		}
		
		// Update the channel in the list
		SetChannel(channelIndex, SCR_StringHelper.Join("|", channelSplit), channelCreation);
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlayerInChannel(int playerId, int index)
	{
		// Split the channel string
		array<string> channelSplit = {};
		m_aVONChannels.Get(index).Split("|", channelSplit, true);
		
		// Check if the channel has player data
		if (channelSplit.Count() == 1)
		{
			return false;
		}
		
		// Get the players in the channel
		array<string> players = {};
		channelSplit.Get(1).Split(",", players, true);
		
		// Check if the player is in the channel
		if (players.Contains(playerId.ToString()))
		{
			return true;
		}
		
		return false;
	}

	//------------------------------------------------------------------------------------------------
	int CreateChannel(string name, int playerId)
	{
		int index = m_aVONChannels.Insert(name + "|");
		AddPlayerToChannel(playerId, index, true);
		m_iChannelChanges++;
		Replication.BumpMe();
		return index;
	}

	//------------------------------------------------------------------------------------------------
	int GetChannel(int playerId)
	{
		// Convert playerId to string once to avoid repeated conversions
		string playerIdStr = playerId.ToString();
		
		// Loop through all channels to find which one contains the player
		for (int i = 0; i < m_aVONChannels.Count(); i++)
		{
			string channel = m_aVONChannels[i];
			array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			
			// Skip channels with no player data
			if (channelSplit.Count() == 1)
				continue;
			
			// Get players in this channel
			array<string> players = {};
			channelSplit.Get(1).Split(",", players, true);
			
			// If player is in this channel, return the channel index
			if (players.Contains(playerIdStr))
				return m_aVONChannels.Find(channel);
		}
		
		// Default to channel 1 (Global) if player is not in any channel
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	void RequestToJoinChannel(int channel, int requestId)
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach (int player : players)
		{
			if (IsPlayerInChannel(player, channel))
				CRF_RplBroadcastManager.GetInstance().SendRequest(player, requestId, channel);
		}
	}

	//------------------------------------------------------------------------------------------------
	void Accept()
	{
		// Get the widget under cursor
		Widget widget = WidgetManager.GetWidgetUnderCursor();
		if (!widget)
			return;
		
		// Check parent hierarchy
		Widget parent1 = widget.GetParent();
		if (!parent1)
			return;
		
		Widget parent2 = parent1.GetParent();
		if (!parent2)
			return;
		
		Widget parent3 = parent2.GetParent();
		if (!parent3)
			return;
		
		Widget parent4 = parent3.GetParent();
		if (!parent4)
			return;
		
		Widget parent5 = parent4.GetParent();
		if (!parent5)
			return;
		
		// Find component and process join channel
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(parent5.FindHandler(CRF_ListBoxElementComponent));
		CRF_RplToAuthorityManager.GetInstance().JoinChannel(comp.m_iPlayerId, comp.m_iChannelId);
	}

	//------------------------------------------------------------------------------------------------
	void Deny()
	{
		// Get the widget under cursor
		Widget widget = WidgetManager.GetWidgetUnderCursor();
		if (!widget)
			return;
		
		// Check parent hierarchy
		Widget parent1 = widget.GetParent();
		if (!parent1)
			return;
		
		Widget parent2 = parent1.GetParent();
		if (!parent2)
			return;
		
		Widget parent3 = parent2.GetParent();
		if (!parent3)
			return;
		
		Widget parent4 = parent3.GetParent();
		if (!parent4)
			return;
		
		Widget parent5 = parent4.GetParent();
		if (!parent5)
			return;
		
		// Find component and process deny
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(parent5.FindHandler(CRF_ListBoxElementComponent));
		if (!comp)
			return;
		
		// Send deny notification to players in the channel
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach (int player : players)
		{
			if (IsPlayerInChannel(player, comp.m_iChannelId))
				CRF_RplBroadcastManager.GetInstance().Deny(player, comp.m_iPlayerId);
		}
	}
}