modded class COA_AdminMenu
{
	// Player whose ticket to select once the Tickets tab has loaded (set by OpenTicketFor)
	protected static int s_iPendingTicketPlayerId;

	//------------------------------------------------------------------------------------------------
	//! Open the admin menu on the Tickets tab with this player's ticket selected - used by the
	//! Game Master player list. The menu opens on Tickets by default (DelayedMenuInitialization), so
	//! only the selection needs doing once the list arrives (PopulateOpenTicketList).
	static void OpenTicketFor(int playerId)
	{
		s_iPendingTicketPlayerId = playerId;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.COA_AdminMenu);
	}

	//------------------------------------------------------------------------------------------------
	override void PopulateOpenTicketList(array<int> tickets)
	{
		super.PopulateOpenTicketList(tickets);

		if (s_iPendingTicketPlayerId <= 0)
			return;

		int pendingPlayerId = s_iPendingTicketPlayerId;
		s_iPendingTicketPlayerId = 0;

		SCR_ListBoxComponent playerList = GetListBox("PlayerListBox0");
		if (!playerList)
			return;

		// Rows are added in the order of the tickets array
		int index = tickets.Find(pendingPlayerId);
		if (index >= 0 && index < playerList.GetItemCount())
			playerList.SetItemSelected(index, true, true);
	}

    //------------------------------------------------------------------------------------------------
    override void InitializeGamemodeMenu()
	{
		super.InitializeGamemodeMenu();
		
        //Toggle VAAR Recording
		SCR_ButtonTextComponent toggleVAARRecording = SCR_ButtonTextComponent.Cast(m_wMenuContent.FindAnyWidget("ToggleVAARButton").FindHandler(SCR_ButtonTextComponent));
		toggleVAARRecording.m_OnClicked.Insert(ToggleVAARRecording);
    }

    //------------------------------------------------------------------------------------------------
	void ToggleVAARRecording()
	{
		COA_PlayerRplToAuthorityManager.GetInstance().ToggleVAARRecording();
	}

    //------------------------------------------------------------------------------------------------
    override void GamemodeMenuUpdate()
	{
		super.GamemodeMenuUpdate();
		
		bool m_bVAARRecordingEnabled = CRF_VAAR_GamemodeComponent.GetInstance().m_bRecording;
		Widget VAARRecordingEnabledButton = m_wMenuContent.FindAnyWidget("ToggleVAARButton");
		TextWidget VAARRecordingEnabledText = TextWidget.Cast(VAARRecordingEnabledButton.FindWidget("ActionButtonText"));
		if (m_bVAARRecordingEnabled)
		{
			VAARRecordingEnabledText.SetText("Recording Enabled");
			VAARRecordingEnabledText.SetColorInt(Color.GREEN);
		}
		else
		{
			VAARRecordingEnabledText.SetText("Recording Disabled");
			VAARRecordingEnabledText.SetColorInt(Color.RED);
        };
    }
}