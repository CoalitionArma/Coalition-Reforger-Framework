class CRF_RaidItemComponentClass: ScriptComponentClass
{
}

class CRF_RaidItemComponent: ScriptComponent
{
	[Attribute("10")] int m_iPointsEarnedWhenDestroyed;
	CRF_RaidGamemodeComponent m_RaidGamemode;
	SCR_DamageManagerComponent m_DestructionComp;
	bool m_bHasPointsBeenGiven = false;
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		m_RaidGamemode = CRF_RaidGamemodeComponent.GetInstance();
		m_DestructionComp = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!m_DestructionComp)
			Print("[CRF RAID ERROR] NO DESTRUCTION COMPONENT ON " + owner);
		m_DestructionComp.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
	}
	
	void OnDamageStateChanged(EDamageState state)
	{
		if (state == EDamageState.DESTROYED)
		{
			m_RaidGamemode.OnObjectDestroyed(m_iPointsEarnedWhenDestroyed);
			m_bHasPointsBeenGiven = true;
		}
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
		if (m_bHasPointsBeenGiven)
			return;
		m_RaidGamemode.OnObjectDestroyed(m_iPointsEarnedWhenDestroyed);
		m_bHasPointsBeenGiven = true;
	}
}