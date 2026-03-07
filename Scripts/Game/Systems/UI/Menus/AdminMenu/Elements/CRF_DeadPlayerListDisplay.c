/**
 * Dead Player List Display Component
 * 
 * A specialized UI component that displays only dead or spectating players.
 * Used primarily in respawn menus to show which players need respawning.
 * Players are displayed with faction color coding and sorted alphabetically.
 * 
 * Usage:
 * \code
 * // In your menu layout, include the player list widget
 * // In your menu class, get a reference to the component:
 * CRF_DeadPlayerListDisplay m_DeadPlayerList;
 * 
 * // In HandlerAttached or initialization:
 * OverlayWidget listRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("DeadPlayerListBox0"));
 * m_DeadPlayerList = CRF_DeadPlayerListDisplay.Cast(listRoot.FindHandler(CRF_DeadPlayerListDisplay));
 * 
 * // Populate the list:
 * m_DeadPlayerList.PopulateList();
 * 
 * // Get selected player:
 * int playerId = m_DeadPlayerList.GetSelectedPlayerId();
 * 
 * // Check if list is empty:
 * bool hasDeadPlayers = m_DeadPlayerList.HasDeadPlayers();
 * \endcode
 */
class CRF_DeadPlayerListDisplay : SCR_ScriptedWidgetComponent
{
	//! List box component for displaying players
	protected SCR_ListBoxComponent m_ListBoxComponent;
	
	//! Player manager reference
	protected PlayerManager m_PlayerManager;
	
	//! Slotting manager reference
	protected CRF_SlottingManager m_SlottingManager;
	
	//! Cached list of all player IDs
	protected ref array<int> m_aAllPlayers = {};
	
	//! Cached list of dead/spectating player IDs
	protected ref array<int> m_aDeadPlayers = {};
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Get the list box component from the same widget
		m_ListBoxComponent = SCR_ListBoxComponent.Cast(w.FindHandler(SCR_ListBoxComponent));
		if (!m_ListBoxComponent)
		{
			Print("CRF_DeadPlayerListDisplay: Failed to find SCR_ListBoxComponent!", LogLevel.ERROR);
			return;
		}
		
		// Cache manager references
		m_PlayerManager = GetGame().GetPlayerManager();
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Populates the list with dead or spectating players
	 * Players are sorted alphabetically and colored by faction
	 */
	void PopulateList()
	{
		if (!m_ListBoxComponent || !m_PlayerManager || !m_SlottingManager)
			return;
		
		// Clear existing entries
		m_ListBoxComponent.Clear();
		m_aDeadPlayers.Clear();
		
		// Get all connected players
		m_PlayerManager.GetPlayers(m_aAllPlayers);
		
		// Collect and sort player names
		TStringArray playerNames = {};
		foreach (int playerId : m_aAllPlayers)
		{
			if (!m_PlayerManager.IsPlayerConnected(playerId))
				continue;
			
			playerNames.Insert(m_PlayerManager.GetPlayerName(playerId));
		}
		
		// Sort alphabetically
		playerNames.Sort(false);
		
		// Add dead or spectating players to list with faction colors
		foreach (string name : playerNames)
		{
			int playerId = GetPlayerIdFromName(name);
			if (playerId == 0)
				continue;
			
			// Check if player is dead or spectating
			bool isDead = m_SlottingManager.IsPlayerConsideredDead(playerId);
			IEntity playerEntity = m_PlayerManager.GetPlayerControlledEntity(playerId);
			bool isSpectator = CRF_EntityHelper.IsSpectator(playerEntity);
			
			if (isDead || isSpectator)
			{
				// Add to dead players cache
				m_aDeadPlayers.Insert(playerId);
				
				// Get faction and add to list
				Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
				if (!playerFaction)
				{
					m_ListBoxComponent.AddItem(name);
					continue;
				}
				
				m_ListBoxComponent.AddItemWithColor(name, playerFaction.GetFactionColor());
			}
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
	 * Checks if there are any dead or spectating players
	 * \return True if dead players exist, false otherwise
	 */
	bool HasDeadPlayers()
	{
		return m_aDeadPlayers.Count() > 0;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the count of dead/spectating players
	 * \return Number of dead or spectating players
	 */
	int GetDeadPlayerCount()
	{
		return m_aDeadPlayers.Count();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the array of dead player IDs
	 * \return Array of player IDs who are dead or spectating
	 */
	array<int> GetDeadPlayerIds()
	{
		return m_aDeadPlayers;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears all items from the list
	 */
	void Clear()
	{
		if (m_ListBoxComponent)
			m_ListBoxComponent.Clear();
		
		m_aDeadPlayers.Clear();
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
