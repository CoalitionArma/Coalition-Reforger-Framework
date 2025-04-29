class CRF_ListBoxElementComponent: SCR_ListBoxElementComponent
{
	// Group and player properties
	SCR_AIGroup group;                   // Reference to the AI group this element represents
	int m_iSlotId;                       // ID of the slot this element occupies
	bool isGroupLocked;                  // Flag indicating if group is locked
	bool m_bIsPlayer = false;            // Flag indicating if element represents a player
	int m_iChannelId;                    // Communication channel ID
	CRF_Gamemode m_Gamemode = CRF_Gamemode.GetInstance();  // Game mode instance reference
	bool m_bDeleteRequest = false;       // Pending delete request flag
	
	//------------------------------------------------------------------------------------------------
	// UI Component Access Methods
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Gets the Accept button component
	 * @return SCR_ButtonTextComponent for the Accept button
	 */
	SCR_ButtonTextComponent GetAccept()
	{
		Widget acceptWidget = m_wRoot.FindAnyWidget("Accept");
		if (!acceptWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(acceptWidget.FindHandler(SCR_ButtonTextComponent));
	}
	
	/**
	 * Gets the Deny button component
	 * @return SCR_ButtonTextComponent for the Deny button
	 */
	SCR_ButtonTextComponent GetDeny()
	{
		Widget denyWidget = m_wRoot.FindAnyWidget("Deny");
		if (!denyWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(denyWidget.FindHandler(SCR_ButtonTextComponent));
	}
	
	/**
	 * Gets the progress bar widget
	 * @return ProgressBarWidget instance
	 */
	ProgressBarWidget GetProgress()
	{
		return ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("ProgressBar"));
	}
	
	/**
	 * Gets the disconnected status widget
	 * @return FrameWidget for the disconnected status or null if not found
	 */
	FrameWidget GetDisconnectWidget()
	{
		FrameWidget widget = FrameWidget.Cast(m_wRoot.FindAnyWidget("Disconnected"));
		if (widget)
			return widget;
		return null;
	}
	
	/**
	 * Gets the killed status widget
	 * @return FrameWidget for the killed status or null if not found
	 */
	FrameWidget GetDeathWidget()
	{
		FrameWidget widget = FrameWidget.Cast(m_wRoot.FindAnyWidget("Killed"));
		if (widget)
			return widget;
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	// Text Setting Methods
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Sets the role text display
	 * @param text The role name to display
	 */
	void SetRoleText(string text)
	{
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("RoleName"));
		if (textWidget)
			textWidget.SetText(text);
	}
	
	/**
	 * Sets the player name text
	 * @param text The player name to display
	 */
	void SetPlayerText(string text)
	{
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerName"));
		if (textWidget)
			textWidget.SetText(text);
	}
	
	/**
	 * Sets the group name text
	 * @param text Group name to display
	 */
	void SetGroupName(string text)
	{
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("RolesGroupName"));
		if (textWidget)
			textWidget.SetText(text);
	}
	
	/**
	 * Gets the channel button component
	 * @return SCR_ButtonTextComponent for the channel button
	 */
	SCR_ButtonTextComponent GetChannelButton()
	{
		Widget slotButtonWidget = m_wRoot.FindAnyWidget("SlotButton");
		if (!slotButtonWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(slotButtonWidget.FindHandler(SCR_ButtonTextComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	// Image Setting Methods
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Sets the role image
	 * @param imageOrImageset Resource path to image or imageset
	 * @param iconName Icon name within imageset (if applicable)
	 */
	void SetRoleImage(ResourceName imageOrImageset, string iconName)
	{
		// Return if no image resource is provided
		if (imageOrImageset.IsEmpty())
			return;
		
		ImageWidget imageWidget = ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleImage"));
		if (!imageWidget)
			return;
			
		// Handle either imageset or direct texture
		if (imageOrImageset.EndsWith("imageset"))
		{
			if (!iconName.IsEmpty())
				imageWidget.LoadImageFromSet(0, imageOrImageset, iconName);
		}
		else
		{
			imageWidget.LoadImageTexture(0, imageOrImageset);
		}
	}
	
	/**
	 * Sets the lock status image
	 * @param imageOrImageset Resource path to image or imageset
	 * @param iconName Icon name within imageset (if applicable)
	 */
	void SetLockImage(ResourceName imageOrImageset, string iconName)
	{
		// Return if no image resource is provided
		if (imageOrImageset.IsEmpty())
			return;
		
		ImageWidget imageWidget = ImageWidget.Cast(m_wRoot.FindAnyWidget("LockImage"));
		if (!imageWidget)
			return;
			
		// Handle either imageset or direct texture
		if (imageOrImageset.EndsWith("imageset"))
		{
			if (!iconName.IsEmpty())
				imageWidget.LoadImageFromSet(0, imageOrImageset, iconName);
		}
		else
		{
			imageWidget.LoadImageTexture(0, imageOrImageset);
		}
	}
	
	/**
	 * Sets the color for role-related UI elements
	 * @param color Color to apply to the elements
	 */
	void SetRoleColor(Color color)
	{
		ImageWidget roleImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleImage"));
		if (roleImage)
			roleImage.SetColor(color);
			
		ImageWidget divider1 = ImageWidget.Cast(m_wRoot.FindAnyWidget("Divider1"));
		if (divider1)
			divider1.SetColor(color);
			
		ImageWidget divider2 = ImageWidget.Cast(m_wRoot.FindAnyWidget("Divider2"));
		if (divider2)
			divider2.SetColor(color);
	}
	
	//------------------------------------------------------------------------------------------------
	// Group Related Methods
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Gets the military symbol UI component for the group
	 * @return SCR_MilitarySymbolUIComponent instance or null if not found
	 */
	SCR_MilitarySymbolUIComponent GetGroupIcon()
	{
		Widget symbolOverlay = m_wRoot.FindAnyWidget("SymbolOverlay");
		if (!symbolOverlay)
			return null;
			
		return SCR_MilitarySymbolUIComponent.Cast(symbolOverlay.FindHandler(SCR_MilitarySymbolUIComponent));
	}
	
	/**
	 * Sets the color for the group icon
	 * @param color Color to apply to the group icon
	 */
	void SetGroupIconColor(Color color)
	{
		ImageWidget roleImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleImage"));
		if (roleImage)
			roleImage.SetColor(color);
	}
	
	/**
	 * Gets the group widget
	 * @return Widget for the group or null if not found
	 */
	Widget GetGroupWidget()
	{
		Widget widget = m_wRoot.FindAnyWidget("SymbolOverlay");
		if (widget)
			return widget;
			
		return null;
	}
	
	/**
	 * Gets the group underline widget
	 * @return Widget for the group underline or null if not found
	 */
	Widget GetGroupUnderline()
	{
		Widget widget = m_wRoot.FindAnyWidget("Underline");
		if (widget)
			return widget;
			
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	// Button Management Methods
	//------------------------------------------------------------------------------------------------
	
	/**
	 * Gets the slot button component
	 * @return SCR_ButtonTextComponent for the slot button or null if not found
	 */
	SCR_ButtonTextComponent GetSlotButton()
	{
		ButtonWidget buttonWidget = ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlotButton"));
		if (!buttonWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
	}
	
	/**
	 * Gets the lock button component
	 * @return SCR_ButtonTextComponent for the lock button or null if not found
	 */
	SCR_ButtonTextComponent GetLockButton()
	{
		ButtonWidget buttonWidget = ButtonWidget.Cast(m_wRoot.FindAnyWidget("LockButton"));
		if (!buttonWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
	}
	
	/**
	 * Gets the kick button component
	 * @return SCR_ButtonTextComponent for the kick button or null if not found
	 */
	SCR_ButtonTextComponent GetKickButton()
	{
		ButtonWidget buttonWidget = ButtonWidget.Cast(m_wRoot.FindAnyWidget("KickButton"));
		if (!buttonWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
	}
	
	/**
	 * Disables the kick button and hides its icon
	 */
	void DisableKickButton()
	{
		ButtonWidget button = ButtonWidget.Cast(m_wRoot.FindAnyWidget("KickButton"));
		if (button)
			button.SetEnabled(false);
			
		ImageWidget image = ImageWidget.Cast(m_wRoot.FindAnyWidget("KickImage"));
		if (image)
			image.SetColor(Color.FromRGBA(0, 0, 0, 0));
	}
}