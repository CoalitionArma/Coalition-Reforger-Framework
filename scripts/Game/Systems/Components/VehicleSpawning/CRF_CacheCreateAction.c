class CRF_CacheCreateAction: ScriptedUserAction
{
	IEntity m_Truck;
	string m_CurrentMessage;
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_Truck = pOwnerEntity.GetRootParent();
		m_CurrentMessage = "Create Supply Cache";
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CRF_RplToAuthorityManager.GetInstance().CreateCache(RplComponent.Cast(m_Truck.FindComponent(RplComponent)).Id(), RplComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(RplComponent)).Id());
	}
	
	override bool GetActionNameScript(out string outName)
	{
		outName = m_CurrentMessage;
		return true;
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		return true;
	}
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
}