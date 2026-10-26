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

	// Website backend game API. RestApi.GetContext hands out one shared context per base URL, and
	// CRF_CommunityTagManager uses "https://api.coalitiongroup.net/" - so this base URL is deliberately
	// different: sharing a context would let its SetHeaders (no server key) and its queued requests
	// apply to ours. Endpoints start with a slash, as in CRF_ReplayDownloader.
	protected static const string API_BASE_URL = "https://api.coalitiongroup.net/api/game";
	protected static const string CURRENT_MISSION_ENDPOINT = "/current-mission?name=";
	protected static const int LOOKUP_RETRY_MS = 15000;
	protected static const int LOOKUP_MAX_ATTEMPTS = 4;
	protected static const string MISSION_RATING_ENDPOINT = "/mission-rating";
	protected static const string AAR_REVIEW_ENDPOINT = "/aar-review";
	// Engine timeout for every request on the context
	protected static const int REQUEST_TIMEOUT_S = 15;
	// Written AAR requests the engine never calls back about are given up on after this
	static const int REVIEW_WATCHDOG_MS = 20000;
	// A written AAR request that got no answer at all (timeout, dropped connection) is sent once more
	protected static const int REVIEW_MAX_ATTEMPTS = 2;
	protected static const int REVIEW_RETRY_DELAY_MS = 1000;
	// Written AAR: form loads and submissions per player, not faster than this
	protected static const int REVIEW_COOLDOWN_MS = 4000;
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

	// Written AAR requests waiting on the website, and each player's last form load / submission (world ms)
	protected ref array<ref CRF_AARReviewRequest> m_aReviewRequests = {};
	protected ref map<int, int> m_mLastReviewFormTime = new map<int, int>();
	protected ref map<int, int> m_mLastReviewSubmitTime = new map<int, int>();

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
		GetGame().GetCallqueue().Remove(SendReviewRequest);
		m_mPendingMarkers.Clear();
		m_sPlayersServed.Clear();
		m_mLastRatingTime.Clear();
		m_mLastReviewFormTime.Clear();
		m_mLastReviewSubmitTime.Clear();
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
	//! This manager's REST context (see API_BASE_URL), with the timeout and headers every request
	//! uses - the server key is sent when configured (the mission lookup doesn't need it)
	protected RestContext GetApiContext()
	{
		RestApi rest = GetGame().GetRestApi();
		if (!rest)
			return null;

		RestContext context = rest.GetContext(API_BASE_URL);
		if (!context)
			return null;

		string headers = "Content-Type,application/json";
		string key = CRF_WebsiteApiConfig.GetGameServerKey();
		if (!key.IsEmpty())
			headers += ",x-game-server-key," + key;

		context.SetTimeout(REQUEST_TIMEOUT_S);
		context.SetHeaders(headers);
		return context;
	}

	//------------------------------------------------------------------------------------------------
	//! Asks the website backend which coalitiongroup.net/aar/<id> page belongs to this round. The bot
	//! only creates that record for qualifying rounds (player count, game type), so "not found" is a
	//! normal outcome and the AAR screen then links to the Past Missions list instead.
	protected void LookupMissionId()
	{
		m_iLookupAttempts++;

		RestContext context = GetApiContext();
		if (!context)
			return;

		m_LookupCallback = new RestCallback();
		m_LookupCallback.SetOnSuccess(OnLookupSuccess);
		m_LookupCallback.SetOnError(OnLookupFailed);

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

		RestContext context = GetApiContext();
		if (!context)
			return;

		m_mLastRatingTime.Set(playerId, now);

		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string payload = string.Format("{\"missionId\":%1,\"guid\":\"%2\",\"name\":\"%3\",\"rating\":%4}",
			m_iMissionId, EscapeJsonString(guid), EscapeJsonString(playerName), rating);

		CRF_MissionRatingRequest request = new CRF_MissionRatingRequest(this, playerId, rating);
		m_aRatingRequests.Insert(request);

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
//	 WRITTEN AAR REVIEW
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Server: a player opened the AAR form - send them their saved review (if any) and the leaders
	//! they can rate: the website ORBAT's leaders plus this round's slotted leaders.
	void LoadReviewForm(int playerId)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (!ownerManager)
			return;

		int now = GetGame().GetWorld().GetWorldTime();
		int last;
		if (m_mLastReviewFormTime.Find(playerId, last) && now - last < REVIEW_COOLDOWN_MS)
			return;
		m_mLastReviewFormTime.Set(playerId, now);

		array<string> leaderOptions = {};
		AddInGameLeaders(playerId, leaderOptions);

		if (CRF_WebsiteApiConfig.GetGameServerKey().IsEmpty() || m_iMissionId <= 0 || !GetGame().GetRestApi())
		{
			SendEmptyReviewForm(ownerManager, leaderOptions);
			return;
		}

		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string path = string.Format("%1?missionId=%2&guid=%3", AAR_REVIEW_ENDPOINT, m_iMissionId, EncodeQueryValue(guid));
		CRF_AARReviewRequest request = new CRF_AARReviewRequest(this, playerId, false, path);
		m_aReviewRequests.Insert(request);
		SendReviewRequest(request);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: post a player's review to the website on their behalf
	void SubmitReview(int playerId, CRF_AARReviewData review)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(playerId);
		if (!ownerManager)
			return;

		if (!review)
		{
			ownerManager.ReceiveAARReviewResult(false, "Your AAR couldn't be read - try again.");
			return;
		}

		string error;
		if (!review.Validate(error))
		{
			ownerManager.ReceiveAARReviewResult(false, error);
			return;
		}

		if (m_iMissionId <= 0)
		{
			ownerManager.ReceiveAARReviewResult(false, "This round has no AAR page on coalitiongroup.net.");
			return;
		}

		int now = GetGame().GetWorld().GetWorldTime();
		int last;
		if (m_mLastReviewSubmitTime.Find(playerId, last) && now - last < REVIEW_COOLDOWN_MS)
		{
			ownerManager.ReceiveAARReviewResult(false, "Please wait a few seconds before submitting again.");
			return;
		}

		if (CRF_WebsiteApiConfig.GetGameServerKey().IsEmpty() || !GetGame().GetRestApi())
		{
			ownerManager.ReceiveAARReviewResult(false, "This server isn't set up to send AARs to the website - write it on coalitiongroup.net instead.");
			return;
		}

		m_mLastReviewSubmitTime.Set(playerId, now);

		CRF_AARReviewRequest request = new CRF_AARReviewRequest(this, playerId, true, BuildReviewJson(playerId, review));
		m_aReviewRequests.Insert(request);
		SendReviewRequest(request);
	}

	//------------------------------------------------------------------------------------------------
	//! Send (or resend) a written AAR request: a form load GETs m_sRequest, a submission POSTs it
	protected void SendReviewRequest(CRF_AARReviewRequest request)
	{
		if (!request || !m_aReviewRequests.Contains(request))
			return;

		RestContext context = GetApiContext();
		if (!context)
		{
			HandleReviewResponse(request, false);
			m_aReviewRequests.RemoveItem(request);
			return;
		}

		RestCallback callback = request.BeginAttempt();
		if (request.m_bSubmit)
			context.POST(callback, AAR_REVIEW_ENDPOINT, request.m_sRequest);
		else
			context.GET(callback, request.m_sRequest);
	}

	//------------------------------------------------------------------------------------------------
	//! Called by CRF_AARReviewRequest when the website answers, the request fails, or the watchdog
	//! gives up on it
	void OnReviewRequestFinished(CRF_AARReviewRequest request, bool ok)
	{
		// No answer at all (timeout, dropped connection, lost callback): try once more. A submission
		// is an upsert on the website, so resending one that did arrive just saves it again.
		if (!ok && request.m_iHttpCode <= 0 && request.m_iAttempts < REVIEW_MAX_ATTEMPTS)
		{
			Print(string.Format("[CRF_AARManager] No answer from the website to %1 for player %2 (%3) - retrying", request.GetDescription(), request.m_iPlayerId, request.GetFailureReason()), LogLevel.WARNING);
			GetGame().GetCallqueue().CallLater(SendReviewRequest, REVIEW_RETRY_DELAY_MS, false, request);
			return;
		}

		HandleReviewResponse(request, ok);
		m_aReviewRequests.RemoveItem(request);
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleReviewResponse(CRF_AARReviewRequest request, bool ok)
	{
		COA_PlayerRplToOwnerManager ownerManager = COA_PlayerRplToOwnerManager.GetForPlayer(request.m_iPlayerId);
		if (!ownerManager)
			return;

		int httpCode = request.m_iHttpCode;
		string data = request.m_sData;

		if (request.m_bSubmit)
		{
			if (ok && httpCode == HttpCode.HTTP_CODE_200)
			{
				ownerManager.ReceiveAARReviewResult(true, "");
				return;
			}

			if (httpCode <= 0)
			{
				Print(string.Format("[CRF_AARManager] AAR review from player %1 got no answer from the website after %2 attempt(s) (%3)", request.m_iPlayerId, request.m_iAttempts, request.GetFailureReason()), LogLevel.WARNING);
				ownerManager.ReceiveAARReviewResult(false, "coalitiongroup.net didn't answer in time - try again in a moment.");
				return;
			}

			string message = ExtractJsonError(data);
			if (message.IsEmpty())
				message = GetReviewErrorMessage(httpCode);

			Print(string.Format("[CRF_AARManager] Website rejected AAR review from player %1 (HTTP %2): %3", request.m_iPlayerId, httpCode, message), LogLevel.WARNING);
			ownerManager.ReceiveAARReviewResult(false, message);
			return;
		}

		if (!ok || httpCode != HttpCode.HTTP_CODE_200)
			Print(string.Format("[CRF_AARManager] Couldn't load the AAR form for player %1 (HTTP %2, %3) - sending an empty one", request.m_iPlayerId, httpCode, request.GetFailureReason()), LogLevel.WARNING);

		// Form load
		array<string> leaderOptions = {};
		CRF_AARReviewFormJson form = new CRF_AARReviewFormJson();
		bool parsed = false;
		if (ok && httpCode == HttpCode.HTTP_CODE_200 && !data.IsEmpty())
		{
			JsonLoadContext loadContext = new JsonLoadContext();
			parsed = loadContext.LoadFromString(data) && loadContext.ReadValue("", form);
		}

		if (!parsed)
		{
			AddInGameLeaders(request.m_iPlayerId, leaderOptions);
			SendEmptyReviewForm(ownerManager, leaderOptions);
			return;
		}

		if (form.leaderOptions)
		{
			foreach (string option : form.leaderOptions)
			{
				if (!option.IsEmpty() && !leaderOptions.Contains(option))
					leaderOptions.Insert(option);
			}
		}
		AddInGameLeaders(request.m_iPlayerId, leaderOptions);

		array<string> strings = {};
		array<int> ints = {};
		if (form.hasReview && form.review)
		{
			CRF_AARReviewData review = new CRF_AARReviewData();
			review.m_sWentWell = form.review.went_well;
			review.m_sWentBad = form.review.went_bad;
			review.m_sFeedback = form.review.mission_feedback;
			review.m_aCategories[0] = form.review.pacing;
			review.m_aCategories[1] = form.review.objectives_rating;
			review.m_aCategories[2] = form.review.terrain_rating;
			review.m_aCategories[3] = form.review.assets_rating;
			review.m_aCategories[4] = form.review.structure_rating;
			if (form.review.leadershipRatings)
			{
				foreach (CRF_AARLeaderJson leaderJson : form.review.leadershipRatings)
				{
					if (!leaderJson || review.m_aLeaders.Count() >= CRF_AARReviewData.MAX_LEADERS)
						continue;

					CRF_AARLeaderRating leader = new CRF_AARLeaderRating();
					leader.m_sName = leaderJson.leader_name;
					leader.m_sReason = leaderJson.reason;
					leader.m_aScores[0] = leaderJson.initiative;
					leader.m_aScores[1] = leaderJson.clarity;
					leader.m_aScores[2] = leaderJson.organization;
					leader.m_aScores[3] = leaderJson.adaptability;
					leader.m_aScores[4] = leaderJson.authority;
					leader.m_iKarma = leaderJson.karma_vote;
					review.m_aLeaders.Insert(leader);
				}
			}

			review.Pack(strings, ints);
		}

		ownerManager.ReceiveAARReviewForm(form.linked, form.hasReview && !strings.IsEmpty(), strings, ints, leaderOptions);
	}

	//------------------------------------------------------------------------------------------------
	protected void SendEmptyReviewForm(COA_PlayerRplToOwnerManager ownerManager, array<string> leaderOptions)
	{
		array<string> strings = {};
		array<int> ints = {};
		ownerManager.ReceiveAARReviewForm(true, false, strings, ints, leaderOptions);
	}

	//------------------------------------------------------------------------------------------------
	//! Names of this round's slotted leaders still connected, except the requesting player
	protected void AddInGameLeaders(int excludePlayerId, notnull array<string> names)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		array<int> leaderRoles = {COA_EGearRole.COMPANY_COMMANDER, COA_EGearRole.FIRST_SERGEANT, COA_EGearRole.PLATOON_LEADER,
			COA_EGearRole.PLATOON_SERGEANT, COA_EGearRole.SQUAD_LEAD, COA_EGearRole.VEHICLE_LEAD, COA_EGearRole.INDIRECT_LEAD,
			COA_EGearRole.LOGI_LEAD, COA_EGearRole.TEAM_LEAD};

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (playerId == excludePlayerId)
				continue;

			COA_SlotData slotData = slottingManager.GetPlayerSlotData(playerId);
			if (!slotData || !leaderRoles.Contains(slotData.GetSlotRole()))
				continue;

			string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!name.IsEmpty() && !names.Contains(name))
				names.Insert(name);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected string BuildReviewJson(int playerId, CRF_AARReviewData review)
	{
		string guid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string json = string.Format("{\"missionId\":%1,\"guid\":\"%2\",", m_iMissionId, EscapeJsonText(guid));
		json += string.Format("\"went_well\":\"%1\",\"went_bad\":\"%2\",\"mission_feedback\":\"%3\",",
			EscapeJsonText(review.m_sWentWell.Trim()), EscapeJsonText(review.m_sWentBad.Trim()), EscapeJsonText(review.m_sFeedback.Trim()));
		json += string.Format("\"pacing\":%1,\"objectives_rating\":%2,\"terrain_rating\":%3,\"assets_rating\":%4,\"structure_rating\":%5,",
			review.m_aCategories[0], review.m_aCategories[1], review.m_aCategories[2], review.m_aCategories[3], review.m_aCategories[4]);

		json += "\"leadershipRatings\":[";
		foreach (int i, CRF_AARLeaderRating leader : review.m_aLeaders)
		{
			if (i > 0)
				json += ",";

			json += string.Format("{\"leader_name\":\"%1\",\"initiative\":%2,\"clarity\":%3,\"organization\":%4,\"adaptability\":%5,\"authority\":%6,",
				EscapeJsonText(leader.m_sName.Trim()), leader.m_aScores[0], leader.m_aScores[1], leader.m_aScores[2], leader.m_aScores[3], leader.m_aScores[4]);
			json += string.Format("\"karma_vote\":%1,\"reason\":\"%2\"}", leader.m_iKarma, EscapeJsonText(leader.m_sReason.Trim()));
		}
		json += "]}";
		return json;
	}

	//------------------------------------------------------------------------------------------------
	//! JSON string escaping for free text (quotes, backslashes, line breaks, tabs)
	protected static string EscapeJsonText(string value)
	{
		string escaped = value;
		escaped.Replace("\\", "\\\\");
		escaped.Replace("\"", "\\\"");
		escaped.Replace("\r", "");
		escaped.Replace("\n", "\\n");
		escaped.Replace("\t", "\\t");
		return escaped;
	}

	//------------------------------------------------------------------------------------------------
	//! The "error" text of a website error response ({"error":"..."}), or ""
	protected static string ExtractJsonError(string data)
	{
		int keyIndex = data.IndexOf("\"error\":\"");
		if (keyIndex < 0)
			return "";

		int start = keyIndex + 9;
		string message;
		for (int i = start, length = data.Length(); i < length; i++)
		{
			string character = data.Get(i);
			if (character == "\"")
				break;

			if (character == "\\" && i + 1 < length)
			{
				i++;
				character = data.Get(i);
			}

			message += character;
		}

		return message;
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetReviewErrorMessage(int httpCode)
	{
		switch (httpCode)
		{
			case 401: return "This server's website key was rejected - write your AAR on coalitiongroup.net instead.";
			case 403: return "Link your Arma GUID on your coalitiongroup.net profile to submit an AAR in game.";
			case 404: return "This round has no AAR page on coalitiongroup.net.";
		}

		return "The website couldn't save your AAR right now - try again in a moment.";
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


//------------------------------------------------------------------------------------------------
//! One in-flight written-AAR request to the website: a form load (GET) or a submission (POST).
//! Each attempt gets a fresh callback and a watchdog, so a request the engine never answers still
//! finishes (and a late answer to an attempt already given up on is ignored).
class CRF_AARReviewRequest
{
	int m_iPlayerId;
	bool m_bSubmit;
	string m_sRequest; // GET path with query, or the POST body
	int m_iAttempts;

	// Outcome of the last attempt (m_iHttpCode is 0 when nothing came back)
	int m_iHttpCode;
	string m_sData;
	ERestResult m_eRestResult;
	bool m_bTimedOut;

	protected ref RestCallback m_Callback;
	// Callbacks of earlier attempts, kept alive in case the engine still calls them
	protected ref array<ref RestCallback> m_aSpentCallbacks = {};
	protected CRF_AARManager m_Owner;

	//------------------------------------------------------------------------------------------------
	void CRF_AARReviewRequest(CRF_AARManager owner, int playerId, bool submit, string request)
	{
		m_Owner = owner;
		m_iPlayerId = playerId;
		m_bSubmit = submit;
		m_sRequest = request;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_AARReviewRequest()
	{
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(OnWatchdog);
	}

	//------------------------------------------------------------------------------------------------
	//! \return the callback to send the next attempt with
	RestCallback BeginAttempt()
	{
		if (m_Callback)
			m_aSpentCallbacks.Insert(m_Callback);

		m_iAttempts++;
		m_iHttpCode = 0;
		m_sData = "";
		m_bTimedOut = false;

		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnSuccess);
		m_Callback.SetOnError(OnError);

		GetGame().GetCallqueue().Remove(OnWatchdog);
		GetGame().GetCallqueue().CallLater(OnWatchdog, CRF_AARManager.REVIEW_WATCHDOG_MS, false);
		return m_Callback;
	}

	//------------------------------------------------------------------------------------------------
	string GetDescription()
	{
		if (m_bSubmit)
			return "an AAR submission";

		return "an AAR form load";
	}

	//------------------------------------------------------------------------------------------------
	//! Why the last attempt failed, for the log
	string GetFailureReason()
	{
		if (m_bTimedOut)
			return string.Format("no callback within %1 ms", CRF_AARManager.REVIEW_WATCHDOG_MS);

		return typename.EnumToString(ERestResult, m_eRestResult);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSuccess(RestCallback callback)
	{
		Finish(callback, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnError(RestCallback callback)
	{
		Finish(callback, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void Finish(RestCallback callback, bool ok)
	{
		// A late answer to an attempt the watchdog already gave up on
		if (!m_Callback || callback != m_Callback)
			return;

		GetGame().GetCallqueue().Remove(OnWatchdog);
		m_iHttpCode = callback.GetHttpCode();
		m_sData = callback.GetData();
		m_eRestResult = callback.GetRestResult();

		if (m_Owner)
			m_Owner.OnReviewRequestFinished(this, ok);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWatchdog()
	{
		if (!m_Callback)
			return;

		m_aSpentCallbacks.Insert(m_Callback);
		m_Callback = null;
		m_bTimedOut = true;

		if (m_Owner)
			m_Owner.OnReviewRequestFinished(this, false);
	}
}
