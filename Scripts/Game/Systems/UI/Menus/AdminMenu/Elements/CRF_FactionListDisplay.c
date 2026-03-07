/**
 * Faction List Display Component
 * 
 * A UI component that displays active factions with players.
 * Only shows factions that currently have players assigned to them.
 * Used primarily in hint/message broadcast systems to target specific factions.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the faction list widget
 * // In your menu class, get a reference to the component:
 * CRF_FactionListDisplay m_FactionList;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("FactionListBox0"));
 * m_FactionList = CRF_FactionListDisplay.Cast(listRoot.FindHandler(CRF_FactionListDisplay));
 * 
 * // Populate with active factions:
 * m_FactionList.PopulateActiveFactions();
 * 
 * // Get selected faction:
 * Faction faction = m_FactionList.GetSelectedFaction();
 * 
 * // Get selected faction key:
 * string factionKey = m_FactionList.GetSelectedFactionKey();
 * \endcode
 */
class CRF_FactionListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying factions
	protected SCR_ListBoxComponent m_ListBoxComponent;
	
	//! Faction manager reference
	protected FactionManager m_FactionManager;
	
	//! Cached list of all factions
	protected ref array<Faction> m_aAllFactions = {};
	
	//! Cached list of faction keys (synchronized with list order)
	protected ref array<string> m_aFactionKeys = {};
	
	//! Whether to only show factions with active players
	protected bool m_bOnlyShowActive = true;
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the list box component from the same widget
		m_ListBoxComponent = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_ListBoxComponent)
		{
			Print("CRF_FactionListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
		
		// Cache manager references
		m_FactionManager = GetGame().GetFactionManager();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with factions that have active players
	 * Factions with no players are excluded by default
	 */
	void PopulateActiveFactions()
	{
		if (!m_ListBoxComponent || !m_FactionManager)
			return;
		
		// Clear existing entries
		Clear();
		
		// Get all factions from faction manager
		m_FactionManager.GetFactionsList(m_aAllFactions);
		
		// Add factions that have active players
		foreach (Faction faction : m_aAllFactions)
		{
			if (!faction)
				continue;
			
			// Check if faction has players (if filtering by active)
			if (m_bOnlyShowActive)
			{
				int playerCount = SCR_FactionManager.SGetFactionPlayerCount(faction);
				if (playerCount <= 0)
					continue;
			}
			
			// Add faction to list
			string factionName = faction.GetFactionName();
			m_ListBoxComponent.AddItem(factionName);
			
			// Cache faction key for lookup
			m_aFactionKeys.Insert(faction.GetFactionKey());
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with all factions regardless of player count
	 */
	void PopulateAllFactions()
	{
		bool previousSetting = m_bOnlyShowActive;
		m_bOnlyShowActive = false;
		PopulateActiveFactions();
		m_bOnlyShowActive = previousSetting;
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
		
		PopulateActiveFactions();
		
		// Restore selection if valid
		if (selectedIndex >= 0 && m_ListBoxComponent && selectedIndex < m_ListBoxComponent.GetItemCount())
			m_ListBoxComponent.SetItemSelected(selectedIndex, true);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the Faction object of the currently selected list item
	 * \return The selected faction, or null if nothing selected
	 */
	Faction GetSelectedFaction()
	{
		string factionKey = GetSelectedFactionKey();
		if (factionKey.IsEmpty())
			return null;
		
		// Find faction with matching key
		foreach (Faction faction : m_aAllFactions)
		{
			if (faction && faction.GetFactionKey() == factionKey)
				return faction;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the faction key of the currently selected list item
	 * \return Faction key, or empty string if nothing selected
	 */
	string GetSelectedFactionKey()
	{
		if (!m_ListBoxComponent)
			return string.Empty;
		
		int selectedIndex = m_ListBoxComponent.GetSelectedItem();
		if (selectedIndex < 0 || selectedIndex >= m_aFactionKeys.Count())
			return string.Empty;
		
		return m_aFactionKeys[selectedIndex];
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the faction name of the currently selected list item
	 * \return Faction name, or empty string if nothing selected
	 */
	string GetSelectedFactionName()
	{
		if (!m_ListBoxComponent)
			return string.Empty;
		
		int selectedIndex = m_ListBoxComponent.GetSelectedItem();
		if (selectedIndex < 0)
			return string.Empty;
		
		Widget itemWidget = m_ListBoxComponent.GetElementComponent(selectedIndex).GetRootWidget();
		TextWidget textWidget = TextWidget.Cast(itemWidget.FindAnyWidget("Text"));
		if (!textWidget)
			return string.Empty;
		
		return textWidget.GetText();
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
	 * Sets the selected faction by faction key
	 * \param factionKey The faction key to select
	 */
	void SetSelectedFactionByKey(string factionKey)
	{
		if (!m_ListBoxComponent)
			return;
		
		// Find the index of the faction key
		for (int i = 0; i < m_aFactionKeys.Count(); i++)
		{
			if (m_aFactionKeys[i] == factionKey)
			{
				m_ListBoxComponent.SetItemSelected(i, true);
				return;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets whether to only show factions with active players
	 * \param onlyActive True to filter by active players, false to show all
	 */
	void SetOnlyShowActive(bool onlyActive)
	{
		m_bOnlyShowActive = onlyActive;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the count of factions in the list
	 * \return Number of factions displayed
	 */
	int GetFactionCount()
	{
		return m_aFactionKeys.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Checks if there are any factions in the list
	 * \return True if factions exist, false otherwise
	 */
	bool HasFactions()
	{
		return m_aFactionKeys.Count() > 0;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears all items from the list
	 */
	void Clear()
	{
		if (m_ListBoxComponent)
			m_ListBoxComponent.Clear();
		
		m_aFactionKeys.Clear();
		m_aAllFactions.Clear();
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
	 * Gets the cached array of faction keys
	 * \return Array of faction key strings
	 */
	array<string> GetFactionKeys()
	{
		return m_aFactionKeys;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the cached array of factions
	 * \return Array of Faction objects
	 */
	array<Faction> GetFactions()
	{
		return m_aAllFactions;
	}
}
