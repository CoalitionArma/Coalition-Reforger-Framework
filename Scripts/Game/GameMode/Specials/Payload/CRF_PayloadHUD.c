//------------------------------------------------------------------------------------
// CRF_PayloadHUD: the panel at the top of the screen - what the truck is doing, how far along the route it
// is, the clock and how far away it is. Worded for the side reading it.
//
// Styled like CRF_HoldoutHUD. Client only, built from what CRF_PayloadGamemodeComponent already replicates,
// so it costs nothing on the network. Out of the way while a menu (map, inventory...) is open.
//------------------------------------------------------------------------------------
class CRF_PayloadHUD
{
	protected static const ResourceName LAYOUT = "{5F3A91C27D4E6B10}UI/layouts/HUD/Payload/CRF_PayloadHUD.layout";

	protected static const int URGENT_SECONDS = 60;		//!< the clock turns to the accent colour

	protected Widget m_wRoot;
	protected TextWidget m_wKicker;
	protected TextWidget m_wTitle;
	protected TextWidget m_wDetail;
	protected ProgressBarWidget m_wBar;

	protected bool m_bUrgent;

	//------------------------------------------------------------------------------------------------
	void Update(CRF_PayloadGamemodeComponent payload)
	{
		if (!payload || !ShouldShow(payload))
		{
			if (m_wRoot)
				m_wRoot.SetVisible(false);
			return;
		}

		if (!m_wRoot && !Create())
			return;

		m_wRoot.SetVisible(true);

		bool attacking = CRF_SpecialsHelper.GetLocalFactionKey() == payload.GetAttackerFactionKey();
		if (attacking)
			m_wKicker.SetText("ESCORT THE TRUCK");
		else
			m_wKicker.SetText("STOP THE TRUCK");

		UpdateTitle(payload);
		UpdateDetail(payload);

		m_wBar.SetMax(1);
		m_wBar.SetCurrent(payload.GetProgress());
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTitle(CRF_PayloadGamemodeComponent payload)
	{
		switch (payload.GetPayloadState())
		{
			case CRF_EPayloadState.WAITING:
				m_wTitle.SetText("WAITING");
				break;

			case CRF_EPayloadState.STOPPED:
				m_wTitle.SetText("STOPPED");
				break;

			case CRF_EPayloadState.MOVING:
				m_wTitle.SetText("MOVING");
				break;

			case CRF_EPayloadState.CONTESTED:
				m_wTitle.SetText("CONTESTED");
				break;

			case CRF_EPayloadState.ROLLING_BACK:
				m_wTitle.SetText("ROLLING BACK");
				break;

			case CRF_EPayloadState.ENDED:
				if (payload.DidAttackersWin())
					m_wTitle.SetText("IT GOT THROUGH");
				else
					m_wTitle.SetText("ROUND OVER");
				break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateDetail(CRF_PayloadGamemodeComponent payload)
	{
		bool overtime = payload.IsOvertime();
		int secondsLeft = payload.GetSecondsLeft();

		string clock = CRF_SpecialsHelper.FormatSeconds(secondsLeft);
		if (overtime)
			clock = "OVERTIME";

		string detail = string.Format("%1  |  Checkpoint %2/%3", clock, payload.GetCheckpointsReached(), payload.GetCheckpointCount());

		IEntity local = SCR_PlayerController.GetLocalControlledEntity();
		if (local)
		{
			int metres = Math.Round(vector.DistanceXZ(local.GetOrigin(), payload.GetTruckPosition()));
			detail += string.Format("  |  %1 m away", metres);
		}

		m_wDetail.SetText(detail);

		bool urgent = overtime || (payload.GetPayloadState() != CRF_EPayloadState.WAITING && secondsLeft <= URGENT_SECONDS);
		if (urgent == m_bUrgent)
			return;

		m_bUrgent = urgent;
		if (urgent)
			m_wDetail.SetColor(COA_PhaseUI.ACCENT);
		else
			m_wDetail.SetColor(new Color(0.6038, 0.6584, 0.7454, 1));
	}

	//------------------------------------------------------------------------------------------------
	//! Slotted players (alive or spectating), with no menu in front of the game other than the spectator's
	protected bool ShouldShow(CRF_PayloadGamemodeComponent payload)
	{
		if (CRF_SpecialsHelper.GetLocalFactionKey().IsEmpty())
			return false;

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
		return true;
	}
}
