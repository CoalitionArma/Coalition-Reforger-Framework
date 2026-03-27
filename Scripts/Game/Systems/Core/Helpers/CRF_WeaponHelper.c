class CRF_WeaponHelper
{	
	//------------------------------------------------------------------------------------------------
	//! Select a random weapon from an array
	//! \param[in] weaponArray Array of weapon options
	//! \return Randomly selected weapon or null if array is empty
	static CRF_Weapon_Class SelectRandomWeapon(array<ref CRF_Weapon_Class> weaponArray)
	{
		if (!weaponArray || weaponArray.IsEmpty())
			return null;

		return weaponArray.GetRandomElement();
	}

	//------------------------------------------------------------------------------------------------
	//! Find a weapon by its resource name
	//! \param[in] weaponManager Weapon manager component
	//! \param[in] weaponResource Weapon resource to find
	//! \return Found weapon entity or null
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
	//! Spawn a weapon and its attachments
	//! \param[in] weaponResource Weapon resource to spawn
	//! \param[in] attachmentResources Attachments to add
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventory Inventory component
	//! \param[in] inventoryManager Inventory manager component
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
		GetGame().GetCallqueue().Call(AddAttachments, weaponResource, attachmentResources, spawnParams, inventoryManager);
		GetGame().GetCallqueue().Call(SelectWeapon, inventory.GetOwner()); 
	}
	
	//------------------------------------------------------------------------------------------------
	//! Have the character or player actually select and hold the weapon
	//! \param[in] entity Entity to force weapon selection on
	static void SelectWeapon(IEntity entity)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		if (playerId > 0)
		{
			// For player-controlled entities, use RPC to execute on client
			CRF_GearscriptCharacter.Cast(entity).SelectPrimaryWeapon();
		}
		else
		{
			// For AI, select directly
			SelectPrimaryWeaponForEntity(entity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Shared weapon selection logic - finds and selects the primary weapon
	//! Prioritizes: Primary weapon (rifle with magazine) > Fallback weapon (sidearm) > Skip explosives/grenades
	//! \param[in] entity Entity to select weapon for
	static void SelectPrimaryWeaponForEntity(IEntity entity)
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
		WeaponSlotComponent primaryWeapon;
		WeaponSlotComponent fallbackWeapon;
		
		// First pass: Look for a primary weapon (rifle/gun with magazine)
		foreach (WeaponSlotComponent outSlot: outSlots)
		{
			if (!outSlot.GetWeaponEntity())
				continue;
			
			IEntity weaponEntity = outSlot.GetWeaponEntity();
			
			// Skip grenades
			if (weaponEntity.FindComponent(GrenadeMoveComponent))
				continue;
			
			// Skip explosives/demo charges (they have ExplosiveChargeComponent)
			if (weaponEntity.FindComponent(SCR_ExplosiveChargeComponent))
				continue;
			
			// Check if it's a firearm with magazine (primary weapon)
			BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(weaponEntity.FindComponent(BaseMuzzleComponent));
			if (muzzle)
			{
				BaseMagazineComponent magazine = muzzle.GetMagazine();
				if (magazine)
				{
					// Found a primary weapon with magazine
					primaryWeapon = outSlot;
					break;
				}
			}
			
			// Store as fallback if no primary found yet
			if (!fallbackWeapon)
				fallbackWeapon = outSlot;
		}
		
		// Use primary weapon if found, otherwise use fallback (sidearm, etc.)
		WeaponSlotComponent weapon = primaryWeapon;
		if (!weapon)
			weapon = fallbackWeapon;
		
		if (!weapon)
			return;

		charController.SelectWeapon(weapon);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Add attachments to a weapon
	//! \param[in] weaponResource Weapon resource
	//! \param[in] attachmentResources Attachments to add
	//! \param[in] spawnParams Spawn parameters
	//! \param[in] inventoryManager Inventory manager component
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
	//! Add a single attachment to a weapon
	//! \param[in] attachmentResource Attachment resource to add
	//! \param[in] weapon Weapon to add attachment to
	//! \param[in] attachmentSlots Available attachment slots
	//! \param[in] spawnParams Spawn parameters (unused - kept for compatibility)
	//! \param[in] inventoryManager Inventory manager component
	static void AddAttachmentToWeapon(ResourceName attachmentResource, IEntity weapon, array<AttachmentSlotComponent> attachmentSlots, 
		EntitySpawnParams spawnParams, SCR_InventoryStorageManagerComponent inventoryManager)
	{
		if (!Resource.Load(attachmentResource).IsValid())
			return;
		
		BaseInventoryStorageComponent weaponStorageComp = BaseInventoryStorageComponent.Cast(weapon.FindComponent(BaseInventoryStorageComponent));
		if (!weaponStorageComp)
			return;

		// Use TrySpawnPrefabToStorage to spawn attachment directly into the weapon's storage
		// This ensures the attachment slots are ready and handles replication properly
		bool spawned = inventoryManager.TrySpawnPrefabToStorage(
			attachmentResource,
			weaponStorageComp,  // Spawn directly into weapon storage
			-1,                 // Auto-select slot
			EStoragePurpose.PURPOSE_ANY
		);
		
		if (!spawned)
		{
			CRF_LoggingHelper.LogItemError(attachmentResource, weapon, "ATTATCHMENT");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Find the appropriate magazine array for the weapon type based off the slected weapons in ApplyDefaultWeapons
	//! \param[in] weaponsSelected Weapons we selected when initilizing the role
	//! \param[in] weaponType the weapon array we are comparing it to 
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
	//! Convert specialized magazine array to standard magazine array
	//! \param[in] specMagazineArray Array of specialized magazines
	//! \return Converted magazine array
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