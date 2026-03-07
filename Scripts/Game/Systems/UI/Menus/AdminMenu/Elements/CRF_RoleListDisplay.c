/**
 * Role List Display Component
 * 
 * A reusable UI component that displays available gear roles/loadouts
 * for admin gear reset functionality. Shows all roles from CRF_EGearRole enum
 * and allows selection for gear reset operations.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the role list widget
 * // In your menu class, get a reference to the component:
 * CRF_RoleListDisplay m_RoleListDisplay;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleListBox0"));
 * m_RoleListDisplay = CRF_RoleListDisplay.Cast(listRoot.FindHandler(CRF_RoleListDisplay));
 * 
 * // Populate the list:
 * m_RoleListDisplay.PopulateRoles();
 * 
 * // Get selected role index:
 * int roleIndex = m_RoleListDisplay.GetSelectedRoleIndex();
 * 
 * // Get selected role name:
 * string roleName = m_RoleListDisplay.GetSelectedRoleName();
 * \endcode
 */
class CRF_RoleListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying roles
	protected SCR_ListBoxComponent m_ListBoxComponent;
	
	//! Cached list of role names
	protected ref array<string> m_aRoleNames = {};
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the list box component from the same widget
		m_ListBoxComponent = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_ListBoxComponent)
		{
			Print("CRF_RoleListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with all available gear roles
	 * Loads role names from CRF_EGearRole enum
	 */
	void PopulateRoles()
	{
		if (!m_ListBoxComponent)
			return;
		
		// Clear existing entries
		m_ListBoxComponent.Clear();
		m_aRoleNames.Clear();
		
		// Get all role names from enum
		SCR_Enum.GetEnumNames(CRF_EGearRole, m_aRoleNames);
		
		// Add each role to the list
		foreach (string roleName : m_aRoleNames)
		{
			m_ListBoxComponent.AddItem(roleName);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the index of the currently selected role
	 * \return Role index (corresponding to CRF_EGearRole enum value), or -1 if nothing selected
	 */
	int GetSelectedRoleIndex()
	{
		if (!m_ListBoxComponent)
			return -1;
		
		return m_ListBoxComponent.GetSelectedItem();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the name of the currently selected role
	 * \return Role name, or empty string if nothing selected
	 */
	string GetSelectedRoleName()
	{
		int selectedIndex = GetSelectedRoleIndex();
		if (selectedIndex < 0 || selectedIndex >= m_aRoleNames.Count())
			return string.Empty;
		
		return m_aRoleNames[selectedIndex];
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the selected role by index
	 * \param index The role index to select
	 */
	void SetSelectedRole(int index)
	{
		if (!m_ListBoxComponent)
			return;
		
		if (index < 0 || index >= m_aRoleNames.Count())
			return;
		
		m_ListBoxComponent.SetItemSelected(index, true);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the selected role by player ID
	 * Selects the role that matches the player's current loadout
	 * \param playerId The player ID to match role for
	 */
	void SetSelectedRoleForPlayer(int playerId)
	{
		if (!m_ListBoxComponent)
			return;
		
		// Get player's current role from slotting manager
		ResourceName playerSlotResource = CRF_SlottingManager.GetInstance().GetPlayerSlotResource(playerId);
		int playerRole = CRF_RoleHelper.ResourceToRole(playerSlotResource);
		
		// Find matching role in list
		for (int i = 0; i < m_aRoleNames.Count(); i++)
		{
			if (i == playerRole)
			{
				m_ListBoxComponent.SetItemSelected(i, true);
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
		
		m_aRoleNames.Clear();
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
	 * Gets the cached array of role names
	 * \return Array of role name strings
	 */
	array<string> GetRoleNames()
	{
		return m_aRoleNames;
	}
}
