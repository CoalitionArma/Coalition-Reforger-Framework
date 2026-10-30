//------------------------------------------------------------------------------------
// CRF_HoldoutHUD: the defenders' panel at the bottom right, above the weapon and ammo display - which wave
// it is, what's happening (the countdown to the next wave), and how much money the player has to spend at
// the crate.
//
// Styled like the Lobby's killed-by card (COA_KilledByCard.layout): dark card, accent edge, a muted caption
// over a large title and a lighter detail line. It fades in when it appears and when the phase changes, the
// countdown turns to the accent colour for its last seconds, and money changes flash the amount and show
// the difference beside it.
//
// Client only, built from what CRF_HoldoutGamemodeComponent already replicates (the phase, wave and
// countdown, and everyone's money), so it costs nothing on the network. Shown to the defending faction -
// spectating defenders included - and out of the way while a menu (map, inventory...) is open.
//------------------------------------------------------------------------------------
class CRF_HoldoutHUD
{
	protected static const ResourceName LAYOUT = "{6D16DF6DD77E16B0}UI/layouts/HUD/Holdout/CRF_HoldoutHUD.layout";

	// The killed-by card's palette, as linear colours (the same values as in the layout)
	protected static const ref Color TITLE = new Color(0.8632, 0.8879, 0.9301, 1);
	protected static const ref Color DETAIL = new Color(0.6038, 0.6584, 0.7454, 1);
	protected static const ref Color MONEY = new Color(0.807, 0.402, 0.065, 1);
	protected static const ref Color GAIN = new Color(0.107, 0.552, 0.216, 1);

	protected static const int URGENT_SECONDS = 10;		//!< the countdown turns to the accent colour
	protected static const float PANEL_FADE_SPEED = 4;
	protected static const float MONEY_FLASH_SPEED = 2.5;
	protected static const float DELTA_FADE_SPEED = 1.5;
	protected static const float DELTA_HOLD_SECONDS = 1.2;

	protected Widget m_wRoot;
	protected Widget m_wPanel;
	protected TextWidget m_wKicker;
	protected TextWidget m_wTitle;
	protected TextWidget m_wDetail;
	protected TextWidget m_wMoney;
	protected TextWidget m_wMoneyDelta;

	protected bool m_bShown;
	protected int m_iShownPhase = -1;
	protected int m_iShownMoney = -1;
	protected bool m_bUrgent;

	//------------------------------------------------------------------------------------------------
	void Update(CRF_HoldoutGamemodeComponent holdout)
	{
		if (!holdout || !ShouldShow(holdout))
		{
			if (m_wRoot)
				m_wRoot.SetVisible(false);
			m_bShown = false;
			return;
		}

		if (!m_wRoot && !Create())
			return;

		m_wRoot.SetVisible(true);

		CRF_EHoldoutPhase phase = holdout.GetPhase();

		// Fade in on appearing, and again when the phase changes
		if (!m_bShown || phase != m_iShownPhase)
			FadeIn();

		m_bShown = true;
		m_iShownPhase = phase;

		UpdatePhase(holdout, phase);
		UpdateMoney(holdout.GetMoney(SCR_PlayerController.GetLocalPlayerId()));
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_bShown = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdatePhase(CRF_HoldoutGamemodeComponent holdout, CRF_EHoldoutPhase phase)
	{
		int wave = holdout.GetWave();
		int secondsLeft = holdout.GetPhaseSecondsLeft();
		bool counting;

		switch (phase)
		{
			case CRF_EHoldoutPhase.PREP:
				m_wKicker.SetText("PREPARE DEFENSES");
				m_wTitle.SetText(string.Format("WAVE %1", wave + 1));
				m_wDetail.SetText("Starts in " + CRF_SpecialsHelper.FormatSeconds(secondsLeft));
				counting = true;
				break;

			case CRF_EHoldoutPhase.WAVE:
				m_wKicker.SetText("UNDER ATTACK");
				m_wTitle.SetText(string.Format("WAVE %1", wave));
				m_wDetail.SetText("Hold the line");
				break;

			case CRF_EHoldoutPhase.LULL:
				m_wKicker.SetText(string.Format("WAVE %1 HELD", wave));
				m_wTitle.SetText(string.Format("WAVE %1", wave + 1));
				m_wDetail.SetText("Starts in " + CRF_SpecialsHelper.FormatSeconds(secondsLeft));
				counting = true;
				break;

			case CRF_EHoldoutPhase.ENDED:
				m_wKicker.SetText("HOLDOUT OVER");
				m_wTitle.SetText("OVERRUN");
				m_wDetail.SetText("Nobody is left");
				break;
		}

		// The last seconds before a wave stand out
		bool urgent = counting && secondsLeft > 0 && secondsLeft <= URGENT_SECONDS;
		if (urgent == m_bUrgent)
			return;

		m_bUrgent = urgent;
		if (urgent)
			m_wDetail.SetColor(COA_PhaseUI.ACCENT);
		else
			m_wDetail.SetColor(DETAIL);
	}

	//------------------------------------------------------------------------------------------------
	//! The amount, and when it changes a flash and the difference beside it (green in, red out)
	protected void UpdateMoney(int money)
	{
		if (money == m_iShownMoney)
			return;

		int change = money - m_iShownMoney;
		bool first = m_iShownMoney < 0;
		m_iShownMoney = money;
		m_wMoney.SetText("$" + FormatMoney(money));

		if (first)
			return;

		Color flash = TITLE;
		Color deltaColor = GAIN;
		string deltaText = "+" + FormatMoney(change);
		if (change < 0)
		{
			flash = COA_PhaseUI.ACCENT;
			deltaColor = COA_PhaseUI.ACCENT;
			deltaText = "-" + FormatMoney(-change);
		}

		m_wMoney.SetColor(flash);
		WidgetAnimationColor settle = AnimateWidget.Color(m_wMoney, MONEY, MONEY_FLASH_SPEED);
		if (settle)
			settle.SetCurve(EAnimationCurve.EASE_OUT_CUBIC);

		m_wMoneyDelta.SetText(deltaText);
		m_wMoneyDelta.SetColor(deltaColor);
		m_wMoneyDelta.SetOpacity(1);
		WidgetAnimationOpacity fade = AnimateWidget.Opacity(m_wMoneyDelta, 0, DELTA_FADE_SPEED);
		if (fade)
		{
			fade.SetDelay(DELTA_HOLD_SECONDS);
			fade.SetCurve(EAnimationCurve.EASE_IN_CUBIC);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void FadeIn()
	{
		m_wPanel.SetOpacity(0);
		WidgetAnimationOpacity fade = AnimateWidget.Opacity(m_wPanel, 1, PANEL_FADE_SPEED);
		if (fade)
			fade.SetCurve(EAnimationCurve.EASE_OUT_CUBIC);
	}

	//------------------------------------------------------------------------------------------------
	//! Defenders (alive or spectating), with no menu in front of the game other than the spectator's
	protected bool ShouldShow(CRF_HoldoutGamemodeComponent holdout)
	{
		if (CRF_SpecialsHelper.GetLocalFactionKey() != holdout.GetDefenderFactionKey())
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

		m_wPanel = m_wRoot.FindAnyWidget("Panel");
		m_wKicker = TextWidget.Cast(m_wRoot.FindAnyWidget("Kicker"));
		m_wTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("Title"));
		m_wDetail = TextWidget.Cast(m_wRoot.FindAnyWidget("Detail"));
		m_wMoney = TextWidget.Cast(m_wRoot.FindAnyWidget("MoneyText"));
		m_wMoneyDelta = TextWidget.Cast(m_wRoot.FindAnyWidget("MoneyDelta"));
		if (!m_wPanel || !m_wKicker || !m_wTitle || !m_wDetail || !m_wMoney || !m_wMoneyDelta)
		{
			Destroy();
			return false;
		}

		m_wMoneyDelta.SetOpacity(0);
		m_iShownMoney = -1;
		m_iShownPhase = -1;
		m_bUrgent = false;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! 12500 -> "12,500"
	protected string FormatMoney(int amount)
	{
		string digits = Math.AbsInt(amount).ToString();
		string grouped;
		int length = digits.Length();
		for (int i = 0; i < length; i++)
		{
			if (i > 0 && (length - i) % 3 == 0)
				grouped += ",";
			grouped += digits.Get(i);
		}

		if (amount < 0)
			return "-" + grouped;

		return grouped;
	}
}
