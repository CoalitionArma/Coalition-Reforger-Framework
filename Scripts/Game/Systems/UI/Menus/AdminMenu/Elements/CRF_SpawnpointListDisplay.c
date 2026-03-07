/**
 * Spawnpoint List Display Component
 * 
 * A UI component that displays available spawnpoints within a selected group.
 * Spawnpoints are typically players in the group who can be used as spawn locations.
 * The component maintains a synchronized list of player names and their world positions.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the spawnpoint list widget
 * // In your menu class, get a reference to the component:
 * CRF_SpawnpointListDisplay m_SpawnpointList;
 * CRF_GroupListDisplay m_GroupList;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("SpawnpointListBox0"));
 * m_SpawnpointList = CRF_SpawnpointListDisplay.Cast(listRoot.FindHandler(CRF_SpawnpointListDisplay));
 * 
 * // Populate for a specific group:
 * int groupId = m_GroupList.GetSelectedGroupId();
 * m_SpawnpointList.PopulateForGroup(groupId);
 * 
 * // Get selected spawnpoint position:
 * vector spawnPos = m_SpawnpointList.GetSelectedSpawnpoint();
 * 
 * // Get selected player name:
 * string playerName = m_SpawnpointList.GetSelectedPlayerName();
 * \endcode
 */
class CRF_SpawnpointListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying spawnpoints
	protected SCR_ListBoxComponent m_ListBoxComponent;
	
	//! Player manager reference
	protected PlayerManager m_PlayerManager;
	
	//! Slotting manager reference
	protected CRF_SlottingManager m_SlottingManager;
	
	//! Cached list of spawnpoint positions (synchronized with list order)
	protected ref array<vector> m_aSpawnpoints = {};
	
	//! Cached list of player IDs (synchronized with list order)
	protected ref array<int> m_aPlayerIds = {};
	
	//! Currently displayed group ID
	protected int m_iCurrentGroupId = -1;
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the list box component from the same widget
		m_ListBoxComponent = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_ListBoxComponent)
		{
			Print("CRF_SpawnpointListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
		
		// Cache manager references
		m_PlayerManager = GetGame().GetPlayerManager();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with players from a specific group as spawnpoints
	 * Each entry shows player name and caches their world position
	 * \param groupId The group ID to show spawnpoints for
	 */
	void PopulateForGroup(int groupId)
	{
		if (!m_ListBoxComponent || !m_PlayerManager || !m_SlottingManager)
			return;
		
		// Clear existing entries
		Clear();
		
		m_iCurrentGroupId = groupId;
		
		// Get all connected players
		array<int> allPlayers = {};
		m_PlayerManager.GetPlayers(allPlayers);
		
		// Collect and sort player names
		TStringArray playerNames = {};
		foreach (int playerId : allPlayers)
		{
			if (!m_PlayerManager.IsPlayerConnected(playerId))
				continue;
			
			playerNames.Insert(m_PlayerManager.GetPlayerName(playerId));
		}
		
		playerNames.Sort(false);
		
		// Add players from the selected group as spawnpoints
		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (playerId == 0)
				continue;
			
			// Check if player is in the target group
			SCR_AIGroup playerGroup = m_SlottingManager.GetPlayerSlotGroup(playerId);
			if (!playerGroup)
				continue;
			
			if (playerGroup.GetGroupID() != groupId)
				continue;
			
			// Get player entity and position
			IEntity playerEntity = m_PlayerManager.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;
			
			// Add to list
			m_ListBoxComponent.AddItem(name);
			m_aSpawnpoints.Insert(playerEntity.GetOrigin());
			m_aPlayerIds.Insert(playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Updates the list for the current group
	 * Refreshes spawnpoint positions and player list
	 */
	void UpdateList()
	{
		if (m_iCurrentGroupId < 0)
			return;
		
		int selectedIndex = -1;
		if (m_ListBoxComponent)
			selectedIndex = m_ListBoxComponent.GetSelectedItem();
		
		PopulateForGroup(m_iCurrentGroupId);
		
		// Restore selection if valid
		if (selectedIndex >= 0 && m_ListBoxComponent && selectedIndex < m_ListBoxComponent.GetItemCount())
			m_ListBoxComponent.SetItemSelected(selectedIndex, true);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the world position of the currently selected spawnpoint
	 * \return World position vector, or vector.Zero if nothing selected
	 */
	vector GetSelectedSpawnpoint()
	{
		if (!m_ListBoxComponent)
			return vector.Zero;
		
		int selectedIndex = m_ListBoxComponent.GetSelectedItem();
		if (selectedIndex < 0 || selectedIndex >= m_aSpawnpoints.Count())
			return vector.Zero;
		
		return m_aSpawnpoints[selectedIndex];
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the player ID of the currently selected spawnpoint
	 * \return Player ID, or 0 if nothing selected
	 */
	int GetSelectedPlayerId()
	{
		if (!m_ListBoxComponent)
			return 0;
		
		int selectedIndex = m_ListBoxComponent.GetSelectedItem();
		if (selectedIndex < 0 || selectedIndex >= m_aPlayerIds.Count())
			return 0;
		
		return m_aPlayerIds[selectedIndex];
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the player name of the currently selected spawnpoint
	 * \return Player name, or empty string if nothing selected
	 */
	string GetSelectedPlayerName()
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
	 * Checks if there are any spawnpoints available
	 * \return True if spawnpoints exist, false otherwise
	 */
	bool HasSpawnpoints()
	{
		return m_aSpawnpoints.Count() > 0;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the count of available spawnpoints
	 * \return Number of spawnpoints
	 */
	int GetSpawnpointCount()
	{
		return m_aSpawnpoints.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears all items from the list
	 */
	void Clear()
	{
		if (m_ListBoxComponent)
			m_ListBoxComponent.Clear();
		
		m_aSpawnpoints.Clear();
		m_aPlayerIds.Clear();
		m_iCurrentGroupId = -1;
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
	 * Gets the cached array of spawnpoint positions
	 * \return Array of world position vectors
	 */
	array<vector> GetSpawnpoints()
	{
		return m_aSpawnpoints;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the current group ID being displayed
	 * \return Group ID, or -1 if no group selected
	 */
	int GetCurrentGroupId()
	{
		return m_iCurrentGroupId;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Converts a player name to player ID
	 * \param name The player name to look up
	 * \return Player ID, or 0 if not found
	 */
	protected int GetPlayerIdFromName(string name)
	{
		if (!m_PlayerManager)
			return 0;
		
		array<int> playerIds = {};
		m_PlayerManager.GetPlayers(playerIds);
		
		foreach (int pid : playerIds)
		{
			if (m_PlayerManager.GetPlayerName(pid) == name)
				return pid;
		}
		
		return 0;
	}
}
