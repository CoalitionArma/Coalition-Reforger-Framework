modded class SCR_StaminaBlurEffect
{
	[Attribute("1.0", UIWidgets.EditBox, desc: "Mask scale applied by CRF lone-wolf tunnel vision")]
	protected float m_fCRFTunnelVisionMaskScale;

	[Attribute("0.75", UIWidgets.EditBox, desc: "Opacity scale applied by CRF lone-wolf tunnel vision")]
	protected float m_fCRFTunnelVisionOpacityScale;

	protected ImageWidget m_wCRFSuppressionVignette;
	protected float m_fCRFTunnelVisionSmoothed;

	override void DisplayStartDraw(IEntity owner)
	{
		super.DisplayStartDraw(owner);
		m_wCRFSuppressionVignette = ImageWidget.Cast(m_wRoot.FindAnyWidget("SuppressionVignette"));
	}

	override void UpdateEffect(float timeSlice)
	{
		super.UpdateEffect(timeSlice);
		UpdateCRFCohesionTunnelVision(timeSlice);
	}

	protected void UpdateCRFCohesionTunnelVision(float timeSlice)
	{
		if (!m_wCRFSuppressionVignette)
			return;

		float target = Math.Clamp(CRF_PlayerCharacter.GetLocalTunnelVisionIntensity(), 0, 1);
		float smoothing = Math.Clamp(timeSlice * 4.0, 0, 1);
		m_fCRFTunnelVisionSmoothed = Math.Lerp(m_fCRFTunnelVisionSmoothed, target, smoothing);

		float customMask = Math.Clamp(m_fCRFTunnelVisionSmoothed * m_fCRFTunnelVisionMaskScale, 0, 1);
		float customOpacity = Math.Clamp(m_fCRFTunnelVisionSmoothed * m_fCRFTunnelVisionOpacityScale, 0, 1);

		float baseMask = m_wCRFSuppressionVignette.GetMaskProgress();
		float baseOpacity = m_wCRFSuppressionVignette.GetOpacity();

		m_wCRFSuppressionVignette.SetMaskProgress(Math.Max(baseMask, customMask));
		m_wCRFSuppressionVignette.SetOpacity(Math.Max(baseOpacity, customOpacity));
		UpdateEffectVisibility(m_wCRFSuppressionVignette);
	}

	override void ClearEffects()
	{
		super.ClearEffects();
		m_fCRFTunnelVisionSmoothed = 0;
	}
}
