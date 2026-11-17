/**
 * JIP (Join In Progress) forward deploy screen. Offered once to a player who joined while the round
 * was live (see COA_PlayerController.OnControlledEntityChanged), and reopened with /fd until they
 * use it. Lets a late joiner teleport to a live friendly unit instead of travelling from their slot's
 * spawn. Header + panel are shared with the respawn screen (COA_DeployPanel).
 *
 * - BFT on for the player's faction: fullscreen map and a list of friendly units with someone alive,
 *   the player's own squad first. Selecting one pans the map to it.
 * - BFT off: no map, and only the player's own squad is offered (no tracking of other units).
 *
 * The request goes to the server (CRF_COA_PlayerRplToAuthorityManager.RpcAsk_RequestJIPForwardDeploy),
 * which re-validates it - eligibility, side, unit alive, not near the enemy - and always answers
 * (OnServerResult). The screen stays open until then, so a refusal is shown here and the player can
 * pick another unit.
 */
class CRF_JIPForwardDeployMenu: ChimeraMenuBase
{
	//! How often the unit list (positions, alive counts) is refreshed
	protected static const int REFRESH_INTERVAL_MS = 2000;
	//! How long to wait for the server's answer before letting the player try again
	protected static const int REQUEST_TIMEOUT_MS = 6000;

	protected static CRF_JIPForwardDeployMenu s_Instance;

	// Hover/press feedback and entrance animation - see COA_UIPolish
	protected ref COA_MenuPolish m_UIPolish;
	protected ref COA_DeployPanel m_Panel;
	protected ref COA_DeployMarkers m_Markers = new COA_DeployMarkers();

	protected SCR_MapEntity m_MapEntity;
	protected SCR_ListBoxComponent m_wUnitListBox;
	protected ref array<SCR_AIGroup> m_aGroups = {};
	protected SCR_AIGroup m_SelectedGroup;
	protected SCR_AIGroup m_OwnGroup;
	protected bool m_bBFTEnabled;
	protected bool m_bRequestPending;
	protected bool m_bUnavailable;

//------------------------------------------------------------------------------------------------
//	MENU LIFECYCLE
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnMenuInit()
	{
		super.OnMenuInit();

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		s_Instance = this;

		m_Panel = new COA_DeployPanel(GetRootWidget());
		m_Panel.SetTitle("Forward Deploy");
		m_Panel.SetTimer(string.Empty, string.Empty);
		m_Panel.SetHeaderInfo("Joined in progress");
		m_Panel.SetSecondaryText("Stay at spawn");
		m_Panel.SetHints("<action name='MenuBack'/> Stay at spawn     Type /fd in chat to come back here");

		SCR_ButtonTextComponent deployButton = m_Panel.GetPrimaryButton();
		if (deployButton)
			deployButton.m_OnClicked.Insert(OnDeployButtonClicked);

		SCR_ButtonTextComponent stayButton = m_Panel.GetSecondaryButton();
		if (stayButton)
			stayButton.m_OnClicked.Insert(Action_Exit);

		m_wUnitListBox = m_Panel.GetList();
		if (m_wUnitListBox)
			m_wUnitListBox.m_OnChanged.Insert(OnSelectionChanged);

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);

		m_bBFTEnabled = IsBFTEnabledForLocalPlayer();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
			m_OwnGroup = slottingManager.GetPlayerSlotGroup(GetGame().GetPlayerController().GetPlayerId());

		if (m_bBFTEnabled && m_MapEntity)
		{
			GetGame().GetCallqueue().CallLater(OpenMapWithConfig, 100);
		}
		else
		{
			Widget mapFrame = GetRootWidget().FindAnyWidget("MapFrame");
			if (mapFrame)
				mapFrame.SetVisible(false);
		}

		RefreshUnits(true);
		GetGame().GetCallqueue().CallLater(RefreshUnits, REFRESH_INTERVAL_MS, true, false);

		// Hover feedback on buttons without their own, and a quick staggered fade-in (COA_UIPolish)
		m_UIPolish = new COA_MenuPolish(GetRootWidget(), true);
	}

	//------------------------------------------------------------------------------------------------
	//! Keeps map interaction active while the menu is open (BFT on only)
	override void OnMenuUpdate(float tDelta)
	{
		if (m_UIPolish)
			m_UIPolish.Update(tDelta);

		super.OnMenuUpdate(tDelta);

		if (m_bBFTEnabled && m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		super.OnMenuClose();

		if (s_Instance == this)
			s_Instance = null;

		// Cancel every pending call into this menu BEFORE anything else. The call queue keeps
		// running after the menu is destroyed, so a call scheduled here and not cancelled fires
		// against a freed object - closing the menu inside the scheduling window (spamming Esc
		// right after opening) is enough to hit it. Remove() on an unscheduled method is a no-op.
		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(RefreshUnits);
			GetGame().GetCallqueue().Remove(OpenMapWithConfig);
			GetGame().GetCallqueue().Remove(AdjustMapZoom);
			GetGame().GetCallqueue().Remove(OnRequestTimedOut);
			GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		}

		m_Markers.Clear();

		if (m_bBFTEnabled && m_MapEntity)
			m_MapEntity.CloseMap();
	}

//------------------------------------------------------------------------------------------------
//	UNIT LIST
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Rebuild the unit list and markers, keeping the selection on the same unit
	//! \param[in] initial first build: select the player's own squad (or the first unit) and pan to it
	protected void RefreshUnits(bool initial)
	{
		if (!m_wUnitListBox)
			return;

		m_aGroups.Clear();
		m_wUnitListBox.Clear();
		m_Markers.Clear();

		vector playerPosition;
		bool hasPlayerPosition;
		IEntity controlled = GetGame().GetPlayerController().GetControlledEntity();
		if (controlled)
		{
			playerPosition = controlled.GetOrigin();
			hasPlayerPosition = true;
		}

		foreach (SCR_AIGroup group : GetCandidateGroups())
		{
			vector groupOrigin;
			if (!COA_EntityHelper.GetGroupOrigin(group, groupOrigin))
				continue; // No living, spawned members - nothing to deploy to

			m_aGroups.Insert(group);

			string name = group.GetCustomNameWithOriginal();
			bool own = group == m_OwnGroup;
			if (own)
				name = name + "  (your squad)";

			string detail = COA_DeployPanel.Plural(CountAliveMembers(group), "alive", "alive");
			if (hasPlayerPosition && m_bBFTEnabled)
				detail = string.Format("%1 · %2", detail, COA_DeployPanel.FormatDistance(vector.DistanceXZ(playerPosition, groupOrigin)));

			m_Panel.AddRow(name, detail);

			if (m_bBFTEnabled)
			{
				if (own)
					m_Markers.Add(group.GetCustomNameWithOriginal(), groupOrigin, ARGB(255, 90, 235, 120));
				else
					m_Markers.Add(group.GetCustomNameWithOriginal(), groupOrigin, ARGB(255, 0, 170, 0));
			}
		}

		// Selection: the same unit as before, else (first build, or it's gone) own squad / first
		int index = m_aGroups.Find(m_SelectedGroup);
		if (index == -1)
		{
			index = m_aGroups.Find(m_OwnGroup);
			if (index == -1 && !m_aGroups.IsEmpty())
				index = 0;

			initial = true; // a different unit is selected now - show it
		}

		if (index != -1)
		{
			m_wUnitListBox.SetItemSelected(index, true, false, true);
			m_SelectedGroup = m_aGroups[index];
			if (initial)
				PanMapToGroup(m_SelectedGroup);
		}
		else
		{
			m_SelectedGroup = null;
		}

		UpdatePanel();
	}

	//------------------------------------------------------------------------------------------------
	//! The units this player may deploy to, own squad first: every playable group of their side with
	//! BFT, only their own squad without it
	protected array<SCR_AIGroup> GetCandidateGroups()
	{
		array<SCR_AIGroup> groups = {};

		if (!m_bBFTEnabled)
		{
			if (m_OwnGroup)
				groups.Insert(m_OwnGroup);

			return groups;
		}

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!slottingManager || !groupsManager)
			return groups;

		Faction playerFaction = slottingManager.GetPlayerSlotFaction(GetGame().GetPlayerController().GetPlayerId(), true);
		if (!playerFaction)
			return groups;

		if (m_OwnGroup)
			groups.Insert(m_OwnGroup);

		foreach (SCR_AIGroup group : groupsManager.GetPlayableGroupsByFaction(playerFaction))
		{
			if (group && group != m_OwnGroup)
				groups.Insert(group);
		}

		return groups;
	}

	//------------------------------------------------------------------------------------------------
	//! Living, spawned players in the group this client can see
	protected int CountAliveMembers(SCR_AIGroup group)
	{
		int alive;
		vector unused;
		foreach (int memberId : group.GetPlayerIDs())
		{
			if (COA_EntityHelper.GetLiveCharacterOrigin(GetGame().GetPlayerManager().GetPlayerControlledEntity(memberId), unused))
				alive++;
		}

		return alive;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSelectionChanged()
	{
		int index = m_wUnitListBox.GetSelectedItem();
		if (index < 0 || index >= m_aGroups.Count())
			return;

		m_SelectedGroup = m_aGroups[index];
		PanMapToGroup(m_SelectedGroup);
		UpdatePanel();
	}

	//------------------------------------------------------------------------------------------------
	protected void PanMapToGroup(SCR_AIGroup group)
	{
		// Before the map is open (first build) AdjustMapZoom pans once it is
		if (!m_bBFTEnabled || !m_MapEntity || !m_MapEntity.IsOpen() || !group)
			return;

		vector groupOrigin;
		if (COA_EntityHelper.GetGroupOrigin(group, groupOrigin))
			m_MapEntity.ZoomPanSmooth(0.5, groupOrigin[0], groupOrigin[2]);
	}

//------------------------------------------------------------------------------------------------
//	PANEL STATE
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void UpdatePanel()
	{
		m_Panel.SetListTitle("Friendly units", COA_DeployPanel.Plural(m_aGroups.Count(), "unit", "units"));

		if (m_aGroups.IsEmpty())
		{
			if (m_bBFTEnabled)
				m_Panel.SetEmptyText("No friendly unit has anyone alive right now.");
			else
				m_Panel.SetEmptyText("Nobody in your squad is alive right now.");
		}
		else
		{
			m_Panel.SetEmptyText(string.Empty);
		}

		// While waiting on the server, or after it said no for good, the status line is left as set
		if (m_bRequestPending || m_bUnavailable)
			return;

		if (!m_SelectedGroup)
		{
			m_Panel.SetPrimary("Forward deploy", false);
			m_Panel.SetStatus("Stay at your spawn, or check back with /fd once a unit is available.", COA_EDeployTone.WARNING);
			return;
		}

		m_Panel.SetPrimary("Forward deploy");

		string status = string.Format("Deploy to %1's position. You can only do this once.", m_SelectedGroup.GetCustomNameWithOriginal());
		if (!m_bBFTEnabled)
			status = string.Format("Deploy to your squad, %1. You can only do this once. Blue Force Tracking is off for your side, so other units aren't listed.", m_SelectedGroup.GetCustomNameWithOriginal());

		m_Panel.SetStatus(status);
	}

//------------------------------------------------------------------------------------------------
//	REQUEST
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Ask the server to deploy to the selected unit; the answer arrives in OnServerResult
	protected void OnDeployButtonClicked()
	{
		if (m_bRequestPending || m_bUnavailable || !m_SelectedGroup)
			return;

		RplId groupRplId = Replication.FindItemId(m_SelectedGroup);
		if (groupRplId == RplId.Invalid())
			return;

		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!rplManager)
			return;

		m_bRequestPending = true;
		m_Panel.SetPrimary("Requesting...", false);
		m_Panel.SetStatus(string.Format("Checking %1 is clear to deploy to...", m_SelectedGroup.GetCustomNameWithOriginal()));

		rplManager.RequestJIPForwardDeploy(groupRplId, GetGame().GetPlayerController().GetPlayerId());
		GetGame().GetCallqueue().CallLater(OnRequestTimedOut, REQUEST_TIMEOUT_MS);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRequestTimedOut()
	{
		if (!m_bRequestPending)
			return;

		m_bRequestPending = false;
		UpdatePanel();
		m_Panel.SetStatus("No answer from the server. Try again.", COA_EDeployTone.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	//! Client: the server's answer to a forward deploy request (CRF_COA_PlayerRplToOwnerManager).
	//! Accepted closes the screen; refused shows why and lets the player pick again, unless they
	//! can't forward deploy at all.
	static void OnServerResult(bool accepted, string message)
	{
		COA_PlayerController pc = COA_PlayerController.Cast(GetGame().GetPlayerController());
		bool canRetry = pc && pc.CanForwardDeploy();

		if (!s_Instance)
		{
			// Screen already closed - say it as a hint instead
			ShowHint(message);
			return;
		}

		s_Instance.HandleServerResult(accepted, canRetry, message);
	}

	//------------------------------------------------------------------------------------------------
	protected void HandleServerResult(bool accepted, bool canRetry, string message)
	{
		GetGame().GetCallqueue().Remove(OnRequestTimedOut);
		m_bRequestPending = false;

		if (accepted)
		{
			ShowHint(message);
			GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_JIPForwardDeployMenu);
			return;
		}

		if (!canRetry)
		{
			m_bUnavailable = true;
			m_Panel.SetPrimary("Unavailable", false);
			m_Panel.SetStatus(message, COA_EDeployTone.BAD);
			return;
		}

		UpdatePanel();
		m_Panel.SetStatus(message, COA_EDeployTone.BAD);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ShowHint(string message)
	{
		COA_RplBroadcastManager broadcastManager = COA_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.ShowLocalHint("[Forward Deploy] " + message, 6000);
	}

	//------------------------------------------------------------------------------------------------
	//! Close without forward deploying - the player keeps their spawn position
	void Action_Exit()
	{
		if (GetGame().GetMenuManager().GetTopMenu() != this)
			return;

		COA_PlayerController pc = COA_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_bRequestPending && pc && pc.CanForwardDeploy())
			ShowHint("Staying at your spawn. Type /fd in chat to forward deploy later.");

		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_JIPForwardDeployMenu);
	}

//------------------------------------------------------------------------------------------------
//	MAP
//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void OpenMapWithConfig()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode || !m_MapEntity)
			return;

		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;

		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);

		GetGame().GetCallqueue().CallLater(AdjustMapZoom, 100);
	}

	//------------------------------------------------------------------------------------------------
	//! Zoom out once the map is open, then centre on the selected unit
	protected void AdjustMapZoom()
	{
		if (!m_MapEntity)
			return;

		m_MapEntity.ZoomOut();
		PanMapToGroup(m_SelectedGroup);
	}

	//------------------------------------------------------------------------------------------------
	//! Checks whether BFT is enabled for the local player's faction
	protected bool IsBFTEnabledForLocalPlayer()
	{
		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!gamemode || !slottingManager)
			return false;

		Faction playerFaction = slottingManager.GetPlayerSlotFaction(GetGame().GetPlayerController().GetPlayerId(), true);
		if (!playerFaction)
			return false;

		return gamemode.IsSideBFTEnabled(playerFaction.GetFactionKey());
	}
}
