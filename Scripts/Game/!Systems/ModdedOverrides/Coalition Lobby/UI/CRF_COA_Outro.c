//------------------------------------------------------------------------------------------------
// The cinematic outro plays for AAR_HANDOVER_TIME seconds, fades its text out, then hands over to
// the AAR screen (COA_AARMenu), which fades in from the same black background. The personal stats
// panel that used to appear here now lives on the AAR screen.
//------------------------------------------------------------------------------------------------
modded class COA_Outro
{
	// Seconds from the outro opening until the AAR screen takes over
	protected static const float AAR_HANDOVER_TIME = 10.0;
	// How long the title text takes to fade out, ending at the handover
	protected static const float TEXT_FADE_OUT_TIME = 1.0;

	protected float m_fOutroElapsed;
	protected bool m_bHandedOver;

	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_fOutroElapsed = 0;
		m_bHandedOver = false;
	}

	override void OnMenuUpdate(float tDelta)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);

		if (m_bHandedOver)
			return;

		m_fOutroElapsed += tDelta;

		float fadeStart = AAR_HANDOVER_TIME - TEXT_FADE_OUT_TIME;
		if (m_fOutroElapsed > fadeStart)
			SetTitleOpacity(1 - Math.Clamp((m_fOutroElapsed - fadeStart) / TEXT_FADE_OUT_TIME, 0, 1));

		if (m_fOutroElapsed >= AAR_HANDOVER_TIME)
		{
			m_bHandedOver = true;
			m_bAllowClose = true;

			// Not from inside this menu's own update
			GetGame().GetCallqueue().Call(HandOverToAAR);
		}
	}

	protected void SetTitleOpacity(float opacity)
	{
		Widget root = GetRootWidget();
		if (!root)
			return;

		array<string> titleWidgets = {"TitleText", "TitleText1", "TitleText2", "TitleText3"};
		foreach (string widgetName : titleWidgets)
		{
			Widget widget = root.FindAnyWidget(widgetName);
			if (widget)
				widget.SetOpacity(opacity);
		}
	}

	protected void HandOverToAAR()
	{
		Close();
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_AARMenu);
	}
}
