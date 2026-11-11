//------------------------------------------------------------------------------------------------
//! Server only. Locks slots so the open slots match the mission's slot ratio for the current
//! player count. Never locks an occupied slot or touches a slot an admin locked by hand.
//! Changes go out in one COA_RplBroadcastManager.BroadcastSlotLockBatch RPC.
class CRF_SlotBalancer
{
	//! Only these are reopened by a re-balance or reset
	protected ref set<int> m_sLockedByBalance = new set<int>();

	//! Slot IDs restart every mission, so a new slotting manager gets a fresh instance
	protected COA_SlottingManager m_SlottingManager;

	protected static ref CRF_SlotBalancer s_Instance;

	//------------------------------------------------------------------------------------------------
	static CRF_SlotBalancer GetInstance()
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!s_Instance || s_Instance.m_SlottingManager != slottingManager)
		{
			s_Instance = new CRF_SlotBalancer();
			s_Instance.m_SlottingManager = slottingManager;
		}

		return s_Instance;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RATIO HELPERS (also used by the slotting menu)
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! The ratio fields hold short keys ("BLU"); slots use full faction keys
	static FactionKey RatioKeyToFactionKey(string ratioKey)
	{
		switch (ratioKey)
		{
			case "BLU": return "BLUFOR";
			case "OPF": return "OPFOR";
			case "IND": return "INDFOR";
			case "CIV": return "CIV";
		}

		return ratioKey;
	}

	//------------------------------------------------------------------------------------------------
	static bool HasValidRatio(COA_Gamemode gamemode)
	{
		if (!gamemode)
			return false;

		if (gamemode.m_iFactionOneRatio <= 0 || gamemode.m_iFactionTwoRatio <= 0)
			return false;

		if (gamemode.m_sFactionOneKey.IsEmpty() || gamemode.m_sFactionTwoKey.IsEmpty())
			return false;

		return RatioKeyToFactionKey(gamemode.m_sFactionOneKey) != RatioKeyToFactionKey(gamemode.m_sFactionTwoKey);
	}

	//------------------------------------------------------------------------------------------------
	//! targetOne + targetTwo always equals playerCount
	static void GetTargets(int playerCount, int ratioOne, int ratioTwo, out int targetOne, out int targetTwo)
	{
		targetOne = 0;
		targetTwo = 0;

		if (playerCount <= 0 || ratioOne + ratioTwo <= 0)
			return;

		float total = ratioOne + ratioTwo;
		float share = playerCount * ratioOne / total;
		targetOne = Math.Round(share);
		targetTwo = playerCount - targetOne;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ACTIONS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! \return false if the mission has no ratio or the round is live
	bool Balance(out string summary)
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!Replication.IsServer() || !HasValidRatio(gamemode) || gamemode.m_GamemodeState >= COA_EGamemodeState.GAME)
			return false;

		int playerCount = GetGame().GetPlayerManager().GetPlayerCount();
		int targetOne, targetTwo;
		GetTargets(playerCount, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, targetOne, targetTwo);

		FactionKey factionOne = RatioKeyToFactionKey(gamemode.m_sFactionOneKey);
		FactionKey factionTwo = RatioKeyToFactionKey(gamemode.m_sFactionTwoKey);

		array<int> toLock = {};
		array<int> toUnlock = {};
		int openOne = BalanceFaction(factionOne, targetOne, toLock, toUnlock);
		int openTwo = BalanceFaction(factionTwo, targetTwo, toLock, toUnlock);
		BroadcastChanges(toLock, toUnlock);

		summary = string.Format("%1 players at %2 %3:%4 %5 - %6 %7 open, %8 %9 open",
			playerCount, gamemode.m_sFactionOneKey, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, gamemode.m_sFactionTwoKey,
			factionOne, openOne, factionTwo, openTwo);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \return slots reopened
	int Reset()
	{
		if (!Replication.IsServer())
			return 0;

		array<int> toUnlock = {};
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
		{
			foreach (int slotId : m_sLockedByBalance)
			{
				COA_SlotData slotData = slottingManager.GetSlotData(slotId);
				if (!slotData || !slotData.GetIsLockedSlot())
					continue;

				slotData.SetIsLockedSlot(false);
				toUnlock.Insert(slotId);
			}
		}

		m_sLockedByBalance.Clear();

		array<int> toLock = {};
		BroadcastChanges(toLock, toUnlock);
		return toUnlock.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected void BroadcastChanges(array<int> toLock, array<int> toUnlock)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.BroadcastSlotLockBatch(toLock, toUnlock);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 APPLYING A PLAN
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! \return the faction's open slot count afterwards
	protected int BalanceFaction(FactionKey factionKey, int target, notnull array<int> toLock, notnull array<int> toUnlock)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager || !slottingManager.IsFactionValid(factionKey))
			return 0;

		CRF_SlotBalancePlan plan = new CRF_SlotBalancePlan();
		plan.Build(factionKey, m_sLockedByBalance);

		// Occupied slots stay open regardless of the plan, so shrink the budget by the ones it left
		// out and re-plan until that settles
		set<int> occupied = new set<int>();
		plan.GetOccupiedSlots(occupied);

		set<int> open = new set<int>();
		int budget = target;
		for (int attempt = 0; attempt < 4; attempt++)
		{
			plan.Plan(budget);
			open.Clear();
			plan.GetOpenSlots(open);

			int outside;
			foreach (int slotId : occupied)
			{
				if (!open.Contains(slotId))
					outside++;
			}

			int newBudget = CRF_SlotBalancePlan.IntMax(target - outside, 0);
			if (newBudget == budget)
				break;

			budget = newBudget;
		}

		foreach (int slotId : occupied)
		{
			if (!open.Contains(slotId))
				open.Insert(slotId);
		}

		array<int> managed = {};
		plan.GetManagedSlots(managed);
		int openCount;
		foreach (int slotId : managed)
		{
			COA_SlotData slotData = slottingManager.GetSlotData(slotId);
			if (!slotData)
				continue;

			bool keepOpen = open.Contains(slotId);
			if (keepOpen)
				openCount++;

			SetLocked(slotId, slotData, !keepOpen, toLock, toUnlock);
		}

		return openCount;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetLocked(int slotId, COA_SlotData slotData, bool lock, array<int> toLock, array<int> toUnlock)
	{
		if (lock)
		{
			if (slotData.GetSlotCurrentPlayerId() > 0)
				return;

			if (!slotData.GetIsLockedSlot())
			{
				slotData.SetIsLockedSlot(true);
				toLock.Insert(slotId);
			}

			if (!m_sLockedByBalance.Contains(slotId))
				m_sLockedByBalance.Insert(slotId);

			return;
		}

		if (!m_sLockedByBalance.Contains(slotId))
			return;

		if (slotData.GetIsLockedSlot())
		{
			slotData.SetIsLockedSlot(false);
			toUnlock.Insert(slotId);
		}

		m_sLockedByBalance.RemoveItem(slotId);
	}
}

//------------------------------------------------------------------------------------------------
enum CRF_EBalanceElementKind
{
	COMPANY_HQ,
	PLATOON_HQ,
	SQUAD,
	SUPPORT_SQUAD,	// weapons/specialist teams, identified by the config group flag
	CREW			// opened whole or not at all
}

//------------------------------------------------------------------------------------------------
//! One slotting group
class CRF_BalanceElement
{
	CRF_EBalanceElementKind m_eKind;
	int m_iParent = -1;						//! element index of the HQ above, -1 for none

	ref array<int> m_aSlotIds = {};
	ref array<int> m_aRoles = {};			//! COA_EGearRole, parallel to m_aSlotIds
	ref array<bool> m_aOccupied = {};

	//! Local indices into m_aSlotIds. Controllers (JTAC/FO) are opened separately from m_iOpen.
	ref array<int> m_aPickable = {};
	ref array<int> m_aControllers = {};
	ref array<bool> m_aControllerOpen = {};

	int m_iOpen;							//! open count of m_aPickable

	//------------------------------------------------------------------------------------------------
	bool IsHQ()
	{
		return m_eKind == CRF_EBalanceElementKind.COMPANY_HQ || m_eKind == CRF_EBalanceElementKind.PLATOON_HQ;
	}

	//------------------------------------------------------------------------------------------------
	bool IsSquad()
	{
		return m_eKind == CRF_EBalanceElementKind.SQUAD || m_eKind == CRF_EBalanceElementKind.SUPPORT_SQUAD;
	}

	//------------------------------------------------------------------------------------------------
	bool IsOpen()
	{
		return m_iOpen > 0;
	}

	//------------------------------------------------------------------------------------------------
	int GetPickableCount()
	{
		return m_aPickable.Count();
	}

	//------------------------------------------------------------------------------------------------
	int GetMinSize()
	{
		if (IsHQ())
			return 1;

		if (IsSquad())
			return CRF_SlotBalancePlan.IntMin(CRF_SlotBalancePlan.MIN_SQUAD_SIZE, GetPickableCount());

		return GetPickableCount();
	}

	//------------------------------------------------------------------------------------------------
	//! Size reached before lower priorities get slots
	int GetPreferredSize()
	{
		if (IsHQ())
			return GetHQPickOrder(true).Count();

		if (IsSquad())
			return CRF_SlotBalancePlan.IntMin(CRF_SlotBalancePlan.PREFERRED_SQUAD_SIZE, GetPickableCount());

		return GetPickableCount();
	}

	//------------------------------------------------------------------------------------------------
	void GetOpenIndices(notnull array<int> outIndices)
	{
		outIndices.Clear();

		array<int> order;
		if (IsHQ())
			order = GetHQPickOrder(false);
		else if (IsSquad())
			order = GetSquadPickOrder(m_iOpen);
		else
			order = m_aPickable;

		for (int i = 0; i < m_iOpen && i < order.Count(); i++)
		{
			outIndices.Insert(order[i]);
		}

		foreach (int c, int localIndex : m_aControllers)
		{
			if (m_aControllerOpen[c])
				outIndices.Insert(localIndex);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Commander, second in command, medics, then everyone else
	//! \param keyOnly stop after the medics
	protected array<int> GetHQPickOrder(bool keyOnly)
	{
		array<int> order = {};
		if (m_aPickable.IsEmpty())
			return order;

		int commander = -1;
		foreach (int i : m_aPickable)
		{
			if (m_aRoles[i] == COA_EGearRole.COMPANY_COMMANDER || m_aRoles[i] == COA_EGearRole.PLATOON_LEADER)
			{
				commander = i;
				break;
			}
		}

		if (commander == -1)
		{
			foreach (int i : m_aPickable)
			{
				if (m_aRoles[i] == COA_EGearRole.FIRST_SERGEANT || m_aRoles[i] == COA_EGearRole.PLATOON_SERGEANT)
				{
					commander = i;
					break;
				}
			}
		}

		if (commander == -1)
			commander = m_aPickable[0];

		order.Insert(commander);

		foreach (int i : m_aPickable)
		{
			if (i != commander && CRF_SlotBalancePlan.IsCommandRole(m_aRoles[i]))
				order.Insert(i);
		}

		foreach (int i : m_aPickable)
		{
			if (m_aRoles[i] == COA_EGearRole.MEDICAL_OFFICER || m_aRoles[i] == COA_EGearRole.MEDIC)
				order.Insert(i);
		}

		if (keyOnly)
			return order;

		foreach (int i : m_aPickable)
		{
			if (!order.Contains(i))
				order.Insert(i);
		}

		return order;
	}

	//------------------------------------------------------------------------------------------------
	//! Squad lead, medics, crew seats, then fire teams. A team lead only opens with two members
	//! under them, and a multi-team squad never opens just one team lead.
	protected array<int> GetSquadPickOrder(int n)
	{
		if (n >= m_aPickable.Count())
			return m_aPickable;

		array<int> order = {};
		if (n <= 0 || m_aPickable.IsEmpty())
			return order;

		int leader = -1;
		foreach (int i : m_aPickable)
		{
			if (m_aRoles[i] == COA_EGearRole.SQUAD_LEAD)
			{
				leader = i;
				break;
			}
		}

		if (leader == -1)
			leader = m_aPickable[0];

		array<int> reserved = {};
		array<int> crewSeats = {};
		array<int> headerOthers = {};
		array<int> teamLeads = {};
		array<ref array<int>> teamMembers = {};

		foreach (int i : m_aPickable)
		{
			if (i == leader)
				continue;

			int role = m_aRoles[i];
			if (role == COA_EGearRole.TEAM_LEAD)
			{
				teamLeads.Insert(i);
				teamMembers.Insert(new array<int>());
				continue;
			}

			if (!teamLeads.IsEmpty())
			{
				teamMembers[teamMembers.Count() - 1].Insert(i);
				continue;
			}

			if (role == COA_EGearRole.MEDIC)
				reserved.Insert(i);
			else if (CRF_SlotBalancePlan.IsCrewSeatRole(role))
				crewSeats.Insert(i);
			else
				headerOthers.Insert(i);
		}

		reserved.InsertAll(crewSeats);

		// How many fire teams keep their team lead
		int teamCount = teamLeads.Count();
		int keptTeams;
		int need = 1 + reserved.Count();
		for (int t = 0; t < teamCount; t++)
		{
			need += 1 + CRF_SlotBalancePlan.IntMin(2, teamMembers[t].Count());
			if (need > n)
				break;

			keptTeams = t + 1;
		}

		if (teamCount >= 2 && keptTeams == 1)
			keptTeams = 0;

		order.Insert(leader);
		order.InsertAll(reserved);

		// No team leads: riflemen before the RTO and other header slots
		if (keptTeams == 0)
		{
			array<int> flat = {};
			foreach (array<int> members : teamMembers)
			{
				flat.InsertAll(members);
			}

			for (int f = 0; f < flat.Count() && f < 2; f++)
			{
				order.Insert(flat[f]);
			}

			order.InsertAll(headerOthers);

			for (int f = 2; f < flat.Count(); f++)
			{
				order.Insert(flat[f]);
			}

			order.Resize(CRF_SlotBalancePlan.IntMin(n, order.Count()));
			return order;
		}

		for (int t = 0; t < keptTeams; t++)
		{
			order.Insert(teamLeads[t]);
		}

		// Round-robin so the teams stay even
		for (int r = 0; r < 2; r++)
		{
			for (int t = 0; t < keptTeams; t++)
			{
				if (r < teamMembers[t].Count())
					order.Insert(teamMembers[t][r]);
			}
		}

		order.InsertAll(headerOthers);

		int longestTeam;
		foreach (array<int> members : teamMembers)
		{
			longestTeam = CRF_SlotBalancePlan.IntMax(longestTeam, members.Count());
		}

		for (int r = 2; r < longestTeam; r++)
		{
			for (int t = 0; t < keptTeams; t++)
			{
				if (r < teamMembers[t].Count())
					order.Insert(teamMembers[t][r]);
			}
		}

		for (int t = keptTeams; t < teamCount; t++)
		{
			order.InsertAll(teamMembers[t]);
		}

		order.Resize(CRF_SlotBalancePlan.IntMin(n, order.Count()));
		return order;
	}
}

//------------------------------------------------------------------------------------------------
//! Picks which of one faction's slots stay open for a budget. Priorities, highest first:
//!  1. Manoeuvre squads: as many as fit at PREFERRED_SQUAD_SIZE, but two at MIN_SQUAD_SIZE beat
//!     one full one. An HQ opens only with two or more open elements under it.
//!  2. Squads grow to PREFERRED_SQUAD_SIZE.
//!  3. HQ second in command and medics.
//!  4. Support and crews in ORBAT order, if they fit. JTAC/FO open only once a pilot/indirect slot is open.
//!  5. HQs that now qualify.
//!  6. Squads grow to full, then support squads, then HQs.
//!  7. Leftovers, so every player has a slot.
//!
//! There is no explicit hierarchy: elements belong to the platoon HQ before them in ORBAT order,
//! and platoon HQs to the company HQ before them.
class CRF_SlotBalancePlan
{
	static const int MIN_SQUAD_SIZE = 4;
	static const int PREFERRED_SQUAD_SIZE = 6;

	protected ref array<ref CRF_BalanceElement> m_aElements = {};
	protected int m_iBudget;
	protected int m_iUsed;

	//------------------------------------------------------------------------------------------------
	static int IntMin(int a, int b)
	{
		if (a < b)
			return a;

		return b;
	}

	//------------------------------------------------------------------------------------------------
	static int IntMax(int a, int b)
	{
		if (a > b)
			return a;

		return b;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCommandRole(int role)
	{
		return role == COA_EGearRole.COMPANY_COMMANDER || role == COA_EGearRole.FIRST_SERGEANT
			|| role == COA_EGearRole.PLATOON_LEADER || role == COA_EGearRole.PLATOON_SERGEANT;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsControllerRole(int role)
	{
		return role == COA_EGearRole.JTAC || role == COA_EGearRole.FORWARD_OBSERVER;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsControlledBy(int controllerRole, int role)
	{
		if (controllerRole == COA_EGearRole.JTAC)
			return role == COA_EGearRole.PILOT;

		if (controllerRole == COA_EGearRole.FORWARD_OBSERVER)
			return role == COA_EGearRole.INDIRECT_LEAD || role == COA_EGearRole.INDIRECT_GUNNER || role == COA_EGearRole.INDIRECT_LOADER;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCrewSeatRole(int role)
	{
		switch (role)
		{
			case COA_EGearRole.VEHICLE_LEAD:
			case COA_EGearRole.VEHICLE_DRIVER:
			case COA_EGearRole.VEHICLE_GUNNER:
			case COA_EGearRole.VEHICLE_LOADER:
			case COA_EGearRole.PILOT:
			case COA_EGearRole.CREW_CHIEF:
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCrewRole(int role)
	{
		if (IsCrewSeatRole(role))
			return true;

		switch (role)
		{
			case COA_EGearRole.INDIRECT_LEAD:
			case COA_EGearRole.INDIRECT_GUNNER:
			case COA_EGearRole.INDIRECT_LOADER:
			case COA_EGearRole.LOGI_LEAD:
			case COA_EGearRole.LOGI_RUNNER:
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsSupportFlag(int flag)
	{
		switch (flag)
		{
			case COA_EFlagType.ANTI_AIR:
			case COA_EFlagType.ANTI_AIR_ARTILLERY:
			case COA_EFlagType.ANTI_ARMOR_MOTORIZED:
			case COA_EFlagType.ANTI_ARMOR:
			case COA_EFlagType.ARMORED:
			case COA_EFlagType.ARTILLERY:
			case COA_EFlagType.HELICOPER:
			case COA_EFlagType.ATTACK_HELICOPTER:
			case COA_EFlagType.MACHINEGUN:
			case COA_EFlagType.MAINTENANCE:
			case COA_EFlagType.MEDICAL:
			case COA_EFlagType.MORTAR:
			case COA_EFlagType.SIGNAL:
			case COA_EFlagType.SNIPER:
			case COA_EFlagType.SUPPLY_MOTORIZED:
				return true;
		}

		return false;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 BUILDING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Skips dead slots, hand-locked slots and Game Master possession groups
	void Build(FactionKey factionKey, set<int> lockedByBalance)
	{
		m_aElements.Clear();

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!slottingManager || !gamemode)
			return;

		COA_GMPossessionManager possessionManager = COA_GMPossessionManager.GetInstance();

		array<SCR_AIGroup> groups = {};
		foreach (SCR_AIGroup group : slottingManager.GetAllGroups(factionKey))
		{
			if (group && !possessionManager.IsGroupAlreadyRegistered(group))
				groups.Insert(group);
		}

		// Group flags come from the config, matched by creation order. If the counts don't line up,
		// classify on roles alone.
		array<ref COA_SlottingGroup> configGroups = gamemode.GetSlots(factionKey);
		bool configMatches = configGroups && configGroups.Count() == groups.Count();

		int currentCompany = -1;
		int currentPlatoon = -1;
		foreach (int groupIndex, SCR_AIGroup group : groups)
		{
			RplId groupId;
			if (!COA_ReplicationHelper.GetRplId(group, groupId))
				continue;

			array<int> slotIds = slottingManager.GetAllSlotIDsForGroup(groupId);

			int flag = -1;
			if (configMatches && configGroups[groupIndex] && configGroups[groupIndex].m_aSlots && configGroups[groupIndex].m_aSlots.Count() == slotIds.Count())
				flag = configGroups[groupIndex].m_FlagType;

			CRF_BalanceElement element = new CRF_BalanceElement();
			foreach (int slotId : slotIds)
			{
				COA_SlotData slotData = slottingManager.GetSlotData(slotId);
				if (!slotData || slotData.GetIsDeadSlot() || slotData.GetSlotFactionKey() != factionKey)
					continue;

				if (slotData.GetIsLockedSlot() && !lockedByBalance.Contains(slotId))
					continue;

				int localIndex = element.m_aSlotIds.Count();
				int role = slotData.GetSlotRole();
				element.m_aSlotIds.Insert(slotId);
				element.m_aRoles.Insert(role);
				element.m_aOccupied.Insert(slotData.GetSlotCurrentPlayerId() > 0);

				if (IsControllerRole(role))
				{
					element.m_aControllers.Insert(localIndex);
					element.m_aControllerOpen.Insert(false);
				}
				else
				{
					element.m_aPickable.Insert(localIndex);
				}
			}

			if (element.m_aSlotIds.IsEmpty())
				continue;

			element.m_eKind = Classify(element, flag);

			int elementIndex = m_aElements.Count();
			if (element.m_eKind == CRF_EBalanceElementKind.COMPANY_HQ)
			{
				currentCompany = elementIndex;
				currentPlatoon = -1;
			}
			else if (element.m_eKind == CRF_EBalanceElementKind.PLATOON_HQ)
			{
				element.m_iParent = currentCompany;
				currentPlatoon = elementIndex;
			}
			else if (currentPlatoon != -1)
			{
				element.m_iParent = currentPlatoon;
			}
			else
			{
				element.m_iParent = currentCompany;
			}

			m_aElements.Insert(element);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_EBalanceElementKind Classify(CRF_BalanceElement element, int flag)
	{
		bool hasTeamLead;
		bool allCrew = true;
		bool platoonCommand;
		foreach (int role : element.m_aRoles)
		{
			if (role == COA_EGearRole.COMPANY_COMMANDER || role == COA_EGearRole.FIRST_SERGEANT)
				return CRF_EBalanceElementKind.COMPANY_HQ;

			if (role == COA_EGearRole.PLATOON_LEADER || role == COA_EGearRole.PLATOON_SERGEANT)
				platoonCommand = true;

			if (role == COA_EGearRole.TEAM_LEAD)
				hasTeamLead = true;

			if (!IsCrewRole(role))
				allCrew = false;
		}

		if (platoonCommand)
			return CRF_EBalanceElementKind.PLATOON_HQ;

		if (allCrew)
			return CRF_EBalanceElementKind.CREW;

		if (!hasTeamLead && IsSupportFlag(flag))
			return CRF_EBalanceElementKind.SUPPORT_SQUAD;

		return CRF_EBalanceElementKind.SQUAD;
	}

	//------------------------------------------------------------------------------------------------
	void GetManagedSlots(notnull array<int> outSlots)
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			outSlots.InsertAll(element.m_aSlotIds);
		}
	}

	//------------------------------------------------------------------------------------------------
	void GetOccupiedSlots(notnull set<int> outSlots)
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			foreach (int i, int slotId : element.m_aSlotIds)
			{
				if (element.m_aOccupied[i])
					outSlots.Insert(slotId);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void GetOpenSlots(notnull set<int> outSlots)
	{
		array<int> indices = {};
		foreach (CRF_BalanceElement element : m_aElements)
		{
			element.GetOpenIndices(indices);
			foreach (int localIndex : indices)
			{
				outSlots.Insert(element.m_aSlotIds[localIndex]);
			}
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 PLANNING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	void Plan(int budget)
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			element.m_iOpen = 0;
			for (int c = 0; c < element.m_aControllerOpen.Count(); c++)
			{
				element.m_aControllerOpen[c] = false;
			}
		}

		m_iBudget = budget;
		m_iUsed = 0;
		if (budget <= 0)
			return;

		OpenSquadStructure();
		GrowElements(CRF_EBalanceElementKind.SQUAD, true);
		GrowHQs(true);

		for (int pass = 0; pass < 3 && m_iUsed < m_iBudget; pass++)
		{
			int usedBefore = m_iUsed;

			OpenSupportElements();
			OpenQualifiedHQs();
			OpenControllers();
			GrowElements(CRF_EBalanceElementKind.SQUAD, false);
			GrowElements(CRF_EBalanceElementKind.SUPPORT_SQUAD, false);
			GrowHQs(false);
			OpenRemainingElements();

			if (m_iUsed == usedBefore)
				break;
		}

		FillLeftovers();
	}

	//------------------------------------------------------------------------------------------------
	protected int Remaining()
	{
		return m_iBudget - m_iUsed;
	}

	//------------------------------------------------------------------------------------------------
	//! Priority 1
	protected void OpenSquadStructure()
	{
		array<int> squads = {};
		foreach (int i, CRF_BalanceElement element : m_aElements)
		{
			if (element.m_eKind == CRF_EBalanceElementKind.SQUAD)
				squads.Insert(i);
		}

		// Most squads that can all reach the preferred size
		int count;
		for (int c = 1; c <= squads.Count(); c++)
		{
			if (GetStructureCost(squads, c, true) > m_iBudget)
				break;

			count = c;
		}

		// but two small squads beat one full one
		if (count < 2)
		{
			for (int c = IntMin(2, squads.Count()); c > count; c--)
			{
				if (GetStructureCost(squads, c, false) <= m_iBudget)
				{
					count = c;
					break;
				}
			}
		}

		// A platoon's lone squad would have no HQ; give its slots to the open squads
		while (count > 2 && IsLoneSquadOfPlatoon(squads, count))
		{
			count--;
		}

		for (int j = 0; j < count; j++)
		{
			CRF_BalanceElement squad = m_aElements[squads[j]];
			squad.m_iOpen = squad.GetMinSize();
			m_iUsed += squad.m_iOpen;
		}

		OpenQualifiedHQs();
	}

	//------------------------------------------------------------------------------------------------
	//! First `count` squads plus the HQs they would qualify
	protected int GetStructureCost(array<int> squads, int count, bool preferred)
	{
		array<int> children = {};
		children.Resize(m_aElements.Count());

		int cost;
		for (int j = 0; j < count; j++)
		{
			CRF_BalanceElement squad = m_aElements[squads[j]];
			if (preferred)
				cost += squad.GetPreferredSize();
			else
				cost += squad.GetMinSize();

			if (squad.m_iParent != -1)
				children[squad.m_iParent] = children[squad.m_iParent] + 1;
		}

		foreach (int i, CRF_BalanceElement element : m_aElements)
		{
			if (element.m_eKind != CRF_EBalanceElementKind.PLATOON_HQ || children[i] < 2)
				continue;

			cost++;
			if (element.m_iParent != -1)
				children[element.m_iParent] = children[element.m_iParent] + 1;
		}

		foreach (int i, CRF_BalanceElement element : m_aElements)
		{
			if (element.m_eKind == CRF_EBalanceElementKind.COMPANY_HQ && children[i] >= 2)
				cost++;
		}

		return cost;
	}

	//------------------------------------------------------------------------------------------------
	//! true if the count-th squad would be the only open squad of a platoon with several
	protected bool IsLoneSquadOfPlatoon(array<int> squads, int count)
	{
		int parent = m_aElements[squads[count - 1]].m_iParent;
		if (parent == -1 || m_aElements[parent].m_eKind != CRF_EBalanceElementKind.PLATOON_HQ)
			return false;

		int openInPlatoon;
		int totalInPlatoon;
		foreach (int j, int squadIndex : squads)
		{
			if (m_aElements[squadIndex].m_iParent != parent)
				continue;

			totalInPlatoon++;
			if (j < count)
				openInPlatoon++;
		}

		return openInPlatoon == 1 && totalInPlatoon > 1;
	}

	//------------------------------------------------------------------------------------------------
	//! One slot at a time to the smallest, so elements stay even
	protected void GrowElements(CRF_EBalanceElementKind kind, bool toPreferred)
	{
		while (Remaining() > 0)
		{
			CRF_BalanceElement smallest;
			foreach (CRF_BalanceElement element : m_aElements)
			{
				if (element.m_eKind != kind || !element.IsOpen())
					continue;

				int cap = element.GetPickableCount();
				if (toPreferred)
					cap = element.GetPreferredSize();

				if (element.m_iOpen >= cap)
					continue;

				if (!smallest || element.m_iOpen < smallest.m_iOpen)
					smallest = element;
			}

			if (!smallest)
				return;

			smallest.m_iOpen++;
			m_iUsed++;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \param keyOnly stop at second in command and medics
	protected void GrowHQs(bool keyOnly)
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			if (!element.IsHQ() || !element.IsOpen())
				continue;

			int cap = element.GetPickableCount();
			if (keyOnly)
				cap = element.GetPreferredSize();

			while (element.m_iOpen < cap && Remaining() > 0)
			{
				element.m_iOpen++;
				m_iUsed++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenSupportElements()
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			if (element.IsOpen())
				continue;

			if (element.m_eKind != CRF_EBalanceElementKind.SUPPORT_SQUAD && element.m_eKind != CRF_EBalanceElementKind.CREW)
				continue;

			OpenAtMinimum(element);

			// A JTAC/FO comes before the next aircraft or gun
			if (element.IsOpen())
				OpenControllers();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Open elements first, then closed ones. Never HQs.
	protected void FillLeftovers()
	{
		array<bool> passes = {true, false};
		foreach (bool wantOpen : passes)
		{
			foreach (CRF_BalanceElement element : m_aElements)
			{
				if (element.IsHQ() || element.IsOpen() != wantOpen)
					continue;

				while (element.m_iOpen < element.GetPickableCount() && Remaining() > 0)
				{
					element.m_iOpen++;
					m_iUsed++;
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenRemainingElements()
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			if (!element.IsOpen() && !element.IsHQ())
				OpenAtMinimum(element);
		}

		OpenQualifiedHQs();
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenAtMinimum(CRF_BalanceElement element)
	{
		int size = element.GetMinSize();
		if (size <= 0 || size > Remaining())
			return;

		element.m_iOpen = size;
		m_iUsed += size;
	}

	//------------------------------------------------------------------------------------------------
	//! Platoons first, so they count towards their company
	protected void OpenQualifiedHQs()
	{
		array<CRF_EBalanceElementKind> order = {CRF_EBalanceElementKind.PLATOON_HQ, CRF_EBalanceElementKind.COMPANY_HQ};
		foreach (CRF_EBalanceElementKind kind : order)
		{
			foreach (int i, CRF_BalanceElement element : m_aElements)
			{
				if (element.m_eKind != kind || element.IsOpen() || element.GetPickableCount() == 0 || Remaining() <= 0)
					continue;

				if (GetOpenChildCount(i) < 2)
					continue;

				element.m_iOpen = 1;
				m_iUsed++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Crews don't count: an HQ isn't worth opening just to command them
	protected int GetOpenChildCount(int parentIndex)
	{
		int count;
		foreach (CRF_BalanceElement element : m_aElements)
		{
			if (element.m_iParent == parentIndex && element.IsOpen() && element.m_eKind != CRF_EBalanceElementKind.CREW)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected void OpenControllers()
	{
		foreach (CRF_BalanceElement element : m_aElements)
		{
			foreach (int c, int localIndex : element.m_aControllers)
			{
				if (element.m_aControllerOpen[c] || Remaining() <= 0)
					continue;

				if (!HasOpenRoleFor(element.m_aRoles[localIndex]))
					continue;

				element.m_aControllerOpen[c] = true;
				m_iUsed++;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasOpenRoleFor(int controllerRole)
	{
		array<int> indices = {};
		foreach (CRF_BalanceElement element : m_aElements)
		{
			if (!element.IsOpen())
				continue;

			element.GetOpenIndices(indices);
			foreach (int localIndex : indices)
			{
				if (IsControlledBy(controllerRole, element.m_aRoles[localIndex]))
					return true;
			}
		}

		return false;
	}
}
