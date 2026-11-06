//------------------------------------------------------------------------------------
// CRF_EvacuationHUD: the panel at the top of the screen - what the site is doing, how many loads are out
// (the bar), what's left and the clock. Worded for the side reading it.
//
// Styled like CRF_PayloadHUD. Client only, built from what CRF_EvacuationGamemodeComponent already
// replicates.
//
// It stays out of the way: it's up while the map is open, fades in for a few seconds when something
// happens (the round starts, a truck loads, a load gets out or is destroyed, the site is contested,
// falling or empty, someone reaches the extraction point, the round ends), and stays up once the round is
// nearly over, which includes the whole withdrawal. Hidden behind any other
// menu (inventory...), except the spectator's.
//------------------------------------------------------------------------------------
class CRF_EvacuationHUD
{
	protected static const ResourceName LAYOUT = "{6ACE7A0100000000}UI/layouts/HUD/Evacuation/CRF_EvacuationHUD.layout";

	protected static const int URGENT_SECONDS = 300;	//!< the clock turns to the accent colour
	protected static const int NEARLY_OVER_SECONDS = 300;	//!< from here on the panel stays up
	protected static const float EVENT_SHOW_MS = 8000;	//!< how long an event brings the panel up for
	protected static const float FADE_SPEED = 2.5;		//!< opacity per second: a 0.4 s fade

	protected Widget m_wRoot;
	protected TextWidget m_wKicker;
	protected TextWidget m_wTitle;
	protected TextWidget m_wDetail;
	protected ProgressBarWidget m_wBar;

	protected bool m_bUrgent;
	protected bool m_bShown;

	// What the panel last saw, to spot events
	protected bool m_bHaveSnapshot;
	protected int m_iLastState;
	protected int m_iLastStock;
	protected int m_iLastInTransit;
	protected int m_iLastDelivered;
	protected int m_iLastLost;
	protected int m_iLastWithdrawn;
	protected float m_fShowUntil;

	//------------------------------------------------------------------------------------------------
	void Update(CRF_EvacuationGamemodeComponent evacuation)
	{
		if (evacuation)
			CheckForEvents(evacuation);

		if (!evacuation || !CanShow())
		{
			Hide();
			return;
		}

		if (!m_wRoot && !Create())
			return;

		SetShown(IsMapOpen() || IsNearlyOver(evacuation) || GetNow() < m_fShowUntil);

		bool evacuating = CRF_SpecialsHelper.GetLocalFactionKey() == evacuation.GetEvacuatingFactionKey();
		if (evacuating)
			m_wKicker.SetText("EVACUATE THE SUPPLIES");
		else
			m_wKicker.SetText("STOP THE EVACUATION");

		m_wTitle.SetText(GetTitle(evacuation, evacuating));
		UpdateDetail(evacuation);

		m_wBar.SetMax(1);
		if (evacuation.GetEvacuationState() == CRF_EEvacuationState.EXTRACTING || (evacuation.GetEvacuationState() == CRF_EEvacuationState.ENDED && evacuation.GetWithdrawNeeded() > 0))
		{
			float withdrawNeeded = Math.Max(1, evacuation.GetWithdrawNeeded());
			m_wBar.SetCurrent(evacuation.GetWithdrawn() / withdrawNeeded);
		}
		else
		{
			float needed = Math.Max(1, evacuation.GetLoadsToWin());
			m_wBar.SetCurrent(evacuation.GetDelivered() / needed);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Something the players should see happened: bring the panel up for a few seconds
	protected void CheckForEvents(CRF_EvacuationGamemodeComponent evacuation)
	{
		int state = evacuation.GetEvacuationState();
		int stock = evacuation.GetStock();
		int inTransit = evacuation.GetInTransit();
		int delivered = evacuation.GetDelivered();
		int lost = evacuation.GetLost();
		int withdrawn = evacuation.GetWithdrawn();

		bool changed = m_bHaveSnapshot && (state != m_iLastState || stock != m_iLastStock || inTransit != m_iLastInTransit || delivered != m_iLastDelivered || lost != m_iLastLost || withdrawn != m_iLastWithdrawn);
		if (changed && state != CRF_EEvacuationState.WAITING)
			m_fShowUntil = GetNow() + EVENT_SHOW_MS;

		m_bHaveSnapshot = true;
		m_iLastState = state;
		m_iLastStock = stock;
		m_iLastInTransit = inTransit;
		m_iLastDelivered = delivered;
		m_iLastLost = lost;
		m_iLastWithdrawn = withdrawn;
	}

	//------------------------------------------------------------------------------------------------
	//! The last few minutes, the site falling, one load from either side winning, or the result
	protected bool IsNearlyOver(CRF_EvacuationGamemodeComponent evacuation)
	{
		CRF_EEvacuationState state = evacuation.GetEvacuationState();
		if (state == CRF_EEvacuationState.WAITING)
			return false;

		if (state == CRF_EEvacuationState.ENDED || state == CRF_EEvacuationState.CAPTURING || state == CRF_EEvacuationState.EXTRACTING)
			return true;

		if (evacuation.GetSecondsLeft() <= NEARLY_OVER_SECONDS)
			return true;

		int needed = evacuation.GetLoadsToWin();
		if (evacuation.GetDelivered() >= needed - 1)
			return true;

		// One more destroyed load and the evacuation can't succeed
		return evacuation.GetDelivered() + evacuation.GetInTransit() + evacuation.GetStock() - 1 < needed;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetShown(bool shown)
	{
		if (shown == m_bShown && m_wRoot.IsVisible())
			return;

		m_bShown = shown;
		m_wRoot.SetVisible(true);
		if (shown)
			AnimateWidget.Opacity(m_wRoot, 1, FADE_SPEED);
		else
			AnimateWidget.Opacity(m_wRoot, 0, FADE_SPEED);
	}

	//------------------------------------------------------------------------------------------------
	//! Straight away, no fade: a menu has opened over the game
	protected void Hide()
	{
		if (!m_wRoot)
			return;

		AnimateWidget.StopAnimation(m_wRoot, WidgetAnimationOpacity);
		m_wRoot.SetOpacity(0);
		m_wRoot.SetVisible(false);
		m_bShown = false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsMapOpen()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return false;

		return mapEntity.IsOpen();
	}

	//------------------------------------------------------------------------------------------------
	protected float GetNow()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetWorldTime();
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetTitle(CRF_EvacuationGamemodeComponent evacuation, bool evacuating)
	{
		int unload = evacuation.GetUnloadPercent();

		switch (evacuation.GetEvacuationState())
		{
			case CRF_EEvacuationState.WAITING:
				return "WAITING";

			case CRF_EEvacuationState.CAPTURING:
				return "SITE FALLING  " + evacuation.GetCapturePercent().ToString() + "%";

			case CRF_EEvacuationState.CONTESTED:
				return "SITE CONTESTED";

			case CRF_EEvacuationState.LOADING:
				if (evacuating)
					return "LOADING  " + evacuation.GetLoadPercent().ToString() + "%";
				return "TRUCK LOADING";

			case CRF_EEvacuationState.EMPTY:
				if (unload >= 0)
					return "UNLOADING  " + unload.ToString() + "%";
				return "SITE EMPTY";

			case CRF_EEvacuationState.IDLE:
				if (unload >= 0)
					return "UNLOADING  " + unload.ToString() + "%";
				if (evacuating)
					return "BRING A TRUCK TO THE BAY";
				return "SITE HELD";

			case CRF_EEvacuationState.EXTRACTING:
				if (evacuating)
					return string.Format("GET OUT  %1/%2", evacuation.GetWithdrawn(), evacuation.GetWithdrawNeeded());
				return "THEY'RE PULLING OUT";

			case CRF_EEvacuationState.ENDED:
				if (evacuation.DidEvacuatorsWin())
					return "EVACUATED";
				return "EVACUATION FAILED";
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateDetail(CRF_EvacuationGamemodeComponent evacuation)
	{
		int secondsLeft = evacuation.GetSecondsLeft();
		CRF_EEvacuationState state = evacuation.GetEvacuationState();

		string detail;
		if (state == CRF_EEvacuationState.EXTRACTING || (state == CRF_EEvacuationState.ENDED && evacuation.GetWithdrawNeeded() > 0))
		{
			detail = string.Format("Loads out %1/%2  |  People out %3/%4", evacuation.GetDelivered(), evacuation.GetLoadsToWin(), evacuation.GetWithdrawn(), evacuation.GetWithdrawNeeded());
		}
		else
		{
			detail = string.Format("Out %1/%2  |  Site %3  |  Road %4", evacuation.GetDelivered(), evacuation.GetLoadsToWin(), evacuation.GetStock(), evacuation.GetInTransit());
			if (evacuation.GetLost() > 0)
				detail += string.Format("  |  Lost %1", evacuation.GetLost());
		}

		detail += "  |  " + CRF_SpecialsHelper.FormatSeconds(secondsLeft);
		m_wDetail.SetText(detail);

		bool live = state != CRF_EEvacuationState.WAITING && state != CRF_EEvacuationState.ENDED;
		bool urgent = live && (secondsLeft <= URGENT_SECONDS || state == CRF_EEvacuationState.CAPTURING || state == CRF_EEvacuationState.EXTRACTING);
		if (urgent == m_bUrgent)
			return;

		m_bUrgent = urgent;
		if (urgent)
			m_wDetail.SetColor(COA_PhaseUI.ACCENT);
		else
			m_wDetail.SetColor(new Color(0.6038, 0.6584, 0.7454, 1));
	}

	//------------------------------------------------------------------------------------------------
	//! Slotted players (alive or spectating), with no menu in front of the game other than the map or the spectator's
	protected bool CanShow()
	{
		if (CRF_SpecialsHelper.GetLocalFactionKey().IsEmpty())
			return false;

		if (IsMapOpen())
			return true;

		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		return topMenu == null || COA_SpectatorMenu.Cast(topMenu) != null;
	}

	//------------------------------------------------------------------------------------------------
	protected bool Create()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		m_wRoot = workspace.CreateWidgets(LAYOUT);
		if (!m_wRoot)
			return false;

		m_wKicker = TextWidget.Cast(m_wRoot.FindAnyWidget("Kicker"));
		m_wTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		m_wDetail = TextWidget.Cast(m_wRoot.FindAnyWidget("Detail"));
		m_wBar = ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("Bar"));
		if (!m_wKicker || !m_wTitle || !m_wDetail || !m_wBar)
		{
			Destroy();
			return false;
		}

		m_bUrgent = false;
		m_bShown = false;
		m_wRoot.SetOpacity(0);
		return true;
	}
}
