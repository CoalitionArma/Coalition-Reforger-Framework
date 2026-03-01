class CRF_WeaponHelper
{	
	const static ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	//-----------------------------------------------------
	//- WEAPONS
	//-----------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Check if an entity is a throwable weapon
	 * @param entity Entity to check
	 * @return True if entity is a throwable weapon
	 */
	static bool IsThrowableWeapon(IEntity entity)
	{
		WeaponComponent weaponComp = WeaponComponent.Cast(entity.FindComponent(WeaponComponent));
		return weaponComp && WEAPON_TYPES_THROWABLE.Contains(weaponComp.GetWeaponType());
	}	
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Select a random weapon from an array
	 * @param weaponArray Array of weapon options
	 * @return Randomly selected weapon or null if array is empty
	 */
	static CRF_Weapon_Class SelectRandomWeapon(array<ref CRF_Weapon_Class> weaponArray)
	{
		if (!weaponArray || weaponArray.IsEmpty())
			return null;

		return weaponArray.GetRandomElement();
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Find a weapon by its resource name
	 * @param weaponManager Weapon manager component
	 * @param weaponResource Weapon resource to find
	 * @return Found weapon entity or null
	 */
	static IEntity FindWeaponByResource(BaseWeaponManagerComponent weaponManager, ResourceName weaponResource)
	{
		array<IEntity> outWeapons = {};
		weaponManager.GetWeaponsList(outWeapons);

		foreach (IEntity weaponToCheck : outWeapons)
		{
			if (weaponToCheck.GetPrefabData().GetPrefabName() == weaponResource)
				return weaponToCheck;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Spawn a weapon and its attachments
	 * @param weaponResource Weapon resource to spawn
	 * @param attachmentResources Attachments to add
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	static void SpawnWeapon(ResourceName weaponResource, array<ResourceName> attachmentResources, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if(weaponResource.IsEmpty())
			return;
		
		bool successfulSpawn = inventoryManager.TrySpawnPrefabToStorage(weaponResource, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);

		if (!successfulSpawn)
		{
			CRF_LoggingHelper.LogItemError(null, inventoryManager.GetOwner(), "WEAPON");
			return;
		}
		
		// Add attachments after a delay to ensure weapon is fully initialized
		GetGame().GetCallqueue().CallLater(AddAttachments, 1000, false, weaponResource, attachmentResources, spawnParams, inventoryManager);
		GetGame().GetCallqueue().CallLater(SelectWeapon, 500, false, inventory.GetOwner()); 
	}
	
	//------------------------------------------------------------------------------------------------
	static void SelectWeapon(IEntity entity)
	{
		if (!ChimeraCharacter.Cast(entity))
			return;
		
		BaseWeaponManagerComponent weaponMan = ChimeraCharacter.Cast(entity).GetWeaponManager();
		if (!weaponMan)
			return;
		
		CharacterControllerComponent charController = ChimeraCharacter.Cast(entity).GetCharacterController();
		if (!charController)
			return;
		
		array<WeaponSlotComponent> outSlots = {};
		weaponMan.GetWeaponsSlots(outSlots);
		WeaponSlotComponent weapon;
		foreach (WeaponSlotComponent outSlot: outSlots)
		{
			if (!outSlot.GetWeaponEntity())
				continue;
			
			if (outSlot.GetWeaponEntity().FindComponent(GrenadeMoveComponent))
				continue;
			
			weapon = outSlot;
			break;
		}
		
		if (!weapon)
			return;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
			SCR_ChimeraCharacter.Cast(entity).SelectPrimaryWeapon();
		else
			charController.SelectWeapon(weapon);
	}
	
	//-----------------------------------------------------
	//- ATTACHMENTS
	//-----------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Add attachments to a weapon
	 * @param weaponResource Weapon resource
	 * @param attachmentResources Attachments to add
	 * @param spawnParams Spawn parameters
	 * @param inventoryManager Inventory manager component
	 */
	static void AddAttachments(ResourceName weaponResource, array<ResourceName> attachmentResources, 
		EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!inventoryManager || !attachmentResources || attachmentResources.IsEmpty())
			return;
			
		ChimeraCharacter character = ChimeraCharacter.Cast(inventoryManager.GetOwner());
		if (!character)
			return;
			
		BaseWeaponManagerComponent weaponManager = character.GetCharacterController().GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		// Find the weapon
		IEntity weapon = FindWeaponByResource(weaponManager, weaponResource);
		if (!weapon)
			return;

		array<AttachmentSlotComponent> attachmentSlotArray = {};
		BaseWeaponComponent.Cast(weapon.FindComponent(BaseWeaponComponent)).GetAttachments(attachmentSlotArray);

		// Add each attachment
		foreach (ResourceName attachment : attachmentResources)
			AddAttachmentToWeapon(attachment, weapon, attachmentSlotArray, spawnParams, inventoryManager);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Add a single attachment to a weapon
	 * @param attachmentResource Attachment resource to add
	 * @param weapon Weapon to add attachment to
	 * @param attachmentSlots Available attachment slots
	 * @param spawnParams Spawn parameters
	 * @param inventoryManager Inventory manager component
	 */
	static void AddAttachmentToWeapon(ResourceName attachmentResource, IEntity weapon, array<AttachmentSlotComponent> attachmentSlots, 
		EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		AttachmentSlotComponent verifyAttachmentSlot = null;
		
		if (!Resource.Load(attachmentResource).IsValid())
			return;
		
		IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachmentResource), GetGame().GetWorld(), spawnParams);
		BaseInventoryStorageComponent weaponStorageComp = BaseInventoryStorageComponent.Cast(weapon.FindComponent(BaseInventoryStorageComponent));
		if (!weaponStorageComp)
			return;

		IEntity oldSight = weaponStorageComp.FindSuitableSlotForItem(attachmentSpawned).GetAttachedEntity();
		
		foreach (AttachmentSlotComponent attachmentSlot : attachmentSlots)
		{
			if (attachmentSlot.CanSetAttachment(attachmentSpawned))
			{
				if (oldSight)
				delete oldSight;
			
				inventoryManager.TryInsertItemInStorage(attachmentSpawned, weaponStorageComp);
				verifyAttachmentSlot = attachmentSlot;
				break;
			}
		}

		if (verifyAttachmentSlot == null)
		{
			CRF_LoggingHelper.LogItemError(attachmentSpawned, weapon, "ATTACHMENT");
			delete attachmentSpawned;
		}
	}
	
	//-----------------------------------------------------
	//- MAGAZINES
	//-----------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief add a weapons magazines
	 * @param magazineArray Magazines to add
	 * @param spawnParams Spawn parameters
	 * @param inventory Inventory component
	 * @param inventoryManager Inventory manager component
	 */
	static void AddMagazines(array<ref CRF_Magazine_Class> magazineArray, EntitySpawnParams spawnParams, 
		SCR_CharacterInventoryStorageComponent inventory, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		// Add magazines
		if (magazineArray != null)
		{
			foreach (CRF_Magazine_Class magazine : magazineArray)
			{
				if (magazine != null)
				{
					CRF_InventoryHelper.AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount, spawnParams, inventory, inventoryManager);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Find the appropriate magazine array for the weapon type based off the slected weapons in ApplyDefaultWeapons
	 * @param weaponsSelected Weapons we selected when initilizing the role
	 * @param weaponType the weapon array we are comparing it to 
	 */
	static array<ref CRF_Magazine_Class> FindMagArrayForWeaponsSelected(array<CRF_Weapon_Class> weaponsSelected, array<ref CRF_Weapon_Class> weaponType, out CRF_Weapon_Class selectedWeapon)
	{	
		foreach (CRF_Weapon_Class weaponSelected : weaponsSelected)
		{
			if (weaponType.Contains(weaponSelected))
			{
				foreach (CRF_Weapon_Class weaponToCompare : weaponType)
				{
					if (weaponToCompare == weaponSelected)
						return weaponSelected.m_MagazineArray;
					
					selectedWeapon = weaponSelected;
				}
			};
		}
		
		return new array<ref CRF_Magazine_Class>; 
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * @brief Convert specialized magazine array to standard magazine array
	 * @param specMagazineArray Array of specialized magazines
	 * @return Converted magazine array
	 */
	static array<ref CRF_Magazine_Class> ConvertSpecMagArrayIntoMagArray(array<ref CRF_Spec_Magazine_Class> specMagazineArray, bool isAssistant)
	{
		array<ref CRF_Magazine_Class> tempArray = {};
		
		if (!specMagazineArray)
			return tempArray;
			
		foreach (CRF_Spec_Magazine_Class specMagazine : specMagazineArray)
		{
			if (!specMagazine)
				continue;
				
			ref CRF_Magazine_Class tempMag = new CRF_Magazine_Class();
			tempMag.m_Magazine = specMagazine.m_Magazine;
			
			if (isAssistant)
				tempMag.m_MagazineCount = specMagazine.m_AssistantMagazineCount;
			else
				tempMag.m_MagazineCount = specMagazine.m_MagazineCount;
			
			tempArray.Insert(tempMag);
		}
		
		return tempArray;
	}

}