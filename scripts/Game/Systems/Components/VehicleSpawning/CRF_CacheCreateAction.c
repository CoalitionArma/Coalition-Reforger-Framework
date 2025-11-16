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
		// Safely get RplIds - prevent crashes if entities don't have RplComponent
		RplId truckRplId;
		if (!CRF_ReplicationHelpers.GetRplId(m_Truck, truckRplId))
		{
			Print("[CRF_CacheCreateAction] ERROR: Truck entity has no RplComponent", LogLevel.ERROR);
			return;
		}
		
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlledEntity)
		{
			Print("[CRF_CacheCreateAction] ERROR: No controlled entity", LogLevel.ERROR);
			return;
		}
		
		RplId playerRplId;
		if (!CRF_ReplicationHelpers.GetRplId(controlledEntity, playerRplId))
		{
			Print("[CRF_CacheCreateAction] ERROR: Player entity has no RplComponent", LogLevel.ERROR);
			return;
		}
		
		CRF_RplToAuthorityManager.GetInstance().CreateCache(truckRplId, playerRplId);
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