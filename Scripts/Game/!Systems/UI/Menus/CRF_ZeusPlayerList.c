//------------------------------------------------------------------------------------------------
// Game Master (Zeus) player list
//
// A drawer on the left edge of the editor UI listing every connected player, grouped by faction.
// Closed, only a tab with the player count shows; hovering it slides the list out. Double-clicking
// a name moves the editor camera to that player.
//
// Players who need the Game Master are shown in red (and the tab turns red): anyone with an open
// admin ticket (polled from the server every few seconds), and anyone who pinged the Game Master in
// the last two minutes (until the Game Master double-clicks them).
//
// Owned by the modded EditorMenuUI (CRF_COA_EditorMenuUI.c): created in OnMenuOpen, updated
// every frame, destroyed in OnMenuClose. The panel only shows in the EDIT and ADMIN editor modes,
// and hides with the rest of the editor UI when the interface is toggled off.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! Row button in the player list - attached in CRF_ZeusPlayerListRow.layout
class CRF_ZeusPlayerRowButton : ScriptedWidgetComponent
{
	int m_iPlayerId;

	ref ScriptInvoker m_OnDoubleClicked = new ScriptInvoker(); // (int playerId)
	ref ScriptInvoker m_OnContextMenu = new ScriptInvoker(); // (int playerId, int screenX, int screenY)

	//------------------------------------------------------------------------------------------------
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (button == 0 && m_iPlayerId > 0)
			m_OnDoubleClicked.Invoke(m_iPlayerId);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Right click: quick actions for this player
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (button == 1 && m_iPlayerId > 0)
		{
			m_OnContextMenu.Invoke(m_iPlayerId, x, y);
			return true;
		}

		return false;
	}
}

//------------------------------------------------------------------------------------------------
//! Quick actions offered on a player row's right-click menu
enum CRF_EZeusPlayerAction
{
	CLOSE = -1,
	MOVE_CAMERA = 0,
	TELEPORT_TO_CAMERA,
	HEAL,
	SEND_TO_SPECTATOR,
	OPEN_TICKET,
}

//------------------------------------------------------------------------------------------------
//! Button in the quick actions menu - attached in CRF_ZeusPlayerActions.layout (the backdrop, which
//! closes the menu) and CRF_ZeusPlayerActionRow.layout (one per action)
class CRF_ZeusActionButton : ScriptedWidgetComponent
{
	[Attribute("0", desc: "CRF_EZeusPlayerAction this button performs; -1 closes the menu")]
	int m_iAction;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker(); // (int action)

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(m_iAction);

		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_ZeusPlayerEntry
{
	int m_iPlayerId;
	string m_sName;
	string m_sSortKey;
	string m_sStatus;
	bool m_bInactive; // dead or spectating
	bool m_bNeedsAttention; // open admin ticket, or pinged the Game Master recently
}

//------------------------------------------------------------------------------------------------
class CRF_ZeusFactionGroup
{
	Faction m_Faction; // null = players without a slot
	string m_sSortKey;
	ref array<ref CRF_ZeusPlayerEntry> m_aPlayers = {};
}

//------------------------------------------------------------------------------------------------
class CRF_ZeusPlayerList
{
	protected static const ResourceName PANEL_LAYOUT = "{C7A3E4D21B9F5A60}UI/layouts/Editor/CRF_ZeusPlayerList.layout";
	protected static const ResourceName ROW_LAYOUT = "{C7A3E4D21B9F5A61}UI/layouts/Editor/CRF_ZeusPlayerListRow.layout";

	// The panel is a drawer like the AAR menu's faction sidebar: tucked off the left edge with only
	// its tab showing, sliding out while the cursor is over it
	protected static const float PANEL_WIDTH = 304; // list + tab
	protected static const float PANEL_HEIGHT = 560;
	protected static const float TAB_WIDTH = 24;
	protected static const float DRAWER_CLOSED_X = -280; // TAB_WIDTH - PANEL_WIDTH: only the tab on screen
	protected static const float DRAWER_OPEN_X = 0;
	// Ease-out rate for the per-frame slide - 12 settles in about a quarter of a second
	protected static const float DRAWER_SLIDE_SPEED = 12;

	protected static const float REFRESH_INTERVAL = 1;
	// How often the server is asked which players have an open admin ticket
	protected static const float TICKET_POLL_INTERVAL = 3;
	// A ping keeps the player's name red for this long, or until they are double-clicked
	protected static const int PING_HIGHLIGHT_MS = 120000;

	protected static const ref Color HEADER_ROW_COLOR = Color.FromSRGBA(28, 31, 40, 255);
	protected static const ref Color NAME_COLOR = Color.FromSRGBA(239, 242, 247, 255);
	protected static const ref Color INACTIVE_NAME_COLOR = Color.FromSRGBA(140, 150, 171, 255);
	protected static const ref Color UNSLOTTED_COLOR = Color.FromSRGBA(140, 150, 171, 255);
	// Readable on the slate panel (about 5.5:1)
	protected static const ref Color ATTENTION_COLOR = Color.FromSRGBA(240, 82, 82, 255);
	protected static const ref Color ACCENT_COLOR = Color.FromSRGBA(201, 54, 54, 255);
	protected static const ref Color TAB_COUNT_COLOR = Color.FromSRGBA(169, 180, 204, 255);
	protected static const ref Color TAB_BG = Color.FromSRGBA(21, 23, 29, 250);
	protected static const ref Color TAB_ATTENTION_BG = Color.FromSRGBA(74, 22, 26, 250);

	// Players with an open admin ticket, as last reported by the server
	protected static ref array<int> s_aTicketHolders = {};
	// System tick count of each player's latest ping to the Game Master. Static so pings received
	// while the editor was open survive it being closed and reopened.
	protected static ref map<int, int> s_mPingTicks = new map<int, int>();

	protected Widget m_wRoot;
	protected VerticalLayoutWidget m_wRows;
	protected TextWidget m_wCountText;
	protected TextWidget m_wTabCount;
	protected TextWidget m_wTabArrow;
	protected Widget m_wTabAccent;

	protected SCR_PingEditorComponent m_PingManager;

	// Right-click quick actions menu (CRF_ZeusPlayerActions.layout), one at a time
	protected static const ResourceName ACTIONS_LAYOUT = "{6A6512AB7E3C9D01}UI/layouts/Editor/CRF_ZeusPlayerActions.layout";
	protected static const ResourceName ACTION_ROW_LAYOUT = "{6A6512AB7E3C9D02}UI/layouts/Editor/CRF_ZeusPlayerActionRow.layout";
	protected static const float ACTIONS_MENU_WIDTH = 240;
	protected static const float ACTIONS_MENU_HEADER = 36;
	protected static const float ACTIONS_ROW_HEIGHT = 36;
	protected static const float ACTIONS_MENU_PADDING = 6;
	protected Widget m_wActionsMenu;
	protected int m_iActionsPlayerId;

	protected string m_sSignature;
	protected float m_fRefreshTimer;
	protected float m_fTicketPollTimer;
	protected bool m_bDrawerOpen;

	//------------------------------------------------------------------------------------------------
	//! Server reply to COA_PlayerRplToAuthorityManager.RequestZeusTicketHolders
	static void SetTicketHolders(array<int> playerIds)
	{
		s_aTicketHolders.Clear();
		if (playerIds)
			s_aTicketHolders.Copy(playerIds);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] parent the editor menu's root widget
	void CRF_ZeusPlayerList(Widget parent)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !parent)
			return;

		m_wRoot = workspace.CreateWidgets(PANEL_LAYOUT, parent);
		if (!m_wRoot)
		{
			Print("[CRF_ZeusPlayerList] Failed to create " + PANEL_LAYOUT, LogLevel.WARNING);
			return;
		}

		// Far left, vertically centred - clear of the editor's top and bottom toolbars
		FrameSlot.SetAnchorMin(m_wRoot, 0, 0.5);
		FrameSlot.SetAnchorMax(m_wRoot, 0, 0.5);
		FrameSlot.SetAlignment(m_wRoot, 0, 0.5);
		FrameSlot.SetSize(m_wRoot, PANEL_WIDTH, PANEL_HEIGHT);
		m_wRoot.SetZOrder(100);

		m_wRows = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("PlayerRows"));
		m_wCountText = TextWidget.Cast(m_wRoot.FindAnyWidget("CountText"));
		m_wTabCount = TextWidget.Cast(m_wRoot.FindAnyWidget("TabCount"));
		m_wTabArrow = TextWidget.Cast(m_wRoot.FindAnyWidget("TabArrow"));
		m_wTabAccent = m_wRoot.FindAnyWidget("TabAccent");

		// Pings reach the Game Master's machine as the "send" event (vanilla CallEvents is called with
		// isReceiver false on the receiving owner); listen to both in case that changes
		m_PingManager = SCR_PingEditorComponent.Cast(SCR_PingEditorComponent.GetInstance(SCR_PingEditorComponent));
		if (m_PingManager)
		{
			m_PingManager.GetOnPingSend().Insert(OnPing);
			m_PingManager.GetOnPingReceive().Insert(OnPing);
		}

		CloseDrawerInstantly();
		RequestTicketHolders();
		Refresh(true);
	}

	//------------------------------------------------------------------------------------------------
	void Update(float tDelta)
	{
		if (!m_wRoot)
			return;

		bool show = ShouldShow();
		if (m_wRoot.IsVisible() != show)
		{
			m_wRoot.SetVisible(show);
			if (show)
			{
				CloseDrawerInstantly();
				RequestTicketHolders();
				Refresh(true);
			}
			else
			{
				CloseActionsMenu();
			}
		}

		if (!show)
			return;

		// The menu's player left or the list rebuilt under it
		if (m_wActionsMenu && !GetGame().GetPlayerManager().IsPlayerConnected(m_iActionsPlayerId))
			CloseActionsMenu();

		AnimateDrawer(tDelta);

		m_fTicketPollTimer += tDelta;
		if (m_fTicketPollTimer >= TICKET_POLL_INTERVAL)
			RequestTicketHolders();

		m_fRefreshTimer += tDelta;
		if (m_fRefreshTimer >= REFRESH_INTERVAL)
			Refresh(false);
	}

	//------------------------------------------------------------------------------------------------
	//! The reply arrives in SetTicketHolders and shows on the next refresh
	protected void RequestTicketHolders()
	{
		m_fTicketPollTimer = 0;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestZeusTicketHolders();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPing(int reporterID, bool reporterInEditor, bool unlimitedOnly, vector position, SCR_EditableEntityComponent target)
	{
		// Only players calling for the Game Master - not our own pings or other Game Masters'
		if (reporterID <= 0 || reporterInEditor || reporterID == SCR_PlayerController.GetLocalPlayerId())
			return;

		s_mPingTicks.Set(reporterID, System.GetTickCount());
		Refresh(false);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasRecentPing(int playerId)
	{
		int pingTick;
		if (!s_mPingTicks.Find(playerId, pingTick))
			return false;

		if (System.GetTickCount() - pingTick <= PING_HIGHLIGHT_MS)
			return true;

		s_mPingTicks.Remove(playerId);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseDrawerInstantly()
	{
		m_bDrawerOpen = false;
		FrameSlot.SetPos(m_wRoot, DRAWER_CLOSED_X, 0);
		UpdateTabArrow();
	}

	//------------------------------------------------------------------------------------------------
	//! Slide the list out while the cursor is over it (see IsDrawerHovered)
	protected void AnimateDrawer(float tDelta)
	{
		float currentX = FrameSlot.GetPosX(m_wRoot);

		bool open = IsDrawerHovered(DRAWER_OPEN_X - currentX);
		if (open != m_bDrawerOpen)
		{
			m_bDrawerOpen = open;
			UpdateTabArrow();
		}

		float targetX = DRAWER_CLOSED_X;
		if (m_bDrawerOpen)
			targetX = DRAWER_OPEN_X;

		if (currentX != targetX)
			FrameSlot.SetPosX(m_wRoot, EaseTowards(currentX, targetX, tDelta));
	}

	//------------------------------------------------------------------------------------------------
	//! Hover test by rectangle rather than the widget under the cursor (which changes as rows slide
	//! past, making the drawer flicker), with hysteresis: closed, it opens when the cursor touches the
	//! part on screen (the tab); open, it stays open while the cursor is anywhere the fully-open
	//! drawer covers - even mid-slide. Same approach as COA_AARMenu.IsDrawerHovered.
	//! \param[in] toOpenX layout-unit distance from the drawer's current to its open position
	protected bool IsDrawerHovered(float toOpenX)
	{
		int mouseX, mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);

		float left, top, width, height;
		m_wRoot.GetScreenPos(left, top);
		m_wRoot.GetScreenSize(width, height);
		float right = left + width;
		float bottom = top + height;

		if (m_bDrawerOpen)
		{
			float shiftX = GetGame().GetWorkspace().DPIScale(toOpenX);
			left += shiftX;
			right += shiftX;
		}
		else
		{
			left = Math.Max(left, 0);
		}

		return mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;
	}

	//------------------------------------------------------------------------------------------------
	//! Ease-out step towards target; snaps once close enough not to matter
	protected float EaseTowards(float current, float target, float tDelta)
	{
		float next = current + (target - current) * (1 - Math.Pow(2.71828, -DRAWER_SLIDE_SPEED * tDelta));
		if (Math.AbsFloat(target - next) < 0.5)
			return target;

		return next;
	}

	//------------------------------------------------------------------------------------------------
	protected void UpdateTabArrow()
	{
		if (!m_wTabArrow)
			return;

		if (m_bDrawerOpen)
			m_wTabArrow.SetText("‹");
		else
			m_wTabArrow.SetText("›");
	}

	//------------------------------------------------------------------------------------------------
	void Destroy()
	{
		if (m_PingManager)
		{
			m_PingManager.GetOnPingSend().Remove(OnPing);
			m_PingManager.GetOnPingReceive().Remove(OnPing);
			m_PingManager = null;
		}

		CloseActionsMenu();

		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		m_wRows = null;
		m_wCountText = null;
		m_wTabCount = null;
		m_wTabArrow = null;
		m_wTabAccent = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Only for full Game Master (EDIT) and admin mode, and not while the editor UI is hidden
	protected bool ShouldShow()
	{
		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (!editorManager)
			return false;

		EEditorMode mode = editorManager.GetCurrentMode();
		if (mode != EEditorMode.EDIT && mode != EEditorMode.ADMIN)
			return false;

		SCR_MenuEditorComponent menuManager = SCR_MenuEditorComponent.Cast(SCR_MenuEditorComponent.GetInstance(SCR_MenuEditorComponent));
		if (menuManager && !menuManager.IsVisible())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuild the rows when anything shown has changed (or always, when forced)
	protected void Refresh(bool force)
	{
		m_fRefreshTimer = 0;
		if (!m_wRows)
			return;

		array<ref CRF_ZeusFactionGroup> groups = {};
		int playerCount = CollectPlayers(groups);

		string signature = BuildSignature(groups);
		if (!force && signature == m_sSignature)
			return;

		m_sSignature = signature;

		if (m_wCountText)
			m_wCountText.SetText(playerCount.ToString());

		if (m_wTabCount)
			m_wTabCount.SetText(playerCount.ToString());

		UpdateTabAttention(groups);
		RebuildRows(groups);

		int rowCount;
		Widget row = m_wRows.GetChildren();
		while (row)
		{
			rowCount++;
			row = row.GetSibling();
		}

		PrintFormat("[CRF_ZeusPlayerList] Rebuilt: %1 players, %2 faction groups, %3 rows", playerCount, groups.Count(), rowCount, level: LogLevel.DEBUG);
	}

	//------------------------------------------------------------------------------------------------
	//! The drawer is closed by default, so its tab turns red while anyone needs attention
	protected void UpdateTabAttention(notnull array<ref CRF_ZeusFactionGroup> groups)
	{
		bool anyAttention;
		foreach (CRF_ZeusFactionGroup group : groups)
		{
			foreach (CRF_ZeusPlayerEntry entry : group.m_aPlayers)
			{
				if (entry.m_bNeedsAttention)
					anyAttention = true;
			}
		}

		if (m_wTabAccent)
		{
			if (anyAttention)
				m_wTabAccent.SetColor(ATTENTION_COLOR);
			else
				m_wTabAccent.SetColor(ACCENT_COLOR);
		}

		if (m_wTabCount)
		{
			if (anyAttention)
				m_wTabCount.SetColor(ATTENTION_COLOR);
			else
				m_wTabCount.SetColor(TAB_COUNT_COLOR);
		}

		// The accent is red too, so the tab's background is what changes for attention
		Widget tabBG = m_wRoot.FindAnyWidget("TabBG");
		if (tabBG)
		{
			if (anyAttention)
				tabBG.SetColor(TAB_ATTENTION_BG);
			else
				tabBG.SetColor(TAB_BG);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return number of players collected
	protected int CollectPlayers(notnull array<ref CRF_ZeusFactionGroup> groups)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();

		foreach (int playerId : playerIds)
		{
			CRF_ZeusPlayerEntry entry = new CRF_ZeusPlayerEntry();
			entry.m_iPlayerId = playerId;
			entry.m_sName = playerManager.GetPlayerName(playerId);
			entry.m_sSortKey = entry.m_sName;
			entry.m_sSortKey.ToLower();

			if (playerId == localPlayerId)
				entry.m_sName += " (you)";

			IEntity entity = playerManager.GetPlayerControlledEntity(playerId);
			if (entity && COA_EntityHelper.IsSpectator(entity))
			{
				entry.m_sStatus = "Spectating";
				entry.m_bInactive = true;
			}
			else
			{
				ChimeraCharacter character = ChimeraCharacter.Cast(entity);
				if (character && character.GetCharacterController() && character.GetCharacterController().IsDead())
				{
					entry.m_sStatus = "Dead";
					entry.m_bInactive = true;
				}
			}

			// Calling for help takes precedence over dead/spectating in the status column
			if (s_aTicketHolders.Contains(playerId))
			{
				entry.m_sStatus = "Ticket";
				entry.m_bNeedsAttention = true;
			}
			else if (HasRecentPing(playerId))
			{
				entry.m_sStatus = "Pinged";
				entry.m_bNeedsAttention = true;
			}

			CRF_ZeusFactionGroup group = FindOrAddGroup(groups, GetPlayerFaction(playerId));
			InsertSorted(group.m_aPlayers, entry);
		}

		return playerIds.Count();
	}

	//------------------------------------------------------------------------------------------------
	protected Faction GetPlayerFaction(int playerId)
	{
		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (slottingManager)
			return slottingManager.GetPlayerSlotFaction(playerId, true);

		return SCR_FactionManager.SGetPlayerFaction(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Groups are kept sorted by faction name, with unslotted players last
	protected CRF_ZeusFactionGroup FindOrAddGroup(notnull array<ref CRF_ZeusFactionGroup> groups, Faction faction)
	{
		foreach (CRF_ZeusFactionGroup existing : groups)
		{
			if (existing.m_Faction == faction)
				return existing;
		}

		CRF_ZeusFactionGroup group = new CRF_ZeusFactionGroup();
		group.m_Faction = faction;
		if (faction)
		{
			group.m_sSortKey = faction.GetFactionName();
			group.m_sSortKey.ToLower();
		}
		else
		{
			group.m_sSortKey = "~"; // after every letter
		}

		int index;
		while (index < groups.Count() && groups[index].m_sSortKey.Compare(group.m_sSortKey) <= 0)
		{
			index++;
		}

		groups.InsertAt(group, index);
		return group;
	}

	//------------------------------------------------------------------------------------------------
	protected void InsertSorted(notnull array<ref CRF_ZeusPlayerEntry> players, CRF_ZeusPlayerEntry entry)
	{
		int index;
		while (index < players.Count() && players[index].m_sSortKey.Compare(entry.m_sSortKey) <= 0)
		{
			index++;
		}

		players.InsertAt(entry, index);
	}

	//------------------------------------------------------------------------------------------------
	protected string BuildSignature(notnull array<ref CRF_ZeusFactionGroup> groups)
	{
		string signature;
		foreach (CRF_ZeusFactionGroup group : groups)
		{
			if (group.m_Faction)
				signature += group.m_Faction.GetFactionKey();

			signature += "{";
			foreach (CRF_ZeusPlayerEntry entry : group.m_aPlayers)
			{
				signature += entry.m_iPlayerId.ToString() + ":" + entry.m_sName + ":" + entry.m_sStatus + ":" + entry.m_bNeedsAttention.ToString() + ";";
			}

			signature += "}";
		}

		return signature;
	}

	//------------------------------------------------------------------------------------------------
	protected void RebuildRows(notnull array<ref CRF_ZeusFactionGroup> groups)
	{
		while (m_wRows.GetChildren())
		{
			m_wRows.GetChildren().RemoveFromHierarchy();
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		foreach (CRF_ZeusFactionGroup group : groups)
		{
			AddHeaderRow(workspace, group);

			foreach (CRF_ZeusPlayerEntry entry : group.m_aPlayers)
			{
				AddPlayerRow(workspace, group, entry);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The row layout's root is the row's button, sized by the SizeLayout inside it
	protected Widget CreateRow(WorkspaceWidget workspace)
	{
		Widget row = workspace.CreateWidgets(ROW_LAYOUT, m_wRows);
		if (!row)
		{
			Print("[CRF_ZeusPlayerList] Failed to create " + ROW_LAYOUT, LogLevel.WARNING);
			return null;
		}

		AlignableSlot.SetHorizontalAlign(row, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetPadding(row, 0, 1, 0, 1);
		return row;
	}

	//------------------------------------------------------------------------------------------------
	protected void AddHeaderRow(WorkspaceWidget workspace, CRF_ZeusFactionGroup group)
	{
		Widget row = CreateRow(workspace);
		if (!row)
			return;

		Color factionColor = GetGroupColor(group);

		// Headers keep m_iPlayerId 0, so double-clicking them does nothing, and get no hover effect
		row.SetColor(HEADER_ROW_COLOR);

		Widget stripe = row.FindAnyWidget("FactionStripe");
		if (stripe)
			stripe.SetColor(factionColor);

		TextWidget nameText = TextWidget.Cast(row.FindAnyWidget("NameText"));
		if (nameText)
		{
			if (group.m_Faction)
				nameText.SetText(group.m_Faction.GetFactionName());
			else
				nameText.SetText("Unslotted");

			nameText.SetColor(LightenForText(factionColor));
			nameText.SetFlags(WidgetFlags.IGNORE_CURSOR);
		}

		TextWidget statusText = TextWidget.Cast(row.FindAnyWidget("StatusText"));
		if (statusText)
		{
			statusText.SetText(group.m_aPlayers.Count().ToString());
			statusText.SetFlags(WidgetFlags.IGNORE_CURSOR);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void AddPlayerRow(WorkspaceWidget workspace, CRF_ZeusFactionGroup group, CRF_ZeusPlayerEntry entry)
	{
		Widget row = CreateRow(workspace);
		if (!row)
			return;

		Widget stripe = row.FindAnyWidget("FactionStripe");
		if (stripe)
		{
			Color stripeColor = GetGroupColor(group);
			stripeColor.SetA(0.5);
			stripe.SetColor(stripeColor);
			stripe.SetFlags(WidgetFlags.IGNORE_CURSOR);
		}

		TextWidget nameText = TextWidget.Cast(row.FindAnyWidget("NameText"));
		if (nameText)
		{
			nameText.SetText(entry.m_sName);
			if (entry.m_bNeedsAttention)
				nameText.SetColor(ATTENTION_COLOR);
			else if (entry.m_bInactive)
				nameText.SetColor(INACTIVE_NAME_COLOR);
			else
				nameText.SetColor(NAME_COLOR);

			nameText.SetFlags(WidgetFlags.IGNORE_CURSOR);
		}

		TextWidget statusText = TextWidget.Cast(row.FindAnyWidget("StatusText"));
		if (statusText)
		{
			statusText.SetText(entry.m_sStatus);
			if (entry.m_bNeedsAttention)
				statusText.SetColor(ATTENTION_COLOR);

			statusText.SetFlags(WidgetFlags.IGNORE_CURSOR);
		}

		CRF_ZeusPlayerRowButton rowButton = CRF_ZeusPlayerRowButton.Cast(row.FindHandler(CRF_ZeusPlayerRowButton));
		if (rowButton)
		{
			rowButton.m_iPlayerId = entry.m_iPlayerId;
			rowButton.m_OnDoubleClicked.Insert(FocusCameraOnPlayer);
			rowButton.m_OnContextMenu.Insert(OpenActionsMenu);
		}
		else
		{
			Print("[CRF_ZeusPlayerList] Row has no CRF_ZeusPlayerRowButton - double-click won't work", LogLevel.WARNING);
		}

		COA_UIHoverEffect.AttachAuto(row);
	}

	//------------------------------------------------------------------------------------------------
	//! Right-click menu for a player row, at the cursor
	protected void OpenActionsMenu(int playerId, int screenX, int screenY)
	{
		CloseActionsMenu();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		Widget parent = m_wRoot.GetParent();
		if (!workspace || !parent)
			return;

		m_wActionsMenu = workspace.CreateWidgets(ACTIONS_LAYOUT, parent);
		if (!m_wActionsMenu)
		{
			Print("[CRF_ZeusPlayerList] Failed to create " + ACTIONS_LAYOUT, LogLevel.WARNING);
			return;
		}

		m_iActionsPlayerId = playerId;
		m_wActionsMenu.SetZOrder(300);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		TextWidget title = TextWidget.Cast(m_wActionsMenu.FindAnyWidget("MenuTitle"));
		if (title)
		{
			string name = playerManager.GetPlayerName(playerId);
			name.ToUpper();
			title.SetText(name);
		}

		CRF_ZeusActionButton backdrop = CRF_ZeusActionButton.Cast(m_wActionsMenu.FindAnyWidget("Backdrop").FindHandler(CRF_ZeusActionButton));
		if (backdrop)
			backdrop.m_OnClicked.Insert(OnActionClicked);

		// Which actions apply to this player right now
		IEntity target = playerManager.GetPlayerControlledEntity(playerId);
		bool hasBody = target && !COA_EntityHelper.IsSpectator(target);
		bool isStaff = SCR_Global.IsAdmin() || (COA_PermissionManager.GetInstance() && COA_PermissionManager.GetInstance().IsModerator());

		int rows;
		rows += AddAction("Move camera to player", CRF_EZeusPlayerAction.MOVE_CAMERA);
		if (hasBody)
		{
			rows += AddAction("Teleport player to camera", CRF_EZeusPlayerAction.TELEPORT_TO_CAMERA);
			rows += AddAction("Heal", CRF_EZeusPlayerAction.HEAL);
			if (!SCR_Global.IsAdmin(playerId))
				rows += AddAction("Send to spectator", CRF_EZeusPlayerAction.SEND_TO_SPECTATOR);
		}
		if (isStaff && s_aTicketHolders.Contains(playerId))
			rows += AddAction("Open ticket", CRF_EZeusPlayerAction.OPEN_TICKET);

		// Size to the rows and keep the menu on screen
		Widget menu = m_wActionsMenu.FindAnyWidget("Menu");
		if (!menu)
			return;

		float height = ACTIONS_MENU_HEADER + rows * ACTIONS_ROW_HEIGHT + ACTIONS_MENU_PADDING;
		float x = workspace.DPIUnscale(screenX);
		float y = workspace.DPIUnscale(screenY);

		float screenWidth, screenHeight;
		parent.GetScreenSize(screenWidth, screenHeight);
		x = Math.Clamp(x, 0, Math.Max(0, workspace.DPIUnscale(screenWidth) - ACTIONS_MENU_WIDTH));
		y = Math.Clamp(y, 0, Math.Max(0, workspace.DPIUnscale(screenHeight) - height));

		FrameSlot.SetSize(menu, ACTIONS_MENU_WIDTH, height);
		FrameSlot.SetPos(menu, x, y);
	}

	//------------------------------------------------------------------------------------------------
	//! \return 1 if the row was added (for sizing), else 0
	protected int AddAction(string label, CRF_EZeusPlayerAction action)
	{
		Widget list = m_wActionsMenu.FindAnyWidget("Actions");
		if (!list)
			return 0;

		Widget row = GetGame().GetWorkspace().CreateWidgets(ACTION_ROW_LAYOUT, list);
		if (!row)
			return 0;

		TextWidget text = TextWidget.Cast(row.FindAnyWidget("ActionText"));
		if (text)
			text.SetText(label);

		CRF_ZeusActionButton button = CRF_ZeusActionButton.Cast(row.FindHandler(CRF_ZeusActionButton));
		if (button)
		{
			button.m_iAction = action;
			button.m_OnClicked.Insert(OnActionClicked);
		}

		// ActionBG starts fully transparent, so brighten by overlaying rather than lightening it
		COA_UIHoverEffect.Attach(row, row.FindAnyWidget("ActionBG"), COA_EHoverStyle.OVERLAY);
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnActionClicked(int action)
	{
		int playerId = m_iActionsPlayerId;
		CloseActionsMenu();

		if (playerId <= 0)
			return;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();

		switch (action)
		{
			case CRF_EZeusPlayerAction.MOVE_CAMERA:
			{
				FocusCameraOnPlayer(playerId);
				break;
			}
			case CRF_EZeusPlayerAction.TELEPORT_TO_CAMERA:
			{
				vector cameraPosition;
				if (authorityManager && GetEditorCameraPosition(cameraPosition))
					authorityManager.RequestZeusTeleportPlayer(playerId, cameraPosition);
				break;
			}
			case CRF_EZeusPlayerAction.HEAL:
			{
				if (authorityManager)
					authorityManager.RequestZeusHeal(playerId);
				break;
			}
			case CRF_EZeusPlayerAction.SEND_TO_SPECTATOR:
			{
				if (authorityManager)
					authorityManager.RequestZeusSendToSpectator(playerId);
				break;
			}
			case CRF_EZeusPlayerAction.OPEN_TICKET:
			{
				COA_AdminMenu.OpenTicketFor(playerId);
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CloseActionsMenu()
	{
		if (m_wActionsMenu)
			m_wActionsMenu.RemoveFromHierarchy();

		m_wActionsMenu = null;
		m_iActionsPlayerId = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Where the Game Master's camera is, for "teleport player to camera"
	protected bool GetEditorCameraPosition(out vector position)
	{
		SCR_CameraEditorComponent cameraManager = SCR_CameraEditorComponent.Cast(SCR_CameraEditorComponent.GetInstance(SCR_CameraEditorComponent));
		if (!cameraManager)
			return false;

		SCR_ManualCamera camera = cameraManager.GetCamera();
		if (!camera)
			return false;

		position = camera.GetOrigin();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected Color GetGroupColor(CRF_ZeusFactionGroup group)
	{
		if (group.m_Faction && group.m_Faction.GetFactionColor())
			return Color.FromInt(group.m_Faction.GetFactionColor().PackToInt());

		return Color.FromInt(UNSLOTTED_COLOR.PackToInt());
	}

	//------------------------------------------------------------------------------------------------
	//! Faction colours can be too dark to read on the slate panel - pull them towards white. Colours are
	//! linear, where a small step already lightens a lot, so 0.2 keeps the faction hue recognisable.
	protected Color LightenForText(Color color)
	{
		Color text = Color.FromInt(color.PackToInt());
		text.Lerp(Color.White, 0.2);
		text.SetA(1);
		return text;
	}

	//------------------------------------------------------------------------------------------------
	//! Move the local editor camera to the player. Players this client has not streamed in are
	//! looked up by the server, which moves the camera through the editor's own RPC.
	protected void FocusCameraOnPlayer(int playerId)
	{
		// Going to a player who pinged counts as answering it (open tickets stay red until closed)
		// Refresh on the next update, not here - rebuilding would delete the row handling this click
		if (s_mPingTicks.Contains(playerId))
		{
			s_mPingTicks.Remove(playerId);
			m_fRefreshTimer = REFRESH_INTERVAL;
		}

		IEntity target = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (target && TeleportLocalCamera(target.GetOrigin()))
			return;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestZeusCameraToPlayer(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected bool TeleportLocalCamera(vector position)
	{
		SCR_CameraEditorComponent cameraManager = SCR_CameraEditorComponent.Cast(SCR_CameraEditorComponent.GetInstance(SCR_CameraEditorComponent));
		if (!cameraManager)
			return false;

		SCR_ManualCamera camera = cameraManager.GetCamera();
		if (!camera)
			return false;

		SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent));
		if (!teleportComponent)
			return false;

		// Same call the editor uses for its own "move camera to" actions
		return teleportComponent.TeleportCamera(position, true, false);
	}
}
