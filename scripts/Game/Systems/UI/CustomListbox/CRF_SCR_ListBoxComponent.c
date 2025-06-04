modded class SCR_ListBoxComponent
{
	/**
	 * Adds an item with an icon to the listbox and associates it with a player.
	 * 
	 * @param item The text to display for this list item
	 * @param imageOrImageset Resource name of the image or imageset to use
	 * @param iconName Name of the icon within the imageset
	 * @param data Custom data to associate with this item (optional)
	 * @param itemLayout Custom layout for this item (optional)
	 * @param playerId ID of the player associated with this item
	 * @return The index of the newly added item
	 */
	int AddItemAndIconPlayer(string item, ResourceName imageOrImageset, string iconName, Managed data = null, ResourceName itemLayout = string.Empty, int playerId = 0)
	{
		// Component that will handle the listbox element
		SCR_ListBoxElementComponent comp;
		
		// Add the item to the listbox and get its ID
		int id = _AddItem(item, data, comp, itemLayout);
		
		// Set the icon for this item and only for the slotted list
		comp.SetImage(imageOrImageset, iconName);
		
		// Associate the item with a player ID
		comp.m_iPlayerId = playerId;
		
		// Return the ID of the newly added item
		return id;
	}
	
	/**
	 * Adds an item with color to the listbox.
	 * 
	 * @param item The text to display for this list item
	 * @param color of the text
	 * @param data Custom data to associate with this item (optional)
	 * @param itemLayout Custom layout for this item (optional)
	 * @return The index of the newly added item
	 */
	int AddItemWithColor(string item, Color color, Managed data = null, ResourceName itemLayout = string.Empty)
	{
		// Component that will handle the listbox element
		SCR_ListBoxElementComponent comp;
		
		// Add the item to the listbox and get its ID
		int id = _AddItem(item, data, comp, itemLayout);
	
		// Set text color
		TextWidget label = TextWidget.Cast(comp.GetRootWidget().FindAnyWidget("Text"));
		if (label)
			label.SetColor(color);
	
		// Return the ID of the newly added item
		return id;
	}
}