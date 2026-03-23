//! Area-based healing DOT effect. Uses EDamageType.HEALING via SCR_DotDamageEffect.
//! Target modes: 0=affected HZ, 1=all physical, 2=blood (DealDot), 3=resilience.
//! Follows BI's saline/morphine pattern. Use with CRF_DotDamageArea or SCR_DotDamageArea.

[BaseContainerProps()]
class CRF_HealingDamageEffect : SCR_DotDamageEffect
{
	[Attribute(defvalue: "0", desc: "Healing target mode.\n0 = Affected hitzone only (set by area)\n1 = All physical hitzones\n2 = Blood hitzone\n3 = Resilience hitzone", uiwidget: UIWidgets.ComboBox, enums: { ParamEnum("Affected HitZone", "0"), ParamEnum("All Physical", "1"), ParamEnum("Blood", "2"), ParamEnum("Resilience", "3") }, category: "CRF Healing")]
	protected int m_iHealingTarget;
	
	protected ref array<HitZone> m_aPhysicalHitZones;
	
	protected override void HandleConsequences(SCR_ExtendedDamageManagerComponent dmgManager, DamageEffectEvaluator evaluator)
	{
		super.HandleConsequences(dmgManager, evaluator);
		evaluator.HandleEffectConsequences(this, dmgManager);
	}
	
	//! Force DPS negative, redirect hitzone by mode, and prevent stacking from same source.
	override bool HijackDamageEffect(SCR_ExtendedDamageManagerComponent dmgManager)
	{
		SetDPS(-Math.AbsFloat(GetDPS()));
		
		// Redirect to the appropriate hitzone based on healing target mode
		SCR_CharacterDamageManagerComponent charDmgMgr = SCR_CharacterDamageManagerComponent.Cast(dmgManager);
		
		switch (m_iHealingTarget)
		{
			case 1: // All Physical — handled in EOnFrame
				SetAffectedHitZone(dmgManager.GetDefaultHitZone());
				break;
			case 2: // Blood
				if (charDmgMgr)
					SetAffectedHitZone(charDmgMgr.GetBloodHitZone());
				break;
			case 3: // Resilience
				if (charDmgMgr)
					SetAffectedHitZone(charDmgMgr.GetResilienceHitZone());
				break;
			// case 0: use whatever hitzone the area set 
		}
		
		// Don't stack multiple healing effects from the same instigator on the same target
		array<ref SCR_PersistentDamageEffect> existingEffects = {};
		dmgManager.FindAllDamageEffectsOfTypeOnHitZone(CRF_HealingDamageEffect, GetAffectedHitZone(), existingEffects);
		
		IEntity instigatorEnt = GetInstigator().GetInstigatorEntity();
		foreach (SCR_PersistentDamageEffect existing : existingEffects)
		{
			if (existing == this)
				continue;
			
			if (existing.GetInstigator().GetInstigatorEntity() == instigatorEnt)
				return true; // Already have one from this source — don't add another
		}
		
		return false;
	}
	
	override void OnEffectAdded(SCR_ExtendedDamageManagerComponent dmgManager)
	{
		super.OnEffectAdded(dmgManager);
		
		if (m_iHealingTarget == 1)
		{
			m_aPhysicalHitZones = {};
			dmgManager.GetPhysicalHitZones(m_aPhysicalHitZones);
		}
	}
	
	//! Per-frame healing. Blood uses DealDot (saline path), all-physical uses DealCustomDot,
	//! modes 0/3 use direct SetHealthScaled.
	protected override void EOnFrame(float timeSlice, SCR_ExtendedDamageManagerComponent dmgManager)
	{
		// Mode 2 (Blood): must go through DealDot — the saline path.
		// DealDot handles its own timeSlice, timer, and DPS internally.
		if (m_iHealingTarget == 2)
		{
			DealDot(timeSlice, dmgManager);
			return;
		}
		
		float accurateTimeSlice = GetAccurateTimeSlice(timeSlice);
		float healAmount = Math.AbsFloat(GetDPS()) * accurateTimeSlice;
		DotDamageEffectTimerToken token = UpdateTimer(accurateTimeSlice, dmgManager);
		
		// Mode 1: Heal all physical hitzones individually (morphine pattern)
		if (m_iHealingTarget == 1 && m_aPhysicalHitZones)
		{
			foreach (HitZone hz : m_aPhysicalHitZones)
			{
				DealCustomDot(hz, -healAmount, token, dmgManager);
			}
			return;
		}
		
		// Modes 0/3: Heal single affected hitzone directly
		HitZone affectedHZ = GetAffectedHitZone();
		if (!affectedHZ)
			return;
		
		float currentHealth = affectedHZ.GetHealthScaled();
		if (currentHealth >= 1.0)
			return;
		
		float maxHP = affectedHZ.GetMaxHealth();
		if (maxHP <= 0)
			return;
		
		float addScaled = healAmount / maxHP;
		affectedHZ.SetHealthScaled(Math.Min(1.0, currentHealth + addScaled));
	}
	
	//! Save/Load m_iHealingTarget to survive engine clone + replication.
	override bool Save(ScriptBitWriter w)
	{
		super.Save(w);
		w.WriteInt(m_iHealingTarget);
		return true;
	}
	
	override bool Load(ScriptBitReader r)
	{
		super.Load(r);
		r.ReadInt(m_iHealingTarget);
		return true;
	}
	
	override EDamageType GetDefaultDamageType()
	{
		return EDamageType.HEALING;
	}
}
