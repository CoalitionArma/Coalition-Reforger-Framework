[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GearScriptGamemodeComponentClass: SCR_BaseGameModeComponentClass {}

class CRF_GearScriptGamemodeComponent: SCR_BaseGameModeComponent
{	
	[Attribute("false", UIWidgets.Auto, desc: "Is Gearscript Enabled")]
	protected bool m_bGearScriptEnabled;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all blufor players", "conf class=CRF_GearScriptConfig")]
	protected ResourceName m_rBluforGearScript;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all opfor players", "conf class=CRF_GearScriptConfig")]
	protected ResourceName m_rOpforGearScript;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all indfor players", "conf class=CRF_GearScriptConfig")]
	protected ResourceName m_rIndforGearScript;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all civ players", "conf class=CRF_GearScriptConfig")]
	protected ResourceName m_rCivGearScript;
	
	const int HEADGEAR    = 0;
	const int SHIRT       = 1;
	const int ARMOREDVEST = 2;
	const int PANTS       = 3;
	const int BOOTS       = 4;
	const int BACKPACK    = 5;
	const int VEST        = 6;
	const int HANDWEAR    = 7;
	const int HEAD        = 8;
	const int EYES        = 9;
	const int EARS        = 10;
	const int FACE        = 11;	
	const int NECK        = 12;
	const int EXTRA1      = 13;
	const int EXTRA2      = 14;
	const int WAIST       = 15;
	const int EXTRA3      = 16;
	const int EXTRA4      = 17;
	
	protected SCR_CharacterInventoryStorageComponent m_Inventory;
	protected InventoryStorageManagerComponent m_InventoryManager;
	protected CRF_GearScriptConfig m_GearConfig;
	protected ref EntitySpawnParams m_SpawnParams = new EntitySpawnParams();
	protected ref array<Managed> m_WeaponSlotComponentArray = {};
	
	void WaitTillGameStart(IEntity entity)
	{
		if(!GetGame().GetWorld())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 100, false, entity);
			return;
		}
		
		AddGearToEntity(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		if(!m_bGearScriptEnabled || !Replication.IsServer())
			return;
		
		GetGame().GetCallqueue().CallLater(WaitTillGameStart, 100, false, entity);
	}
	
	void AddGearToEntity(IEntity entity)
	{
		ResourceName ResourceNameToScan = entity.GetPrefabData().GetPrefabName();	
		
		if(!ResourceNameToScan.Contains("CRF_GS_"))
			return;
		
		ResourceName gearScriptResourceName;
		
		switch(true)
		{
			case(ResourceNameToScan.Contains("BLUFOR")) : {gearScriptResourceName = m_rBluforGearScript; break;}
			case(ResourceNameToScan.Contains("OPFOR"))  : {gearScriptResourceName = m_rOpforGearScript;  break;}
			case(ResourceNameToScan.Contains("INDFOR")) : {gearScriptResourceName = m_rIndforGearScript; break;}
			case(ResourceNameToScan.Contains("CIV"))    : {gearScriptResourceName = m_rCivGearScript;    break;}
		}
		
		if(gearScriptResourceName.IsEmpty())
			return;

        m_SpawnParams.TransformMode = ETransformMode.WORLD;
        m_SpawnParams.Transform[3] = entity.GetOrigin();
		
		Resource container = BaseContainerTools.LoadContainer(gearScriptResourceName);
        CRF_GearScriptConfig GearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResourceName).GetResource().ToBaseContainer()));
		entity.FindComponents(WeaponSlotComponent, m_WeaponSlotComponentArray);
		m_Inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		m_InventoryManager = InventoryStorageManagerComponent.Cast(entity.FindComponent(InventoryStorageManagerComponent));
		
		//------------------------------------------------------------------------------------------------
		// CLOTHING
		//------------------------------------------------------------------------------------------------
		
		foreach(CRF_Clothing clothing : GearConfig.m_DefaultGear.m_DefaultClothing)
		{
			UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
		}
		
		//------------------------------------------------------------------------------------------------
		// CUSTOM GEAR
		//------------------------------------------------------------------------------------------------
		
		//--------------------- LEADERSHIP ---------------------\\
		switch(true)
		{
			case(ResourceNameToScan.Contains("COY_P")) : 
			{	
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Company Commander");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("PLT_P")) : 
			{
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Platoon Commander");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("SL_P")) : 
			{
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Medical Officer");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("MO_P")) : 
			{
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Squad Lead");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}	
			case(ResourceNameToScan.Contains("IndirectLead_P")) : 
			{
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Vehicle Lead");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}	
			case(ResourceNameToScan.Contains("LogiLead_P")) : 
			{
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Indirect Lead");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}	
			case(ResourceNameToScan.Contains("VehLead_P")) : 
			{
				UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, "Logi Lead");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			//--------------------- Squad Level ---------------------\\
			case(ResourceNameToScan.Contains("TL_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Team Lead");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("MED_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Medic");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("GREN_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Grenadier");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("Rifle_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Rifleman");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("DEMO_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Rifleman Demo");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AT_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Rifleman AntiTank");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "AT", false);
				break;
			}
			case(ResourceNameToScan.Contains("AAT_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Assistant Rifleman AntiTank");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AR_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Automatic Rifleman");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "AR", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AAR_P")) : 
			{
				UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, "Assistant Automatic Rifleman");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			//--------------------- Infantry Specialties ---------------------\\
			case(ResourceNameToScan.Contains("HAT_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Heavy AntiTank");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "HAT", false);
				break;
			}
			case(ResourceNameToScan.Contains("AHAT_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Assistant Heavy AntiTank");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("MAT_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Medium AntiTank");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "MAT", false);
				break;
			}
			case(ResourceNameToScan.Contains("AMAT_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Assistant Medium AntiTank");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("HMG_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Heavy MachineGun");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "HMG", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AHMG_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Assistant Heavy MachineGun");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("MMG_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Medium MachineGun");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "MMG", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AMMG_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Assistant Medium MachineGun");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("AA_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Anit-Air");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "AA", false);
				break;
			}
			case(ResourceNameToScan.Contains("AAA_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Assistant Anit-Air");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("Sniper_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Sniper");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Sniper", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("Spotter_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Spotter");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("FO_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Forward Observer");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("JTAC_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "JTAC");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("LogiRunner_P")) : 
			{
				UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, "Logi Runner");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			//--------------------- Vehicle Specialties ---------------------\\
			case(ResourceNameToScan.Contains("VehDriver_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Vehicle Driver");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("VehGunner_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Vehicle Gunner");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("VehLoader_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Vehicle Loader");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("Pilot_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Pilot");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("CrewChief_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Crew Chief");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("IndirectGunner_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Indirect Gunner");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("IndirectLoader_P")) : 
			{
				UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, "Indirect Loader");
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
		}
		
		//------------------------------------------------------------------------------------------------
		// ITEMS
		//------------------------------------------------------------------------------------------------
		
		foreach(CRF_Inventory_Item item : GearConfig.m_DefaultGear.m_DefaultInventoryItems)
		{
			AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
		}
	}
	
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, string slot)
	{
		if(clothingArray.IsEmpty() || slot.IsEmpty())
			return;
		
		int slotInt = -1;
		
		// All the arrays belong to us
		switch(slot)
		{
			case "HEADGEAR"     : {slotInt = HEADGEAR;    break;}
			case "SHIRT"        : {slotInt = SHIRT;       break;}
			case "ARMOREDVEST"  : {slotInt = ARMOREDVEST; break;}
			case "PANTS"        : {slotInt = PANTS;       break;}
			case "BOOTS"        : {slotInt = BOOTS;       break;}
			case "BACKPACK"     : {slotInt = BACKPACK;    break;}
			case "VEST"         : {slotInt = VEST;        break;}
			case "HANDWEAR"     : {slotInt = HANDWEAR;    break;}
			case "HEAD"         : {slotInt = HEAD;        break;}
			case "EYES"         : {slotInt = EYES;        break;}
			case "EARS"         : {slotInt = EARS;        break;}
			case "FACE"         : {slotInt = FACE;        break;}
			case "NECK"         : {slotInt = NECK;        break;}
			case "EXTRA1"       : {slotInt = EXTRA1;      break;}
			case "EXTRA2"       : {slotInt = EXTRA2;      break;}
			case "WAIST"        : {slotInt = WAIST;       break;}
			case "EXTRA3"       : {slotInt = EXTRA3;      break;}
			case "EXTRA4"       : {slotInt = EXTRA4;      break;}
		};
		
		if(slotInt == -1)
			return;
		
		ResourceName clothing = clothingArray.GetRandomElement();
		IEntity previousClothing = m_Inventory.Get(slotInt);
		
		if (previousClothing != null)
		{
			m_InventoryManager.TryRemoveItemFromStorage(previousClothing, m_Inventory);
			SCR_EntityHelper.DeleteEntityAndChildren(previousClothing);
		};
		
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothing),GetGame().GetWorld(),m_SpawnParams);
		bool isThereSpace = m_InventoryManager.TryReplaceItem(resourceSpawned, m_Inventory, slotInt);
		if (!isThereSpace)
		{
			Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
			Print(string.Format("CRF: UNABLE TO INSERT CLOTHING: %1 /n INTO ENTITY: %2", clothing, m_InventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
			Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
			delete resourceSpawned;
		};
	}
	
	protected void AddInventoryItems(ResourceName item, int itemAmmount)
	{
		if(item.IsEmpty() || itemAmmount <= 0)
			return;
		
		for(int i = 1; i <= itemAmmount; i++)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item),GetGame().GetWorld(),m_SpawnParams);
			bool isThereSpace = m_InventoryManager.TryInsertItem(resourceSpawned);
			if (!isThereSpace)
			{
				Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
				Print(string.Format("CRF: UNABLE TO INSERT ITEM: %1 /n INTO ENTITY: %2", item, m_InventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
				delete resourceSpawned;
			};
		}
	}
	
	protected void AddWeapons(array<Managed> WeaponSlotComponentArray, CRF_GearScriptConfig GearConfig, string WeaponType, string ATType, bool GivePistol)
	{
		for(int i = 0; i < WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(WeaponSlotComponentArray.Get(i));
			IEntity weapon = weaponSlotComponent.GetWeaponEntity();		
			IEntity weaponSpawned;
			array<ResourceName> WeaponsAttachments = {};
			array<AttachmentSlotComponent> AttatchmentSlotArray = {};
			ResourceName MagazineName;
			int MagazineCount;
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(WeaponSlotComponentArray.Get((WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(ATType != "")
					{
						switch(ATType)
						{
							case "AT"   : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AT.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_AT.m_Attachments; MagazineName = GearConfig.m_Weapons.m_AT.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_AT.m_MagazineCount;      break;}
							case "MAT"  : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_MAT.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_MAT.m_Attachments; MagazineName = GearConfig.m_Weapons.m_MAT.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_MAT.m_MagazineCount;  break;}
							case "HAT"  : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_HAT.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_HAT.m_Attachments; MagazineName = GearConfig.m_Weapons.m_HAT.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_HAT.m_MagazineCount;  break;}
						}
						if(MagazineName != "" || !MagazineName)
						{
							Print("No AT magazine");
							AddInventoryItems(MagazineName, MagazineCount);
						}
						
						weaponSlotComponent.SetWeapon(weaponSpawned);
						weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
						
						if(WeaponsAttachments.Count() == 0)
							continue;
				
						foreach(ResourceName Attachment : WeaponsAttachments)
						{
							foreach(AttachmentSlotComponent AttachmentSlot : AttatchmentSlotArray)
							{
								IEntity AttachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(Attachment),GetGame().GetWorld(),m_SpawnParams);
								if(AttachmentSlot.CanSetAttachment(AttachmentSpawned))
								{
									delete AttachmentSlot.GetAttachedEntity();
									AttachmentSlot.SetAttachment(AttachmentSpawned);
									break;
								}
								delete AttachmentSpawned;
							} 
						}
					}
					continue;
				}
				switch(WeaponType)
				{
					case "Rifle"     : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Rifle.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Rifle.m_Attachments; MagazineName = GearConfig.m_Weapons.m_Rifle.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_Rifle.m_MagazineCount;               break;}
					case "RifleUGL"  : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_RifleUGL.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_RifleUGL.m_Attachments; MagazineName = GearConfig.m_Weapons.m_RifleUGL.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_RifleUGL.m_MagazineCount;   break;}
					case "Carbine"   : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Carbine.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Carbine.m_Attachments; MagazineName = GearConfig.m_Weapons.m_Carbine.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_Carbine.m_MagazineCount;       break;}
					case "AR"        : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AR.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_AR.m_Attachments; MagazineName = GearConfig.m_Weapons.m_AR.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_AR.m_MagazineCount;                           break;}
					case "MMG"       : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_MMG.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_MMG.m_Attachments; MagazineName = GearConfig.m_Weapons.m_MMG.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_MMG.m_MagazineCount;                       break;}
					case "HMG"       : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_HMG.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_HMG.m_Attachments; MagazineName = GearConfig.m_Weapons.m_HMG.m_Magazine; MagazineCount = GearConfig.m_Weapons.m_HMG.m_MagazineCount;                       break;}
				}
				
					
				
				if((MagazineName != "" || !MagazineName) && WeaponType == "RifleUGL")
				{
					AddInventoryItems(GearConfig.m_Weapons.m_RifleUGL.m_UGLMagazine, GearConfig.m_Weapons.m_RifleUGL.m_UGLMagazineCount);
					AddInventoryItems(MagazineName, MagazineCount);
				} 
				else
				{
					AddInventoryItems(MagazineName, MagazineCount);
				}
				
				weaponSlotComponent.SetWeapon(weaponSpawned);
				weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
				
				if(WeaponsAttachments.Count() == 0)
						continue;
				
				foreach(ResourceName Attachment : WeaponsAttachments)
				{
					foreach(AttachmentSlotComponent AttachmentSlot : AttatchmentSlotArray)
					{
						IEntity AttachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(Attachment),GetGame().GetWorld(),m_SpawnParams);
						if(AttachmentSlot.CanSetAttachment(AttachmentSpawned))
						{
							delete AttachmentSlot.GetAttachedEntity();
							AttachmentSlot.SetAttachment(AttachmentSpawned);
							break;
						}
						delete AttachmentSpawned;
					} 
				}
			}
			if(weaponSlotComponent.GetWeaponSlotType() == "secondary" && GivePistol == true)
			{
				weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Pistol.m_Weapon),GetGame().GetWorld(),m_SpawnParams);
				if(GearConfig.m_Weapons.m_Pistol.m_Magazine != "" || !GearConfig.m_Weapons.m_Pistol.m_Magazine)
							AddInventoryItems(GearConfig.m_Weapons.m_Pistol.m_Magazine, GearConfig.m_Weapons.m_Pistol.m_MagazineCount);
				weaponSlotComponent.SetWeapon(weaponSpawned);
				weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
				
				foreach(ResourceName Attachment : GearConfig.m_Weapons.m_Pistol.m_Attachments)
				{
					foreach(AttachmentSlotComponent AttachmentSlot : AttatchmentSlotArray)
					{
						IEntity AttachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(Attachment),GetGame().GetWorld(),m_SpawnParams);
						if(AttachmentSlot.CanSetAttachment(AttachmentSpawned))
						{
							delete AttachmentSlot.GetAttachedEntity();
							AttachmentSlot.SetAttachment(AttachmentSpawned);
							break;
						}
						delete AttachmentSpawned;
					} 
				}	
			}
		}
	}
	
	protected void UpdateLeadershipCustomGear(array<ref CRF_Leadership_Custom_Gear> customGearArray, string role)
	{
		foreach(ref CRF_Leadership_Custom_Gear customGear : customGearArray)
		{
			if(customGear.m_sRoleToOverride == role)
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
			}
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
			}
		}
	}
	
	protected void UpdateSquadLevelCustomGear(array<ref CRF_Squad_Level_Custom_Gear> customGearArray, string role)
	{
		foreach(ref CRF_Squad_Level_Custom_Gear customGear : customGearArray)
		{
			if(customGear.m_sRoleToOverride == role)
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
			}
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
			}
		}
	}
	
	protected void UpdateInfantrySpecialtiesCustomGear(array<ref CRF_Infantry_Specialties_Custom_Gear> customGearArray, string role)
	{
		foreach(ref CRF_Infantry_Specialties_Custom_Gear customGear : customGearArray)
		{
			if(customGear.m_sRoleToOverride == role)
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
			}
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
			}
		}
	}
	
	protected void UpdateVehicleSpecialtiesCustomGear(array<ref CRF_Vehicle_Specialties_Custom_Gear> customGearArray, string role)
	{
		foreach(ref CRF_Vehicle_Specialties_Custom_Gear customGear : customGearArray)
		{
			if(customGear.m_sRoleToOverride == role)
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
			{
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
			}
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
			{
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
			}
		}
	}
}