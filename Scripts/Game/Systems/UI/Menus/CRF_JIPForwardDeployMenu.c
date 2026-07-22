/**
 * JIP (Join In Progress) forward deploy menu for the Coalition Reforger Framework.
 * Shown to a player when they load into a character while JIP is enabled (Disable JIP off) and
 * SafeStart has already ended. Lets a late joiner teleport to a live friendly unit instead of
 * walking/driving back from their slot's spawn point.
 *
 * - If BFT is enabled for the player's faction: shows a fullscreen map with a list of friendly
 *   units (squads with at least one living, spawned member). Selecting an entry pans the map to
 *   it; "Forward Deploy" requests a teleport to the selected unit.
 * - If BFT is disabled: the map/list are hidden and "Forward Deploy" always targets the player's
 *   own slotted squad (no situational awareness of other units without BFT).
 *
 * The server (CRF_GameplayRplToAuthorityManager.RpcAsk_RequestJIPForwardDeploy) re-validates the
 * request and denies it (with an on-screen hint) if the target unit is in/near contact with the
 * enemy - see CRF_ForwardDeployManager.IsPositionNearEnemy().
 */
class CRF_JIPForwardDeployMenu: ChimeraMenuBase
{
	protected Widget m_wRoot;
	protected SCR_MapEntity m_MapEntity;
	protected OverlayWidget m_wUnitListRoot;
	protected SCR_ListBoxComponent m_wUnitListBox;
	protected SCR_ButtonTextComponent m_bDeployButton;
	protected ref array<SCR_AIGroup> m_aGroups = {};
	protected ref map<string, vector> m_MapMarkers = new map<string, vector>;
	protected int m_iSelectedIndex = -1;
	protected bool m_bBFTEnabled = false;

	/**
	 * Handles actions when the menu is opened: decides BFT-on (map + unit list) vs BFT-off
	 * (deploy button only) presentation, then sets up whichever is relevant.
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_wRoot = GetRootWidget();

		SetHeaderText();
		InitializeDeployButton();
		RegisterInputHandlers();

		m_bBFTEnabled = IsBFTEnabledForLocalPlayer();

		if (m_bBFTEnabled)
		{
			InitializeUnitSelection();

			if (!m_MapEntity)
				m_MapEntity = SCR_MapEntity.GetMapInstance();

			if (m_MapEntity)
				InitializeMap();
		}
		else
		{
			HideMapAndList();
		}
	}

	/**
	 * Initializes the map reference when the menu is first created.
	 */
	override void OnMenuInit()
	{
		super.OnMenuInit();

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
	}

	/**
	 * Keeps map interaction active while the menu is open (BFT-on only).
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		if (m_bBFTEnabled && m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
	}

	/**
	 * Cleans up resources when the menu is closed.
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();

		UnregisterInputHandlers();
		RemoveAllUnitMarkers();

		if (m_wUnitListBox)
			m_wUnitListBox.m_OnChanged.Remove(UpdateUnitSelection);

		if (m_bDeployButton)
			m_bDeployButton.m_OnClicked.Remove(OnDeployButtonClicked);

		if (m_bBFTEnabled && m_MapEntity)
			m_MapEntity.CloseMap();

		m_MapMarkers.Clear();
	}

	/**
	 * Sets the static header text (the copied layout's title still reads "Respawn in" by default).
	 */
	protected void SetHeaderText()
	{
		TextWidget titleWidget = TextWidget.Cast(m_wRoot.FindAnyWidget("timerTitle"));
		if (titleWidget)
			titleWidget.SetText("Forward Deploy");

		Widget countdownWidget = m_wRoot.FindAnyWidget("timerCountDown");
		if (countdownWidget)
			countdownWidget.SetVisible(false);
	}

	/**
	 * Hides the map/unit list for the BFT-disabled case, leaving only the deploy button.
	 */
	protected void HideMapAndList()
	{
		Widget mapFrame = m_wRoot.FindAnyWidget("MapFrame");
		if (mapFrame)
			mapFrame.SetVisible(false);

		Widget listRoot = m_wRoot.FindAnyWidget("List1Box");
		if (listRoot)
			listRoot.SetVisible(false);
	}

	/**
	 * Checks whether BFT is enabled for the local player's faction.
	 */
	protected bool IsBFTEnabledForLocalPlayer()
	{
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!gamemode || !slottingManager)
			return false;

		int playerId = GetGame().GetPlayerController().GetPlayerId();
		Faction playerFaction = slottingManager.GetPlayerSlotFaction(playerId, true);
		if (!playerFaction)
			return false;

		return gamemode.IsSideBFTEnabled(playerFaction.GetFactionKey());
	}

	/**
	 * Sets up the "Forward Deploy" button.
	 */
	protected void InitializeDeployButton()
	{
		m_bDeployButton = SCR_ButtonTextComponent.GetButtonText("ActionButton", m_wRoot);
		if (!m_bDeployButton)
			return;

		TextWidget buttonText = TextWidget.Cast(m_bDeployButton.GetRootWidget().FindWidget("ActionButtonText"));
		if (buttonText)
			buttonText.SetText("Forward Deploy");

		m_bDeployButton.m_OnClicked.Insert(OnDeployButtonClicked);
	}

	/**
	 * Populate the unit selection list with friendly units that currently have a living, spawned member.
	 */
	protected void InitializeUnitSelection()
	{
		m_wUnitListRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List1Box"));
		if (!m_wUnitListRoot)
			return;

		m_wUnitListBox = SCR_ListBoxComponent.Cast(m_wUnitListRoot.FindHandler(SCR_ListBoxComponent));
		if (!m_wUnitListBox)
			return;

		m_wUnitListBox.m_OnChanged.Insert(UpdateUnitSelection);
		PopulateUnitList();
	}

	/**
	 * Populates the list box + map markers with the player's faction's friendly units.
	 */
	protected void PopulateUnitList()
	{
		m_aGroups.Clear();
		m_wUnitListBox.Clear();
		RemoveAllUnitMarkers();

		int playerId = GetGame().GetPlayerController().GetPlayerId();
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		Faction playerFaction = slottingManager.GetPlayerSlotFaction(playerId, true);
		if (!playerFaction)
			return;

		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return;

		array<SCR_AIGroup> factionGroups = groupsManager.GetPlayableGroupsByFaction(playerFaction);

		int index = 0;
		foreach (SCR_AIGroup group : factionGroups)
		{
			vector groupOrigin;
			if (!CRF_EntityHelper.GetGroupOrigin(group, groupOrigin))
				continue; // No living, spawned members - nothing to deploy to.

			m_aGroups.Insert(group);

			string label = group.GetCustomNameWithOriginal();
			CreateUnitMarker(label, groupOrigin);
			m_wUnitListBox.AddItem(label);

			if (index == 0)
			{
				m_wUnitListBox.SetItemSelected(index, true, true, true);
				GetGame().GetCallqueue().CallLater(UpdateUnitSelection, 500, false);
			}
			index++;
		}
	}

	/**
	 * Tracks the current list selection and pans the map to it.
	 */
	protected void UpdateUnitSelection()
	{
		m_iSelectedIndex = m_wUnitListBox.GetSelectedItem();
		if (m_iSelectedIndex < 0 || m_iSelectedIndex >= m_aGroups.Count())
			return;

		vector groupOrigin;
		if (!CRF_EntityHelper.GetGroupOrigin(m_aGroups[m_iSelectedIndex], groupOrigin))
			return;

		if (m_MapEntity)
			m_MapEntity.ZoomPanSmooth(0.5, groupOrigin[0], groupOrigin[2]);
	}

	/**
	 * Registers all input action listeners for the menu.
	 */
	protected void RegisterInputHandlers()
	{
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
	}

	/**
	 * Unregisters all input action listeners when closing the menu.
	 */
	protected void UnregisterInputHandlers()
	{
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
	}

	/**
	 * Dismisses the menu without forward deploying (the player keeps their normal spawn position).
	 */
	void Action_Exit()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_JIPForwardDeployMenu);
	}

	/**
	 * Sends the forward deploy request for the selected unit (BFT on) or the player's own
	 * squad (BFT off), then closes the menu. The server re-validates and may deny the request.
	 */
	protected void OnDeployButtonClicked()
	{
		int playerId = GetGame().GetPlayerController().GetPlayerId();
		SCR_AIGroup targetGroup;

		if (m_bBFTEnabled)
		{
			if (m_iSelectedIndex < 0 || m_iSelectedIndex >= m_aGroups.Count())
				return;

			targetGroup = m_aGroups[m_iSelectedIndex];
		}
		else
		{
			CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
			if (!slottingManager)
				return;

			targetGroup = slottingManager.GetPlayerSlotGroup(playerId);
		}

		if (!targetGroup)
			return;

		RplId groupRplId = Replication.FindItemId(targetGroup);
		if (groupRplId == RplId.Invalid())
			return;

		CRF_GameplayRplToAuthorityManager rplManager = CRF_GameplayRplToAuthorityManager.GetInstance();
		if (rplManager)
			rplManager.RequestJIPForwardDeploy(groupRplId, playerId);

		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_JIPForwardDeployMenu);
	}

	/**
	 * Configures and opens the map with proper settings.
	 */
	protected void InitializeMap()
	{
		GetGame().GetCallqueue().CallLater(OpenMapWithConfig, 100);
	}

	/**
	 * Configures and opens the fullscreen map.
	 */
	protected void OpenMapWithConfig()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		SCR_MapConfigComponent configComp = SCR_MapConfigComponent.Cast(gameMode.FindComponent(SCR_MapConfigComponent));
		if (!configComp)
			return;

		MapConfiguration mapConfigFullscreen = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, "{1B8AC767E06A0ACD}Configs/Map/MapFullscreen.conf", GetRootWidget());
		m_MapEntity.OpenMap(mapConfigFullscreen);

		GetGame().GetCallqueue().CallLater(AdjustMapZoom, 100);
	}

	/**
	 * Adjusts map zoom level after map is opened.
	 */
	protected void AdjustMapZoom()
	{
		if (m_MapEntity)
			m_MapEntity.ZoomOut();
	}

	/**
	 * Creates a marker on the map for a friendly unit.
	 * \param name Unit's callsign, shown as the marker label.
	 * \param worldPos Unit's current world position.
	 */
	protected void CreateUnitMarker(string name, vector worldPos)
	{
		string worldPosFormatted = string.Format("%1 %2 %3", worldPos[0], worldPos[1], worldPos[2]);

		CRF_PlayerScriptedMarkerManager playerScriptedMarkerManager = CRF_PlayerScriptedMarkerManager.GetInstance();
		if (!playerScriptedMarkerManager)
			return;

		playerScriptedMarkerManager.AddScriptedMarker("Static Marker",
			worldPosFormatted,
			1,
			name,
			"{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds",
			50,
			ARGB(255, 0, 225, 0));

		m_MapMarkers.Insert(name, worldPos);
	}

	/**
	 * Removes all tracked unit markers (used on close/repopulate).
	 */
	protected void RemoveAllUnitMarkers()
	{
		CRF_PlayerScriptedMarkerManager psm = CRF_PlayerScriptedMarkerManager.GetInstance();
		foreach (string name, vector worldPos : m_MapMarkers)
		{
			if (!psm)
				break;
			string worldPosFormatted = string.Format("%1 %2 %3", worldPos[0], worldPos[1], worldPos[2]);
			psm.RemoveScriptedMarker("Static Marker",
				worldPosFormatted, 1, name,
				"{302979C3EAF01D2E}UI/Textures/Editor/ContentBrowser/ContentBrowser_Trait_SpawnPoint.edds",
				50, ARGB(255, 0, 225, 0));
		}
		m_MapMarkers.Clear();
	}
}
