modded class CVON_VONGameModeComponent
{
	// REPLICATION FIX: Removed [RplProp()] from array
	// Arrays with RplProp send ENTIRE array on every change = massive bandwidth waste
	// Solution: Use RPCs to send only the delta (changed player state)
	// Old: Every toggle sent all player IDs to all clients
	// New: Each toggle sends only 8 bytes (playerId + bool) to all clients
	protected ref set<int> m_sListeningPlayers = new set<int>;
	
	// Server-side: Toggle listening state and broadcast change
	void TogglePlayerListening(int playerId, bool input)
	{
		// Only server should modify state
		if (!Replication.IsServer())
			return;
		
		if (!input)
			m_sListeningPlayers.Remove(playerId);
		else
			m_sListeningPlayers.Insert(playerId);
		
		// Send only this change to all clients (not entire array)
		Rpc(RpcDo_PlayerListeningChanged, playerId, input);
	}
	
	// RPC: Notify all clients of single player's listening state change
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayerListeningChanged(int playerId, bool isListening)
	{
		// Ensure set is initialized before accessing
		if (!m_sListeningPlayers)
			m_sListeningPlayers = new set<int>;
		
		// Update local state on all clients
		if (isListening)
			m_sListeningPlayers.Insert(playerId);
		else
			m_sListeningPlayers.Remove(playerId);
	}
	
	// Helper: Check if player is listening
	bool IsPlayerListening(int playerId)
	{
		// Safety check
		if (!m_sListeningPlayers)
			return false;
			
		return m_sListeningPlayers.Contains(playerId);
	}
	
	// Helper: Get all listening players (if needed)
	set<int> GetListeningPlayers()
	{
		// Safety check
		if (!m_sListeningPlayers)
			m_sListeningPlayers = new set<int>;
			
		return m_sListeningPlayers;
	}
}