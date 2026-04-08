class CRF_DestroyArtyAction: ScriptedUserAction
{
	bool m_bHasExplosive = false;
	bool m_bHasBeenDestroyed = false;
	RplId m_Id = RplId.Invalid();
	CRF_ArtyGunComponent m_ArtyComponent;
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		int gunsDestroyed = CRF_GamemodeManager.GetInstance().m_iGunsDestroyed;
		CRF_PlayerRplToAuthorityManager.GetInstance().DestroyArtyGun(m_Id);
		CRF_RplBroadcastManager broadcastMgr = CRF_RplBroadcastManager.GetInstance();
		if (broadcastMgr && gunsDestroyed < 3)
        	broadcastMgr.PopUpNotification(10, string.Format("%1/4 Guns have been destroyed!", gunsDestroyed + 1));
		else if (broadcastMgr)
			broadcastMgr.PopUpNotification(10, string.Format("All Guns have been destroyed, the US has captured Brecourt Manor!"));
	}
	
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_Id == RplId.Invalid())
			m_Id = RplComponent.Cast(GetOwner().FindComponent(RplComponent)).Id();

		if (!CanBeShownScript(user))
			return false;
			
		if (HasExplosive(user))
		{
			m_bHasExplosive = true;
			return true;
		}
		else
		{
			m_bHasExplosive = false;
			return false;
		}
			
	}
	
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (m_Id == RplId.Invalid())
			m_Id = RplComponent.Cast(GetOwner().FindComponent(RplComponent)).Id();
		m_ArtyComponent = CRF_ArtyGunComponent.Cast(pOwnerEntity.FindComponent(CRF_ArtyGunComponent));
	}
	
	bool HasExplosive(IEntity user)
	{
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(user.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!inventory)
			return false;
		
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventoryManager)
			return false;
		
		array<IEntity> items = {};
		array<IEntity> itemsRoot = {};
		inventoryManager.GetAllItems(items, inventory);
		inventoryManager.GetItems(itemsRoot);

		foreach (IEntity item: items)
		{
			if (item.FindComponent(GrenadeMoveComponent) || item.FindComponent(SCR_ExplosiveChargeComponent))
				return true;
		}
		
		return false;
	}
	
	override bool GetActionNameScript(out string outName)
	{
		if (m_bHasExplosive)
			outName = "Throw explosive down barrel";
		else
			outName = "You need an explosive";
		
			return true;
	}
	
	override bool CanBeShownScript(IEntity user)
	{
				
		if (m_ArtyComponent.m_bIsDestroyed)
			return false;
		
		return true;
	}
	
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
}