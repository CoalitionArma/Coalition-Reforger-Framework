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
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, false, entity);
			return;
		}
		
		GetGame().GetCallqueue().CallLater(AddGearToEntity, 1000, false, entity);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		GetGame().GetCallqueue().CallLater(WaitTillGameStart, 1000, false, entity);
	}
	
	void AddGearToEntity(IEntity entity)
	{
	
	if(!m_bGearScriptEnabled || !Replication.IsServer())
			return;
		
		ResourceName ResourceNameToScan = entity.GetPrefabData().GetPrefabName();
		
		
		if(!ResourceNameToScan.Contains("CRF_GS_")) //I'd Prefer "CRF_" but that's require nuking all premade character prefabs.
			return;
		
		ResourceName gearScriptResourceName;
		
		switch(true)
		{
			case(ResourceNameToScan.Contains("BLUFOR")) : {gearScriptResourceName = m_rBluforGearScript;  break;}
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
		//m_GearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResourceName).GetResource().ToBaseContainer()));
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
		// ITEMS
		//------------------------------------------------------------------------------------------------
		
		foreach(CRF_Inventory_Item item : GearConfig.m_DefaultGear.m_DefaultInventoryItems)
		{
			AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
		}
		
		//Patman Note for salami: would heavily recommend setting up seperate functions for common items like total bandages, weapon and ammo, etc. I'll continue fleshing this out tmrw if you'd like. 
		//Also, naming convention of blank character prefabs should be CRF_GS_OPF_MMG, CRF_GS_BLU_RAT, CRF_GS_INF_HAT etc...
		
		// We could also lean into role-specific gear with the bellow method, but that'd require more config work on your end, but it would be cool..... fuck it ima write the functions and populate Medic rq so you get an idea
		
		switch(true)
		{
			case(ResourceNameToScan.Contains("COY")) : 
			{
				//COY Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("PLT")) : 
			{
				//PLT Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("SL")) : 
			{
				//SL Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("TL")) : 
			{
				//TL Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("Medic")) : 
			{
				//MED Items go here, preferebly by reading a array/set of arrays on the config.
				
				
				//UpdateClothingSlot(m_GearConfig.m_DefaultGear.m_DefaultClothing.m_MedicBackpack, BACKPACK); //update the backpack of the medic to whatever is defined in the gearscript
				//AddInventoryItems(m_GearConfig.m_DefaultGear.m_DefaultClothing.m_MedicInventory);			 //add the role specific inventory items (blood, medkit, etc)
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);;					 //add the carbine weapon and we'll want to also add the mags with this function
				
				break;
			}
			case(ResourceNameToScan.Contains("AR")) : 
			{
				//AR Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "AR", "", true);
				break;
			}
			case(ResourceNameToScan.Contains("AAR")) : 
			{
				//AAR Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AT")) : 
			{
				//RAT Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "AT", false);
				break;
			}
			case(ResourceNameToScan.Contains("GREN")) : 
			{
				//GREN Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("DEMO")) : 
			{
				//DEMO Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("MMG")) : 
			{
				//MMG Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "MMG", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AMMG")) : 
			{
				//AMMG Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("MAT")) : 
			{
				//MAT Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "MAT", false);
				break;
			}
			case(ResourceNameToScan.Contains("AMAT")) : 
			{
				//AMAT Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("HAT")) : 
			{
				//HAT Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "HAT", false);
				break;
			}
			case(ResourceNameToScan.Contains("AHAT")) : 
			{
				//AHAT Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("HMG")) : 
			{
				//HMG Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "HMG", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("AHMG")) : 
			{
				//AHMG Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
				break;
			}
			case(ResourceNameToScan.Contains("CREW")) : 
			{
				//CREW Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);
				break;
			}
			default : 
			{
				//Rifleman Items go here, preferebly by reading a array/set of arrays on the config.
				AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);
			}
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
		
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothingArray.GetRandomElement()),GetGame().GetWorld(),m_SpawnParams);
		m_InventoryManager.TryReplaceItem(resourceSpawned, m_Inventory, slotInt);
	}
	
	protected void AddInventoryItems(ResourceName itemResource, int itemAmmount)
	{
		if(itemResource.IsEmpty() || itemAmmount <= 0)
			return;
		
		for(int i = 1; i <= itemAmmount; i++)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(itemResource),GetGame().GetWorld(),m_SpawnParams);
			m_InventoryManager.TryInsertItem(resourceSpawned);
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
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(WeaponSlotComponentArray.Get((WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(ATType != "")
					{
						switch(ATType)
						{
							case "AT"   : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AT.m_Weapon),GetGame().GetWorld(),m_SpawnParams);   break;}
							case "MAT"  : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_MAT.m_Weapon),GetGame().GetWorld(),m_SpawnParams);  break;}
							case "HAT"  : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_HAT.m_Weapon),GetGame().GetWorld(),m_SpawnParams);  break;}
						}
						weaponSlotComponent.SetWeapon(weaponSpawned);
					}
					continue;
				}
				switch(WeaponType)
				{
					case "Rifle"     : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Rifle.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Rifle.m_Attachments;       break;}
					case "RifleUGL"  : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_RifleUGL.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_RifleUGL.m_Attachments; break;}
					case "Carbine"   : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Carbine.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Carbine.m_Attachments;   break;}
					case "AR"        : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AR.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_AR.m_Attachments;             break;}
					case "MMG"       : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_MMG.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_MMG.m_Attachments;           break;}
					case "HMG"       : {weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_HMG.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_HMG.m_Attachments;           break;}
				}
				weaponSlotComponent.SetWeapon(weaponSpawned);
				weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
				if(WeaponsAttachments.Count() == 0)
						continue;
				Print("There are attachments");
				foreach(ResourceName Attachment : WeaponsAttachments)
				{
					foreach(AttachmentSlotComponent AttachmentSlot : AttatchmentSlotArray)
					{
						IEntity AttachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(Attachment),GetGame().GetWorld(),m_SpawnParams);
						if(AttachmentSlot.CanSetAttachment(AttachmentSpawned))
						{
							Print("Setting Attachment");
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
				weaponSlotComponent.SetWeapon(weaponSpawned);
			}
		}
	}
}