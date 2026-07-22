modded enum ChimeraMenuPreset
{
	CRF_SupplyArsenalVehicle
}

class CRF_SupplyArsenalVehicle: ChimeraMenuBase
{
	VerticalLayoutWidget m_Items;
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
		m_aTrucks.Clear();
		m_Items = VerticalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("ItemButtons"));
		m_Notifications = VerticalLayoutWidget.Cast(GetRootWidget().FindAnyWidget("Notifications"));
		Widget addButtonWidget = GetRootWidget().FindAnyWidget("AddSupplyButton");
		Widget refreshButtonWidget = GetRootWidget().FindAnyWidget("RefreshButton");
		if (!m_Items || !m_Notifications || !addButtonWidget || !refreshButtonWidget)
			return;

		m_AddItem = SCR_ButtonComponent.Cast(addButtonWidget.FindHandler(SCR_ButtonComponent));
		SCR_ButtonComponent refreshButton = SCR_ButtonComponent.Cast(refreshButtonWidget.FindHandler(SCR_ButtonComponent));
		if (!m_AddItem || !refreshButton)
			return;

		m_AddItem.m_OnClicked.Insert(RearmTruck);
		refreshButton.m_OnClicked.Insert(InitMenu);
		GetGame().GetCallqueue().CallLater(InitMenu, 100, false);
		
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		array<Widget> notificationsToDelete = {};
		foreach (Widget notification: m_aNotifications)
		{
			if (!notification)
				continue;

			CRF_SupplyNotification notificationComp = CRF_SupplyNotification.Cast(notification.FindHandler(CRF_SupplyNotification));
			if (!notificationComp)
				continue;

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
			notification.RemoveFromHierarchy();
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
		if (!m_Items || !m_Truck)
			return;

		while(m_Items.GetChildren())
			m_Items.GetChildren().RemoveFromHierarchy();
		m_aTrucks.Clear();
		m_RearmComponent = CRF_SupplyArsenalComponent.Cast(m_Truck.FindComponent(CRF_SupplyArsenalComponent));
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		SCR_Faction localFaction = SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction());
		if (!controlledEntity || !localFaction)
			return;

		GetGame().GetWorld().QueryEntitiesBySphere(controlledEntity.GetOrigin(), 50, FindTruckCallback, null);
		string factionKey = localFaction.GetFactionKey();
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
		CRF_GameplayRplToAuthorityManager rplManager = CRF_GameplayRplToAuthorityManager.GetInstance();
		RplId truckId;
		if (rplManager && CRF_ReplicationHelper.GetRplId(m_Truck, truckId))
			rplManager.UpdateSupplyArsneal(truckId);

		foreach (IEntity truck: m_aTrucks)
		{
			CRF_VehicleGearscriptManager gearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
			if (!truck || !manager || !gearscriptManager)
				continue;

			CRF_MiniArsenalItemButton itemButton = DrawTruck(truck, manager, gearscriptManager.IsSupplyTruck(truck, factionKey), factionKey);
			if (itemButton)
				itemButton.m_OnClicked.Insert(SelectItem);
		}
	}
	
	CRF_MiniArsenalItemButton DrawTruck(IEntity truck, ItemPreviewManagerEntity manager, bool isSupply, string factionKey)
	{
		if (!truck || !manager || !truck.GetPrefabData())
			return null;

		RplId truckId;
		if (!CRF_ReplicationHelper.GetRplId(truck, truckId))
			return null;

		Widget item = GetGame().GetWorkspace().CreateWidgets("{ADD28B3C4F9377B1}UI/layouts/Menus/Arsenal/SupplyArsenalItem.layout", m_Items);
		if (!item)
			return null;

		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		if (!itemPreview)
		{
			item.RemoveFromHierarchy();
			return null;
		}

		manager.SetPreviewItemFromPrefab(itemPreview, truck.GetPrefabData().GetPrefabName());
		if (m_bSupplyEnabled)
		{
			CRF_VehicleGearscriptManager gearscriptManager = CRF_VehicleGearscriptManager.GetInstance();
			CRF_GameplayRplToAuthorityManager rplManager = CRF_GameplayRplToAuthorityManager.GetInstance();
			if (!gearscriptManager || !rplManager)
			{
				item.RemoveFromHierarchy();
				return null;
			}

			int supplies = gearscriptManager.GetTruckResupplyCost(truck.GetPrefabData().GetPrefabName());
			rplManager.RequestVehicleSupplies(truckId);
			GetGame().GetCallqueue().CallLater(RequestCurrentVehicleSupply, 500, false, truckId, item, supplies);
		}
		else
		{
			item.FindAnyWidget("Supply").SetVisible(false);
			item.FindAnyWidget("SupplyImage").SetVisible(false);
		}
		
		SCR_EditableEntityComponent editableComp = SCR_EditableEntityComponent.Cast(truck.FindComponent(SCR_EditableEntityComponent));
		TextWidget itemText = TextWidget.Cast(item.FindWidget("ArsenalItemText"));
		if (!editableComp || !editableComp.GetInfo() || !itemText)
		{
			item.RemoveFromHierarchy();
			return null;
		}

		itemText.SetText(editableComp.GetInfo().GetName());
		
		Widget buttonWidget = item.FindWidget("ArsenalItemButton");
		if (!buttonWidget)
		{
			item.RemoveFromHierarchy();
			return null;
		}

		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(buttonWidget.FindHandler(CRF_MiniArsenalItemButton));
		if (!itemButton)
		{
			item.RemoveFromHierarchy();
			return null;
		}

		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = truck.GetPrefabData().GetPrefabName();
		itemButton.m_iEntityId = truckId;
		return itemButton;
	}
	
	void RequestCurrentVehicleSupply(RplId truckId, Widget item, int supplyNeeded)
	{
		IEntity truck = CRF_ReplicationHelper.GetEntityFromRplId(truckId);
		Vehicle vehicle = Vehicle.Cast(truck);
		if (!vehicle || !item)
			return;

		TextWidget supplyWidget = TextWidget.Cast(item.FindAnyWidget("Supply"));
		Widget buttonWidget = item.FindWidget("ArsenalItemButton");
		if (!buttonWidget)
			return;

		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(buttonWidget.FindHandler(CRF_MiniArsenalItemButton));
		if (!supplyWidget || !itemButton)
			return;

		int supplyCost = supplyNeeded - vehicle.m_iCurrentSupplies;
		supplyWidget.SetText(supplyCost.ToString());
		itemButton.m_iSupplyCost = supplyCost;
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
		if (!itemButton || (m_bSupplyEnabled && !m_RearmComponent))
			return;
		
		int supplyNeeded = itemButton.m_iSupplyCost;
		
		int totalAvailable = 0;
		array<RplId> supplyObjectRplId = {};
		array<IEntity> supplyObjects = {};
		array<int> supplyToSubtract = {};
		if (m_bSupplyEnabled)
		{
			foreach (int supply : m_RearmComponent.GetLocalSupplyCounts())
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
			    array<int> supplyCounts = m_RearmComponent.GetLocalSupplyCounts();
			    int availableCount = Math.Min(m_RearmComponent.GetLocalEntityArray().Count(), supplyCounts.Count());
			    for (int i = 0; i < availableCount; i++)
			    {
			        int count = supplyCounts[i];
			        if (count <= 0)
			            continue;
			
			        if (count < minSupply)
			        {
			            minSupply = count;
			            supplyObject = m_RearmComponent.GetLocalEntityArray()[i];
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
			    supplyCounts[minIndex] = supplyCounts[minIndex] - subtractAmount;
			}
			
			foreach (IEntity supplyObject: supplyObjects)
			{
				RplId supplyObjectId;
				if (!CRF_ReplicationHelper.GetRplId(supplyObject, supplyObjectId))
					return;

				supplyObjectRplId.Insert(supplyObjectId);
			}
		}
		
		RplId rearmTruckId;
		CRF_GameplayRplToAuthorityManager rplManager = CRF_GameplayRplToAuthorityManager.GetInstance();
		if (!rplManager || !CRF_ReplicationHelper.GetRplId(m_Truck, rearmTruckId))
			return;

		rplManager.RearmVehicle(itemButton.m_iEntityId, supplyObjectRplId, supplyToSubtract, rearmTruckId);
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
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(truckId));
		if (!rplComponent)
			return;

		IEntity truck = rplComponent.GetEntity();
		if (!truck)
			return;

		SCR_EditableEntityComponent editComp = SCR_EditableEntityComponent.Cast(truck.FindComponent(SCR_EditableEntityComponent));
		if (!editComp || !editComp.GetInfo())
			return;
		
		Widget item = GetGame().GetWorkspace().CreateWidgets("{8DE299D2A550FAFB}UI/layouts/Menus/Arsenal/SupplyArsenalNotification.layout", m_Notifications);
		if (!item)
			return;

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

	override void OnMenuClose()
	{
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(InitMenu);
			GetGame().GetCallqueue().Remove(RequestCurrentVehicleSupply);
		}

		foreach (Widget notification : m_aNotifications)
		{
			if (notification)
				notification.RemoveFromHierarchy();
		}
		m_aNotifications.Clear();

		super.OnMenuClose();
	}
}
