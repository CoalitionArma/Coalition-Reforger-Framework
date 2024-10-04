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
		
		Print(ResourceNameToScan);
		
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
		
		Print(gearScriptResourceName);
		Resource container = BaseContainerTools.LoadContainer(gearScriptResourceName);
        CRF_GearScriptConfig GearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResourceName).GetResource().ToBaseContainer()));
		//m_GearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResourceName).GetResource().ToBaseContainer()));
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
	
				break;
			}
			case(ResourceNameToScan.Contains("PLT")) : 
			{
				//PLT Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("SL")) : 
			{
				//SL Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("TL")) : 
			{
				//TL Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("MED")) : 
			{
				//MED Items go here, preferebly by reading a array/set of arrays on the config.
				
				/*
				UpdateClothingSlot(m_GearConfig.m_DefaultGear.m_DefaultClothing.m_MedicBackpack, BACKPACK); //update the backpack of the medic to whatever is defined in the gearscript
				AddInventoryItems(m_GearConfig.m_DefaultGear.m_DefaultClothing.m_MedicInventory);			 //add the role specific inventory items (blood, medkit, etc)
				AddWeapon(m_GearConfig.m_DefaultGear.m_DefaultClothing.m_CarbineWeapon)					 //add the carbine weapon and we'll want to also add the mags with this function
				
				We're done!
				*/
				
				break;
			}
			case(ResourceNameToScan.Contains("AR")) : 
			{
				//AR Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("AAR")) : 
			{
				//AAR Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("RAT")) : 
			{
				//RAT Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("GREN")) : 
			{
				//GREN Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("DEMO")) : 
			{
				//DEMO Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("MMG")) : 
			{
				//MMG Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("AMMG")) : 
			{
				//AMMG Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("MAT")) : 
			{
				//MAT Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("AMAT")) : 
			{
				//AMAT Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("HAT")) : 
			{
				//HAT Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("AHAT")) : 
			{
				//AHAT Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("HMG")) : 
			{
				//HMG Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("AHMG")) : 
			{
				//AHMG Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			case(ResourceNameToScan.Contains("CREW")) : 
			{
				//CREW Items go here, preferebly by reading a array/set of arrays on the config.
	
				break;
			}
			default : 
			{
				//Rifleman Items go here, preferebly by reading a array/set of arrays on the config.
			
				
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
	
	protected void AddWeapon(CRF_Weapon_Class weapon)
	{
		//idk, do whatever you need to do.
	}
}