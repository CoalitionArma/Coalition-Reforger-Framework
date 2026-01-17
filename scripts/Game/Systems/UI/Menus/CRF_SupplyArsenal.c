modded enum ChimeraMenuPreset
{
	CRF_SupplyArsenal
}

class CRF_SupplyArsenal: ChimeraMenuBase
{
	Widget m_wRoot;
	CRF_GearscriptManager m_GearscriptManager;
	CRF_GearScriptContainer m_GearScriptContainer;
	CRF_SupplyArsenalComponent m_SupplyArsnealComponent;
	ref CRF_GearScriptConfig m_GearScriptConfig;
	bool m_bSupplyEnabled;
	
	VerticalLayoutWidget m_Notifications;
	VerticalLayoutWidget m_Categories;
	VerticalLayoutWidget m_Items;
	float m_fArsenalTimeout;
	protected InputManager m_InputManager;
	protected bool m_bFocused = true;
	
	EditBoxWidget m_wEditBox;
	SCR_ButtonComponent m_SelectedButton;
	SCR_ButtonComponent m_AddItem;
	
	ref array<Widget> m_aNotifications = {};
	
	IEntity m_ArsenalPoint;
	IEntity m_ClosestTruck;
	
	ref map<string, int> m_SupplyCosts = new map<string, int>;
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_wRoot = GetRootWidget();
		m_wEditBox = EditBoxWidget.Cast(m_wRoot.FindAnyWidget("Amount"));
		m_InputManager = GetGame().GetInputManager();
		
		string factionKey = SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_GearScriptContainer = m_GearscriptManager.GetGearScriptSettings(factionKey);
		ResourceName gearResource = m_GearscriptManager.GetGearScriptResource(factionKey);
		m_GearScriptConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearResource).GetResource().ToBaseContainer()));
		m_Categories = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("CategoryButtons"));
		m_Items = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("ItemButtons"));
		m_Notifications = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("Notifications"));
		m_AddItem = SCR_ButtonComponent.Cast(m_wRoot.FindAnyWidget("AddSupplyButton").FindHandler(SCR_ButtonComponent));
		m_AddItem.m_OnClicked.Insert(SpawnItem);
		PopulateCategories();
		GetGame().GetCallqueue().CallLater(UpdateArsenal, 500, false);
	}
	
	void UpdateArsenal()
	{
		m_SupplyArsnealComponent = CRF_SupplyArsenalComponent.Cast(m_ArsenalPoint.FindComponent(CRF_SupplyArsenalComponent));
		m_bSupplyEnabled = SCR_ResourceSystemHelper.IsGlobalResourceTypeEnabled(EResourceType.SUPPLIES) && m_SupplyArsnealComponent.m_bSupplyEnabled;
		CRF_RplToAuthorityManager.GetInstance().UpdateSupplyArsneal(RplComponent.Cast(m_ArsenalPoint.FindComponent(RplComponent)).Id());
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		array<Widget> notificationsToDelete = {};
		foreach (Widget notification: m_aNotifications)
		{
			CRF_SupplyNotification notificationComp = CRF_SupplyNotification.Cast(notification.FindHandler(CRF_SupplyNotification));
			notificationComp.m_fTimeAlive += tDelta;
			if (notificationComp.m_fTimeAlive > 3 && !notificationComp.m_bAnimationStarted)
			{
				notificationComp.m_bAnimationStarted = true;
				AnimateWidget.Opacity(notification, 0, 3);
			}
			
			if (notificationComp.m_bAnimationStarted && notification.GetOpacity() <= 0)
				notificationsToDelete.Insert(notification);
		}
		
		foreach (Widget notification: notificationsToDelete)
		{
			m_aNotifications.RemoveItem(notification);
			delete notification;
		}
		
		if (m_bSupplyEnabled)
			m_wRoot.FindAnyWidget("CurrentSupplies").SetVisible(true);
		else
			m_wRoot.FindAnyWidget("CurrentSupplies").SetVisible(false);
		
		if (m_SupplyArsnealComponent && m_bSupplyEnabled)
			TextWidget.Cast(m_wRoot.FindAnyWidget("CurrentSupplies")).SetText("Current Supplies: " + m_SupplyArsnealComponent.GetCurrentSupply().ToString());
	}
	
	void PopulateCategories()
	{
		for (int i = 0; i <= 11; i++)
		{
			//Regular Weapons
			if (i < 4 || i == 11)
			{
				array<ref CRF_Weapon_Class> weapons = GetWeaponsByIndex(i, m_GearScriptConfig);
				if (weapons.Count() == 0)
					continue;
				Widget category = GetGame().GetWorkspace().CreateWidgets("{BC371ACC7C58B63E}UI/layouts/Menus/Arsenal/MiniArsenalCategory.layout", m_Categories);
				ImageWidget categoryIcon = ImageWidget.Cast(category.FindWidget("CategoryIcon"));
				categoryIcon.LoadImageTexture(0, GetCategoryIcon(i));
				categoryIcon.SetImage(0);
				CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(category.FindHandler(CRF_MiniArsenalCategoryButton));
				button.m_OnClicked.Insert(SelectCategory);
				
				foreach (CRF_Weapon_Class weapon: weapons)
				{
					if (!weapon)
						continue;
					button.m_Weapons.Insert(weapon);
					if (!weapon.m_MagazineArray)
						continue;
					if (weapon.m_MagazineArray.Count() == 0)
						continue;
					foreach (CRF_Magazine_Class magazine: weapon.m_MagazineArray)
					{
						button.m_Magazines.Insert(magazine);
					}
				}
			}
			else
			{
				CRF_Spec_Weapon_Class weapon = GetSpecWeaponByIndex(i, m_GearScriptConfig);
				if (!weapon)
					continue;
				Widget category = GetGame().GetWorkspace().CreateWidgets("{BC371ACC7C58B63E}UI/layouts/Menus/Arsenal/MiniArsenalCategory.layout", m_Categories);
				ImageWidget categoryIcon = ImageWidget.Cast(category.FindWidget("CategoryIcon"));
				categoryIcon.LoadImageTexture(0, GetCategoryIcon(i));
				categoryIcon.SetImage(0);
				CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(category.FindHandler(CRF_MiniArsenalCategoryButton));
				button.m_OnClicked.Insert(SelectCategory);
				
				button.m_SpecWeapons.Insert(weapon);
			}
		}
		
		array<ResourceName> miscItems = {};
		array<ResourceName> explosiveItems = {};
		array<ResourceName> grenadeItems = {};
		array<ResourceName> radioItems = {};
		array<ResourceName> medicalItems = {};
		
		foreach (CRF_Inventory_Item item: m_GearScriptConfig.m_DefaultInventoryItems)
		{
			if (miscItems.Contains(item.m_sItemPrefab) || explosiveItems.Contains(item.m_sItemPrefab) || grenadeItems.Contains(item.m_sItemPrefab) || 
				radioItems.Contains(item.m_sItemPrefab) || medicalItems.Contains(item.m_sItemPrefab))
					continue;
			switch (GetEquipmentType(item.m_sItemPrefab))
			{
				case -1: continue; break;
				case 0: miscItems.Insert(item.m_sItemPrefab); break;
				case 1: explosiveItems.Insert(item.m_sItemPrefab); break;
				case 2: grenadeItems.Insert(item.m_sItemPrefab); break;
				case 3: radioItems.Insert(item.m_sItemPrefab); break;
				case 4: medicalItems.Insert(item.m_sItemPrefab); break;
			}		
		}
		
		//Just used to check for duplicates
		array<ResourceName> overrideWeaponsResources = {};
		array<ref CRF_Weapon_Class> overrideWeapons = {};
		
		foreach (CRF_Role_Custom_Gear customGear: m_GearScriptConfig.m_RolesToSetCustomSettings)
		{
			foreach (CRF_Weapon_Class primary: customGear.m_PrimaryWeapon)
			{
				if (overrideWeaponsResources.Contains(primary.m_Weapon))
					continue;
				overrideWeapons.Insert(primary);
				overrideWeaponsResources.Insert(primary.m_Weapon);
			}
			
			foreach (CRF_Weapon_Class secondary: customGear.m_SecondaryWeapon)
			{
				if (overrideWeaponsResources.Contains(secondary.m_Weapon))
					continue;
				overrideWeapons.Insert(secondary);
				overrideWeaponsResources.Insert(secondary.m_Weapon);
			}
			
			foreach (CRF_Weapon_Class pistol: customGear.m_Pistols)
			{
				if (overrideWeaponsResources.Contains(pistol.m_Weapon))
					continue;
				overrideWeapons.Insert(pistol);
				overrideWeaponsResources.Insert(pistol.m_Weapon);
			}
			
			foreach (CRF_Inventory_Item item: customGear.m_AdditionalInventoryItems)
			{
				if (miscItems.Contains(item.m_sItemPrefab) || explosiveItems.Contains(item.m_sItemPrefab) || grenadeItems.Contains(item.m_sItemPrefab) || 
					radioItems.Contains(item.m_sItemPrefab) || medicalItems.Contains(item.m_sItemPrefab))
						continue;
				switch (GetEquipmentType(item.m_sItemPrefab))
				{
					case -1: continue; break;
					case 0: miscItems.Insert(item.m_sItemPrefab); break;
					case 1: explosiveItems.Insert(item.m_sItemPrefab); break;
					case 2: grenadeItems.Insert(item.m_sItemPrefab); break;
					case 3: radioItems.Insert(item.m_sItemPrefab); break;
					case 4: medicalItems.Insert(item.m_sItemPrefab); break;
				}	
			}
		}
		
		foreach (ResourceName item: m_GearScriptContainer.m_aAdditonalItemsForSupplyArsenal)
		{
			switch (GetEquipmentType(item))
			{
				case -1: continue; break;
				case 0: miscItems.Insert(item); break;
				case 1: explosiveItems.Insert(item); break;
				case 2: grenadeItems.Insert(item); break;
				case 3: radioItems.Insert(item); break;
				case 4: medicalItems.Insert(item); break;
			}	
		}
		
		foreach (CRF_Inventory_Item item: m_GearScriptConfig.m_InfantryMedicalItems)
		{
			medicalItems.Insert(item.m_sItemPrefab);
		}

		
		foreach (CRF_Inventory_Item item: m_GearScriptConfig.m_MedicMedicalItems)
		{
			medicalItems.Insert(item.m_sItemPrefab);
		}
		
		radioItems.Insert(m_GearScriptContainer.m_rShortRangeRadioPrefab);
		radioItems.Insert(m_GearScriptContainer.m_rLongRangeRadioPrefab);
		radioItems.Insert(m_GearScriptContainer.m_rRTORadiosPrefab);
		
		if (miscItems.Count() > 0)
			PopulateInventoryItems(miscItems, 0);
		
		if (explosiveItems.Count() > 0)
			PopulateInventoryItems(explosiveItems, 1);
		
		if (grenadeItems.Count() > 0)
			PopulateInventoryItems(grenadeItems, 2);
		
		if (radioItems.Count() > 0)
			PopulateInventoryItems(radioItems, 3);
		
		if (medicalItems.Count() > 0)
			PopulateInventoryItems(medicalItems, 4);
		
		if (overrideWeapons.Count() == 0)
			return;
		Widget category = GetGame().GetWorkspace().CreateWidgets("{BC371ACC7C58B63E}UI/layouts/Menus/Arsenal/MiniArsenalCategory.layout", m_Categories);
		ImageWidget categoryIcon = ImageWidget.Cast(category.FindWidget("CategoryIcon"));
		categoryIcon.LoadImageTexture(0, GetCategoryIcon(12));
		categoryIcon.SetImage(0);
		CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(category.FindHandler(CRF_MiniArsenalCategoryButton));
		button.m_OnClicked.Insert(SelectCategory);
		
		foreach (CRF_Weapon_Class weapon: overrideWeapons)
		{
			button.m_Weapons.Insert(weapon);
			foreach (CRF_Magazine_Class magazine: weapon.m_MagazineArray)
			{
				button.m_Magazines.Insert(magazine);
			}
		}
	}
	
	void PopulateInventoryItems(array<ResourceName> items, int index)
	{
		Widget category = GetGame().GetWorkspace().CreateWidgets("{BC371ACC7C58B63E}UI/layouts/Menus/Arsenal/MiniArsenalCategory.layout", m_Categories);
		ImageWidget categoryIcon = ImageWidget.Cast(category.FindWidget("CategoryIcon"));
		categoryIcon.LoadImageTexture(0, GetItemCategoryIcon(index));
		categoryIcon.SetImage(0);
		CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(category.FindHandler(CRF_MiniArsenalCategoryButton));
		button.m_OnClicked.Insert(SelectCategory);
			
		foreach (ResourceName item: items)
		{
			CRF_Inventory_Item itemObject = new CRF_Inventory_Item();
			itemObject.m_sItemPrefab = item;
			button.m_Items.Insert(itemObject);
		}
	}
	
	//-1 Error
	//0 Misc
	//1 Explosive
	//2 Grenade
	//3 Radio
	//4 Medical
	int GetEquipmentType(string resource)
	{
		Resource loaded = Resource.Load(resource);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loaded);
		if (!entitySource)
			return -1;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
			typename typeToCheck = componentSource.GetClassName().ToType();
	        if (typeToCheck.IsInherited(SCR_MineWeaponComponent) || typeToCheck.IsInherited(SCR_ExplosiveChargeComponent))
	       		return 1;
			
			if (typeToCheck.IsInherited(GrenadeMoveComponent))
	       		return 2;
			
			if (typeToCheck.IsInherited(CVON_RadioComponent))
	       		return 3;
			
			if (typeToCheck.IsInherited(SCR_ConsumableItemComponent))
	       		return 4;
	    }
		
		return 0;
	}
	
	array<ref CRF_Weapon_Class> GetWeaponsByIndex(int index, CRF_GearScriptConfig gearSriptConfig)
	{
		array<ref CRF_Weapon_Class> weapons = {};
		
		switch(index)
		{
			case 0:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_Rifles)
				weapons.Insert(weapon);
			break;

			case 1:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_RifleUGLs)
				weapons.Insert(weapon);
			break;

			case 2:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_Carbines)
				weapons.Insert(weapon);
			break;

			case 3:
			foreach (CRF_Weapon_Class weapon: gearSriptConfig.m_Pistols)
				weapons.Insert(weapon);
			break;

			case 11:
			weapons.Insert(gearSriptConfig.m_SNIPER);
			break;
		}

		return weapons;
	}
	
	CRF_Spec_Weapon_Class GetSpecWeaponByIndex(int index, CRF_GearScriptConfig gearSriptConfig)
	{
		CRF_Spec_Weapon_Class weapon;

		switch (index)
		{
			case 4:
			weapon = gearSriptConfig.m_AR;
			break;

			case 5:
			weapon = gearSriptConfig.m_MMG;
			break;

			case 6:
			weapon = gearSriptConfig.m_HMG;
			break;

			case 7:
			weapon = gearSriptConfig.m_AT;
			break;

			case 8:
			weapon = gearSriptConfig.m_MAT;
			break;

			case 9:
			weapon = gearSriptConfig.m_HAT;
			break;

			case 10:
			weapon = gearSriptConfig.m_AA;
			break;
		}

		return weapon;
	}
	
	ResourceName GetItemCategoryIcon(int index)
	{
		switch (index)
		{
			case 0: return "{CDF94F179A33CFB9}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Accessories.edds"; break;
			case 1: return "{91C4A9B5AD80D443}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Explosive.edds"; break;
			case 2: return "{233A8BC0520B1B8B}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Grenades.edds"; break;
			case 3: return "{4B4A98921A5C3583}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Radio.edds"; break;
			case 4: return "{CDF868B16669B7EA}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Medical.edds"; break;
		}
		
		return "{CDF94F179A33CFB9}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Accessories.edds";
	}
	
	ResourceName GetCategoryIcon(int index)
	{
		switch (index)
		{
			case 0: return "{71648F15B3984B87}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_AssaultRifles.edds"; break;
			case 1: return "{71648F15B3984B87}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_AssaultRifles.edds"; break;
			case 2: return "{71648F15B3984B87}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_AssaultRifles.edds"; break;
			case 3: return "{2EEBBBCA36DD775F}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Pistols.edds"; break;
			case 4: return "{A02EB3B80E276400}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_MachineGuns.edds"; break;
			case 5: return "{A02EB3B80E276400}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_MachineGuns.edds"; break;
			case 6: return "{A02EB3B80E276400}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_MachineGuns.edds"; break;
			case 7: return "{10840233D666C940}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Launcher.edds"; break;
			case 8: return "{10840233D666C940}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Launcher.edds"; break;
			case 9: return "{10840233D666C940}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Launcher.edds"; break;
			case 10: return "{10840233D666C940}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Launcher.edds"; break;
			case 11: return "{A2B4C0BFBECE6400}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_SniperRifles.edds"; break;
			case 12: return "{71648F15B3984B87}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_AssaultRifles.edds"; break;
		}
		
		return "{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds";
	}
	
	void SelectCategory(SCR_ButtonBaseComponent button)
	{
		while (m_Items.GetChildren())
			delete m_Items.GetChildren();
		
		CRF_MiniArsenalCategoryButton miniArsnealCategory = CRF_MiniArsenalCategoryButton.Cast(button);
		ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		if (!manager)
			return;
		
		array<ResourceName> itemsToBeAdded = {};
		foreach (CRF_Weapon_Class weapon: miniArsnealCategory.m_Weapons)
		{
			itemsToBeAdded.Insert(weapon.m_Weapon);
			
			foreach (CRF_Magazine_Class magazine: weapon.m_MagazineArray)
			{
				itemsToBeAdded.Insert(magazine.m_Magazine);
			}
		}
		foreach (CRF_Spec_Weapon_Class specWeapon: miniArsnealCategory.m_SpecWeapons)
		{
			itemsToBeAdded.Insert(specWeapon.m_Weapon);
			
			foreach (CRF_Magazine_Class magazine: specWeapon.m_MagazineArray)
			{
				itemsToBeAdded.Insert(magazine.m_Magazine);
			}
		}
		foreach (CRF_Inventory_Item item: miniArsnealCategory.m_Items)
		{
			itemsToBeAdded.Insert(item.m_sItemPrefab);
		}
		
		array<int> supplyCosts = m_GearscriptManager.GetSupplyValuesForItems(itemsToBeAdded);
		m_SupplyCosts.Clear();
		for (int i = 0; i < itemsToBeAdded.Count(); i ++)
		{
			m_SupplyCosts.Insert(itemsToBeAdded[i], supplyCosts[i]);
		}
		
		foreach (CRF_Weapon_Class weapon: miniArsnealCategory.m_Weapons)
		{
			DrawWeaponItem(weapon, manager, miniArsnealCategory).m_OnClicked.Insert(SelectItem);
			
			foreach (CRF_Magazine_Class magazine: weapon.m_MagazineArray)
			{
				DrawMagazineItem(magazine, manager, miniArsnealCategory).m_OnClicked.Insert(SelectItem);
			}
		}
		foreach (CRF_Spec_Weapon_Class specWeapon: miniArsnealCategory.m_SpecWeapons)
		{
			DrawSpecWeaponItem(specWeapon, manager, miniArsnealCategory).m_OnClicked.Insert(SelectItem);
			
			foreach (CRF_Magazine_Class magazine: specWeapon.m_MagazineArray)
			{
				DrawMagazineItem(magazine, manager, miniArsnealCategory).m_OnClicked.Insert(SelectItem);
			}
		}
		foreach (CRF_Inventory_Item item: miniArsnealCategory.m_Items)
		{
			DrawEquipmentItem(item, manager, miniArsnealCategory).m_OnClicked.Insert(SelectItem);
		}
		return;
	}

	CRF_MiniArsenalItemButton DrawWeaponItem(CRF_Weapon_Class weapon, ItemPreviewManagerEntity manager, CRF_MiniArsenalCategoryButton miniArsnealCategory)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{ADD28B3C4F9377B1}UI/layouts/Menus/Arsenal/SupplyArsenalItem.layout", m_Items);
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		manager.SetPreviewItemFromPrefab(itemPreview, weapon.m_Weapon);
		if (m_bSupplyEnabled)
			TextWidget.Cast(item.FindAnyWidget("Supply")).SetText(m_SupplyCosts.Get(weapon.m_Weapon).ToString());
		else
		{
			item.FindAnyWidget("Supply").SetVisible(false);
			item.FindAnyWidget("SupplyImage").SetVisible(false);
		}
			
		Resource loadedweapon = Resource.Load(weapon.m_Weapon);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedweapon);
		if (entitySource)
		{
		    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if (componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
		        {
		            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
		            if (attributesContainer)
		            {
		                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
		                if (itemDisplayNameContainer)
		                {
		                    string name;
		                    itemDisplayNameContainer.Get("Name", name);
		
		                    TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(name);
		                    break;
		                }
		            }
		        }
		    }
		}
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = weapon.m_Weapon;
		itemButton.m_iSupplyCost = m_SupplyCosts.Get(weapon.m_Weapon);
		return itemButton;
	}
	
	CRF_MiniArsenalItemButton DrawMagazineItem(CRF_Magazine_Class magazine, ItemPreviewManagerEntity manager, CRF_MiniArsenalCategoryButton miniArsnealCategory)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{DE9732402EA37142}UI/layouts/Menus/Arsenal/SupplyArsenalMagazine.layout", m_Items);
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		manager.SetPreviewItemFromPrefab(itemPreview, magazine.m_Magazine);
		if (m_bSupplyEnabled)
			TextWidget.Cast(item.FindAnyWidget("Supply")).SetText(m_SupplyCosts.Get(magazine.m_Magazine).ToString());
		else
		{
			item.FindAnyWidget("Supply").SetVisible(false);
			item.FindAnyWidget("SupplyImage").SetVisible(false);
		}
			
		Resource loadedweapon = Resource.Load(magazine.m_Magazine);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedweapon);
		if (entitySource)
		{
		    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if (componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
		        {
		            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
		            if (attributesContainer)
		            {
		                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
		                if (itemDisplayNameContainer)
		                {
		                    string name;
		                    itemDisplayNameContainer.Get("Name", name);
		
		                    TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(name);
		                    break;
		                }
		            }
		        }
		    }
		}
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = magazine.m_Magazine;
		itemButton.m_iSupplyCost = m_SupplyCosts.Get(magazine.m_Magazine);
		return itemButton;
	}
	
	CRF_MiniArsenalItemButton DrawEquipmentItem(CRF_Inventory_Item itemObject, ItemPreviewManagerEntity manager, CRF_MiniArsenalCategoryButton miniArsnealCategory)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{DE9732402EA37142}UI/layouts/Menus/Arsenal/SupplyArsenalMagazine.layout", m_Items);
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		manager.SetPreviewItemFromPrefab(itemPreview, itemObject.m_sItemPrefab);
		if (m_bSupplyEnabled)
			TextWidget.Cast(item.FindAnyWidget("Supply")).SetText(m_SupplyCosts.Get(itemObject.m_sItemPrefab).ToString());
		else
		{
			item.FindAnyWidget("Supply").SetVisible(false);
			item.FindAnyWidget("SupplyImage").SetVisible(false);
		}
			
		Resource loadedweapon = Resource.Load(itemObject.m_sItemPrefab);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedweapon);
		if (entitySource)
		{
		    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if (componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
		        {
		            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
		            if (attributesContainer)
		            {
		                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
		                if (itemDisplayNameContainer)
		                {
		                    string name;
		                    itemDisplayNameContainer.Get("Name", name);
		
		                    TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(name);
		                    break;
		                }
		            }
		        }
		    }
		}
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = itemObject.m_sItemPrefab;
		itemButton.m_iSupplyCost = m_SupplyCosts.Get(itemObject.m_sItemPrefab);
		return itemButton;
	}
	
	CRF_MiniArsenalItemButton DrawSpecWeaponItem(CRF_Spec_Weapon_Class weapon, ItemPreviewManagerEntity manager, CRF_MiniArsenalCategoryButton miniArsnealCategory)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{ADD28B3C4F9377B1}UI/layouts/Menus/Arsenal/SupplyArsenalItem.layout", m_Items);
		ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
		manager.SetPreviewItemFromPrefab(itemPreview, weapon.m_Weapon);
		if (m_bSupplyEnabled)
			TextWidget.Cast(item.FindAnyWidget("Supply")).SetText(m_SupplyCosts.Get(weapon.m_Weapon).ToString());
		else
		{
			item.FindAnyWidget("Supply").SetVisible(false);
			item.FindAnyWidget("SupplyImage").SetVisible(false);
		}
			
		Resource loadedweapon = Resource.Load(weapon.m_Weapon);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedweapon);
		if (entitySource)
		{
		    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if (componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
		        {
		            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
		            if (attributesContainer)
		            {
		                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
		                if (itemDisplayNameContainer)
		                {
		                    string name;
		                    itemDisplayNameContainer.Get("Name", name);
		
		                    TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(name);
		                    break;
		                }
		            }
		        }
		    }
		}
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
		itemButton.m_wButtonRoot = item;
		itemButton.m_sResource = weapon.m_Weapon;
		itemButton.m_iSupplyCost = m_SupplyCosts.Get(weapon.m_Weapon);
		return itemButton;
	}
	
	void SelectItem(SCR_ButtonComponent button)
	{
		m_SelectedButton = button;
	}
	
	void SpawnItem()
	{
		if (!m_SelectedButton)
			return;
		
		if (!m_SupplyArsnealComponent)
			m_SupplyArsnealComponent = CRF_SupplyArsenalComponent.Cast(m_ArsenalPoint.FindComponent(CRF_SupplyArsenalComponent));
		
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(m_SelectedButton);
		
		int supplyNeeded = itemButton.m_iSupplyCost;
		array<RplId> supplyObjectRplId = {};
		array<IEntity> supplyObjects = {};
		array<int> supplyToSubtract = {};
		
		if (m_bSupplyEnabled)
		{
			int totalAvailable = 0;
			foreach (int supply : m_SupplyArsnealComponent.GetLocalSupplyCounts())
			    totalAvailable += supply;
			
			if (totalAvailable < supplyNeeded)
			{
			    NoSupplyNotification();
			    return;
			}
			
			
			
			while (supplyNeeded > 0)
			{
			    IEntity supplyObject = null;
			    int minSupply = int.MAX;
			    int minIndex = -1;
			
			    // Find the supply object with the *smallest* nonzero count
			    array<int> supplyCounts = m_SupplyArsnealComponent.GetLocalSupplyCounts();
			    for (int i = 0; i < m_SupplyArsnealComponent.GetLocalEntityArray().Count(); i++)
			    {
			        int count = supplyCounts[i];
			        if (count <= 0)
			            continue;
			
			        if (count < minSupply)
			        {
			            minSupply = count;
			            supplyObject = m_SupplyArsnealComponent.GetLocalEntityArray()[i];
			            minIndex = i;
			        }
			    }
			
			    // If no supply was found, break out
			    if (minIndex == -1)
			        break;
			
			    // Decide how much to subtract
			    int subtractAmount;
			    if (supplyNeeded < minSupply)
			    {
			        subtractAmount = supplyNeeded;
			        supplyNeeded = 0;
			    }
			    else
			    {
			        subtractAmount = minSupply;
			        supplyNeeded -= minSupply;
			    }
			
			    // Store results
			    supplyObjects.Insert(supplyObject);
			    supplyToSubtract.Insert(subtractAmount);
			
			    // Reduce the count of that supply item
			    supplyCounts[minIndex] = supplyCounts[minIndex] - subtractAmount;
			}
			
			foreach (IEntity supplyObject: supplyObjects)
			{
				supplyObjectRplId.Insert(RplComponent.Cast(supplyObject.FindComponent(RplComponent)).Id());
			}
		}
		
		IEntity truck = GetNearestVehicle();
		if (!truck)
			return;
		CRF_RplToAuthorityManager.GetInstance().AddItemToTruck(RplComponent.Cast(truck.FindComponent(RplComponent)).Id(), itemButton.m_sResource, m_wEditBox.GetText().ToInt(), supplyObjectRplId, supplyToSubtract, RplComponent.Cast(m_ArsenalPoint.FindComponent(RplComponent)).Id());
		AddNotification(TextWidget.Cast(itemButton.m_wButtonRoot.FindAnyWidget("ArsenalItemText")).GetText(), m_wEditBox.GetText().ToInt());
	}
	
	IEntity GetNearestVehicle()
	{
		GetGame().GetWorld().QueryEntitiesBySphere(m_ArsenalPoint.GetOrigin(), 50, FindTruckCallback, null);
		return m_ClosestTruck;
	}
	
	void NoSupplyNotification()
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{8DE299D2A550FAFB}UI/layouts/Menus/Arsenal/SupplyArsenalNotification.layout", m_Notifications);
		TextWidget.Cast(item.FindAnyWidget("ArsenalItemText")).SetText(string.Format("Not Enough Supply Nearby"));
		item.FindAnyWidget("Image0").SetColor(Color.FromInt(Color.RED));
		m_aNotifications.Insert(item);
	}
	
	void AddNotification(string name, int amount)
	{
		Widget item = GetGame().GetWorkspace().CreateWidgets("{8DE299D2A550FAFB}UI/layouts/Menus/Arsenal/SupplyArsenalNotification.layout", m_Notifications);
		TextWidget.Cast(item.FindAnyWidget("ArsenalItemText")).SetText(string.Format("x%1 %2 added", amount, name));
		m_aNotifications.Insert(item);
	}
	
	bool FindTruckCallback(IEntity entity)
	{
		if (Vehicle.Cast(entity))
		{
			if (!m_ClosestTruck)
			{
				m_ClosestTruck = entity;
				return true;
			}
				
			if (vector.Distance(m_ClosestTruck.GetOrigin(), m_ArsenalPoint.GetOrigin()) > vector.Distance(entity.GetOrigin(), m_ArsenalPoint.GetOrigin()))
				m_ClosestTruck = entity;
			
			return true;
		}
			
		return true;
	}
	
	override void OnMenuFocusLost()
	{
		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
		#endif
	}
}