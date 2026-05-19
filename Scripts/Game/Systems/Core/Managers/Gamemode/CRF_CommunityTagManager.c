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

	//! API endpoint path (relative to base URL)
	protected static const string TAG_API_ENDPOINT = "api/game/community-tags?names=";

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

	//! Fired after tags are fetched and the cache is populated
	protected ref ScriptInvoker m_OnTagsUpdated = new ScriptInvoker;

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
	//! Returns a ScriptInvoker that fires with no arguments when tags are fetched.
	//! Menus can subscribe their refresh method to this to update when tags arrive.
	ScriptInvoker GetOnTagsUpdated()
	{
		return m_OnTagsUpdated;
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
	//! Triggers an async fetch of community tags for all currently connected players.
	//! Safe to call multiple times; duplicate calls while a fetch is in-flight are ignored.
	//! Subscribers to GetOnTagsUpdated() are notified when tags arrive.
	void FetchTagsForCurrentPlayers()
	{
		if (m_bFetching)
			return;

		// Collect connected player names
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);

		if (playerIds.IsEmpty())
			return;

		// Build comma-separated URL-encoded names query string
		string queryNames = "";
		bool first = true;
		foreach (int playerId : playerIds)
		{
			if (playerId <= 0)
				continue;

			string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (name.IsEmpty())
				continue;

			// URL-encode spaces so the query string is valid
			string encodedName = name;
			encodedName.Replace(" ", "%20");

			if (!first)
				queryNames += ",";
			first = false;

			queryNames += encodedName;
		}

		if (queryNames.IsEmpty())
			return;

		// Set up REST API context
		RestApi rest = GetGame().GetRestApi();
		if (!rest)
			return;

		RestContext ctx = rest.GetContext(BOT_API_BASE_URL);
		if (!ctx)
			return;

		// Set up callback (stored as ref to survive until the response arrives)
		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnTagsFetched);
		m_Callback.SetOnError(OnTagsFetchFailed);

		m_bFetching = true;
		ctx.SetHeaders("Content-Type,application/json");
		ctx.GET(m_Callback, TAG_API_ENDPOINT + queryNames);
	}

	//------------------------------------------------------------------------------------------------
	//! Clears the tag cache. Call when the player roster changes significantly (e.g. new mission).
	void ClearCache()
	{
		m_mTagCache.Clear();
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	PRIVATE — REST CALLBACKS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Called when the tag API responds successfully.
	//! Parses: {"success":true,"tags":{"PlayerName":"CRF","OtherPlayer":null}}
	protected void OnTagsFetched(RestCallback cb)
	{
		m_bFetching = false;

		string data = cb.GetData();
		if (data.IsEmpty())
			return;

		// Locate the "tags":{ opening
		int tagsObjStart = data.IndexOf("\"tags\":{");
		if (tagsObjStart < 0)
			return;

		// pos starts just after the opening { of the tags value object
		int pos = tagsObjStart + 8; // len("\"tags\":{") == 8

		// Parse all key-value pairs until we reach the closing } of the tags object
		while (pos < data.Length())
		{
			// Look for the opening quote that starts a key
			int keyOpen = data.IndexOfFrom(pos, "\"");
			if (keyOpen < 0)
				break;

			// If there is a closing brace } before this quote, we have left the tags object
			string between = data.Substring(pos, keyOpen - pos);
			if (between.Contains("}"))
				break;

			// Find the closing quote of the key
			int keyClose = data.IndexOfFrom(keyOpen + 1, "\"");
			if (keyClose < 0)
				break;

			string playerName = data.Substring(keyOpen + 1, keyClose - keyOpen - 1);
			// Decode URL-encoded spaces back to spaces (server returns decoded names,
			// but add this as a safety measure in case encoding leaks through)
			playerName.Replace("%20", " ");

			// Advance past the colon separator
			int colonPos = data.IndexOfFrom(keyClose + 1, ":");
			if (colonPos < 0)
				break;

			int valueStart = colonPos + 1;
			if (valueStart >= data.Length())
				break;

			string tag = "";

			// Check for JSON null
			if (data.ContainsAt("null", valueStart))
			{
				// No tag for this player — tag stays as empty string
				pos = valueStart + 4;
			}
			// Check for a quoted string value
			else if (data.ContainsAt("\"", valueStart))
			{
				int tagClose = data.IndexOfFrom(valueStart + 1, "\"");
				if (tagClose < 0)
					break;

				tag = data.Substring(valueStart + 1, tagClose - valueStart - 1);
				pos = tagClose + 1;
			}
			else
			{
				// Unexpected format — stop parsing
				break;
			}

			if (!playerName.IsEmpty())
				m_mTagCache.Set(playerName, tag);

			// Advance past the comma separating pairs, if any
			if (pos < data.Length() && data.ContainsAt(",", pos))
				pos++;
		}

		// Notify menus so they can re-render with the new tags
		m_OnTagsUpdated.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnTagsFetchFailed(RestCallback cb)
	{
		m_bFetching = false;
		Print(string.Format("[CRF_CommunityTagManager] Failed to fetch community tags (result: %1)", cb.GetRestResult()), LogLevel.WARNING);
	}
}
