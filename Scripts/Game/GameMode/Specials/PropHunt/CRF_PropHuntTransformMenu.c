// CRF_PropHuntTransformMenu.c
//
// Selection UI shown to Prop players during the grace phase so they can choose
// which nearby world object to disguise themselves as.
//
// Uses workspace.CreateWidgets() — the same mechanism as the blackout overlay
// and hunter health bar — so it works in both Workbench and multiplayer.
// ChimeraMenuBase is intentionally NOT used; the menu manager's rendering
// is unreliable in Workbench preview sessions.
//
// Flow:
//   1. CRF_PlayerRplToOwnerManager (modded) calls CRF_PropHuntTransformMenu.Open()
//      when the player presses F during the grace phase.
//   2. Open() creates the widget tree via workspace.CreateWidgets, populates
//      the entry list, and disables character movement so the player can't
//      ghost around while choosing.
//   3. Clicking an entry calls back to the local
//      CRF_PlayerRplToOwnerManager.ConfirmPropTransform(prefab) which fires
//      the server-side transform RPC, then calls Close().
//   4. Esc / Back closes the menu without transforming. Movement is restored.

//--------------------------------------------------------------
// Per-entry button script component.
// Stores the prefab ResourceName for the entry it represents.
//--------------------------------------------------------------
class CRF_PropHuntEntryButton : ScriptedWidgetComponent
{
	ResourceName    m_sPrefab;
	float           m_fDistance;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(this);
		return false;
	}
}

//--------------------------------------------------------------
// Lightweight sort helper — holds one candidate entity with its
// display metadata and implements Enforce's Comparable interface
// so array.Sort() works without an explicit comparator.
//--------------------------------------------------------------
class CRF_PropHuntSortEntry
{
	IEntity      m_Entity;
	ResourceName m_sPrefab;
	string       m_sDisplay;
	float        m_fDist;

	int Compare(CRF_PropHuntSortEntry other)
	{
		if (m_fDist < other.m_fDist) return -1;
		if (m_fDist > other.m_fDist) return 1;
		return 0;
	}
}

//--------------------------------------------------------------
// Script component attached to the Cancel button in the menu
// layout. Fires Close() on the owning menu when clicked.
//--------------------------------------------------------------
class CRF_PropHuntCancelButton : ScriptedWidgetComponent
{
	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke();
		return false;
	}
}

//--------------------------------------------------------------
// Transform selection UI.
// Plain class — NOT a ChimeraMenuBase — uses workspace.CreateWidgets()
// so it renders correctly in Workbench and multiplayer.
//--------------------------------------------------------------
class CRF_PropHuntTransformMenu
{
	protected static const ResourceName MENU_LAYOUT  = "{AF1B0030C3D4E500}UI/layouts/Menus/PropHunt/CRF_PropHuntTransformMenu.layout";
	protected static const ResourceName ENTRY_LAYOUT = "{AF1B0031C3D4E500}UI/layouts/Menus/PropHunt/CRF_PropHuntTransformEntry.layout";

	protected static ref CRF_PropHuntTransformMenu m_sInstance;

	protected Widget                           m_wRoot;
	protected VerticalLayoutWidget             m_wPropList;
	protected SCR_CharacterControllerComponent m_CharCtrl;

	//------------------------------------------------------------
	// Singleton accessor
	//------------------------------------------------------------
	static CRF_PropHuntTransformMenu GetInstance()
	{
		return m_sInstance;
	}

	//! True while the selection widget is shown on screen.
	static bool IsOpen()
	{
		return m_sInstance != null;
	}

	//------------------------------------------------------------
	// Open — create and show the selection UI.
	// Replaces any previously open instance.
	//------------------------------------------------------------
	static void Open(array<IEntity> nearbyEntities)
	{
		if (m_sInstance)
			m_sInstance.Close();

		m_sInstance = new CRF_PropHuntTransformMenu();
		m_sInstance.Create(nearbyEntities);
	}

	//------------------------------------------------------------
	// Create — builds the widget tree, freezes the character,
	// and populates the list.
	//------------------------------------------------------------
	protected void Create(array<IEntity> nearbyEntities)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			m_sInstance = null;
			return;
		}

		m_wRoot = workspace.CreateWidgets(MENU_LAYOUT);
		if (!m_wRoot)
		{
			m_sInstance = null;
			return;
		}

		m_wPropList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("PropList"));

		// Wire up the Cancel button so a mouse click also calls Close().
		ButtonWidget cancelBtn = ButtonWidget.Cast(m_wRoot.FindAnyWidget("CancelButton"));
		if (cancelBtn)
		{
			CRF_PropHuntCancelButton cancelComp = CRF_PropHuntCancelButton.Cast(cancelBtn.FindHandler(CRF_PropHuntCancelButton));
			if (cancelComp)
				cancelComp.m_OnClicked.Insert(Close);
		}

		// Disable character movement & weapons while choosing, so the player
		// cannot ghost around or accidentally fire.
		IEntity localChar = SCR_PlayerController.GetLocalControlledEntity();
		if (localChar)
		{
			m_CharCtrl = SCR_CharacterControllerComponent.Cast(
				localChar.FindComponent(SCR_CharacterControllerComponent)
			);
			if (m_CharCtrl)
			{
				m_CharCtrl.SetDisableMovementControls(true);
				m_CharCtrl.SetDisableWeaponControls(true);
			}
		}

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, ActionCancel);

		PopulateList(nearbyEntities);

		// Activate input contexts every frame (delay 0 = per-frame) so the cursor is
		// visible and button-click events fire on every tick, matching the cadence of
		// DialogUI.OnMenuUpdate which also activates these contexts every frame.
		GetGame().GetCallqueue().CallLater(KeepCursorActive, 0, true);
	}

	//------------------------------------------------------------
	// Close — removes the widget and restores character controls.
	//------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(KeepCursorActive);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, ActionCancel);

		if (m_CharCtrl)
		{
			m_CharCtrl.SetDisableMovementControls(false);
			m_CharCtrl.SetDisableWeaponControls(false);
			m_CharCtrl = null;
		}

		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}

		m_sInstance = null;
	}

	//------------------------------------------------------------
	// KeepCursorActive — called every ~16 ms by CallLater while
	// the menu is open. Activates the interactive dialog input
	// context so the cursor is visible and button clicks fire.
	// Mirrors what DialogUI.OnMenuUpdate() does in vanilla.
	//------------------------------------------------------------
	protected void KeepCursorActive()
	{
		if (!m_wRoot)
		{
			GetGame().GetCallqueue().Remove(KeepCursorActive);
			return;
		}
		// MenuContext suppresses PlayerCameraContext (camera mouse-look) each frame.
		// InteractableDialogContext shows the cursor and enables button OnClick events.
		// Both must be activated every single frame, matching DialogUI.OnMenuUpdate behaviour.
		GetGame().GetInputManager().ActivateContext("MenuContext");
		GetGame().GetInputManager().ActivateContext("InteractableDialogContext");
	}

	//------------------------------------------------------------
	// Build the list entries from the nearby-entity array.
	//------------------------------------------------------------
	protected void PopulateList(array<IEntity> nearbyEntities)
	{
		if (!m_wPropList)
			return;

		IEntity localChar = SCR_PlayerController.GetLocalControlledEntity();
		vector localPos = vector.Zero;
		if (localChar)
			localPos = localChar.GetOrigin();

		array<ref CRF_PropHuntSortEntry> sorted = {};
		foreach (IEntity ent : nearbyEntities)
		{
			if (!ent)
				continue;

			CRF_PropHuntSortEntry se = new CRF_PropHuntSortEntry();
			se.m_Entity   = ent;
			se.m_fDist    = vector.Distance(ent.GetOrigin(), localPos);
			se.m_sPrefab  = ent.GetPrefabData().GetPrefabName();
			se.m_sDisplay = ExtractDisplayName(se.m_sPrefab);
			sorted.Insert(se);
		}
		sorted.Sort();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		foreach (CRF_PropHuntSortEntry se : sorted)
		{
			Widget entryWidget = workspace.CreateWidgets(ENTRY_LAYOUT, m_wPropList);
			if (!entryWidget)
				continue;

			TextWidget labelW = TextWidget.Cast(entryWidget.FindAnyWidget("EntryLabel"));
			if (labelW)
				labelW.SetText(se.m_sDisplay);

			TextWidget distW = TextWidget.Cast(entryWidget.FindAnyWidget("EntryDistance"));
			if (distW)
				distW.SetText(string.Format("%.1f m", se.m_fDist));

			ItemPreviewWidget previewW = ItemPreviewWidget.Cast(entryWidget.FindAnyWidget("PropPreview"));
			if (previewW)
			{
				ChimeraWorld chimeraWorld = ChimeraWorld.CastFrom(GetGame().GetWorld());
				if (chimeraWorld)
				{
					ItemPreviewManagerEntity previewMgr = chimeraWorld.GetItemPreviewManager();
					if (previewMgr)
						previewMgr.SetPreviewItemFromPrefab(previewW, se.m_sPrefab);
				}
			}

			ButtonWidget btn = ButtonWidget.Cast(entryWidget.FindAnyWidget("EntryButton"));
			if (!btn)
				continue;

			CRF_PropHuntEntryButton comp = CRF_PropHuntEntryButton.Cast(btn.FindHandler(CRF_PropHuntEntryButton));
			if (!comp)
				continue;

			comp.m_sPrefab   = se.m_sPrefab;
			comp.m_fDistance = se.m_fDist;
			comp.m_OnClicked.Insert(OnEntryClicked);
		}
	}

	//------------------------------------------------------------
	// Called when a list entry is clicked
	//------------------------------------------------------------
	protected void OnEntryClicked(CRF_PropHuntEntryButton btn)
	{
		if (!btn || !btn.m_sPrefab)
			return;

		CRF_PlayerRplToOwnerManager mgr = CRF_PlayerRplToOwnerManager.GetInstance();
		if (mgr)
			mgr.ConfirmPropTransform(btn.m_sPrefab);

		Close();
	}

	//------------------------------------------------------------
	// ConfirmFirst — keyboard fallback: confirms the first (nearest)
	// entry in the list. Called when the player presses F a second
	// time while the menu is open (Workbench cursor fallback).
	//------------------------------------------------------------
	void ConfirmFirst()
	{
		if (!m_wPropList)
		{
			Close();
			return;
		}

		Widget firstEntry = m_wPropList.GetChildren();
		if (!firstEntry)
		{
			Close();
			return;
		}

		ButtonWidget btn = ButtonWidget.Cast(firstEntry.FindAnyWidget("EntryButton"));
		if (!btn)
		{
			Close();
			return;
		}

		CRF_PropHuntEntryButton comp = CRF_PropHuntEntryButton.Cast(btn.FindHandler(CRF_PropHuntEntryButton));
		if (comp && comp.m_sPrefab)
		{
			CRF_PlayerRplToOwnerManager mgr = CRF_PlayerRplToOwnerManager.GetInstance();
			if (mgr)
				mgr.ConfirmPropTransform(comp.m_sPrefab);
		}

		Close();
	}

	//------------------------------------------------------------
	// Cancel via Esc / Back key
	//------------------------------------------------------------
	protected void ActionCancel(float value, EActionTrigger reason)
	{
		Close();
	}

	//------------------------------------------------------------
	// Derive a human-readable name from a ResourceName.
	// "{GUID}path/to/SomeProp.et"  →  "SomeProp"
	//------------------------------------------------------------
	static string ExtractDisplayName(ResourceName prefab)
	{
		string s = prefab;

		if (s.StartsWith("{"))
		{
			int end = s.IndexOf("}");
			if (end >= 0)
				s = s.Substring(end + 1, s.Length() - end - 1);
		}

		int lastSlash = s.LastIndexOf("/");
		if (lastSlash >= 0)
			s = s.Substring(lastSlash + 1, s.Length() - lastSlash - 1);

		int dot = s.LastIndexOf(".");
		if (dot > 0)
			s = s.Substring(0, dot);

		return s;
	}
}

