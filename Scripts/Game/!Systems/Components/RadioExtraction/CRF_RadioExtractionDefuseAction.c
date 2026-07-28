//------------------------------------------------------------------------------------
// CRF_RadioExtractionDefuseAction: Defuse action for the Radio Extraction lobby prop
// Allows the Extractor side to defuse the bomb while its countdown is active
//------------------------------------------------------------------------------------

class CRF_RadioExtractionDefuseAction : ScriptedUserAction
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

		// Stop the defusing sound since the action completed, then defuse the bomb
		COA_PlayerRplToAuthorityManager.GetInstance().RequestStopPositionalSound(m_RadioExtraction.m_sDefuseSoundEvent);
		COA_PlayerRplToAuthorityManager.GetInstance().RequestRadioExtractionSetPlanted(false);

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);

		if (m_RadioExtraction && GetOwner())
			COA_PlayerRplToAuthorityManager.GetInstance().RequestPlayPositionalSound(m_RadioExtraction.m_rDefuseSoundResource, m_RadioExtraction.m_sDefuseSoundEvent, GetOwner().GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.OnActionCanceled(pOwnerEntity, pUserEntity);

		if (m_RadioExtraction)
			COA_PlayerRplToAuthorityManager.GetInstance().RequestStopPositionalSound(m_RadioExtraction.m_sDefuseSoundEvent);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_RadioExtraction || !user)
			return false;

		if (!m_RadioExtraction.IsUserOnSide(user, CRF_ERadioExtractionSide.EXTRACTOR))
			return false;

		return m_RadioExtraction.IsCountdownActive();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_RadioExtraction)
			return false;

		return m_RadioExtraction.IsCountdownActive();
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
