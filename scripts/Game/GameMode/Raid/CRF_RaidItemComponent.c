class CRF_RaidItemComponentClass: ScriptComponentClass
{
}

class CRF_RaidItemComponent: ScriptComponent
{
	[Attribute("10")] int m_iPointsEarnedWhenDestroyed;
	CRF_RaidGamemodeComponent m_RaidGamemode;
	SCR_DestructionMultiPhaseComponent m_DestructionComp;
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_RaidGamemode = CRF_RaidGamemodeComponent.GetInstance();
		m_DestructionComp = SCR_DestructionMultiPhaseComponent.Cast(owner.FindComponent(SCR_DestructionMultiPhaseComponent));
		if (!m_DestructionComp)
			Print("[CRF RAID ERROR] NO DESTRUCTION COMPONENT ON " + owner);
		
	}
	
	void ~CRF_RaidItemComponent()
	{
		if (!GetGame().GetWorld())
			return;
		
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		m_RaidGamemode.OnObjectDestroyed(m_iPointsEarnedWhenDestroyed);
	}
}