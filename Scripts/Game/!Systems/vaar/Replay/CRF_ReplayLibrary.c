//------------------------------------------------------------------------------------------------
//! Replays saved on this PC ($profile:vaar/replays)
class CRF_ReplayLibrary
{
	//------------------------------------------------------------------------------------------------
	static void EnsureDirectory()
	{
		FileIO.MakeDirectory("$profile:vaar");
		FileIO.MakeDirectory(CRF_Replay.DIR);
	}

	//------------------------------------------------------------------------------------------------
	//! Every downloaded (or partly downloaded) replay, newest mission first
	static array<ref CRF_ReplayMeta> ListLocal()
	{
		array<ref CRF_ReplayMeta> metas = {};
		array<string> files = SCR_FileIOHelper.FindFiles(CRF_Replay.DIR, ".json");
		foreach (string file : files)
		{
			if (!file.EndsWith(".meta.json"))
				continue;

			string path = file;
			if (!path.StartsWith("$"))
				path = CRF_Replay.DIR + "/" + FilePath.StripPath(file);

			CRF_ReplayMeta meta = LoadMetaFile(path);
			if (meta && meta.id > 0)
				metas.Insert(meta);
		}

		// Newest first (ISO dates sort as text), then by mission ID
		for (int i = 1; i < metas.Count(); i++)
		{
			for (int j = i; j > 0 && IsNewer(metas[j], metas[j - 1]); j--)
				metas.SwapItems(j, j - 1);
		}

		return metas;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsNewer(CRF_ReplayMeta a, CRF_ReplayMeta b)
	{
		if (a.playedAt != b.playedAt)
			return a.playedAt.Compare(b.playedAt) > 0;

		return a.id > b.id;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_ReplayMeta LoadMeta(int missionId)
	{
		return LoadMetaFile(CRF_Replay.MetaPath(missionId));
	}

	//------------------------------------------------------------------------------------------------
	protected static CRF_ReplayMeta LoadMetaFile(string path)
	{
		if (!FileIO.FileExists(path))
			return null;

		JsonLoadContext context = new JsonLoadContext(false);
		if (!context.LoadFromFile(path))
			return null;

		CRF_ReplayMeta meta = new CRF_ReplayMeta();
		if (!context.ReadValue("meta", meta))
			return null;

		return meta;
	}

	//------------------------------------------------------------------------------------------------
	static bool SaveMeta(notnull CRF_ReplayMeta meta)
	{
		EnsureDirectory();
		JsonSaveContext context = new JsonSaveContext();
		context.WriteValue("meta", meta);
		return context.SaveToFile(CRF_Replay.MetaPath(meta.id));
	}

	//------------------------------------------------------------------------------------------------
	static bool IsComplete(int missionId)
	{
		CRF_ReplayMeta meta = LoadMeta(missionId);
		return meta && meta.complete && FileIO.FileExists(CRF_Replay.ReplayPath(missionId));
	}

	//------------------------------------------------------------------------------------------------
	static void Delete(int missionId)
	{
		FileIO.DeleteFile(CRF_Replay.ReplayPath(missionId));
		FileIO.DeleteFile(CRF_Replay.MetaPath(missionId));
	}
}
