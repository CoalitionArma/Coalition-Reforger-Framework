modded enum ChimeraMenuPreset
{
	CRF_SightArsenal
}

class CRF_SightArsenal: ChimeraMenuBase
{
	InputManager m_InputManager;
	bool m_bFocused = true;
	
	COA_Gamemode m_Gamemode;
	COA_GearScriptContainer m_GearScriptContainer;
	ref COA_GearScriptConfig m_GearScriptConfig;
	ref COA_SightArsenalConfig m_SightArsenalConfig;
	ref COA_SightArsenalConfig m_MagnifiedSightArsenalConfig;
	COA_SafestartManager m_SafeStart;
	
	Widget m_wRoot;
	VerticalLayoutWidget m_SightSlots;
	Widget m_wSightFrame;
	
	IEntity m_Sight;
	AttachmentSlotComponent m_SightSlot;
	
	ref array<ResourceName> m_aAddedSights = {};
	
	override void OnMenuOpen()
	{
		m_InputManager = GetGame().GetInputManager();
		m_Gamemode = COA_Gamemode.GetInstance();
		m_GearScriptContainer = m_Gamemode.GetGearScriptSettings(SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey());
		m_SafeStart = COA_SafestartManager.GetInstance();
		ResourceName gearResource = m_Gamemode.GetGearScriptResource(SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey());
		m_GearScriptConfig = COA_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearResource).GetResource().ToBaseContainer()));
		m_SightArsenalConfig = COA_SightArsenalConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
		BaseContainerTools.LoadContainer(m_GearScriptContainer.m_rSightArsenal).GetResource().ToBaseContainer()));
		m_MagnifiedSightArsenalConfig = COA_SightArsenalConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
		BaseContainerTools.LoadContainer(m_GearScriptContainer.m_rMagnifiedSightArsenal).GetResource().ToBaseContainer()));
		m_wRoot = GetRootWidget();
		m_wSightFrame = m_wRoot.FindAnyWidget("SightFrame");
		m_SightSlots = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("SightArsenalList"));
		
		PopulateSights();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (!m_SafeStart.GetSafestartStatus() && COA_PlayerController.IsGracePeriodOver())
			Close();
		if (!m_SightSlot)
		{
			if (!m_Sight)
				return;
			IEntity weapon = m_Sight.GetParent();
			array<Managed> attachmentComponents = {};
			weapon.FindComponents(AttachmentSlotComponent, attachmentComponents);
			foreach (Managed component: attachmentComponents)
			{
				AttachmentSlotComponent slotComponent = AttachmentSlotComponent.Cast(component);
				if (!slotComponent.GetAttachedEntity())
					continue;
				
				if (slotComponent.GetAttachedEntity() == m_Sight)
					m_SightSlot = slotComponent;
			}
		}
		m_Sight = m_SightSlot.GetAttachedEntity();
		if (!m_Sight)
			return;
	    vector origin  = m_Sight.GetOrigin();
	    vector forward = m_Sight.GetWorldTransformAxis(2);
	
	    // anchor point 30 cm forward
	    vector worldPos = origin + forward * 0.3;
	
	    // project to screen
	    vector screenPos = GetGame().GetWorkspace().ProjWorldToScreenNative(worldPos, GetGame().GetWorld());
	
	    float uiX = screenPos[0];
	    float uiY = screenPos[1];
		
		uiX += GetGame().GetWorkspace().DPIScale(-300);
		uiY += GetGame().GetWorkspace().DPIScale(-350);
	
	    FrameSlot.SetPos(m_wSightFrame, uiX, uiY);
		m_wSightFrame.SetVisible(true);
	}
	
	void PopulateSights()
	{
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent));
		array<AttachmentSlotComponent> attachments = {};
		charController.GetWeaponManagerComponent().GetCurrentWeapon().GetAttachments(attachments);
		ref array<ref BaseAttachmentType> attachmentTypes = {};
		bool magnified = m_GearScriptContainer.m_bEnableMagnifiedOptics;
		foreach (AttachmentSlotComponent attachment: attachments)
		{
			attachmentTypes.Insert(attachment.GetAttachmentSlotType());
		}
		
		array<ResourceName> defaultAttachments = GetDefaultAttachments();
		if (defaultAttachments.Count() > 0)
			PopulateSightItems(attachmentTypes, defaultAttachments);
		
		if (magnified)
			PopulateSightItems(attachmentTypes, m_MagnifiedSightArsenalConfig.m_aSights);
		PopulateSightItems(attachmentTypes, m_SightArsenalConfig.m_aSights);
	}
	
	bool IsSight(ResourceName resource)
	{
		Resource loaded = Resource.Load(resource);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loaded);
		if (!entitySource)
			return false;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
	    {
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
	        if(componentSource.GetClassName().ToType().IsInherited(SCR_2DOpticsComponent) || componentSource.GetClassName().ToType().IsInherited(SCR_CollimatorSightsComponent))
		        return true;
		}
		
		return false;
	}
	
	//Gets all the default attachments on the prefab itself and assigned in the GS.
	array<ResourceName> GetDefaultAttachments()
	{
		COA_EGearRole role = COA_RoleHelper.ResourceToRole(SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName());
		COA_RoleConfig rolesConfig = COA_GearscriptManager.GetRolesConfig().FindRoleConfig(role);
		array<ResourceName> attachmentsToAdd = {};
		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent));
		string currentWeapon = charController.GetWeaponManagerComponent().GetCurrentWeapon().GetOwner().GetPrefabData().GetPrefabName();
		foreach (COA_EGearscriptWeapons weaponType : rolesConfig.m_aWeapons)
		{
			switch (weaponType)
			{
				case COA_EGearscriptWeapons.RIFLE:
					if(m_GearScriptConfig.m_Rifles && !m_GearScriptConfig.m_Rifles.IsEmpty())
					{
						foreach (COA_Weapon_Class rifle: m_GearScriptConfig.m_Rifles)
						{
							if (currentWeapon != rifle.m_Weapon)
								continue;
							bool overrideAttachment = false;
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								if (IsSight(attachment))
									overrideAttachment = true;
								attachmentsToAdd.Insert(attachment);
							}
							
							if (overrideAttachment)
								break;
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
							
						}
					}
					break;
				
				case COA_EGearscriptWeapons.RIFLEUGL:
					if(m_GearScriptConfig.m_RifleUGLs && !m_GearScriptConfig.m_RifleUGLs.IsEmpty())
					{
						foreach (COA_Weapon_Class rifle: m_GearScriptConfig.m_RifleUGLs)
						{
							if (currentWeapon != rifle.m_Weapon)
								continue;
							bool overrideAttachment = false;
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								if (IsSight(attachment))
									overrideAttachment = true;
								attachmentsToAdd.Insert(attachment);
							}
							
							if (overrideAttachment)
								break;
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
						}
					};
					break;
				
				case COA_EGearscriptWeapons.CARBINE:
					if(m_GearScriptConfig.m_Carbines && !m_GearScriptConfig.m_Carbines.IsEmpty())
					{
						foreach (COA_Weapon_Class rifle: m_GearScriptConfig.m_Carbines)
						{
							if (currentWeapon != rifle.m_Weapon)
								continue;
							bool overrideAttachment = false;
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								if (IsSight(attachment))
									overrideAttachment = true;
								attachmentsToAdd.Insert(attachment);
							}
							
							if (overrideAttachment)
								break;
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
						}
					};
					break;

				case COA_EGearscriptWeapons.PISTOL:
					if(m_GearScriptConfig.m_Pistols && !m_GearScriptConfig.m_Pistols.IsEmpty())
					{
						
						foreach (COA_Weapon_Class rifle: m_GearScriptConfig.m_Pistols)
						{
							if (currentWeapon != rifle.m_Weapon)
								continue;
							bool overrideAttachment = false;
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								if (IsSight(attachment))
									overrideAttachment = true;
								attachmentsToAdd.Insert(attachment);
							}
							
							if (overrideAttachment)
								break;
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
						}
					};
					break;

				case COA_EGearscriptWeapons.SNIPER:
					if(m_GearScriptConfig.m_SNIPER)
					{
						if (currentWeapon != m_GearScriptConfig.m_SNIPER.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_SNIPER.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_SNIPER.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;

				case COA_EGearscriptWeapons.AR:
					if(m_GearScriptConfig.m_AR)
					{
						if (currentWeapon != m_GearScriptConfig.m_AR.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_AR.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_AR.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;

				case COA_EGearscriptWeapons.MMG:
					if(m_GearScriptConfig.m_MMG)
					{
						if (currentWeapon != m_GearScriptConfig.m_MMG.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_MMG.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_MMG.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;

				case COA_EGearscriptWeapons.AT:
					if(m_GearScriptConfig.m_AT)
					{
						if (currentWeapon != m_GearScriptConfig.m_AT.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_AT.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_AT.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;
	
				case COA_EGearscriptWeapons.MAT:
					if(m_GearScriptConfig.m_MAT)
					{
						if (currentWeapon != m_GearScriptConfig.m_MAT.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_MAT.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_MAT.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;
	
				case COA_EGearscriptWeapons.HAT:
					if(m_GearScriptConfig.m_HAT)
					{
						if (currentWeapon != m_GearScriptConfig.m_HAT.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_HAT.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_HAT.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;

				case COA_EGearscriptWeapons.AA:
					if(m_GearScriptConfig.m_AA)
					{
						if (currentWeapon != m_GearScriptConfig.m_AA.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_AA.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_AA.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;

				case COA_EGearscriptWeapons.HMG:
					if(m_GearScriptConfig.m_HMG)
					{
						if (currentWeapon != m_GearScriptConfig.m_HMG.m_Weapon)
								continue;
						bool overrideAttachment = false;
						foreach (ResourceName attachment: m_GearScriptConfig.m_HMG.m_Attachments)
						{
							if (IsSight(attachment))
								overrideAttachment = true;
							attachmentsToAdd.Insert(attachment);
						}
						
						if (overrideAttachment)
							break;
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_HMG.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
					}
					break;
			}
		}
		
		return attachmentsToAdd;
	}
	
	ResourceName GetPrefabsDefaultSight(ResourceName prefab)
	{
		Resource loadedPrefab = Resource.Load(prefab);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(Resource.Load(prefab));
		if (entitySource)
		{
		    for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		    {
		        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
		        if(componentSource.GetClassName().ToType().IsInherited(WeaponComponent))
		        {
					BaseContainerList attachmentComponents = componentSource.GetObjectArray("components");
					for (int i = 0; i < attachmentComponents.Count(); i++)
					{
						IEntityComponentSource attachmentComponent = attachmentComponents.Get(i);
						if (attachmentComponent)
						{
							BaseAttachmentType type;
							attachmentComponent.Get("AttachmentType", type);
							if (!type)
								continue;
							if (type.Type().IsInherited(AttachmentOptics))
							{
								BaseContainer attachmentSlot = attachmentComponent.GetObject("AttachmentSlot");
								if (attachmentSlot)
								{
									ResourceName attachment;
									attachmentSlot.Get("Prefab", attachment);
									if (attachment)
										return attachment;
									else
										return "";
								}	
							}
						}
					}
				}
			}
		}
		return "";
	}
	
	
	//Used to populate the clickable items
	void PopulateSightItems(array<ref BaseAttachmentType> attachmentTypes, array<ResourceName> sights)
	{
		foreach (ResourceName sight: sights)
		{
			if (m_aAddedSights.Contains(sight))
				continue;
			
			bool isValid = IsSightValid(sight);
			if (!isValid)
				continue;
			
			m_aAddedSights.Insert(sight);
			Resource loadedSight = Resource.Load(sight);
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(loadedSight);
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
							BaseContainerList itemAttributes = attributesContainer.GetObjectArray("CustomAttributes");
							for (int i = 0; i < itemAttributes.Count(); i++)
							{
								if (itemAttributes.Get(i).GetClassName().ToType().IsInherited(WeaponAttachmentAttributes) 
								&& !itemAttributes.Get(i).GetClassName().ToType().IsInherited(SCR_WeaponAttachmentObstructionAttributes))
								{

									BaseAttachmentType type;
									itemAttributes.Get(i).Get("AttachmentType", type);
									foreach (BaseAttachmentType attachmentType: attachmentTypes)
									{
										if (!attachmentType)
											continue;
										if (type.Type().IsInherited(attachmentType.Type()))
										{
											BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
											string name;
							                if (itemDisplayNameContainer) 
							                    itemDisplayNameContainer.Get("Name", name);
											Widget attachmentItem = GetGame().GetWorkspace().CreateWidgets("{0F2ABBB04106C724}UI/layouts/Menus/Arsenal/SightArsenalItem.layout", m_SightSlots);
											TextWidget.Cast(attachmentItem.FindWidget("SightArsenalItemText")).SetText(name);
											
											ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
											if (!manager)
												break;
											
											ItemPreviewWidget itemPreview = ItemPreviewWidget.Cast(attachmentItem.FindWidget("SightArsenalItemPreview"));
											manager.SetPreviewItemFromPrefab(itemPreview, sight);
											
											CRF_SightArsenalItemButton itemButton = CRF_SightArsenalItemButton.Cast(attachmentItem.FindWidget("SightArsenalItemButton").FindHandler(CRF_SightArsenalItemButton));
											itemButton.m_sResource = sight;
											itemButton.m_sType = type.Type().ToString();
											itemButton.m_OnClicked.Insert(SelectSight);
											break;
										}
									}
								}
							}
						}
			        }
			    }
			}
		}
	}
	
	bool IsSightValid(ResourceName sight)
	{
		Resource Sight = Resource.Load(sight);
		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(Sight);
		if (!entitySource)
			return false;
		
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		{
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
			bool collimeter = componentSource.GetClassName().ToType().IsInherited(SCR_CollimatorSightsComponent);
			bool magnified = componentSource.GetClassName().ToType().IsInherited(SCR_2DOpticsComponent);
	        if(!collimeter && !magnified)
	        	continue;
			else
				return true;
		}
		
		return false;
	}
	
	void SelectSight(SCR_ButtonBaseComponent button)
	{
		CRF_SightArsenalItemButton itemButton = CRF_SightArsenalItemButton.Cast(button);
		COA_PlayerRplToAuthorityManager.GetInstance().SightArsenalRequestNewSight(SCR_PlayerController.GetLocalPlayerId(), itemButton.m_sResource, itemButton.m_sType);
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