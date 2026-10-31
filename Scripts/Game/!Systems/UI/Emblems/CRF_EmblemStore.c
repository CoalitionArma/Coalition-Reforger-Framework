//------------------------------------------------------------------------------------------------
//! One emblem in the website's answer (GET /api/game/emblems?guids=...)
class CRF_EmblemEntry
{
	string guid;
	string data;
}

//------------------------------------------------------------------------------------------------
//! Emblems by player. The server looks connected players' in-game identities up on the website as they join
//! (batched, see Prefetch) and caches the answers for CACHE_MS. Every client keeps a copy: a joining player asks for
//! the server's whole cache once it owns its controller, and from then on gets the emblems that are new or changed -
//! so the killed-by card only ever reads the local cache. Members make their emblems at coalitiongroup.net/emblem; only accounts linked to an
//! identity have one.
class CRF_EmblemStore
{
	// Its own base URL: RestApi.GetContext shares one context per base URL, and CRF_CommunityTagManager's
	// "https://api.coalitiongroup.net/" context has its own headers and queued requests (see CRF_AARManager)
	protected static const string API_BASE = "https://api.coalitiongroup.net/api/game/emblems";
	protected static const string ENDPOINT = "?guids=";
	protected static const int CACHE_MS = 600000;		// 10 minutes: edits on the website show up on the next join after that
	protected static const int MAX_GUIDS = 100;
	protected static const int REQUEST_TIMEOUT_S = 15;
	protected static const int FETCH_TIMEOUT_MS = 20000;	// a request that never calls back mustn't block emblems for the session
	protected static const int PREFETCH_DELAY_MS = 3000;	// joins within this long share one request
	protected static const int SEND_CHUNK = 10;			// emblems per RPC - a full one is ~700 characters

	protected static ref CRF_EmblemStore s_Instance;

	// Server
	protected ref map<string, string> m_mByGuid = new map<string, string>();		// identity -> emblem ("" for none)
	protected ref map<string, int> m_mFetchedAt = new map<string, int>();
	protected ref array<string> m_aFetchingGuids = {};
	protected ref RestCallback m_Callback;
	protected bool m_bFetching;
	protected bool m_bFetchAgain;		// asked for while a request was out
	protected bool m_bPrefetchScheduled;

	// Client: playerId -> emblem ("" for none)
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
	//! \return false when the server hasn't sent anything for the player (yet) - COA_KilledByCard.CRF_OnEmblem
	//! is called when it does
	bool Find(int playerId, out string data)
	{
		return m_mByPlayer.Find(playerId, data);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] reset the first part of the server's whole cache: what's here is from before we joined
	void OnReceived(array<int> playerIds, array<string> emblems, bool reset)
	{
		if (reset)
			m_mByPlayer.Clear();

		foreach (int i, int playerId : playerIds)
		{
			m_mByPlayer.Set(playerId, emblems[i]);
			COA_KilledByCard.CRF_OnEmblem(playerId, emblems[i]);
		}

		Print(string.Format("[CRF_EmblemStore] Received %1 emblem(s), %2 player(s) cached", playerIds.Count(), m_mByPlayer.Count()), LogLevel.NORMAL);
	}

	// ── Server ───────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! A player passed authentication (CRF_COA_Gamemode.OnPlayerAuditSuccess): look everyone who joined in the next
	//! PREFETCH_DELAY_MS up in one request, then send the clients what's new
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
	protected bool IsFresh(string guid)
	{
		int fetchedAt;
		return m_mByGuid.Contains(guid) && m_mFetchedAt.Find(guid, fetchedAt) && System.GetTickCount() - fetchedAt < CACHE_MS;
	}

	//------------------------------------------------------------------------------------------------
	//! One request for every connected player not cached (or due a refresh)
	protected void Fetch()
	{
		if (m_bFetching)
		{
			m_bFetchAgain = true;
			return;
		}

		m_bFetchAgain = false;

		set<string> guids = new set<string>();
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

		// Everyone's cached already (a quick reconnect): the newcomer still needs the cache
		if (guids.IsEmpty())
		{
			SyncClients(null);
			return;
		}

		RestApi rest = GetGame().GetRestApi();
		RestContext context;
		if (rest)
			context = rest.GetContext(API_BASE);

		if (!context)
		{
			Print("[CRF_EmblemStore] No REST context for " + API_BASE, LogLevel.WARNING);
			SyncClients(null);
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
			SyncClients(null);
			return;
		}

		// Everyone asked about has an answer now, "" for no emblem
		map<string, string> answers = new map<string, string>();
		foreach (string guid : m_aFetchingGuids)
		{
			answers.Set(guid, "");
		}

		int found;
		foreach (CRF_EmblemEntry entry : entries)
		{
			if (entry && answers.Contains(entry.guid))
			{
				answers.Set(entry.guid, entry.data);
				found++;
			}
		}

		// Clients already have the rest
		set<string> changed = new set<string>();
		int now = System.GetTickCount();
		foreach (string answerGuid, string data : answers)
		{
			string previous;
			if (!m_mByGuid.Find(answerGuid, previous) || previous != data)
				changed.Insert(answerGuid);

			m_mByGuid.Set(answerGuid, data);
			m_mFetchedAt.Set(answerGuid, now);
		}

		Print(string.Format("[CRF_EmblemStore] The website has emblems for %1 of %2 identities", found, m_aFetchingGuids.Count()), LogLevel.NORMAL);
		SyncClients(changed);
	}

	//------------------------------------------------------------------------------------------------
	//! The website didn't answer: nothing new, but whoever just joined still gets what's cached. The next join tries again.
	protected void OnFetchFailed(RestCallback callback)
	{
		if (callback != m_Callback)
			return;

		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		m_bFetching = false;
		Print(string.Format("[CRF_EmblemStore] Couldn't fetch emblems from the website (REST result %1, HTTP %2)", callback.GetRestResult(), callback.GetHttpCode()), LogLevel.WARNING);
		SyncClients(null);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFetchTimeout()
	{
		if (!m_bFetching)
			return;

		// Still m_Callback until the next fetch, so a late answer still fills the cache and reaches the clients
		m_bFetching = false;
		Print(string.Format("[CRF_EmblemStore] The website didn't answer within %1 ms", FETCH_TIMEOUT_MS), LogLevel.WARNING);
		SyncClients(null);
	}

	//------------------------------------------------------------------------------------------------
	//! The emblems of changedGuids to everyone who has had the whole cache (SendAll)
	protected void SyncClients(set<string> changedGuids)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		// "" too: the others may have had an emblem for them
		array<int> changedIds = {};
		array<string> changedEmblems = {};
		foreach (int playerId : playerIds)
		{
			string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			string data;
			if (changedGuids && changedGuids.Contains(guid) && m_mByGuid.Find(guid, data))
			{
				changedIds.Insert(playerId);
				changedEmblems.Insert(data);
			}
		}

		foreach (int recipientId : playerIds)
		{
			SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(recipientId));
			if (!playerController)
				continue;

			// Our own player, on a server we're playing on, never asks (see SCR_PlayerController.OnOwnershipChanged)
			if (!playerController.CRF_HasEmblems() && IsLocal(playerController))
				SendAll(playerController);
			else if (playerController.CRF_HasEmblems() && !changedIds.IsEmpty())
				Send(playerController, changedIds, changedEmblems, false);
		}

		if (m_bFetchAgain)
			Fetch();
	}

	//------------------------------------------------------------------------------------------------
	//! A client owns its controller now and asks for the whole cache (RpcAsk_CRF_RequestEmblems); from here on it also
	//! gets the changes. Players the website hasn't answered for yet come with those.
	void SendAll(notnull SCR_PlayerController playerController)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		array<int> ids = {};
		array<string> emblems = {};
		foreach (int playerId : playerIds)
		{
			string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
			string data;
			if (!guid.IsEmpty() && m_mByGuid.Find(guid, data) && !data.IsEmpty())
			{
				ids.Insert(playerId);
				emblems.Insert(data);
			}
		}

		Send(playerController, ids, emblems, true);
		playerController.CRF_SetHasEmblems();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsLocal(notnull SCR_PlayerController playerController)
	{
		return RplSession.Mode() != RplMode.Dedicated && playerController.GetPlayerId() == SCR_PlayerController.GetLocalPlayerId();
	}

	//------------------------------------------------------------------------------------------------
	//! In SEND_CHUNK parts; with reset, the first part (sent even when empty) clears the client's cache
	protected void Send(notnull SCR_PlayerController playerController, array<int> playerIds, array<string> emblems, bool reset)
	{
		bool isLocal = IsLocal(playerController);
		int start;
		while (start < playerIds.Count() || (reset && start == 0))
		{
			array<int> partIds = {};
			array<string> partEmblems = {};
			for (int i = start; i < playerIds.Count() && i < start + SEND_CHUNK; i++)
			{
				partIds.Insert(playerIds[i]);
				partEmblems.Insert(emblems[i]);
			}

			bool partReset = reset && start == 0;
			if (isLocal)
				OnReceived(partIds, partEmblems, partReset);
			else
				playerController.CRF_SendEmblems(partIds, partEmblems, partReset);

			start += SEND_CHUNK;
		}
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	protected bool m_bCRF_HasEmblems;		// server: this controller's owner has had the whole emblem cache

	//------------------------------------------------------------------------------------------------
	//! Ownership comes in a handshake with the server, and an owner RPC sent before it's done goes nowhere: so the
	//! client asks for the emblems once it's the owner, rather than the server guessing when that is
	protected override void OnOwnershipChanged(bool changing, bool becameOwner)
	{
		super.OnOwnershipChanged(changing, becameOwner);

		if (!changing && becameOwner && RplSession.Mode() == RplMode.Client)
			Rpc(RpcAsk_CRF_RequestEmblems);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_CRF_RequestEmblems()
	{
		CRF_EmblemStore.Get().SendAll(this);
	}

	//------------------------------------------------------------------------------------------------
	bool CRF_HasEmblems()
	{
		return m_bCRF_HasEmblems;
	}

	//------------------------------------------------------------------------------------------------
	void CRF_SetHasEmblems()
	{
		m_bCRF_HasEmblems = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: players' emblems to this controller's owner (CRF_EmblemStore.Send)
	void CRF_SendEmblems(array<int> playerIds, array<string> emblems, bool reset)
	{
		Rpc(RpcDo_CRF_Emblems, playerIds, emblems, reset);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CRF_Emblems(array<int> playerIds, array<string> emblems, bool reset)
	{
		CRF_EmblemStore.Get().OnReceived(playerIds, emblems, reset);
	}
}
