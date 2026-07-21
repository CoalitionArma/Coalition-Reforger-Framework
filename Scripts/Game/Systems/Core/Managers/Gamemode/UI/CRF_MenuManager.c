class CRF_MenuManagerClass : ScriptComponentClass {}

class CRF_MenuManager : ScriptComponent
{	
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 VARIABLE
//=============================================================================================================================================================================================================================================================================================================================================================
	
	[RplProp()]
	ref array<string> m_aVONChannels = {"Deafen|", "Global|"};

	[RplProp()]
	int m_iChannelChanges = 0;
	
	ref array<int> m_aPlayersTalking = {};
	
	// Constants for better readability
	private const string CHANNEL_SEPARATOR = "|";
	private const string PLAYER_SEPARATOR = ",";
	private const int DEFAULT_CHANNEL_COUNT = 2; // Deafen and Global
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CHANNEL MANAGEMENT
//=============================================================================================================================================================================================================================================================================================================================================================
	
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
	//! Moves a player into channelIndex, leaving whatever channel they were previously in (if any).
	//! Adds to the target channel FIRST, then removes from the old one, rather than the reverse -
	//! removing first can trigger CleanupEmptyChannels() (via SetChannel), which deletes array
	//! entries and shifts every later index down. If that happened before we'd read channelIndex,
	//! a shifted channelIndex could silently add the player to the wrong channel (or go out of
	//! bounds) whenever the channel being left sits earlier in the array than the target - e.g.
	//! switching directly from one private room to a later one. Doing the target write first means
	//! channelIndex is only ever used while still guaranteed valid by the bounds check above.
	void AddPlayerToChannel(int playerId, int channelIndex, bool channelCreation)
	{
		if (channelIndex < 0 || channelIndex >= m_aVONChannels.Count())
			return;

		// Capture the player's current channel (if any) before mutating anything. A Set() call
		// doesn't reorder the array, so this index stays valid through the "add" step below.
		int previousChannelIndex;
		bool wasInChannel = IsPlayerInAnyChannel(playerId, previousChannelIndex);

		// Add to the target channel first, while channelIndex is still guaranteed valid.
		array<string> channelSplit = SplitChannel(m_aVONChannels[channelIndex]);
		array<string> players = GetPlayersFromChannel(channelSplit);

		if (!players.Contains(playerId.ToString()))
			players.Insert(playerId.ToString());

		m_aVONChannels.Set(channelIndex, UpdateChannelWithPlayers(channelSplit, players));

		// Then remove them from wherever they were before (if different) - safe even if this
		// shrinks/reorders the array, since we're done touching channelIndex by this point.
		if (wasInChannel && previousChannelIndex != channelIndex)
		{
			array<string> oldSplit = SplitChannel(m_aVONChannels[previousChannelIndex]);
			array<string> oldPlayers = GetPlayersFromChannel(oldSplit);

			int oldPlayerIdx = oldPlayers.Find(playerId.ToString());
			if (oldPlayerIdx >= 0)
				oldPlayers.RemoveOrdered(oldPlayerIdx);

			m_aVONChannels.Set(previousChannelIndex, UpdateChannelWithPlayers(oldSplit, oldPlayers));
		}

		m_iChannelChanges++;
		Replication.BumpMe();

		if (!channelCreation)
			CleanupEmptyChannels();
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
	// Static variables for request throttling
	private static int s_lastRequestTime = 0;
	private static int s_lastRequestChannel = -1;
	private static int s_lastRequestId = -1;
	
	//------------------------------------------------------------------------------------------------
	void RequestToJoinChannel(int channel, int requestId)
	{
		// This method is now called on the server side
		// The logic has been moved to CRF_PlayerRplToAuthorityManager.RpcAsk_RequestToJoinChannel
		// to handle the request properly in the client-server architecture
		Print(string.Format("[VON] RequestToJoinChannel called on server: channel=%1, requestId=%2", channel, requestId), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	void Accept()
	{
		CRF_ListBoxElementComponent comp = GetComponentFromWidgetHierarchy();
		if (!comp)
			return;
			
		// Join the requester to the channel
		CRF_PlayerRplToAuthorityManager.GetInstance().JoinChannel(comp.m_iPlayerId, comp.m_iChannelId);
		
		// Send acceptance notification with sound to the requester
		CRF_RplBroadcastManager.GetInstance().NotifyRequestAccepted(comp.m_iPlayerId);
	}

	//------------------------------------------------------------------------------------------------
	void Deny()
	{
		CRF_ListBoxElementComponent comp = GetComponentFromWidgetHierarchy();
		if (!comp)
			return;
		
		// Send denial notification with sound to the requester
		CRF_RplBroadcastManager.GetInstance().NotifyRequestDenied(comp.m_iPlayerId);
		
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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 HELPER METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	private array<string> SplitChannel(string channel)
	{
		array<string> channelSplit = {};
		channel.Split(CHANNEL_SEPARATOR, channelSplit, true);
		return channelSplit;
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_MenuManager m_sInstance;
	void CRF_MenuManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_MenuManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_MenuManager GetInstance()
	{
		return m_sInstance;
	}
}
