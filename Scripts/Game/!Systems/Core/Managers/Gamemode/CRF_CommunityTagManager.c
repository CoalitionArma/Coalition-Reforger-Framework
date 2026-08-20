class CRF_CommunityTagManagerClass : ScriptComponentClass {}

//! Fetches and caches Coalition community tags for players from the website API.
//! Tags are displayed next to player names in slotting and briefing menus.
//! This component must be added to the COA_Gamemode entity in the Workbench editor.
//! The HTTP fetch only ever runs on the server; results are replicated to clients
//! via RpcDo_PlayerInfoUpdated so every machine ends up with the same cache.
class CRF_CommunityTagManager : ScriptComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	CONSTANTS
//=============================================================================================================================================================================================================================================================================================================================================================

	//! Base URL for the Coalition bot API — trailing slash required
	protected static const string BOT_API_BASE_URL = "https://api.coalitiongroup.net/";

	//! Combined tags + XP endpoint (replaces the two separate endpoints)
	protected static const string PLAYER_INFO_ENDPOINT = "api/game/player-info?names=";

	//! How long to wait after the last connect event before firing the batch fetch.
	//! Resets on each new join so a burst of connects produces exactly one request.
	protected static const int DEBOUNCE_DELAY_MS = 3000;
	protected static const int DEBOUNCE_MAX_WAIT_MS = 10000;
	protected static const int RECONCILE_DELAY_MS = 8000;

	//! Cap on those follow-ups, so players who genuinely have no backend record (and will never
	//! resolve) don't produce an endless fetch loop.
	protected static const int MAX_RECONCILE_PASSES = 3;

	//! If neither the success nor the error callback fires within this window, the
	//! in-flight request is considered lost and m_bFetching is force-reset so future
	//! fetches aren't permanently blocked.
	protected static const int FETCH_TIMEOUT_MS = 15000;

	//! How long a client must wait between two RpcAsk_RequestPlayerInfo calls. Menus call
	//! FetchPlayerInfo() on every open, so without this a player toggling the slotting menu
	//! would hammer the authority.
	protected static const int CLIENT_REQUEST_COOLDOWN_MS = 5000;

	//! Stop auto-retrying after this many consecutive lost/failed requests so an unreachable
	//! backend doesn't produce an endless 15s retry loop for the rest of the session.
	//! Reset to zero as soon as one request succeeds, or when a player connects.
	protected static const int MAX_CONSECUTIVE_FAILURES = 5;

	//! XP thresholds for rank tiers.
	//! All players start at 0 XP (rank 1 of any track).
	//! Enlisted E1–E9 (E4/E8/E9 have sub-variants a/b/c):
	protected static const int RANK_XP_E1  = 0;      // Private               (E-1, no insignia)
	protected static const int RANK_XP_E2  = 5000;   // Private Second Class  (E-2)
	protected static const int RANK_XP_E3  = 15000;  // Private First Class   (E-3)
	protected static const int RANK_XP_E4A = 30000;  // Specialist            (E-4a)
	protected static const int RANK_XP_E4B = 45000;  // Corporal              (E-4b)
	protected static const int RANK_XP_E5  = 65000;  // Sergeant              (E-5)
	protected static const int RANK_XP_E6  = 90000;  // Staff Sergeant        (E-6)
	protected static const int RANK_XP_E7  = 140000; // Sergeant First Class  (E-7)
	protected static const int RANK_XP_E8A = 215000; // Master Sergeant       (E-8a)
	protected static const int RANK_XP_E8B = 315000; // First Sergeant        (E-8b)
	protected static const int RANK_XP_E9A = 440000; // Sergeant Major        (E-9a)
	protected static const int RANK_XP_E9B = 590000; // Command Sergeant Major(E-9b)
	protected static const int RANK_XP_E9C = 790000; // Sergeant Major of the Army (E-9c)
	//! Warrant Officer W1–W5 (5 ranks evenly spaced to 800 000 XP):
	protected static const int RANK_XP_W1  = 0;      // Warrant Officer 1
	protected static const int RANK_XP_W2  = 190000; // Chief Warrant Officer 2
	protected static const int RANK_XP_W3  = 390000; // Chief Warrant Officer 3
	protected static const int RANK_XP_W4  = 590000; // Chief Warrant Officer 4
	protected static const int RANK_XP_W5  = 790000; // Chief Warrant Officer 5
	//! Commissioned Officer O1–O11:
	protected static const int RANK_XP_O1  = 0;      // Second Lieutenant   (O-1)
	protected static const int RANK_XP_O2  = 20000;  // First Lieutenant    (O-2)
	protected static const int RANK_XP_O3  = 50000;  // Captain             (O-3)
	protected static const int RANK_XP_O4  = 90000;  // Major               (O-4)
	protected static const int RANK_XP_O5  = 140000; // Lieutenant Colonel  (O-5)
	protected static const int RANK_XP_O6  = 215000; // Colonel             (O-6)
	protected static const int RANK_XP_O7  = 340000; // Brigadier General   (O-7)
	protected static const int RANK_XP_O8  = 490000; // Major General       (O-8)
	protected static const int RANK_XP_O9  = 640000; // Lieutenant General  (O-9)
	protected static const int RANK_XP_O10 = 790000; // General             (O-10)
	protected static const int RANK_XP_O11 = 990000; // General of the Army (O-11)

//=============================================================================================================================================================================================================================================================================================================================================================
//	RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	//! Singleton reference
	protected static CRF_CommunityTagManager m_sInstance;

	//! Cache: player ID -> community tag string (empty string means no tag)
	protected ref map<int, string> m_mTagCache = new map<int, string>;

	//! Prevents duplicate simultaneous in-flight HTTP requests
	protected bool m_bFetching = false;

	//! Set when FetchPlayerInfo() is called while a fetch is already in-flight.
	//! The next completed fetch will automatically trigger a follow-up fetch.
	protected bool m_bPendingFetch = false;

	//! REST callback stored as ref so it is not garbage-collected mid-flight
	protected ref RestCallback m_Callback;

	//! Cache: player ID -> XP value (-1 means not cached)
	protected ref map<int, int> m_mXpCache = new map<int, int>;

	//! Cache: player ID -> rank track ("enlisted" / "warrant" / "officer"), empty = default enlisted
	protected ref map<int, string> m_mTrackCache = new map<int, string>;

	//! Fired after both tags and XP are fetched and caches are populated
	protected ref ScriptInvoker m_OnPlayerInfoUpdated = new ScriptInvoker;

	//! Fired when connected player roster changes (join/leave).
	//! UI menus can rebuild list boxes immediately without waiting for screen reopen.
	protected ref ScriptInvoker m_OnPlayerRosterChanged = new ScriptInvoker;

	//! Guards against duplicate game-mode player event registration.
	protected bool m_bPlayerEventsSubscribed = false;

	//! Client-side: set while a request to the server is on cooldown.
	protected bool m_bClientRequestOnCooldown = false;

	//! Server-side: consecutive lost/failed fetches, see MAX_CONSECUTIVE_FAILURES.
	protected int m_iConsecutiveFailures = 0;

	//! Tick at which the currently open debounce window started, or -1 when none is open.
	//! Used to enforce DEBOUNCE_MAX_WAIT_MS. Deliberately System.GetTickCount() rather than world
	//! time: this is a purely server-local elapsed-time measurement that is never compared across
	//! machines, and it must not depend on a loaded world — the Workbench instantiates this
	//! component with no world present.
	protected int m_iDebounceWindowStart = -1;

	//! Follow-up passes used by the current reconcile cycle, see MAX_RECONCILE_PASSES.
	protected int m_iReconcilePasses = 0;

//=============================================================================================================================================================================================================================================================================================================================================================
//	STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void CRF_CommunityTagManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_CommunityTagManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	//! EOnInit is only dispatched to components that have asked for EntityEvent.INIT. Without this
	//! the whole component is inert: lifecycle callbacks never register, so the server never fetches
	//! and no client ever receives a tag or rank.
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// The World Editor instantiates the gamemode entity (and therefore this component) while
		// editing — switching worlds runs EOnInit with no world loaded and no players to fetch for.
		// Nothing this component does is meaningful outside play mode.
		if (!GetGame().InPlayMode())
			return;

		RegisterPlayerLifecycleCallbacks();
	}

	//------------------------------------------------------------------------------------------------
	static CRF_CommunityTagManager GetInstance()
	{
		return m_sInstance;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PUBLIC API
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Returns the invoker that fires when both tags and XP have been fetched.
	ScriptInvoker GetOnPlayerInfoUpdated()
	{
		return m_OnPlayerInfoUpdated;
	}

	//! Kept for backward compatibility — points at the combined invoker.
	ScriptInvoker GetOnTagsUpdated()
	{
		return m_OnPlayerInfoUpdated;
	}

	//! Kept for backward compatibility — points at the combined invoker.
	ScriptInvoker GetOnRanksUpdated()
	{
		return m_OnPlayerInfoUpdated;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the invoker that fires when players connect/disconnect.
	ScriptInvoker GetOnPlayerRosterChanged()
	{
		return m_OnPlayerRosterChanged;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the plain player name (no tag prefix). Convenience wrapper.
	string GetPlayerDisplayName(int playerId)
	{
		return GetGame().GetPlayerManager().GetPlayerName(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the community tag string for a player (e.g. "CRF"), or empty string if none cached.
	string GetPlayerTag(int playerId)
	{
		string tag;
		if (m_mTagCache.Find(playerId, tag))
			return tag;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the cached XP value for a player, or -1 if not cached / no data.
	int GetPlayerXp(int playerId)
	{
		int xp;
		if (m_mXpCache.Find(playerId, xp))
			return xp;

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the cached rank track for a player: "enlisted", "warrant", or "officer".
	//! Defaults to "enlisted" if the player has no stored preference.
	string GetPlayerRankTrack(int playerId)
	{
		string track;
		if (m_mTrackCache.Find(playerId, track) && !track.IsEmpty())
			return track;

		return "enlisted";
	}

	//------------------------------------------------------------------------------------------------
	//! Fetches community tags and XP for all connected players in a single HTTP request.
	//! Server-authoritative: the HTTP call only ever happens on the authority, so clients don't
	//! independently hammer the backend. A client calling this instead asks the server to re-send
	//! (see RpcAsk_RequestPlayerInfo), which is what makes opening a menu populate tags for a
	//! player who joined after the last broadcast.
	//! If a fetch is already in-flight, queues one follow-up fetch to run after completion.
	//! Subscribers to GetOnPlayerInfoUpdated() are notified when data arrives.
	void FetchPlayerInfo()
	{
		if (!Replication.IsServer())
		{
			RequestPlayerInfoFromServer();
			return;
		}

		// The debounce window (if any) has now closed.
		m_iDebounceWindowStart = -1;

		if (m_bFetching)
		{
			m_bPendingFetch = true;
			return;
		}

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		if (playerIds.IsEmpty())
			return;

		string queryNames = "";
		bool first = true;
		foreach (int playerId : playerIds)
		{
			if (playerId <= 0)
				continue;
			string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (name.IsEmpty())
				continue;
			if (!first)
				queryNames += ",";
			first = false;
			queryNames += EncodeNameForQuery(name);
		}

		if (queryNames.IsEmpty())
			return;

		RestApi rest = GetGame().GetRestApi();
		if (!rest)
		{
			Print("[CRF_CommunityTagManager] GetRestApi() returned null on server — cannot fetch player info", LogLevel.ERROR);
			return;
		}
		RestContext ctx = rest.GetContext(BOT_API_BASE_URL);
		if (!ctx)
		{
			Print("[CRF_CommunityTagManager] rest.GetContext() returned null for " + BOT_API_BASE_URL, LogLevel.ERROR);
			return;
		}

		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnPlayerInfoFetched);
		m_Callback.SetOnError(OnPlayerInfoFetchFailed);

		m_bFetching = true;
		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		GetGame().GetCallqueue().CallLater(OnFetchTimeout, FETCH_TIMEOUT_MS, false);
		ctx.SetHeaders("Content-Type,application/json");
		Print(string.Format("[CRF_CommunityTagManager] Issuing GET for %1 player(s): %2", playerIds.Count(), queryNames), LogLevel.NORMAL);
		ctx.GET(m_Callback, PLAYER_INFO_ENDPOINT + queryNames);
	}

	//! Debounced fetch: cancels any pending scheduled fetch and reschedules it
	//! DEBOUNCE_DELAY_MS from now. Call this on every connect event — a burst of
	//! N joins produces exactly one HTTP request once the burst settles.
	//! When the fetch completes, m_OnPlayerInfoUpdated fires so all UI subscribers
	//! automatically refresh their tag/rank icons.
	//! The deferral is capped at DEBOUNCE_MAX_WAIT_MS: a steady stream of joins would otherwise
	//! reset the timer indefinitely and the fetch would never run at all.
	protected void ScheduleFetchDebounced()
	{
		int now = System.GetTickCount();

		if (m_iDebounceWindowStart < 0)
			m_iDebounceWindowStart = now;

		// Cancel whatever was pending either way — we are about to replace it or run it now.
		GetGame().GetCallqueue().Remove(FetchPlayerInfo);

		if (now - m_iDebounceWindowStart >= DEBOUNCE_MAX_WAIT_MS)
		{
			// Held off long enough — fetch what we have now. Players who join after this point
			// still get picked up by the next debounce window and by the reconcile pass.
			FetchPlayerInfo();
			return;
		}

		GetGame().GetCallqueue().CallLater(FetchPlayerInfo, DEBOUNCE_DELAY_MS, false);
	}

	//! Backward-compatible wrappers — both now delegate to FetchPlayerInfo.
	void FetchTagsForCurrentPlayers()  { FetchPlayerInfo(); }
	void FetchRanksForCurrentPlayers() { FetchPlayerInfo(); }

	//------------------------------------------------------------------------------------------------
	//! Client side of FetchPlayerInfo(). Rate-limited because menus call it on every open.
	protected void RequestPlayerInfoFromServer()
	{
		if (m_bClientRequestOnCooldown)
			return;

		m_bClientRequestOnCooldown = true;
		GetGame().GetCallqueue().Remove(ClearClientRequestCooldown);
		GetGame().GetCallqueue().CallLater(ClearClientRequestCooldown, CLIENT_REQUEST_COOLDOWN_MS, false);

		Rpc(RpcAsk_RequestPlayerInfo);
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearClientRequestCooldown()
	{
		m_bClientRequestOnCooldown = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Clears the tag, XP, and rank track caches.
	void ClearCache()
	{
		m_mTagCache.Clear();
		m_mXpCache.Clear();
		m_mTrackCache.Clear();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — QUERY STRING ENCODING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Escapes characters in a player display name that would otherwise corrupt the comma-separated
	//! query string or the API's JSON response keys. '%' must be encoded first so it doesn't double-encode.
	protected string EncodeNameForQuery(string name)
	{
		string encoded = name;
		encoded.Replace("%", "%25");
		encoded.Replace(" ", "%20");
		encoded.Replace(",", "%2C");
		encoded.Replace("&", "%26");
		encoded.Replace("#", "%23");
		encoded.Replace("\"", "%22");
		return encoded;
	}

	//------------------------------------------------------------------------------------------------
	//! Reverses EncodeNameForQuery() on names echoed back in the API response. '%' must be decoded last.
	protected string DecodeNameFromResponse(string name)
	{
		string decoded = name;
		decoded.Replace("%20", " ");
		decoded.Replace("%2C", ",");
		decoded.Replace("%26", "&");
		decoded.Replace("%23", "#");
		decoded.Replace("%22", "\"");
		decoded.Replace("%25", "%");
		return decoded;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — REST CALLBACKS (SERVER ONLY)
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called when the combined player-info API responds successfully.
	//! Parses: {"success":true,"tags":{"Name":"CRF"|null},"xp":{"Name":15000|null}}
	//! Runs on the server only (FetchPlayerInfo() is server-gated). Resolves the by-name response
	//! against currently connected players and pushes the result to every client via RPC.
	protected void OnPlayerInfoFetched(RestCallback cb)
	{
		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		m_bFetching = false;
		m_iConsecutiveFailures = 0;
		bool bRetry = m_bPendingFetch;
		m_bPendingFetch = false;

		string data = cb.GetData();
		if (data.IsEmpty())
		{
			Print("[CRF_CommunityTagManager] Player info response body was empty", LogLevel.WARNING);
			if (bRetry)
				FetchPlayerInfo();
			return;
		}
		Print(string.Format("[CRF_CommunityTagManager] Received player info response (%1 bytes)", data.Length()), LogLevel.NORMAL);

		map<string, string> tagsByName = new map<string, string>;
		map<string, int> xpByName = new map<string, int>;
		map<string, string> trackByName = new map<string, string>;

		// --- Parse tags ---
		int tagsObjStart = data.IndexOf("\"tags\":{");
		if (tagsObjStart >= 0)
		{
			int pos = tagsObjStart + 8;
			while (pos < data.Length())
			{
				int keyOpen = data.IndexOfFrom(pos, "\"");
				if (keyOpen < 0)
					break;
				string between = data.Substring(pos, keyOpen - pos);
				if (between.Contains("}"))
					break;
				int keyClose = data.IndexOfFrom(keyOpen + 1, "\"");
				if (keyClose < 0)
					break;
				string playerName = data.Substring(keyOpen + 1, keyClose - keyOpen - 1);
				playerName = DecodeNameFromResponse(playerName);
				int colonPos = data.IndexOfFrom(keyClose + 1, ":");
				if (colonPos < 0)
					break;
				int valueStart = colonPos + 1;
				if (valueStart >= data.Length())
					break;
				string tag = "";
				if (data.ContainsAt("null", valueStart))
				{
					pos = valueStart + 4;
				}
				else if (data.ContainsAt("\"", valueStart))
				{
					int tagClose = data.IndexOfFrom(valueStart + 1, "\"");
					if (tagClose < 0)
						break;
					tag = data.Substring(valueStart + 1, tagClose - valueStart - 1);
					pos = tagClose + 1;
				}
				else
					break;
				if (!playerName.IsEmpty())
					tagsByName.Set(playerName, tag);
				if (pos < data.Length() && data.ContainsAt(",", pos))
					pos++;
			}
		}

		// --- Parse XP ---
		int xpObjStart = data.IndexOf("\"xp\":{");
		if (xpObjStart >= 0)
		{
			int pos = xpObjStart + 6;
			while (pos < data.Length())
			{
				int keyOpen = data.IndexOfFrom(pos, "\"");
				if (keyOpen < 0)
					break;
				string between = data.Substring(pos, keyOpen - pos);
				if (between.Contains("}"))
					break;
				int keyClose = data.IndexOfFrom(keyOpen + 1, "\"");
				if (keyClose < 0)
					break;
				string playerName = data.Substring(keyOpen + 1, keyClose - keyOpen - 1);
				playerName = DecodeNameFromResponse(playerName);
				int colonPos = data.IndexOfFrom(keyClose + 1, ":");
				if (colonPos < 0)
					break;
				int valueStart = colonPos + 1;
				if (valueStart >= data.Length())
					break;
				int xp = -1;
				if (data.ContainsAt("null", valueStart))
				{
					pos = valueStart + 4;
				}
				else
				{
					int numEnd = valueStart;
					while (numEnd < data.Length())
					{
						string ch = data.Substring(numEnd, 1);
						if (ch == "," || ch == "}")
							break;
						numEnd++;
					}
					xp = data.Substring(valueStart, numEnd - valueStart).ToInt();
					pos = numEnd;
				}
				if (!playerName.IsEmpty())
					xpByName.Set(playerName, xp);
				if (pos < data.Length() && data.ContainsAt(",", pos))
					pos++;
			}
		}

		// --- Parse rank tracks ---
		// Expected format: "rankTrack":{"PlayerName":"enlisted"|"warrant"|"officer"|null, ...}
		int trackObjStart = data.IndexOf("\"rankTrack\":{");
		if (trackObjStart >= 0)
		{
			int pos = trackObjStart + 13;
			while (pos < data.Length())
			{
				int keyOpen = data.IndexOfFrom(pos, "\"");
				if (keyOpen < 0)
					break;
				string between = data.Substring(pos, keyOpen - pos);
				if (between.Contains("}"))
					break;
				int keyClose = data.IndexOfFrom(keyOpen + 1, "\"");
				if (keyClose < 0)
					break;
				string playerName = data.Substring(keyOpen + 1, keyClose - keyOpen - 1);
				playerName = DecodeNameFromResponse(playerName);
				int colonPos = data.IndexOfFrom(keyClose + 1, ":");
				if (colonPos < 0)
					break;
				int valueStart = colonPos + 1;
				if (valueStart >= data.Length())
					break;
				string track = "enlisted";
				if (data.ContainsAt("null", valueStart))
				{
					pos = valueStart + 4;
				}
				else if (data.ContainsAt("\"", valueStart))
				{
					int trackClose = data.IndexOfFrom(valueStart + 1, "\"");
					if (trackClose < 0)
						break;
					track = data.Substring(valueStart + 1, trackClose - valueStart - 1);
					pos = trackClose + 1;
				}
				else
					break;
				if (!playerName.IsEmpty())
					trackByName.Set(playerName, track);
				if (pos < data.Length() && data.ContainsAt(",", pos))
					pos++;
			}
		}

		// Resolve by-name results against the currently connected roster (player IDs are stable,
		// names are not — this is the join point between the API's name-keyed response and our
		// ID-keyed cache) and build the replication payload in one pass.
		array<int> allPlayerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayerIds);

		array<int> outIds = {};
		array<string> outTags = {};
		array<int> outXp = {};
		array<string> outTracks = {};

		foreach (int playerId : allPlayerIds)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName.IsEmpty())
				continue;

			string tag = "";
			int xp = -1;
			string track = "enlisted";
			bool hasAny = false;

			if (tagsByName.Find(playerName, tag))
				hasAny = true;
			if (xpByName.Find(playerName, xp))
				hasAny = true;
			if (trackByName.Find(playerName, track) && !track.IsEmpty())
				hasAny = true;
			else
				track = "enlisted";

			if (!hasAny)
				continue;

			outIds.Insert(playerId);
			outTags.Insert(tag);
			outXp.Insert(xp);
			outTracks.Insert(track);
		}

		Print(string.Format("[CRF_CommunityTagManager] Resolved %1 of %2 connected player(s) to tag/xp/track data — broadcasting", outIds.Count(), allPlayerIds.Count()), LogLevel.NORMAL);

		// Apply locally (covers dedicated + listen server) and replicate to every client.
		RpcDo_PlayerInfoUpdated(outIds, outTags, outXp, outTracks);
		Rpc(RpcDo_PlayerInfoUpdated, outIds, outTags, outXp, outTracks);

		if (bRetry)
		{
			FetchPlayerInfo();
			return;
		}

		ScheduleReconcileIfIncomplete();
	}

	//------------------------------------------------------------------------------------------------
	//! Schedule another fetch if any connected player is still absent from the cache.
	//! Two things put a player in that state during a mass join, and neither of them recovers on
	//! its own: their name had not replicated when the query string was built (FetchPlayerInfo
	//! skips empty names), or the response simply had no row keyed to their name. Once the join
	//! burst is over there are no further connect events, so without this they stay untagged for
	//! the rest of the session.
	protected void ScheduleReconcileIfIncomplete()
	{
		if (!Replication.IsServer())
			return;

		if (m_iReconcilePasses >= MAX_RECONCILE_PASSES)
			return;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);

		int missing = 0;
		foreach (int playerId : playerIds)
		{
			if (playerId <= 0)
				continue;
			if (!m_mTagCache.Contains(playerId))
				missing++;
		}

		if (missing == 0)
		{
			m_iReconcilePasses = 0;
			return;
		}

		m_iReconcilePasses++;
		Print(string.Format("[CRF_CommunityTagManager] %1 connected player(s) still have no cached info — scheduling reconcile pass %2 of %3", missing, m_iReconcilePasses, MAX_RECONCILE_PASSES), LogLevel.NORMAL);
		GetGame().GetCallqueue().Remove(FetchPlayerInfo);
		GetGame().GetCallqueue().CallLater(FetchPlayerInfo, RECONCILE_DELAY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Re-sends whatever is already cached to every client, without touching the backend.
	//! Used to answer a client request instantly — a player who joins mid-session gets the
	//! existing roster's tags right away instead of waiting on the next HTTP round trip.
	protected void BroadcastCachedPlayerInfo()
	{
		if (!Replication.IsServer() || m_mTagCache.IsEmpty())
			return;

		array<int> outIds = {};
		array<string> outTags = {};
		array<int> outXp = {};
		array<string> outTracks = {};

		foreach (int playerId, string tag : m_mTagCache)
		{
			outIds.Insert(playerId);
			outTags.Insert(tag);
			outXp.Insert(GetPlayerXp(playerId));
			outTracks.Insert(GetPlayerRankTrack(playerId));
		}

		Print(string.Format("[CRF_CommunityTagManager][SERVER] Re-broadcasting cached info for %1 player(s)", outIds.Count()), LogLevel.NORMAL);
		Rpc(RpcDo_PlayerInfoUpdated, outIds, outTags, outXp, outTracks);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerInfoFetchFailed(RestCallback cb)
	{
		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		m_bFetching = false;
		m_iConsecutiveFailures++;
		bool bRetry = m_bPendingFetch;
		m_bPendingFetch = false;
		Print(string.Format("[CRF_CommunityTagManager] Failed to fetch player info (result: %1, http: %2)", cb.GetRestResult(), cb.GetHttpCode()), LogLevel.WARNING);
		if (bRetry)
			FetchPlayerInfo();
	}

	//------------------------------------------------------------------------------------------------
	//! Safety net for requests that never call back (dropped connection, silently swallowed error, etc).
	//! Without this, a single lost request would permanently wedge m_bFetching = true and no player
	//! would ever get tags/ranks again for the rest of the session.
	protected void OnFetchTimeout()
	{
		if (!m_bFetching)
			return;

		m_bFetching = false;
		m_iConsecutiveFailures++;

		if (m_iConsecutiveFailures >= MAX_CONSECUTIVE_FAILURES)
		{
			Print(string.Format("[CRF_CommunityTagManager] Player info fetch timed out after %1ms (%2 consecutive failures) — giving up until the next player connects or a client requests a refresh", FETCH_TIMEOUT_MS, m_iConsecutiveFailures), LogLevel.WARNING);
			return;
		}

		Print(string.Format("[CRF_CommunityTagManager] Player info fetch timed out after %1ms — resetting and retrying (attempt %2 of %3)", FETCH_TIMEOUT_MS, m_iConsecutiveFailures + 1, MAX_CONSECUTIVE_FAILURES), LogLevel.WARNING);
		FetchPlayerInfo();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — REPLICATION
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Client -> server: "send me what you have". Answered immediately from cache so the requesting
	//! player's UI populates without waiting on the backend, then refreshed in the background in
	//! case the roster changed since the last fetch.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestPlayerInfo()
	{
		BroadcastCachedPlayerInfo();
		m_iConsecutiveFailures = 0;
		ScheduleFetchDebounced();
	}

	//------------------------------------------------------------------------------------------------
	//! Applies resolved player-info data to the local cache. Called directly on the server (so the
	//! host's own cache updates even on a listen server) and via RPC on every client.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayerInfoUpdated(array<int> playerIds, array<string> tags, array<int> xp, array<string> tracks)
	{
		int count = playerIds.Count();
		for (int i = 0; i < count; i++)
		{
			m_mTagCache.Set(playerIds[i], tags[i]);
			m_mXpCache.Set(playerIds[i], xp[i]);
			m_mTrackCache.Set(playerIds[i], tracks[i]);
		}

		string side = "CLIENT";
		if (Replication.IsServer())
			side = "SERVER";
		Print(string.Format("[CRF_CommunityTagManager][%1] Applied player info for %2 player(s) to local cache", side, count), LogLevel.NORMAL);

		m_OnPlayerInfoUpdated.Invoke();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — PLAYER LIFECYCLE
//=============================================================================================================================================================================================================================================================================================================================================================

	protected ref CRF_GamemodeReadyWaiter m_GamemodeReadyWaiter;

	//------------------------------------------------------------------------------------------------
	//! Registers to game-mode connect/disconnect events so UI can refresh immediately
	//! and player info can be re-fetched when roster changes.
	protected void RegisterPlayerLifecycleCallbacks()
	{
		if (m_bPlayerEventsSubscribed)
			return;

		m_GamemodeReadyWaiter = new CRF_GamemodeReadyWaiter();
		m_GamemodeReadyWaiter.Setup(this);
		m_GamemodeReadyWaiter.Start(CRF_GamemodeReadyWaiter.INTERVAL_MS, CRF_GamemodeReadyWaiter.MAX_ATTEMPTS, "CommunityTagManager gamemode lookup");
	}

	//------------------------------------------------------------------------------------------------
	//! Called by CRF_GamemodeReadyWaiter once SCR_BaseGameMode is confirmed available.
	void OnGamemodeReady(SCR_BaseGameMode gameMode)
	{
		// Remove before insert to keep registration idempotent.
		gameMode.GetOnPlayerConnected().Remove(OnTrackedPlayerConnected);
		gameMode.GetOnPlayerConnected().Insert(OnTrackedPlayerConnected);
		gameMode.GetOnPlayerDisconnected().Remove(OnTrackedPlayerDisconnected);
		gameMode.GetOnPlayerDisconnected().Insert(OnTrackedPlayerDisconnected);

		m_bPlayerEventsSubscribed = true;

		// Bootstrap fetch: covers players already connected before this component initialized
		// (e.g. a listen-server host, or anyone present before the next new-player connect event).
		// OnTrackedPlayerConnected only fires for players joining AFTER this point.
		if (Replication.IsServer())
		{
			Print("[CRF_CommunityTagManager][SERVER] Lifecycle callbacks registered — scheduling initial fetch", LogLevel.NORMAL);
			ScheduleFetchDebounced();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTrackedPlayerConnected(int playerId)
	{
		m_OnPlayerRosterChanged.Invoke();

		// Only the server re-fetches; clients receive the result via RpcDo_PlayerInfoUpdated.
		if (Replication.IsServer())
		{
			Print(string.Format("[CRF_CommunityTagManager][SERVER] Player %1 connected — scheduling debounced fetch", playerId), LogLevel.NORMAL);
			// A new join is a fresh chance for the backend to be reachable again, and a new player
			// to resolve — both budgets start over.
			m_iConsecutiveFailures = 0;
			m_iReconcilePasses = 0;
			ScheduleFetchDebounced();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTrackedPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_mTagCache.Remove(playerId);
		m_mXpCache.Remove(playerId);
		m_mTrackCache.Remove(playerId);

		m_OnPlayerRosterChanged.Invoke();
		// No re-fetch: remaining players' cached data is still valid.
	}
}

//! Waits for SCR_BaseGameMode to exist before wiring up connect/disconnect tracking.
//! See CRF_CommunityTagManager.RegisterPlayerLifecycleCallbacks.
class CRF_GamemodeReadyWaiter : COA_RetryWaiter
{
	static const int INTERVAL_MS = 500;
	static const int MAX_ATTEMPTS = 120; // ~1 minute at 500ms interval

	protected CRF_CommunityTagManager m_Owner;

	//------------------------------------------------------------------------------------------------
	void Setup(CRF_CommunityTagManager owner)
	{
		m_Owner = owner;
	}

	//------------------------------------------------------------------------------------------------
	protected override bool IsConditionMet()
	{
		return SCR_BaseGameMode.Cast(GetGame().GetGameMode()) != null;
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnReady()
	{
		if (m_Owner)
			m_Owner.OnGamemodeReady(SCR_BaseGameMode.Cast(GetGame().GetGameMode()));
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnTimeout()
	{
		Print("[CRF_CommunityTagManager] ERROR: SCR_BaseGameMode never became available — player connect/disconnect tracking was not registered.", LogLevel.ERROR);
	}
}
