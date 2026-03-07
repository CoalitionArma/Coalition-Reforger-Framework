/**
 * Group List Display Component
 * 
 * A UI component that displays available AI groups with faction tags.
 * Used primarily in respawn menus to show valid spawn groups.
 * Groups are formatted as "FAC | GroupName" (e.g., "BLU | Alpha 1-1").
 * Excludes spectator faction groups.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the group list widget
 * // In your menu class, get a reference to the component:
 * CRF_GroupListDisplay m_GroupListDisplay;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("GroupListBox0"));
 * m_GroupListDisplay = CRF_GroupListDisplay.Cast(listRoot.FindHandler(CRF_GroupListDisplay));
 * 
 * // Populate the list:
 * m_GroupListDisplay.PopulateList();
 * 
 * // Get selected group ID:
 * int groupId = m_GroupListDisplay.GetSelectedGroupId();
 * 
 * // Get selected group:
 * SCR_AIGroup group = m_GroupListDisplay.GetSelectedGroup();
 * 
 * // Select group for specific player:
 * m_GroupListDisplay.SetSelectedGroupForPlayer(playerId);
 * \endcode
 */
class CRF_GroupListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying groups
	protected SCR_ListBoxComponent m_ListBoxComponent;
	
	//! Slotting manager reference
	protected CRF_SlottingManager m_SlottingManager;
	
	//! Cached list of all groups
	protected ref array<SCR_AIGroup> m_aAllGroups = {};
	
	//! Cached list of group IDs matching list order
	protected ref array<int> m_aGroupIds = {};
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the list box component from the same widget
		m_ListBoxComponent = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_ListBoxComponent)
		{
			Print("CRF_GroupListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
		
		// Cache manager references
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with all valid groups
	 * Groups are displayed with faction tag prefix (e.g., "BLU | Alpha 1-1")
	 * Excludes spectator faction groups
	 */
	void PopulateList()
	{
		if (!m_ListBoxComponent || !m_SlottingManager)
			return;
		
		// Clear existing entries
		m_ListBoxComponent.Clear();
		m_aGroupIds.Clear();
		
		// Get all groups from slotting manager
		m_aAllGroups = m_SlottingManager.GetAllGroups();
		
		// Add each valid group to the list
		foreach (SCR_AIGroup group : m_aAllGroups)
		{
			if (!group)
				continue;
			
			// Get faction info
			Faction groupFaction = group.GetFaction();
			if (!groupFaction)
				continue;
			
			string factionKey = groupFaction.GetFactionKey();
			if (factionKey.IsEmpty() || factionKey == "SPEC")
				continue;
			
			// Extract 3-letter faction tag
			string factionTag = factionKey.Substring(0, 3);
			
			// Format: "FAC | GroupName"
			string displayText = string.Format("%1 | %2", factionTag, group.GetCustomNameWithOriginal());
			m_ListBoxComponent.AddItem(displayText);
			
			// Cache group ID for lookup
			m_aGroupIds.Insert(group.GetGroupID());
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Updates the list without clearing selection
	 * Useful for refreshing data while maintaining user selection
	 */
	void UpdateList()
	{
		int selectedIndex = -1;
		if (m_ListBoxComponent)
			selectedIndex = m_ListBoxComponent.GetSelectedItem();
		
		PopulateList();
		
		// Restore selection if valid
		if (selectedIndex >= 0 && m_ListBoxComponent && selectedIndex < m_ListBoxComponent.GetItemCount())
			m_ListBoxComponent.SetItemSelected(selectedIndex, true);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the group ID of the currently selected list item
	 * \return Group ID, or -1 if nothing selected
	 */
	int GetSelectedGroupId()
	{
		if (!m_ListBoxComponent)
			return -1;
		
		int selectedIndex = m_ListBoxComponent.GetSelectedItem();
		if (selectedIndex < 0 || selectedIndex >= m_aGroupIds.Count())
			return -1;
		
		return m_aGroupIds[selectedIndex];
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the SCR_AIGroup object of the currently selected list item
	 * \return The selected group, or null if nothing selected
	 */
	SCR_AIGroup GetSelectedGroup()
	{
		int groupId = GetSelectedGroupId();
		if (groupId < 0)
			return null;
		
		// Find group with matching ID
		foreach (SCR_AIGroup group : m_aAllGroups)
		{
			if (group && group.GetGroupID() == groupId)
				return group;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the selected list index
	 * \return List index, or -1 if nothing selected
	 */
	int GetSelectedIndex()
	{
		if (!m_ListBoxComponent)
			return -1;
		
		return m_ListBoxComponent.GetSelectedItem();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the selected group by group ID
	 * \param groupId The group ID to select
	 */
	void SetSelectedGroupById(int groupId)
	{
		if (!m_ListBoxComponent)
			return;
		
		// Find the index of the group ID
		for (int i = 0; i < m_aGroupIds.Count(); i++)
		{
			if (m_aGroupIds[i] == groupId)
			{
				m_ListBoxComponent.SetItemSelected(i, true);
				return;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the selected group based on a player's current slot group
	 * \param playerId The player ID to match group for
	 */
	void SetSelectedGroupForPlayer(int playerId)
	{
		if (!m_ListBoxComponent || !m_SlottingManager)
			return;
		
		// Get player's slotted group ID
		int playerGroupId = m_SlottingManager.GetPlayerSlotGroup(playerId);
		
		// Find matching group in list
		for (int i = 0; i < m_aGroupIds.Count(); i++)
		{
			if (m_aGroupIds[i] == playerGroupId)
			{
				// Adjust index for client mode if needed
				int itemIndex = i;
				if (RplSession.Mode() == RplMode.Client)
					itemIndex = i - 1;
				
				if (itemIndex >= 0 && itemIndex < m_ListBoxComponent.GetItemCount())
					m_ListBoxComponent.SetItemSelected(itemIndex, true);
				
				return;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears all items from the list
	 */
	void Clear()
	{
		if (m_ListBoxComponent)
			m_ListBoxComponent.Clear();
		
		m_aGroupIds.Clear();
		m_aAllGroups.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the count of groups in the list
	 * \return Number of groups displayed
	 */
	int GetGroupCount()
	{
		return m_aGroupIds.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the underlying list box component
	 * \return The SCR_ListBoxComponent, or null if not initialized
	 */
	SCR_ListBoxComponent GetListBoxComponent()
	{
		return m_ListBoxComponent;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the cached array of group IDs
	 * \return Array of group IDs in display order
	 */
	array<int> GetGroupIds()
	{
		return m_aGroupIds;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the cached array of groups
	 * \return Array of SCR_AIGroup objects
	 */
	array<SCR_AIGroup> GetGroups()
	{
		return m_aAllGroups;
	}
}
