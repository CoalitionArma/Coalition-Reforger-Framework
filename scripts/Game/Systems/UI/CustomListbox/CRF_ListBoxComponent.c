class CRF_ListboxComponent: SCR_ListBoxComponent
{
	CRF_Gamemode m_Gamemode;	
	int AddItemSpecSlot(Managed data = null, int slotId = -1)
	{	
		CRF_ListBoxElementComponent comp;
		
		int id = _AddItemSpecSlot(data, comp, slotId);
		
		return id;
	}
	
	protected int _AddItemSpecSlot(Managed data, out CRF_ListBoxElementComponent compOut, int slotId = -1)
	{
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{2FCF236EEB073259}UI/Listbox/SpecPlayerSlotListboxElementNonAdmin.layout", m_wList);
		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		comp.SetToggleable(true);
		comp.SetData(data);
		comp.SetRoleImage(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_rSlotIconResource, "roleimage");
		comp.SetRoleColor(GetGame().GetFactionManager().GetFactionByKey(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotFactionKey).GetFactionColor());
		comp.m_iSlotId = slotId;
		
		// Pushback to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up explicit navigation rules for elements. Otherwise we can't navigate
		// Through separators when we are at the edge of scrolling if there is an element
		// directly above/below the list box which intercepts focus
		string widgetName = this.GetUniqueWidgetName();
		newWidget.SetName(widgetName);
		if (m_aElementComponents.Count() > 1)
		{
			Widget prevWidget = m_aElementComponents[m_aElementComponents.Count() - 2].GetRootWidget();
			prevWidget.SetNavigation(WidgetNavigationDirection.DOWN, WidgetNavigationRuleType.EXPLICIT, newWidget.GetName());
			newWidget.SetNavigation(WidgetNavigationDirection.UP, WidgetNavigationRuleType.EXPLICIT, prevWidget.GetName());
		}
		
		compOut = comp;
		
		return id;
	}
	
	int AddItemSlot(Managed data = null, int slotId = -1, ResourceName overrideLayout = "")
	{	
		CRF_ListBoxElementComponent comp;
		
		int id = _AddItemSlot(data, comp, slotId, overrideLayout);
		
		return id;
	}
	
	protected int _AddItemSlot(Managed data, out CRF_ListBoxElementComponent compOut, int slotId = -1, ResourceName overrideLayout = "")
	{	
		// Create widget for this item
		// The layout can be provided either as argument or through attribute
		ResourceName selectedLayout;
		if(SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()) && CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.AAR)
			selectedLayout = "{9B0771FD74AAEB4B}UI/Listbox/PlayerSlotListboxElement.layout";
		else
			selectedLayout = "{64B8BF7DEE93A755}UI/Listbox/PlayerSlotListboxElementNonAdmin.layout";
		if(overrideLayout != "")
			selectedLayout = overrideLayout;
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets(selectedLayout, m_wList);
		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		comp.SetRoleText(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_sSlotName);
		comp.SetToggleable(true);
		comp.SetData(data);
		if(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_bIsLockedSlot)
			comp.SetPlayerText("CLOSED");
		else if(m_Gamemode.m_SlottingState == 0)
			{
				if(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_iSlotType != CRF_ESlotType.LEADERORMEDIC)
					comp.SetPlayerText("CLOSED");
				else
					comp.SetPlayerText("OPEN");
			}		
		else if(m_Gamemode.m_SlottingState == 1)
		{
			if(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_iSlotType != CRF_ESlotType.LEADERORMEDIC && CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_iSlotType != CRF_ESlotType.SPECIALTY)
				comp.SetPlayerText("CLOSED");
			else
				comp.SetPlayerText("OPEN");
		}
		else
			comp.SetPlayerText("OPEN");
		comp.SetRoleImage(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotUIData.m_rSlotIconResource, "roleimage");
		comp.SetRoleColor(GetGame().GetFactionManager().GetFactionByKey(CRF_SlottingManager.GetInstance().GetSlotData(slotId).m_SlotFactionKey).GetFactionColor());
		comp.m_iSlotId = slotId;
		
		// Pushback to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up explicit navigation rules for elements. Otherwise we can't navigate
		// Through separators when we are at the edge of scrolling if there is an element
		// directly above/below the list box which intercepts focus
		string widgetName = this.GetUniqueWidgetName();
		newWidget.SetName(widgetName);
		if (m_aElementComponents.Count() > 1)
		{
			Widget prevWidget = m_aElementComponents[m_aElementComponents.Count() - 2].GetRootWidget();
			prevWidget.SetNavigation(WidgetNavigationDirection.DOWN, WidgetNavigationRuleType.EXPLICIT, newWidget.GetName());
			newWidget.SetNavigation(WidgetNavigationDirection.UP, WidgetNavigationRuleType.EXPLICIT, prevWidget.GetName());
		}
		
		compOut = comp;
		
		return id;
	}
	
	int AddItemSpecGroup(Managed data = null, SCR_AIGroup group = null, ResourceName groupIcon = "")
	{	
		CRF_ListBoxElementComponent comp;
		
		int id = _AddItemSpecGroup(data, comp, group, groupIcon);
		
		return id;
	}
	
	protected int _AddItemSpecGroup(Managed data, out CRF_ListBoxElementComponent compOut, SCR_AIGroup group = null, ResourceName groupIcon = "")
	{
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{8C5AB6540BD27A7D}UI/Listbox/SpecGroupListBoxElementNonAdmin.layout", m_wList);
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));

		comp.SetGroupName(group.GetCustomNameWithOriginal());
		comp.SetToggleable(true);
		comp.SetData(data);
		comp.group = group;
		if(groupIcon)
			comp.SetRoleImage(groupIcon, "groupIcon");
		
				// Pushback to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up explicit navigation rules for elements. Otherwise we can't navigate
		// Through separators when we are at the edge of scrolling if there is an element
		// directly above/below the list box which intercepts focus
		string widgetName = this.GetUniqueWidgetName();
		newWidget.SetName(widgetName);
		if (m_aElementComponents.Count() > 1)
		{
			Widget prevWidget = m_aElementComponents[m_aElementComponents.Count() - 2].GetRootWidget();
			prevWidget.SetNavigation(WidgetNavigationDirection.DOWN, WidgetNavigationRuleType.EXPLICIT, newWidget.GetName());
			newWidget.SetNavigation(WidgetNavigationDirection.UP, WidgetNavigationRuleType.EXPLICIT, prevWidget.GetName());
		}
		
		compOut = comp;
		
		return id;
	}
	
	int AddItemChannel(Managed data = null, string channelName = "")
	{	
		CRF_ListBoxElementComponent comp;
		
		int id = _AddItemChannel(data, comp, channelName);
		
		return id;
	}
	
	
	protected int _AddItemChannel(Managed data, out CRF_ListBoxElementComponent compOut, string channelName)
	{
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{72CE576C888BC27A}UI/Listbox/VONChannelListBox.layout", m_wList);
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));

		comp.SetPlayerText(channelName);
		comp.SetToggleable(true);
		comp.SetData(data);
		
				// Pushback to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up explicit navigation rules for elements. Otherwise we can't navigate
		// Through separators when we are at the edge of scrolling if there is an element
		// directly above/below the list box which intercepts focus
		string widgetName = this.GetUniqueWidgetName();
		newWidget.SetName(widgetName);
		if (m_aElementComponents.Count() > 1)
		{
			Widget prevWidget = m_aElementComponents[m_aElementComponents.Count() - 2].GetRootWidget();
			prevWidget.SetNavigation(WidgetNavigationDirection.DOWN, WidgetNavigationRuleType.EXPLICIT, newWidget.GetName());
			newWidget.SetNavigation(WidgetNavigationDirection.UP, WidgetNavigationRuleType.EXPLICIT, prevWidget.GetName());
		}
		
		compOut = comp;
		
		return id;
	}
	
	int AddItemGroup(Managed data = null, SCR_AIGroup group = null, ResourceName overrideLayout = "", ResourceName groupIcon = "")
	{	
		CRF_ListBoxElementComponent comp;
		
		int id = _AddItemGroup(data, comp, group, overrideLayout, groupIcon);
		
		return id;
	}
	
	protected int _AddItemGroup(Managed data, out CRF_ListBoxElementComponent compOut, SCR_AIGroup group = null, ResourceName overrideLayout = "", ResourceName groupIcon = "")
	{	
		// Create widget for this item
		// The layout can be provided either as argument or through attribute
		ResourceName selectedLayout;
		if(SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId()) && CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.AAR)
			selectedLayout = "{80FE0FE1E3146535}UI/Listbox/GroupListBoxElement.layout";
		else
			selectedLayout = "{A078BC05E0FF79C5}UI/Listbox/GroupListBoxElementNonAdmin.layout";
		if(overrideLayout != "")
			selectedLayout = overrideLayout;
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets(selectedLayout, m_wList);
		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));

		comp.SetGroupName(group.GetCustomNameWithOriginal());
		comp.SetToggleable(true);
		comp.SetData(data);
		comp.group = group;
		if(groupIcon)
			comp.SetRoleImage(groupIcon, "groupIcon");
		
		// Pushback to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up explicit navigation rules for elements. Otherwise we can't navigate
		// Through separators when we are at the edge of scrolling if there is an element
		// directly above/below the list box which intercepts focus
		string widgetName = this.GetUniqueWidgetName();
		newWidget.SetName(widgetName);
		if (m_aElementComponents.Count() > 1)
		{
			Widget prevWidget = m_aElementComponents[m_aElementComponents.Count() - 2].GetRootWidget();
			prevWidget.SetNavigation(WidgetNavigationDirection.DOWN, WidgetNavigationRuleType.EXPLICIT, newWidget.GetName());
			newWidget.SetNavigation(WidgetNavigationDirection.UP, WidgetNavigationRuleType.EXPLICIT, prevWidget.GetName());
		}
		
		compOut = comp;
		
		return id;
	}
	
		
	CRF_ListBoxElementComponent GetCRFElementComponent(int item)
	{
		if (item < 0 || item > m_aElementComponents.Count())
			return null;
		
		return CRF_ListBoxElementComponent.Cast(m_aElementComponents[item]);
	}
}