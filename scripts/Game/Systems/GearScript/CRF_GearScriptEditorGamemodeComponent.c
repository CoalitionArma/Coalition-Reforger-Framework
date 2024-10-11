[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GearScriptEditorGamemodeComponentClass: SCR_BaseGameModeComponentClass {}

class CRF_GearScriptEditorGamemodeComponent: SCR_BaseGameModeComponent
{	
	ref CRF_GearScriptConfig m_masterConfig;
	ref CRF_Default_Gear m_defaultGearConfig;
	ref CRF_Weapons m_weaponsConfig;
	ref array<ref CRF_Clothing> m_clothingClassArray = {};
	ref array<ref CRF_Weapon_Class> m_riflesClassArray = {};
	ref array<ref CRF_Weapon_Class> m_rifleUGLClassArray = {};
	ref array<ref CRF_Weapon_Class> m_carbinesClassArray = {};
	ref array<ref CRF_Weapon_Class> m_pistolsClassArray = {};
	string savedGearString;
	ref CRF_Spec_Weapon_Class m_ARClass;
	ref CRF_Spec_Weapon_Class m_MMGClass;
	ref CRF_Spec_Weapon_Class m_HMGClass;
	ref CRF_Spec_Weapon_Class m_ATClass;
	ref CRF_Spec_Weapon_Class m_MATClass;
	ref CRF_Spec_Weapon_Class m_HATClass;
	ref CRF_Spec_Weapon_Class m_AAClass;
	ref CRF_Weapon_Class m_sniperClass;
	ref array<array<ResourceName>>m_resourceGearArray;
	protected ref array<ResourceName> m_resourceHelmetsArray = {};
	protected ref array<ResourceName> m_resourceShirtsArray = {};
	protected ref array<ResourceName> m_resourceArmoredVestArray = {};
	protected ref array<ResourceName> m_resourcePantsArray = {};
	protected ref array<ResourceName> m_resourceBootsArray = {};
	protected ref array<ResourceName> m_resourceBackpackArray = {};
	protected ref array<ResourceName> m_resourceVestArray = {};
	protected ref array<ResourceName> m_resourceHandwearArray = {};
	protected ref array<ResourceName> m_resourceHeadArray = {};
	protected ref array<ResourceName> m_resourceEyesArray = {};
	protected ref array<ResourceName> m_resourceEarsArray = {};
	protected ref array<ResourceName> m_resourceFaceArray = {};
	protected ref array<ResourceName> m_resourceNeckArray = {};
	protected ref array<ResourceName> m_resourceExtra1Array = {};
	protected ref array<ResourceName> m_resourceExtra2Array = {};
	protected ref array<ResourceName> m_resourceWaistArray = {};
	protected ref array<ResourceName> m_resourceExtra3Array = {};
	protected ref array<ResourceName> m_resourceExtra4Array = {};
	protected ref array<ResourceName> m_resourceARPrefab = {};
	protected ref array<ResourceName> m_resourceMMGPrefab = {};
	protected ref array<ResourceName> m_resourceHMGPrefab = {};
	protected ref array<ResourceName> m_resourceATPrefab = {};
	protected ref array<ResourceName> m_resourceMATPrefab = {};
	protected ref array<ResourceName> m_resourceHATPrefab = {};
	protected ref array<ResourceName> m_resourceAAPrefab = {};
	protected ref array<ResourceName> m_resourceSniperPrefab = {};
	
	protected ref array<ResourceName> m_resourceRiflesArray = {};
	protected ref array<ResourceName> m_resourceRifleUGLArray = {};
	protected ref array<ResourceName> m_resourceCarbinesArray = {};
	protected ref array<ResourceName> m_resourcePistolsArray = {};
	
	void SaveGear(array<array<string>> gearArray)
	{
		Print(gearArray);
		array<string> finalGearArray = {};
		foreach(array<string> innerGearArray : gearArray)
		{
			ref array<string> gearArrayString = {};
			foreach(string gear : innerGearArray)
			{
				string gearString = gear;
				gearArrayString.Insert(gear);
			}
			string gearString = SCR_StringHelper.Join(",", gearArrayString, true);
			finalGearArray.Insert(gearString);
		}
		savedGearString = SCR_StringHelper.Join("|", finalGearArray, true);
		Print(savedGearString);
	}
	
	string GetGear()
	{
		return savedGearString;
	}
	void ExportGearScript(array<array<string>> gearArray)
	{
		m_masterConfig = new CRF_GearScriptConfig();
		m_defaultGearConfig = new CRF_Default_Gear();
		m_weaponsConfig = new CRF_Weapons;
		m_clothingClassArray = {};
		m_riflesClassArray = {};
		m_rifleUGLClassArray = {};
		m_carbinesClassArray = {};
		m_pistolsClassArray = {};
		m_ARClass = null;
		m_MMGClass = null;
		m_HMGClass = null;
		m_ATClass = null;
		m_MATClass = null;
		m_HATClass = null;
		m_AAClass = null;
		m_sniperClass = null;
		
		

		//------------------------------------Headgear------------------------------------------
		ConvertStringArraystoResourceName(gearArray[0], "HEADGEAR");
		AddClothingToConfig(m_resourceHelmetsArray, "HEADGEAR");
		
		//------------------------------------Shirts--------------------------------------------
		ConvertStringArraystoResourceName(gearArray[1], "SHIRT");
		AddClothingToConfig(m_resourceShirtsArray, "SHIRT");

		//------------------------------------Armored Vests-------------------------------------
		ConvertStringArraystoResourceName(gearArray[2], "ARMOREDVEST");
		AddClothingToConfig(m_resourceArmoredVestArray, "ARMOREDVEST");

		//------------------------------------Pants---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[3], "PANTS");
		AddClothingToConfig(m_resourcePantsArray, "PANTS");

		//------------------------------------Boots---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[4], "BOOTS");
		AddClothingToConfig(m_resourceBootsArray, "BOOTS");

		//------------------------------------BACKPACK---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[5], "BACKPACK");
		AddClothingToConfig(m_resourceBackpackArray, "BACKPACK");
		
		//------------------------------------VEST---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[6], "VEST");
		AddClothingToConfig(m_resourceVestArray, "VEST");
		
		//------------------------------------HANDWEAR---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[7], "HANDWEAR");
		AddClothingToConfig(m_resourceHandwearArray, "HANDWEAR");
		
		//------------------------------------HEAD---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[8], "HEAD");
		AddClothingToConfig(m_resourceHeadArray, "HEAD");
		
		//------------------------------------EYES---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[9], "EYES");
		AddClothingToConfig(m_resourceEyesArray, "EYES");
		
		//------------------------------------EARS---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[10], "EARS");
		AddClothingToConfig(m_resourceEarsArray, "EARS");
		
		//------------------------------------FACE---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[11], "FACE");
		AddClothingToConfig(m_resourceFaceArray, "FACE");
		
		//------------------------------------NECK---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[12], "NECK");
		AddClothingToConfig(m_resourceNeckArray, "NECK");
		
		//------------------------------------EXTRA1---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[13], "EXTRA1");
		AddClothingToConfig(m_resourceExtra1Array, "EXTRA1");
		
		//------------------------------------EXTRA2---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[14], "EXTRA2");
		AddClothingToConfig(m_resourceExtra2Array, "EXTRA2");
		
		//------------------------------------WASTE---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[15], "WAIST");
		AddClothingToConfig(m_resourceWaistArray, "WAIST");
		
		//------------------------------------EXTRA3---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[16], "EXTRA3");
		AddClothingToConfig(m_resourceExtra3Array, "EXTRA3");
		
		//------------------------------------EXTRA4---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[17], "EXTRA4");
		AddClothingToConfig(m_resourceExtra4Array, "EXTRA4");
		
		//------------------------------------Rifles---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[18], "Rifle");
		AddRegularWeaponsToConfig(m_resourceRiflesArray, "Rifle");
		
		//------------------------------------RifleUGL-------------------------------------------
		ConvertStringArraystoResourceName(gearArray[19], "RifleUGL");
		AddRegularWeaponsToConfig(m_resourceRifleUGLArray, "RifleUGL");
		
		//------------------------------------Carbines-------------------------------------------
		ConvertStringArraystoResourceName(gearArray[20], "Carbine");
		AddRegularWeaponsToConfig(m_resourceCarbinesArray, "Carbine");
		
		//------------------------------------Pistols--------------------------------------------
		ConvertStringArraystoResourceName(gearArray[21], "Pistol");
		AddRegularWeaponsToConfig(m_resourcePistolsArray, "Pistol");
		
		//------------------------------------AR-------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[22], "AR");
		AddSpecWeaponToConfig(m_resourceARPrefab, "AR");
		
		//------------------------------------MMG------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[23], "MMG");
		AddSpecWeaponToConfig(m_resourceMMGPrefab, "MMG");
		
		//------------------------------------HMG------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[24], "HMG");
		AddSpecWeaponToConfig(m_resourceHMGPrefab, "HMG");
		
		//------------------------------------AT-------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[25], "AT");
		AddSpecWeaponToConfig(m_resourceATPrefab, "AT");
		
		//------------------------------------MAT------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[26], "MAT");
		AddSpecWeaponToConfig(m_resourceMATPrefab, "MAT");
		
		//------------------------------------HAT------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[27], "HAT");
		AddSpecWeaponToConfig(m_resourceHATPrefab, "HAT");
		
		//------------------------------------AA-------------------------------------------------
		ConvertStringArraystoResourceName(gearArray[28], "AA");
		AddSpecWeaponToConfig(m_resourceAAPrefab, "AA");
		
		//------------------------------------Sniper---------------------------------------------
		ConvertStringArraystoResourceName(gearArray[29], "Sniper");
		AddSpecWeaponToConfig(m_resourceSniperPrefab, "Sniper");
		
		SaveToConfig();
		
	}
	
	void SaveToConfig()
	{
		m_masterConfig.m_DefaultGear = m_defaultGearConfig;
		m_masterConfig.m_Weapons = m_weaponsConfig;
		
		if(m_clothingClassArray.Count() > 0)
		{
			m_defaultGearConfig.m_DefaultClothing = m_clothingClassArray;
		}
		
		if(m_riflesClassArray.Count() > 0)
		{
			m_masterConfig.m_Weapons.m_Rifle = m_riflesClassArray;
		}
		
		if(m_rifleUGLClassArray.Count() > 0)
		{
			m_masterConfig.m_Weapons.m_RifleUGL = m_rifleUGLClassArray;
		}
		
		if(m_carbinesClassArray.Count() > 0)
		{
			m_masterConfig.m_Weapons.m_Carbine = m_carbinesClassArray;
		}
		
		if(m_pistolsClassArray.Count() > 0)
		{
			m_masterConfig.m_Weapons.m_Pistol = m_pistolsClassArray;
		}
		
		if(m_ARClass)
		{
			m_masterConfig.m_Weapons.m_AR = m_ARClass;
		}
		
		if(m_MMGClass)
		{
			m_masterConfig.m_Weapons.m_MMG = m_MMGClass;
		}
		
		if(m_HMGClass)
		{
			m_masterConfig.m_Weapons.m_HMG = m_HMGClass;
		}
		
		if(m_ATClass)
		{
			m_masterConfig.m_Weapons.m_AT = m_ATClass;
		}
		
		if(m_MATClass)
		{
			m_masterConfig.m_Weapons.m_MAT = m_MATClass;
		}
		
		if(m_HATClass)
		{
			m_masterConfig.m_Weapons.m_HAT = m_HATClass;
		}
		
		if(m_AAClass)
		{
			m_masterConfig.m_Weapons.m_AA = m_AAClass;
		}
		
		if(m_sniperClass)
		{
			m_masterConfig.m_Weapons.m_Sniper = m_sniperClass;
		}
		
		// save config
		Resource holder = BaseContainerTools.CreateContainerFromInstance(m_masterConfig);
		if (holder)
		{
			BaseContainerTools.SaveContainer(holder.GetResource().ToBaseContainer(), "{A579F1490FFB037C}Configs/Gearscripts/GearScriptBluforTest.conf");
		}
	}
	
	void ConvertStringArraystoResourceName(array<string> clothingArray, string type)
	{
		switch(type)
		{
			case "HEADGEAR": 	{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem;	m_resourceHelmetsArray.Insert(clothingResource)}; 		break;}
			case "SHIRT": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceShirtsArray.Insert(clothingResource)}; 		break;}
			case "ARMOREDVEST": {foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceArmoredVestArray.Insert(clothingResource)};   break;}
			case "PANTS": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourcePantsArray.Insert(clothingResource)}; 		break;}
			case "BOOTS": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceBootsArray.Insert(clothingResource)}; 		break;}
			case "BACKPACK": 	{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem;	m_resourceBackpackArray.Insert(clothingResource)}; 		break;}
			case "VEST": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceVestArray.Insert(clothingResource)}; 			break;}
			case "HANDWEAR": 	{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem;	m_resourceHandwearArray.Insert(clothingResource)}; 		break;}
			case "HEAD": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceHeadArray.Insert(clothingResource)}; 			break;}
			case "EYES": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceEyesArray.Insert(clothingResource)}; 			break;}
			case "EARS": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceEarsArray.Insert(clothingResource)}; 			break;}
			case "FACE": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceFaceArray.Insert(clothingResource)}; 			break;}
			case "NECK":		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceNeckArray.Insert(clothingResource)}; 			break;}
			case "EXTRA1":		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceExtra1Array.Insert(clothingResource)}; 		break;}
			case "EXTRA2": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem;	m_resourceExtra2Array.Insert(clothingResource)}; 		break;}
			case "WAIST": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceWaistArray.Insert(clothingResource)}; 		break;}
			case "EXTRA3": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceExtra3Array.Insert(clothingResource)};		break;}
			case "EXTRA4": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceExtra4Array.Insert(clothingResource)}; 		break;}
			case "Rifle": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceRiflesArray.Insert(clothingResource)}; 		break;}
			case "RifleUGL": 	{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceRifleUGLArray.Insert(clothingResource)};		break;}
			case "Carbine": 	{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceCarbinesArray.Insert(clothingResource)};		break;}
			case "Pistol": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourcePistolsArray.Insert(clothingResource)}; 		break;}
			case "AR": 			{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceARPrefab.Insert(clothingResource)}; 			break;}
			case "MMG": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceMMGPrefab.Insert(clothingResource)}; 			break;}
			case "HMG": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceHMGPrefab.Insert(clothingResource)}; 			break;}
			case "AT": 			{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceATPrefab.Insert(clothingResource)}; 			break;}
			case "MAT": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceMATPrefab.Insert(clothingResource)}; 			break;}
			case "HAT": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceHATPrefab.Insert(clothingResource)}; 				break;}
			case "AA": 			{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceAAPrefab.Insert(clothingResource)}; 				break;}
			case "Sniper": 		{foreach(string clothingItem : clothingArray){ResourceName clothingResource = clothingItem; m_resourceSniperPrefab.Insert(clothingResource)}; 		break;}
		}
	}
	
	void AddClothingToConfig(array<ResourceName> clothingArray, string type)
	{
		if(clothingArray.Count() == 0)
			return;
	
		CRF_Clothing defaultClothing = new CRF_Clothing;
		defaultClothing.m_sClothingType = type;
		defaultClothing.m_ClothingPrefabs = clothingArray;
		m_clothingClassArray.Insert(defaultClothing);
	}
	
	void AddRegularWeaponsToConfig(array<ResourceName> weaponArray, string weaponType)
	{
		if(weaponArray.Count() == 0)
			return;
		int iteration = 0;
		if(weaponType == "RifleUGL")
		{
			for(int i = 0; i < weaponArray.Count()/4; i++)
			{
				CRF_Weapon_Class weaponsClass = new CRF_Weapon_Class();
				weaponsClass.m_Weapon = weaponArray[0 + iteration];
				CRF_Magazine_Class magazineClass = new CRF_Magazine_Class();
				magazineClass.m_Magazine = weaponArray[1 + iteration];
				int magCount = weaponArray[2 + iteration].ToInt();
				magazineClass.m_MagazineCount = magCount;
				ref array<ref CRF_Magazine_Class> magazineArray = {};
				magazineArray.Insert(magazineClass);
				weaponsClass.m_MagazineArray = magazineArray;
				if(weaponType == "RifleUGL")
				{
					if(weaponArray[3 + iteration].Contains("VOG"))
					{
						for(int g = 1; g <= 5; g++)
						{
							CRF_Magazine_Class UGLMagazineClass = new CRF_Magazine_Class();
							if(g == 1)
							{
								UGLMagazineClass.m_Magazine = "{262F0D09C4130826}Prefabs/Weapons/Ammo/Ammo_Grenade_HE_VOG25.et";
								UGLMagazineClass.m_MagazineCount = 5;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 2)
							{
								UGLMagazineClass.m_Magazine = "{B4CFBAF48A598F45}Prefabs/Weapons/Ammo/SOV_Ammo_Smoke_White.et";
								UGLMagazineClass.m_MagazineCount = 2;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 3)
							{
								UGLMagazineClass.m_Magazine = "{4D358DC7649B5CEA}Prefabs/Weapons/Ammo/SOV_Ammo_Smoke_Red.et";
								UGLMagazineClass.m_MagazineCount = 1;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 4)
							{
								UGLMagazineClass.m_Magazine = "{21335500CF96B660}Prefabs/Weapons/Ammo/SOV_Ammo_Smoke_Green.et";
								UGLMagazineClass.m_MagazineCount = 1;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 5)
							{
								UGLMagazineClass.m_Magazine = "{906F07BD0366E08F}Prefabs/Weapons/Ammo/Ammo_Flare_40mm_VG40OP_White.et";
								UGLMagazineClass.m_MagazineCount = 2;
								magazineArray.Insert(UGLMagazineClass);
							}
							ref array<ResourceName> vogAttachment = {"{1ABABE3551512B0A}Prefabs/Weapons/Attachments/Underbarrel/UGL_GP25.et"};
							weaponsClass.m_Attachments = vogAttachment;
							
						}
					} else if(weaponArray[3 + iteration].Contains("M406") || weaponArray[3 + iteration].Contains("M433"))
					{
						for(int g = 1; g <= 5; g++)
						{
							CRF_Magazine_Class UGLMagazineClass = new CRF_Magazine_Class();
							if(g == 1)
							{
								UGLMagazineClass.m_Magazine = "{5375FA7CB1F68573}Prefabs/Weapons/Ammo/Ammo_Grenade_HE_M406.et";
								UGLMagazineClass.m_MagazineCount = 5;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 2)
							{
								UGLMagazineClass.m_Magazine = "{AABE8FE0BA33DC8A}Prefabs/Weapons/Ammo/US_Ammo_Smoke_White.et";
								UGLMagazineClass.m_MagazineCount = 2;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 3)
							{
								UGLMagazineClass.m_Magazine = "{2B0C4280078D96B6}Prefabs/Weapons/Ammo/US_Ammo_Smoke_Red.et";
								UGLMagazineClass.m_MagazineCount = 1;
								magazineArray.Insert(UGLMagazineClass);
							}
							if(g == 4)
							{
								UGLMagazineClass.m_Magazine = "{0AF10C206CF1A282}Prefabs/Weapons/Ammo/US_Ammo_Smoke_Green.et";
								UGLMagazineClass.m_MagazineCount = 1;
								magazineArray.Insert(UGLMagazineClass);
							}		
							if(g == 5)
							{
								UGLMagazineClass.m_Magazine = "{98DB57ECEDC81CC2}Prefabs/Weapons/Ammo/Ammo_Flare_40mm_M583A1_White.et";
								UGLMagazineClass.m_MagazineCount = 2;
								magazineArray.Insert(UGLMagazineClass);
							}	
						}
					}
				}
				switch(weaponType)
				{
					case "Rifle":    {m_riflesClassArray.Insert(weaponsClass);   break;}
					case "RifleUGL": {m_rifleUGLClassArray.Insert(weaponsClass); break;}
					case "Carbine":  {m_carbinesClassArray.Insert(weaponsClass); break;}
					case "Pistol":   {m_pistolsClassArray.Insert(weaponsClass);  break;}
				}
				iteration = iteration + 4;
			}
			return;
		}
		for(int i = 0; i < weaponArray.Count()/3; i++)
		{
			CRF_Weapon_Class weaponsClass = new CRF_Weapon_Class();
			weaponsClass.m_Weapon = weaponArray[0 + iteration];
			CRF_Magazine_Class magazineClass = new CRF_Magazine_Class();
			magazineClass.m_Magazine = weaponArray[1 + iteration];
			int magCount = weaponArray[2 + iteration].ToInt();
			magazineClass.m_MagazineCount = magCount;
			ref array<ref CRF_Magazine_Class> magazineArray = {};
			magazineArray.Insert(magazineClass);
			weaponsClass.m_MagazineArray = magazineArray;
			
			switch(weaponType)
			{
				case "Rifle":    {m_riflesClassArray.Insert(weaponsClass);   break;}
				case "RifleUGL": {m_rifleUGLClassArray.Insert(weaponsClass); break;}
				case "Carbine":  {m_carbinesClassArray.Insert(weaponsClass); break;}
				case "Pistol":   {m_pistolsClassArray.Insert(weaponsClass);  break;}
			}
			iteration = iteration + 3;
		}
	}
	void AddSpecWeaponToConfig(array<ResourceName> weaponArray, string weaponType)
	{
		if(weaponArray.Count() == 0)
			return;
		
		CRF_Spec_Weapon_Class specWeaponsClass = new CRF_Spec_Weapon_Class();
		CRF_Spec_Magazine_Class specMagazineClass = new CRF_Spec_Magazine_Class();
		CRF_Weapon_Class weaponsClass = new CRF_Weapon_Class();
		CRF_Magazine_Class magazineClass = new CRF_Magazine_Class();
		if(weaponType != "Sniper")
		{
			specWeaponsClass.m_Weapon = weaponArray[0];
			
			if(weaponArray[1] == "none")
			{
				switch(weaponType)
				{
					case "AR":     {m_ARClass = specWeaponsClass;  break;}
					case "MMG":    {m_MMGClass = specWeaponsClass; break;}
					case "HMG":    {m_HMGClass = specWeaponsClass; break;}
					case "AT":     {m_ATClass = specWeaponsClass;  break;}
					case "MAT":    {m_MATClass = specWeaponsClass; break;}
					case "HAT":    {m_HATClass = specWeaponsClass; break;}
					case "AA":     {m_AAClass = specWeaponsClass;  break;}
				}
				return;
			}
			
			specMagazineClass.m_Magazine = weaponArray[1];
			int magCount = weaponArray[2].ToInt();
			specMagazineClass.m_MagazineCount = magCount;
			int assistantMagCount = weaponArray[3].ToInt();
			specMagazineClass.m_AssistantMagazineCount = assistantMagCount;
			specWeaponsClass.m_MagazineArray = {specMagazineClass};
		} else
		{
			weaponsClass.m_Weapon = weaponArray[0];
			magazineClass.m_Magazine = weaponArray[1];
			int magCount = weaponArray[2].ToInt();
			magazineClass.m_MagazineCount = magCount;
			weaponsClass.m_MagazineArray = {magazineClass};
		}
		switch(weaponType)
		{
			case "AR":     {m_ARClass = specWeaponsClass;  break;}
			case "MMG":    {m_MMGClass = specWeaponsClass; break;}
			case "HMG":    {m_HMGClass = specWeaponsClass; break;}
			case "AT":     {m_ATClass = specWeaponsClass;  break;}
			case "MAT":    {m_MATClass = specWeaponsClass; break;}
			case "HAT":    {m_HATClass = specWeaponsClass; break;}
			case "AA":     {m_AAClass = specWeaponsClass;  break;}
			case "Sniper": {m_sniperClass = weaponsClass;  break;}
		}
	}
}