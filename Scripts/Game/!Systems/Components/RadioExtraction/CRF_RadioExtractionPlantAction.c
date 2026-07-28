//------------------------------------------------------------------------------------
// CRF_RadioExtractionPlantAction: Plant action for the Radio Extraction lobby prop
// Allows the Planter side to arm the bomb, starting its detonation countdown
//------------------------------------------------------------------------------------

class CRF_RadioExtractionPlantAction : ScriptedUserAction
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

		// Stop the planting sound since the action completed, then arm the bomb
		COA_PlayerRplToAuthorityManager.GetInstance().RequestStopPositionalSound(m_RadioExtraction.m_sPlantSoundEvent);
		COA_PlayerRplToAuthorityManager.GetInstance().RequestRadioExtractionSetPlanted(true);

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);

		if (m_RadioExtraction && GetOwner())
			COA_PlayerRplToAuthorityManager.GetInstance().RequestPlayPositionalSound(m_RadioExtraction.m_rPlantSoundResource, m_RadioExtraction.m_sPlantSoundEvent, GetOwner().GetOrigin());
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.OnActionCanceled(pOwnerEntity, pUserEntity);

		if (m_RadioExtraction)
			COA_PlayerRplToAuthorityManager.GetInstance().RequestStopPositionalSound(m_RadioExtraction.m_sPlantSoundEvent);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_RadioExtraction || !user)
			return false;

		return m_RadioExtraction.IsUserOnSide(user, CRF_ERadioExtractionSide.PLANTER);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_RadioExtraction)
			return false;

		return !m_RadioExtraction.IsBombPlanted() && !m_RadioExtraction.IsDetonated();
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
