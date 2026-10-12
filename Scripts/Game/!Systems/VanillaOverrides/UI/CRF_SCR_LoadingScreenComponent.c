//------------------------------------------------------------------------------------------------
//! COALITION mission loading screen extras, for UI/layouts/Menus/LoadingScreen/CRF_ScenarioLoadingScreen.layout
//! (created by CRF_ArmaReforgerLoadingAnim):
//!  - the mission's loading image is also shown faded full-screen behind the content
//!  - the three loading dots pulse in sequence
//! The vanilla radar spinner (SCR_LoadingSpinner) isn't used - the layout has no "Spinner" widget, so
//! m_SpinnerComp stays null and vanilla skips it.
modded class SCR_LoadingScreenComponent
{
	protected ImageWidget m_wCRF_Backdrop;
	protected ref array<Widget> m_aCRF_Dots;
	protected float m_fCRF_DotTime;

	// Dot pulse: speed and the phase gap between neighbouring dots
	protected static const float CRF_DOT_SPEED = 5;
	protected static const float CRF_DOT_PHASE = 0.9;

	//------------------------------------------------------------------------------------------------
	override protected void InitWidgets()
	{
		super.InitWidgets();

		m_wCRF_Backdrop = ImageWidget.Cast(m_wRoot.FindAnyWidget("COA_BackdropImage"));

		m_aCRF_Dots = {};
		for (int i = 0; i < 3; i++)
		{
			Widget dot = m_wRoot.FindAnyWidget("COA_Dot" + i.ToString());
			if (dot)
				m_aCRF_Dots.Insert(dot);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnMissionDataRetrieved(SCR_MissionHeader header)
	{
		super.OnMissionDataRetrieved(header);

		if (!header || !m_wCRF_Backdrop)
			return;

		// Same image vanilla puts in the card
		string image = header.m_sLoadingScreen;
		if (image.IsEmpty())
			image = header.m_sIcon;

		if (image.IsEmpty())
			return;

		m_wCRF_Backdrop.LoadImageTexture(0, image);
		m_wCRF_Backdrop.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice, float progress = 0, float minDurationRatio = 0)
	{
		super.Update(timeSlice, progress, minDurationRatio);

		if (!m_aCRF_Dots)
			return;

		m_fCRF_DotTime += timeSlice;
		foreach (int i, Widget dot : m_aCRF_Dots)
		{
			float wave = Math.Sin(m_fCRF_DotTime * CRF_DOT_SPEED - i * CRF_DOT_PHASE);
			dot.SetOpacity(0.25 + 0.75 * Math.Max(0, wave));
		}
	}
}
