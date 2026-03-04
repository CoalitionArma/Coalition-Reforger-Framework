/**
 * CRF_PlayerStatusListDisplay
 *
 * Modular widget component that manages the connected-player list shown in
 * the Slotting and AAR menus. Handles:
 *   - Sorted rendering (admins first, then regular players)
 *   - Per-player status colouring (admin / moderator / donator / selected)
 *   - VON "talking" indicator
 *   - Faction icon per slotted player
 *
 * The component owns no list-box widget directly; instead the owning menu
 * passes in its already-resolved SCR_ListBoxComponent via Init() and then
 * calls UpdatePlayerList() each frame.
 *
 * Usage:
 *   Widget playerListRoot = m_wRoot.FindAnyWidget("PlayerListDisplay");
 *   m_PlayerListDisplay = CRF_PlayerStatusListDisplay.Cast(
 *       playerListRoot.FindHandler(CRF_PlayerStatusListDisplay));
 *
 *   // After resolving your list-box component:
 *   m_PlayerListDisplay.Init(m_cPlayerListBoxComponent, m_VONController);
 *
 *   // In OnMenuUpdate:
 *   m_PlayerListDisplay.UpdatePlayerList(selectedPlayerId, factionIconGetter);
 */
class CRF_PlayerStatusListDisplay : SCR_ScriptedWidgetComponent
{
	protected SCR_ListBoxComponent   m_cListBox;
	protected SCR_VONController      m_VONController;

	protected static const string EMPTY_RESOURCE =
		"{2E717F4664C6E49D}UI/Textures/Nametags/Nametag-Filter-Icons/Player.edds";
	protected static const string PLAYER_ELEMENT_LAYOUT =
		"{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout";

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		// Widget fields not needed — the list-box is supplied by the parent menu.
	}

	/**
	 * Provides the resolved list-box component and optional VON controller.
	 * Call once after the menu has initialised its list components.
	 *
	 * @param listBox     Resolved SCR_ListBoxComponent to write into.
	 * @param vonCtrl     Optional VON controller for talking indicators.
	 */
	void Init(SCR_ListBoxComponent listBox, SCR_VONController vonCtrl = null)
	{
		m_cListBox      = listBox;
		m_VONController = vonCtrl;
	}

	/**
	 * Clears and rebuilds the player list sorted by admin status.
	 * Call every frame from the owning menu's OnMenuUpdate.
	 *
	 * @param selectedPlayerId   The currently highlighted player ID (yellow).
	 * @param factionIconGetter  Delegate/helper — pass null to skip faction icons.
	 *                           The component will attempt GetFactionIcon on
	 *                           the CRF_SlottingManager slot data directly.
	 */
	void UpdatePlayerList(int selectedPlayerId)
	{
		if (!m_cListBox)
			return;

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);

		m_cListBox.Clear();

		// Admins first
		foreach (int playerId : playerIds)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
			if (!SCR_Global.IsAdmin(playerId))
				continue;
			AddPlayerEntry(playerId, selectedPlayerId);
		}

		// Then everyone else
		foreach (int playerId : playerIds)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
			if (SCR_Global.IsAdmin(playerId))
				continue;
			AddPlayerEntry(playerId, selectedPlayerId);
		}
	}

	/**
	 * Sets a single list element's color based on the player's permission level
	 * and selection state. May be called externally to colorise unslotted lists.
	 */
	void ApplyStatusColor(int playerId, int selectedPlayerId, SCR_ListBoxElementComponent comp)
	{
		if (!comp)
			return;

		if (playerId == selectedPlayerId)
		{
			comp.SetColor(Color.DarkYellow);
			return;
		}

		if (SCR_Global.IsAdmin(playerId))
		{
			comp.SetColor(Color.Red);
			return;
		}

		CRF_PermissionManager permMgr = CRF_PermissionManager.GetInstance();
		if (permMgr)
		{
			if (permMgr.IsModerator(playerId))
			{
				comp.SetColor(Color.Yellow);
				return;
			}
			if (permMgr.IsDonator(playerId))
			{
				comp.SetColor(Color.Violet);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddPlayerEntry(int playerId, int selectedPlayerId)
	{
		ResourceName iconResource = ResolvePlayerFactionIcon(playerId);

		int listIndex = m_cListBox.AddItemAndIconPlayer(
			GetGame().GetPlayerManager().GetPlayerName(playerId),
			iconResource,
			"flag",
			null,
			PLAYER_ELEMENT_LAYOUT);

		SCR_ListBoxElementComponent comp = m_cListBox.GetElementComponent(listIndex);
		ApplyStatusColor(playerId, selectedPlayerId, comp);
		ApplyVONIndicator(playerId, comp);
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName ResolvePlayerFactionIcon(int playerId)
	{
		Faction playerFaction = CRF_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId, true);
		if (!playerFaction)
			return EMPTY_RESOURCE;

		// Ask the slotting manager's faction map for the gearscript icon, falling
		// back to the vanilla faction flag.
		string factionKey = playerFaction.GetFactionKey();
		ResourceName gearScriptResource = CRF_Gamemode.GetInstance().GetGearScriptResource(factionKey);
		if (!gearScriptResource.IsEmpty())
		{
			CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(
				BaseContainerTools.CreateInstanceFromContainer(
					BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
			if (gearConfig && !gearConfig.m_FactionIcon.IsEmpty())
				return gearConfig.m_FactionIcon;
		}

		SCR_Faction scrFaction = SCR_Faction.Cast(playerFaction);
		if (scrFaction)
			return scrFaction.GetFactionFlag();

		return EMPTY_RESOURCE;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyVONIndicator(int playerId, SCR_ListBoxElementComponent comp)
	{
		if (!comp)
			return;

		if (!CVON_VONGameModeComponent.GetInstance())
		{
			// Legacy VON path — check the MenuManager's talking list
			CRF_MenuManager menuMgr = CRF_MenuManager.GetInstance();
			if (menuMgr && menuMgr.m_aPlayersTalking.Contains(playerId))
				comp.SetTalking();
		}
		else
		{
			// CVON path — only apply to the local player
			if (playerId == SCR_PlayerController.GetLocalPlayerId()
				&& m_VONController
				&& m_VONController.m_bIsBroadcasting)
			{
				comp.SetTalking();
			}
		}
	}
}
