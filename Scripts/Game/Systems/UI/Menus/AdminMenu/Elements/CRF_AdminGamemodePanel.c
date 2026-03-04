/**
 * CRF_AdminGamemodePanel
 *
 * Modular widget component for the Gamemode Settings panel inside the Admin
 * Menu. Handles the per-second live-update pass for:
 *   - Mission timer text ("CurrentGameTime0")
 *   - Per-faction gear-set name titles ("<FACTION>ListTitle")
 *   - Per-faction ticket counters ("<faction>TicketCount")
 *   - Respawn-wave toggle button label & colour
 *   - Respawn-enabled toggle button label & colour
 *
 * The component is handed the already-created m_wMenuContent widget (the one
 * loaded via CreateWidgets in InitializeGamemodeMenu) through Init() and then
 * Update() is called every second from CRF_AdminMenu.GamemodeMenuUpdate().
 *
 * Usage:
 *   // After CreateWidgets returns m_wMenuContent:
 *   Widget panelRoot = m_wMenuContent.FindAnyWidget("AdminGamemodePanel");
 *   m_AdminGamemodePanel = CRF_AdminGamemodePanel.Cast(
 *       panelRoot.FindHandler(CRF_AdminGamemodePanel));
 *   if (m_AdminGamemodePanel)
 *       m_AdminGamemodePanel.Init(m_wMenuContent);
 *
 *   // In the per-second update:
 *   if (m_AdminGamemodePanel)
 *       m_AdminGamemodePanel.Update();
 */
class CRF_AdminGamemodePanel : SCR_ScriptedWidgetComponent
{
	// Cached section roots (set by Init)
	protected Widget m_wGameTimer;
	protected Widget m_wTicketCounters;
	protected Widget m_wGearSets;
	protected Widget m_wRespawnWaveButton;
	protected Widget m_wRespawnEnabledButton;

	// Faction keys iterated in fixed order
	protected static const ref array<string> FACTIONS = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		// Widget resolution is deferred to Init() because the panel layout is
		// created dynamically after HandlerAttached fires.
	}

	/**
	 * Resolves all child widget references from the dynamically-created menu
	 * content root. Call once immediately after CreateWidgets returns.
	 *
	 * @param menuContent  The root widget returned by CreateWidgets for the
	 *                     Gamemode Settings layout.
	 */
	void Init(Widget menuContent)
	{
		if (!menuContent)
			return;

		m_wGameTimer           = menuContent.FindAnyWidget("GameTimer");
		m_wTicketCounters      = menuContent.FindAnyWidget("Tickets");
		m_wGearSets            = menuContent.FindAnyWidget("GearSets");
		m_wRespawnWaveButton   = menuContent.FindAnyWidget("RespawnWaveButton");
		m_wRespawnEnabledButton = menuContent.FindAnyWidget("EnableRespawnButton");
	}

	/**
	 * Refreshes all live-updating elements inside the Gamemode Settings panel.
	 * Call once per second while the Gamemode Settings tab is open.
	 */
	void Update()
	{
		UpdateTimer();
		UpdateGearSetTitles();
		UpdateTicketCounters();
		UpdateRespawnWaveButton();
		UpdateRespawnEnabledButton();
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTimer()
	{
		if (!m_wGameTimer)
			return;

		string serverTime = CRF_GameTimerManager.GetInstance().GetServerWorldTime();
		TextWidget timerText = TextWidget.Cast(m_wGameTimer.FindWidget("CurrentGameTime0"));
		if (timerText)
			timerText.SetText(serverTime);
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateGearSetTitles()
	{
		if (!m_wGearSets)
			return;

		CRF_Gamemode gm = CRF_Gamemode.GetInstance();
		if (!gm)
			return;

		foreach (string faction : FACTIONS)
		{
			string resourceName;
			switch (faction)
			{
				case "BLUFOR":    resourceName = gm.m_rBLUFORCurrentGearScript;    break;
				case "OPFOR":     resourceName = gm.m_rOPFORCurrentGearScript;     break;
				case "INDFOR":    resourceName = gm.m_rINDFORCurrentGearScript;    break;
				case "CIV":       resourceName = gm.m_rCIVILIANCurrentGearScript;  break;
			}

			if (resourceName.IsEmpty())
				continue;

			// Strip path prefix and extension to get a human-readable name
			string name = resourceName.Substring(
				resourceName.LastIndexOf("/") + 1,
				resourceName.LastIndexOf(".") - resourceName.LastIndexOf("/") - 1);
			name.Replace("CRF_GS_", "");

			TextWidget titleWidget = TextWidget.Cast(
				m_wGearSets.FindAnyWidget(string.Format("%1ListTitle", faction)));
			if (titleWidget)
				titleWidget.SetText(name);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTicketCounters()
	{
		if (!m_wTicketCounters)
			return;

		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		if (!rm)
			return;

		// Widget naming convention: <Faction>TicketCount (capitalised first letter only)
		SetTicketText("BluforTicketCount", rm.GetFactionTickets("BLUFOR"));
		SetTicketText("OpforTicketCount",  rm.GetFactionTickets("OPFOR"));
		SetTicketText("IndforTicketCount", rm.GetFactionTickets("INDFOR"));
		SetTicketText("CivTicketCount",    rm.GetFactionTickets("CIV"));
	}

	//------------------------------------------------------------------------------------------------
	protected void SetTicketText(string widgetName, int count)
	{
		TextWidget w = TextWidget.Cast(m_wTicketCounters.FindWidget(widgetName));
		if (w)
			w.SetText(count.ToString());
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRespawnWaveButton()
	{
		if (!m_wRespawnWaveButton)
			return;

		TextWidget label = TextWidget.Cast(m_wRespawnWaveButton.FindWidget("ActionButtonText"));
		if (!label)
			return;

		bool enabled = CRF_RespawnManager.GetInstance().m_bCurrentWaveRespawn;
		if (enabled)
		{
			label.SetText("Respawn Wave Enabled");
			label.SetColorInt(Color.GREEN);
		}
		else
		{
			label.SetText("Respawn Wave Disabled");
			label.SetColorInt(Color.RED);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateRespawnEnabledButton()
	{
		if (!m_wRespawnEnabledButton)
			return;

		TextWidget label = TextWidget.Cast(m_wRespawnEnabledButton.FindWidget("ActionButtonText"));
		if (!label)
			return;

		bool enabled = CRF_RespawnManager.GetInstance().m_bCurrentRespawnEnabled;
		if (enabled)
		{
			label.SetText("Respawns Enabled");
			label.SetColorInt(Color.GREEN);
		}
		else
		{
			label.SetText("Respawns Disabled");
			label.SetColorInt(Color.RED);
		}
	}
}
