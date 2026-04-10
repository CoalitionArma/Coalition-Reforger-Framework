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
	IEntity         m_Entity;
	float           m_fDistance;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker();
	ref ScriptInvoker m_OnHovered = new ScriptInvoker();

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(this);
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_OnHovered.Invoke(this);
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
	protected ItemPreviewWidget                m_wSidePreview;
	protected TextWidget                       m_wPreviewName;
	protected ItemPreviewManagerEntity         m_PreviewMgr;
	protected IEntity                          m_PreviewEntity; // local-only entity used for the side preview

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

		m_wPropList    = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("PropList"));
		m_wSidePreview = ItemPreviewWidget.Cast(m_wRoot.FindAnyWidget("PropPreview"));
		m_wPreviewName = TextWidget.Cast(m_wRoot.FindAnyWidget("PreviewNameText"));

		ChimeraWorld chWorld = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (chWorld)
			m_PreviewMgr = chWorld.GetItemPreviewManager();

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

		// Delete the local-only preview entity so it doesn't linger in the world.
		if (m_PreviewEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);
			m_PreviewEntity = null;
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

			ButtonWidget btn = ButtonWidget.Cast(entryWidget.FindAnyWidget("EntryButton"));
			if (!btn)
				continue;

			CRF_PropHuntEntryButton comp = CRF_PropHuntEntryButton.Cast(btn.FindHandler(CRF_PropHuntEntryButton));
			if (!comp)
				continue;

			comp.m_sPrefab   = se.m_sPrefab;
			comp.m_Entity    = se.m_Entity;
			comp.m_fDistance = se.m_fDist;
			comp.m_OnClicked.Insert(OnEntryClicked);
			comp.m_OnHovered.Insert(OnEntryHovered);
		}
	}

	//------------------------------------------------------------
	// Called when a list entry is hovered — updates the side preview
	//------------------------------------------------------------
	protected void OnEntryHovered(CRF_PropHuntEntryButton btn)
	{
		if (!btn || !btn.m_sPrefab)
			return;

		// Delete any previously-spawned local preview entity.
		if (m_PreviewEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);
			m_PreviewEntity = null;
		}

		// Spawn the prefab locally (no replication) so the preview manager
		// can render its actual model. This is the same approach used by
		// SCR_FieldManualUI for weapon previews.
		if (m_wSidePreview && m_PreviewMgr)
		{
			Resource res = Resource.Load(btn.m_sPrefab);
			if (res.IsValid())
			{
				m_PreviewEntity = GetGame().SpawnEntityPrefabLocal(res, null, null);
				if (m_PreviewEntity)
					m_PreviewMgr.SetPreviewItem(m_wSidePreview, m_PreviewEntity);
			}
		}

		if (m_wPreviewName)
			m_wPreviewName.SetText(ExtractDisplayName(btn.m_sPrefab));
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
		// 1. Try the engine's designer-authored localized name via SCR_EditableEntityComponent UIInfo.
		//    Most world props (benches, fences, barrels, etc.) have this component set in the editor.
		Resource res = Resource.Load(prefab);
		if (res.IsValid())
		{
			IEntityComponentSource editableSource = SCR_EditableEntityComponentClass.GetEditableEntitySource(res);
			if (editableSource)
			{
				SCR_EditableEntityUIInfo info = SCR_EditableEntityComponentClass.GetInfo(editableSource);
				if (info)
				{
					string localizedName = info.GetName();
					// Reject the engine fallback placeholder string.
					if (!localizedName.IsEmpty() && localizedName != "#AR-AttributesDialog_TitlePage_Entity_Text")
						return localizedName;
				}
			}
		}

		// 2. Fall back to filename-based formatting for props without SCR_EditableEntityComponent.
		string s = prefab;

		// Strip GUID prefix: "{ABCD1234...}path/to/file.et" → "path/to/file.et"
		if (s.StartsWith("{"))
		{
			int end = s.IndexOf("}");
			if (end >= 0)
				s = s.Substring(end + 1, s.Length() - end - 1);
		}

		// Keep only the filename
		int lastSlash = s.LastIndexOf("/");
		if (lastSlash >= 0)
			s = s.Substring(lastSlash + 1, s.Length() - lastSlash - 1);

		// Strip file extension
		int dot = s.LastIndexOf(".");
		if (dot > 0)
			s = s.Substring(0, dot);

		// Strip common low-information prefixes that add no display value.
		string lower = s;
		lower.ToLower();
		array<string> skipPrefixes = {"prop_", "bc_", "us_", "ussr_", "ge_", "fia_", "civ_",
		                              "ins_", "indfor_", "blufor_", "opfor_"};
		foreach (string pfx : skipPrefixes)
		{
			if (lower.StartsWith(pfx))
			{
				s = s.Substring(pfx.Length(), s.Length() - pfx.Length());
				break;
			}
		}

		// Split on underscore, capitalise each token, rejoin with spaces.
		array<string> parts = {};
		s.Split("_", parts, false);

		string result;
		foreach (int i, string part : parts)
		{
			if (part.Length() == 0)
				continue;

			string firstChar = part.Substring(0, 1);
			firstChar.ToUpper();
			string rest = "";
			if (part.Length() > 1)
				rest = part.Substring(1, part.Length() - 1);

			if (i > 0)
				result += " ";
			result += firstChar + rest;
		}

		return result;
	}
}

