modded enum ChimeraMenuPreset
{
	CRF_MiniArsenal
}

class CRF_MiniArsenal: ChimeraMenuBase
{
	protected InputManager m_InputManager;
	protected bool m_bFocused = true;
	CameraBase m_Camera;
	CameraBase m_OldCamera;
	CRF_GearscriptManager m_GearscriptManager;
	ref CRF_GearScriptConfig m_GearScriptConfig;
	
	Widget m_wRoot;
	VerticalLayoutWidget m_Categories;
	VerticalLayoutWidget m_Items;
	
	override void OnMenuOpen()
	{
		m_wRoot = GetRootWidget();
		m_InputManager = GetGame().GetInputManager();
		SpawnCameraFacingPlayer();
		m_GearscriptManager = CRF_GearscriptManager.GetInstance();
		m_Categories = VerticalLayoutWidget.Cast(m_wRoot.FindWidget("CategoryButtons"));
		m_Items = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("ItemButtons"));
		
		ResourceName gearResource = m_GearscriptManager.GetGearScriptResource(SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey());
		m_GearScriptConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearResource).GetResource().ToBaseContainer()));
		PopulateCategories();
		
		SelectCategory(SCR_ButtonBaseComponent.Cast(m_Categories.GetChildren().FindHandler(SCR_ButtonBaseComponent)));
	}
	
	void PopulateCategories()
	{
		for (int i = 0; i < 18; i++)
		{
			foreach (CRF_Clothing clothing: m_GearScriptConfig.m_DefaultFactionGear.m_DefaultClothing)
			{
				if (clothing.m_iClothingType != i)
					continue;
				
				Widget category = GetGame().GetWorkspace().CreateWidgets("{BC371ACC7C58B63E}UI/layouts/Menus/Arsenal/MiniArsenalCategory.layout", m_Categories);
				ImageWidget categoryIcon = ImageWidget.Cast(category.FindWidget("CategoryIcon"));
				categoryIcon.LoadImageTexture(0, GetCategoryIcon(i));
				categoryIcon.SetImage(0);
				CRF_MiniArsenalCategoryButton button = CRF_MiniArsenalCategoryButton.Cast(category.FindHandler(CRF_MiniArsenalCategoryButton));
				button.m_iCategoryIndex = i;
				button.m_OnClicked.Insert(SelectCategory);
				break;
			}
		}
	}
	
	void SelectCategory(SCR_ButtonBaseComponent button)
	{
		while (m_Items.GetChildren())
			delete m_Items.GetChildren();
		
		CRF_MiniArsenalCategoryButton miniArsnealCategory = CRF_MiniArsenalCategoryButton.Cast(button);
		array<string> m_addedItems = {};
		foreach (CRF_Clothing clothing: m_GearScriptConfig.m_DefaultFactionGear.m_DefaultClothing)
		{
			if (clothing.m_iClothingType != miniArsnealCategory.m_iCategoryIndex)
				continue;
			
			ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
			if (!manager)
				return;
			foreach(ResourceName cloth: clothing.m_ClothingPrefabs)
			{
				if (m_addedItems.Contains(cloth))
					continue;
				
				m_addedItems.Insert(cloth);
				Widget item = GetGame().GetWorkspace().CreateWidgets("{2B983EDBF688480D}UI/layouts/Menus/Arsenal/MiniArsenalItem.layout", m_Items);
				ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
				manager.SetPreviewItemFromPrefab(itemPreview, cloth);
				//Thank you random BI Forum from Arkensor
				Resource loadedCloth = Resource.Load(cloth);
				IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedCloth);
				if (entitySource)
				{
				    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
				    {
				        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
				        if(componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
				        {
				            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
				            if (attributesContainer)
				            {
				                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
				                if (itemDisplayNameContainer)
				                {
				                    string name
				                    itemDisplayNameContainer.Get("Name", name);
				
				                    TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(name);
				                    break;
				                }
				            }
				        }
				    }
				}
				
				CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
				itemButton.m_sResource = cloth;
				itemButton.m_iSlotId = miniArsnealCategory.m_iCategoryIndex;
				itemButton.m_OnClicked.Insert(SelectItem);
			}
		}
		
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName());
		
		foreach (CRF_Role_Custom_Gear customGear: m_GearScriptConfig.m_CustomFactionGear.m_RolesToSetCustomGear)
		{
			if (customGear.m_Role != role)
				continue;
			
			foreach (CRF_Clothing clothing: customGear.m_Clothing)
			{
				if (clothing.m_iClothingType != miniArsnealCategory.m_iCategoryIndex)
					continue;
				
				ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
				if (!manager)
					return;
				
				foreach(ResourceName cloth: clothing.m_ClothingPrefabs)
				{
					if (m_addedItems.Contains(cloth))
						continue;
					
					m_addedItems.Insert(cloth);
					Widget item = GetGame().GetWorkspace().CreateWidgets("{2B983EDBF688480D}UI/layouts/Menus/Arsenal/MiniArsenalItem.layout", m_Items);
					ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(item.FindWidget("ArsenalItemPreview"));
					manager.SetPreviewItemFromPrefab(itemPreview, cloth);
					//Thank you random BI Forum from Arkensor
					Resource loadedCloth = Resource.Load(cloth);
					IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedCloth);
					if (entitySource)
					{
					    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
					    {
					        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
					        if(componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
					        {
					            BaseContainer attributesContainer = componentSource.GetObject("Attributes");
					            if (attributesContainer)
					            {
					                BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
					                if (itemDisplayNameContainer)
					                {
					                    string name
					                    itemDisplayNameContainer.Get("Name", name);
					
					                    TextWidget.Cast(item.FindWidget("ArsenalItemText")).SetText(name);
					                    break;
					                }
					            }
					        }
					    }
					}
					
					CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(item.FindWidget("ArsenalItemButton").FindHandler(CRF_MiniArsenalItemButton));
					itemButton.m_sResource = cloth;
					itemButton.m_iSlotId = miniArsnealCategory.m_iCategoryIndex;
					itemButton.m_OnClicked.Insert(SelectItem);
				}
			}
		}
	}
	
	void SelectItem(SCR_ButtonBaseComponent button)
	{
		CRF_MiniArsenalItemButton itemButton = CRF_MiniArsenalItemButton.Cast(button);
		CRF_RplToAuthorityManager.GetInstance().MiniArsenalRequestNewItem(SCR_PlayerController.GetLocalPlayerId(), itemButton.m_sResource, itemButton.m_iSlotId);
	}
	
	override void OnMenuClose()
	{
		delete m_Camera;
		GetGame().GetCameraManager().SetCamera(m_OldCamera);
	}
	
	ResourceName GetCategoryIcon(int index)
	{
		switch (index)
		{
			case 0: return "{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds"; break;
			case 1: return "{92245C15E122EDB2}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Jackets.edds"; break;
			case 2: return "{2DCA69EEB8628C06}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Vests.edds"; break;
			case 3: return "{2CD4D1D2199CE475}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Trousers.edds"; break;
			case 4: return "{AC6095E11A1E4144}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Footware.edds"; break;
			case 5: return "{769B709DF200BF84}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Backpacks.edds"; break;
			case 6: return "{2DCA69EEB8628C06}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Vests.edds"; break;
			case 7: return "{A785C2D354BC7382}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Handwear.edds"; break;
			case 8: return "{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds"; break;
			case 9: return "{077B2AB761904B92}UI/textures/inventory/glasses.edds"; break;
			case 10: return "{FA22678B4CA2F5F6}UI/textures/inventory/headphones.edds"; break;
			case 11: return "{168CCC01315CE526}UI/textures/inventory/mask.edds"; break;
			case 12: return "{44CA33D6376563EE}UI/textures/inventory/neck.edds"; break;
			case 13: return "{140A80B0B3DCF2B5}UI/textures/inventory/extra.edds"; break;
			case 14: return "{140A80B0B3DCF2B5}UI/textures/inventory/extra.edds"; break;
			case 15: return "{67FBAF5873940829}UI/textures/inventory/belt.edds"; break;
			case 16: return "{140A80B0B3DCF2B5}UI/textures/inventory/extra.edds"; break;
			case 17: return "{140A80B0B3DCF2B5}UI/textures/inventory/extra.edds"; break;
		}
		
		return "{F349167C49E996FB}UI/Textures/Editor/Attributes/Arsenal/Attribute_Arsenal_Headwear.edds";
	}
	
	void SpawnCameraFacingPlayer()
	{
		IEntity player = SCR_PlayerController.GetLocalControlledEntity();
		if (!player)
			return;

		vector mat[4];
		player.GetWorldTransform(mat);
		vector playerPos = mat[3];
		vector forward = mat[2];
		vector camPos = playerPos + forward * 2.0;
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
		camMat[3] = camPos;
		camMat[3][1] = camMat[3][1] + 0.75;
		
		cam.SetWorldTransform(camMat);
		m_Camera = CameraBase.Cast(cam);
		m_OldCamera = GetGame().GetCameraManager().CurrentCamera();
		GetGame().GetCameraManager().SetCamera(m_Camera);
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