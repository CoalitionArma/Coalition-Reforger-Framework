class CRF_SlotLotteryClass : SCR_BaseGameModeComponentClass {}
 
[ComponentEditorProps(category: "Game Mode Component", description: "Slot lottery — players /roll to signup, admins /runlottery to randomly fill open roles.")]
class CRF_SlotLottery : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------
	// Configurable attributes
	//------------------------------------------------------------
 
	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key for the slot lottery (e.g., 'BLUFOR', 'OPFOR', or custom faction key).")]
	string m_sTargetFactionKey;
 
	ref array<int> m_iRegisteredPlayerIDs = {};
 
	//------------------------------------------------------------
	// Singleton
	//------------------------------------------------------------
 
	protected static CRF_SlotLottery m_sInstance;
 
	//------------------------------------------------------------
	// Lifecycle
	//------------------------------------------------------------
 
	override void OnPostInit(IEntity owner)
	{
		Print("[SlotLottery] OnPostInit called. Mode: " + RplSession.Mode());
		super.OnPostInit(owner);
 
		m_sInstance = this;
 
       	GetGame().GetCallqueue().CallLater(RegisterChatCommands, 500, false);
	}
 
	//------------------------------------------------------------
	// Chat command registration
	//------------------------------------------------------------
 
	protected void RegisterChatCommands()
	{
		SCR_ChatPanelManager chatMgr = SCR_ChatPanelManager.GetInstance();
		if (!chatMgr)
		{
			Print("[SlotLottery] WARNING: SCR_ChatPanelManager not available — retrying in 1s.");
			GetGame().GetCallqueue().CallLater(RegisterChatCommands, 1000, false);
			return;
		}
 
		chatMgr.GetCommandInvoker("roll").Insert(OnChatCmd_Roll);
		chatMgr.GetCommandInvoker("runlottery").Insert(OnChatCmd_RunLottery);
		chatMgr.GetCommandInvoker("clearlottery").Insert(OnChatCmd_ClearLottery);

		Print("[SlotLottery] Chat commands registered: /roll, /runlottery, /clearlottery");
	}

	//------------------------------------------------------------
	// Chat command handlers (all run on local client, route to server via RPC)
	//------------------------------------------------------------
 
	protected void OnChatCmd_Roll(SCR_ChatPanel panel, string data)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return;

		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();

		if (IsGameRunning())
		{
			if (bm)
				bm.SendHint("[SlotLottery] Cannot sign up while the game is running!", localPlayerId);
			return;
		}

		// Find() returns -1 if not found; >= 0 means already registered
		if (m_iRegisteredPlayerIDs.Find(localPlayerId) >= 0)
		{
			if (bm)
				bm.SendHint("[SlotLottery] You are already signed up!", localPlayerId);
			return;
		}

		// Route through authority manager for proper replication
		CRF_PlayerRplToAuthorityManager authMgr = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (authMgr)
			authMgr.RegisterPlayerForLottery(localPlayerId);

		if (bm)
			bm.SendHint(string.Format("You have signed up for the %1 lottery!", m_sTargetFactionKey), localPlayerId);
	}

	//------------------------------------------------------------
	// Server-side RPC handlers (called from PlayerRplToAuthorityManager)
	//------------------------------------------------------------

	void RegisterPlayerForLottery_Server(int playerId)
	{
		if (playerId <= 0)
			return;

		if (IsGameRunning())
			return;

		// Double-check on server (client state may be slightly stale)
		if (m_iRegisteredPlayerIDs.Find(playerId) >= 0)
			return;

		m_iRegisteredPlayerIDs.Insert(playerId);

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		Print(string.Format("[SlotLottery] %1 signed up for the lottery.", playerName));
	}
 
	protected void OnChatCmd_RunLottery(SCR_ChatPanel panel, string data)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return;
 
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
 
		if (!SCR_Global.IsAdmin(localPlayerId))
		{
			if (bm)
				bm.SendHint("[SlotLottery] Only admins can use this command!", localPlayerId);
			return;
		}
 
		if (IsGameRunning())
		{
			if (bm)
				bm.SendHint("[SlotLottery] Cannot slot players while the game is running!", localPlayerId);
			return;
		}

		// Route through authority manager for proper replication
		CRF_PlayerRplToAuthorityManager authMgr = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (authMgr)
			authMgr.RunSlotLottery(localPlayerId);
	}

	void RunSlotLottery_Server(int requestingPlayerId)
	{
		if (!SCR_Global.IsAdmin(requestingPlayerId))
			return;

		if (IsGameRunning())
			return;

		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();

		if (!bm || !sm)
		{
			Print("[SlotLottery] ERROR: Manager(s) not available on server!");
			return;
		}

		if (m_iRegisteredPlayerIDs.IsEmpty())
		{
			Print("[SlotLottery] No players signed up.");
			bm.SendHint("[SlotLottery] No players have signed up for the lottery!", requestingPlayerId);
			return;
		}
 
		array<int> availableSlotIds = GetAvailableSlotsForFaction(m_sTargetFactionKey);
		Print(string.Format("[SlotLottery] Found %1 available slots for faction '%2'.", availableSlotIds.Count(), m_sTargetFactionKey));
 
		if (availableSlotIds.IsEmpty())
		{
			bm.SendHint("[SlotLottery] No available roles found for slotting!", requestingPlayerId);
			return;
		}
 
		ShuffleArray(m_iRegisteredPlayerIDs);
 
		int slottedCount = 0;

		foreach (int signedUpId : m_iRegisteredPlayerIDs)
		{
			if (availableSlotIds.IsEmpty())
				break;

			// Remove player from their current slot if already slotted
			int currentSlotId = sm.GetPlayerSlotID(signedUpId);
			if (currentSlotId > 0)
				sm.UpdateSlotPlayerID(currentSlotId, -1);

			int randomIdx = Math.RandomInt(0, availableSlotIds.Count());
			int slotId = availableSlotIds[randomIdx];
			availableSlotIds.RemoveItem(slotId);

			// UpdateSlotPlayerID is authoritative on server and handles replication internally
			sm.UpdateSlotPlayerID(slotId, signedUpId);
			slottedCount++;

			Print(string.Format("[SlotLottery] Slotted player %1 into slot %2.", signedUpId, slotId));
		}

		Print(string.Format("[SlotLottery] Slotted %1 players.", slottedCount));

		// Clear the list after slotting and replicate the cleared state
		m_iRegisteredPlayerIDs.Clear();
		Replication.BumpMe();

		bm.SendHint(
			string.Format("%1 players have been randomly slotted into the %2 team!", slottedCount, m_sTargetFactionKey),
			-1
		);
	}
 
	protected void OnChatCmd_ClearLottery(SCR_ChatPanel panel, string data)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return;
 
		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
 
		if (!SCR_Global.IsAdmin(localPlayerId))
		{
			if (bm)
				bm.SendHint("[SlotLottery] Only admins can use this command!", localPlayerId);
			return;
		}

		if (IsGameRunning())
		{
			if (bm)
				bm.SendHint("[SlotLottery] Cannot clear queue while the game is running!", localPlayerId);
			return;
		}

		// Route through authority manager for proper replication
		CRF_PlayerRplToAuthorityManager authMgr = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (authMgr)
			authMgr.ClearSlotLottery(localPlayerId);
	}

	void ClearSlotLottery_Server(int requestingPlayerId)
	{
		if (!SCR_Global.IsAdmin(requestingPlayerId))
			return;

		if (IsGameRunning())
			return;

		int count = m_iRegisteredPlayerIDs.Count();
		m_iRegisteredPlayerIDs.Clear();
		Replication.BumpMe();

		Print(string.Format("[SlotLottery] Cleared %1 signups.", count));

		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.SendHint(
				string.Format("The %1 lottery queue has been cleared (%2 signups removed).", m_sTargetFactionKey, count),
				-1
			);
	}
 
	//------------------------------------------------------------
	// Helper methods
	//------------------------------------------------------------
 
	protected array<int> GetAvailableSlotsForFaction(string factionKey)
	{
    	array<int> result = new array<int>;

    	CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();
    	if (!sm)
       		return result;

    	map<int, ref CRF_SlotData> slotMap = sm.GetSlotMap();
    	if (!slotMap)
        	return result;

    	foreach (int slotId, CRF_SlotData slotData : slotMap)
    	{
        	if (!slotData)
            	continue;

        	if (slotData.GetSlotFactionKey() != factionKey)
            	continue;

        	if (slotData.GetSlotCurrentPlayerId() > 0)
            	continue;

        	if (slotData.GetIsLockedSlot())
            	continue;

        	if (slotData.GetIsDeadSlot())
            	continue;

        	string roleName = slotData.GetSlotName();
        	if (roleName && roleName.IndexOf("Zeus") != -1)
            	continue;

        	result.Insert(slotId);
    	}

    	return result;
	}
 
	protected void ShuffleArray(array<int> arr)
	{
		if (!arr)
			return;
 
		int n = arr.Count();
		for (int i = n - 1; i > 0; i--)
		{
			int j = Math.RandomInt(0, i + 1);
			int temp = arr[i];
			arr[i] = arr[j];
			arr[j] = temp;
		}
	}
 
	protected bool IsGameRunning()
	{
		CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();
		if (!sm)
			return false;
 
		map<int, ref CRF_SlotData> slotMap = sm.GetSlotMap();
		if (!slotMap)
			return false;
 
		foreach (int slotId, CRF_SlotData slotData : slotMap)
		{
			if (slotData && slotData.GetSlotCurrentCharacter() != RplId.Invalid())
				return true;
		}
 
		return false;
	}
 
	//------------------------------------------------------------
	// Singleton accessor
	//------------------------------------------------------------
	static CRF_SlotLottery GetInstance()
	{
		return m_sInstance;
	}
}