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
	const int EXTRA       = 13;
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
			case(ResourceNameToScan.Contains("OPF")) : {gearScriptResourceName = m_rOpforGearScript;  break;}
			case(ResourceNameToScan.Contains("INF")) : {gearScriptResourceName = m_rIndforGearScript; break;}
			default                                  : {gearScriptResourceName = m_rBluforGearScript;  break;}
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
		
		for(int i = HEADGEAR; i <= EXTRA4; i++)
		{
			array<ResourceName> resourceArray = {};
			
			// All the arrays belong to us
			switch(i)
			{
				case HEADGEAR     : {resourceArray = GearConfig.m_GearScript.m_headgear;    break;}
				case SHIRT        : {resourceArray = GearConfig.m_GearScript.m_shirts;      break;}
				case ARMOREDVEST  : {resourceArray = GearConfig.m_GearScript.m_armoredVest; break;}
				case PANTS        : {resourceArray = GearConfig.m_GearScript.m_pants;       break;}
				case BOOTS        : {resourceArray = GearConfig.m_GearScript.m_boots;       break;}
				case BACKPACK     : {resourceArray = GearConfig.m_GearScript.m_backpack;    break;}
				case VEST         : {resourceArray = GearConfig.m_GearScript.m_vest;        break;}
				case HANDWEAR     : {resourceArray = GearConfig.m_GearScript.m_handwear;    break;}
				case HEAD         : {resourceArray = GearConfig.m_GearScript.m_head;        break;}
				case EYES         : {resourceArray = GearConfig.m_GearScript.m_eyes;        break;}
				case EARS         : {resourceArray = GearConfig.m_GearScript.m_ears;        break;}
				case FACE         : {resourceArray = GearConfig.m_GearScript.m_face;        break;}
				case NECK         : {resourceArray = GearConfig.m_GearScript.m_neck;        break;}
				case EXTRA        : {resourceArray = GearConfig.m_GearScript.m_extra;       break;}
				case EXTRA2       : {resourceArray = GearConfig.m_GearScript.m_extra2;      break;}
				case WAIST        : {resourceArray = GearConfig.m_GearScript.m_waist;       break;}
				case EXTRA3       : {resourceArray = GearConfig.m_GearScript.m_extra3;      break;}
				case EXTRA4       : {resourceArray = GearConfig.m_GearScript.m_extra4;      break;}
			};
			UpdateClothingSlot(resourceArray, i);
		}
		
		//------------------------------------------------------------------------------------------------
		// ITEMS
		//------------------------------------------------------------------------------------------------
		
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
				UpdateClothingSlot(m_GearConfig.m_GearScript.m_MedicBackpack, BACKPACK); //update the backpack of the medic to whatever is defined in the gearscript
				AddInventoryItems(m_GearConfig.m_GearScript.m_BasicInventory);			 //add the default inventory items (bandages, radios, grenades, etc)
				AddInventoryItems(m_GearConfig.m_GearScript.m_MedicInventory);			 //add the role specific inventory items (blood, medkit, etc)
				AddWeapon(m_GearConfig.m_GearScript.m_CarbineWeapon)					 //add the carbine weapon and we'll want to also add the mags with this function
				
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
	
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, int slot)
	{
		if(clothingArray.IsEmpty() || slot < 0)
			return;
		
		IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothingArray.GetRandomElement()),GetGame().GetWorld(),m_SpawnParams);
		m_InventoryManager.TryReplaceItem(resourceSpawned, m_Inventory, slot);
	}
	
	protected void AddInventoryItems(array<ResourceName> inventoryArray)
	{
		if(inventoryArray.IsEmpty())
			return;
		
		foreach (ResourceName resource : inventoryArray)
		{
			IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(resource),GetGame().GetWorld(),m_SpawnParams);
			m_InventoryManager.TryInsertItem(resourceSpawned);
		}
	}
	
	protected void AddWeapon(CRF_weaponClass weapon)
	{
		//idk, do whatever you need to do.
	}
}