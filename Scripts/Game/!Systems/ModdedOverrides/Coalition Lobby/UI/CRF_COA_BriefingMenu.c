modded class COA_PreviewMenu
{
    override void OnMenuOpen()
	{	
		super.OnMenuOpen();

		// Don't open menu on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			Close();
			return;
		}

		// Fetch community tags + ranks so they appear in the player list
		CRF_CommunityTagManager tagMgr = CRF_CommunityTagManager.GetInstance();
		if (tagMgr)
		{
			tagMgr.FetchPlayerInfo();
			tagMgr.GetOnPlayerInfoUpdated().Remove(OnPlayerInfoUpdated);
			tagMgr.GetOnPlayerInfoUpdated().Insert(OnPlayerInfoUpdated);
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
			tagMgr.GetOnPlayerInfoUpdated().Remove(OnPlayerInfoUpdated);
			tagMgr.GetOnPlayerRosterChanged().Remove(OnPlayerRosterChanged);
		};
    }

	/**
	 * Updates the player list with connected players
	 */
	protected override void UpdatePlayerList()
	{
		ref array<int> playerIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		m_cPlayerListBoxComponent.Clear();
		
		foreach (int player : playerIds)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
				
			string displayName = GetGame().GetPlayerManager().GetPlayerName(player);
			string playerTag = "";
			int playerXp = -1;
			string playerTrack = "enlisted";
			
			if (CRF_CommunityTagManager.GetInstance())
			{
				playerTag = CRF_CommunityTagManager.GetInstance().GetPlayerTag(player);
				playerXp = CRF_CommunityTagManager.GetInstance().GetPlayerXp(player);
				playerTrack = CRF_CommunityTagManager.GetInstance().GetPlayerRankTrack(player);
			}

			int index = m_cPlayerListBoxComponent.AddItem(
				displayName, 
				null, 
				"{51F58D728FBCAD99}UI/Listbox/PlayerListboxElementNoIcon.layout"
			);
			
			SCR_ListBoxElementComponent comp = m_cPlayerListBoxComponent.GetElementComponent(index);
			COA_ListBoxElementComponent crfComp = COA_ListBoxElementComponent.Cast(comp);
			if (crfComp)
			{
				crfComp.SetTagText(playerTag);
				crfComp.SetRankChevron(playerXp, playerTrack);
			}
			
			// Color code players by role
			SetPlayerStatusColor(player,comp);
			
			// Highlight talking players
			if (!CVON_VONGameModeComponent.GetInstance())
			{
				if (m_MenuManager.m_aPlayersTalking.Contains(player))
					comp.SetTalking();
			}
			else
			{
				if (player == SCR_PlayerController.GetLocalPlayerId())
				{
					if (m_VONController.m_bIsBroadcasting)
						comp.SetTalking();
				}
			}
		}
	}
}