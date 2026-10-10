modded enum ChimeraMenuPreset
{
	CRF_MiniArsenal
}

//------------------------------------------------------------------------------------------------
//! Safe-start loadout picker. Options come from CRF_MiniArsenalCatalog (faction gear script + role
//! overrides) - the same list the server validates requests against.
class CRF_MiniArsenal: ChimeraMenuBase
{
	protected static const ResourceName CATEGORY_ROW_LAYOUT = "{6A6C0F0000000001}UI/layouts/Menus/Arsenal/MiniArsenalCategoryRow.layout";
	protected static const ResourceName ITEM_ROW_LAYOUT = "{6A6C0F0000000002}UI/layouts/Menus/Arsenal/MiniArsenalItemRow.layout";

	protected static const float REQUEST_COOLDOWN = 1;			// seconds between requests (server enforces its own)
	protected static const int EQUIPPED_REFRESH_MS = 1600;		// server applies clothing after ~0.5 s, weapons after ~1 s
	protected static const int EQUIPPED_REFRESH_LATE_MS = 3200;

	// Weapon slot indices in the character's weapon manager (matches the server handler)
	protected static const int WEAPON_SLOT_PRIMARY = 2;
	protected static const int WEAPON_SLOT_PISTOL = 4;

	// Hover/press feedback and entrance animation - see COA_UIPolish
	protected ref COA_MenuPolish m_UIPolish;

	protected InputManager m_InputManager;
	protected bool m_bFocused = true;
	CameraBase m_Camera;
	CameraBase m_OldCamera;
	LightEntity m_Light;
	COA_Gamemode m_Gamemode;
	ref COA_GearScriptConfig m_GearScriptConfig;
	COA_SafestartManager m_SafeStart;

	Widget m_wRoot;
	VerticalLayoutWidget m_Categories;
	VerticalLayoutWidget m_Items;
	protected TextWidget m_wCategoryTitle;
	protected TextWidget m_wCategoryCount;

	protected COA_EGearRole m_eRole;
	protected CRF_MiniArsenalCategoryButton m_SelectedCategory;
	protected ref array<CRF_MiniArsenalItemButton> m_aItemButtons = {};
	protected string m_sPendingResource;

	float m_fArsenalTimeout = 0;

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_wRoot = GetRootWidget();
		m_InputManager = GetGame().GetInputManager();
		SpawnCameraFacingPlayer();
		m_Gamemode = COA_Gamemode.GetInstance();
		m_SafeStart = COA_SafestartManager.GetInstance();
		m_Categories = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("CategoryButtons"));
		m_Items = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("ItemButtons"));
		m_wCategoryTitle = TextWidget.Cast(m_wRoot.FindAnyWidget("CategoryTitle"));
		m_wCategoryCount = TextWidget.Cast(m_wRoot.FindAnyWidget("CategoryCount"));

		Widget doneButton = m_wRoot.FindAnyWidget("DoneButton");
		if (doneButton)
		{
			SCR_ButtonComponent done = SCR_ButtonComponent.Cast(doneButton.FindHandler(SCR_ButtonComponent));
			if (done)
				done.m_OnClicked.Insert(OnDoneClicked);
		}

		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId());
		if (!m_Gamemode || !player || !player.GetPrefabData() || !playerFaction || !m_Categories || !m_Items)
			return;

		COA_GearScriptContainer container = m_Gamemode.GetGearScriptSettings(playerFaction.GetFactionKey());
		m_GearScriptConfig = CRF_MiniArsenalCatalog.GetConfig(playerFaction.GetFactionKey());
		if (!container || !m_GearScriptConfig)
			return;

		m_eRole = COA_RoleHelper.ResourceToRole(player.GetPrefabData().GetPrefabName());

		array<int> categories = {};
		if (container.m_bEnableMiniWeaponArsenal)
		{
			CRF_MiniArsenalCatalog.GetWeaponCategories(m_GearScriptConfig, m_eRole, categories);
			foreach (int category : categories)
				CreateCategory(category);
		}

		CRF_MiniArsenalCatalog.GetClothingSlots(m_GearScriptConfig, m_eRole, categories);
		foreach (int slot : categories)
			CreateCategory(slot);

		Widget first = m_Categories.GetChildren();
		if (first)
			SelectCategory(SCR_ButtonBaseComponent.Cast(first.FindHandler(CRF_MiniArsenalCategoryButton)));

		// Hover feedback on buttons without their own, and a quick staggered fade-in (COA_UIPolish)
		m_UIPolish = new COA_MenuPolish(GetRootWidget(), true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		if (m_UIPolish)
			m_UIPolish.Update(tDelta);

		if (m_SafeStart && !m_SafeStart.GetSafestartStatus() && COA_PlayerController.IsGracePeriodOver())
			Close();

		if (m_fArsenalTimeout > 0)
			m_fArsenalTimeout -= tDelta;
	}

	//------------------------------------------------------------------------------------------------
	protected void CreateCategory(int category)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets(CATEGORY_ROW_LAYOUT, m_Categories);
		if (!row)
			return;

		ImageWidget icon = ImageWidget.Cast(row.FindAnyWidget("CategoryIcon"));
		if (icon)
		{
			icon.LoadImageTexture(0, CRF_MiniArsenalCatalog.GetCategoryIcon(category));
			icon.SetImage(0);
		}

		TextWidget label = TextWidget.Cast(row.FindAnyWidget("CategoryLabel"));
		if (label)
		{
			string name = CRF_MiniArsenalCatalog.GetCategoryName(category);
			name.ToUpper();
			label.SetText(name);
		}

		CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(row.FindHandler(CRF_MiniArsenalCategoryButton));
		if (!button)
			return;

		button.m_iCategoryIndex = category;
		button.m_bIsPistol = category == CRF_MiniArsenalCatalog.CATEGORY_PISTOL;
		button.m_OnClicked.Insert(SelectCategory);
	}

	//------------------------------------------------------------------------------------------------
	void SelectCategory(SCR_ButtonBaseComponent button)
	{
		CRF_MiniArsenalCategoryButton category = CRF_MiniArsenalCategoryButton.Cast(button);
		if (!category)
			return;

		m_SelectedCategory = category;
		UpdateCategoryStates();

		while (m_Items.GetChildren())
			m_Items.GetChildren().RemoveFromHierarchy();
		m_aItemButtons.Clear();

		int count;
		if (CRF_MiniArsenalCatalog.IsWeaponCategory(category.m_iCategoryIndex))
		{
			array<COA_Weapon_Class> weapons = {};
			CRF_MiniArsenalCatalog.GetWeapons(m_GearScriptConfig, m_eRole, category.m_iCategoryIndex, weapons);
			foreach (COA_Weapon_Class weapon : weapons)
			{
				CRF_MiniArsenalItemButton itemButton = CreateItemRow(weapon.m_Weapon, DescribeWeapon(weapon));
				if (!itemButton)
					continue;

				itemButton.m_iSlotId = category.m_iCategoryIndex;
				itemButton.m_bIsPistol = category.m_bIsPistol;
				if (weapon.m_Attachments)
				{
					foreach (ResourceName attachment : weapon.m_Attachments)
						itemButton.m_aAttachments.Insert(attachment);
				}

				if (weapon.m_MagazineArray)
				{
					foreach (COA_Magazine_Class ammo : weapon.m_MagazineArray)
					{
						itemButton.m_aMagazines.Insert(ammo.m_Magazine);
						itemButton.m_aMagazineCounts.Insert(ammo.m_MagazineCount);
					}
				}

				itemButton.m_OnClicked.Insert(SelectWeapon);
				count++;
			}
		}
		else
		{
			array<ResourceName> items = {};
			CRF_MiniArsenalCatalog.GetClothingOptions(m_GearScriptConfig, m_eRole, category.m_iCategoryIndex, items);
			foreach (ResourceName item : items)
			{
				CRF_MiniArsenalItemButton itemButton = CreateItemRow(item, "");
				if (!itemButton)
					continue;

				itemButton.m_iSlotId = category.m_iCategoryIndex;
				itemButton.m_OnClicked.Insert(SelectItem);
				count++;
			}
		}

		if (m_wCategoryTitle)
		{
			string title = CRF_MiniArsenalCatalog.GetCategoryName(category.m_iCategoryIndex);
			title.ToUpper();
			m_wCategoryTitle.SetText(title);
		}

		if (m_wCategoryCount)
		{
			if (count == 1)
				m_wCategoryCount.SetText("1 OPTION");
			else
				m_wCategoryCount.SetText(string.Format("%1 OPTIONS", count));
		}

		RefreshEquipped();
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_MiniArsenalItemButton CreateItemRow(ResourceName resource, string subtitle)
	{
		Widget row = GetGame().GetWorkspace().CreateWidgets(ITEM_ROW_LAYOUT, m_Items);
		if (!row)
			return null;

		ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		ItemPreviewWidget preview = ItemPreviewWidget.Cast(row.FindAnyWidget("ArsenalItemPreview"));
		if (manager && preview)
			manager.SetPreviewItemFromPrefab(preview, resource);

		string name = CRF_MiniArsenalCatalog.GetDisplayName(resource);
		if (name.IsEmpty())
			name = FilePath.StripExtension(FilePath.StripPath(resource));

		TextWidget nameText = TextWidget.Cast(row.FindAnyWidget("ArsenalItemText"));
		if (nameText)
			nameText.SetText(name);

		TextWidget subText = TextWidget.Cast(row.FindAnyWidget("ArsenalItemSub"));
		if (subText)
		{
			subText.SetText(subtitle);
			subText.SetVisible(!subtitle.IsEmpty());
		}

		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(row.FindHandler(CRF_MiniArsenalItemButton));
		if (!itemButton)
			return null;

		itemButton.m_wButtonRoot = row;
		itemButton.m_sResource = resource;
		m_aItemButtons.Insert(itemButton);
		return itemButton;
	}

	//------------------------------------------------------------------------------------------------
	//! "6 MAGAZINES · 2 ATTACHMENTS"
	protected string DescribeWeapon(COA_Weapon_Class weapon)
	{
		int magazines;
		if (weapon.m_MagazineArray)
		{
			foreach (COA_Magazine_Class ammo : weapon.m_MagazineArray)
				magazines += ammo.m_MagazineCount;
		}

		int attachments;
		if (weapon.m_Attachments)
		{
			foreach (ResourceName attachment : weapon.m_Attachments)
			{
				if (!attachment.IsEmpty())
					attachments++;
			}
		}

		string text = string.Format("%1 MAGAZINES", magazines);
		if (magazines == 1)
			text = "1 MAGAZINE";

		if (attachments == 1)
			text += "  ·  1 ATTACHMENT";
		else if (attachments > 1)
			text += string.Format("  ·  %1 ATTACHMENTS", attachments);

		return text;
	}

	//------------------------------------------------------------------------------------------------
	//! Category rail look: toggled state drives the fill (SCR_ButtonBaseComponent colorization),
	//! script drives accent bar, icon and label
	protected void UpdateCategoryStates()
	{
		Widget row = m_Categories.GetChildren();
		while (row)
		{
			CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(row.FindHandler(CRF_MiniArsenalCategoryButton));
			bool selected = button && button == m_SelectedCategory;
			if (button)
				button.SetToggled(selected, true, false);

			Color foreground = Color.FromSRGBA(169, 180, 204, 255);
			if (selected)
				foreground = Color.FromSRGBA(239, 242, 247, 255);

			Widget accent = row.FindAnyWidget("CategoryAccent");
			if (accent)
			{
				if (selected)
					accent.SetColor(Color.FromSRGBA(201, 54, 54, 255));
				else
					accent.SetColor(Color.FromSRGBA(201, 54, 54, 0));
			}

			Widget icon = row.FindAnyWidget("CategoryIcon");
			if (icon)
				icon.SetColor(foreground);

			Widget label = row.FindAnyWidget("CategoryLabel");
			if (label)
				label.SetColor(foreground);

			row = row.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Mark the row that matches what the character currently has in the selected slot
	protected void RefreshEquipped()
	{
		if (!m_SelectedCategory)
			return;

		ResourceName equipped = GetEquippedResource(m_SelectedCategory.m_iCategoryIndex, m_SelectedCategory.m_bIsPistol);
		if (!equipped.IsEmpty() && equipped == m_sPendingResource)
			m_sPendingResource = "";

		foreach (CRF_MiniArsenalItemButton itemButton : m_aItemButtons)
		{
			if (!itemButton || !itemButton.m_wButtonRoot)
				continue;

			bool isEquipped = !equipped.IsEmpty() && itemButton.m_sResource == equipped;
			bool isPending = !m_sPendingResource.IsEmpty() && itemButton.m_sResource == m_sPendingResource;

			Widget accent = itemButton.m_wButtonRoot.FindAnyWidget("ItemAccent");
			if (accent)
			{
				if (isEquipped)
					accent.SetColor(Color.FromSRGBA(201, 54, 54, 255));
				else
					accent.SetColor(Color.FromSRGBA(201, 54, 54, 0));
			}

			TextWidget tag = TextWidget.Cast(itemButton.m_wButtonRoot.FindAnyWidget("EquippedTag"));
			if (tag)
			{
				tag.SetVisible(isEquipped || isPending);
				if (isPending)
				{
					tag.SetText("APPLYING…");
					tag.SetColor(Color.FromSRGBA(169, 180, 204, 255));
				}
				else
				{
					tag.SetText("EQUIPPED");
					tag.SetColor(Color.FromSRGBA(232, 96, 96, 255));
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName GetEquippedResource(int category, bool isPistol)
	{
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		if (!player)
			return "";

		IEntity item;
		if (CRF_MiniArsenalCatalog.IsWeaponCategory(category))
		{
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
			if (!controller || !controller.GetWeaponManagerComponent())
				return "";

			array<WeaponSlotComponent> slots = {};
			controller.GetWeaponManagerComponent().GetWeaponsSlots(slots);
			int index = WEAPON_SLOT_PRIMARY;
			if (isPistol)
				index = WEAPON_SLOT_PISTOL;

			if (slots.IsIndexValid(index) && slots[index])
				item = slots[index].GetWeaponEntity();
		}
		else
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(player.FindComponent(BaseInventoryStorageComponent));
			if (storage)
				item = storage.Get(category);
		}

		if (!item || !item.GetPrefabData())
			return "";

		return item.GetPrefabData().GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	protected bool BeginRequest(CRF_MiniArsenalItemButton itemButton)
	{
		if (!itemButton)
			return false;

		if (m_fArsenalTimeout > 0)
		{
			SCR_NotificationsComponent.GetInstance().SendLocal(ENotification.ACTION_ON_COOLDOWN, m_fArsenalTimeout * 100);
			return false;
		}

		m_fArsenalTimeout = REQUEST_COOLDOWN;
		m_sPendingResource = itemButton.m_sResource;
		RefreshEquipped();

		GetGame().GetCallqueue().Remove(RefreshEquipped);
		GetGame().GetCallqueue().CallLater(RefreshEquipped, EQUIPPED_REFRESH_MS);
		GetGame().GetCallqueue().CallLater(ClearPendingAndRefresh, EQUIPPED_REFRESH_LATE_MS);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearPendingAndRefresh()
	{
		m_sPendingResource = "";
		RefreshEquipped();
	}

	//------------------------------------------------------------------------------------------------
	void SelectItem(SCR_ButtonBaseComponent button)
	{
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(button);
		if (!BeginRequest(itemButton))
			return;

		COA_PlayerRplToAuthorityManager.GetInstance().MiniArsenalRequestNewItem(SCR_PlayerController.GetLocalPlayerId(), itemButton.m_sResource, itemButton.m_iSlotId);
	}

	//------------------------------------------------------------------------------------------------
	void SelectWeapon(SCR_ButtonBaseComponent button)
	{
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(button);
		if (!BeginRequest(itemButton))
			return;

		// The server takes attachments and magazines from the gear script, not from these lists
		COA_PlayerRplToAuthorityManager.GetInstance().MiniArsenalRequestNewWeapon(SCR_PlayerController.GetLocalPlayerId(), itemButton.m_sResource, itemButton.m_aAttachments,
			itemButton.m_aMagazines, itemButton.m_aMagazineCounts, itemButton.m_bIsPistol);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDoneClicked(SCR_ButtonBaseComponent button)
	{
		GetGame().GetCallqueue().CallLater(Close, 0); // not from inside the button's own handler
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		GetGame().GetCallqueue().Remove(RefreshEquipped);
		GetGame().GetCallqueue().Remove(ClearPendingAndRefresh);

		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		super.OnMenuClose();
		if (m_Light)
			delete m_Light;

		if (m_Camera)
			delete m_Camera;
		GetGame().GetCameraManager().SetCamera(m_OldCamera);
	}

	//------------------------------------------------------------------------------------------------
	//! Kept for compatibility with anything still calling it - see CRF_MiniArsenalCatalog
	ResourceName GetCategoryIcon(int index)
	{
		return CRF_MiniArsenalCatalog.GetCategoryIcon(index);
	}

	//------------------------------------------------------------------------------------------------
	//! Camera in front of the player, shifted sideways so the character stands right of the panel
	void SpawnCameraFacingPlayer()
	{
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		if (!player)
			return;

		vector mat[4];
		player.GetWorldTransform(mat);
		vector playerPos = mat[3];
		vector forward = mat[2];
		vector camPos = playerPos + forward * 2.2;
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform[3] = camPos;
		IEntity cam = GetGame().SpawnEntityPrefab(Resource.Load("{D6DE32D1C0FCC1C7}Prefabs/Editor/Camera/ManualCameraBase.et"), GetGame().GetWorld(), spawnParams);
		if (!cam)
			return;

		vector dir = vector.Direction(camPos, playerPos).Normalized();

		vector worldUp = "0 1 0";

		vector right = worldUp * dir;
		right.Normalize();

		vector up = dir * right;
		up.Normalize();

		vector camMat[4];
		camMat[0] = right;
		camMat[1] = up;
		camMat[2] = dir;
		camMat[3] = camPos - right * 0.55;	// player appears right of centre, clear of the left panel
		camMat[3][1] = camMat[3][1] + 0.75;

		cam.SetWorldTransform(camMat);
		m_Camera = CameraBase.Cast(cam);
		m_OldCamera = GetGame().GetCameraManager().CurrentCamera();
		GetGame().GetCameraManager().SetCamera(m_Camera);
		EntitySpawnParams lightParam = new EntitySpawnParams();
		m_Camera.GetTransform(lightParam.Transform);
		m_Light = LightEntity.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{226FC159EFA3EAA2}Prefabs/Systems/MiniArsenalLight.et"), null, lightParam));
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusLost()
	{
		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}
}
