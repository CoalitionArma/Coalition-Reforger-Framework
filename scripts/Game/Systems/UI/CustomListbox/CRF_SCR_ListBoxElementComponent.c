modded class SCR_ListBoxElementComponent
{
	//-------------------------------------------------------------
	// Member Variables
	//-------------------------------------------------------------
	
	//! Player identifier used for tracking player-specific elements
	int m_iPlayerId;
	
	//! Index for descriptive text or information related to this element
	int m_iDescriptionIndex;
	
	//-------------------------------------------------------------
	// Public Methods
	//-------------------------------------------------------------
	
	/**
	 * Sets the text color for this list box element
	 * @param color The color to apply to the text widget
	 */
	void SetColor(Color color)
	{
		// Find the text widget using the configured widget name
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget(m_sWidgetTextName));
		
		// Only set the color if we found a valid text widget
		if (textWidget)
		{
			textWidget.SetColor(color);
		}
	}
	
	/**
	 * Sets the description index for this element
	 * @param input The index value to set
	 */
	void SetDescriptionIndex(int input)
	{
		m_iDescriptionIndex = input;
	}
	
	/**
	 * Retrieves the selection button component associated with this element
	 * @return The button component if found, null otherwise
	 */
	SCR_ButtonTextComponent GetSelectButton()
	{
		// Find the player button widget
		ButtonWidget buttonWidget = ButtonWidget.Cast(m_wRoot.FindAnyWidget("PlayerButton"));
		
		// If button doesn't exist, return null
		if (!buttonWidget)
		{
			return null;
		}
		
		// Find the button component handler
		SCR_ButtonTextComponent buttonComponent = SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
		
		// Return the component (or null if not found)
		return buttonComponent;
	}
}