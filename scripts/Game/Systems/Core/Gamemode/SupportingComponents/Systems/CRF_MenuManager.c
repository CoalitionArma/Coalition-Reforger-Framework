class CRF_MenuManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_MenuManager : SCR_BaseGameModeComponent
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
		m_aVONChannels.Set(index, inputString);
		if (!channelCreation)
		{
			foreach (string channel : m_aVONChannels)
			{
				array<string> channelSplit = {};
				channel.Split("|", channelSplit, true);
				if (channelSplit.Count() == 1 && m_aVONChannels.Find(channel) > 1)
					m_aVONChannels.RemoveOrdered(m_aVONChannels.Find(channel));
			}
		}
		m_iChannelChanges++;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlayerInAnyChannel(int playerId, out int channelId)
	{
		bool isInChannel = false;
		channelId = -1;
		foreach (string channel : m_aVONChannels)
		{
			if (isInChannel)
				break;
			array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			array<string> players = {};
			if (channelSplit.Count() == 1)
				continue;
			channelSplit.Get(1).Split(",", players, true);
			foreach (string player : players)
			{
				if	(player == "")
					continue;
				if (player.ToInt() == playerId)
				{
					channelId = m_aVONChannels.Find(channel);
					isInChannel = true;
					break;
				}
			}
		}
		return isInChannel;
	}

	//------------------------------------------------------------------------------------------------
	void AddPlayerToChannel(int playerId, int channelIndex, bool channelCreation)
	{
		int index;
		if (IsPlayerInAnyChannel(playerId, index))
			RemovePlayerFromAnyChannel(playerId, channelCreation);
		array<string> channelSplit = {};
		m_aVONChannels.Get(channelIndex).Split("|", channelSplit, true);
		array<string> players = {};
		if (channelSplit.Count() > 1)
			channelSplit.Get(1).Split(",", players, true);
		players.Insert(playerId.ToString());
		if (channelSplit.Count() > 1)
			channelSplit.Set(1, SCR_StringHelper.Join(",", players));
		else
			channelSplit.Insert(SCR_StringHelper.Join(",", players));
		SetChannel(channelIndex, SCR_StringHelper.Join("|", channelSplit), channelCreation);
	}

	//------------------------------------------------------------------------------------------------
	void RemovePlayerFromAnyChannel(int playerId, bool channelCreation)
	{
		int index;
		if (!IsPlayerInAnyChannel(playerId, index))
			return;
		array<string> channelSplit = {};
		m_aVONChannels.Get(index).Split("|", channelSplit, true);
		array<string> players = {};
		if (channelSplit.Count() > 1)
			channelSplit.Get(1).Split(",", players, true);
		players.RemoveOrdered(players.Find(playerId.ToString()));
		if (channelSplit.Count() > 1)
			channelSplit.Set(1, SCR_StringHelper.Join(",", players));
		else
			channelSplit.Insert(SCR_StringHelper.Join(",", players));
		SetChannel(index, SCR_StringHelper.Join("|", channelSplit), channelCreation);
	}

	//------------------------------------------------------------------------------------------------
	bool IsPlayerInChannel(int playerId, int index)
	{
		array<string> channelSplit = {};
		m_aVONChannels.Get(index).Split("|", channelSplit, true);
		array<string> players = {};
		if (channelSplit.Count() == 1)
			return false;
		else
			channelSplit.Get(1).Split(",", players, true);
		if (players.Contains(playerId.ToString()))
			return true;
		else
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
		foreach (string channel : m_aVONChannels)
		{
			array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			array<string> players = {};
			if (channelSplit.Count() == 1)
				continue;
			else
				channelSplit.Get(1).Split(",", players, true);
			if (players.Contains(playerId.ToString()))
				return m_aVONChannels.Find(channel);
			else
				continue;
		}
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
		if (!WidgetManager.GetWidgetUnderCursor())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent())
			return;

		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent().FindHandler(CRF_ListBoxElementComponent));
		CRF_RplToAuthorityManager.GetInstance().JoinChannel(comp.m_iplayerId, comp.m_iChannelId);
	}

	//------------------------------------------------------------------------------------------------
	void Deny()
	{
		if (!WidgetManager.GetWidgetUnderCursor())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent())
			return;
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent().FindHandler(CRF_ListBoxElementComponent));

		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach (int player : players)
		{
			if (IsPlayerInChannel(player, comp.m_iChannelId))
				CRF_RplBroadcastManager.GetInstance().Deny(player, comp.m_iplayerId);
		}
	}
}