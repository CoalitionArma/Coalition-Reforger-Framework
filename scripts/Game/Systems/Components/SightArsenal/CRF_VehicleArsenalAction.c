class CRF_VehicleArsenalAction: ScriptedUserAction
{
	IEntity m_ArsenalPoint;
	IEntity m_ClosestTruck;
	string m_CurrentMessage;
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_ArsenalPoint = pOwnerEntity;
		m_CurrentMessage = "Open Supply Arsenal";
	}
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		CRF_SupplyArsenal menu = CRF_SupplyArsenal.Cast(GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SupplyArsenal));
		menu.m_ArsenalPoint = pOwnerEntity.GetRootParent();
		RplComponent rplComp = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		if (!rplComp)
			return;
		
		CRF_RplToAuthorityManager.GetInstance().RequestSupplyUpdate(rplComp.Id());
	}
	
	override bool CanBePerformedScript(IEntity user)
	{
		IEntity truck = GetNearestVehicle();
		if (!truck)
		{
			m_CurrentMessage = "No Vehicle in Range";
			return false;
		}
		else
		{
			m_CurrentMessage = "Open Supply Arsenal";
			return true;
		}
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
	
	IEntity GetNearestVehicle()
	{
		m_ClosestTruck = null;
		GetGame().GetWorld().QueryEntitiesBySphere(m_ArsenalPoint.GetOrigin(), 50, FindTruckCallback, null);
		return m_ClosestTruck;
	}
	
	bool FindTruckCallback(IEntity entity)
	{
		if (Vehicle.Cast(entity))
		{
			if (!m_ClosestTruck)
			{
				m_ClosestTruck = entity;
				return true;
			}
				
			if (vector.Distance(m_ClosestTruck.GetOrigin(), m_ArsenalPoint.GetOrigin()) > vector.Distance(entity.GetOrigin(), m_ArsenalPoint.GetOrigin()))
				m_ClosestTruck = entity;
			
			return true;
		}
			
		return true;
	}
}