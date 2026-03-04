/**
 * CRF_FactionRatioDisplay
 *
 * Modular widget component that renders the faction ratio configuration strip
 * in the Slotting Menu. It reads the configured faction-one / faction-two
 * ratio values from CRF_Gamemode and shows or hides the strip depending on
 * whether valid ratios are set.
 *
 * It also exposes UpdateRatioCalculation() which should be called every frame
 * so the "X : Y" live-player split stays current.
 *
 * Expected child widget names (relative to the root widget):
 *   "RatioBox1"        — EditBoxWidget  (faction 1 ratio integer input)
 *   "RatioBox1Text"    — TextWidget     (faction 1 key label)
 *   "RatioBox1Image"   — ImageWidget    (faction 1 color swatch)
 *   "RatioBox1IntImage"— ImageWidget    (decorative)
 *   "RatioBox2"        — EditBoxWidget  (faction 2 ratio integer input)
 *   "RatioBox2Text"    — TextWidget     (faction 2 key label)
 *   "RatioBox2Image"   — ImageWidget    (faction 2 color swatch)
 *   "RatioBox2IntImage"— ImageWidget    (decorative)
 *   "FinalImage"       — ImageWidget    (separator)
 *   "Final"            — TextWidget     (live "X : Y" result)
 *
 * Usage:
 *   Widget ratioRoot = m_wRoot.FindAnyWidget("RatioDisplay");
 *   m_RatioDisplay = CRF_FactionRatioDisplay.Cast(
 *       ratioRoot.FindHandler(CRF_FactionRatioDisplay));
 *   m_RatioDisplay.Populate();                           // once on open
 *   m_RatioDisplay.UpdateRatioCalculation(playerCount);  // every frame
 */
class CRF_FactionRatioDisplay : SCR_ScriptedWidgetComponent
{
	protected EditBoxWidget m_wRatioBox1;
	protected TextWidget    m_wRatioBox1Text;
	protected ImageWidget   m_wRatioBox1Image;
	protected ImageWidget   m_wRatioBox1IntImage;

	protected EditBoxWidget m_wRatioBox2;
	protected TextWidget    m_wRatioBox2Text;
	protected ImageWidget   m_wRatioBox2Image;
	protected ImageWidget   m_wRatioBox2IntImage;

	protected ImageWidget   m_wFinalImage;
	protected TextWidget    m_wFinal;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wRatioBox1        = EditBoxWidget.Cast(w.FindAnyWidget("RatioBox1"));
		m_wRatioBox1Text    = TextWidget.Cast(w.FindAnyWidget("RatioBox1Text"));
		m_wRatioBox1Image   = ImageWidget.Cast(w.FindAnyWidget("RatioBox1Image"));
		m_wRatioBox1IntImage= ImageWidget.Cast(w.FindAnyWidget("RatioBox1IntImage"));

		m_wRatioBox2        = EditBoxWidget.Cast(w.FindAnyWidget("RatioBox2"));
		m_wRatioBox2Text    = TextWidget.Cast(w.FindAnyWidget("RatioBox2Text"));
		m_wRatioBox2Image   = ImageWidget.Cast(w.FindAnyWidget("RatioBox2Image"));
		m_wRatioBox2IntImage= ImageWidget.Cast(w.FindAnyWidget("RatioBox2IntImage"));

		m_wFinalImage = ImageWidget.Cast(w.FindAnyWidget("FinalImage"));
		m_wFinal      = TextWidget.Cast(w.FindAnyWidget("Final"));
	}

	/**
	 * Reads ratio configuration from CRF_Gamemode and populates the strip.
	 * Hides the entire strip when either ratio is missing or zero.
	 * Call once when the menu opens.
	 */
	void Populate()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
		{
			HideRatioDisplay();
			return;
		}

		bool valid = true;

		if (gamemode.m_iFactionOneRatio > 0 && !gamemode.m_sFactionOneKey.IsEmpty())
		{
			if (m_wRatioBox1)
				m_wRatioBox1.SetText(gamemode.m_iFactionOneRatio.ToString());
			if (m_wRatioBox1Text)
				m_wRatioBox1Text.SetText(gamemode.m_sFactionOneKey);
			if (m_wRatioBox1Image)
				m_wRatioBox1Image.SetColor(GetFactionColor(gamemode.m_sFactionOneKey));
		}
		else
		{
			valid = false;
		}

		if (gamemode.m_iFactionTwoRatio > 0 && !gamemode.m_sFactionTwoKey.IsEmpty())
		{
			if (m_wRatioBox2)
				m_wRatioBox2.SetText(gamemode.m_iFactionTwoRatio.ToString());
			if (m_wRatioBox2Text)
				m_wRatioBox2Text.SetText(gamemode.m_sFactionTwoKey);
			if (m_wRatioBox2Image)
				m_wRatioBox2Image.SetColor(GetFactionColor(gamemode.m_sFactionTwoKey));
		}
		else
		{
			valid = false;
		}

		if (!valid)
			HideRatioDisplay();
	}

	/**
	 * Recalculates and displays the live "X : Y" player split based on the
	 * ratio values currently entered in the edit boxes.
	 * Call every frame from the owning menu's OnMenuUpdate.
	 *
	 * @param playerCount Current total player count.
	 */
	void UpdateRatioCalculation(int playerCount)
	{
		if (!m_wRatioBox1 || !m_wRatioBox2 || !m_wFinal)
			return;

		int leftRatio  = m_wRatioBox1.GetText().ToInt();
		int rightRatio = m_wRatioBox2.GetText().ToInt();

		if (leftRatio + rightRatio == 0)
			return;

		int leftPlayers  = Math.Round(playerCount / (leftRatio + rightRatio) * leftRatio);
		int rightPlayers = Math.Round(playerCount / (leftRatio + rightRatio) * rightRatio);

		m_wFinal.SetText(leftPlayers.ToString() + " : " + rightPlayers.ToString());
	}

	//------------------------------------------------------------------------------------------------
	protected void HideRatioDisplay()
	{
		if (m_wRatioBox1)        m_wRatioBox1.SetVisible(false);
		if (m_wRatioBox1Image)   m_wRatioBox1Image.SetVisible(false);
		if (m_wRatioBox1IntImage)m_wRatioBox1IntImage.SetVisible(false);
		if (m_wRatioBox1Text)    m_wRatioBox1Text.SetVisible(false);
		if (m_wRatioBox2)        m_wRatioBox2.SetVisible(false);
		if (m_wRatioBox2Image)   m_wRatioBox2Image.SetVisible(false);
		if (m_wRatioBox2IntImage)m_wRatioBox2IntImage.SetVisible(false);
		if (m_wRatioBox2Text)    m_wRatioBox2Text.SetVisible(false);
		if (m_wFinalImage)       m_wFinalImage.SetVisible(false);
		if (m_wFinal)            m_wFinal.SetVisible(false);
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetFactionColor(string factionKey)
	{
		switch (factionKey)
		{
			case "BLU": return Color.FromRGBA(0,   20,  255, 255);
			case "OPF": return Color.FromRGBA(188, 0,   0,   255);
			case "IND": return Color.FromRGBA(0,   145, 43,  255);
			case "CIV": return Color.FromRGBA(137, 0,   188, 255);
		}
		return Color.White;
	}
}
