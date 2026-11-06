//------------------------------------------------------------------------------------
// CRF_PayloadHUD: the panel at the top of the screen - what the truck is doing, how far along the route it
// is, the clock and how far away it is. Worded for the side reading it. Plus the route minimap at the top
// right (CRF_PayloadMinimap).
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
	protected Widget m_wMinimapPanel;
	protected ref CRF_PayloadMinimap m_Minimap;	//!< kept when the HUD goes down, so it's only built once

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

		UpdateMinimap(payload);
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wMinimapPanel = null;

		if (m_Minimap)
			m_Minimap.SetCanvas(null);
	}

	//------------------------------------------------------------------------------------------------
	//! Built the first time the route is known here, then the truck and the player moved along it
	protected void UpdateMinimap(CRF_PayloadGamemodeComponent payload)
	{
		bool show = payload.ShowMinimap() && payload.IsRouteReady();
		if (m_wMinimapPanel)
			m_wMinimapPanel.SetVisible(show);

		if (!show || !m_wMinimapPanel)
			return;

		if (!m_Minimap)
			m_Minimap = new CRF_PayloadMinimap();

		if (!m_Minimap.IsBuilt())
		{
			m_Minimap.Build(payload.GetRoute(), payload.GetRouteDistances(), payload.GetCheckpointDistances());
			m_Minimap.SetCanvas(CanvasWidget.Cast(m_wMinimapPanel.FindAnyWidget("MinimapCanvas")));
		}

		// Vehicle position/heading when seated, character otherwise
		IEntity local = SCR_PlayerController.GetLocalControlledEntity();
		bool showPlayer = local != null && !COA_EntityHelper.IsSpectator(local);
		if (showPlayer)
		{
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(local);
			showPlayer = damageManager == null || damageManager.GetState() != EDamageState.DESTROYED;
		}
		vector playerPos;
		float playerYaw;
		if (showPlayer)
		{
			IEntity tracked = local;
			if (local.GetParent())
				tracked = local.GetParent();

			playerPos = tracked.GetOrigin();
			playerYaw = tracked.GetAngles()[1];
		}

		m_Minimap.Update(payload.GetShownDistance(), payload.GetCheckpointsReached(), showPlayer, playerPos, playerYaw);
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

		m_wMinimapPanel = m_wRoot.FindAnyWidget("MinimapPanel");
		if (m_wMinimapPanel && m_Minimap)
			m_Minimap.SetCanvas(CanvasWidget.Cast(m_wMinimapPanel.FindAnyWidget("MinimapCanvas")));

		m_bUrgent = false;
		return true;
	}
}
