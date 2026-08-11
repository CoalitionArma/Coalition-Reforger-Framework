modded class COA_SlottingMenu
{	
    /**
	 * Called when the menu is opened
	 * Initializes UI elements and sets up event listeners
	 */
	override void OnMenuOpen()
	{	
		super.OnMenuOpen();

		// Don't open this menu on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}

		// Fetch community tags + ranks and register for updates when they arrive
		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (tagMgr)
		{
			tagMgr.FetchPlayerInfo();
			tagMgr.GetOnPlayerInfoUpdated().Remove(RefreshTagsAndRanks);
			tagMgr.GetOnPlayerInfoUpdated().Insert(RefreshTagsAndRanks);
			tagMgr.GetOnPlayerRosterChanged().Remove(OnPlayerRosterChanged);
			tagMgr.GetOnPlayerRosterChanged().Insert(OnPlayerRosterChanged);
		}
    }

	/**
	 * Called when menu is closed
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();

		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (tagMgr)
		{
			tagMgr.GetOnPlayerInfoUpdated().Remove(RefreshTagsAndRanks);
			tagMgr.GetOnPlayerRosterChanged().Remove(OnPlayerRosterChanged);
		};
    }

	/**
	 * Surgically updates a single slot element when only its player ID has changed.
	 * Subscribed to GetOnSlotChanged() so only player-claim/vacate events trigger this —
	 * lock, death, group, and role deltas still fall through to UpdateSlots() via GetOnSlottingUpdate().
	 * Eliminates the full Clear+rebuild that previously fired on every client for every slot click.
	 */
	override void UpdateSlotInPlace()
	{
		int slotId = COA_SlottingManager.GetInstance().GetLastChangedSlotId();
		if (slotId <= 0)
			return;
		
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_SlotData slotData = slottingManager.GetSlotData(slotId);
		
		if (!slotData || !m_fSelectedFaction)
			return;
		
		// Update faction slot counters (lightweight, no widget changes)
		InitSlots();
		
		// If the changed slot belongs to a different faction than the one currently displayed,
		// we only need to refresh the unslotted player list (player may have moved in/out).
		if (GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
		{
			UpdateUnslottedPlayersList();
			return;
		}
		
		// Locate the matching element in the main slot list and update it in place
		int currentPlayerId = slotData.GetSlotCurrentPlayerId();
		int elemCount = m_cSlotListBoxComponent.GetItemCount();
		
		for (int i = 0; i < elemCount; i++)
		{
			COA_ListBoxElementComponent elem = COA_ListBoxElementComponent.Cast(
				m_cSlotListBoxComponent.GetElementComponent(i));
			
			if (!elem || elem.m_iSlotId != slotId)
				continue;
			
			if (currentPlayerId > 0)
			{
				string playerName = GetGame().GetPlayerManager().GetPlayerName(currentPlayerId);
				string playerTag = "";
				if (CRF_CommunityTagManager.GetInstance())
					playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(currentPlayerId);
				
				elem.SetPlayerText(playerName);
				elem.SetTagText(playerTag);
				
				Color factionColor = GetGame().GetFactionManager().GetFactionByKey(
					slotData.GetSlotFactionKey()).GetFactionColor();
				elem.GetPlayerText().SetColor(factionColor);
				elem.GetRoleText().SetColor(factionColor);
				elem.GetPlayerText().SetOpacity(0.5);
				elem.GetRoleText().SetOpacity(0.5);
				
				elem.GetDisconnectWidget().SetVisible(
					!GetGame().GetPlayerManager().IsPlayerConnected(currentPlayerId));
			}
			else
			{
				// Slot vacated — restore OPEN/CLOSED status text using the same phase/type
				// logic as COA_ListBoxComponent._AddItemSlot so the display matches initial state.
				COA_ESlotType vacatedSlotType = slotData.GetSlotType();
				string statusText;
				if (slotData.GetIsLockedSlot())
				{
					statusText = "CLOSED";
				}
				else if (m_Gamemode.m_SlottingState == 0)
				{
					if (vacatedSlotType != COA_ESlotType.TEAM_LEADER
						&& vacatedSlotType != COA_ESlotType.SQUAD_LEADER
						&& vacatedSlotType != COA_ESlotType.MEDIC)
						statusText = "CLOSED";
					else
						statusText = "OPEN";
				}
				else if (m_Gamemode.m_SlottingState == 1)
				{
					if (vacatedSlotType != COA_ESlotType.TEAM_LEADER
						&& vacatedSlotType != COA_ESlotType.SQUAD_LEADER
						&& vacatedSlotType != COA_ESlotType.MEDIC
						&& vacatedSlotType != COA_ESlotType.SPECIALTY
						&& vacatedSlotType != COA_ESlotType.SPECIALTY_ASSISTANT)
						statusText = "CLOSED";
					else
						statusText = "OPEN";
				}
				else
				{
					statusText = "OPEN";
				}
				
				elem.SetPlayerText(statusText);
				elem.SetTagText("");
				elem.GetDisconnectWidget().SetVisible(false);
				elem.GetPlayerText().SetColor(Color.White);
				elem.GetRoleText().SetColor(Color.White);
				elem.GetPlayerText().SetOpacity(1.0);
				elem.GetRoleText().SetOpacity(1.0);
			}
			break;
		}
		
		// Rebuild orbat if this slot affects the ORBAT view (leader/medic roles)
		COA_ESlotType slotType = slotData.GetSlotType();
		if (slotType == COA_ESlotType.TEAM_LEADER
			|| slotType == COA_ESlotType.SQUAD_LEADER
			|| slotType == COA_ESlotType.MEDIC)
		{
			RebuildOrbat();
		}
		
		// Refresh unslotted players list
		UpdateUnslottedPlayersList();

		// If game is live and the local player was just placed into this specific slot, close and insert into unit
		if (m_Gamemode.m_GamemodeState == COA_EGamemodeState.GAME
			&& currentPlayerId == SCR_PlayerController.GetLocalPlayerId()
			&& !slotData.GetIsDeadSlot())
			InitilizePlayer();
	}

	/**
	 * Updates the slot display UI with current data
	 * Shows available slots for the selected faction
	 */
	//! Lightweight refresh — updates only tag and rank widgets on already-displayed list items.
	//! Called when the async tag/rank fetch completes; avoids a full rebuild of the slot list.
	protected void RefreshTagsAndRanks()
	{
		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (!tagMgr)
			return;

		map<int, ref COA_SlotData> slotMap = COA_SlottingManager.GetInstance().GetSlotMap();

		// Update slot list
		for (int i = 0; i < m_cSlotListBoxComponent.GetItemCount(); i++)
		{
			COA_ListBoxElementComponent elem = m_cSlotListBoxComponent.GetCRFElementComponent(i);
			if (!elem)
				continue;
			COA_SlotData slotData;
			if (!slotMap.Find(elem.m_iSlotId, slotData) || !slotData)
				continue;
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId <= 0)
				continue;
			elem.SetTagText(tagMgr.GetPlayerTag(playerId));
			elem.SetRankChevron(tagMgr.GetPlayerXp(playerId), tagMgr.GetPlayerRankTrack(playerId));
		}

		// Update ORBAT list
		for (int i = 0; i < m_cOrbatListBoxComponent.GetItemCount(); i++)
		{
			COA_ListBoxElementComponent elem = m_cOrbatListBoxComponent.GetCRFElementComponent(i);
			if (!elem)
				continue;
			COA_SlotData slotData;
			if (!slotMap.Find(elem.m_iSlotId, slotData) || !slotData)
				continue;
			int playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId <= 0)
				continue;
			elem.SetTagText(tagMgr.GetPlayerTag(playerId));
			elem.SetRankChevron(tagMgr.GetPlayerXp(playerId), tagMgr.GetPlayerRankTrack(playerId));
		}

		// Update unslotted players list
		// Use m_iPlayerId stored on each element — avoids fragile index-order matching.
		for (int i = 0; i < m_cUnslotPlayerListBoxComponent.GetItemCount(); i++)
		{
			SCR_ListBoxElementComponent comp = m_cUnslotPlayerListBoxComponent.GetElementComponent(i);
			COA_ListBoxElementComponent crfComp = COA_ListBoxElementComponent.Cast(comp);
			if (!crfComp || crfComp.m_iPlayerId <= 0)
				continue;
			crfComp.SetTagText(tagMgr.GetPlayerTag(crfComp.m_iPlayerId));
			crfComp.SetRankChevron(tagMgr.GetPlayerXp(crfComp.m_iPlayerId), tagMgr.GetPlayerRankTrack(crfComp.m_iPlayerId));
		}
	}

	/**
	 * Adds slots to a group in the UI
	 * @param group - Group to add slots to
	 * @param slotMap - Array of all slot data
	 * @param groupIndex - UI index of the group
	 * @param orbatGroupIndex - UI index in orbat view
	 * @param leadersInGroup - Counter for leaders in group
	 * @param playersInGroup - Counter for players in group
	 * @param deadPlayersInGroup - Counter for dead players in group
	 * @param isAdmin - Whether current player is admin
	 */
	private override void AddSlotsToGroup(SCR_AIGroup group, map<int, ref COA_SlotData> slotMap, 
		int groupIndex, int orbatGroupIndex, out int leadersInGroup, out int playersInGroup, 
		out int deadPlayersInGroup, bool isAdmin, out bool isGroupFull)
	{
		RplId groupId;
		if (!COA_ReplicationHelper.GetRplId(group, groupId))
			return;
		
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();

		isGroupFull = true;
		// GetAllSlotIDsForGroup already returns exactly this group's slot IDs, sorted ascending by
		// definition order - look each one up directly instead of scanning the whole slot map for a
		// resource match. Two slots with the same role (e.g. two Team Leads in one group) share the
		// same resource, and map<K,V> iteration order is unspecified in this engine, so the old
		// resource-search could pair a slot to the wrong visual row.
		foreach (int slotId: slottingManager.GetAllSlotIDsForGroup(groupId))
		{
			COA_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (!slotData)
				continue;

			// Skip slots not in this faction
			if (GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
				continue;

			// Skip locked slots for non-admins
			if (slotData.GetIsLockedSlot() && !isAdmin && slotData.GetSlotCurrentPlayerId() <= 0)
				continue;

			// Track dead slots but don't display them
			if (slotData.GetIsDeadSlot())
			{
				deadPlayersInGroup++;
				continue;
			}

			// Skip dead empty slots
			if (slotData.GetSlotCurrentPlayerId() == 0 && slotData.GetIsDeadSlot())
				continue;

			// Add slot to UI
			int slotIndex = m_cSlotListBoxComponent.AddItemSlot(null, slotId);

			// Recolor the slot icon with its CSI fireteam color instead of the faction color
			// COA_ListboxComponent.AddItemSlot set it to by default.
			COA_GamemodeManager.ApplySlotIconColor(m_cSlotListBoxComponent, slotIndex, slotId);

			// Count players
			if (slotData.GetSlotCurrentPlayerId() >= 0)
				playersInGroup++;

			// Set player text if slot is taken
			if (slotData.GetSlotCurrentPlayerId() > 0)
			{
				string playerName = GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId());

				string playerTag = "";
				int slotPlayerXp = -1;
				string slotPlayerTrack = "enlisted";
				if (CRF_CommunityTagManager.GetInstance())
				{
					playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(slotData.GetSlotCurrentPlayerId());
					slotPlayerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(slotData.GetSlotCurrentPlayerId());
					slotPlayerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(slotData.GetSlotCurrentPlayerId());
				}
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetPlayerText(playerName);
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetTagText(playerTag);
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).SetRankChevron(slotPlayerXp, slotPlayerTrack);
				//Sets slot to faction color when selected
				//m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetSlottedWidget().SetVisible(true);
				Color factionColor = GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()).GetFactionColor();
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetPlayerText().SetColor(factionColor);
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetRoleText().SetColor(factionColor);

				//Sets the opacity too
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetPlayerText().SetOpacity(0.5);
				m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex).GetRoleText().SetOpacity(0.5);

			}
			else
				isGroupFull = false;

			// subscribe to SlotButton's m_OnClicked specifically so that
			// clicking LockButton or KickButton does NOT propagate up and accidentally trigger
			// slot selection. Fall back to the element's own m_OnClicked only for layouts that
			// have no dedicated SlotButton (those layouts also have no admin sub-buttons, so
			// propagation interference is not a concern).
			COA_ListBoxElementComponent slotElem = m_cSlotListBoxComponent.GetCRFElementComponent(slotIndex);
			if (slotElem)
			{
				SCR_ButtonTextComponent slotBtn = slotElem.GetSlotButton();
				if (slotBtn)
					slotBtn.m_OnClicked.Insert(SelectSlotDelay);
				else
					slotElem.m_OnClicked.Insert(SelectSlotDelay);
			}

			COA_ESlotType slotType = slotData.GetSlotType();

			// Add leaders/medics to ORBAT view
			if ((slotType == COA_ESlotType.TEAM_LEADER
				|| slotType == COA_ESlotType.SQUAD_LEADER
				|| slotType == COA_ESlotType.MEDIC)
				&& slotData.GetSlotCurrentPlayerId() > 0)
			{
				AddLeaderToOrbat(slotData, slotId, orbatGroupIndex, leadersInGroup);
				leadersInGroup++;
			}

			// Add admin-only slot controls
			if (isAdmin)
				SetupAdminSlotControls(slotIndex, slotData);
		}
	}

	/**
	 * Adds a leader slot to the ORBAT view
	 * @param slotData - Slot data for the leader
	 * @param slotId - ID of the slot
	 * @param orbatGroupIndex - UI index of group in orbat view
	 * @param leadersInGroup - Counter for leaders in group
	 */
	private override void AddLeaderToOrbat(COA_SlotData slotData, int slotId, int orbatGroupIndex, int leadersInGroup)
	{
		int orbatIndex = m_cOrbatListBoxComponent.AddItemSlot(null, slotId,
			"{BD36FFAE9AB69175}UI/Listbox/PlayerSlotListboxOrbatElementNonAdmin.layout");

		// Recolor the slot icon with its CSI fireteam color instead of the faction color
		// COA_ListboxComponent.AddItemSlot set it to by default.
		COA_GamemodeManager.ApplySlotIconColor(m_cOrbatListBoxComponent, orbatIndex, slotId);

		// Set player text
		string playerName = GetGame().GetPlayerManager().GetPlayerName(slotData.GetSlotCurrentPlayerId());
		string playerTag = "";
		int orbatPlayerXp = -1;
		string orbatPlayerTrack = "enlisted";
		if (CRF_CommunityTagManager.GetInstance())
		{
			playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(slotData.GetSlotCurrentPlayerId());
			orbatPlayerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(slotData.GetSlotCurrentPlayerId());
			orbatPlayerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(slotData.GetSlotCurrentPlayerId());
		}
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetPlayerText(playerName);
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetTagText(playerTag);
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).SetRankChevron(orbatPlayerXp, orbatPlayerTrack);
		
		// Show disconnect indicator if player not connected
		if (!GetGame().GetPlayerManager().IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetDisconnectWidget().SetVisible(true);
		
		// Hide slot button in orbat view
		m_cOrbatListBoxComponent.GetCRFElementComponent(orbatIndex).GetSlotButton().SetVisible(false);
		
		// Set group icon based on first leader's role
		if (leadersInGroup == 0)
		{
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetRoleImage(
				slotData.GetSlotIconResource(), "groupRoleName");
			
			Color factionColor = GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()).GetFactionColor();
			m_cOrbatListBoxComponent.GetCRFElementComponent(orbatGroupIndex).SetGroupIconColor(factionColor);
		}
	}

	/**
	 * Updates the list of unslotted players
	 */
	private override void UpdateUnslottedPlayersList()
	{
		m_cUnslotPlayerListBoxComponent.Clear();
		
		// Get all player IDs
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		
		foreach(int playerId : playerIds)
		{	
			// Skip invalid players, players without faction, already slotted players, or disconnected players
			if (playerId <= 0)
				continue;
			
			if (slottingManager.GetPlayerSlotFaction(playerId, true))
				continue;
				
			if (!GetGame().GetPlayerManager().IsPlayerConnected(playerId))
				continue;
			
			// Add player to unslotted list
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			string playerTag = "";
			int unslottedXp = -1;
			string unslottedTrack = "enlisted";
			if (CRF_CommunityTagManager.GetInstance())
			{
				playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(playerId);
				unslottedXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(playerId);
				unslottedTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(playerId);
			}
			int index = m_cUnslotPlayerListBoxComponent.AddItemAndIconPlayer(
				playerName, 
				EMPTY_RESOURCE, 
				"flag", 
				null,
				"{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout", 
				playerId);
			
			// Set up selection handler — subscribe to the element's own m_OnClicked
			// (same pattern as the slot list) because GetSelectButton() can return null
			// when SCR_ButtonTextComponent fails to attach, and even when it doesn't,
			// a sub-button click may not update the listbox's m_iCurrentItem before
			// SelectPlayer reads GetSelectedItem(). The element's m_OnClicked fires
			// after OnItemClick has already updated the selection state, so it's safe.
			SCR_ListBoxElementComponent comp = m_cUnslotPlayerListBoxComponent.GetElementComponent(index);
			COA_ListBoxElementComponent crfComp = COA_ListBoxElementComponent.Cast(comp);
			if (crfComp)
			{
				crfComp.SetTagText(playerTag);
				crfComp.SetRankChevron(unslottedXp, unslottedTrack);
				crfComp.m_OnClicked.Insert(SelectPlayerDelay);
			}
			
			// Highlight admins, moderators, and selected players
			SetPlayerStatusColor(playerId, comp);
		}
	}

	/**
	 * Adds a player to the player list with appropriate faction icon and status color
	 * @param playerId - ID of the player to add
	 */
	private override void AddPlayerToList(int playerId)
	{
		int listIndex;
		ResourceName playerIconResource;
		Faction playerFaction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(playerId, true);
		
		// Add player with appropriate faction icon
		if (playerFaction)
			playerIconResource = GetFactionIcon(playerFaction.GetFactionKey());
		 else 
			playerIconResource = EMPTY_RESOURCE;
			
		string displayName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string playerTag = "";
		int playerXp = -1;
		string playerTrack = "enlisted";
		if (CRF_CommunityTagManager.GetInstance())
		{
			playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(playerId);
			playerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(playerId);
			playerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(playerId);
		}

		listIndex = m_cPlayerListBoxComponent.AddItemAndIconPlayer(
			displayName, 
			playerIconResource, 
			"flag", 
			null, 
			"{4B1BA5F8E3442E93}UI/Listbox/PlayerListboxElement.layout");
		
		// Apply appropriate color based on player status
		SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(listIndex);
		COA_ListBoxElementComponent crfComp = COA_ListBoxElementComponent.Cast(comp);
		if (crfComp)
		{
			crfComp.SetTagText(playerTag);
			crfComp.SetRankChevron(playerXp, playerTrack);
		}
		
		SetPlayerStatusColor(playerId, comp);
		
		// Highlight players who are talking
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			if (m_MenuManager.m_aPlayersTalking.Contains(playerId))
				comp.SetTalking();
		}
		else
		{
			if (playerId == SCR_PlayerController.GetLocalPlayerId())
			{
				if (m_VONController.m_bIsBroadcasting)
					comp.SetTalking();
			}
		}
	}
}