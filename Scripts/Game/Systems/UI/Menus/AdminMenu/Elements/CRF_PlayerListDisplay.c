/**
 * Player List Display Component
 * 
 * A reusable UI component that displays a filterable list of active players
 * with faction-colored entries. Supports search/filter functionality and
 * excludes spectators by default.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the player list widget
 * // In your menu class, get a reference to the component:
 * CRF_PlayerListDisplay m_PlayerListDisplay;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("PlayerListBox0"));
 * m_PlayerListDisplay = CRF_PlayerListDisplay.Cast(listRoot.FindHandler(CRF_PlayerListDisplay));
 * 
 * // Populate the list:
 * m_PlayerListDisplay.PopulateList();
 * 
 * // Get selected player:
 * int playerId = m_PlayerListDisplay.GetSelectedPlayerId();
 * 
 * // Filter by search term:
 * m_PlayerListDisplay.FilterBySearch("PlayerName");
 * \endcode
 */
class CRF_PlayerListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying players
	protected SCR_ListBoxComponent m_ListBoxComponent;
	
	//! Player manager reference
	protected PlayerManager m_PlayerManager;
	
	//! Groups manager reference
	protected SCR_GroupsManagerComponent m_GroupsManager;
	
	//! Cached list of all player IDs
	protected ref array<int> m_aAllPlayers = {};
	
	//! Whether to exclude spectators from the list
	protected bool m_bExcludeSpectators = true;
	
	//! Whether to require players to be in a group
	protected bool m_bRequireGroup = true;
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the list box component from the same widget
		m_ListBoxComponent = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_ListBoxComponent)
		{
			Print("CRF_PlayerListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
		
		// Cache manager references
		m_PlayerManager = GetGame().GetPlayerManager();
		m_GroupsManager = SCR_GroupsManagerComponent.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with all active players
	 * Players are sorted alphabetically and colored by faction
	 */
	void PopulateList()
	{
		if (!m_ListBoxComponent || !m_PlayerManager)
			return;
		
		// Clear existing entries
		m_ListBoxComponent.Clear();
		
		// Get all connected players
		m_PlayerManager.GetPlayers(m_aAllPlayers);
		
		// Collect and sort player names
		TStringArray playerNames = {};
		foreach (int playerId : m_aAllPlayers)
		{
			// Skip if player should be filtered
			if (!ShouldShowPlayer(playerId))
				continue;
				
			playerNames.Insert(m_PlayerManager.GetPlayerName(playerId));
		}
		
		// Sort alphabetically
		playerNames.Sort(false);
		
		// Add players to list with faction colors
		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (playerId == 0)
				continue;
			
			Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
			if (!playerFaction)
			{
				m_ListBoxComponent.AddItem(name);
				continue;
			}
			
			m_ListBoxComponent.AddItemWithColor(name, playerFaction.GetFactionColor());
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Filters the list based on search text
	 * \param searchTerm Text to filter by (case-insensitive)
	 */
	void FilterBySearch(string searchTerm)
	{
		if (!m_ListBoxComponent || !m_PlayerManager)
			return;
		
		// Clear list
		m_ListBoxComponent.Clear();
		
		// If search is empty, show all players
		if (searchTerm.IsEmpty())
		{
			PopulateList();
			return;
		}
		
		// Filter players by search term
		TStringArray filteredNames = {};
		string searchLower = searchTerm;
		searchLower.ToLower();
		
		foreach (int playerId : m_aAllPlayers)
		{
			// Skip if player should be filtered
			if (!ShouldShowPlayer(playerId))
				continue;
			
			string playerName = m_PlayerManager.GetPlayerName(playerId);
			string playerNameLower = playerName;
			playerNameLower.ToLower();
			
			if (playerNameLower.Contains(searchLower))
				filteredNames.Insert(playerName);
		}
		
		// Sort and add filtered players
		filteredNames.Sort(false);
		foreach (string name : filteredNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (playerId == 0)
				continue;
			
			Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId);
			if (!playerFaction)
			{
				m_ListBoxComponent.AddItem(name);
				continue;
			}
			
			m_ListBoxComponent.AddItemWithColor(name, playerFaction.GetFactionColor());
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the player ID of the currently selected list item
	 * \return Player ID, or 0 if nothing selected or player not found
	 */
	int GetSelectedPlayerId()
	{
		if (!m_ListBoxComponent)
			return 0;
		
		int selectedIndex = m_ListBoxComponent.GetSelectedItem();
		if (selectedIndex < 0)
			return 0;
		
		// Get player name from selected item
		Widget itemWidget = m_ListBoxComponent.GetElementComponent(selectedIndex).GetRootWidget();
		TextWidget textWidget = TextWidget.Cast(itemWidget.FindAnyWidget("Text"));
		if (!textWidget)
			return 0;
		
		string playerName = textWidget.GetText();
		return GetPlayerIdFromName(playerName);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the player name of the currently selected list item
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
	 * Clears all items from the list
	 */
	void Clear()
	{
		if (m_ListBoxComponent)
			m_ListBoxComponent.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets whether to exclude spectators from the list
	 * \param exclude True to hide spectators, false to show all players
	 */
	void SetExcludeSpectators(bool exclude)
	{
		m_bExcludeSpectators = exclude;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets whether to require players to be in a group
	 * \param require True to only show grouped players, false to show all
	 */
	void SetRequireGroup(bool require)
	{
		m_bRequireGroup = require;
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
	 * Determines if a player should be shown in the list based on filters
	 * \param playerId The player ID to check
	 * \return True if player should be shown, false otherwise
	 */
	protected bool ShouldShowPlayer(int playerId)
	{
		// Check if player is connected
		if (!m_PlayerManager.IsPlayerConnected(playerId))
			return false;
		
		// Check group requirement
		if (m_bRequireGroup && m_GroupsManager)
		{
			if (!m_GroupsManager.GetPlayerGroup(playerId))
				return false;
		}
		
		// Check spectator exclusion
		if (m_bExcludeSpectators)
		{
			IEntity playerEntity = m_PlayerManager.GetPlayerControlledEntity(playerId);
			if (CRF_EntityHelper.IsSpectator(playerEntity))
				return false;
		}
		
		return true;
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
