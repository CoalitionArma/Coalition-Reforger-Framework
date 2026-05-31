class CRF_CommunityTagManagerClass : ScriptComponentClass {}

//! Fetches and caches Coalition community tags for players from the website API.
//! Tags are displayed next to player names in slotting and briefing menus.
//! This component must be added to the CRF_Gamemode entity in the Workbench editor.
class CRF_CommunityTagManager : ScriptComponent
{
//=============================================================================================================================================================================================================================================================================================================================================================
//	CONSTANTS
//=============================================================================================================================================================================================================================================================================================================================================================

	//! Base URL for the Coalition bot API — trailing slash required
	protected static const string BOT_API_BASE_URL = "https://api.coalitiongroup.net/";

	//! Combined tags + XP endpoint (replaces the two separate endpoints)
	protected static const string PLAYER_INFO_ENDPOINT = "api/game/player-info?names=";

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

	//! Cache: player name -> community tag string (empty string means no tag)
	protected ref map<string, string> m_mTagCache = new map<string, string>;

	//! Prevents duplicate simultaneous in-flight HTTP requests
	protected bool m_bFetching = false;

	//! Set when FetchPlayerInfo() is called while a fetch is already in-flight.
	//! The next completed fetch will automatically trigger a follow-up fetch.
	protected bool m_bPendingFetch = false;

	//! REST callback stored as ref so it is not garbage-collected mid-flight
	protected ref RestCallback m_Callback;

	//! Cache: player name -> XP value (-1 means not cached)
	protected ref map<string, int> m_mXpCache = new map<string, int>;

	//! Cache: player name -> rank track ("enlisted" / "warrant" / "officer"), empty = default enlisted
	protected ref map<string, string> m_mTrackCache = new map<string, string>;

	//! Fired after both tags and XP are fetched and caches are populated
	protected ref ScriptInvoker m_OnPlayerInfoUpdated = new ScriptInvoker;

//=============================================================================================================================================================================================================================================================================================================================================================
//	STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void CRF_CommunityTagManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
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
	//! Returns the plain player name (no tag prefix). Convenience wrapper.
	string GetPlayerDisplayName(int playerId)
	{
		return GetGame().GetPlayerManager().GetPlayerName(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the community tag string for a player (e.g. "CRF"), or empty string if none cached.
	string GetPlayerTag(int playerId)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (name.IsEmpty())
			return string.Empty;

		string tag;
		if (m_mTagCache.Find(name, tag))
			return tag;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the cached XP value for a player, or -1 if not cached / no data.
	int GetPlayerXp(int playerId)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (name.IsEmpty())
			return -1;

		int xp;
		if (m_mXpCache.Find(name, xp))
			return xp;

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the cached rank track for a player: "enlisted", "warrant", or "officer".
	//! Defaults to "enlisted" if the player has no stored preference.
	string GetPlayerRankTrack(int playerId)
	{
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (name.IsEmpty())
			return "enlisted";

		string track;
		if (m_mTrackCache.Find(name, track) && !track.IsEmpty())
			return track;

		return "enlisted";
	}

	//------------------------------------------------------------------------------------------------
	//! Fetches community tags and XP for all connected players in a single HTTP request.
	//! If a fetch is already in-flight, queues one follow-up fetch to run after completion.
	//! Subscribers to GetOnPlayerInfoUpdated() are notified when data arrives.
	void FetchPlayerInfo()
	{
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
			string encodedName = name;
			encodedName.Replace(" ", "%20");
			if (!first)
				queryNames += ",";
			first = false;
			queryNames += encodedName;
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
		ctx.SetHeaders("Content-Type,application/json");
		ctx.GET(m_Callback, PLAYER_INFO_ENDPOINT + queryNames);
	}

	//! Schedules a FetchPlayerInfo() call after a delay in milliseconds.
	//! Use this for JIP connects where the player name may not be registered yet.
	void FetchPlayerInfoDelayed(int delayMs = 2000)
	{
		GetGame().GetCallqueue().CallLater(FetchPlayerInfo, delayMs, false);
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
//	PRIVATE — REST CALLBACKS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called when the combined player-info API responds successfully.
	//! Parses: {"success":true,"tags":{"Name":"CRF"|null},"xp":{"Name":15000|null}}
	protected void OnPlayerInfoFetched(RestCallback cb)
	{
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
				playerName.Replace("%20", " ");
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
					m_mTagCache.Set(playerName, tag);
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
				playerName.Replace("%20", " ");
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
					m_mXpCache.Set(playerName, xp);
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
				playerName.Replace("%20", " ");
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
					m_mTrackCache.Set(playerName, track);
				if (pos < data.Length() && data.ContainsAt(",", pos))
					pos++;
			}
		}

		m_OnPlayerInfoUpdated.Invoke();

		if (bRetry)
			FetchPlayerInfo();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerInfoFetchFailed(RestCallback cb)
	{
		m_bFetching = false;
		bool bRetry = m_bPendingFetch;
		m_bPendingFetch = false;
		Print(string.Format("[CRF_CommunityTagManager] Failed to fetch player info (result: %1)", cb.GetRestResult()), LogLevel.WARNING);
		if (bRetry)
			FetchPlayerInfo();
	}
}
