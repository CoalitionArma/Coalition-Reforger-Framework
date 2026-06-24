class CRF_BushMovementComponentClass: ScriptComponentClass
{
}

class CRF_BushMovementComponent: ScriptComponent
{
	[Attribute(params: "et")]
	ref array<ResourceName> m_aBushPrefabs;
	
	
	const float BUSH_DISTANCE_SQ = 2.25; // 1.5m x 1.5m
	bool m_bEffectsApplied = false;
	bool m_bEffectsAppliedThisFrame = false;
	vector m_vOriginThisFrame;
	vector m_vLastCheckedPos;
	protected SCR_CharacterControllerComponent m_CharacterController;
	protected SCR_HintManagerComponent m_HintManager;
	
	void RegisterEntity()
	{
		//redundency to ensure this is NEVER on the server
		if (Replication.IsServer())
			return;
		
		m_CharacterController = SCR_CharacterControllerComponent.Cast(GetOwner().FindComponent(SCR_CharacterControllerComponent));
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
		if (!m_CharacterController)
			return;
		
		if (m_fBuffer <= 0.1)
		{
			m_fBuffer += timeSlice;
			return;
		}
		
		m_fBuffer = 0;
		
		// Skip the sphere query if the player hasn't moved and is not currently inside a bush.
		// If effects ARE applied we still re-query so we can detect when the player leaves.
		vector currentPos = owner.GetOrigin();
		if (!m_bEffectsApplied && vector.DistanceSq(currentPos, m_vLastCheckedPos) < 0.01)
			return;
		m_vLastCheckedPos = currentPos;
		
		m_bEffectsAppliedThisFrame = false;
		m_vOriginThisFrame = owner.GetOrigin();
		m_vOriginThisFrame[1] = m_vOriginThisFrame[1] + 1.3;
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
	}

	
	void ApplyBushEffects()
	{
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE && !m_CharacterController.IsUnconscious())
		{
			m_CharacterController.SetStanceChange(2);
			if (m_HintManager)
				m_HintManager.ShowCustomHint("Cannot prone in the center of a bush", "Move away and try again", 10);
		}
	
		m_bEffectsAppliedThisFrame = true;
		m_bEffectsApplied = true;
	}

	
	//Extra redundancy incase something fucking insane happens
	void ~CRF_BushMovementComponent()
	{
		UnregisterEntity();
	}
}