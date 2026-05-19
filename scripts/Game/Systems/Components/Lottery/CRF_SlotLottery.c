class CRF_SlotLotteryClass : SCR_BaseGameModeComponentClass {}

[ComponentEditorProps(category: "Game Mode Component", description: "Slot lottery — players /roll <faction> to signup, admins /runlottery to randomly fill open roles across all factions.")]
class CRF_SlotLottery : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------
	// Runtime state
	// Map<factionKey, array<playerId>> — keys are uppercase: "BLUFOR", "OPFOR", "INDFOR"
	//------------------------------------------------------------

	ref map<string, ref array<int>> m_mRegisteredPlayersByFaction = new map<string, ref array<int>>();

	//------------------------------------------------------------
	// Singleton
	//------------------------------------------------------------

	protected static CRF_SlotLottery m_sInstance;

	//------------------------------------------------------------
	// Faction list
	// Enfusion does not support static const array<string> with an
	// inline initialiser, so we populate a caller-supplied array.
	// Keys are the canonical uppercase strings used by the slotting system.
	//------------------------------------------------------------

	protected void GetLotteryFactions(out array<string> factions)
	{
		factions.Clear();
		factions.Insert("BLUFOR");
		factions.Insert("OPFOR");
		factions.Insert("INDFOR");
	}

	//------------------------------------------------------------
	// Normalize whatever the player typed into a canonical key.
	// Enfusion strings have no ToUpper/ToLower/Trim in this build,
	// so we do explicit comparisons for every expected variant.
	// Returns "" if the input doesn't match any known faction.
	//------------------------------------------------------------

	protected string NormalizeFactionKey(string input)
	{
		if (input == "BLUFOR" || input == "blufor" || input == "Blufor" || input == "BLufor" || input == "bLUFOR")
			return "BLUFOR";
		if (input == "OPFOR" || input == "opfor" || input == "Opfor" || input == "OpFor")
			return "OPFOR";
		if (input == "INDFOR" || input == "indfor" || input == "Indfor" || input == "IndFor")
			return "INDFOR";
		return "";
	}

	//------------------------------------------------------------
	// Lifecycle
	//------------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		Print("[SlotLottery] OnPostInit called. Mode: " + RplSession.Mode());
		super.OnPostInit(owner);

		m_sInstance = this;

		// Pre-populate per-faction queues
		array<string> factions = {};
		GetLotteryFactions(factions);
		foreach (string fk : factions)
			m_mRegisteredPlayersByFaction.Insert(fk, new array<int>());

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

		Print("[SlotLottery] Chat commands registered: /roll <BLUFOR|OPFOR|INDFOR>, /runlottery, /clearlottery");
	}

	//------------------------------------------------------------
	// Chat command handlers (run on local client, routed to server via RPC)
	//------------------------------------------------------------

	protected void OnChatCmd_Roll(SCR_ChatPanel panel, string data)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return;

		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();

		// Normalize the faction argument — handles any common casing the player might type
		string factionKey = NormalizeFactionKey(data);

		if (factionKey == "")
		{
			if (bm)
				bm.SendHint("[SlotLottery] Usage: /roll <BLUFOR|OPFOR|INDFOR>", localPlayerId);
			return;
		}

		if (IsGameRunning())
		{
			if (bm)
				bm.SendHint("[SlotLottery] Cannot sign up while the game is running!", localPlayerId);
			return;
		}

		// Check if already signed up for any faction
		string alreadySignedUpFaction = GetPlayerSignedUpFaction(localPlayerId);
		if (alreadySignedUpFaction != "")
		{
			if (bm)
				bm.SendHint(string.Format("[SlotLottery] You are already signed up for %1!", alreadySignedUpFaction), localPlayerId);
			return;
		}

		// Route through authority manager for proper replication
		CRF_PlayerRplToAuthorityManager authMgr = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (authMgr)
			authMgr.RegisterPlayerForLottery(localPlayerId, factionKey);

		if (bm)
			bm.SendHint(string.Format("[SlotLottery] You have signed up for the %1 lottery!", factionKey), localPlayerId);
	}

	//------------------------------------------------------------
	// Server-side handlers (called from PlayerRplToAuthorityManager)
	//------------------------------------------------------------

	void RegisterPlayerForLottery_Server(int playerId, string factionKey)
	{
		if (playerId <= 0)
			return;

		if (IsGameRunning())
			return;

		// factionKey arrives already normalized from the client, but normalize again
		// on the server as a safety measure — never trust raw client strings directly
		string normalizedKey = NormalizeFactionKey(factionKey);
		if (normalizedKey == "")
			return;

		// Reject if the player is already queued under any faction
		if (GetPlayerSignedUpFaction(playerId) != "")
			return;

		array<int> factionQueue = m_mRegisteredPlayersByFaction.Get(normalizedKey);
		if (!factionQueue)
			return;

		factionQueue.Insert(playerId);

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		Print(string.Format("[SlotLottery] %1 signed up for the %2 lottery.", playerName, normalizedKey));
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

		array<string> factions = {};
		GetLotteryFactions(factions);

		// Abort early if nobody has signed up across all factions
		bool anySignups = false;
		foreach (string fk : factions)
		{
			array<int> queue = m_mRegisteredPlayersByFaction.Get(fk);
			if (queue && queue.Count() > 0)
			{
				anySignups = true;
				break;
			}
		}

		if (!anySignups)
		{
			Print("[SlotLottery] No players signed up.");
			bm.SendHint("[SlotLottery] No players have signed up for the lottery!", requestingPlayerId);
			return;
		}

		int totalSlotted = 0;

		// Run an independent lottery pass per faction
		foreach (string fk : factions)
		{
			array<int> signups = m_mRegisteredPlayersByFaction.Get(fk);
			if (!signups || signups.Count() == 0)
				continue;

			// fk is already the canonical uppercase key the slotting system uses
			array<int> availableSlotIds = GetAvailableSlotsForFaction(fk);
			Print(string.Format("[SlotLottery] Faction %1: %2 signups, %3 available slots.",
				fk, signups.Count(), availableSlotIds.Count()));

			if (availableSlotIds.Count() == 0)
			{
				bm.SendHint(string.Format("[SlotLottery] No available slots for %1 — skipping.", fk), requestingPlayerId);
				continue;
			}

			ShuffleArray(signups);

			foreach (int signedUpId : signups)
			{
				if (availableSlotIds.Count() == 0)
					break;

				// Vacate any slot the player currently holds
				int currentSlotId = sm.GetPlayerSlotID(signedUpId);
				if (currentSlotId > 0)
					sm.UpdateSlotPlayerID(currentSlotId, -1);

				int randomIdx = Math.RandomInt(0, availableSlotIds.Count());
				int slotId = availableSlotIds[randomIdx];
				availableSlotIds.RemoveItem(slotId);

				sm.UpdateSlotPlayerID(slotId, signedUpId);
				totalSlotted++;

				Print(string.Format("[SlotLottery] Slotted player %1 into slot %2 (%3).", signedUpId, slotId, fk));
			}
		}

		Print(string.Format("[SlotLottery] Lottery complete. %1 players slotted.", totalSlotted));

		ClearAllQueues();

		bm.SendHint(
			string.Format("[SlotLottery] Lottery complete! %1 players randomly slotted across all factions.", totalSlotted),
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

		int totalCount = GetTotalSignupCount();
		ClearAllQueues();

		Print(string.Format("[SlotLottery] Cleared %1 total signups.", totalCount));

		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();
		if (bm)
			bm.SendHint(
				string.Format("[SlotLottery] All lottery queues cleared (%1 signups removed).", totalCount),
				-1
			);
	}

	//------------------------------------------------------------
	// Helper methods
	//------------------------------------------------------------

	// Returns the canonical faction key the player is queued under, or "" if not signed up
	protected string GetPlayerSignedUpFaction(int playerId)
	{
		array<string> factions = {};
		GetLotteryFactions(factions);
		foreach (string fk : factions)
		{
			array<int> queue = m_mRegisteredPlayersByFaction.Get(fk);
			if (queue && queue.Find(playerId) >= 0)
				return fk;
		}
		return "";
	}

	protected int GetTotalSignupCount()
	{
		int total = 0;
		array<string> factions = {};
		GetLotteryFactions(factions);
		foreach (string fk : factions)
		{
			array<int> queue = m_mRegisteredPlayersByFaction.Get(fk);
			if (queue)
				total += queue.Count();
		}
		return total;
	}

	protected void ClearAllQueues()
	{
		array<string> factions = {};
		GetLotteryFactions(factions);
		foreach (string fk : factions)
		{
			array<int> queue = m_mRegisteredPlayersByFaction.Get(fk);
			if (queue)
				queue.Clear();
		}
	}

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
			if (roleName != "" && roleName.IndexOf("Zeus") != -1)
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