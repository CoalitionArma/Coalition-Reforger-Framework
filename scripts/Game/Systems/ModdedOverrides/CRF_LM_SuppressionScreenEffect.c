modded class LM_SuppressionScreenEffect
{
	override private float GetSuppressionAmount()
	{
		if (m_pPlayerController && m_pPlayerController.GetLocalMainEntity() && m_pPlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
		{
			return m_pPlayerController.GetSuppressionAmount();
		}
		return 0;
	}
	
	override private void OnSuppressionFlinch()
	{
		if (m_pPlayerController && m_pPlayerController.GetLocalMainEntity() && m_pPlayerController.GetLocalMainEntity().GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
		{
			FlinchEffect();
		}
	}
}