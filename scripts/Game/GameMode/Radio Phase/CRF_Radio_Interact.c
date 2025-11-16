//------------------------------------------------------------------------------------------------
class CRF_Radio_Interact : ScriptedUserAction
{	
	SCR_FactionManager factionManager;
	CRF_RadioPhaseManager rp;
	CRF_HighValueTargetGamemodeManager hvtm;
	IEntity radio;
	vector radioPos;
	IEntity hvt;
	vector hvtPos;
	
	bool m_fired = false;
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode()) return;
		
		factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		rp = CRF_RadioPhaseManager.Cast(pOwnerEntity.FindComponent(CRF_RadioPhaseManager));
		radio = pOwnerEntity;
		
	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		rp.fireTrigger();
		
		SCR_PopUpNotification.GetInstance().PopupMsg("Radio has been triggered", duration: 10);
		
		m_fired = true;
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		// Get user faction
		// Get game mode manager
		Faction userFaction = factionManager.GetLocalPlayerFaction();
		
		// Get attacker side definition
		if (rp.interactingSide != userFaction.GetFactionKey())
			return false;
		
		if (m_fired)
			return false;
		
		if (rp.requireHVT)
		{
			hvtm = CRF_HighValueTargetGamemodeManager.Cast(CRF_Gamemode.GetInstance().FindComponent(CRF_HighValueTargetGamemodeManager));
			hvt = hvtm.m_eHvtEntity;
			hvtPos = hvt.GetOrigin();
			radioPos = radio.GetOrigin();
			float distance = vector.Distance(hvtPos, radioPos);
			
			if (distance <50)
				return true;
			else 
				return false;
		}
		
		
		// else true
		return true;
	}	
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_fired)
			return false;
		
		// else true
		return true;
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
};