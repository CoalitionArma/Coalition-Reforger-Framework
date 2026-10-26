//------------------------------------------------------------------------------------------------
//! One emblem in the website's answer (GET /api/game/emblems?guids=...)
class CRF_EmblemEntry
{
	string guid;
	string data;
}

//------------------------------------------------------------------------------------------------
//! Emblems by player. A client asks for one when it needs it (the killed-by card, for whoever killed it); the
//! server looks the player's in-game identity up on the website - batching every connected player it hasn't
//! cached yet into the same request - caches the answer for CACHE_MS and sends it back to the asking client
//! only. Members make their emblems at coalitiongroup.net/emblem; only accounts linked to an identity have one.
class CRF_EmblemStore
{
	protected static const string API_BASE = "https://api.coalitiongroup.net/";
	protected static const string ENDPOINT = "api/game/emblems?guids=";
	protected static const int CACHE_MS = 600000;		// 10 minutes: edits on the website show up mid-session
	protected static const int MAX_GUIDS = 100;

	protected static ref CRF_EmblemStore s_Instance;

	// Server
	protected ref map<string, string> m_mByGuid = new map<string, string>();		// identity -> emblem ("" for none)
	protected ref map<string, int> m_mFetchedAt = new map<string, int>();
	protected ref array<int> m_aWaitingRequesters = {};
	protected ref array<int> m_aWaitingPlayers = {};
	protected ref array<string> m_aFetchingGuids = {};
	protected ref RestCallback m_Callback;
	protected bool m_bFetching;

	// Client
	protected ref map<int, string> m_mByPlayer = new map<int, string>();

	//------------------------------------------------------------------------------------------------
	static CRF_EmblemStore Get()
	{
		if (!s_Instance)
			s_Instance = new CRF_EmblemStore();

		return s_Instance;
	}

	// ── Client ───────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! The player's emblem arrives at COA_KilledByCard.CRF_OnEmblem, from the cache straight away or once the
	//! server answers
	void Request(int playerId)
	{
		if (playerId <= 0)
			return;

		string data;
		if (m_mByPlayer.Find(playerId, data))
		{
			COA_KilledByCard.CRF_OnEmblem(playerId, data);
			return;
		}

		// Hosting the game ourselves: no round trip
		if (Replication.IsServer())
		{
			ServerRequest(SCR_PlayerController.GetLocalPlayerId(), playerId);
			return;
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (playerController)
			playerController.CRF_RequestEmblem(playerId);
	}

	//------------------------------------------------------------------------------------------------
	void OnReceived(int playerId, string data)
	{
		m_mByPlayer.Set(playerId, data);
		COA_KilledByCard.CRF_OnEmblem(playerId, data);
	}

	// ── Server ───────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	void ServerRequest(int requesterId, int playerId)
	{
		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (guid.IsEmpty())
		{
			Send(requesterId, playerId, "");
			return;
		}

		if (IsFresh(guid))
		{
			Send(requesterId, playerId, m_mByGuid.Get(guid));
			return;
		}

		m_aWaitingRequesters.Insert(requesterId);
		m_aWaitingPlayers.Insert(playerId);
		Fetch();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsFresh(string guid)
	{
		int fetchedAt;
		return m_mByGuid.Contains(guid) && m_mFetchedAt.Find(guid, fetchedAt) && System.GetTickCount() - fetchedAt < CACHE_MS;
	}

	//------------------------------------------------------------------------------------------------
	//! One request for the waiting players and every other connected player not cached yet
	protected void Fetch()
	{
		if (m_bFetching)
			return;

		set<string> guids = new set<string>();
		foreach (int waitingPlayer : m_aWaitingPlayers)
		{
			string waitingGuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(waitingPlayer);
			if (!waitingGuid.IsEmpty())
				guids.Insert(waitingGuid);
		}

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (guids.Count() >= MAX_GUIDS)
				break;

			string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			if (!guid.IsEmpty() && !IsFresh(guid))
				guids.Insert(guid);
		}

		RestApi rest = GetGame().GetRestApi();
		RestContext context;
		if (rest)
			context = rest.GetContext(API_BASE);

		if (guids.IsEmpty() || !context)
		{
			Flush();
			return;
		}

		string query;
		m_aFetchingGuids.Clear();
		foreach (string fetchGuid : guids)
		{
			if (!query.IsEmpty())
				query += ",";
			query += fetchGuid;
			m_aFetchingGuids.Insert(fetchGuid);
		}

		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnFetched);
		m_Callback.SetOnError(OnFetchFailed);
		m_bFetching = true;
		context.GET(m_Callback, ENDPOINT + query);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFetched(RestCallback callback)
	{
		m_bFetching = false;

		// Everyone asked about has an answer now, "" for no emblem
		int now = System.GetTickCount();
		foreach (string guid : m_aFetchingGuids)
		{
			m_mByGuid.Set(guid, "");
			m_mFetchedAt.Set(guid, now);
		}

		SCR_JsonLoadContext context = new SCR_JsonLoadContext();
		array<ref CRF_EmblemEntry> entries = {};
		if (context.LoadFromString(callback.GetData()) && context.ReadValue("emblems", entries))
		{
			foreach (CRF_EmblemEntry entry : entries)
			{
				if (entry && !entry.guid.IsEmpty())
					m_mByGuid.Set(entry.guid, entry.data);
			}
		}

		Flush();
	}

	//------------------------------------------------------------------------------------------------
	//! The website didn't answer: nobody gets an emblem this time, and nothing is cached
	protected void OnFetchFailed(RestCallback callback)
	{
		m_bFetching = false;
		Print("[CRF_EmblemStore] Couldn't fetch emblems from the website", LogLevel.WARNING);
		Flush();
	}

	//------------------------------------------------------------------------------------------------
	protected void Flush()
	{
		array<int> requesters = {};
		array<int> players = {};
		requesters.Copy(m_aWaitingRequesters);
		players.Copy(m_aWaitingPlayers);
		m_aWaitingRequesters.Clear();
		m_aWaitingPlayers.Clear();

		foreach (int i, int requesterId : requesters)
		{
			string data;
			m_mByGuid.Find(SCR_PlayerIdentityUtils.GetPlayerIdentityId(players[i]), data);
			Send(requesterId, players[i], data);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Send(int requesterId, int playerId, string data)
	{
		// Our own request, on a server we're playing on
		if (RplSession.Mode() != RplMode.Dedicated && requesterId == SCR_PlayerController.GetLocalPlayerId())
		{
			OnReceived(playerId, data);
			return;
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		if (playerController)
			playerController.CRF_SendEmblem(playerId, data);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	//! Ask the server for a player's emblem (CRF_EmblemStore.Request)
	void CRF_RequestEmblem(int playerId)
	{
		Rpc(RpcAsk_CRF_RequestEmblem, playerId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CRF_RequestEmblem(int playerId)
	{
		CRF_EmblemStore.Get().ServerRequest(GetPlayerId(), playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a player's emblem to this controller's owner
	void CRF_SendEmblem(int playerId, string data)
	{
		Rpc(RpcDo_CRF_Emblem, playerId, data);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CRF_Emblem(int playerId, string data)
	{
		CRF_EmblemStore.Get().OnReceived(playerId, data);
	}
}
