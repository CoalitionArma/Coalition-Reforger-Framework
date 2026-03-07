/**
 * Search Box Component
 * 
 * A reusable UI component that provides search/filter functionality for list boxes.
 * Typically used in conjunction with CRF_PlayerListDisplay or other list components
 * to provide real-time filtering.
 * 
 * Usage:
 * \code
 * // In your menu layout, include an EditBoxWidget with the search box
 * // In your menu class, get references to the component and target list:
 * CRF_SearchBoxComponent m_SearchBox;
 * CRF_PlayerListDisplay m_PlayerList;
 * 
 * // In HandlerAttached or initialization:
 * Widget searchWidget = m_wRoot.FindAnyWidget("SearchBox0");
 * m_SearchBox = CRF_SearchBoxComponent.Cast(searchWidget.FindHandler(CRF_SearchBoxComponent));
 * 
 * // Setup with target list display:
 * m_SearchBox.SetTargetListDisplay(m_PlayerList);
 * 
 * // Or manually trigger search:
 * string searchTerm = m_SearchBox.GetSearchText();
 * m_PlayerList.FilterBySearch(searchTerm);
 * \endcode
 */
class CRF_SearchBoxComponent : SCR_ScriptedWidgetComponent
{
	//! The edit box widget for text input
	protected EditBoxWidget m_EditBoxWidget;
	
	//! Target player list to filter (optional)
	protected CRF_PlayerListDisplay m_TargetPlayerList;
	
	//! Button to trigger search
	protected SCR_ButtonTextComponent m_SearchButton;
	
	//! Last search term used
	protected string m_sLastSearchTerm = string.Empty;
	
	//------------------------------------------------------------------------------------------------
	//! Initializes the component and caches references
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		// Try to find EditBoxWidget
		m_EditBoxWidget = EditBoxWidget.Cast(w);
		if (!m_EditBoxWidget)
		{
			Print("CRF_SearchBoxComponent: Widget is not an EditBoxWidget!", LogLevel.WARNING);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the target player list display to filter
	 * \param targetList The player list component to filter when searching
	 */
	void SetTargetPlayerList(CRF_PlayerListDisplay targetList)
	{
		m_TargetPlayerList = targetList;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the search button that triggers filtering
	 * \param button The button component that triggers search
	 */
	void SetSearchButton(SCR_ButtonTextComponent button)
	{
		if (m_SearchButton)
			m_SearchButton.m_OnClicked.Remove(ExecuteSearch);
		
		m_SearchButton = button;
		
		if (m_SearchButton)
			m_SearchButton.m_OnClicked.Insert(ExecuteSearch);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the current search text from the edit box
	 * \return The search text, or empty string if no edit box
	 */
	string GetSearchText()
	{
		if (!m_EditBoxWidget)
			return string.Empty;
		
		return m_EditBoxWidget.GetText();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Sets the search text in the edit box
	 * \param text The text to set
	 */
	void SetSearchText(string text)
	{
		if (m_EditBoxWidget)
			m_EditBoxWidget.SetText(text);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Clears the search text
	 */
	void Clear()
	{
		SetSearchText(string.Empty);
		m_sLastSearchTerm = string.Empty;
		
		// Refresh target list to show all items
		if (m_TargetPlayerList)
			m_TargetPlayerList.PopulateList();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Executes the search/filter on the target list
	 * Called when search button is clicked or Enter is pressed
	 */
	void ExecuteSearch()
	{
		if (!m_TargetPlayerList)
			return;
		
		string searchTerm = GetSearchText();
		m_sLastSearchTerm = searchTerm;
		
		// Filter the target list
		m_TargetPlayerList.FilterBySearch(searchTerm);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the last search term that was executed
	 * \return The last search term
	 */
	string GetLastSearchTerm()
	{
		return m_sLastSearchTerm;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Gets the underlying edit box widget
	 * \return The EditBoxWidget, or null if not initialized
	 */
	EditBoxWidget GetEditBoxWidget()
	{
		return m_EditBoxWidget;
	}
}
