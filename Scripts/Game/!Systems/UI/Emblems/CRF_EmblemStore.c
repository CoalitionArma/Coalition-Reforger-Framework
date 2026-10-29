//------------------------------------------------------------------------------------------------
//! One emblem in the website's answer (GET /api/game/emblems?guids=...)
class CRF_EmblemEntry
{
	string guid;
	string data;
}

//------------------------------------------------------------------------------------------------
//! Emblems by player. The server looks connected players' in-game identities up on the website as they join
//! (batched, see Prefetch) and caches the answers for CACHE_MS, so a client asking for one when it needs it (the
//! killed-by card, for whoever killed it) usually gets it straight from the cache. Answers go back to the asking
//! client only. Members make their emblems at coalitiongroup.net/emblem; only accounts linked to an identity have one.
class CRF_EmblemStore
{
	// Its own base URL: RestApi.GetContext shares one context per base URL, and CRF_CommunityTagManager's
	// "https://api.coalitiongroup.net/" context has its own headers and queued requests (see CRF_AARManager)
	protected static const string API_BASE = "https://api.coalitiongroup.net/api/game/emblems";
	protected static const string ENDPOINT = "?guids=";
	protected static const int CACHE_MS = 600000;		// 10 minutes: edits on the website show up mid-session
	protected static const int MAX_GUIDS = 100;
	protected static const int REQUEST_TIMEOUT_S = 15;
	protected static const int FETCH_TIMEOUT_MS = 20000;	// a request that never calls back mustn't block emblems for the session
	protected static const int PREFETCH_DELAY_MS = 3000;	// joins within this long share one request

	protected static ref CRF_EmblemStore s_Instance;

	// Server
	protected ref map<string, string> m_mByGuid = new map<string, string>();		// identity -> emblem ("" for none)
	protected ref map<string, int> m_mFetchedAt = new map<string, int>();
	protected ref array<int> m_aWaitingRequesters = {};
	protected ref array<int> m_aWaitingPlayers = {};
	protected ref array<string> m_aFetchingGuids = {};
	protected ref RestCallback m_Callback;
	protected bool m_bFetching;
	protected bool m_bFetchAgain;		// asked for while a request was out
	protected bool m_bPrefetchScheduled;

	// Client: only answers the website gave, so a failed lookup is asked again next time
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
	//! \param[in] known false when the website couldn't be asked - shown as no emblem, but not cached
	void OnReceived(int playerId, string data, bool known)
	{
		string state = "emblem received";
		if (data.IsEmpty())
			state = "no emblem";
		if (!known)
			state = "website unavailable";

		Print(string.Format("[CRF_EmblemStore] Player %1: %2", playerId, state), LogLevel.NORMAL);
		if (known)
			m_mByPlayer.Set(playerId, data);

		COA_KilledByCard.CRF_OnEmblem(playerId, data);
	}

	// ── Server ───────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! A player passed authentication (CRF_COA_Gamemode.OnPlayerAuditSuccess): look everyone who joined in the next
	//! PREFETCH_DELAY_MS up in one request, before anyone needs their emblem
	void Prefetch()
	{
		if (m_bPrefetchScheduled)
			return;

		m_bPrefetchScheduled = true;
		GetGame().GetCallqueue().CallLater(OnPrefetchDue, PREFETCH_DELAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPrefetchDue()
	{
		m_bPrefetchScheduled = false;
		Fetch();
	}

	//------------------------------------------------------------------------------------------------
	void ServerRequest(int requesterId, int playerId)
	{
		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		if (guid.IsEmpty())
		{
			Send(requesterId, playerId, "", true);
			return;
		}

		// Cached, even if it's due a refresh: answer now and refresh it in the background
		if (m_mByGuid.Contains(guid))
		{
			Send(requesterId, playerId, m_mByGuid.Get(guid), true);
			if (!IsFresh(guid))
				Fetch();

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
		{
			m_bFetchAgain = true;
			return;
		}

		m_bFetchAgain = false;

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

		if (guids.IsEmpty())
		{
			Flush(true);
			return;
		}

		RestApi rest = GetGame().GetRestApi();
		RestContext context;
		if (rest)
			context = rest.GetContext(API_BASE);

		if (!context)
		{
			Print("[CRF_EmblemStore] No REST context for " + API_BASE, LogLevel.WARNING);
			Flush(false);
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
		context.SetTimeout(REQUEST_TIMEOUT_S);
		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		GetGame().GetCallqueue().CallLater(OnFetchTimeout, FETCH_TIMEOUT_MS, false);
		Print(string.Format("[CRF_EmblemStore] Fetching emblems for %1 identities", guids.Count()), LogLevel.NORMAL);
		context.GET(m_Callback, ENDPOINT + query);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFetched(RestCallback callback)
	{
		// A late answer to a request that already timed out, after a newer one went out
		if (callback != m_Callback)
			return;

		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		m_bFetching = false;

		SCR_JsonLoadContext context = new SCR_JsonLoadContext();
		array<ref CRF_EmblemEntry> entries = {};
		if (!context.LoadFromString(callback.GetData()) || !context.ReadValue("emblems", entries))
		{
			Print("[CRF_EmblemStore] Couldn't read the website's answer: " + callback.GetData(), LogLevel.WARNING);
			Flush(false);
			return;
		}

		// Everyone asked about has an answer now, "" for no emblem
		int now = System.GetTickCount();
		foreach (string guid : m_aFetchingGuids)
		{
			m_mByGuid.Set(guid, "");
			m_mFetchedAt.Set(guid, now);
		}

		int found;
		foreach (CRF_EmblemEntry entry : entries)
		{
			if (entry && !entry.guid.IsEmpty())
			{
				m_mByGuid.Set(entry.guid, entry.data);
				found++;
			}
		}

		Print(string.Format("[CRF_EmblemStore] The website has emblems for %1 of %2 identities", found, m_aFetchingGuids.Count()), LogLevel.NORMAL);
		Flush(true);
	}

	//------------------------------------------------------------------------------------------------
	//! The website didn't answer: the waiting players get no emblem this time, and nothing is cached
	protected void OnFetchFailed(RestCallback callback)
	{
		if (callback != m_Callback)
			return;

		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		m_bFetching = false;
		Print(string.Format("[CRF_EmblemStore] Couldn't fetch emblems from the website (REST result %1, HTTP %2)", callback.GetRestResult(), callback.GetHttpCode()), LogLevel.WARNING);
		Flush(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFetchTimeout()
	{
		if (!m_bFetching)
			return;

		// Still m_Callback until the next fetch, so a late answer only fills the cache
		m_bFetching = false;
		Print(string.Format("[CRF_EmblemStore] The website didn't answer within %1 ms", FETCH_TIMEOUT_MS), LogLevel.WARNING);
		Flush(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Answer the waiting requests the cache now covers
	//! \param[in] fetched true after the website answered: requests that came in during that request (so weren't
	//! in it) keep waiting for another. false when it couldn't be asked: everyone waiting gets an uncached "none".
	protected void Flush(bool fetched)
	{
		array<int> requesters = {};
		array<int> players = {};
		requesters.Copy(m_aWaitingRequesters);
		players.Copy(m_aWaitingPlayers);
		m_aWaitingRequesters.Clear();
		m_aWaitingPlayers.Clear();

		foreach (int i, int requesterId : requesters)
		{
			string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(players[i]);
			if (guid.IsEmpty() || m_mByGuid.Contains(guid))
			{
				Send(requesterId, players[i], m_mByGuid.Get(guid), true);
			}
			else if (fetched)
			{
				m_aWaitingRequesters.Insert(requesterId);
				m_aWaitingPlayers.Insert(players[i]);
			}
			else
			{
				Send(requesterId, players[i], "", false);
			}
		}

		// Don't keep asking a website that isn't answering - the next request or join tries again
		if (fetched && (m_bFetchAgain || !m_aWaitingPlayers.IsEmpty()))
			Fetch();
	}

	//------------------------------------------------------------------------------------------------
	protected void Send(int requesterId, int playerId, string data, bool known)
	{
		// Our own request, on a server we're playing on
		if (RplSession.Mode() != RplMode.Dedicated && requesterId == SCR_PlayerController.GetLocalPlayerId())
		{
			OnReceived(playerId, data, known);
			return;
		}

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(requesterId));
		if (playerController)
			playerController.CRF_SendEmblem(playerId, data, known);
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
	void CRF_SendEmblem(int playerId, string data, bool known)
	{
		Rpc(RpcDo_CRF_Emblem, playerId, data, known);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CRF_Emblem(int playerId, string data, bool known)
	{
		CRF_EmblemStore.Get().OnReceived(playerId, data, known);
	}
}
