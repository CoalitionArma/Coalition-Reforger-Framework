//------------------------------------------------------------------------------------------------
// Server-side set-up for the after action review, started when the round enters
// COA_EGamemodeState.AAR (see CRF_COA_Gamemode.OnGamemodeStateChanged):
//  - puts every player into a voice channel for their group (COA_MenuManager.SetAllChannels)
//  - looks up this round's coalitiongroup.net/aar/<id> page from the website backend
//  - sends every map marker, of every faction, to each client that opens the AAR screen
//    (clients normally discard other factions' markers - see SCR_MapMarkerManagerComponent)
//  - forwards players' 1-5 ratings of the round to the website (needs CRF_WebsiteApiConfig)
//------------------------------------------------------------------------------------------------
class CRF_AARManager
{
	protected static ref CRF_AARManager s_Instance;

	// Website backend (same host CRF_CommunityTagManager uses) - trailing slash required
	protected static const string API_BASE_URL = "https://api.coalitiongroup.net/";
	protected static const string CURRENT_MISSION_ENDPOINT = "api/game/current-mission?name=";
	protected static const int LOOKUP_RETRY_MS = 15000;
	protected static const int LOOKUP_MAX_ATTEMPTS = 4;
	protected static const string MISSION_RATING_ENDPOINT = "api/game/mission-rating";
	// A player can change their rating, but not faster than this
	protected static const int RATING_COOLDOWN_MS = 2000;

	// Markers are streamed in small batches so a full map doesn't arrive as one burst of RPCs
	protected static const int MARKERS_PER_BATCH = 10;
	protected static const int MARKER_BATCH_INTERVAL_MS = 100;

	protected int m_iMissionId; // 0 until the lookup succeeds
	protected int m_iLookupAttempts;
	protected ref RestCallback m_LookupCallback;

	// playerId -> marker IDs still to send to that player
	protected ref map<int, ref array<int>> m_mPendingMarkers = new map<int, ref array<int>>();
	protected ref set<int> m_sPlayersServed = new set<int>();

	// Rating requests waiting on the website, and each player's last submission time (world ms)
	protected ref array<ref CRF_MissionRatingRequest> m_aRatingRequests = {};
	protected ref map<int, int> m_mLastRatingTime = new map<int, int>();
	protected bool m_bWarnedMissingKey;

	//------------------------------------------------------------------------------------------------
	static CRF_AARManager GetInstance()
	{
		if (!s_Instance)
			s_Instance = new CRF_AARManager();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Drop the previous round's AAR state (called when a new round reaches slotting)
	static void Reset()
	{
		if (s_Instance)
			s_Instance.Cleanup();

		// Side channels only exist for the AAR
		COA_MenuManager menuManager = COA_MenuManager.GetInstance();
		if (menuManager)
			menuManager.RemoveFactionChannels();

		s_Instance = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void Cleanup()
	{
		GetGame().GetCallqueue().Remove(LookupMissionId);
		GetGame().GetCallqueue().Remove(SendMarkerBatch);
		m_mPendingMarkers.Clear();
		m_sPlayersServed.Clear();
		m_mLastRatingTime.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: called once when the round enters the AAR state
	void StartAAR()
	{
		if (!Replication.IsServer())
			return;

		BuildGroupVoiceChannels();

		m_iLookupAttempts = 0;
		LookupMissionId();
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a client opened the AAR screen - send it the mission link and every map marker.
	//! Each player is served once per round.
	void SendAARDataToPlayer(int playerId)
	{
		if (playerId <= 0 || m_sPlayersServed.Contains(playerId))
			return;

		m_sPlayersServed.Insert(playerId);

		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (!ownerManager)
			return;

		if (m_iMissionId > 0)
			ownerManager.ReceiveAARMissionLink(m_iMissionId);

		QueueAllMarkersForPlayer(playerId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GROUP VOICE CHANNELS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! One channel per group, in slotting (ORBAT) order, each holding that group's connected players,
	//! plus an empty side channel per faction that had players (listed under Global, joinable only by
	//! that faction). Unslotted players and spectators stay in Global.
	protected void BuildGroupVoiceChannels()
	{
		COA_MenuManager menuManager = COA_MenuManager.GetInstance();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!menuManager || !slottingManager)
			return;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);

		array<string> channelNames = {};
		array<ref array<int>> channelPlayers = {};
		array<FactionKey> sideChannelFactions = {};

		array<string> factionKeys = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};
		foreach (string factionKey : factionKeys)
		{
			array<SCR_AIGroup> groups = slottingManager.GetAllGroups(factionKey);
			if (!groups)
				continue;

			foreach (SCR_AIGroup group : groups)
			{
				if (!group)
					continue;

				array<int> members = {};
				foreach (int playerId : playerIds)
				{
					if (slottingManager.GetPlayerSlotGroup(playerId) == group)
						members.Insert(playerId);
				}

				if (members.IsEmpty())
					continue;

				if (!sideChannelFactions.Contains(factionKey))
					sideChannelFactions.Insert(factionKey);

				channelNames.Insert(GetGroupChannelName(factionKey, group));
				channelPlayers.Insert(members);
			}
		}

		menuManager.SetAllChannels(channelNames, channelPlayers, sideChannelFactions);
		Print(string.Format("[CRF_AARManager] Created %1 AAR group voice channel(s) and %2 side channel(s)", channelNames.Count(), sideChannelFactions.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Name of a group's AAR voice channel, exactly as stored in COA_MenuManager.m_aVONChannels.
	//! Also used by the AAR screen to tell whether the local player's group channel still exists.
	static string GetGroupChannelName(FactionKey factionKey, SCR_AIGroup group)
	{
		return COA_MenuManager.SanitizeChannelName(string.Format("%1 - %2", factionKey, group.GetCustomNameWithOriginal()));
	}

	//------------------------------------------------------------------------------------------------
	//! Server: put a player in their group's AAR channel, recreating it if everyone had left (empty
	//! channels are removed automatically). Players can only open their own group's channel.
	void OpenGroupChannel(int playerId)
	{
		COA_MenuManager menuManager = COA_MenuManager.GetInstance();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!menuManager || !slottingManager)
			return;

		Faction faction = slottingManager.GetPlayerSlotFaction(playerId, true);
		SCR_AIGroup group = slottingManager.GetPlayerSlotGroup(playerId);
		if (!faction || !group)
			return;

		string channelName = GetGroupChannelName(faction.GetFactionKey(), group);

		// Someone may have reopened it already
		foreach (int channelIndex, string channelData : menuManager.m_aVONChannels)
		{
			array<string> channelParts = {};
			channelData.Split("|", channelParts, true);
			if (channelParts.IsEmpty() || channelParts[0] != channelName)
				continue;

			if (menuManager.GetChannel(playerId) != channelIndex)
				menuManager.AddPlayerToChannel(playerId, channelIndex, false);

			return;
		}

		menuManager.CreateChannel(channelName, playerId);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 WEBSITE MISSION LINK
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Asks the website backend which coalitiongroup.net/aar/<id> page belongs to this round. The bot
	//! only creates that record for qualifying rounds (player count, game type), so "not found" is a
	//! normal outcome and the AAR screen then links to the Past Missions list instead.
	protected void LookupMissionId()
	{
		m_iLookupAttempts++;

		RestApi rest = GetGame().GetRestApi();
		if (!rest)
			return;

		RestContext context = rest.GetContext(API_BASE_URL);
		if (!context)
			return;

		m_LookupCallback = new RestCallback();
		m_LookupCallback.SetOnSuccess(OnLookupSuccess);
		m_LookupCallback.SetOnError(OnLookupFailed);

		context.SetHeaders("Content-Type,application/json");
		context.GET(m_LookupCallback, CURRENT_MISSION_ENDPOINT + EncodeQueryValue(GetGame().GetMissionName()));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLookupSuccess(RestCallback callback)
	{
		// Only a 200 means the round's AAR page exists (clients copy the link to the clipboard on it)
		if (callback.GetHttpCode() != HttpCode.HTTP_CODE_200)
		{
			RetryLookup();
			return;
		}

		int missionId = ParseMissionId(callback.GetData());
		if (missionId <= 0)
		{
			RetryLookup();
			return;
		}

		m_iMissionId = missionId;
		Print(string.Format("[CRF_AARManager] AAR page: https://coalitiongroup.net/aar/%1", m_iMissionId), LogLevel.NORMAL);

		// Push to everyone already on the AAR screen; later arrivals get it in SendAARDataToPlayer
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
			if (ownerManager)
				ownerManager.ReceiveAARMissionLink(m_iMissionId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnLookupFailed(RestCallback callback)
	{
		RetryLookup();
	}

	//------------------------------------------------------------------------------------------------
	protected void RetryLookup()
	{
		if (m_iLookupAttempts >= LOOKUP_MAX_ATTEMPTS)
		{
			Print("[CRF_AARManager] No coalitiongroup.net AAR record found for this round", LogLevel.NORMAL);
			return;
		}

		GetGame().GetCallqueue().CallLater(LookupMissionId, LOOKUP_RETRY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Reads the "id" field from {"success":true,"id":260}. Returns 0 if absent.
	protected static int ParseMissionId(string data)
	{
		int keyIndex = data.IndexOf("\"id\":");
		if (keyIndex < 0)
			return 0;

		const string numerals = "0123456789";
		string digits;
		for (int i = keyIndex + 5, length = data.Length(); i < length; i++)
		{
			string character = data.Get(i);
			if (character == " ")
				continue;

			if (!numerals.Contains(character))
				break;

			digits += character;
		}

		return digits.ToInt();
	}

	//------------------------------------------------------------------------------------------------
	//! Percent-encodes the characters a mission name can contain that would break a query string
	protected static string EncodeQueryValue(string value)
	{
		string encoded = value;
		encoded.Replace("%", "%25");
		encoded.Replace(" ", "%20");
		encoded.Replace("&", "%26");
		encoded.Replace("#", "%23");
		encoded.Replace("+", "%2B");
		encoded.Replace("\"", "%22");
		return encoded;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISSION RATING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Server: a player rated the round 1-5 on the AAR screen. Posted to the website against this
	//! round's mission ID; the player is told whether it was saved.
	void SubmitRating(int playerId, int rating)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (!ownerManager)
			return;

		int now = GetGame().GetWorld().GetWorldTime();
		int lastRating;
		bool onCooldown = m_mLastRatingTime.Find(playerId, lastRating) && now - lastRating < RATING_COOLDOWN_MS;

		if (rating < 1 || rating > 5 || m_iMissionId <= 0 || onCooldown)
		{
			ownerManager.ReceiveMissionRatingResult(rating, false);
			return;
		}

		string key = CRF_WebsiteApiConfig.GetGameServerKey();
		RestApi rest = GetGame().GetRestApi();
		if (key.IsEmpty() || !rest)
		{
			if (!m_bWarnedMissingKey)
			{
				m_bWarnedMissingKey = true;
				Print("[CRF_AARManager] No gameServerKey in $profile:CRF_WebsiteApiConfig.json - mission ratings are not sent", LogLevel.WARNING);
			}

			ownerManager.ReceiveMissionRatingResult(rating, false);
			return;
		}

		RestContext context = rest.GetContext(API_BASE_URL);
		if (!context)
			return;

		m_mLastRatingTime.Set(playerId, now);

		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string payload = string.Format("{\"missionId\":%1,\"guid\":\"%2\",\"name\":\"%3\",\"rating\":%4}",
			m_iMissionId, EscapeJsonString(guid), EscapeJsonString(playerName), rating);

		CRF_MissionRatingRequest request = new CRF_MissionRatingRequest(this, playerId, rating);
		m_aRatingRequests.Insert(request);

		context.SetHeaders("Content-Type,application/json,x-game-server-key," + key);
		context.POST(request.m_Callback, MISSION_RATING_ENDPOINT, payload);
	}

	//------------------------------------------------------------------------------------------------
	//! Called by CRF_MissionRatingRequest when the website answers (or the request fails)
	void OnRatingRequestFinished(CRF_MissionRatingRequest request, bool saved)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(request.m_iPlayerId);
		if (ownerManager)
			ownerManager.ReceiveMissionRatingResult(request.m_iRating, saved);

		if (!saved)
			Print(string.Format("[CRF_AARManager] Website rejected mission rating from player %1 (HTTP %2)", request.m_iPlayerId, request.m_Callback.GetHttpCode()), LogLevel.WARNING);

		m_aRatingRequests.RemoveItem(request);
	}

	//------------------------------------------------------------------------------------------------
	protected static string EscapeJsonString(string value)
	{
		string escaped = value;
		escaped.Replace("\\", "\\\\");
		escaped.Replace("\"", "\\\"");
		return escaped;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MAP MARKERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void QueueAllMarkersForPlayer(int playerId)
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return;

		// The server keeps every marker (on a dedicated server they all sit in the disabled list)
		array<int> markerIds = {};
		foreach (SCR_MapMarkerBase marker : markerManager.GetStaticMarkers())
		{
			if (marker && !markerIds.Contains(marker.GetMarkerID()))
				markerIds.Insert(marker.GetMarkerID());
		}
		foreach (SCR_MapMarkerBase marker : markerManager.GetDisabledMarkers())
		{
			if (marker && !markerIds.Contains(marker.GetMarkerID()))
				markerIds.Insert(marker.GetMarkerID());
		}

		if (markerIds.IsEmpty())
			return;

		m_mPendingMarkers.Set(playerId, markerIds);

		GetGame().GetCallqueue().Remove(SendMarkerBatch);
		GetGame().GetCallqueue().CallLater(SendMarkerBatch, 0, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendMarkerBatch()
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return;

		int budget = MARKERS_PER_BATCH;
		array<int> finishedPlayers = {};

		foreach (int playerId, array<int> markerIds : m_mPendingMarkers)
		{
			COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
			if (!ownerManager)
			{
				finishedPlayers.Insert(playerId); // disconnected
				continue;
			}

			while (budget > 0 && !markerIds.IsEmpty())
			{
				int markerId = markerIds[0];
				markerIds.RemoveOrdered(0);

				SCR_MapMarkerBase marker = markerManager.GetStaticMarkerByID(markerId);
				if (!marker)
					marker = markerManager.GetDisabledMarkerByID(markerId);

				// Removed since it was queued
				if (!marker)
					continue;

				ownerManager.ReceiveAARMarker(marker);
				budget--;
			}

			if (markerIds.IsEmpty())
				finishedPlayers.Insert(playerId);

			if (budget <= 0)
				break;
		}

		foreach (int playerId : finishedPlayers)
			m_mPendingMarkers.Remove(playerId);

		if (!m_mPendingMarkers.IsEmpty())
			GetGame().GetCallqueue().CallLater(SendMarkerBatch, MARKER_BATCH_INTERVAL_MS, false);
	}
}


//------------------------------------------------------------------------------------------------
//! One in-flight POST of a player's mission rating (see CRF_AARManager.SubmitRating)
class CRF_MissionRatingRequest
{
	ref RestCallback m_Callback = new RestCallback();
	int m_iPlayerId;
	int m_iRating;
	protected CRF_AARManager m_Owner;

	//------------------------------------------------------------------------------------------------
	void CRF_MissionRatingRequest(CRF_AARManager owner, int playerId, int rating)
	{
		m_Owner = owner;
		m_iPlayerId = playerId;
		m_iRating = rating;
		m_Callback.SetOnSuccess(OnSuccess);
		m_Callback.SetOnError(OnError);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSuccess(RestCallback callback)
	{
		if (m_Owner)
			m_Owner.OnRatingRequestFinished(this, callback.GetHttpCode() == HttpCode.HTTP_CODE_200);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnError(RestCallback callback)
	{
		if (m_Owner)
			m_Owner.OnRatingRequestFinished(this, false);
	}
}
