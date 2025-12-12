class CRF_BushMovementComponentClass: ScriptComponentClass
{
}

class CRF_BushMovementComponent: ScriptComponent
{
	[Attribute(params: "et")]
	ref array<ResourceName> m_aBushPrefabs;
	
	float m_fOriginalSpeed = 0;
	bool m_bEffectsApplied = false;
	bool m_bEffectsAppliedThisFrame = false;
	vector m_vOriginThisFrame;
	protected SCR_CharacterControllerComponent m_CharacterController;
	protected SCR_CharacterDamageManagerComponent m_DamageManager;
	
	void RegisterEntity()
	{
		//redundency to ensure this is NEVER on the server
		if (Replication.IsServer())
			return;
		
		m_CharacterController = SCR_CharacterControllerComponent.Cast(GetOwner().FindComponent(SCR_CharacterControllerComponent));
		m_DamageManager = SCR_CharacterDamageManagerComponent.Cast(GetOwner().FindComponent(SCR_CharacterDamageManagerComponent));
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
		if (soundType >= 7 || soundType == 1)
			return true;
		else
			return false;
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
		if (vector.Distance(m_vOriginThisFrame, entity.GetOrigin()) >= 2.5)
			return true;
		
		ApplyBushEffects();
			
		return true;
	}
	
	void ResetBushEffects()
	{
		m_DamageManager.SetMovementDamage(0);
	}
	
	void ApplyBushEffects()
	{
		m_bEffectsAppliedThisFrame = true;
		m_bEffectsApplied = true;
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
		{
			m_CharacterController.SetStanceChange(2);
			SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
			if (hintManager)
			{
			    hintManager.ShowCustomHint("Can't prone here mf", "Too thicc", 10);
			}
		}
		
		m_DamageManager.SetMovementDamage(0.5);
	}
	
	//Extra redundancy incase something fucking insane happens
	void ~CRF_BushMovementComponent()
	{
		UnregisterEntity();
	}
}