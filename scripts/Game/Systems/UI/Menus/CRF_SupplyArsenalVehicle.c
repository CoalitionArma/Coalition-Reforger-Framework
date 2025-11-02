class CRF_SupplyArsenalVehicle: ChimeraMenuBase
{
	VerticalLayoutWidget m_Items;
	ref array<IEntity> m_aTrucks = {};
	
	override void OnMenuOpen()
	{
		m_aTrucks.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(SCR_PlayerController.GetLocalControlledEntity().GetOrigin(), 50, FindTruckCallback, null);
		foreach (IEntity truck: m_aTrucks)
		{
			
		}
	}
	
	CRF_MiniArsenalItemButton DrawTruck(IEntity truck, ItemPreviewManagerEntity manager)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{ADD28B3C4F9377B1}UI/layouts/Menus/Arsenal/SupplyArsenalItem.layout", m_Items);
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		manager.SetPreviewItemFromPrefab(itemPreview, truck.GetPrefabData().GetPrefabName());
		//TextWidget.Cast(item.FindAnyWidget("Supply")).SetText(m_SupplyCosts.Get(weapon.m_Weapon).ToString());
		
		SCR_EditableEntityComponent editableComp = SCR_EditableEntityComponent.Cast(truck.FindComponent(SCR_EditableEntityComponent));
		TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(editableComp.GetInfo().GetName());
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = truck.GetPrefabData().GetPrefabName();
		//itemButton.m_iSupplyCost = m_SupplyCosts.Get(weapon.m_Weapon);
		return itemButton;
	}
	
	bool FindTruckCallback(IEntity entity)
	{
		if (Vehicle.Cast(entity))
		{
			m_aTrucks.Insert(entity);
			
			return true;
		}
			
		return true;
	}
}