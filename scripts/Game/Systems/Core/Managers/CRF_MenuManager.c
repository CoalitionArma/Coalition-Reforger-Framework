class CRF_MenuManagerClass : ScriptComponentClass {}

class CRF_MenuManager : ScriptComponent
{	
	[RplProp()]
	ref array<string> m_aVONChannels = {"Deafen|", "Global|"};

	[RplProp()]
	int m_iChannelChanges = 0;
	
	ref array<int> m_aPlayersTalking = {};
	
	// Constants for better readability
	private const string CHANNEL_SEPARATOR = "|";
	private const string PLAYER_SEPARATOR = ",";
	private const int DEFAULT_CHANNEL_COUNT = 2; // Deafen and Global
	
	//------------------------------------------------------------------------------------------------
	static CRF_MenuManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;
			
		return CRF_MenuManager.Cast(gameMode.FindComponent(CRF_MenuManager));
	}
	
	//------------------------------------------------------------------------------------------------
	void SetChannel(int index, string inputString, bool channelCreation)
	{
		if (index < 0 || index >= m_aVONChannels.Count())
			return;
			
		// Update the channel in the array
		m_aVONChannels.Set(index, inputString);
		
		// If this is not a channel creation operation, perform cleanup
		if (!channelCreation)
			CleanupEmptyChannels();
		
		// Increment change counter and notify replication
		m_iChannelChanges++;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	// Remove empty non-default channels
	private void CleanupEmptyChannels()
	{
		// Iterate in reverse to safely remove items
		for (int i = m_aVONChannels.Count() - 1; i >= DEFAULT_CHANNEL_COUNT; i--)
		{
			array<string> channelSplit = SplitChannel(m_aVONChannels[i]);
			
			// If channel has no players, remove it
			if (channelSplit.Count() == 1)
			{
				m_aVONChannels.RemoveOrdered(i);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlayerInAnyChannel(int playerId, out int channelId)
	{
		channelId = -1;
		string playerIdStr = playerId.ToString();
		
		for (int i = 0; i < m_aVONChannels.Count(); i++)
		{
			array<string> channelSplit = SplitChannel(m_aVONChannels[i]);
			
			// Skip channels with no player data
			if (channelSplit.Count() <= 1)
				continue;
			
			array<string> players = GetPlayersFromChannel(channelSplit);
			
			if (players.Contains(playerIdStr))
			{
				channelId = i;
				return true;
			}
		}
		
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void AddPlayerToChannel(int playerId, int channelIndex, bool channelCreation)
	{
		if (channelIndex < 0 || channelIndex >= m_aVONChannels.Count())
			return;
			
		// Check if player is already in a channel and remove them
		int currentChannelIndex;
		if (IsPlayerInAnyChannel(playerId, currentChannelIndex))
		{
			RemovePlayerFromAnyChannel(playerId, channelCreation);
		}
		
		// Split the channel string into parts
		array<string> channelSplit = SplitChannel(m_aVONChannels[channelIndex]);
		
		// Get the current players in the channel
		array<string> players = GetPlayersFromChannel(channelSplit);
		
		// Add the player to the channel
		players.Insert(playerId.ToString());
		
		// Update the channel string
		string updatedChannel = UpdateChannelWithPlayers(channelSplit, players);
		
		// Update the channel in the list
		SetChannel(channelIndex, updatedChannel, channelCreation);
	}

	//------------------------------------------------------------------------------------------------
	void RemovePlayerFromAnyChannel(int playerId, bool channelCreation)
	{
		// Find which channel the player is in
		int channelIndex;
		if (!IsPlayerInAnyChannel(playerId, channelIndex))
			return;
		
		// Get the channel parts
		array<string> channelSplit = SplitChannel(m_aVONChannels[channelIndex]);
		
		// Get the players in the channel
		array<string> players = GetPlayersFromChannel(channelSplit);
		
		// Remove the player from the list
		int playerIndex = players.Find(playerId.ToString());
		if (playerIndex >= 0)
		{
			players.RemoveOrdered(playerIndex);
		}
		
		// Update the channel string
		string updatedChannel = UpdateChannelWithPlayers(channelSplit, players);
		
		// Update the channel in the list
		SetChannel(channelIndex, updatedChannel, channelCreation);
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlayerInChannel(int playerId, int index)
	{
		if (index < 0 || index >= m_aVONChannels.Count())
			return false;
			
		// Split the channel string
		array<string> channelSplit = SplitChannel(m_aVONChannels[index]);
		
		// Check if the channel has player data
		if (channelSplit.Count() == 1)
			return false;
		
		// Get the players in the channel
		array<string> players = GetPlayersFromChannel(channelSplit);
		
		// Check if the player is in the channel
		return players.Contains(playerId.ToString());
	}

	//------------------------------------------------------------------------------------------------
	int CreateChannel(string name, int playerId)
	{
		string channelString = name + CHANNEL_SEPARATOR;
		int index = m_aVONChannels.Insert(channelString);
		AddPlayerToChannel(playerId, index, true);
		m_iChannelChanges++;
		Replication.BumpMe();
		return index;
	}

	//------------------------------------------------------------------------------------------------
	int GetChannel(int playerId)
	{
		int channelId;
		if (IsPlayerInAnyChannel(playerId, channelId))
			return channelId;
		
		// Default to channel 1 (Global) if player is not in any channel
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	void RequestToJoinChannel(int channel, int requestId)
	{
		if (channel < 0 || channel >= m_aVONChannels.Count())
			return;
			
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		
		foreach (int player : players)
		{
			if (IsPlayerInChannel(player, channel))
			{
				CRF_RplBroadcastManager.GetInstance().SendRequest(player, requestId, channel);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void Accept()
	{
		CRF_ListBoxElementComponent comp = GetComponentFromWidgetHierarchy();
		if (!comp)
			return;
			
		CRF_RplToAuthorityManager.GetInstance().JoinChannel(comp.m_iPlayerId, comp.m_iChannelId);
	}

	//------------------------------------------------------------------------------------------------
	void Deny()
	{
		CRF_ListBoxElementComponent comp = GetComponentFromWidgetHierarchy();
		if (!comp)
			return;
		
		// Send deny notification to players in the channel
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		
		foreach (int player : players)
		{
			if (IsPlayerInChannel(player, comp.m_iChannelId))
			{
				CRF_RplBroadcastManager.GetInstance().Deny(player, comp.m_iPlayerId);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Helper methods
	//------------------------------------------------------------------------------------------------
	
	private array<string> SplitChannel(string channel)
	{
		array<string> channelSplit = {};
		channel.Split(CHANNEL_SEPARATOR, channelSplit, true);
		return channelSplit;
	}
	
	private array<string> GetPlayersFromChannel(array<string> channelSplit)
	{
		array<string> players = {};
		
		if (channelSplit.Count() > 1)
		{
			channelSplit.Get(1).Split(PLAYER_SEPARATOR, players, true);
			
			// Remove empty entries
			for (int i = players.Count() - 1; i >= 0; i--)
			{
				if (players[i] == "")
					players.RemoveOrdered(i);
			}
		}
		
		return players;
	}
	
	private string UpdateChannelWithPlayers(array<string> channelSplit, array<string> players)
	{
		string playersStr = SCR_StringHelper.Join(PLAYER_SEPARATOR, players);
		
		if (channelSplit.Count() > 1)
		{
			channelSplit.Set(1, playersStr);
		}
		else
		{
			channelSplit.Insert(playersStr);
		}
		
		return SCR_StringHelper.Join(CHANNEL_SEPARATOR, channelSplit);
	}
	
	private CRF_ListBoxElementComponent GetComponentFromWidgetHierarchy()
	{
		// Get the widget under cursor
		Widget widget = WidgetManager.GetWidgetUnderCursor();
		if (!widget)
			return null;
		
		// Navigate up to the 5th parent
		Widget parent = widget;
		for (int i = 0; i < 5; i++)
		{
			parent = parent.GetParent();
			if (!parent)
				return null;
		}
		
		// Return the component
		return CRF_ListBoxElementComponent.Cast(parent.FindHandler(CRF_ListBoxElementComponent));
	}
}
