//------------------------------------------------------------------------------------
// CRF_RadioExtractionSendMessageAction: Send Extraction Message action for the
// Radio Extraction lobby prop. Available to the Extractor side once unlocked by the
// timer, and only while the bomb is neither planted nor detonated.
//------------------------------------------------------------------------------------

class CRF_RadioExtractionSendMessageAction : ScriptedUserAction
{
	protected CRF_RadioExtraction m_RadioExtraction;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode())
			return;

		m_RadioExtraction = CRF_RadioExtraction.GetInstance();
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity || !m_RadioExtraction)
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;

		CRF_GameplayRplToAuthorityManager.GetInstance().RequestRadioExtractionSendMessage();

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_RadioExtraction || !user)
			return false;

		return m_RadioExtraction.IsUserOnSide(user, CRF_ERadioExtractionSide.EXTRACTOR);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_RadioExtraction)
			return false;

		return m_RadioExtraction.IsExtractionUnlocked() && !m_RadioExtraction.IsBombPlanted() && !m_RadioExtraction.IsDetonated() && !m_RadioExtraction.IsExtractionMessageSent();
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
