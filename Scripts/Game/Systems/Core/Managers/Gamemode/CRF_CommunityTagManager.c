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
	//! All players start at 10000 XP (rank 1 = PVT).
	protected static const int RANK_XP_PVT = 0;     // Private          (E-1)
	protected static const int RANK_XP_PV2 = 15000; // Private Second Class (E-2)
	protected static const int RANK_XP_PFC = 25000; // Private First Class  (E-3)
	protected static const int RANK_XP_SPC = 40000; // Specialist           (E-4)
	protected static const int RANK_XP_CPL = 55000; // Corporal             (E-4)
	protected static const int RANK_XP_SGT = 75000; // Sergeant             (E-5)
	protected static const int RANK_XP_SSG = 100000; // Staff Sergeant      (E-6)
	protected static const int RANK_XP_SFC = 150000; // Sergeant First Class (E-7)

//=============================================================================================================================================================================================================================================================================================================================================================
//	RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================

	//! Singleton reference
	protected static CRF_CommunityTagManager m_sInstance;

	//! Cache: player name -> community tag string (empty string means no tag)
	protected ref map<string, string> m_mTagCache = new map<string, string>;

	//! Prevents duplicate simultaneous in-flight HTTP requests
	protected bool m_bFetching = false;

	//! REST callback stored as ref so it is not garbage-collected mid-flight
	protected ref RestCallback m_Callback;

	//! Cache: player name -> XP value (-1 means not cached)
	protected ref map<string, int> m_mXpCache = new map<string, int>;

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
	//! Fetches community tags and XP for all connected players in a single HTTP request.
	//! Safe to call multiple times; duplicate calls while a fetch is in-flight are ignored.
	//! Subscribers to GetOnPlayerInfoUpdated() are notified when data arrives.
	void FetchPlayerInfo()
	{
		if (m_bFetching)
			return;

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

	//! Backward-compatible wrappers — both now delegate to FetchPlayerInfo.
	void FetchTagsForCurrentPlayers()  { FetchPlayerInfo(); }
	void FetchRanksForCurrentPlayers() { FetchPlayerInfo(); }

	//------------------------------------------------------------------------------------------------
	//! Clears the tag and XP caches.
	void ClearCache()
	{
		m_mTagCache.Clear();
		m_mXpCache.Clear();
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

		string data = cb.GetData();
		if (data.IsEmpty())
			return;

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

		m_OnPlayerInfoUpdated.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPlayerInfoFetchFailed(RestCallback cb)
	{
		m_bFetching = false;
		Print(string.Format("[CRF_CommunityTagManager] Failed to fetch player info (result: %1)", cb.GetRestResult()), LogLevel.WARNING);
	}
}
