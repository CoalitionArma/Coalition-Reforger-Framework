modded class SCR_StaminaBlurEffect
{
	[Attribute("1.0", UIWidgets.EditBox, desc: "Mask scale applied by CRF lone-wolf tunnel vision")]
	protected float m_fCRFTunnelVisionMaskScale;

	[Attribute("0.75", UIWidgets.EditBox, desc: "Opacity scale applied by CRF lone-wolf tunnel vision")]
	protected float m_fCRFTunnelVisionOpacityScale;

	protected ImageWidget m_wCRFSuppressionVignette;
	protected float m_fCRFTunnelVisionSmoothed;
	protected float m_fCRFLastAppliedMask;
	protected float m_fCRFLastAppliedOpacity;

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

		float target = Math.Clamp(CRF_LoneWolfPenaltyManager.GetLocalTunnelVisionIntensity(), 0, 1);
		float smoothing = Math.Clamp(timeSlice * 4.0, 0, 1);
		m_fCRFTunnelVisionSmoothed = Math.Lerp(m_fCRFTunnelVisionSmoothed, target, smoothing);

		float customMask = Math.Clamp(m_fCRFTunnelVisionSmoothed * m_fCRFTunnelVisionMaskScale, 0, 1);
		float customOpacity = Math.Clamp(m_fCRFTunnelVisionSmoothed * m_fCRFTunnelVisionOpacityScale, 0, 1);

		float baseMask = m_wCRFSuppressionVignette.GetMaskProgress();
		float baseOpacity = m_wCRFSuppressionVignette.GetOpacity();

		// Ignore our own previous write as a "floor" - otherwise Math.Max below only ever
		// ratchets upward against itself and the vignette never fades once we're the last writer.
		if (Math.AbsFloat(baseMask - m_fCRFLastAppliedMask) < 0.0005)
			baseMask = 0;
		if (Math.AbsFloat(baseOpacity - m_fCRFLastAppliedOpacity) < 0.0005)
			baseOpacity = 0;

		float finalMask = Math.Max(baseMask, customMask);
		float finalOpacity = Math.Max(baseOpacity, customOpacity);

		m_wCRFSuppressionVignette.SetMaskProgress(finalMask);
		m_wCRFSuppressionVignette.SetOpacity(finalOpacity);
		m_fCRFLastAppliedMask = finalMask;
		m_fCRFLastAppliedOpacity = finalOpacity;
		UpdateEffectVisibility(m_wCRFSuppressionVignette);
	}

	override void ClearEffects()
	{
		super.ClearEffects();
		m_fCRFTunnelVisionSmoothed = 0;
		m_fCRFLastAppliedMask = 0;
		m_fCRFLastAppliedOpacity = 0;
	}
}
