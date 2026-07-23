//------------------------------------------------------------
// Lottery entry — stores one player's signup with optional
// squad name filter.  An empty m_sSquadFilter means "any squad".
//------------------------------------------------------------
class CRF_LotteryEntry
{
	int    m_iPlayerId;
	string m_sSquadFilter;   // Substring the group's custom name must contain.
	                          // Empty string = no preference.

	void CRF_LotteryEntry(int playerId, string squadFilter)
	{
		m_iPlayerId    = playerId;
		m_sSquadFilter = squadFilter;
	}
}

class CRF_SlotLotteryClass : SCR_BaseGameModeComponentClass {}

[ComponentEditorProps(category: "Game Mode Component", description: "Slot lottery — players /roll <faction> [squad] to signup, admins /runlottery to randomly fill open roles across all factions.")]
class CRF_SlotLottery : SCR_BaseGameModeComponent
{
	//------------------------------------------------------------
	// Runtime state
	// Map<factionKey, array<CRF_LotteryEntry>>
	// Keys are uppercase: "BLUFOR", "OPFOR", "INDFOR"
	//------------------------------------------------------------

	ref map<string, ref array<ref CRF_LotteryEntry>> m_mRegisteredPlayersByFaction =
		new map<string, ref array<ref CRF_LotteryEntry>>();

	//------------------------------------------------------------
	// Singleton
	//------------------------------------------------------------

	protected static CRF_SlotLottery m_sInstance;

	//------------------------------------------------------------
	// Faction list
	//------------------------------------------------------------

	protected void GetLotteryFactions(out array<string> factions)
	{
		factions.Clear();
		factions.Insert("BLUFOR");
		factions.Insert("OPFOR");
		factions.Insert("INDFOR");
	}

	//------------------------------------------------------------
	// Normalize faction input to canonical key.
	// Returns "" on no match.
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
	// Validate that at least one available slot exists inside
	// the requested squad (case-insensitive substring match on
	// the group's custom name).  When squadFilter is empty the
	// check passes as long as any open slot exists for the faction.
	//------------------------------------------------------------

	protected bool ValidateSquadExists(string factionKey, string squadFilter)
	{
		CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();
		if (!sm)
			return false;

		map<int, ref CRF_SlotData> slotMap = sm.GetSlotMap();
		if (!slotMap)
			return false;

		foreach (int slotId, CRF_SlotData slotData : slotMap)
		{
			if (!slotData)
				continue;
			if (slotData.GetSlotFactionKey() != factionKey)
				continue;
			if (slotData.GetIsLockedSlot())
				continue;
			if (slotData.GetIsDeadSlot())
				continue;

			string roleName = slotData.GetSlotName();
			if (roleName != "" && roleName.IndexOf("Zeus") != -1)
				continue;

			// If no squad filter was requested, any slot satisfies the check
			if (squadFilter == "")
				return true;

			// Resolve the group name
			RplId groupRplId = slotData.GetSlotCurrentGroup();
			if (groupRplId == RplId.Invalid())
				continue;

			RplComponent rplComp = RplComponent.Cast(Replication.FindItem(groupRplId));
			if (!rplComp)
				continue;

			SCR_AIGroup group = SCR_AIGroup.Cast(rplComp.GetEntity());
			if (!group)
				continue;

			string groupName = group.GetCustomNameWithOriginal();

			// Case-insensitive substring match: compare lowercased strings.
			string groupNameLower = groupName;
			groupNameLower.ToLower();
			string squadFilterLower = squadFilter;
			squadFilterLower.ToLower();

			if (groupNameLower.IndexOf(squadFilterLower) != -1)
				return true;
		}

		return false;
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
			m_mRegisteredPlayersByFaction.Insert(fk, new array<ref CRF_LotteryEntry>());

		GetGame().GetCallqueue().CallLater(RegisterChatCommands, 500, false);
	}

	//------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_sInstance == this)
			m_sInstance = null;
		super.OnDelete(owner);
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

		Print("[SlotLottery] Chat commands registered: /roll <BLUFOR|OPFOR|INDFOR> [squadName], /runlottery, /clearlottery");
	}

	//------------------------------------------------------------
	// Chat command handlers — run on local client, routed to
	// server via RPC.
	//------------------------------------------------------------

	protected void OnChatCmd_Roll(SCR_ChatPanel panel, string data)
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return;

		CRF_RplBroadcastManager bm = CRF_RplBroadcastManager.GetInstance();

		// --- Parse: data may be "BLUFOR" or "BLUFOR MAT" etc. ---
		string factionToken  = "";
		string squadFilter   = "";

		array<string> tokens = {};
		data.Split(" ", tokens, true);

		if (tokens.Count() >= 1)
			factionToken = tokens[0];
		if (tokens.Count() >= 2)
		{
			// Rejoin any remaining tokens in case the squad name has spaces
			for (int i = 1; i < tokens.Count(); i++)
			{
				if (i > 1)
					squadFilter += " ";
				squadFilter += tokens[i];
			}
		}

		// --- Validate faction ---
		string factionKey = NormalizeFactionKey(factionToken);
		if (factionKey == "")
		{
			if (bm)
				bm.SendHint("[SlotLottery] Usage: /roll <BLUFOR|OPFOR|INDFOR> [squadName]", localPlayerId);
			return;
		}

		// --- Validate squad exists (client-side early-out for UX) ---
		// Full authoritative validation is also done server-side.
		if (squadFilter != "" && !ValidateSquadExists(factionKey, squadFilter))
		{
			if (bm)
				bm.SendHint(
					string.Format("[SlotLottery] No available slots match squad '%1' in %2.", squadFilter, factionKey),
					localPlayerId);
			return;
		}

		if (IsGameRunning())
		{
			if (bm)
				bm.SendHint("[SlotLottery] Cannot sign up while the game is running!", localPlayerId);
			return;
		}

		// --- Check if already signed up for any faction ---
		string alreadySignedUpFaction = GetPlayerSignedUpFaction(localPlayerId);
		if (alreadySignedUpFaction != "")
		{
			if (bm)
				bm.SendHint(string.Format("[SlotLottery] You are already signed up for %1!", alreadySignedUpFaction), localPlayerId);
			return;
		}

		// --- Route through authority manager for proper replication ---
		CRF_PlayerRplToAuthorityManager authMgr = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (authMgr)
			authMgr.RegisterPlayerForLottery(localPlayerId, factionKey, squadFilter);

		string msg;
		if (squadFilter != "")
			msg = string.Format("[SlotLottery] You have signed up for the %1 lottery (squad: %2)!", factionKey, squadFilter);
		else
			msg = string.Format("[SlotLottery] You have signed up for the %1 lottery!", factionKey);

		if (bm)
			bm.SendHint(msg, localPlayerId);
	}

	//------------------------------------------------------------
	// Server-side handlers
	//------------------------------------------------------------

	void RegisterPlayerForLottery_Server(int playerId, string factionKey, string squadFilter)
	{
		if (playerId <= 0)
			return;

		if (IsGameRunning())
			return;

		// Normalize again on the server — never trust raw client strings
		string normalizedKey = NormalizeFactionKey(factionKey);
		if (normalizedKey == "")
			return;

		// Reject if the player is already queued under any faction
		if (GetPlayerSignedUpFaction(playerId) != "")
			return;

		// Validate that the faction + squad combination actually exists.
		// This prevents recording attempts with an invalid squad name.
		if (squadFilter != "" && !ValidateSquadExists(normalizedKey, squadFilter))
		{
			Print(string.Format("[SlotLottery] Rejected signup from player %1: squad '%2' not found in %3.",
				playerId, squadFilter, normalizedKey));
			return;
		}

		array<ref CRF_LotteryEntry> factionQueue = m_mRegisteredPlayersByFaction.Get(normalizedKey);
		if (!factionQueue)
			return;

		factionQueue.Insert(new CRF_LotteryEntry(playerId, squadFilter));

		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		if (squadFilter != "")
			Print(string.Format("[SlotLottery] %1 signed up for the %2 lottery (squad filter: '%3').",
				playerName, normalizedKey, squadFilter));
		else
			Print(string.Format("[SlotLottery] %1 signed up for the %2 lottery (any squad).",
				playerName, normalizedKey));
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
			array<ref CRF_LotteryEntry> queue = m_mRegisteredPlayersByFaction.Get(fk);
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

		// --- Independent lottery pass per faction ---
		foreach (string fk : factions)
		{
			array<ref CRF_LotteryEntry> signups = m_mRegisteredPlayersByFaction.Get(fk);
			if (!signups || signups.Count() == 0)
				continue;

			Print(string.Format("[SlotLottery] Faction %1: %2 signups.", fk, signups.Count()));

			// Shuffle for equal odds across the whole faction before splitting by squad.
			ShuffleEntries(signups);

			// --- Group entries by squad filter ---
			// Players with no preference are collected separately; they draw from
			// any available slot in the faction (same behaviour as before).
			//
			// Map key: the normalized squad filter string ("" = no preference).
			ref map<string, ref array<ref CRF_LotteryEntry>> bySquad =
				new map<string, ref array<ref CRF_LotteryEntry>>();

			foreach (ref CRF_LotteryEntry entry : signups)
			{
				string key = entry.m_sSquadFilter;
				if (!bySquad.Contains(key))
					bySquad.Insert(key, new array<ref CRF_LotteryEntry>());
				bySquad.Get(key).Insert(entry);
			}

			// --- Process players with a squad preference first ---
			// We keep a single shared pool of available slots per faction so that
			// slots consumed by squad-preference players are correctly excluded
			// when processing the no-preference pool.

			// Build the full available-slot map for this faction (slot → group name)
			array<int> allAvailableSlots = GetAvailableSlotsForFaction(fk);

			// Iterate over each squad bucket, skipping the no-preference bucket
			foreach (string squadFilter, array<ref CRF_LotteryEntry> entries : bySquad)
			{
				if (squadFilter == "")
					continue;   // handled after

				// Collect slots that belong to this squad out of the still-available pool
				array<int> squadSlots = FilterSlotsBySquad(allAvailableSlots, squadFilter);

				Print(string.Format("[SlotLottery]   Squad '%1': %2 signups, %3 available slots.",
					squadFilter, entries.Count(), squadSlots.Count()));

				if (squadSlots.Count() == 0)
				{
					// Squad is full or doesn't exist — skip without slotting elsewhere
					foreach (ref CRF_LotteryEntry entry : entries)
					{
						string playerName = GetGame().GetPlayerManager().GetPlayerName(entry.m_iPlayerId);
						Print(string.Format("[SlotLottery]   Skipped %1 — squad '%2' has no open slots.",
							playerName, squadFilter));
					}
					bm.SendHint(
						string.Format("[SlotLottery] No open slots in squad '%1' (%2) — affected players were not slotted.",
							squadFilter, fk),
						requestingPlayerId);
					continue;
				}

				foreach (ref CRF_LotteryEntry entry : entries)
				{
					if (squadSlots.Count() == 0)
					{
						// Squad filled up mid-loop
						string playerName = GetGame().GetPlayerManager().GetPlayerName(entry.m_iPlayerId);
						Print(string.Format("[SlotLottery]   Skipped %1 — squad '%2' filled up.",
							playerName, squadFilter));
						continue;
					}

					int currentSlotId = sm.GetPlayerSlotID(entry.m_iPlayerId);
					if (currentSlotId > 0)
						sm.UpdateSlotPlayerID(currentSlotId, 0);

					int randomIdx = Math.RandomInt(0, squadSlots.Count());
					int slotId    = squadSlots[randomIdx];
					squadSlots.RemoveItem(slotId);
					allAvailableSlots.RemoveItem(slotId);  // remove from global pool too

					sm.UpdateSlotPlayerID(slotId, entry.m_iPlayerId);
					totalSlotted++;

					Print(string.Format("[SlotLottery]   Slotted player %1 into slot %2 (squad '%3', %4).",
						entry.m_iPlayerId, slotId, squadFilter, fk));
				}
			}

			// --- Process players with no squad preference ---
			array<ref CRF_LotteryEntry> noPreference = bySquad.Get("");
			if (!noPreference || noPreference.Count() == 0)
				continue;

			Print(string.Format("[SlotLottery]   No-preference: %1 signups, %2 available slots remaining.",
				noPreference.Count(), allAvailableSlots.Count()));

			if (allAvailableSlots.Count() == 0)
			{
				bm.SendHint(
					string.Format("[SlotLottery] No remaining slots for %1 after squad draws.", fk),
					requestingPlayerId);
				continue;
			}

			foreach (ref CRF_LotteryEntry entry : noPreference)
			{
				if (allAvailableSlots.Count() == 0)
					break;

				int currentSlotId = sm.GetPlayerSlotID(entry.m_iPlayerId);
				if (currentSlotId > 0)
					sm.UpdateSlotPlayerID(currentSlotId, 0);

				int randomIdx = Math.RandomInt(0, allAvailableSlots.Count());
				int slotId    = allAvailableSlots[randomIdx];
				allAvailableSlots.RemoveItem(slotId);

				sm.UpdateSlotPlayerID(slotId, entry.m_iPlayerId);
				totalSlotted++;

				Print(string.Format("[SlotLottery]   Slotted player %1 into slot %2 (no preference, %3).",
					entry.m_iPlayerId, slotId, fk));
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

	// Returns the canonical faction key the player is queued under, or "" if not signed up.
	protected string GetPlayerSignedUpFaction(int playerId)
	{
		array<string> factions = {};
		GetLotteryFactions(factions);
		foreach (string fk : factions)
		{
			array<ref CRF_LotteryEntry> queue = m_mRegisteredPlayersByFaction.Get(fk);
			if (!queue)
				continue;
			foreach (ref CRF_LotteryEntry entry : queue)
			{
				if (entry.m_iPlayerId == playerId)
					return fk;
			}
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
			array<ref CRF_LotteryEntry> queue = m_mRegisteredPlayersByFaction.Get(fk);
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
			array<ref CRF_LotteryEntry> queue = m_mRegisteredPlayersByFaction.Get(fk);
			if (queue)
				queue.Clear();
		}
	}

	// Returns all open, non-locked, non-dead, non-Zeus slots for a faction.
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

	// Filters a list of slot IDs down to those whose group name contains squadFilter
	// (case-insensitive substring match).
	protected array<int> FilterSlotsBySquad(array<int> slotIds, string squadFilter)
	{
		array<int> result = new array<int>;

		if (squadFilter == "")
		{
			// No filter — return a copy of the full list
			foreach (int id : slotIds)
				result.Insert(id);
			return result;
		}

		CRF_SlottingManager sm = CRF_SlottingManager.GetInstance();
		if (!sm)
			return result;

		string filterLower = squadFilter;
		filterLower.ToLower();

		foreach (int slotId : slotIds)
		{
			CRF_SlotData slotData = sm.GetSlotData(slotId);
			if (!slotData)
				continue;

			RplId groupRplId = slotData.GetSlotCurrentGroup();
			if (groupRplId == RplId.Invalid())
				continue;

			RplComponent rplComp = RplComponent.Cast(Replication.FindItem(groupRplId));
			if (!rplComp)
				continue;

			SCR_AIGroup group = SCR_AIGroup.Cast(rplComp.GetEntity());
			if (!group)
				continue;

			string groupName = group.GetCustomNameWithOriginal();
			groupName.ToLower();
			if (groupName.IndexOf(filterLower) != -1)
				result.Insert(slotId);
		}

		return result;
	}

	// Fisher-Yates shuffle for CRF_LotteryEntry arrays.
	// This ensures every player in a faction has an equal chance regardless
	// of which squad they requested — the shuffle happens before squad bucketing.
	protected void ShuffleEntries(array<ref CRF_LotteryEntry> arr)
	{
		if (!arr)
			return;

		int n = arr.Count();
		for (int i = n - 1; i > 0; i--)
		{
			int j = Math.RandomInt(0, i + 1);
			ref CRF_LotteryEntry temp = arr[i];
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