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
	 * Gets the slotted status widget
	 * @return FrameWidget for the slotted status or null if not found
	 */
	Widget GetSlottedWidget()
	{
		Widget widget = Widget.Cast(m_wRoot.FindAnyWidget("Slotted"));
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
	 * Sets the community tag text (yellow label before the player name).
	 * Pass an empty string to hide the tag.
	 * @param tag The raw tag string e.g. "CRF" — brackets are added automatically.
	 */
	void SetTagText(string tag)
	{
		// Tag widget may live in CRF slot/orbat layouts ("TagName")
		// or in the plain player-list layouts which share the same component.
		TextWidget tagWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("TagName"));
		if (!tagWidget)
			return;

		if (tag.IsEmpty())
		{
			tagWidget.SetText("");
			return;
		}

		tagWidget.SetText(string.Format("[%1] ", tag));
		tagWidget.SetColor(Color.FromRGBA(255, 220, 0, 255)); // bright yellow
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Shows or hides the rank chevron image based on the player's XP and chosen rank track.
	 * Pass xp = -1 to hide the chevron (player has no data).
	 * @param xp    The player's total XP from the Coalition database.
	 * @param track The player's chosen rank track: "enlisted", "warrant", or "officer". Defaults to "enlisted".
	 */
	void SetRankChevron(int xp, string track = "enlisted")
	{
		ImageWidget chevron = ImageWidget.Cast(m_wRoot.FindAnyWidget("RankChevron"));
		if (!chevron)
			return;

		ResourceName texture = GetRankTexture(xp, track);
		if (texture.IsEmpty())
		{
			chevron.SetVisible(false);
			return;
		}

		chevron.SetVisible(true);
		chevron.LoadImageTexture(0, texture);
		chevron.SetImage(0);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Dispatches to the correct per-track texture lookup based on the track string.
	 * Warrant and officer tracks always show at minimum W-1/O-1 (even when xp is
	 * unknown/negative), because their lowest grade still has insignia.
	 * Only enlisted hides the chevron for E-1 (xp < 15000) or unknown xp (< 0).
	 */
	protected ResourceName GetRankTexture(int xp, string track)
	{
		if (track == "warrant")
			return GetWarrantRankTexture(xp);
		if (track == "officer")
			return GetOfficerRankTexture(xp);

		// Enlisted: hide chevron when XP is unknown (< 0) or at E-1 (< 15000)
		if (xp < 0)
			return ResourceName.Empty;
		return GetEnlistedRankTexture(xp);
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Enlisted track: E1 (no icon) through E9a/b/c.
	 * Thresholds mirror CRF_CommunityTagManager RANK_XP_E* constants.
	 */
	protected ResourceName GetEnlistedRankTexture(int xp)
	{
		if (xp >= 800000) return "{5600F59E472DBA19}UI/Images/Ranks/rank_E9c.edds"; // E-9c SMA
		if (xp >= 600000) return "{A208B2064AF0D26D}UI/Images/Ranks/rank_E9b.edds"; // E-9b CSM
		if (xp >= 450000) return "{FCE09B45F57D5C62}UI/Images/Ranks/rank_E9a.edds"; // E-9a SGM
		if (xp >= 325000) return "{65C4229BBFA69161}UI/Images/Ranks/rank_E8b.edds"; // E-8b 1SG
		if (xp >= 225000) return "{3B2C0BD8002B1F6E}UI/Images/Ranks/rank_E8a.edds"; // E-8a MSG
		if (xp >= 150000) return "{6FF3DCD212537B87}UI/Images/Ranks/rank_E7.edds";  // E-7  SFC
		if (xp >= 100000) return "{9BFB9B4A1F8E13F3}UI/Images/Ranks/rank_E6.edds";  // E-6  SSG
		if (xp >= 75000)  return "{C513B209A0039DFC}UI/Images/Ranks/rank_E5.edds";  // E-5  SGT
		if (xp >= 55000)  return "{4F5163AD67E55F7D}UI/Images/Ranks/rank_E4b.edds"; // E-4b CPL
		if (xp >= 40000)  return "{11B94AEED868D172}UI/Images/Ranks/rank_E4a.edds"; // E-4a SPC
		if (xp >= 25000)  return "{78C3E08EDF1881E2}UI/Images/Ranks/rank_E3.edds";  // E-3  PFC
		if (xp >= 15000)  return "{8CCBA716D2C5E996}UI/Images/Ranks/rank_E2.edds";  // E-2  PV2
		// E-1 PVT: no insignia
		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Warrant Officer track: W1–W5 (all grades have insignia).
	 */
	protected ResourceName GetWarrantRankTexture(int xp)
	{
		if (xp >= 650000) return "{AFE6D1C9A4E628D6}UI/Images/Ranks/rank_W5.edds"; // W-5
		if (xp >= 400000) return "{5BEE9651A93B40A2}UI/Images/Ranks/rank_W4.edds"; // W-4
		if (xp >= 200000) return "{1236834EDBFD34C8}UI/Images/Ranks/rank_W3.edds"; // W-3
		if (xp >= 75000)  return "{E63EC4D6D6205CBC}UI/Images/Ranks/rank_W2.edds"; // W-2
		return             "{B8D6ED9569ADD2B3}UI/Images/Ranks/rank_W1.edds";        // W-1
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Commissioned Officer track: O1–O11 (all grades have insignia).
	 */
	protected ResourceName GetOfficerRankTexture(int xp)
	{
		if (xp >= 1000000) return "{B30621CD1B8E9D61}UI/Images/Ranks/rank_O11.edds"; // O-11
		if (xp >= 800000)  return "{470E66551653F515}UI/Images/Ranks/rank_O10.edds"; // O-10
		if (xp >= 650000)  return "{C39C174143BDBA41}UI/Images/Ranks/rank_O9.edds";  // O-9
		if (xp >= 500000)  return "{379450D94E60D235}UI/Images/Ranks/rank_O8.edds";  // O-8
		if (xp >= 350000)  return "{502C3D7FA6315295}UI/Images/Ranks/rank_O7.edds";  // O-7
		if (xp >= 225000)  return "{A4247AE7ABEC3AE1}UI/Images/Ranks/rank_O6.edds";  // O-6
		if (xp >= 150000)  return "{FACC53A41461B4EE}UI/Images/Ranks/rank_O5.edds";  // O-5
		if (xp >= 100000)  return "{0EC4143C19BCDC9A}UI/Images/Ranks/rank_O4.edds";  // O-4
		if (xp >= 60000)   return "{471C01236B7AA8F0}UI/Images/Ranks/rank_O3.edds";  // O-3
		if (xp >= 30000)   return "{B31446BB66A7C084}UI/Images/Ranks/rank_O2.edds";  // O-2
		return              "{EDFC6FF8D92A4E8B}UI/Images/Ranks/rank_O1.edds";        // O-1
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
	
	/**
	 * Gets the player name text
	 */
	TextWidget GetPlayerText()
	{
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerName"));
		if (textWidget)
			return textWidget;
			
		return null;
	}
	
	/**
	 * Gets the role name text
	 */
	TextWidget GetRoleText()
	{
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("RoleName"));
		if (textWidget)
			return textWidget;
			
		return null;
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
	 * Gets the ImageWidget for the group
	 * @return ImageWidget instance or null if not found
	 */
	ImageWidget GetGroupIcon()
	{
		ImageWidget symbolImage = ImageWidget.Cast(m_wRoot.FindAnyWidget("SymbolImage"));
		if (symbolImage)
			return symbolImage;
			
		return null;
	}
	
		/**
	 * Gets the Group text for the group
	 * @return TextWidget instance or null if not found
	 */
	TextWidget GetGroupText()
	{
		TextWidget textWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("RolesGroupName"));
		if (textWidget)
			return textWidget;
			
		return null;
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