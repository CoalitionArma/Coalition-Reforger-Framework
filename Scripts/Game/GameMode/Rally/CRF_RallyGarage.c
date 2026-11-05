//------------------------------------------------------------------------------------
// CRF_RallyGarage: the rally stage garage, where a crew's driver picks their car's paint before
// their start
//
// Paint only changes how the car looks - every crew still drives the same car. A paint is a set of
// material swaps on the car's mesh, the same thing that makes vanilla's S105 colour prefabs differ,
// applied with SetObject on every machine that draws the car. So the car is never swapped for
// another prefab with the crew sitting in it.
//
// - The paints come from a CRF_RallyPaintConfig (CRF_RallyStageGamemodeComponent's Paint Config,
//   Configs/Rally/CRF_RallyPaints_S105.conf by default). A car whose mesh lacks the materials the
//   paints swap gets no garage.
// - The driver can repaint while their car is held at the start. The garage opens by itself the
//   first time they're held at the wheel, and from the dashboard (CRF_RallyGarageAction, on
//   S105_rally_CRF's starter switch) until their start, when it closes.
// - The server keeps which car has which paint and replicates it from the gamemode component
//   (CRF_RallyStageGamemodeComponent.m_aPaintedCars) rather than from the car, which is spawned at
//   runtime - see the note in CRF_CacheHunt_FlagComponent on why that isn't relied on.
//------------------------------------------------------------------------------------

[BaseContainerProps(configRoot: true)]
class CRF_RallyPaintConfig
{
	[Attribute(desc: "Paints offered in the garage, in order. One without materials is the car as its prefab has it.")]
	protected ref array<ref CRF_RallyPaint> m_aPaints;

	//------------------------------------------------------------------------------------------------
	int Count()
	{
		if (!m_aPaints)
			return 0;

		return m_aPaints.Count();
	}

	//------------------------------------------------------------------------------------------------
	CRF_RallyPaint Get(int index)
	{
		if (!m_aPaints || !m_aPaints.IsIndexValid(index))
			return null;

		return m_aPaints[index];
	}

	//------------------------------------------------------------------------------------------------
	//! Whether the car's mesh has every material these paints swap - S105 paints on another car
	//! would do nothing
	bool Fits(notnull IEntity car)
	{
		VObject mesh = car.GetVObject();
		if (!mesh || Count() == 0)
			return false;

		string meshMaterials[256];
		int count = mesh.GetMaterials(meshMaterials);

		array<string> found = {};
		for (int i = 0; i < count; i++)
		{
			found.Insert(meshMaterials[i]);
		}

		bool swapsAny;
		foreach (CRF_RallyPaint paint : m_aPaints)
		{
			if (!paint.m_aMaterials)
				continue;

			foreach (CRF_RallyPaintMaterial swap : paint.m_aMaterials)
			{
				if (!found.Contains(swap.m_sSourceMaterial))
					return false;

				swapsAny = true;
			}
		}

		return swapsAny;
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), BaseContainerCustomTitleField("m_sName")]
class CRF_RallyPaint
{
	[Attribute(desc: "Name shown in the garage")]
	string m_sName;

	[Attribute("1 1 1 1", UIWidgets.ColorPicker, desc: "Swatch shown next to the name, as an sRGB colour")]
	ref Color m_Swatch;

	[Attribute(desc: "Material swaps on the car's mesh. None: the car as its prefab has it")]
	ref array<ref CRF_RallyPaintMaterial> m_aMaterials;
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), BaseContainerCustomTitleField("m_sSourceMaterial")]
class CRF_RallyPaintMaterial
{
	[Attribute(desc: "Material in the car's mesh, as a MeshObject's SourceMaterial names it")]
	string m_sSourceMaterial;

	[Attribute(uiwidget: UIWidgets.ResourcePickerThumbnail, params: "emat", desc: "Material to put there. Empty: the mesh's own, dropping any the car's prefab assigns")]
	ResourceName m_sMaterial;
}


//------------------------------------------------------------------------------------------------
class CRF_RallyGarage
{
	//------------------------------------------------------------------------------------------------
	//! Every machine that draws the car: give it a paint. SetObject replaces all of the mesh's
	//! material swaps, so the ones the car's prefab makes are kept except where the paint replaces them.
	static bool ApplyPaint(notnull IEntity car, CRF_RallyPaint paint)
	{
		VObject mesh = car.GetVObject();
		if (!mesh)
			return false;

		array<string> sources = {};
		array<string> materials = {};
		GetPrefabMaterials(car, sources, materials);

		if (paint && paint.m_aMaterials)
		{
			foreach (CRF_RallyPaintMaterial swap : paint.m_aMaterials)
			{
				int at = sources.Find(swap.m_sSourceMaterial);
				if (at < 0)
				{
					sources.Insert(swap.m_sSourceMaterial);
					materials.Insert(swap.m_sMaterial);
				}
				else
				{
					materials.Set(at, swap.m_sMaterial);
				}
			}
		}

		string remap;
		foreach (int i, string source : sources)
		{
			string material = materials.Get(i);
			if (!material.IsEmpty())
				remap += string.Format("$remap '%1' '%2';", source, material);
		}

		car.SetObject(mesh, remap);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! The material swaps on the car prefab's MeshObject, as SCR_Global.GetModelAndRemapFromResource
	//! reads them
	protected static void GetPrefabMaterials(notnull IEntity car, notnull array<string> outSources, notnull array<string> outMaterials)
	{
		EntityPrefabData prefabData = car.GetPrefabData();
		if (!prefabData)
			return;

		BaseContainer prefab = prefabData.GetPrefab();
		if (!prefab)
			return;

		BaseContainerList components = prefab.GetObjectArray("components");
		if (!components)
			return;

		BaseContainer meshComponent;
		for (int c = 0; c < components.Count(); c++)
		{
			if (components.Get(c).GetClassName() == "MeshObject")
			{
				meshComponent = components.Get(c);
				break;
			}
		}

		if (!meshComponent)
			return;

		BaseContainerList swaps = meshComponent.GetObjectArray("Materials");
		if (!swaps)
			return;

		ResourceName source;
		ResourceName material;
		for (int m = 0; m < swaps.Count(); m++)
		{
			BaseContainer swap = swaps.Get(m);
			swap.Get("SourceMaterial", source);
			swap.Get("AssignedMaterial", material);
			outSources.Insert(source);
			outMaterials.Insert(material);
		}
	}
}


//------------------------------------------------------------------------------------------------
// The garage window, built with workspace.CreateWidgets like CRF_PropHuntTransformMenu, so it needs
// no menu preset. CRF_RallyStageGamemodeComponent opens it and, ten times a second while it is
// open, tells it which paint is on the car - or closes it once the car can't be repainted.
//------------------------------------------------------------------------------------------------
class CRF_RallyGarageEntryButton : ScriptedWidgetComponent
{
	int m_iPaint;

	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke(this);

		return false;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_RallyGarageDoneButton : ScriptedWidgetComponent
{
	ref ScriptInvoker m_OnClicked = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button == 0)
			m_OnClicked.Invoke();

		return false;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_RallyGarageMenu
{
	protected static const ResourceName MENU_LAYOUT  = "{5F3C91C27D4E9302}UI/layouts/Menus/Rally/CRF_RallyGarage.layout";
	protected static const ResourceName ENTRY_LAYOUT = "{5F3C91C27D4E9303}UI/layouts/Menus/Rally/CRF_RallyGarageEntry.layout";

	protected static ref CRF_RallyGarageMenu s_Instance;

	protected Widget m_wRoot;
	protected RplId m_CarId;
	protected int m_iShownPaint = -2;                // paint marked as on the car, -2 = not marked yet
	protected ref array<Widget> m_aEntries = {};     // one row per paint, in config order
	protected ref COA_MenuPolish m_UIPolish;

	protected static const ref Color LABEL_COLOR   = Color.FromSRGBA(230, 235, 255, 255);
	protected static const ref Color CURRENT_COLOR = Color.FromSRGBA(255, 200, 90, 255);

	//------------------------------------------------------------------------------------------------
	static bool IsOpen()
	{
		return s_Instance != null;
	}

	//------------------------------------------------------------------------------------------------
	//! The car the open garage paints
	static RplId GetCarId()
	{
		if (!s_Instance)
			return RplId.Invalid();

		return s_Instance.m_CarId;
	}

	//------------------------------------------------------------------------------------------------
	static void Open(RplId carId, notnull CRF_RallyPaintConfig paints)
	{
		if (s_Instance)
			s_Instance.Close();

		s_Instance = new CRF_RallyGarageMenu();
		s_Instance.Create(carId, paints);
	}

	//------------------------------------------------------------------------------------------------
	static void CloseGarage()
	{
		if (s_Instance)
			s_Instance.Close();
	}

	//------------------------------------------------------------------------------------------------
	//! Mark the paint that is on the car now (-1: none picked, so the first - the prefab's own)
	static void ShowCurrentPaint(int paint)
	{
		if (!s_Instance)
			return;

		if (paint < 0)
			paint = 0;

		if (paint == s_Instance.m_iShownPaint)
			return;

		s_Instance.m_iShownPaint = paint;
		foreach (int i, Widget entry : s_Instance.m_aEntries)
		{
			bool current = i == paint;

			Widget mark = entry.FindAnyWidget("CurrentText");
			if (mark)
				mark.SetVisible(current);

			TextWidget label = TextWidget.Cast(entry.FindAnyWidget("EntryLabel"));
			if (label != null && current)
				label.SetColor(CURRENT_COLOR);
			else if (label)
				label.SetColor(LABEL_COLOR);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Create(RplId carId, notnull CRF_RallyPaintConfig paints)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			s_Instance = null;
			return;
		}

		m_wRoot = workspace.CreateWidgets(MENU_LAYOUT);
		if (!m_wRoot)
		{
			s_Instance = null;
			return;
		}

		m_CarId = carId;

		ButtonWidget doneButton = ButtonWidget.Cast(m_wRoot.FindAnyWidget("DoneButton"));
		if (doneButton)
		{
			CRF_RallyGarageDoneButton done = CRF_RallyGarageDoneButton.Cast(doneButton.FindHandler(CRF_RallyGarageDoneButton));
			if (done)
				done.m_OnClicked.Insert(Close);
		}

		Widget list = m_wRoot.FindAnyWidget("PaintList");
		if (list)
		{
			for (int i = 0; i < paints.Count(); i++)
			{
				CreateEntry(workspace, list, i, paints.Get(i));
			}
		}

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, ActionClose);

		m_UIPolish = new COA_MenuPolish(m_wRoot, true);

		// The cursor, and button clicks, need these contexts every frame (see CRF_PropHuntTransformMenu)
		GetGame().GetCallqueue().CallLater(KeepCursorActive, 0, true);
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateEntry(WorkspaceWidget workspace, Widget list, int index, CRF_RallyPaint paint)
	{
		Widget entry = workspace.CreateWidgets(ENTRY_LAYOUT, list);
		if (!entry)
			return;

		m_aEntries.Insert(entry);

		TextWidget label = TextWidget.Cast(entry.FindAnyWidget("EntryLabel"));
		if (label && paint)
			label.SetText(paint.m_sName);

		// UI colours are linear: the swatch is given in sRGB, as it would be picked from a photo
		ImageWidget swatch = ImageWidget.Cast(entry.FindAnyWidget("Swatch"));
		if (swatch && paint && paint.m_Swatch)
		{
			int red = paint.m_Swatch.R() * 255;
			int green = paint.m_Swatch.G() * 255;
			int blue = paint.m_Swatch.B() * 255;
			swatch.SetColor(Color.FromSRGBA(red, green, blue, 255));
		}

		Widget mark = entry.FindAnyWidget("CurrentText");
		if (mark)
			mark.SetVisible(false);

		ButtonWidget button = ButtonWidget.Cast(entry.FindAnyWidget("EntryButton"));
		if (!button)
			return;

		CRF_RallyGarageEntryButton handler = CRF_RallyGarageEntryButton.Cast(button.FindHandler(CRF_RallyGarageEntryButton));
		if (!handler)
			return;

		handler.m_iPaint = index;
		handler.m_OnClicked.Insert(OnEntryClicked);
	}

	//------------------------------------------------------------------------------------------------
	//! Ask the server to repaint. The window stays open, so the driver can try another; the mark
	//! moves when the server's answer comes back
	protected void OnEntryClicked(CRF_RallyGarageEntryButton handler)
	{
		if (!handler)
			return;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestRallyPaint(m_CarId, handler.m_iPaint);
	}

	//------------------------------------------------------------------------------------------------
	protected void ActionClose(float value, EActionTrigger reason)
	{
		Close();
	}

	//------------------------------------------------------------------------------------------------
	protected void KeepCursorActive()
	{
		if (!m_wRoot)
		{
			GetGame().GetCallqueue().Remove(KeepCursorActive);
			return;
		}

		GetGame().GetInputManager().ActivateContext("MenuContext");
		GetGame().GetInputManager().ActivateContext("InteractableDialogContext");
	}

	//------------------------------------------------------------------------------------------------
	void Close()
	{
		GetGame().GetCallqueue().Remove(KeepCursorActive);
		GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, ActionClose);

		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		if (m_wRoot)
		{
			m_wRoot.RemoveFromHierarchy();
			m_wRoot = null;
		}

		m_aEntries.Clear();
		if (s_Instance == this)
			s_Instance = null;
	}
}


//------------------------------------------------------------------------------------------------
//! Dashboard action (S105_rally_CRF's starter switch): reopen the garage while the car is held at
//! the start. Local only - the paint itself goes to the server from the garage.
class CRF_RallyGarageAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_RallyStageGamemodeComponent stage = CRF_RallyStageGamemodeComponent.Cast(CRF_RallyGamemodeComponent.GetInstance());
		return stage != null && stage.CanUseGarage(user, GetOwner().GetRootParent());
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (pUserEntity != SCR_PlayerController.GetLocalControlledEntity())
			return;

		CRF_RallyStageGamemodeComponent stage = CRF_RallyStageGamemodeComponent.Cast(CRF_RallyGamemodeComponent.GetInstance());
		if (stage)
			stage.OpenGarage(pUserEntity, GetOwner().GetRootParent());
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
