modded class LM_SuppressionScreenEffect
{
	override private float GetSuppressionAmount()
	{
		if (m_pPlayerController && m_pPlayerController.GetLocalMainEntity() && !CRF_GamemodeManager.IsSpectator(m_pPlayerController.GetLocalMainEntity()))
		{
			return m_pPlayerController.GetSuppressionAmount();
		}
		return 0;
	}
	
	override private void OnSuppressionFlinch()
	{
		if (m_pPlayerController && m_pPlayerController.GetLocalMainEntity() && !CRF_GamemodeManager.IsSpectator(m_pPlayerController.GetLocalMainEntity()))
		{
			FlinchEffect();
		}
	}
}