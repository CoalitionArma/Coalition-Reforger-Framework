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
	
	const ref TStringArray m_aLeadershipRolesUGL = {"COY_P", "PL_P", "SL_P"};
	const ref TStringArray m_aLeadershipRolesCarbine = {"MO_P", "IndirectLead_P", "LogiLead_P", "VehLead_P"};
		
	const ref TStringArray m_aSquadLevelRolesUGL = {"TL_P", "Gren_P"};
	const ref TStringArray m_aSquadLevelRolesRifle = {"Rifleman_P", "Demo_P", "AAT_P", "AAR_P"};
	const ref TStringArray m_aSquadLevelRolesCarbine = {"Medic_P"};
		
	const ref TStringArray m_aInfantrySpecialtiesRolesRifle = {"AHAT_P", "AMAT_P", "AHMG_P", "AMMG_P", "AAA_P", "Spotter_P", "FO_P", "JTAC_P"};
		
	const ref TStringArray m_aVehicleSpecialtiesRolesCarbine = {"VehDriver_P", "VehGunner_P", "VehLoader_P", "LogiRunner_P", "IndirectGunner_P", "IndirectLoader_P"};
	const ref TStringArray m_aVehicleSpecialtiesRolesPistol = {"Pilot_P", "CrewChief_P"};
	
	protected SCR_CharacterInventoryStorageComponent m_Inventory;
	protected InventoryStorageManagerComponent m_InventoryManager;
	protected CRF_GearScriptConfig m_GearConfig;
	protected ref EntitySpawnParams m_SpawnParams = new EntitySpawnParams();
	protected ref array<Managed> m_WeaponSlotComponentArray = {};
	
	protected ref RandomGenerator m_RNG = new RandomGenerator;
	
	void WaitTillGameStart(IEntity entity)
	{
		if(!GetGame().GetWorld())
		{
			GetGame().GetCallqueue().CallLater(WaitTillGameStart, 100, false, entity);
			return;
		}
		
		GetGame().GetCallqueue().CallLater(AddGearToEntity, m_RNG.RandInt(100, 5000), false, entity);
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
			UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
		
		//------------------------------------------------------------------------------------------------
		// CUSTOM GEAR
		//------------------------------------------------------------------------------------------------
		
		array<string> value = {};
		ResourceNameToScan.Split("_", value, true);
		
		string role = value[3] + "_" + value[4];
		
		role.Split(".", value, true);
		role = value[0];
		
		bool isLeader = false;
		bool isSquad = false;
		bool isInfSpec = false;
		bool isVehSpec = false;
		
		switch(true)
		{
			case(m_aLeadershipRolesUGL.Contains(role))             : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", true);  isLeader = true; break;}
			case(m_aLeadershipRolesCarbine.Contains(role))         : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", true);   isLeader = true; break;}	
			
			case(m_aSquadLevelRolesUGL.Contains(role))             : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "RifleUGL", "", false); isSquad = true; break;}
			case(m_aSquadLevelRolesCarbine.Contains(role))         : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);  isSquad = true; break;}
			case(m_aSquadLevelRolesRifle.Contains(role))           : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);    isSquad = true;   AddAssistantMagazines(GearConfig, role); break;}
			case(role == "AT_P")                                   : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "AT", false);  isSquad = true; break;}
			case(role == "AR_P")                                   : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "AR", "", false);       isSquad = true; break;}
	
			case(m_aInfantrySpecialtiesRolesRifle.Contains(role))  : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "", false);    isInfSpec = true; AddAssistantMagazines(GearConfig, role); break;}
			case(role == "HAT_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "HAT", false); isInfSpec = true; break;}
			case(role == "MAT_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "MAT", false); isInfSpec = true; break;}
			case(role == "HMG_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "HMG", "", true);       isInfSpec = true; break;}
			case(role == "MMG_P")                                  : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "MMG", "", true);       isInfSpec = true; break;}
			case(role == "AA_P")                                   : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Rifle", "AA", false);  isInfSpec = true; break;}
			case(role == "Sniper_P")                               : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Sniper", "", true);    isInfSpec = true; break;}
			
			case(m_aVehicleSpecialtiesRolesCarbine.Contains(role)) : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "Carbine", "", false);  isVehSpec = true; break;}
			case(m_aVehicleSpecialtiesRolesPistol.Contains(role))  : {AddWeapons(m_WeaponSlotComponentArray, GearConfig, "", "", true);          isVehSpec = true; break;}
		}
		
		switch(true)
		{
			case(isLeader)  : {UpdateLeadershipCustomGear(GearConfig.m_CustomGear.m_LeadershipCustomGear, role);                   break;}
			case(isSquad)   : {UpdateSquadLevelCustomGear(GearConfig.m_CustomGear.m_SquadLevelCustomGear, role);                   break;}
			case(isInfSpec) : {UpdateInfantrySpecialtiesCustomGear(GearConfig.m_CustomGear.m_InfantrySpecialtiesCustomGear, role); break;}
			case(isVehSpec) : {UpdateVehicleSpecialtiesCustomGear(GearConfig.m_CustomGear.m_VehicleSpecialtiesCustomGear, role);   break;}
		}
		
		//------------------------------------------------------------------------------------------------
		// ITEMS
		//------------------------------------------------------------------------------------------------
		
		foreach(CRF_Inventory_Item item : GearConfig.m_DefaultGear.m_DefaultInventoryItems)
			AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
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
			Print(string.Format("CRF GEAR SCRIPT : UNABLE TO INSERT CLOTHING: %1", clothing), LogLevel.ERROR);
			Print(string.Format("CRF GEAR SCRIPT : INTO ENTITY: %1", m_InventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
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
				Print(string.Format("CRF GEAR SCRIPT : UNABLE TO INSERT ITEM: %1", item), LogLevel.ERROR);
				Print(string.Format("CRF GEAR SCRIPT : INTO ENTITY: %1", m_InventoryManager.GetOwner().GetPrefabData().GetPrefabName()), LogLevel.ERROR);
				Print("-------------------------------------------------------------------------------------------------------------", LogLevel.ERROR);
				delete resourceSpawned;
			};
		}
	}
	
	protected void AddAssistantMagazines(CRF_GearScriptConfig GearConfig, string WeaponType)
	{
		Print("Giving Assistant mags");
		Print(WeaponType);
		array<ref CRF_Magazine_Class> MagazineArray = {};
		switch(WeaponType)
		{
			case "AAR_P" : {if(!GearConfig.m_Weapons.m_AR){return;} if(!GearConfig.m_Weapons.m_AR.m_Weapon){return;} MagazineArray = GearConfig.m_Weapons.m_AR.m_MagazineArray; break;}
			case "AMMG_P" : {if(!GearConfig.m_Weapons.m_MMG){return;} if(!GearConfig.m_Weapons.m_MMG.m_Weapon){return;} MagazineArray = GearConfig.m_Weapons.m_MMG.m_MagazineArray; break;}
			case "AHMG_P" : {if(!GearConfig.m_Weapons.m_HMG){return;} if(!GearConfig.m_Weapons.m_HMG.m_Weapon){return;} MagazineArray = GearConfig.m_Weapons.m_HMG.m_MagazineArray; break;}
			case "AMAT_P" : {if(!GearConfig.m_Weapons.m_MAT){return;} if(!GearConfig.m_Weapons.m_MAT.m_Weapon){return;} MagazineArray = GearConfig.m_Weapons.m_MAT.m_MagazineArray; break;}
			case "AHAT_P" : {if(!GearConfig.m_Weapons.m_HAT){return;} if(!GearConfig.m_Weapons.m_HAT.m_Weapon){return;} MagazineArray = GearConfig.m_Weapons.m_HAT.m_MagazineArray; break;}
			case "AAA_P" : {if(!GearConfig.m_Weapons.m_AA){return;} if(!GearConfig.m_Weapons.m_AA.m_Weapon){return;} MagazineArray = GearConfig.m_Weapons.m_AA.m_MagazineArray; break;}
		}
		
		foreach(ref CRF_Magazine_Class Magazine : MagazineArray)
		{
			AddInventoryItems(Magazine.m_Magazine, Magazine.m_AssistantMagazineCount);
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
			array<ref CRF_Magazine_Class> MagazineArray = {};
			
			//AT Assignment
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(WeaponSlotComponentArray.Get((WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(ATType != "")
					{
						switch(ATType)
						{
							case "AT"   : {if(!GearConfig.m_Weapons.m_AT){continue;} if(!GearConfig.m_Weapons.m_AT.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AT.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_AT.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_AT.m_MagazineArray;       break;}
							case "MAT"  : {if(!GearConfig.m_Weapons.m_MAT){continue;} if(!GearConfig.m_Weapons.m_MAT.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_MAT.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_MAT.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_MAT.m_MagazineArray;  break;}
							case "HAT"  : {if(!GearConfig.m_Weapons.m_HAT){continue;} if(!GearConfig.m_Weapons.m_HAT.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_HAT.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_HAT.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_HAT.m_MagazineArray;  break;}
							case "AA"   : {if(!GearConfig.m_Weapons.m_AA){continue;} if(!GearConfig.m_Weapons.m_AA.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AA.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_AA.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_AA.m_MagazineArray;       break;}
						}
						if(MagazineArray.Count() != 0)
						{
							foreach(CRF_Magazine_Class Magazine : MagazineArray)
							{
								AddInventoryItems(Magazine.m_Magazine, Magazine.m_MagazineCount);
							}
						}
						
						if(!weaponSpawned)
							continue;

						weaponSlotComponent.SetWeapon(weaponSpawned);
						
						weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
						if(!WeaponsAttachments)
							continue;
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
				//RIfle Assignment
				switch(WeaponType)
				{
					case "Rifle"     : {if(!GearConfig.m_Weapons.m_Rifle){continue;} if(!GearConfig.m_Weapons.m_Rifle.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Rifle.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Rifle.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_Rifle.m_MagazineArray;                 break;}
					case "RifleUGL"  : {if(!GearConfig.m_Weapons.m_RifleUGL){continue;} if(!GearConfig.m_Weapons.m_RifleUGL.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_RifleUGL.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_RifleUGL.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_RifleUGL.m_MagazineArray;  break;}
					case "Carbine"   : {if(!GearConfig.m_Weapons.m_Carbine){continue;} if(!GearConfig.m_Weapons.m_Carbine.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Carbine.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Carbine.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_Carbine.m_MagazineArray;       break;}
					case "AR"        : {if(!GearConfig.m_Weapons.m_AR){continue;} if(!GearConfig.m_Weapons.m_AR.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_AR.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_AR.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_AR.m_MagazineArray;                                break;}
					case "MMG"       : {if(!GearConfig.m_Weapons.m_MMG){continue;} if(!GearConfig.m_Weapons.m_MMG.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_MMG.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_MMG.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_MMG.m_MagazineArray;                           break;}
					case "HMG"       : {if(!GearConfig.m_Weapons.m_HMG){continue;} if(!GearConfig.m_Weapons.m_HMG.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_HMG.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_HMG.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_HMG.m_MagazineArray;                           break;}
					case "Sniper"    : {if(!GearConfig.m_Weapons.m_Sniper){continue;} if(!GearConfig.m_Weapons.m_Sniper.m_Weapon){continue;} weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Sniper.m_Weapon),GetGame().GetWorld(),m_SpawnParams); WeaponsAttachments = GearConfig.m_Weapons.m_Sniper.m_Attachments; MagazineArray = GearConfig.m_Weapons.m_Sniper.m_MagazineArray;            break;}
				}
				if(MagazineArray.Count() != 0)
				{
					foreach(CRF_Magazine_Class Magazine : MagazineArray)
					{
						AddInventoryItems(Magazine.m_Magazine, Magazine.m_MagazineCount);
					}
				}

				if(!weaponSpawned)
					continue;
				
				weaponSlotComponent.SetWeapon(weaponSpawned);
				
				weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
					if(!WeaponsAttachments)
						continue;
					if(WeaponsAttachments.Count() == 0)
						continue;
				
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
			
			//Pistol
			if(weaponSlotComponent.GetWeaponSlotType() == "secondary" && GivePistol == true)
			{
				if(!GearConfig.m_Weapons.m_Pistol)
					continue;
				
				if(!GearConfig.m_Weapons.m_Pistol.m_Weapon)
					continue;
				
				weaponSpawned = GetGame().SpawnEntityPrefab(Resource.Load(GearConfig.m_Weapons.m_Pistol.m_Weapon),GetGame().GetWorld(),m_SpawnParams);
				MagazineArray = GearConfig.m_Weapons.m_Pistol.m_MagazineArray;
				if(MagazineArray.Count() != 0)
				{
					Print("Giving Pistol Mags");
					foreach(CRF_Magazine_Class Magazine : MagazineArray)
					{
						Print(Magazine.m_Magazine);
						Print(Magazine.m_MagazineCount);
						AddInventoryItems(Magazine.m_Magazine, Magazine.m_MagazineCount);
					}
				}
				
				if(!weaponSpawned)
					continue;
	
				weaponSlotComponent.SetWeapon(weaponSpawned);
				
				weaponSlotComponent.GetAttachments(AttatchmentSlotArray);
					if(!WeaponsAttachments)
						continue;
					if(WeaponsAttachments.Count() == 0)
						continue;
				
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
		Print(role);
		switch(role)
		{
			case "COY_P"          : {role = "Company Commander"; break;}
			case "PL_P"           : {role = "Platoon Leader"; break;}
			case "MO_P"           : {role = "Medical Officer";   break;}
			case "SL_P"           : {role = "Squad Lead";        break;}
			case "VehLead_P"      : {role = "Vehicle Lead";      break;}
			case "IndirectLead_P" : {role = "Indirect Lead";     break;}
			case "LogiLead_P"     : {role = "Logi Lead";         break;}
		}	
		
		foreach(ref CRF_Leadership_Custom_Gear customGear : customGearArray)
		{	
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
		}
	}
	
	protected void UpdateSquadLevelCustomGear(array<ref CRF_Squad_Level_Custom_Gear> customGearArray, string role)
	{
		Print(role);
		switch(role)
		{
			case "TL_P"       : {role = "Team Lead";                    break;}
			case "Gren_P"     : {role = "Grenadier";                    break;}
			case "Rifleman_P" : {role = "Rifleman";                     break;}
			case "Demo_P"     : {role = "Rifleman Demo";                break;}
			case "AT_P"       : {role = "Rifleman AntiTank";            break;}
			case "AAT_P"      : {role = "Assistant Rifleman AntiTank";  break;}
			case "AR_P"       : {role = "Automatic Rifleman";           break;}
			case "AAR_P"      : {role = "Assistant Automatic Rifleman"; break;}
			case "Medic_P"    : {role = "Medic";                        break;}
		}
		
		foreach(ref CRF_Squad_Level_Custom_Gear customGear : customGearArray)
		{		
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
		}
	}
	
	protected void UpdateInfantrySpecialtiesCustomGear(array<ref CRF_Infantry_Specialties_Custom_Gear> customGearArray, string role)
	{
		Print(role);
		switch(role)
		{
			case "HAT_P"     : {role = "Heavy AntiTank";              break;}
			case "AHAT_P"    : {role = "Assistant Heavy AntiTank";    break;}
			case "MAT_P"     : {role = "Medium AntiTank";             break;}
			case "AMAT_P"    : {role = "Assistant Medium AntiTank";   break;}
			case "HMG_P"     : {role = "Heavy MachineGun";            break;}
			case "AHMG_P"    : {role = "Assistant Heavy MachineGun";  break;}
			case "MMG_P"     : {role = "Medium MachineGun";           break;}
			case "AMMG_P"    : {role = "Assistant Medium MachineGun"; break;}
			case "AA_P"      : {role = "Anit-Air";                    break;}
			case "AAA_P"     : {role = "Assistant Anit-Air";          break;}
			case "Sniper_P"  : {role = "Sniper";                      break;}
			case "Spotter_P" : {role = "Spotter";                     break;}
			case "FO_P"      : {role = "Forward Observer";            break;}
			case "JTAC_P"    : {role = "JTAC";                        break;}
		}
		
		foreach(ref CRF_Infantry_Specialties_Custom_Gear customGear : customGearArray)
		{	
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
		}
	}
	
	protected void UpdateVehicleSpecialtiesCustomGear(array<ref CRF_Vehicle_Specialties_Custom_Gear> customGearArray, string role)
	{
		Print(role);
		switch(role)
		{
			case "VehDriver_P"      : {role = "Vehicle Driver";  break;}
			case "VehGunner_P"      : {role = "Vehicle Gunner";  break;}
			case "VehLoader_P"      : {role = "Vehicle Loader";  break;}
			case "Pilot_P"          : {role = "Pilot";           break;}
			case "CrewChief_P"      : {role = "Crew Chief";      break;}
			case "LogiRunner_P"     : {role = "Logi Runner";     break;}
			case "IndirectGunner_P" : {role = "Indirect Gunner"; break;}
			case "IndirectLoader_P" : {role = "Indirect Loader"; break;}
		}
		
		foreach(ref CRF_Vehicle_Specialties_Custom_Gear customGear : customGearArray)
		{
			if(customGear.m_sRoleToOverride != role)
				continue;
				
			foreach(CRF_Clothing clothing : customGear.m_CustomClothing)
				UpdateClothingSlot(clothing.m_ClothingPrefabs, clothing.m_sClothingType);
					
			foreach(CRF_Inventory_Item item : customGear.m_AdditionalInventoryItems)
				AddInventoryItems(item.m_sItemPrefab, item.m_iItemCount);
		}
	}
}