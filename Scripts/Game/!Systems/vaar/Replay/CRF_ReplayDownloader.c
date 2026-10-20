//------------------------------------------------------------------------------------------------
//! Talks to the website: the list of recorded missions, and downloading one into $profile:vaar/replays.
//! A recording is fetched a minute of frames per request (GET /aar/<id>/replay/<chunk>) so no single
//! response is big, and appended to the file as it arrives; an interrupted download resumes.
class CRF_ReplayDownloader
{
	protected static ref CRF_ReplayDownloader s_Instance;

	protected static const int MAX_RETRIES = 3;
	protected static const int TIMEOUT_S = 30;

	//! (array<ref CRF_ReplayListEntry> replays, string error)
	ref ScriptInvoker m_OnList = new ScriptInvoker();
	//! (int missionId, float progress 0-1, string status)
	ref ScriptInvoker m_OnProgress = new ScriptInvoker();
	//! (int missionId, bool success, string message)
	ref ScriptInvoker m_OnFinished = new ScriptInvoker();

	protected RestContext m_Context;
	protected ref RestCallback m_ListCallback;
	protected ref RestCallback m_DownloadCallback;

	protected int m_iMissionId;
	protected ref CRF_ReplayMeta m_Meta;
	protected int m_iRetries;
	protected bool m_bDownloading;

	//------------------------------------------------------------------------------------------------
	static CRF_ReplayDownloader Get()
	{
		if (!s_Instance)
			s_Instance = new CRF_ReplayDownloader();

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	protected RestContext GetContext()
	{
		if (!m_Context)
		{
			m_Context = GetGame().GetRestApi().GetContext(CRF_Replay.API_BASE);
			if (m_Context)
				m_Context.SetTimeout(TIMEOUT_S);
		}

		return m_Context;
	}

	//------------------------------------------------------------------------------------------------
	bool IsDownloading()
	{
		return m_bDownloading;
	}

	//------------------------------------------------------------------------------------------------
	int GetDownloadingId()
	{
		if (!m_bDownloading)
			return 0;

		return m_iMissionId;
	}

	// ── Mission list ─────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	void FetchList()
	{
		RestContext context = GetContext();
		if (!context)
		{
			m_OnList.Invoke(new array<ref CRF_ReplayListEntry>(), "No connection to the website");
			return;
		}

		m_ListCallback = new RestCallback();
		m_ListCallback.SetOnSuccess(OnListSuccess);
		m_ListCallback.SetOnError(OnListError);
		context.GET(m_ListCallback, "/aar/replays?limit=40");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnListSuccess(RestCallback callback)
	{
		array<ref CRF_ReplayListEntry> replays = {};
		JsonLoadContext context = new JsonLoadContext(false);
		if (!context.LoadFromString(callback.GetData()) || !context.ReadValue("replays", replays))
		{
			m_OnList.Invoke(replays, "The website sent a list this version can't read");
			return;
		}

		m_OnList.Invoke(replays, "");
	}

	//------------------------------------------------------------------------------------------------
	protected void OnListError(RestCallback callback)
	{
		m_OnList.Invoke(new array<ref CRF_ReplayListEntry>(), string.Format("Couldn't reach the website (%1)", callback.GetHttpCode()));
	}

	// ── Download ─────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! Downloads a recording, or carries on with one an earlier download left unfinished
	bool Download(int missionId)
	{
		if (m_bDownloading)
			return false;

		RestContext context = GetContext();
		if (!context)
			return false;

		CRF_ReplayLibrary.EnsureDirectory();
		m_iMissionId = missionId;
		m_iRetries = 0;
		m_bDownloading = true;

		m_DownloadCallback = new RestCallback();
		m_DownloadCallback.SetOnSuccess(OnInfoSuccess);
		m_DownloadCallback.SetOnError(OnDownloadError);
		context.GET(m_DownloadCallback, string.Format("/aar/%1/replay", missionId));
		m_OnProgress.Invoke(missionId, 0, "Contacting the website…");
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnInfoSuccess(RestCallback callback)
	{
		CRF_ReplayInfo info = new CRF_ReplayInfo();
		JsonLoadContext context = new JsonLoadContext(false);
		if (!context.LoadFromString(callback.GetData()) || !ReadInfoFields(context, info))
		{
			Finish(false, "The website sent replay details this version can't read");
			return;
		}

		if (info.chunks <= 0 || info.header.IsEmpty())
		{
			Finish(false, "This mission has no recording");
			return;
		}

		// Resume a download that stopped part way, otherwise start the file with the header line
		CRF_ReplayMeta meta = CRF_ReplayLibrary.LoadMeta(m_iMissionId);
		bool resume = meta && !meta.complete && meta.chunks == info.chunks && meta.chunksDone > 0 && FileIO.FileExists(CRF_Replay.ReplayPath(m_iMissionId));
		if (!resume)
		{
			meta = new CRF_ReplayMeta();
			meta.id = m_iMissionId;
			meta.chunksDone = 0;
			FileHandle file = FileIO.OpenFile(CRF_Replay.ReplayPath(m_iMissionId), FileMode.WRITE);
			if (!file)
			{
				Finish(false, "Couldn't write to " + CRF_Replay.DIR);
				return;
			}

			file.Write(info.header + "\n");
			file.Close();
		}

		meta.name = info.name;
		meta.terrainKey = info.terrainKey;
		meta.frames = info.frames;
		meta.chunks = info.chunks;
		meta.complete = false;
		m_Meta = meta;
		CRF_ReplayLibrary.SaveMeta(meta);
		RequestChunk();
	}

	//------------------------------------------------------------------------------------------------
	//! GET /aar/<id>/replay, field by field
	protected bool ReadInfoFields(JsonLoadContext context, CRF_ReplayInfo info)
	{
		bool ok = context.ReadValue("header", info.header);
		context.ReadValue("id", info.id);
		context.ReadValue("name", info.name);
		context.ReadValue("terrainKey", info.terrainKey);
		context.ReadValue("frames", info.frames);
		context.ReadValue("chunkFrames", info.chunkFrames);
		context.ReadValue("chunks", info.chunks);
		return ok;
	}

	//------------------------------------------------------------------------------------------------
	protected void RequestChunk()
	{
		m_DownloadCallback = new RestCallback();
		m_DownloadCallback.SetOnSuccess(OnChunkSuccess);
		m_DownloadCallback.SetOnError(OnDownloadError);
		GetContext().GET(m_DownloadCallback, string.Format("/aar/%1/replay/%2", m_iMissionId, m_Meta.chunksDone));
		m_OnProgress.Invoke(m_iMissionId, m_Meta.chunksDone / (float)Math.Max(1, m_Meta.chunks), string.Format("Downloading %1 of %2 min", m_Meta.chunksDone, m_Meta.chunks));
	}

	//------------------------------------------------------------------------------------------------
	protected void OnChunkSuccess(RestCallback callback)
	{
		FileHandle file = FileIO.OpenFile(CRF_Replay.ReplayPath(m_iMissionId), FileMode.APPEND);
		if (!file)
		{
			Finish(false, "Couldn't write to " + CRF_Replay.DIR);
			return;
		}

		file.Write(callback.GetData());
		file.Close();

		m_iRetries = 0;
		m_Meta.chunksDone++;
		if (m_Meta.chunksDone >= m_Meta.chunks)
		{
			m_Meta.complete = true;
			CRF_ReplayLibrary.SaveMeta(m_Meta);
			Finish(true, "Downloaded");
			return;
		}

		CRF_ReplayLibrary.SaveMeta(m_Meta);
		RequestChunk();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDownloadError(RestCallback callback)
	{
		if (m_Meta && m_iRetries < MAX_RETRIES)
		{
			m_iRetries++;
			GetGame().GetCallqueue().CallLater(RequestChunk, 1500 * m_iRetries);
			return;
		}

		Finish(false, string.Format("Download failed (%1) - try again to resume", callback.GetHttpCode()));
	}

	//------------------------------------------------------------------------------------------------
	protected void Finish(bool success, string message)
	{
		int missionId = m_iMissionId;
		m_bDownloading = false;
		m_Meta = null;
		m_OnProgress.Invoke(missionId, 1, message);
		m_OnFinished.Invoke(missionId, success, message);
	}
}
