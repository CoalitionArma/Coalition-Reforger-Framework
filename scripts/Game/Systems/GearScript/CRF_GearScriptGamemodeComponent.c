[BaseContainerProps()]
class CRF_GearScriptContainer
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Gearscript applied to all players on this side", "conf class=CRF_GearScriptConfig")]
	ResourceName m_rGearScript;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableMiniArsenal;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rLeadershipRadiosPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rRTORadiosPrefab;
	
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")]
	ResourceName m_rGIRadiosPrefab;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableLeadershipRadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableRTORadios;
	
	[Attribute("true", UIWidgets.CheckBox)]
	bool m_bEnableGIRadios;
};

[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GearScriptGamemodeComponentClass: SCR_BaseGameModeComponentClass {}

class CRF_GearScriptGamemodeComponent: SCR_BaseGameModeComponent
{	
	[Attribute("false", UIWidgets.Auto, desc: "Is Gearscript Enabled", category: "GENERAL")]
	protected bool m_bGearScriptEnabled;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "GENERAL")]
	protected ref CRF_GearScriptContainer m_BLUFORGearScriptSettings;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "GENERAL")]
	protected ref CRF_GearScriptContainer m_OPFORGearScriptSettings;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "GENERAL")]
	protected ref CRF_GearScriptContainer m_INDFORGearScriptSettings;
	
	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "GENERAL")]
	protected ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
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
	
	const ref array<EWeaponType> WEAPON_TYPES_THROWABLE = {EWeaponType.WT_FRAGGRENADE, EWeaponType.WT_SMOKEGRENADE};
	
	const ref TStringArray m_aLeadershipRolesUGL = {"_COY_P", "_PL_P", "_SL_P", "_FO_P", "_JTAC_P"};
	const ref TStringArray m_aLeadershipRolesCarbine = {"_MO_P", "_IndirectLead_P", "_LogiLead_P", "_VehLead_P"};
		
	const ref TStringArray m_aSquadLevelRolesUGL = {"_TL_P", "_Gren_P", "_RTO_P"};
	const ref TStringArray m_aSquadLevelRolesRifle = {"_Rifleman_P", "_Demo_P", "_AAT_P", "_AAR_P"};
	const ref TStringArray m_aSquadLevelRolesCarbine = {"_Medic_P"};
		
	const ref TStringArray m_aInfantrySpecialtiesRolesRifle = {"_AHAT_P", "_AMAT_P", "_AHMG_P", "_AMMG_P", "_AAA_P"};
		
	const ref TStringArray m_aVehicleSpecialtiesRolesCarbine = {"_VehDriver_P", "_VehGunner_P", "_VehLoader_P", "_LogiRunner_P", "_IndirectGunner_P", "_IndirectLoader_P"};
	const ref TStringArray m_aVehicleSpecialtiesRolesPistol = {"_Pilot_P", "_CrewChief_P"};
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	
	protected SCR_CharacterInventoryStorageComponent m_Inventory;
	protected InventoryStorageManagerComponent m_InventoryManager;
	protected SCR_InventoryStorageManagerComponent m_SCRInventoryManager;
	protected BaseInventoryStorageComponent m_StorageComp;
	protected ref EntitySpawnParams m_SpawnParams = new EntitySpawnParams();
	protected ref array<Managed> m_WeaponSlotComponentArray = {};
	protected ref RandomGenerator m_RNG = new RandomGenerator;

	// A array we use primarily for replication of m_mAllPlayerGearScriptsMap to clients.
	[RplProp(onRplName: "UpdateLocalPlayerGearScriptsMap")]
	protected ref array<string> m_aAllPlayerGearScriptsArray = new array<string>;
	
	// A hashmap that is modified only on each client by a .BumpMe from the authority.
	protected ref map<string, string> m_mAllPlayerGearScriptsMap = new map<string, string>;
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	static CRF_GearScriptGamemodeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_GearScriptGamemodeComponent.Cast(gameMode.FindComponent(CRF_GearScriptGamemodeComponent));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void WaitTillGameStart(IEntity entity)
	{
		if(!GetGame().GetWorld())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 100, false, entity);
			return;
		}
		
		GetGame().GetCallqueue().CallLater(AddGearToEntity, m_RNG.RandInt(500, 2500), false, entity, entity.GetPrefabData().GetPrefabName());
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		if(!m_bGearScriptEnabled || RplSession.Mode() == RplMode.Client)
			return;
		
		GetGame().GetCallqueue().CallLater(WaitTillGameStart, 100, false, entity);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ResourceName GetGearScriptResource(string factionKey)
	{
		return GetGearScriptSettings(factionKey).m_rGearScript;
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	CRF_GearScriptContainer GetGearScriptSettings(string factionKey)
	{
		CRF_GearScriptContainer gearScriptContainer;
		
		switch(factionKey)
		{
			case "BLUFOR" : {gearScriptContainer = m_BLUFORGearScriptSettings;   break;}
			case "OPFOR"  : {gearScriptContainer = m_OPFORGearScriptSettings;    break;}
			case "INDFOR" : {gearScriptContainer = m_INDFORGearScriptSettings;   break;}
			case "CIV"    : {gearScriptContainer = m_CIVILIANGearScriptSettings; break;}
		}
		
		return gearScriptContainer;
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Functions to replicate and store values to each clients m_mAllPlayerGearScriptsMap
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		//--- Server only
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		GetGame().GetCallqueue().CallLater(UpdatePlayerGearScriptsArray, m_RNG.RandInt(4000, 6000), true);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override protected void OnGameEnd()
	{
		super.OnGameEnd();
		
		//--- Server only
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		GetGame().GetCallqueue().Remove(UpdatePlayerGearScriptsArray);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	string ReturnPlayerGearScriptsMapValue(int playerID, string key)
	{
		// Get the players key
		key = string.Format("%1%2", playerID, key);
		return m_mAllPlayerGearScriptsMap.Get(key);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetPlayerGearScriptsMapValue(string value, int playerID, string key)
	{
		// Get the players key
		key = string.Format("%1%2", playerID, key);
		m_mAllPlayerGearScriptsMap.Set(key, value);
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdatePlayerGearScriptsArray()
	{
		// Create a temp array so we arent broadcasting for each change to m_aAllPlayerGearScriptsArray.
		protected ref array<string> tempPlayerArray = new array<string>;

		// Fill tempPlayerArray with all keys and values in m_mAllPlayerGearScriptsMap.
		for (int i = 0; i < m_mAllPlayerGearScriptsMap.Count(); i++)
		{
			string key = m_mAllPlayerGearScriptsMap.GetKey(i);
			string value = m_mAllPlayerGearScriptsMap.Get(key);
			
			tempPlayerArray.Insert(string.Format("%1~%2", key, value));
		};

		// Replicate m_aAllPlayerGearScriptsArray to all clients.
		m_aAllPlayerGearScriptsArray = tempPlayerArray;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdateLocalPlayerGearScriptsMap()
	{
		// Fill m_mAllPlayerGearScriptsMap with all keys and values from authorities m_mAllPlayerGearScriptsMap.
		foreach (string playerKeyAndValueToSplit : m_aAllPlayerGearScriptsArray)
		{
			array<string> playerKeyAndValueArray = {};
			playerKeyAndValueToSplit.Split("~", playerKeyAndValueArray, false);
			m_mAllPlayerGearScriptsMap.Set(playerKeyAndValueArray[0], playerKeyAndValueArray[1]);
		};
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Functions to for Gear Script
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AddGearToEntity(IEntity entity, ResourceName prefabName)
	{
		ResourceName ResourceNameToScan = prefabName;	
		
		if(!ResourceNameToScan.Contains("CRF_GS_") || !entity)
			return;
		
		ResourceName gearScriptResourceName;
		CRF_GearScriptContainer gearScriptSettings;
		
		switch(true)
		{
			case(ResourceNameToScan.Contains("BLUFOR")) : {gearScriptResourceName = m_BLUFORGearScriptSettings.m_rGearScript;   gearScriptSettings = m_BLUFORGearScriptSettings;   break;}
			case(ResourceNameToScan.Contains("OPFOR"))  : {gearScriptResourceName = m_OPFORGearScriptSettings.m_rGearScript;    gearScriptSettings = m_OPFORGearScriptSettings;    break;}
			case(ResourceNameToScan.Contains("INDFOR")) : {gearScriptResourceName = m_INDFORGearScriptSettings.m_rGearScript;   gearScriptSettings = m_INDFORGearScriptSettings;   break;}
			case(ResourceNameToScan.Contains("CIV"))    : {gearScriptResourceName = m_CIVILIANGearScriptSettings.m_rGearScript; gearScriptSettings = m_CIVILIANGearScriptSettings; break;}
		}
		
		if(gearScriptResourceName.IsEmpty())
			return;

        m_SpawnParams.TransformMode = ETransformMode.WORLD;
        m_SpawnParams.Transform[3] = entity.GetOrigin();
		
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
        CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResourceName).GetResource().ToBaseContainer()));
		
		entity.FindComponents(WeaponSlotComponent, m_WeaponSlotComponentArray);
		m_Inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		m_InventoryManager = InventoryStorageManagerComponent.Cast(entity.FindComponent(InventoryStorageManagerComponent));
		m_SCRInventoryManager = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		
		if(!m_Inventory || !m_InventoryManager || !m_SCRInventoryManager)
		{
			Print(string.Format("CRF GEAR SCRIPT ERROR: %1 DOESN'T HAVE COMPONENTS WE NEED!", entity), LogLevel.ERROR);
			return;
		}
		
		array<IEntity> items = {};
		array<IEntity> itemsRoot = {};
		m_SCRInventoryManager.GetAllItems(items, m_Inventory);
		m_SCRInventoryManager.GetAllRootItems(itemsRoot);
		
		items.InsertAll(itemsRoot);
		
		foreach(IEntity item : items)
			SCR_EntityHelper.DeleteEntityAndChildren(item);

		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// GET ROLE
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		array<string> value = {};
		ResourceNameToScan.Split("_", value, true);
		
		string role = "_" + value[3] + "_" + value[4];
		
		role.Split(".", value, true);
		role = value[0];
		
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// CLOTHING
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

		if(gearConfig.m_DefaultFactionGear)
			foreach(CRF_Clothing clothing : gearConfig.m_DefaultFactionGear.m_DefaultClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType, role);
		
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// CUSTOM GEAR
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		bool isLeader = false;
		bool isSquad = false;
		bool isInfSpec = false;
		bool isVehSpec = false;
		
		if(gearConfig.m_FactionWeapons)
		{
			switch(true)
			{
				// Leadership --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				case(m_aLeadershipRolesUGL.Contains(role))             : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "RifleUGL", "",    true);  isLeader = true;  break;}
				case(m_aLeadershipRolesCarbine.Contains(role))         : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Carbine",  "",    true);  isLeader = true;  break;}	
				// Squad Level -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				case(m_aSquadLevelRolesUGL.Contains(role))             : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "RifleUGL", "",    false); isSquad = true;   break;}
				case(m_aSquadLevelRolesCarbine.Contains(role))         : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Carbine",  "",    false); isSquad = true;   break;}
				case(m_aSquadLevelRolesRifle.Contains(role))           : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Rifle",    "",    false); isSquad = true;   break;}
				case(role == "_AT_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Rifle",    "AT",  false); isSquad = true;   break;}
				case(role == "_AR_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "AR",       "",    false); isSquad = true;   break;}
				// Infantry Specialties ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				case(m_aInfantrySpecialtiesRolesRifle.Contains(role))  : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Rifle",    "",    false); isInfSpec = true; break;}
				case(role == "_HAT_P")                                 : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Rifle",    "HAT", false); isInfSpec = true; break;}
				case(role == "_MAT_P")                                 : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Rifle",    "MAT", false); isInfSpec = true; break;}
				case(role == "_HMG_P")                                 : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "HMG",      "",    true);  isInfSpec = true; break;}
				case(role == "_MMG_P")                                 : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "MMG",      "",    true);  isInfSpec = true; break;}
				case(role == "_AA_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Rifle",    "AA",  false); isInfSpec = true; break;}
				case(role == "_Sniper_P")                              : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Sniper",   "",    true);  isInfSpec = true; break;}
				case(role == "_Spotter_P")                             : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "RifleUGL", "",    false); isInfSpec = true; break;}
				// Vehicle Specialties -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				case(m_aVehicleSpecialtiesRolesCarbine.Contains(role)) : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "Carbine",  "",    false); isVehSpec = true; break;}
				case(m_aVehicleSpecialtiesRolesPistol.Contains(role))  : {AddWeapons(m_WeaponSlotComponentArray, gearConfig, "",         "",    true);  isVehSpec = true; break;}
			}
		} else
			Print(string.Format("CRF GEAR SCRIPT ERROR: NO WEAPONS SET: %1", gearScriptResourceName), LogLevel.ERROR);
			
		if(gearConfig.m_CustomFactionGear)
		{
			switch(true)
			{
				case(isLeader)  : {UpdateLeadershipCustomGear(gearConfig.m_CustomFactionGear.m_LeadershipCustomGear, role);                   break;}
				case(isSquad)   : {UpdateSquadLevelCustomGear(gearConfig.m_CustomFactionGear.m_SquadLevelCustomGear, role);                   break;}
				case(isInfSpec) : {UpdateInfantrySpecialtiesCustomGear(gearConfig.m_CustomFactionGear.m_InfantrySpecialtiesCustomGear, role); break;}
				case(isVehSpec) : {UpdateVehicleSpecialtiesCustomGear(gearConfig.m_CustomFactionGear.m_VehicleSpecialtiesCustomGear, role);   break;}
			}
		} else
			Print(string.Format("CRF GEAR SCRIPT ERROR: NO CUSTOM GEAR SET: %1", gearScriptResourceName), LogLevel.ERROR);
		
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// ITEMS
		//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		
		if(gearConfig.m_DefaultFactionGear)
		{
			//Who we give Leadership Radios
			if(gearScriptSettings.m_bEnableLeadershipRadios && (m_aLeadershipRolesUGL.Contains(role) || m_aLeadershipRolesCarbine.Contains(role) || role == "_Spotter_P" || role == "_Pilot_P" || role == "_CrewChief_P"))
				AddInventoryItem(gearScriptSettings.m_rLeadershipRadiosPrefab, 1);
			
			//Who we give GI Radios
			if(gearScriptSettings.m_bEnableGIRadios && !(m_aLeadershipRolesUGL.Contains(role) || m_aLeadershipRolesCarbine.Contains(role) || role == "_Spotter_P" || role == "_Pilot_P" || role == "_CrewChief_P"))
				AddInventoryItem(gearScriptSettings.m_rGIRadiosPrefab, 1);
			
			//Who we give RTO Radios
			if(gearScriptSettings.m_bEnableRTORadios && role == "_RTO_P")
				AddInventoryItem(gearScriptSettings.m_rRTORadiosPrefab, 1);
			
			//Who we give Leadership Binos
			if(gearConfig.m_DefaultFactionGear.m_bEnableLeadershipBinoculars && (m_aLeadershipRolesUGL.Contains(role) || m_aLeadershipRolesCarbine.Contains(role) || role == "_Spotter_P" || role == "_TL_P"))
				AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sLeadershipBinocularsPrefab, 1);
			
			//Who we give Assistant Binos/Extra magazines
			if(role == "_AAR_P" || role == "_AMMG_P" || role == "_AHMG_P" || role == "_AMAT_P" || role == "_AHAT_P" || role == "_AAA_P" || role == "_AAT_P")
			{
				if(gearConfig.m_FactionWeapons)
					AddAssistantMagazines(gearConfig, role);
				
				if(gearConfig.m_DefaultFactionGear.m_bEnableAssistantBinoculars)
					AddInventoryItem(gearConfig.m_DefaultFactionGear.m_sAssistantBinocularsPrefab, 1, role);
			}
			
			//Who we give extra medical items
			if(role == "_Medic_P" || role == "_MO_P")
			{	
				foreach(CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultMedicMedicalItems)
					AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, role);
			}
			
			//What everyone gets
			foreach(CRF_Inventory_Item item : gearConfig.m_DefaultFactionGear.m_DefaultInventoryItems)
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, role, gearConfig.m_DefaultFactionGear.m_bEnableMedicFrags);
		} else 
			Print(string.Format("CRF GEAR SCRIPT ERROR: NO DEFAULT GEAR SET: %1", gearScriptResourceName), LogLevel.ERROR);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdateClothingSlot(array<ResourceName> clothingArray, string clothingStr, string role)
	{
		if(clothingArray.IsEmpty() || clothingStr.IsEmpty())
			return;
		
		int slotInt = ConvertClothingStringToInt(clothingStr);
		
		if(slotInt == -1)
			return;
		
		array<IEntity> removedItems = {};
		IEntity previousClothing = m_Inventory.Get(slotInt);
		ResourceName clothing = clothingArray.GetRandomElement();
		
		if (previousClothing != null)
		{
			BaseInventoryStorageComponent oldStorage = BaseInventoryStorageComponent.Cast(previousClothing.FindComponent(BaseInventoryStorageComponent));
			if(oldStorage)
			{
				array<IEntity> outItems = {};
				oldStorage.GetAll(outItems);
				foreach(IEntity item : outItems)
				{
					if(!item || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)) || SCR_EquipmentStorageComponent.Cast(item.FindComponent(SCR_EquipmentStorageComponent)) ||  SCR_UniversalInventoryStorageComponent.Cast(item.FindComponent(SCR_UniversalInventoryStorageComponent)) || BaseInventoryStorageComponent.Cast(item.FindComponent(BaseInventoryStorageComponent)))
						continue;
					
					m_InventoryManager.TryRemoveItemFromStorage(item, oldStorage);
					removedItems.Insert(item);
				}
			};
			
			m_InventoryManager.TryRemoveItemFromStorage(previousClothing, m_Inventory);
			SCR_EntityHelper.DeleteEntityAndChildren(previousClothing);
		};
		
		if(!clothing.IsEmpty())
		{
			ref IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(clothing), GetGame().GetWorld(), m_SpawnParams);
			m_InventoryManager.TryReplaceItem(resourceSpawned, m_Inventory, slotInt);
			
			if (!m_InventoryManager.Contains(resourceSpawned))
			{
				Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT CLOTHING: %1", clothing), LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", m_InventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print(" ", LogLevel.ERROR);
				Print("CRF GEAR SCRIPT ERROR: INVALID CLOTHING ITEM!", LogLevel.ERROR);
				Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
			};
		}
		
		foreach(IEntity oldItem : removedItems)
			InsertInventoryItem(oldItem, role);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void AddInventoryItem(ResourceName item, int itemAmmount, string role = "", bool enableMedicFrags = false, bool isAssistant = false)
	{	
		if(item.IsEmpty() || itemAmmount <= 0)
			return;
		
		for(int i = 1; i <= itemAmmount; i++)
		{
			ref IEntity resourceSpawned = GetGame().SpawnEntityPrefab(Resource.Load(item), GetGame().GetWorld(), m_SpawnParams);
			
			if(!resourceSpawned)
				continue;
			
			bool isThrowable = (WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)) && WEAPON_TYPES_THROWABLE.Contains(WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType()));
			
			if(!enableMedicFrags && (role == "_Medic_P" || role == "_MO_P") && (isThrowable && WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_FRAGGRENADE))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(resourceSpawned);
				continue;
			};
			
			if(m_SCRInventoryManager.CanInsertItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT))
			{
				m_StorageComp = m_SCRInventoryManager.FindStorageForItem(resourceSpawned, EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
				m_SCRInventoryManager.EquipAny(m_StorageComp, resourceSpawned, -1);
				continue;
			};
			
			InsertInventoryItem(resourceSpawned, role, isAssistant, isThrowable);
			
			if(isThrowable)
			{
				CharacterGrenadeSlotComponent grenadeSlot = CharacterGrenadeSlotComponent.Cast(m_InventoryManager.GetOwner().FindComponent(CharacterGrenadeSlotComponent));
				
				if(grenadeSlot && !grenadeSlot.GetWeaponEntity())
				{
					if(WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_FRAGGRENADE)
					{
						grenadeSlot.SetWeapon(resourceSpawned);
					} else {
						if (WeaponComponent.Cast(resourceSpawned.FindComponent(WeaponComponent)).GetWeaponType() == EWeaponType.WT_SMOKEGRENADE && !grenadeSlot.GetWeaponEntity())
							grenadeSlot.SetWeapon(resourceSpawned);
					};
				};
			};
		}
	}
	
	protected void InsertInventoryItem(IEntity item, string role = "", bool isAssistant = false, bool isThrowable = false)
	{
		if(!item) 
			return;
		
		TIntArray clothingIDs = FilterItemToClothing(item, role, isAssistant, isThrowable);
		
		// Try and insert into select clothing at first
		foreach(int clothingID : clothingIDs)
		{			
			IEntity clothing = m_Inventory.Get(clothingID);

			if(!clothing || m_InventoryManager.Contains(item))
				continue;
				
			BaseInventoryStorageComponent clothingStorage = BaseInventoryStorageComponent.Cast(clothing.FindComponent(BaseInventoryStorageComponent));
			
			if(!clothingStorage)
				continue;
			
			bool successfulInsert = m_InventoryManager.TryInsertItemInStorage(item, clothingStorage);
			
			if(!successfulInsert)
				m_SCRInventoryManager.InsertItemCRF(item, clothingStorage, null, null, false);
		};
			
		// if we cant do select clothing, just slap it in wherever
		if(!m_InventoryManager.Contains(item))
			m_InventoryManager.TryInsertItem(item);
			
		if (!m_InventoryManager.Contains(item) || !InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent)))
		{
			Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
			Print(string.Format("CRF GEAR SCRIPT ERROR: UNABLE TO INSERT ITEM: %1", item.GetPrefabData().GetPrefabName()), LogLevel.ERROR);
			Print(string.Format("CRF GEAR SCRIPT ERROR: INTO ENTITY: %1", m_InventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
			Print(" ", LogLevel.ERROR);
			Print("CRF GEAR SCRIPT ERROR: NOT ENOUGH SPACE IN INVENTORY/INVALID INVENTORY ITEM!", LogLevel.ERROR);
			Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		};
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void AddAssistantMagazines(CRF_GearScriptConfig gearConfig, string role)
	{
		array<ref CRF_Spec_Magazine_Class> magazineArray = {};
		
		switch(role)
		{
			case "_AAR_P"  : {if(!gearConfig.m_FactionWeapons.m_AR  || !gearConfig.m_FactionWeapons.m_AR.m_Weapon)  {return;} magazineArray = gearConfig.m_FactionWeapons.m_AR.m_MagazineArray;  break;}
			case "_AMMG_P" : {if(!gearConfig.m_FactionWeapons.m_MMG || !gearConfig.m_FactionWeapons.m_MMG.m_Weapon) {return;} magazineArray = gearConfig.m_FactionWeapons.m_MMG.m_MagazineArray; break;}
			case "_AHMG_P" : {if(!gearConfig.m_FactionWeapons.m_HMG || !gearConfig.m_FactionWeapons.m_HMG.m_Weapon) {return;} magazineArray = gearConfig.m_FactionWeapons.m_HMG.m_MagazineArray; break;}
			case "_AMAT_P" : {if(!gearConfig.m_FactionWeapons.m_MAT || !gearConfig.m_FactionWeapons.m_MAT.m_Weapon) {return;} magazineArray = gearConfig.m_FactionWeapons.m_MAT.m_MagazineArray; break;}
			case "_AHAT_P" : {if(!gearConfig.m_FactionWeapons.m_HAT || !gearConfig.m_FactionWeapons.m_HAT.m_Weapon) {return;} magazineArray = gearConfig.m_FactionWeapons.m_HAT.m_MagazineArray; break;}
			case "_AAA_P"  : {if(!gearConfig.m_FactionWeapons.m_AA  || !gearConfig.m_FactionWeapons.m_AA.m_Weapon)  {return;} magazineArray = gearConfig.m_FactionWeapons.m_AA.m_MagazineArray;  break;}
			case "_AAT_P"  : {if(!gearConfig.m_FactionWeapons.m_AT  || !gearConfig.m_FactionWeapons.m_AT.m_Weapon)  {return;} magazineArray = gearConfig.m_FactionWeapons.m_AT.m_MagazineArray;  break;}
		}
		
		foreach(ref CRF_Spec_Magazine_Class magazine : magazineArray)
			AddInventoryItem(magazine.m_Magazine, magazine.m_AssistantMagazineCount, role, true);
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void AddWeapons(array<Managed> weaponSlotComponentArray, CRF_GearScriptConfig gearConfig, string weaponType, string atType, bool givePistol)
	{	
		for(int i = 0; i < weaponSlotComponentArray.Count(); i++)
		{
			//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(weaponSlotComponentArray.Get(i));
			array<AttachmentSlotComponent> attatchmentSlotArray = {};
			array<ref CRF_Spec_Magazine_Class> specMagazineArray = {};
			array<ref CRF_Magazine_Class> magazineArray = {};
			array<ResourceName> weaponsAttachments = {};
			ref CRF_Spec_Weapon_Class specWeaponToSpawn;
			ref CRF_Weapon_Class weaponToSpawn;
			IEntity weaponSpawned;
			//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				//Second Primary Assignment
				//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				if(WeaponSlotComponent.Cast(weaponSlotComponentArray.Get((weaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(atType != "")
					{
						switch(atType)
						{
							case "AT"  : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_AT;  break;}
							case "MAT" : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_MAT; break;}
							case "HAT" : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_HAT; break;}	
							case "AA"  : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_AA;  break;}
						}
						
						if(!specWeaponToSpawn || !specWeaponToSpawn.m_Weapon)
							continue; 
						
						weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(specWeaponToSpawn.m_Weapon), GetGame().GetWorld(), m_SpawnParams);
								
						if(!weaponSpawned)
							continue;
						
						weaponsAttachments = specWeaponToSpawn.m_Attachments; 
						specMagazineArray = specWeaponToSpawn.m_MagazineArray; 
						
						foreach(ref CRF_Spec_Magazine_Class magazine : specMagazineArray)
							AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount);

						weaponSlotComponent.SetWeapon(weaponSpawned);
						
						weaponSlotComponent.GetAttachments(attatchmentSlotArray);
						
						if(!weaponsAttachments)
							continue;
						
						if(weaponsAttachments.Count() == 0)
							continue;
				
						foreach(ResourceName attachment : weaponsAttachments)
						{
							foreach(AttachmentSlotComponent attachmentSlot : attatchmentSlotArray)
							{
								IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachment),GetGame().GetWorld(),m_SpawnParams);
								if(attachmentSlot.CanSetAttachment(attachmentSpawned))
								{
									delete attachmentSlot.GetAttachedEntity();
									attachmentSlot.SetAttachment(attachmentSpawned);
									break;
								}
								delete attachmentSpawned;
							} 
						}
					}
					continue;
				}
				
				//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				//First Primary Assignment
				//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
				switch(weaponType)
				{
					case "Rifle"    : {weaponToSpawn = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Rifle);    break;}
					case "RifleUGL" : {weaponToSpawn = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_RifleUGL); break;}
					case "Carbine"  : {weaponToSpawn = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Carbine);  break;}
					case "AR"       : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_AR;                       break;}
					case "MMG"      : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_MMG;                      break;}
					case "HMG"      : {specWeaponToSpawn = gearConfig.m_FactionWeapons.m_HMG;                      break;}
					case "Sniper"   : {weaponToSpawn = gearConfig.m_FactionWeapons.m_Sniper;                       break;}
				}
				
				if(weaponToSpawn && weaponToSpawn.m_Weapon)
				{
					weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(weaponToSpawn.m_Weapon), GetGame().GetWorld(), m_SpawnParams);
						
					weaponsAttachments = weaponToSpawn.m_Attachments; 
					magazineArray = weaponToSpawn.m_MagazineArray; 
					
					foreach(ref CRF_Magazine_Class magazine : magazineArray)
						AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount);
				};
				
				if(specWeaponToSpawn && specWeaponToSpawn.m_Weapon)
				{
					weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(specWeaponToSpawn.m_Weapon), GetGame().GetWorld(), m_SpawnParams);
						
					weaponsAttachments = specWeaponToSpawn.m_Attachments; 
					specMagazineArray = specWeaponToSpawn.m_MagazineArray; 
			
					foreach(ref CRF_Spec_Magazine_Class magazine : specMagazineArray)
						AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount);
				};
				
				if(!weaponSpawned)
					continue;
				
				weaponSlotComponent.SetWeapon(weaponSpawned);
				
				SCR_CharacterControllerComponent.Cast(m_InventoryManager.GetOwner().FindComponent(SCR_CharacterControllerComponent)).SelectWeapon(weaponSlotComponent);
				
				weaponSlotComponent.GetAttachments(attatchmentSlotArray);
				
				if(!weaponsAttachments)
					continue;
				
				if(weaponsAttachments.Count() == 0)
					continue;
				
				foreach(ResourceName attachment : weaponsAttachments)
				{
					foreach(AttachmentSlotComponent attachmentSlot : attatchmentSlotArray)
					{
						IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachment), GetGame().GetWorld(), m_SpawnParams);
						if(attachmentSlot.CanSetAttachment(attachmentSpawned))
						{
							delete attachmentSlot.GetAttachedEntity();
							attachmentSlot.SetAttachment(attachmentSpawned);
							break;
						}
						delete attachmentSpawned;
					} 
				}
			}
		
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			//Pistol
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
			if(weaponSlotComponent.GetWeaponSlotType() == "secondary" && givePistol == true)
			{
				weaponToSpawn = SelectRandomWeapon(gearConfig.m_FactionWeapons.m_Pistol);  
				
				if(!weaponToSpawn || !weaponToSpawn.m_Weapon)
					continue; 
				
				weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(weaponToSpawn.m_Weapon), GetGame().GetWorld(), m_SpawnParams);
								
				if(!weaponSpawned)
					continue;
						
				weaponsAttachments = weaponToSpawn.m_Attachments; 
				magazineArray = weaponToSpawn.m_MagazineArray;

				foreach(ref CRF_Magazine_Class magazine : magazineArray)
					AddInventoryItem(magazine.m_Magazine, magazine.m_MagazineCount);
				
				if(!weaponSpawned)
					continue;
	
				weaponSlotComponent.SetWeapon(weaponSpawned);
				weaponSlotComponent.GetAttachments(attatchmentSlotArray);
				
				if(!weaponsAttachments)
					continue;
				
				if(weaponsAttachments.Count() == 0)
					continue;
				
				foreach(ResourceName attachment : weaponToSpawn.m_Attachments)
				{
					foreach(AttachmentSlotComponent attachmentSlot : attatchmentSlotArray)
					{
						IEntity attachmentSpawned = GetGame().SpawnEntityPrefab(Resource.Load(attachment), GetGame().GetWorld(), m_SpawnParams);
						if(attachmentSlot.CanSetAttachment(attachmentSpawned))
						{
							delete attachmentSlot.GetAttachedEntity();
							attachmentSlot.SetAttachment(attachmentSpawned);
							break;
						}
						delete attachmentSpawned;
					} 
				}	
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdateLeadershipCustomGear(array<ref CRF_Leadership_Custom_Gear> customGearArray, string role)
	{
		switch(role)
		{
			case "_COY_P"          : {role = "Company Commander"; break;}
			case "_PL_P"           : {role = "Platoon Leader";    break;}
			case "_MO_P"           : {role = "Medical Officer";   break;}
			case "_SL_P"           : {role = "Squad Lead";        break;}
			case "_FO_P"           : {role = "Forward Observer";  break;}
			case "_JTAC_P"         : {role = "JTAC";              break;}
			case "_VehLead_P"      : {role = "Vehicle Lead";      break;}
			case "_IndirectLead_P" : {role = "Indirect Lead";     break;}
			case "_LogiLead_P"     : {role = "Logi Lead";         break;}
		}	
		
		foreach(ref CRF_Leadership_Custom_Gear customGear : customGearArray)
		{	
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType, role);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, role);
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdateSquadLevelCustomGear(array<ref CRF_Squad_Level_Custom_Gear> customGearArray, string role)
	{
		switch(role)
		{
			case "_TL_P"       : {role = "Team Lead";                    break;}
			case "_Gren_P"     : {role = "Grenadier";                    break;}
			case "_RTO_P"      : {role = "Radio Telephone Operator";     break;}
			case "_Rifleman_P" : {role = "Rifleman";                     break;}
			case "_Demo_P"     : {role = "Rifleman Demo";                break;}
			case "_AT_P"       : {role = "Rifleman AntiTank";            break;}
			case "_AAT_P"      : {role = "Assistant Rifleman AntiTank";  break;}
			case "_AR_P"       : {role = "Automatic Rifleman";           break;}
			case "_AAR_P"      : {role = "Assistant Automatic Rifleman"; break;}
			case "_Medic_P"    : {role = "Medic";                        break;}
		}
		
		foreach(ref CRF_Squad_Level_Custom_Gear customGear : customGearArray)
		{		
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType, role);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, role);
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdateInfantrySpecialtiesCustomGear(array<ref CRF_Infantry_Specialties_Custom_Gear> customGearArray, string role)
	{
		switch(role)
		{
			case "_HAT_P"     : {role = "Heavy AntiTank";              break;}
			case "_AHAT_P"    : {role = "Assistant Heavy AntiTank";    break;}
			case "_MAT_P"     : {role = "Medium AntiTank";             break;}
			case "_AMAT_P"    : {role = "Assistant Medium AntiTank";   break;}
			case "_HMG_P"     : {role = "Heavy MachineGun";            break;}
			case "_AHMG_P"    : {role = "Assistant Heavy MachineGun";  break;}
			case "_MMG_P"     : {role = "Medium MachineGun";           break;}
			case "_AMMG_P"    : {role = "Assistant Medium MachineGun"; break;}
			case "_AA_P"      : {role = "Anit-Air";                    break;}
			case "_AAA_P"     : {role = "Assistant Anit-Air";          break;}
			case "_Sniper_P"  : {role = "Sniper";                      break;}
			case "_Spotter_P" : {role = "Spotter";                     break;}
		}
		
		foreach(ref CRF_Infantry_Specialties_Custom_Gear customGear : customGearArray)
		{	
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType, role);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, role);
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected void UpdateVehicleSpecialtiesCustomGear(array<ref CRF_Vehicle_Specialties_Custom_Gear> customGearArray, string role)
	{
		switch(role)
		{
			case "_VehDriver_P"      : {role = "Vehicle Driver";  break;}
			case "_VehGunner_P"      : {role = "Vehicle Gunner";  break;}
			case "_VehLoader_P"      : {role = "Vehicle Loader";  break;}
			case "_Pilot_P"          : {role = "Pilot";           break;}
			case "_CrewChief_P"      : {role = "Crew Chief";      break;}
			case "_LogiRunner_P"     : {role = "Logi Runner";     break;}
			case "_IndirectGunner_P" : {role = "Indirect Gunner"; break;}
			case "_IndirectLoader_P" : {role = "Indirect Loader"; break;}
		}
		
		foreach(ref CRF_Vehicle_Specialties_Custom_Gear customGear : customGearArray)
		{
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType, role);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItem(item.m_sItemPrefab, item.m_iItemCount, role);
		}
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	int ConvertClothingStringToInt(string clothingStr)
	{
		int slotInt = -1;
		
		// All the arrays belong to us
		switch(clothingStr)
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
		
		return slotInt;
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected CRF_Weapon_Class SelectRandomWeapon(array<ref CRF_Weapon_Class> weaponArray)
	{
		if(!weaponArray || weaponArray.IsEmpty())
			return null; 
								
		ref CRF_Weapon_Class weaponToSpawn = weaponArray.GetRandomElement();
								
		if(!weaponToSpawn)
			return null; 
								
		return weaponToSpawn;
	}
	
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	TIntArray FilterItemToClothing(IEntity item, string role = "", bool isAssistant = false, bool isThrowable = false)
	{
		ref array<int> clothingIDs = {};
		
		// Any magazine
		if(MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)))
			clothingIDs = {VEST, ARMOREDVEST, BACKPACK, PANTS, SHIRT};
		else // Any Non-magazine
			clothingIDs = {SHIRT, PANTS, VEST, ARMOREDVEST, BACKPACK};
			
		// Any medical item
		if((role == "_Medic_P" || role == "_MO_P") && SCR_ConsumableItemComponent.Cast(item.FindComponent(SCR_ConsumableItemComponent)))
			clothingIDs = {BACKPACK, VEST, ARMOREDVEST};
		
		// Any pistol ammo
		if((InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)) && InventoryMagazineComponent.Cast(item.FindComponent(InventoryMagazineComponent)).GetAttributes().GetCommonType() == ECommonItemType.RHS_PISTOL_AMMO) || isThrowable)
			clothingIDs = {PANTS, VEST, ARMOREDVEST, BACKPACK};
		
		// Any radio
		if(BaseRadioComponent.Cast(item.FindComponent(BaseRadioComponent)))
			clothingIDs = {PANTS, SHIRT, VEST, ARMOREDVEST, BACKPACK};
		
		// Any Assistant Mags
		if(isAssistant && MagazineComponent.Cast(item.FindComponent(MagazineComponent)))
			clothingIDs = {BACKPACK, VEST, ARMOREDVEST};
		
		// Check if item is explosives related
		SCR_DetonatorGadgetComponent detonator = SCR_DetonatorGadgetComponent.Cast(item.FindComponent(SCR_DetonatorGadgetComponent));
		SCR_ExplosiveChargeComponent explosives = SCR_ExplosiveChargeComponent.Cast(item.FindComponent(SCR_ExplosiveChargeComponent));
		SCR_MineWeaponComponent mine = SCR_MineWeaponComponent.Cast(item.FindComponent(SCR_MineWeaponComponent));
		SCR_RepairSupportStationComponent engTool = SCR_RepairSupportStationComponent.Cast(item.FindComponent(SCR_RepairSupportStationComponent));
		SCR_HealSupportStationComponent medTool = SCR_HealSupportStationComponent.Cast(item.FindComponent(SCR_HealSupportStationComponent));
		if(detonator || explosives || mine || engTool || medTool)
			clothingIDs = {BACKPACK, VEST, ARMOREDVEST};
		
		return clothingIDs;
	}
}
