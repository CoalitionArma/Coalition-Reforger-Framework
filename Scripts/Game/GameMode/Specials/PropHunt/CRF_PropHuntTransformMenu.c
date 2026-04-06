// CRF_PropHuntTransformMenu.c
//
// Selection UI shown to Prop players during the grace phase so they can choose
// which nearby world object to disguise themselves as.
//
// Flow:
//   1. CRF_PlayerRplToOwnerManager (modded) calls
//      CRF_PropHuntTransformMenu.Open(nearbyEntities) when the player presses F.
//   2. The menu populates a scrollable list with one entry per valid nearby entity,
//      showing a friendly name (derived from the prefab filename) and distance.
//   3. When the player clicks an entry the menu calls back to the local
//      CRF_PlayerRplToOwnerManager.ConfirmPropTransform(prefab) which fires the
//      server-side transform RPC and closes the menu.
//   4. Esc / Cancel button closes the menu without transforming.  The player may
//      press F again to re-open it.

//--------------------------------------------------------------
// Per-entry button script component.
// Stores the prefab ResourceName and distance for the entry
// it belongs to; exposes m_OnClicked so the menu can subscribe.
//--------------------------------------------------------------
class CRF_PropHuntEntryButton : ScriptedWidgetComponent
{
	ResourceName    m_sPrefab;
	float           m_fDistance;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	//------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(this);
		return false;
	}
}

//--------------------------------------------------------------
// Main transform selection menu
//--------------------------------------------------------------
class CRF_PropHuntTransformMenu : ChimeraMenuBase
{
	//------------------------------------------------------------
	// Layout resource paths
	//------------------------------------------------------------
	protected static const ResourceName MENU_LAYOUT  = "{AF1B0030C3D4E500}UI/layouts/Menus/PropHunt/CRF_PropHuntTransformMenu.layout";
	protected static const ResourceName ENTRY_LAYOUT = "{AF1B0031C3D4E500}UI/layouts/Menus/PropHunt/CRF_PropHuntTransformEntry.layout";

	//------------------------------------------------------------
	// Static data handed in before the menu is opened
	//------------------------------------------------------------
	protected static ref array<IEntity> s_aNearbyEntities = {};

	//------------------------------------------------------------
	// Widgets
	//------------------------------------------------------------
	protected Widget                   m_wRoot;
	protected VerticalLayoutWidget     m_wPropList;
	protected ButtonWidget             m_wCancelButton;

	//------------------------------------------------------------
	// Static opener — call this instead of OpenMenu directly so
	// the entity list is set before OnMenuOpen fires.
	//------------------------------------------------------------
	static void Open(array<IEntity> nearbyEntities)
	{
		s_aNearbyEntities.Clear();
		foreach (IEntity ent : nearbyEntities)
			s_aNearbyEntities.Insert(ent);

		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PropHuntTransformMenu);
	}

	//------------------------------------------------------------
	// ChimeraMenuBase lifecycle
	//------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		m_wRoot      = GetRootWidget();
		m_wPropList  = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("PropList"));
		m_wCancelButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("CancelButton"));

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, ActionCancel);

		PopulateList();
	}

	override void OnMenuClose()
	{
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, ActionCancel);
		super.OnMenuClose();
	}

	//------------------------------------------------------------
	// Build the list entries from s_aNearbyEntities
	//------------------------------------------------------------
	protected void PopulateList()
	{
		if (!m_wPropList)
			return;

		IEntity localChar = SCR_PlayerController.GetLocalControlledEntity();
		vector localPos = vector.Zero;
		if (localChar)
			localPos = localChar.GetOrigin();

		// Sort entries by ascending distance for convenience.
		array<ref CRF_PropHuntSortEntry> sorted = {};
		foreach (IEntity ent : s_aNearbyEntities)
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
		sorted.Sort(); // uses CRF_PropHuntSortEntry.Compare via the Comparable interface

		foreach (CRF_PropHuntSortEntry se : sorted)
		{
			Widget entryWidget = GetGame().GetWorkspace().CreateWidgets(ENTRY_LAYOUT, m_wPropList);
			if (!entryWidget)
				continue;

			// Set labels
			TextWidget labelW = TextWidget.Cast(entryWidget.FindAnyWidget("EntryLabel"));
			if (labelW)
				labelW.SetText(se.m_sDisplay);

			TextWidget distW = TextWidget.Cast(entryWidget.FindAnyWidget("EntryDistance"));
			if (distW)
				distW.SetText(string.Format("%.1f m", se.m_fDist));

			// Wire button
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

		RequestClose();
	}

	//------------------------------------------------------------
	// Cancel
	//------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_wCancelButton)
			RequestClose();
		return false;
	}

	protected void ActionCancel(float value, EActionTrigger reason)
	{
		RequestClose();
	}

	protected void RequestClose()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_PropHuntTransformMenu);
	}

	//------------------------------------------------------------
	// Derive a human-readable name from a full ResourceName.
	// "{GUID}path/to/SomeProp.et"  →  "SomeProp"
	//------------------------------------------------------------
	static string ExtractDisplayName(ResourceName prefab)
	{
		string s = prefab;

		// Strip optional GUID prefix  "{...}"
		if (s.StartsWith("{"))
		{
			int end = s.IndexOf("}");
			if (end >= 0)
				s = s.Substring(end + 1, s.Length() - end - 1);
		}

		// Take filename after last '/'
		int lastSlash = s.LastIndexOf("/");
		if (lastSlash >= 0)
			s = s.Substring(lastSlash + 1, s.Length() - lastSlash - 1);

		// Remove extension (last '.' and beyond)
		int dot = s.LastIndexOf(".");
		if (dot > 0)
			s = s.Substring(0, dot);

		return s;
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

	// Required by array.Sort() — sorts ascending by distance.
	int Compare(CRF_PropHuntSortEntry other)
	{
		if (m_fDist < other.m_fDist) return -1;
		if (m_fDist > other.m_fDist) return 1;
		return 0;
	}
}
