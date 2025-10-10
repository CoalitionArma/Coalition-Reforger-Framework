modded enum ChimeraMenuPreset
{
	CRF_SightArsenal
}

class CRF_SightArsenal: ChimeraMenuBase
{
	InputManager m_InputManager;
	bool m_bFocused = true;
	
	CRF_GearscriptManager m_GearScriptManager;
	CRF_GearScriptContainer m_GearScriptContainer;
	ref CRF_GearScriptConfig m_GearScriptConfig;
	ref CRF_SightArsenalConfig m_SightArsenalConfig;
	ref CRF_SightArsenalConfig m_MagnifiedSightArsenalConfig;
	CRF_SafestartManager m_SafeStart;
	
	Widget m_wRoot;
	VerticalLayoutWidget m_SightSlots;
	Widget m_wSightFrame;
	
	IEntity m_Sight;
	AttachmentSlotComponent m_SightSlot;
	
	ref array<ResourceName> m_aAddedSights = {};
	
	override void OnMenuOpen()
	{
		m_InputManager = GetGame().GetInputManager();
		m_GearScriptManager = CRF_GearscriptManager.GetInstance();
		m_GearScriptContainer = m_GearScriptManager.GetGearScriptSettings(SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey());
		m_SafeStart = CRF_SafestartManager.GetInstance();
		ResourceName gearResource = m_GearScriptManager.GetGearScriptResource(SCR_FactionManager.SGetPlayerFaction(SCR_PlayerController.GetLocalPlayerId()).GetFactionKey());
		m_GearScriptConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearResource).GetResource().ToBaseContainer()));
		m_SightArsenalConfig = CRF_SightArsenalConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
		BaseContainerTools.LoadContainer(m_GearScriptContainer.m_rSightArsenal).GetResource().ToBaseContainer()));
		m_MagnifiedSightArsenalConfig = CRF_SightArsenalConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(
		BaseContainerTools.LoadContainer(m_GearScriptContainer.m_rMagnifiedSightArsenal).GetResource().ToBaseContainer()));
		m_wRoot = GetRootWidget();
		m_wSightFrame = m_wRoot.FindAnyWidget("SightFrame");
		m_SightSlots = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("SightArsenalList"));
		
		PopulateSights();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		if (!m_SafeStart.GetSafestartStatus())
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
	
	//Gets all the default attachments on the prefab itself and assigned in the GS.
	array<ResourceName> GetDefaultAttachments()
	{
		CRF_EGearRole role = CRF_RoleHelper.ResourceToRole(SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName());
		CRF_RoleConfig rolesConfig = CRF_GamemodeManager.RolesConfig().FindRoleConfig(role);
		array<ResourceName> attachmentsToAdd = {};
		foreach (CRF_EGearscriptWeapons weaponType : rolesConfig.m_aWeapons)
		{
			switch (weaponType)
			{
				case CRF_EGearscriptWeapons.RIFLE:
					if(m_GearScriptConfig.m_FactionWeapons.m_Rifle && !m_GearScriptConfig.m_FactionWeapons.m_Rifle.IsEmpty())
					{
						foreach (CRF_Weapon_Class rifle: m_GearScriptConfig.m_FactionWeapons.m_Rifle)
						{
							
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								attachmentsToAdd.Insert(attachment);
							}
						}
					}
					break;
				
				case CRF_EGearscriptWeapons.RIFLEUGL:
					if(m_GearScriptConfig.m_FactionWeapons.m_RifleUGL && !m_GearScriptConfig.m_FactionWeapons.m_RifleUGL.IsEmpty())
					{
						foreach (CRF_Weapon_Class rifle: m_GearScriptConfig.m_FactionWeapons.m_RifleUGL)
						{
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								attachmentsToAdd.Insert(attachment);
							}
						}
					};
					break;
				
				case CRF_EGearscriptWeapons.CARBINE:
					if(m_GearScriptConfig.m_FactionWeapons.m_Carbine && !m_GearScriptConfig.m_FactionWeapons.m_Carbine.IsEmpty())
					{
						foreach (CRF_Weapon_Class rifle: m_GearScriptConfig.m_FactionWeapons.m_Carbine)
						{
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								attachmentsToAdd.Insert(attachment);
							}
						}
					};
					break;

				case CRF_EGearscriptWeapons.PISTOL:
					if(m_GearScriptConfig.m_FactionWeapons.m_Pistol && !m_GearScriptConfig.m_FactionWeapons.m_Pistol.IsEmpty())
					{
						foreach (CRF_Weapon_Class rifle: m_GearScriptConfig.m_FactionWeapons.m_Pistol)
						{
							ResourceName sightAttachment = GetPrefabsDefaultSight(rifle.m_Weapon);
							if (sightAttachment != "")
								attachmentsToAdd.Insert(sightAttachment);
							foreach (ResourceName attachment: rifle.m_Attachments)
							{
								attachmentsToAdd.Insert(attachment);
							}
						}
					};
					break;

				case CRF_EGearscriptWeapons.SNIPER:
					if(m_GearScriptConfig.m_FactionWeapons.m_Sniper)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_Sniper.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_Sniper.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;

				case CRF_EGearscriptWeapons.AR:
					if(m_GearScriptConfig.m_FactionWeapons.m_AR)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_AR.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_AR.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;

				case CRF_EGearscriptWeapons.MMG:
					if(m_GearScriptConfig.m_FactionWeapons.m_MMG)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_MMG.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_MMG.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;

				case CRF_EGearscriptWeapons.AT:
					if(m_GearScriptConfig.m_FactionWeapons.m_AT)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_AT.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_AT.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;
	
				case CRF_EGearscriptWeapons.MAT:
					if(m_GearScriptConfig.m_FactionWeapons.m_MAT)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_MAT.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_MAT.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;
	
				case CRF_EGearscriptWeapons.HAT:
					if(m_GearScriptConfig.m_FactionWeapons.m_HAT)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_HAT.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_HAT.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;

				case CRF_EGearscriptWeapons.AA:
					if(m_GearScriptConfig.m_FactionWeapons.m_AA)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_AA.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_AA.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
					}
					break;

				case CRF_EGearscriptWeapons.HMG:
					if(m_GearScriptConfig.m_FactionWeapons.m_HMG)
					{
						ResourceName sightAttachment = GetPrefabsDefaultSight(m_GearScriptConfig.m_FactionWeapons.m_HMG.m_Weapon);
						if (sightAttachment != "")
							attachmentsToAdd.Insert(sightAttachment);
						foreach (ResourceName attachment: m_GearScriptConfig.m_FactionWeapons.m_HMG.m_Attachments)
						{
							attachmentsToAdd.Insert(attachment);
						}
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
		
		array<IEntityComponentSource> collimeters = {};
		array<IEntityComponentSource> magnifiers = {};
		for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
		{
	        IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
			bool collimeter = componentSource.GetClassName().ToType().IsInherited(SCR_CollimatorSightsComponent);
			bool magnified = componentSource.GetClassName().ToType().IsInherited(SCR_2DOpticsComponent);
	        if(!collimeter && !magnified)
	        	continue;
			else
			{
				if (collimeter)
					return true;

				if (magnified)
					return false;
			}
		}
		
		return false;
	}
	
	void SelectSight(SCR_ButtonBaseComponent button)
	{
		CRF_SightArsenalItemButton itemButton = CRF_SightArsenalItemButton.Cast(button);
		CRF_RplToAuthorityManager.GetInstance().SightArsenalRequestNewSight(SCR_PlayerController.GetLocalPlayerId(), itemButton.m_sResource, itemButton.m_sType);
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