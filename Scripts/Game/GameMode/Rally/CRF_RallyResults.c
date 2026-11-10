//------------------------------------------------------------------------------------
// Rally results for coalitiongroup.net: each raced round goes up as one session
// (POST https://api.coalitiongroup.net/api/game/rally/session, contract version 1 - shared with the
// website backend, so field names and meanings must not drift). The session JSON is built by
// CRF_RallyGamemodeComponent.BuildSessionJson; this file has the run record, the JSON helpers and
// the upload. Server only.
//
// The upload is retried a few times with a growing delay, and the same JSON is always written to
// $profile:rally/<sessionId>.json as a backup. The website replaces a session's runs on a repeat
// POST of its sessionId, so a retry of an upload that did arrive is harmless.
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
//! JSON text pieces. The session is written by hand rather than through JsonSaveContext because the
//! contract needs real nulls (missionId, timeMs, carNumber, position) and the save context has no
//! verified way to write one.
class CRF_RallyJson
{
	//------------------------------------------------------------------------------------------------
	//! A JSON string literal, quotes included
	static string Str(string value)
	{
		return "\"" + Escape(value) + "\"";
	}

	//------------------------------------------------------------------------------------------------
	//! Backslash and quote escaped, and every control character (player names can hold them)
	static string Escape(string value)
	{
		string escaped = value;
		escaped.Replace("\\", "\\\\");
		escaped.Replace("\"", "\\\"");

		for (int code = 1; code < 32; code++)
		{
			string control = code.AsciiToString();
			if (!escaped.Contains(control))
				continue;

			string replacement;
			switch (code)
			{
				case 8:  replacement = "\\b"; break;
				case 9:  replacement = "\\t"; break;
				case 10: replacement = "\\n"; break;
				case 12: replacement = "\\f"; break;
				case 13: replacement = "\\r"; break;
				default: replacement = "\\u00" + code.ToString(2, true); break;
			}

			escaped.Replace(control, replacement);
		}

		return escaped;
	}

	//------------------------------------------------------------------------------------------------
	static string IntOrNull(int value, bool present)
	{
		if (!present)
			return "null";

		return value.ToString();
	}

	//------------------------------------------------------------------------------------------------
	static string Bool(bool value)
	{
		if (value)
			return "true";

		return "false";
	}

	//------------------------------------------------------------------------------------------------
	//! Whole numbers without a fraction, the rest to three places
	static string Number(float value)
	{
		int whole = Math.Round(value);
		if (Math.AbsFloat(value - whole) < 0.0005)
			return whole.ToString();

		return string.Format("%1", Math.Round(value * 1000) / 1000);
	}

	//------------------------------------------------------------------------------------------------
	static string IntArray(notnull array<int> values)
	{
		string json = "[";
		foreach (int i, int value : values)
		{
			if (i > 0)
				json += ",";

			json += value.ToString();
		}

		return json + "]";
	}

	//------------------------------------------------------------------------------------------------
	//! 32-bit FNV-1a of the text's bytes as 8 lowercase hex digits. Worked in 16-bit halves so no step
	//! overflows a signed int - the result must be the same on every server and game version, which
	//! string.Hash() doesn't promise.
	static string HashHex(string text)
	{
		// Offset basis 0x811C9DC5
		int hi = 0x811C;
		int lo = 0x9DC5;
		for (int i = 0, length = text.Length(); i < length; i++)
		{
			lo = lo ^ (text.ToAscii(i) & 0xFF);

			// (hi:lo) * 0x01000193 mod 2^32. 0x193 = 403; the 0x0100 << 16 part only adds lo * 256 to hi.
			int loProduct = lo * 403;
			int hiProduct = hi * 403 + lo * 256 + (loProduct >> 16);
			lo = loProduct & 0xFFFF;
			hi = hiProduct & 0xFFFF;
		}

		string hex = hi.ToString(4, true) + lo.ToString(4, true);
		hex.ToLower();
		return hex;
	}
}


//------------------------------------------------------------------------------------------------
//! One crew's run, as a "runs" entry of the session
class CRF_RallyRunResult
{
	string m_sCrewKey;
	string m_sCrewName;
	int m_iCarNumber = -1;     // -1 = null
	string m_sVehicle;
	string m_sStatus;          // finished | dnf_dead | dnf_disconnect | dnf_unfinished | dns
	int m_iTimeMs = -1;        // -1 = null (not finished)
	int m_iPenaltyMs;
	bool m_bJumpStart;
	int m_iCheckpointsPassed;
	ref array<int> m_aSplitsMs = {};
	ref array<int> m_aLapsMs = {};
	int m_iPosition = -1;      // -1 = null (not finished)

	protected ref array<string> m_aMemberGuids = {};
	protected ref array<string> m_aMemberNames = {};
	protected ref array<string> m_aMemberSeats = {};
	protected ref array<int> m_aMemberIds = {};

	//------------------------------------------------------------------------------------------------
	//! seat: "driver", "codriver" or "passenger"
	void AddMember(string guid, string name, string seat, int playerId)
	{
		m_aMemberGuids.Insert(guid);
		m_aMemberNames.Insert(name);
		m_aMemberSeats.Insert(seat);
		m_aMemberIds.Insert(playerId);
	}

	//------------------------------------------------------------------------------------------------
	string ToJson()
	{
		string json = "{\"crewKey\":" + CRF_RallyJson.Str(m_sCrewKey);
		json += ",\"crewName\":" + CRF_RallyJson.Str(m_sCrewName);
		json += ",\"carNumber\":" + CRF_RallyJson.IntOrNull(m_iCarNumber, m_iCarNumber > 0);
		json += ",\"vehicle\":" + CRF_RallyJson.Str(m_sVehicle);
		json += ",\"status\":" + CRF_RallyJson.Str(m_sStatus);
		json += ",\"timeMs\":" + CRF_RallyJson.IntOrNull(m_iTimeMs, m_iTimeMs >= 0);
		json += ",\"penaltyMs\":" + m_iPenaltyMs.ToString();
		json += ",\"jumpStart\":" + CRF_RallyJson.Bool(m_bJumpStart);
		json += ",\"checkpointsPassed\":" + m_iCheckpointsPassed.ToString();
		json += ",\"splitsMs\":" + CRF_RallyJson.IntArray(m_aSplitsMs);
		json += ",\"lapsMs\":" + CRF_RallyJson.IntArray(m_aLapsMs);
		json += ",\"position\":" + CRF_RallyJson.IntOrNull(m_iPosition, m_iPosition > 0);

		json += ",\"members\":[";
		foreach (int i, string guid : m_aMemberGuids)
		{
			if (i > 0)
				json += ",";

			json += "{\"guid\":" + CRF_RallyJson.Str(guid);
			json += ",\"name\":" + CRF_RallyJson.Str(m_aMemberNames[i]);
			json += ",\"seat\":" + CRF_RallyJson.Str(m_aMemberSeats[i]);
			json += ",\"playerId\":" + m_aMemberIds[i].ToString() + "}";
		}

		return json + "]}";
	}
}


//------------------------------------------------------------------------------------------------
//! One session on its way to the website. Each attempt gets a fresh callback and a watchdog (as
//! CRF_AARReviewRequest does), so an attempt the engine never answers still ends; no answer or a
//! server error is retried after FIRST_RETRY_MS, then three times as long each time.
class CRF_RallySessionUpload
{
	// The same base URL as CRF_AARManager, so the same shared RestContext with the same headers
	protected static const string API_BASE_URL = "https://api.coalitiongroup.net/api/game";
	protected static const string SESSION_ENDPOINT = "/rally/session";
	protected static const string BACKUP_DIR = "$profile:rally";
	protected static const int REQUEST_TIMEOUT_S = 15;
	protected static const int WATCHDOG_MS = 25000;
	protected static const int MAX_ATTEMPTS = 4;
	protected static const int FIRST_RETRY_MS = 5000;

	// Uploads outlive the rally component's round reset; finished ones are dropped on the next Send
	protected static ref array<ref CRF_RallySessionUpload> s_aUploads;

	protected string m_sSessionId;
	protected string m_sJson;
	protected int m_iAttempts;
	protected int m_iRetryDelayMs = FIRST_RETRY_MS;
	protected bool m_bDone;
	protected ref RestCallback m_Callback;
	// Callbacks of earlier attempts, kept alive in case the engine still calls them
	protected ref array<ref RestCallback> m_aSpentCallbacks = {};

	//------------------------------------------------------------------------------------------------
	//! Server: back the session up to $profile:rally and post it to the website
	static void Send(string sessionId, string json)
	{
		WriteBackup(sessionId, json);

		if (CRF_WebsiteApiConfig.GetGameServerKey().IsEmpty())
		{
			Print(string.Format("[CRF_Rally] No gameServerKey in $profile:CRF_WebsiteApiConfig.json - results session %1 is only in %2", sessionId, BACKUP_DIR), LogLevel.WARNING);
			return;
		}

		if (!s_aUploads)
			s_aUploads = {};

		for (int i = s_aUploads.Count() - 1; i >= 0; i--)
		{
			if (s_aUploads[i].m_bDone)
				s_aUploads.Remove(i);
		}

		CRF_RallySessionUpload upload = new CRF_RallySessionUpload(sessionId, json);
		s_aUploads.Insert(upload);
		upload.Attempt();
	}

	//------------------------------------------------------------------------------------------------
	//! The website game API context, with the timeout and headers every request uses (the server key
	//! when configured - the mission lookup doesn't need it)
	static RestContext GetApiContext()
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
	protected static void WriteBackup(string sessionId, string json)
	{
		FileIO.MakeDirectory(BACKUP_DIR);
		string path = string.Format("%1/%2.json", BACKUP_DIR, sessionId);
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
		{
			Print(string.Format("[CRF_Rally] Couldn't write the results backup %1", path), LogLevel.ERROR);
			return;
		}

		file.Write(json);
		file.Close();
		Print(string.Format("[CRF_Rally] Results session %1 saved to %2", sessionId, path), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	void CRF_RallySessionUpload(string sessionId, string json)
	{
		m_sSessionId = sessionId;
		m_sJson = json;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_RallySessionUpload()
	{
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(Attempt);
			GetGame().GetCallqueue().Remove(OnWatchdog);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Attempt()
	{
		m_iAttempts++;
		RestContext context = GetApiContext();
		if (!context)
		{
			RetryOrGiveUp("no REST context");
			return;
		}

		if (m_Callback)
			m_aSpentCallbacks.Insert(m_Callback);

		m_Callback = new RestCallback();
		m_Callback.SetOnSuccess(OnAnswer);
		m_Callback.SetOnError(OnAnswer);

		GetGame().GetCallqueue().Remove(OnWatchdog);
		GetGame().GetCallqueue().CallLater(OnWatchdog, WATCHDOG_MS, false);
		context.POST(m_Callback, SESSION_ENDPOINT, m_sJson);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAnswer(RestCallback callback)
	{
		// A late answer to an attempt the watchdog already gave up on
		if (!m_Callback || callback != m_Callback)
			return;

		GetGame().GetCallqueue().Remove(OnWatchdog);
		int httpCode = callback.GetHttpCode();
		if (httpCode == HttpCode.HTTP_CODE_200)
		{
			Print(string.Format("[CRF_Rally] Results session %1 uploaded: %2", m_sSessionId, callback.GetData()), LogLevel.NORMAL);
			m_bDone = true;
			return;
		}

		string reason = string.Format("HTTP %1, %2: %3", httpCode, typename.EnumToString(ERestResult, callback.GetRestResult()), callback.GetData());

		// No answer at all, or the website had a problem: worth another go. A 4xx won't change on a resend.
		if (httpCode <= 0 || httpCode >= 500)
		{
			RetryOrGiveUp(reason);
			return;
		}

		Print(string.Format("[CRF_Rally] Website rejected results session %1 (%2) - it is in %3", m_sSessionId, reason, BACKUP_DIR), LogLevel.ERROR);
		m_bDone = true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnWatchdog()
	{
		if (!m_Callback)
			return;

		m_aSpentCallbacks.Insert(m_Callback);
		m_Callback = null;
		RetryOrGiveUp(string.Format("no callback within %1 ms", WATCHDOG_MS));
	}

	//------------------------------------------------------------------------------------------------
	protected void RetryOrGiveUp(string reason)
	{
		if (m_iAttempts >= MAX_ATTEMPTS)
		{
			Print(string.Format("[CRF_Rally] Giving up on results session %1 after %2 attempt(s) (%3) - it is in %4", m_sSessionId, m_iAttempts, reason, BACKUP_DIR), LogLevel.ERROR);
			m_bDone = true;
			return;
		}

		Print(string.Format("[CRF_Rally] Results session %1 upload failed (%2) - retrying in %3 s", m_sSessionId, reason, m_iRetryDelayMs / 1000), LogLevel.WARNING);
		GetGame().GetCallqueue().CallLater(Attempt, m_iRetryDelayMs, false);
		m_iRetryDelayMs = m_iRetryDelayMs * 3;
	}
}
