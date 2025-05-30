class CRF_ListboxComponent: SCR_ListBoxComponent
{
	/**
	 * Reference to the game mode instance for slot state checking
	 */
	CRF_Gamemode m_Gamemode;
	
	/**
	 * Adds a specialized slot item to the listbox
	 * @param data Data to associate with the item
	 * @param slotId ID of the slot
	 * @return Index of the added item
	 */
	int AddItemSpecSlot(Managed data = null, int slotId = -1)
	{	
		CRF_ListBoxElementComponent comp;
		int id = _AddItemSpecSlot(data, comp, slotId);
		return id;
	}
	
	/**
	 * Protected implementation for adding a specialized slot
	 * @param data Data to associate with the item
	 * @param compOut Output parameter for the created component
	 * @param slotId ID of the slot
	 * @return Index of the added item
	 */
	protected int _AddItemSpecSlot(Managed data, out CRF_ListBoxElementComponent compOut, int slotId = -1)
	{
		// Create the widget from layout
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{2FCF236EEB073259}UI/Listbox/SpecPlayerSlotListboxElementNonAdmin.layout", m_wList);
		
		// Get the component from the widget
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		// Configure the component
		comp.SetToggleable(true);
		comp.SetData(data);
		
		// Get slot data and configure role image and color
		CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotData(slotId);
		comp.SetRoleImage(slotData.GetSlotIconResource(), "roleimage");
		
		// Set the role color based on faction
		FactionKey factionKey = slotData.GetSlotFactionKey();
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		Color factionColor = faction.GetFactionColor();
		comp.SetRoleColor(factionColor);
		
		comp.m_iSlotId = slotId;
		
		// Add to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up explicit navigation rules for elements
		// This ensures we can navigate through separators when at the edge of scrolling
		// if there's an element directly above/below the listbox that intercepts focus
		SetupWidgetNavigation(newWidget, comp);
		
		compOut = comp;
		return id;
	}
	
	/**
	 * Adds a standard slot item to the listbox
	 * @param data Data to associate with the item
	 * @param slotId ID of the slot
	 * @param overrideLayout Optional custom layout to use
	 * @return Index of the added item
	 */
	int AddItemSlot(Managed data = null, int slotId = -1, ResourceName overrideLayout = "")
	{	
		CRF_ListBoxElementComponent comp;
		int id = _AddItemSlot(data, comp, slotId, overrideLayout);
		return id;
	}
	
	/**
	 * Protected implementation for adding a standard slot
	 * @param data Data to associate with the item
	 * @param compOut Output parameter for the created component
	 * @param slotId ID of the slot
	 * @param overrideLayout Optional custom layout to use
	 * @return Index of the added item
	 */
	protected int _AddItemSlot(Managed data, out CRF_ListBoxElementComponent compOut, int slotId = -1, ResourceName overrideLayout = "")
	{	
		// Select the appropriate layout based on conditions
		ResourceName selectedLayout;
		
		// Use override layout if provided
		if (overrideLayout != "") {
			selectedLayout = overrideLayout;
		} else {
			bool isAdmin = SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId());
			bool notInAARState = CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.AAR;
			
			if (isAdmin && notInAARState) 
				selectedLayout = "{9B0771FD74AAEB4B}UI/Listbox/PlayerSlotListboxElement.layout";
			else 
				selectedLayout = "{64B8BF7DEE93A755}UI/Listbox/PlayerSlotListboxElementNonAdmin.layout";
		}
		
		// Create the widget
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets(selectedLayout, m_wList);
		
		// Get and configure the component
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		// Get slot data
		CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotData(slotId);
		
		// Configure basic properties
		comp.SetRoleText(slotData.GetSlotName());
		comp.SetToggleable(true);
		comp.SetData(data);
		
		// Set player text based on slot state
		if(slotData.GetIsLockedSlot()) {
			comp.SetPlayerText("CLOSED");
		} else if(m_Gamemode.m_SlottingState == 0) {
			if(slotData.GetSlotType() != CRF_ESlotType.LEADERORMEDIC) {
				comp.SetPlayerText("CLOSED");
			} else {
				comp.SetPlayerText("OPEN");
			}
		} else if(m_Gamemode.m_SlottingState == 1) {
			if(slotData.GetSlotType() != CRF_ESlotType.LEADERORMEDIC && slotData.GetSlotType() != CRF_ESlotType.SPECIALTY) {
				comp.SetPlayerText("CLOSED");
			} else {
				comp.SetPlayerText("OPEN");
			}
		} else {
			comp.SetPlayerText("OPEN");
		}
		
		// Set role image and color
		comp.SetRoleImage(slotData.GetSlotIconResource(), "roleimage");
		
		FactionKey factionKey = slotData.GetSlotFactionKey();
		Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
		Color factionColor = faction.GetFactionColor();
		comp.SetRoleColor(factionColor);
		
		comp.m_iSlotId = slotId;
		
		// Add to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up navigation
		SetupWidgetNavigation(newWidget, comp);
		
		compOut = comp;
		return id;
	}
	
	/**
	 * Adds a specialized group item to the listbox
	 * @param data Data to associate with the item
	 * @param group The AI group this item represents
	 * @param groupIcon Icon resource for the group
	 * @return Index of the added item
	 */
	int AddItemSpecGroup(Managed data = null, SCR_AIGroup group = null, ResourceName groupIcon = "")
	{	
		CRF_ListBoxElementComponent comp;
		int id = _AddItemSpecGroup(data, comp, group, groupIcon);
		return id;
	}
	
	/**
	 * Protected implementation for adding a specialized group
	 * @param data Data to associate with the item
	 * @param compOut Output parameter for the created component
	 * @param group The AI group this item represents
	 * @param groupIcon Icon resource for the group
	 * @return Index of the added item
	 */
	protected int _AddItemSpecGroup(Managed data, out CRF_ListBoxElementComponent compOut, SCR_AIGroup group = null, ResourceName groupIcon = "")
	{
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{8C5AB6540BD27A7D}UI/Listbox/SpecGroupListBoxElementNonAdmin.layout", m_wList);
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));

		comp.SetGroupName(group.GetCustomNameWithOriginal());
		comp.SetToggleable(true);
		comp.SetData(data);
		comp.group = group;
		
		// Set group icon if provided
		if(groupIcon != "") {
			comp.SetRoleImage(groupIcon, "groupIcon");
		}
		
		// Add to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up navigation
		SetupWidgetNavigation(newWidget, comp);
		
		compOut = comp;
		return id;
	}
	
	/**
	 * Adds a channel item to the listbox
	 * @param data Data to associate with the item
	 * @param channelName Name of the channel
	 * @return Index of the added item
	 */
	int AddItemChannel(Managed data = null, string channelName = "")
	{	
		CRF_ListBoxElementComponent comp;
		int id = _AddItemChannel(data, comp, channelName);
		return id;
	}
	
	/**
	 * Protected implementation for adding a channel
	 * @param data Data to associate with the item
	 * @param compOut Output parameter for the created component
	 * @param channelName Name of the channel
	 * @return Index of the added item
	 */
	protected int _AddItemChannel(Managed data, out CRF_ListBoxElementComponent compOut, string channelName)
	{
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{72CE576C888BC27A}UI/Listbox/VONChannelListBox.layout", m_wList);
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));

		comp.SetPlayerText(channelName);
		comp.SetToggleable(true);
		comp.SetData(data);
		
		// Add to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up navigation
		SetupWidgetNavigation(newWidget, comp);
		
		compOut = comp;
		return id;
	}
	
	/**
	 * Adds a group item to the listbox
	 * @param data Data to associate with the item
	 * @param group The AI group this item represents
	 * @param overrideLayout Optional custom layout to use
	 * @param groupIcon Icon resource for the group
	 * @return Index of the added item
	 */
	int AddItemGroup(Managed data = null, SCR_AIGroup group = null, ResourceName overrideLayout = "", ResourceName groupIcon = "")
	{	
		CRF_ListBoxElementComponent comp;
		int id = _AddItemGroup(data, comp, group, overrideLayout, groupIcon);
		return id;
	}
	
	/**
	 * Protected implementation for adding a group
	 * @param data Data to associate with the item
	 * @param compOut Output parameter for the created component
	 * @param group The AI group this item represents
	 * @param overrideLayout Optional custom layout to use
	 * @param groupIcon Icon resource for the group
	 * @return Index of the added item
	 */
	protected int _AddItemGroup(Managed data, out CRF_ListBoxElementComponent compOut, SCR_AIGroup group = null, ResourceName overrideLayout = "", ResourceName groupIcon = "")
	{	
		// Select the appropriate layout
		ResourceName selectedLayout;
		
		// Use override layout if provided
		if(overrideLayout != "") {
			selectedLayout = overrideLayout;
		} else {
			bool isAdmin = SCR_Global.IsAdmin(SCR_PlayerController.GetLocalPlayerId());
			bool notInAARState = CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.AAR;
			
			if (isAdmin && notInAARState)
				selectedLayout = "{80FE0FE1E3146535}UI/Listbox/GroupListBoxElement.layout";
			else 
				selectedLayout = "{A078BC05E0FF79C5}UI/Listbox/GroupListBoxElementNonAdmin.layout";
		}
		
		Widget newWidget = GetGame().GetWorkspace().CreateWidgets(selectedLayout, m_wList);
		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(newWidget.FindHandler(CRF_ListBoxElementComponent));

		comp.SetGroupName(group.GetCustomNameWithOriginal());
		comp.SetToggleable(true);
		comp.SetData(data);
		comp.group = group;
		
		// Set group icon if provided
		if(groupIcon != "") {
			comp.SetRoleImage(groupIcon, "groupIcon");
		}
		
		// Add to internal arrays
		int id = m_aElementComponents.Insert(comp);
		
		// Setup event handlers
		comp.m_OnClicked.Insert(OnItemClick);
		
		// Set up navigation
		SetupWidgetNavigation(newWidget, comp);
		
		compOut = comp;
		return id;
	}
	
	/**
	 * Helper method to set up widget navigation between list elements
	 * @param newWidget The widget to configure navigation for
	 * @param comp The component associated with the widget
	 */
	protected void SetupWidgetNavigation(Widget newWidget, CRF_ListBoxElementComponent comp)
	{
		// Set up explicit navigation rules for elements
		// This ensures we can navigate through separators when at the edge of scrolling
		// if there's an element directly above/below the listbox that intercepts focus
		string widgetName = this.GetUniqueWidgetName();
		newWidget.SetName(widgetName);
		
		if (m_aElementComponents.Count() > 1)
		{
			Widget prevWidget = m_aElementComponents[m_aElementComponents.Count() - 2].GetRootWidget();
			prevWidget.SetNavigation(WidgetNavigationDirection.DOWN, WidgetNavigationRuleType.EXPLICIT, newWidget.GetName());
			newWidget.SetNavigation(WidgetNavigationDirection.UP, WidgetNavigationRuleType.EXPLICIT, prevWidget.GetName());
		}
	}
	
	/**
	 * Gets a component at the specified index
	 * @param item Index of the element
	 * @return The component at the specified index or null if out of bounds
	 */
	CRF_ListBoxElementComponent GetCRFElementComponent(int item)
	{
		if (item < 0 || item > m_aElementComponents.Count())
			return null;
		
		return CRF_ListBoxElementComponent.Cast(m_aElementComponents[item]);
	}
}