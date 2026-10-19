//------------------------------------------------------------------------------------------------
//! One mission in the list: what's known about it locally and on the website, and its row widgets
class CRF_ReplayRow
{
	int m_iId;
	string m_sName;
	string m_sDetail;
	ref CRF_ReplayMeta m_Meta;			// downloaded (perhaps partly)
	ref CRF_ReplayListEntry m_Remote;	// on the website

	CRF_ReplayBrowserMenu m_Menu;
	Widget m_wRoot;
	TextWidget m_wName;
	TextWidget m_wDetail;
	TextWidget m_wStatus;
	TextWidget m_wPrimaryText;
	Widget m_wDeleteFrame;
	Widget m_wProgressFill;

	//------------------------------------------------------------------------------------------------
	void Bind(Widget root, CRF_ReplayBrowserMenu menu)
	{
		m_wRoot = root;
		m_Menu = menu;
		m_wName = TextWidget.Cast(root.FindAnyWidget("Name"));
		m_wDetail = TextWidget.Cast(root.FindAnyWidget("Detail"));
		m_wStatus = TextWidget.Cast(root.FindAnyWidget("RowStatus"));
		m_wDeleteFrame = root.FindAnyWidget("DeleteButtonFrame");
		m_wProgressFill = root.FindAnyWidget("ProgressFill");

		Widget primaryFrame = root.FindAnyWidget("PrimaryButtonFrame");
		if (primaryFrame)
			m_wPrimaryText = TextWidget.Cast(primaryFrame.FindAnyWidget("ButtonText"));

		CRF_ReplayHUDMenu.GetButtonClick(root, "PrimaryButton").Insert(OnPrimary);
		CRF_ReplayHUDMenu.GetButtonClick(root, "DeleteButton").Insert(OnDelete);
	}

	//------------------------------------------------------------------------------------------------
	void Refresh()
	{
		if (!m_wRoot)
			return;

		if (m_wName)
			m_wName.SetText(m_sName);

		if (m_wDetail)
			m_wDetail.SetText(m_sDetail);

		CRF_ReplayDownloader downloader = CRF_ReplayDownloader.Get();
		bool downloading = downloader.GetDownloadingId() == m_iId;
		bool complete = m_Meta && m_Meta.complete && CRF_ReplayLibrary.IsComplete(m_iId);

		string primary = "DOWNLOAD";
		if (complete)
			primary = "WATCH";
		else if (downloading)
			primary = "...";
		else if (m_Meta && m_Meta.chunksDone > 0)
			primary = "RESUME";

		if (m_wPrimaryText)
			m_wPrimaryText.SetText(primary);

		if (m_wDeleteFrame)
			m_wDeleteFrame.SetVisible(m_Meta != null && !downloading);

		float progress;
		if (complete)
			progress = 1;
		else if (m_Meta && m_Meta.chunks > 0)
			progress = m_Meta.chunksDone / (float)m_Meta.chunks;

		if (m_wProgressFill)
		{
			FrameSlot.SetAnchorMax(m_wProgressFill, progress, 1);
			m_wProgressFill.SetVisible(progress > 0 && !complete);
		}

		if (m_wStatus && !downloading)
		{
			if (complete)
				m_wStatus.SetText("ON THIS PC");
			else if (m_Meta && m_Meta.chunksDone > 0)
				m_wStatus.SetText(((int)(progress * 100)).ToString() + "% DOWNLOADED");
			else if (m_Remote)
				m_wStatus.SetText(CRF_Replay.SizeLabel(m_Remote.sizeBytes));
			else
				m_wStatus.SetText("");
		}
	}

	//------------------------------------------------------------------------------------------------
	void SetStatus(string status)
	{
		if (m_wStatus)
			m_wStatus.SetText(status);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPrimary()
	{
		if (m_Menu)
			m_Menu.OnRowPrimary(this);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDelete()
	{
		if (m_Menu)
			m_Menu.OnRowDelete(this);
	}
}

//------------------------------------------------------------------------------------------------
//! Main menu > REPLAYS: missions recorded on COALITION servers, downloaded ones first. Download one,
//! then WATCH loads its terrain and plays it back locally.
class CRF_ReplayBrowserMenu : ChimeraMenuBase
{
	protected static const ResourceName ROW_LAYOUT = "{6A7E2B0000000003}UI/layouts/Menus/Replay/CRF_ReplayRow.layout";

	protected Widget m_wRoot;
	protected Widget m_wList;
	protected TextWidget m_wStatus;
	protected InputManager m_InputManager;

	protected ref array<ref CRF_ReplayRow> m_aRows = {};
	protected ref map<int, CRF_ReplayRow> m_mRowsById = new map<int, CRF_ReplayRow>();
	protected ref array<ref CRF_ReplayListEntry> m_aRemote = {};
	protected bool m_bListLoaded;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_wRoot = GetRootWidget();
		m_wList = m_wRoot.FindAnyWidget("ReplayList");
		m_wStatus = TextWidget.Cast(m_wRoot.FindAnyWidget("Status"));
		m_InputManager = GetGame().GetInputManager();

		CRF_ReplayHUDMenu.GetButtonClick(m_wRoot, "RefreshButton").Insert(FetchList);
		CRF_ReplayHUDMenu.GetButtonClick(m_wRoot, "CloseButton").Insert(OnBack);
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnBack);

		CRF_ReplayDownloader downloader = CRF_ReplayDownloader.Get();
		downloader.m_OnList.Insert(OnList);
		downloader.m_OnProgress.Insert(OnProgress);
		downloader.m_OnFinished.Insert(OnFinished);

		Rebuild();
		FetchList();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		CRF_ReplayDownloader downloader = CRF_ReplayDownloader.Get();
		downloader.m_OnList.Remove(OnList);
		downloader.m_OnProgress.Remove(OnProgress);
		downloader.m_OnFinished.Remove(OnFinished);

		if (m_InputManager)
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnBack);

		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		GetGame().GetCallqueue().CallLater(Close, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void FetchList()
	{
		SetStatus("Fetching recordings from coalitiongroup.net…");
		CRF_ReplayDownloader.Get().FetchList();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnList(array<ref CRF_ReplayListEntry> replays, string error)
	{
		m_bListLoaded = error.IsEmpty();
		if (m_bListLoaded)
		{
			m_aRemote.Clear();
			foreach (CRF_ReplayListEntry entry : replays)
			{
				m_aRemote.Insert(entry);
			}
		}

		Rebuild();
		if (!error.IsEmpty())
			SetStatus(error + " - downloaded replays still play");
		else
			SetStatus(string.Format("%1 recordings on the website. Downloads are kept in %2", m_aRemote.Count(), CRF_Replay.DIR));
	}

	//------------------------------------------------------------------------------------------------
	//! Downloaded replays first (newest first), then the website's list
	protected void Rebuild()
	{
		foreach (CRF_ReplayRow row : m_aRows)
		{
			if (row.m_wRoot)
				row.m_wRoot.RemoveFromHierarchy();
		}

		m_aRows.Clear();
		m_mRowsById.Clear();

		foreach (CRF_ReplayMeta meta : CRF_ReplayLibrary.ListLocal())
		{
			CRF_ReplayRow row = AddRow(meta.id);
			row.m_Meta = meta;
			row.m_sName = meta.name;
			row.m_sDetail = Detail(meta.terrain, meta.terrainKey, meta.playedAt, "");
		}

		foreach (CRF_ReplayListEntry entry : m_aRemote)
		{
			CRF_ReplayRow row = m_mRowsById.Get(entry.id);
			if (!row)
				row = AddRow(entry.id);

			row.m_Remote = entry;
			row.m_sName = entry.name;
			row.m_sDetail = Detail(entry.terrain, entry.terrainKey, entry.playedAt, entry.author);

			// Fill in what an older download didn't save
			if (row.m_Meta && (row.m_Meta.playedAt.IsEmpty() || row.m_Meta.terrain.IsEmpty()))
			{
				row.m_Meta.playedAt = entry.playedAt;
				row.m_Meta.terrain = entry.terrain;
				CRF_ReplayLibrary.SaveMeta(row.m_Meta);
			}
		}

		if (m_aRows.IsEmpty())
			SetStatus("No recordings yet");

		foreach (CRF_ReplayRow row : m_aRows)
		{
			if (!m_wList)
				break;

			Widget widget = GetGame().GetWorkspace().CreateWidgets(ROW_LAYOUT, m_wList);
			if (!widget)
				continue;

			row.Bind(widget, this);
			row.Refresh();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayRow AddRow(int id)
	{
		CRF_ReplayRow row = new CRF_ReplayRow();
		row.m_iId = id;
		m_aRows.Insert(row);
		m_mRowsById.Set(id, row);
		return row;
	}

	//------------------------------------------------------------------------------------------------
	protected string Detail(string terrain, string terrainKey, string playedAt, string author)
	{
		array<string> parts = {};
		if (!terrain.IsEmpty())
			parts.Insert(terrain);
		else if (!terrainKey.IsEmpty())
			parts.Insert(terrainKey);

		// ISO date, minute precision
		if (playedAt.Length() >= 16)
		{
			string date = playedAt.Substring(0, 16);
			date.Replace("T", " ");
			parts.Insert(date);
		}

		if (!author.IsEmpty())
			parts.Insert("by " + author);

		string detail;
		foreach (int i, string part : parts)
		{
			if (i > 0)
				detail += "  ·  ";

			detail += part;
		}

		return detail;
	}

	//------------------------------------------------------------------------------------------------
	void OnRowPrimary(CRF_ReplayRow row)
	{
		if (CRF_ReplayLibrary.IsComplete(row.m_iId))
		{
			string error = CRF_ReplayLauncher.Launch(row.m_iId);
			if (!error.IsEmpty())
				SetStatus(error);

			return;
		}

		CRF_ReplayDownloader downloader = CRF_ReplayDownloader.Get();
		if (downloader.IsDownloading())
		{
			SetStatus("One download at a time - wait for the current one to finish");
			return;
		}

		if (!downloader.Download(row.m_iId))
			SetStatus("Couldn't start the download");

		row.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	void OnRowDelete(CRF_ReplayRow row)
	{
		if (CRF_ReplayDownloader.Get().GetDownloadingId() == row.m_iId)
			return;

		CRF_ReplayLibrary.Delete(row.m_iId);
		if (!row.m_Remote)
		{
			Rebuild();
			return;
		}

		row.m_Meta = null;
		row.Refresh();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnProgress(int missionId, float progress, string status)
	{
		CRF_ReplayRow row = m_mRowsById.Get(missionId);
		if (!row)
			return;

		row.m_Meta = CRF_ReplayLibrary.LoadMeta(missionId);
		row.Refresh();
		status.ToUpper();
		row.SetStatus(status);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnFinished(int missionId, bool success, string message)
	{
		CRF_ReplayRow row = m_mRowsById.Get(missionId);
		if (row)
		{
			row.m_Meta = CRF_ReplayLibrary.LoadMeta(missionId);
			if (row.m_Meta && row.m_Remote && row.m_Meta.terrain.IsEmpty())
			{
				row.m_Meta.terrain = row.m_Remote.terrain;
				row.m_Meta.playedAt = row.m_Remote.playedAt;
				CRF_ReplayLibrary.SaveMeta(row.m_Meta);
			}

			row.Refresh();
			if (!success)
			{
				string upper = message;
				upper.ToUpper();
				row.SetStatus(upper);
			}
		}

		if (!success)
			SetStatus(message);
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string text)
	{
		if (m_wStatus)
			m_wStatus.SetText(text);
	}
}

//------------------------------------------------------------------------------------------------
//! A REPLAYS button on the main menu
modded class MainMenuUI
{
	protected static const ResourceName CRF_REPLAY_BUTTON_LAYOUT = "{6A7E2B0000000004}UI/layouts/Menus/Replay/CRF_ReplayMainMenuButton.layout";
	protected Widget m_wCRF_ReplayButton;

	//------------------------------------------------------------------------------------------------
	protected override void OnMenuOpen()
	{
		super.OnMenuOpen();

		Widget root = GetRootWidget();
		if (!root)
			return;

		m_wCRF_ReplayButton = GetGame().GetWorkspace().CreateWidgets(CRF_REPLAY_BUTTON_LAYOUT, root);
		if (!m_wCRF_ReplayButton)
			return;

		// Fill the menu so the button's own anchors place it
		FrameSlot.SetAnchorMin(m_wCRF_ReplayButton, 0, 0);
		FrameSlot.SetAnchorMax(m_wCRF_ReplayButton, 1, 1);
		FrameSlot.SetOffsets(m_wCRF_ReplayButton, 0, 0, 0, 0);
		m_wCRF_ReplayButton.SetZOrder(100);

		CRF_ReplayHUDMenu.GetButtonClick(m_wCRF_ReplayButton, "ReplaysButton").Insert(CRF_OnReplaysClicked);
	}

	//------------------------------------------------------------------------------------------------
	protected void CRF_OnReplaysClicked()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_ReplayBrowser);
	}
}
