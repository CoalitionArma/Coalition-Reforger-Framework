//------------------------------------------------------------------------------------
// CRF_TankBattleMenu: the tank picker
//
// Built with workspace.CreateWidgets like CRF_RallyGarageMenu, so it needs no menu preset.
// CRF_TankBattleGamemodeComponent opens it while the local player is alive on foot without a tank,
// refreshes it four times a second (how many of each tank the side has out) and closes it once they
// have one. Clicking a tank asks the server for it; the server checks the limit and spawns it.
//------------------------------------------------------------------------------------
class CRF_TankBattleMenuEntryButton : ScriptedWidgetComponent
{
	int m_iOption;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(this);

		return false;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_TankBattleMenu
{
	protected static const ResourceName MENU_LAYOUT  = "{7C2E5A91B3D04F61}UI/layouts/Menus/TankBattle/CRF_TankBattleMenu.layout";
	protected static const ResourceName ENTRY_LAYOUT = "{7C2E5A91B3D04F62}UI/layouts/Menus/TankBattle/CRF_TankBattleMenuEntry.layout";

	protected static const int REQUEST_WAIT_MS = 3000;	//!< "Requesting..." this long before the row can be clicked again

	protected static ref CRF_TankBattleMenu s_Instance;

	protected Widget m_wRoot;
	protected int m_iSide = -1;
	protected ref array<Widget> m_aEntries = {};
	protected ref array<int> m_aEntryOptions = {};	//!< the vehicle option (index in the side's list) of each row
	protected ref COA_MenuPolish m_UIPolish;
	protected int m_iRequested = -1;
	protected int m_iRequestedAtMs;

	protected static const ref Color LABEL_COLOR = Color.FromSRGBA(230, 235, 255, 255);
	protected static const ref Color FULL_COLOR  = Color.FromSRGBA(120, 125, 140, 255);
	protected static const ref Color COUNT_COLOR = Color.FromSRGBA(255, 200, 90, 255);

	//------------------------------------------------------------------------------------------------
	static bool IsOpen()
	{
		return s_Instance != null;
	}

	//------------------------------------------------------------------------------------------------
	static void Open(notnull CRF_TankBattleGamemodeComponent mode, int side)
	{
		if (s_Instance)
			s_Instance.Close();

		s_Instance = new CRF_TankBattleMenu();
		s_Instance.Create(mode, side);
	}

	//------------------------------------------------------------------------------------------------
	static void CloseMenu()
	{
		if (s_Instance)
			s_Instance.Close();
	}

	//------------------------------------------------------------------------------------------------
	//! How many of each tank the side has out, and whether a row can be picked
	static void Refresh(notnull CRF_TankBattleGamemodeComponent mode, int side)
	{
		if (!s_Instance)
			return;

		if (side != s_Instance.m_iSide)
		{
			Open(mode, side);
			return;
		}

		s_Instance.UpdateEntries(mode);
	}

	//------------------------------------------------------------------------------------------------
	protected void Create(notnull CRF_TankBattleGamemodeComponent mode, int side)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		CRF_TankBattleSide sideData = mode.GetSide(side);
		if (!workspace || !sideData || !sideData.m_aTanks)
		{
			s_Instance = null;
			return;
		}

		m_wRoot = workspace.CreateWidgets(MENU_LAYOUT);
		if (!m_wRoot)
		{
			s_Instance = null;
			return;
		}

		m_iSide = side;

		TextWidget subTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("SubTitle"));
		if (subTitle)
			subTitle.SetText(string.Format("%1: pick your tank. You drive it and fire the main gun.", CRF_SpecialsHelper.GetFactionName(sideData.m_sFactionKey)));

		Widget list = m_wRoot.FindAnyWidget("TankList");
		if (list)
		{
			// Only what the loaded mods can spawn (CRF_TankBattleTank.IsAvailable)
			foreach (int i, CRF_TankBattleTank tank : sideData.m_aTanks)
			{
				if (tank && tank.IsAvailable())
					CreateEntry(workspace, list, i, tank);
			}
		}

		UpdateEntries(mode);

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, ActionClose);

		m_UIPolish = new COA_MenuPolish(m_wRoot, true);

		// The cursor, and button clicks, need these contexts every frame (see CRF_PropHuntTransformMenu)
		GetGame().GetCallqueue().CallLater(KeepCursorActive, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateEntry(WorkspaceWidget workspace, Widget list, int option, CRF_TankBattleTank tank)
	{
		Widget entry = workspace.CreateWidgets(ENTRY_LAYOUT, list);
		if (!entry)
			return;

		m_aEntries.Insert(entry);
		m_aEntryOptions.Insert(option);

		TextWidget label = TextWidget.Cast(entry.FindAnyWidget("EntryLabel"));
		if (label && tank)
			label.SetText(tank.m_sName);

		TextWidget description = TextWidget.Cast(entry.FindAnyWidget("EntryDescription"));
		if (description && tank)
			description.SetText(tank.m_sDescription);

		ButtonWidget button = ButtonWidget.Cast(entry.FindAnyWidget("EntryButton"));
		if (!button)
			return;

		CRF_TankBattleMenuEntryButton handler = CRF_TankBattleMenuEntryButton.Cast(button.FindHandler(CRF_TankBattleMenuEntryButton));
		if (!handler)
			return;

		handler.m_iOption = option;
		handler.m_OnClicked.Insert(OnEntryClicked);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateEntries(notnull CRF_TankBattleGamemodeComponent mode)
	{
		int now = GetGame().GetWorld().GetWorldTime();
		bool waiting = m_iRequested >= 0 && now - m_iRequestedAtMs < REQUEST_WAIT_MS;

		foreach (int e, Widget entry : m_aEntries)
		{
			int i = m_aEntryOptions[e];
			CRF_TankBattleTank tank = mode.GetTankOption(m_iSide, i);
			if (!tank)
				continue;

			int inUse = mode.CountTanks(m_iSide, i);
			bool full = tank.m_iLimit > 0 && inUse >= tank.m_iLimit;

			string countText;
			if (waiting && i == m_iRequested)
				countText = "SPAWNING...";
			else if (full)
				countText = string.Format("FULL  %1/%2", inUse, tank.m_iLimit);
			else if (tank.m_iLimit > 0)
				countText = string.Format("%1/%2", inUse, tank.m_iLimit);
			else if (inUse > 0)
				countText = string.Format("%1 out", inUse);

			TextWidget count = TextWidget.Cast(entry.FindAnyWidget("CountText"));
			if (count)
				count.SetText(countText);

			TextWidget label = TextWidget.Cast(entry.FindAnyWidget("EntryLabel"));
			if (label && full)
				label.SetColor(FULL_COLOR);
			else if (label)
				label.SetColor(LABEL_COLOR);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnEntryClicked(CRF_TankBattleMenuEntryButton handler)
	{
		if (!handler)
			return;

		int now = GetGame().GetWorld().GetWorldTime();
		if (m_iRequested >= 0 && now - m_iRequestedAtMs < REQUEST_WAIT_MS)
			return;

		m_iRequested = handler.m_iOption;
		m_iRequestedAtMs = now;

		CRF_TankBattleGamemodeComponent mode = CRF_TankBattleGamemodeComponent.GetInstance();
		if (mode)
			UpdateEntries(mode);

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestTankBattleTank(handler.m_iOption);
	}

	//------------------------------------------------------------------------------------------------
	protected void ActionClose(float value, EActionTrigger reason)
	{
		CRF_TankBattleGamemodeComponent mode = CRF_TankBattleGamemodeComponent.GetInstance();
		if (mode)
			mode.SnoozeMenu();

		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void KeepCursorActive()
	{
		if (!m_wRoot)
		{
			GetGame().GetCallqueue().Remove(KeepCursorActive);
			return;
		}

		GetGame().GetInputManager().ActivateContext("MenuContext");
		GetGame().GetInputManager().ActivateContext("InteractableDialogContext");
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(KeepCursorActive);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, ActionClose);

		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}

		m_aEntries.Clear();
		m_aEntryOptions.Clear();
		if (s_Instance == this)
			s_Instance = null;
	}
}
