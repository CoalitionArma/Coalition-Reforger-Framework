class CRF_BushMovementComponentClass: ScriptComponentClass
{
}

class CRF_BushMovementComponent: ScriptComponent
{
	[Attribute(params: "et")]
	ref array<ResourceName> m_aBushPrefabs;
	
	
	float m_fOldMovementDamage = 0;
	float m_fAppliedBushDamage = 0;
	const float BUSH_DISTANCE_SQ = 6.25; // 2.5 * 2.5
	bool m_bEffectsApplied = false;
	bool m_bEffectsAppliedThisFrame = false;
	vector m_vOriginThisFrame;
	protected SCR_CharacterControllerComponent m_CharacterController;
	protected SCR_CharacterDamageManagerComponent m_DamageManager;
	protected SCR_HintManagerComponent m_HintManager;
	
	void RegisterEntity()
	{
		//redundency to ensure this is NEVER on the server
		if (Replication.IsServer())
			return;
		
		m_CharacterController = SCR_CharacterControllerComponent.Cast(GetOwner().FindComponent(SCR_CharacterControllerComponent));
		m_DamageManager = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
		m_HintManager = SCR_HintManagerComponent.GetInstance();
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	void UnregisterEntity()
	{
		//redundency to ensure this is NEVER on the server
		if (Replication.IsServer())
			return;
		
		ClearEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	float m_fBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_DamageManager || !m_CharacterController)
			return;
		
		if (m_fBuffer <= 0.1)
		{
			m_fBuffer += timeSlice;
			return;
		}
		
		m_fBuffer = 0;
		
		m_bEffectsAppliedThisFrame = false;
		m_vOriginThisFrame = owner.GetOrigin();
		m_vOriginThisFrame[1] = m_vOriginThisFrame[1] + 1;
		GetGame().GetWorld().QueryEntitiesBySphere(m_vOriginThisFrame, 1, BushCheckCallback, null);
		if (!m_bEffectsAppliedThisFrame && m_bEffectsApplied)
			ResetBushEffects();
	}
	
	bool IsBush(int soundType)
	{
		return (soundType >= 7 || soundType == 1);
	}
	
	bool BushCheckCallback(IEntity entity)
	{	
		TreeClass treeClass = TreeClass.Cast(entity.GetPrefabData());
		if (!treeClass)
			return true;
		
		if (!IsBush(treeClass.SoundType))
			return true;
		
		//2.5 because query entity sphere fucking lies, thanks BI
		//Good balance between outside of bush/inside
		if (vector.DistanceSq(m_vOriginThisFrame, entity.GetOrigin()) >= BUSH_DISTANCE_SQ)
    		return true;
		
		ApplyBushEffects();
			
		return true;
	}
	
	void ResetBushEffects()
	{
		m_bEffectsApplied = false;
	
		// restore whatever the engine value was (including any injury/heal changes we captured)
		m_DamageManager.SetMovementDamage(m_fOldMovementDamage);
	
		m_fOldMovementDamage = 0;
		m_fAppliedBushDamage = 0;
	}

	
	void ApplyBushEffects()
	{
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
		{
			m_CharacterController.SetStanceChange(2);
			if (m_HintManager)
				m_HintManager.ShowCustomHint("Can't prone here mf", "Too thicc", 10);
		}
	
		// Read current engine movement damage BEFORE we override it
		float current = m_DamageManager.GetMovementDamage();
	
		// First time entering bush: cache baseline
		if (!m_bEffectsApplied)
		{
			m_fOldMovementDamage = current;
		}
		else
		{
			// If the engine changed movement damage since our last override (injury/heal), update baseline
			// If it's identical to what we applied, assume nothing changed.
			if (Math.AbsFloat(current - m_fAppliedBushDamage) > 0.001)
				m_fOldMovementDamage = current;
		}
	
		// Enforce "bush minimum"
		float desired = m_fOldMovementDamage;
		if (desired < 0.5)
			desired = 0.5;
	
		// Only set if needed
		if (Math.AbsFloat(current - desired) > 0.001)
			m_DamageManager.SetMovementDamage(desired);
	
		m_fAppliedBushDamage = desired;
	
		m_bEffectsAppliedThisFrame = true;
		m_bEffectsApplied = true;
	}

	
	//Extra redundancy incase something fucking insane happens
	void ~CRF_BushMovementComponent()
	{
		UnregisterEntity();
	}
}