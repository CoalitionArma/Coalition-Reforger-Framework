class CRF_CommunityTagManagerClass : ScriptComponentClass {}

//! Fetches and caches Coalition community tags for players from the website API.
//! Tags are displayed next to player names in slotting and briefing menus.
//! This component must be added to the CRF_Gamemode entity in the Workbench editor.
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

	//! If neither the success nor the error callback fires within this window, the
	//! in-flight request is considered lost and m_bFetching is force-reset so future
	//! fetches aren't permanently blocked.
	protected static const int FETCH_TIMEOUT_MS = 15000;

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
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
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
	//! Server-authoritative: on clients this is a no-op — results arrive via RpcDo_PlayerInfoUpdated
	//! instead, so every client doesn't independently hammer the backend on every menu open.
	//! If a fetch is already in-flight, queues one follow-up fetch to run after completion.
	//! Subscribers to GetOnPlayerInfoUpdated() are notified when data arrives.
	void FetchPlayerInfo()
	{
		if (!Replication.IsServer())
			return;

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
			return;
		RestContext ctx = rest.GetContext(BOT_API_BASE_URL);
		if (!ctx)
			return;

		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnPlayerInfoFetched);
		m_Callback.SetOnError(OnPlayerInfoFetchFailed);

		m_bFetching = true;
		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		GetGame().GetCallqueue().CallLater(OnFetchTimeout, FETCH_TIMEOUT_MS, false);
		ctx.SetHeaders("Content-Type,application/json");
		ctx.GET(m_Callback, PLAYER_INFO_ENDPOINT + queryNames);
	}

	//! Debounced fetch: cancels any pending scheduled fetch and reschedules it
	//! DEBOUNCE_DELAY_MS from now. Call this on every connect event — a burst of
	//! N joins produces exactly one HTTP request once the burst settles.
	//! When the fetch completes, m_OnPlayerInfoUpdated fires so all UI subscribers
	//! automatically refresh their tag/rank icons.
	protected void ScheduleFetchDebounced()
	{
		GetGame().GetCallqueue().Remove(FetchPlayerInfo);
		GetGame().GetCallqueue().CallLater(FetchPlayerInfo, DEBOUNCE_DELAY_MS, false);
	}

	//! Backward-compatible wrappers — both now delegate to FetchPlayerInfo.
	void FetchTagsForCurrentPlayers()  { FetchPlayerInfo(); }
	void FetchRanksForCurrentPlayers() { FetchPlayerInfo(); }

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
		bool bRetry = m_bPendingFetch;
		m_bPendingFetch = false;

		string data = cb.GetData();
		if (data.IsEmpty())
		{
			if (bRetry)
				FetchPlayerInfo();
			return;
		}

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

		// Apply locally (covers dedicated + listen server) and replicate to every client.
		RpcDo_PlayerInfoUpdated(outIds, outTags, outXp, outTracks);
		Rpc(RpcDo_PlayerInfoUpdated, outIds, outTags, outXp, outTracks);

		if (bRetry)
			FetchPlayerInfo();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerInfoFetchFailed(RestCallback cb)
	{
		GetGame().GetCallqueue().Remove(OnFetchTimeout);
		m_bFetching = false;
		bool bRetry = m_bPendingFetch;
		m_bPendingFetch = false;
		Print(string.Format("[CRF_CommunityTagManager] Failed to fetch player info (result: %1)", cb.GetRestResult()), LogLevel.WARNING);
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

		Print(string.Format("[CRF_CommunityTagManager] Player info fetch timed out after %1ms — resetting and retrying", FETCH_TIMEOUT_MS), LogLevel.WARNING);
		m_bFetching = false;
		FetchPlayerInfo();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — REPLICATION
//=============================================================================================================================================================================================================================================================================================================================================================

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

		m_OnPlayerInfoUpdated.Invoke();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — PLAYER LIFECYCLE
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Registers to game-mode connect/disconnect events so UI can refresh immediately
	//! and player info can be re-fetched when roster changes.
	protected void RegisterPlayerLifecycleCallbacks()
	{
		if (m_bPlayerEventsSubscribed)
			return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
		{
			GetGame().GetCallqueue().CallLater(RegisterPlayerLifecycleCallbacks, 500, false);
			return;
		}

		// Remove before insert to keep registration idempotent.
		gameMode.GetOnPlayerConnected().Remove(OnTrackedPlayerConnected);
		gameMode.GetOnPlayerConnected().Insert(OnTrackedPlayerConnected);
		gameMode.GetOnPlayerDisconnected().Remove(OnTrackedPlayerDisconnected);
		gameMode.GetOnPlayerDisconnected().Insert(OnTrackedPlayerDisconnected);

		m_bPlayerEventsSubscribed = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTrackedPlayerConnected(int playerId)
	{
		m_OnPlayerRosterChanged.Invoke();

		// Only the server re-fetches; clients receive the result via RpcDo_PlayerInfoUpdated.
		if (Replication.IsServer())
			ScheduleFetchDebounced();
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
