modded class CVON_VONGameModeComponent
{
	[RplProp()] ref array<int> m_aListeningPlayers = {};
	
	void TogglePlayerListening(int playerId, bool input)
	{
		if (!input)
		{
			if (m_aListeningPlayers.Contains(playerId))
				m_aListeningPlayers.RemoveItem(playerId);
		}
		else
			m_aListeningPlayers.Insert(playerId);
		Replication.BumpMe();
	}
}