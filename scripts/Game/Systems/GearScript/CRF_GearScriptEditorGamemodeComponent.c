[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_GearScriptEditorGamemodeComponentClass: SCR_BaseGameModeComponentClass {}

class CRF_GearScriptEditorGamemodeComponent: SCR_BaseGameModeComponent
{	
	//Half works rn, GUID generation does not
	ref array<string> finalWriteArray = {};
	
	void WriteToConfig(array<string> writeArray)
	{
		FileHandle fHandleOut = FileIO.OpenFile("$COALITIONFramework:Configs/Gearscripts/GearScriptBluforTest.conf", FileMode.WRITE);
		foreach( string line : writeArray)
 		 {
  			  fHandleOut.WriteLine(line);
 		 }
  		fHandleOut.Close();
	}
	void ExportGearScript(array<array<ResourceName>> gearArray)
	{
		string ending = "}";
		//Top Line
		string  stringCRF_GearScriptConfig = "CRF_GearScriptConfig {";
		
		//Default Gear Line
		string defaultGear = "m_DefaultGear CRF_Default_Gear \"{6628CDE935E91D23F}\" {";
		string defaultClothing = string.Format("m_DefaultClothing {");
		
		//Insert GearScriptConfig Line
		finalWriteArray.Insert(stringCRF_GearScriptConfig);
		
		//Insert Default Gear
		finalWriteArray.Insert(defaultGear);
		
		//Insert Default Clothing
		finalWriteArray.Insert(defaultClothing);
		
		Print(gearArray);

		//------------------------------------Headgear------------------------------------------
		AddToFinalWriteClothing(gearArray[0], "628CDE935E91D392", "HEADGEAR");
		
		//------------------------------------Shirts--------------------------------------------
		//AddToFinalWriteClothing(gearArray[1], "628CDE935E91D394", "SHIRT");

		//------------------------------------Armored Vests-------------------------------------
		//AddToFinalWriteClothing(gearArray[2], "628CDE935E91D397", "ARMOREDVEST");

		//------------------------------------Pants---------------------------------------------
		//AddToFinalWriteClothing(gearArray[3], "628CDE935E91D39A", "PANTS");

		//------------------------------------Boots---------------------------------------------
		//AddToFinalWriteClothing(gearArray[4], "628CDE935E91D39C", "BOOTS");

		//------------------------------------BACKPACK---------------------------------------------
		//AddToFinalWriteClothing(gearArray[5], "628CDE935E91D346", "BACKPACK");
		
		//------------------------------------VEST---------------------------------------------
		//AddToFinalWriteClothing(gearArray[6], "628EFB124AF91CCF", "VEST");
		
		//------------------------------------HANDWEAR---------------------------------------------
		//AddToFinalWriteClothing(gearArray[7], "628EFB124A002F35", "HANDWEAR");
		
		//------------------------------------HEAD---------------------------------------------
		//AddToFinalWriteClothing(gearArray[8], "628EFB124A5761FB", "HEAD");
		
		//------------------------------------EYES---------------------------------------------
		//AddToFinalWriteClothing(gearArray[9], "628EFB12499FD016", "EYES");
		
		//------------------------------------EARS---------------------------------------------
		//AddToFinalWriteClothing(gearArray[10], "628EFB1249A5AB40", "EARS");
		
		//------------------------------------FACE---------------------------------------------
		//AddToFinalWriteClothing(gearArray[11], "628EFB12490E73EC", "FACE");
		
		//------------------------------------NECK---------------------------------------------
		//AddToFinalWriteClothing(gearArray[12], "628EFB12494C7A73", "NECK");
		
		//------------------------------------EXTRA1---------------------------------------------
		//AddToFinalWriteClothing(gearArray[13], "628EFB124893CAA8", "EXTRA1");
		
		//------------------------------------EXTRA2---------------------------------------------
		//AddToFinalWriteClothing(gearArray[14], "628EFB1248D93CEF", "EXTRA2");
		
		//------------------------------------WASTE---------------------------------------------
		//AddToFinalWriteClothing(gearArray[15], "628EFB1248E2B814", "WASTE");
		
		//------------------------------------EXTRA3---------------------------------------------
		//AddToFinalWriteClothing(gearArray[16], "628EFB12482816BA", "EXTRA3");
		
		//------------------------------------EXTRA4---------------------------------------------
		//AddToFinalWriteClothing(gearArray[17], "628EFB12484FFD60", "EXTRA4");

		
		//Default Clothing Ending
		finalWriteArray.Insert(ending);
		
		//Default Gear Ending
		finalWriteArray.Insert(ending);
		
		//Gear Script Ending
		finalWriteArray.Insert(ending);
		
	}
	
	void AddToFinalWriteClothing(array<ResourceName> clothingArray, string GUID, string type)
	{
		if(clothingArray.Count() == 0)
			return;

		CRF_GearScriptConfig masterConfig = new CRF_GearScriptConfig();
		CRF_Default_Gear defaultGearConfig = new CRF_Default_Gear();
		CRF_Clothing defaultClothing = new CRF_Clothing();
		defaultClothing.m_sClothingType = type;
		defaultClothing.m_ClothingPrefabs = clothingArray;
		defaultGearConfig.m_DefaultClothing = {defaultClothing};
		
		masterConfig.m_DefaultGear = defaultGearConfig;
		
		// save config
		Resource holder = BaseContainerTools.CreateContainerFromInstance(masterConfig);
		if (holder)
		{
			BaseContainerTools.SaveContainer(holder.GetResource().ToBaseContainer(), "{A579F1490FFB037C}Configs/Gearscripts/GearScriptBluforTest.conf");
		}
	}
}