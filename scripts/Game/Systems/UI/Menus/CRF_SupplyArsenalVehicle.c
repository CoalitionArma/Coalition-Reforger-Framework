modded enum ChimeraMenuPreset
{
	CRF_SupplyArsenalVehicle
}

class CRF_SupplyArsenalVehicle: ChimeraMenuBase
{
	VerticalLayoutWidget m_Items;
	CRF_GearscriptManager m_GearscriptManager;
	SCR_ButtonComponent m_SelectedButton;
	IEntity m_Truck;
	ref array<IEntity> m_aTrucks = {};
	CRF_SupplyArsenalComponent m_RearmComponent;
	VerticalLayoutWidget m_Notifications;
	SCR_ButtonComponent m_AddItem;
	
	bool m_bSupplyEnabled;
	
	ref array<Widget> m_aNotifications = {};
	
	InputManager m_InputManager;
	bool m_bFocused = true;
	
	override void OnMenuOpen()
	{
		m_InputManager = GetGame().GetInputManager();
		m_bSupplyEnabled = SCR_ResourceSystemHelper.IsGlobalResourceTypeEnabled(EResourceType.SUPPLIES);
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_aTrucks.Clear();
		m_Items = VerticalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("ItemButtons"));
		m_Notifications = VerticalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("Notifications"));
		m_AddItem = SCR_ButtonComponent.Cast(GetRootWidget().FindAnyWidget("AddSupplyButton").FindHandler(SCR_ButtonComponent));
		m_AddItem.m_OnClicked.Insert(RearmTruck);
		SCR_ButtonComponent.Cast(GetRootWidget().FindAnyWidget("RefreshButton").FindHandler(SCR_ButtonComponent)).m_OnClicked.Insert(InitMenu);
		GetGame().GetCallqueue().CallLater(InitMenu, 100, false);
		
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		array<Widget> notificationsToDelete = {};
		foreach (Widget notification: m_aNotifications)
		{
			CRF_SupplyNotification notificationComp = CRF_SupplyNotification.Cast(notification.FindHandler(CRF_SupplyNotification));
			notificationComp.m_fTimeAlive += tDelta;
			if (notificationComp.m_fTimeAlive > 3 && !notificationComp.m_bAnimationStarted)
			{
				notificationComp.m_bAnimationStarted = true;
				AnimateWidget.Opacity(notification, 0, 3);
			}
			
			if (notificationComp.m_bAnimationStarted && notification.GetOpacity() <= 0)
				notificationsToDelete.Insert(notification);
		}
		
		foreach (Widget notification: notificationsToDelete)
		{
			m_aNotifications.RemoveItem(notification);
			delete notification;
		}
		
		if (m_bSupplyEnabled)
			GetRootWidget().FindAnyWidget("CurrentSupplies").SetVisible(true);
		else
			GetRootWidget().FindAnyWidget("CurrentSupplies").SetVisible(false);
		
		if (m_RearmComponent && m_bSupplyEnabled)
			TextWidget.Cast(GetRootWidget().FindAnyWidget("CurrentSupplies")).SetText("Current Supplies: " + m_RearmComponent.GetCurrentSupply().ToString());
	}
	
	void InitMenu()
	{
		while(m_Items.GetChildren())
			delete m_Items.GetChildren();
		m_aTrucks.Clear();
		m_RearmComponent = CRF_SupplyArsenalComponent.Cast(m_Truck.FindComponent(CRF_SupplyArsenalComponent));
		GetGame().GetWorld().QueryEntitiesBySphere(SCR_PlayerController.GetLocalControlledEntity().GetOrigin(), 50, FindTruckCallback, null);
		string factionKey = SCR_FactionManager.SGetLocalPlayerFaction().GetFactionKey();
		ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		CRF_RplToAuthorityManager.GetInstance().UpdateSupplyArsneal(RplComponent.Cast(m_Truck.FindComponent(RplComponent)).Id());
		foreach (IEntity truck: m_aTrucks)
		{
			DrawTruck(truck, manager, m_GearscriptManager.IsSupplyTruck(truck, factionKey), factionKey).m_OnClicked.Insert(SelectItem);
		}
	}
	
	CRF_MiniArsenalItemButton DrawTruck(IEntity truck, ItemPreviewManagerEntity manager, bool isSupply, string factionKey)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{ADD28B3C4F9377B1}UI/layouts/Menus/Arsenal/SupplyArsenalItem.layout", m_Items);
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		manager.SetPreviewItemFromPrefab(itemPreview, truck.GetPrefabData().GetPrefabName());
		if (m_bSupplyEnabled)
		{
			int supplies = m_GearscriptManager.GetTruckResupplyCost(truck.GetPrefabData().GetPrefabName());
			CRF_RplToAuthorityManager.GetInstance().RequestVehicleSupplies(RplComponent.Cast(truck.FindComponent(RplComponent)).Id());
			GetGame().GetCallqueue().CallLater(RequestCurrentVehicleSupply, 500, false, truck, item, supplies);
		}
		else
		{
			item.FindAnyWidget("Supply").SetVisible(false);
			item.FindAnyWidget("SupplyImage").SetVisible(false);
		}
		
		SCR_EditableEntityComponent editableComp = SCR_EditableEntityComponent.Cast(truck.FindComponent(SCR_EditableEntityComponent));
		TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(editableComp.GetInfo().GetName());
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = truck.GetPrefabData().GetPrefabName();
		itemButton.m_iEntityId = RplComponent.Cast(truck.FindComponent(RplComponent)).Id();
		return itemButton;
	}
	
	void RequestCurrentVehicleSupply(IEntity truck, Widget item, int supplyNeeded)
	{
		TextWidget.Cast(item.FindAnyWidget("Supply")).SetText((supplyNeeded - Vehicle.Cast(truck).m_iCurrentSupplies).ToString());
		CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton)).m_iSupplyCost = supplyNeeded - Vehicle.Cast(truck).m_iCurrentSupplies;
	}
	
	bool FindTruckCallback(IEntity entity)
	{
		if (m_Truck == entity)
			return true;
		
		if (Vehicle.Cast(entity))
		{
			m_aTrucks.Insert(entity);
			
			return true;
		}
			
		return true;
	}
	
	void SelectItem(SCR_ButtonComponent button)
	{
		m_SelectedButton = button;
	}
	
	void RearmTruck()
	{
		if (!m_SelectedButton)
			return;
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(m_SelectedButton);
		
		int supplyNeeded = itemButton.m_iSupplyCost;
		
		int totalAvailable = 0;
		array<RplId> supplyObjectRplId = {};
		array<IEntity> supplyObjects = {};
		array<int> supplyToSubtract = {};
		if (m_bSupplyEnabled)
		{
			foreach (int supply : m_RearmComponent.m_aSupplyCounts)
			    totalAvailable += supply;
			
			if (totalAvailable < supplyNeeded)
			{
			    NoSupplyNotification();
			    return;
			}
			
			while (supplyNeeded > 0)
			{
			    IEntity supplyObject = null;
			    int minSupply = int.MAX;
			    int minIndex = -1;
			
			    // Find the supply object with the *smallest* nonzero count
			    for (int i = 0; i < m_RearmComponent.GetEntityArray().Count(); i++)
			    {
			        int count = m_RearmComponent.m_aSupplyCounts[i];
			        if (count <= 0)
			            continue;
			
			        if (count < minSupply)
			        {
			            minSupply = count;
			            supplyObject = m_RearmComponent.GetEntityArray()[i];
			            minIndex = i;
			        }
			    }
			
			    // If no supply was found, break out
			    if (minIndex == -1)
			        break;
			
			    // Decide how much to subtract
			    int subtractAmount;
			    if (supplyNeeded < minSupply)
			    {
			        subtractAmount = supplyNeeded;
			        supplyNeeded = 0;
			    }
			    else
			    {
			        subtractAmount = minSupply;
			        supplyNeeded -= minSupply;
			    }
			
			    // Store results
			    supplyObjects.Insert(supplyObject);
			    supplyToSubtract.Insert(subtractAmount);
			
			    // Reduce the count of that supply item
			    m_RearmComponent.m_aSupplyCounts.Get(minIndex) -= subtractAmount;
			}
			
			
			foreach (IEntity supplyObject: supplyObjects)
			{
				supplyObjectRplId.Insert(RplComponent.Cast(supplyObject.FindComponent(RplComponent)).Id());
			}
		}
		

		CRF_RplToAuthorityManager.GetInstance().RearmVehicle(itemButton.m_iEntityId, supplyObjectRplId, supplyToSubtract, RplComponent.Cast(m_Truck.FindComponent(RplComponent)).Id());
		AddNotification(itemButton.m_iEntityId);
		GetGame().GetCallqueue().CallLater(InitMenu, 100, false);
	}
	
	void NoSupplyNotification()
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{8DE299D2A550FAFB}UI/layouts/Menus/Arsenal/SupplyArsenalNotification.layout", m_Notifications);
		TextWidget.Cast(item.FindAnyWidget("ArsenalItemText")).SetText(string.Format("Not Enough Supply Nearby"));
		item.FindAnyWidget("Image0").SetColor(Color.FromInt(Color.RED));
		m_aNotifications.Insert(item);
	}
	
	void AddNotification(RplId truckId)
	{
		IEntity truck = RplComponent.Cast(Replication.FindItem(truckId)).GetEntity();
		SCR_EditableEntityComponent editComp = SCR_EditableEntityComponent.Cast(truck.FindComponent(SCR_EditableEntityComponent));
		
		Widget item = GetGame().GetWorkspace().CreateWidgets("{8DE299D2A550FAFB}UI/layouts/Menus/Arsenal/SupplyArsenalNotification.layout", m_Notifications);
		TextWidget.Cast(item.FindAnyWidget("ArsenalItemText")).SetText(string.Format("%1 has been rearmed", editComp.GetInfo().GetName()));
		m_aNotifications.Insert(item);
	}
	
	override void OnMenuFocusLost()
	{
		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}
}