// CRF_AreaTimerDisplay.c
//
// HUD display for the area majority timer. Reads state replicated by
// COA_RplBroadcastManager (set each server tick via BroadcastAreaTimerUpdate)
// and renders a top-centre panel showing:
//   - Zone label  ("THE HILL")
//   - Win countdown (MM:SS – shifts yellow then red as it approaches zero)
//   - Status line  ("{Faction} capturing..." / "CONTESTED" / "{Faction} WIN!")
//
// The panel is hidden when no area timer is active in the mission, during
// safestart, in spectator, and when the HUD is toggled off.

class CRF_AreaTimerDisplay : SCR_InfoDisplayExtended
{
	//---------------------------------------------------------------------------------------------
	// Widget references (resolved lazily on first valid frame)
	//---------------------------------------------------------------------------------------------

	protected ImageWidget m_wBG;
	protected TextWidget  m_wZoneLabel;
	protected TextWidget  m_wCountdown;
	protected TextWidget  m_wStatusText;

	protected float m_fUpdateBuffer = 0;
	protected bool  m_bWidgetsInit  = false;

	//---------------------------------------------------------------------------------------------
	// Lifecycle
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override protected void DisplayInit(IEntity owner)
	{
		super.DisplayInit(owner);
		if (RplSession.Mode() == RplMode.Dedicated)
			return;
	}

	//------------------------------------------------------------------------------------------------
	override protected void DisplayUpdate(IEntity owner, float timeSlice)
	{
		super.DisplayUpdate(owner, timeSlice);

		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		if (!GetGame().GetWorld().GetWorldTime() || !SCR_PlayerController.GetLocalControlledEntity())
			return;

		// Late-init: resolve widget references once the layout is loaded
		if (!m_bWidgetsInit)
		{
			m_wBG         = ImageWidget.Cast(m_wRoot.FindWidget("BG"));
			m_wZoneLabel  = TextWidget.Cast(m_wRoot.FindWidget("ZoneLabel"));
			m_wCountdown  = TextWidget.Cast(m_wRoot.FindWidget("Countdown"));
			m_wStatusText = TextWidget.Cast(m_wRoot.FindWidget("StatusText"));

			if (m_wBG && m_wZoneLabel && m_wCountdown && m_wStatusText)
				m_bWidgetsInit = true;

			return;
		}

		// Update at 1 Hz – no need to run every frame
		m_fUpdateBuffer = m_fUpdateBuffer + timeSlice;
		if (m_fUpdateBuffer < 1.0)
			return;
		m_fUpdateBuffer = 0;

		UpdateDisplay();
	}

	//---------------------------------------------------------------------------------------------
	// Display Logic
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void UpdateDisplay()
	{
		COA_RplBroadcastManager bm = COA_RplBroadcastManager.GetInstance();

		// --- Visibility guards ---
		if (!bm
			|| !COA_Gamemode.GetInstance()
			|| COA_Gamemode.GetInstance().m_GamemodeState != COA_EGamemodeState.GAME
			|| !COA_SafestartManager.GetInstance()
			|| COA_SafestartManager.GetInstance().GetSafestartStatus()
			|| COA_EntityHelper.IsSpectator()
			|| !COA_PlayerControllerManager.GetInstance()
			|| !COA_PlayerControllerManager.GetInstance().m_bHUDVisible)
		{
			SetPanelVisible(false);
			return;
		}

		CRF_EAreaTimerState state   = bm.m_eAreaTimerState;
		string              faction = bm.m_sAreaTimerFaction;

		// No area timer in this mission, or no one has touched the zone yet
		if (bm.m_sAreaTimerZoneLabel == "")
		{
			SetPanelVisible(false);
			return;
		}

		// Hide when idle with no prior captor (zone untouched / fully reset)
		if (state == CRF_EAreaTimerState.IDLE && faction == "")
		{
			SetPanelVisible(false);
			return;
		}

		SetPanelVisible(true);

		m_wZoneLabel.SetText(bm.m_sAreaTimerZoneLabel);

		SCR_FactionManager fm = SCR_FactionManager.Cast(GetGame().GetFactionManager());

		switch (state)
		{
			//------------------------------------------------------------------
			case CRF_EAreaTimerState.IDLE:
			{
				// A faction previously held it but lost majority – countdown paused
				m_wStatusText.SetText("CONTESTED");
				m_wStatusText.SetColorInt(ARGB(255, 180, 180, 180));
				m_wCountdown.SetText(FormatCountdown(bm.m_iAreaTimerCountdown));
				m_wCountdown.SetColorInt(ARGB(255, 160, 160, 160));
				break;
			}

			//------------------------------------------------------------------
			case CRF_EAreaTimerState.CAPTURING:
			{
				string factionName = faction;
				bool   hasColor    = false;
				Color  factionColor;

				if (fm && faction != "")
				{
					Faction f = fm.GetFactionByKey(faction);
					if (f)
					{
						factionName  = f.GetFactionName();
						factionColor = f.GetFactionColor();
						hasColor     = true;
					}
				}

				m_wStatusText.SetText(factionName + " capturing...");
				if (hasColor)
					m_wStatusText.SetColor(factionColor);
				else
					m_wStatusText.SetColorInt(ARGB(255, 255, 255, 255));

				int remaining = bm.m_iAreaTimerCountdown;
				m_wCountdown.SetText(FormatCountdown(remaining));

				if (remaining <= 30)
					m_wCountdown.SetColorInt(ARGB(255, 220, 55, 55));
				else if (remaining <= 60)
					m_wCountdown.SetColorInt(ARGB(255, 230, 200, 0));
				else
					m_wCountdown.SetColorInt(ARGB(255, 255, 255, 255));

				break;
			}

			//------------------------------------------------------------------
			case CRF_EAreaTimerState.WIN:
			{
				string winnerName = faction;
				if (fm && faction != "")
				{
					Faction f = fm.GetFactionByKey(faction);
					if (f) winnerName = f.GetFactionName();
				}

				m_wStatusText.SetText(winnerName + " WIN!");
				m_wStatusText.SetColorInt(ARGB(255, 255, 255, 255));
				m_wCountdown.SetText("00:00");
				m_wCountdown.SetColorInt(ARGB(255, 220, 55, 55));
				break;
			}
		}
	}

	//---------------------------------------------------------------------------------------------
	// Helpers
	//---------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Returns "MM:SS", dropping the "00:" hour prefix when hours == 0.
	protected string FormatCountdown(int seconds)
	{
		string full = SCR_FormatHelper.FormatTime(seconds);
		array<string> parts = new array<string>();
		full.Split(":", parts, false);

		if (parts.Count() == 3 && parts[0] == "00")
			return string.Format("%1:%2", parts[1], parts[2]);

		return full;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetPanelVisible(bool visible)
	{
		float opacity = 0;
		if (visible)
			opacity = 1;

		if (m_wBG)         m_wBG.SetOpacity(opacity);
		if (m_wZoneLabel)  m_wZoneLabel.SetOpacity(opacity);
		if (m_wCountdown)  m_wCountdown.SetOpacity(opacity);
		if (m_wStatusText) m_wStatusText.SetOpacity(opacity);
	}
}
