class CRF_InventoryUI : SCR_InventoryMenuUI
{
	protected Widget m_wRoot;
	protected FrameWidget m_hudRoot;
	protected TextWidget m_arrayWidget;
	protected string m_item;
	protected ref array<string> m_itemArray = {};
	
	protected ref array<string> m_helmetsArray = {};
	protected ref array<string> m_shirtsArray = {};
	protected ref array<string> m_armoredVestArray = {};
	protected ref array<string> m_pantsArray = {};
	protected ref array<string> m_bootsArray = {};
	protected ref array<string> m_backpackArray = {};
	protected ref array<string> m_vestArray = {};
	protected ref array<string> m_handwearArray = {};
	protected ref array<string> m_headArray = {};
	protected ref array<string> m_eyesArray = {};
	protected ref array<string> m_earsArray = {};
	protected ref array<string> m_faceArray = {};
	protected ref array<string> m_neckArray = {};
	protected ref array<string> m_extra1Array = {};
	protected ref array<string> m_extra2Array = {};
	protected ref array<string> m_waistArray = {};
	protected ref array<string> m_extra3Array = {};
	protected ref array<string> m_extra4Array = {};
	
	protected ref array<string> m_ARPrefab = {};
	protected ref array<string> m_MMGPrefab = {};
	protected ref array<string> m_HMGPrefab = {};
	protected ref array<string> m_ATPrefab = {};
	protected ref array<string> m_MATPrefab = {};
	protected ref array<string> m_HATPrefab = {};
	protected ref array<string> m_AAPrefab = {};
	protected ref array<string> m_sniperPrefab = {};
	
	protected ref array<string> m_riflesArray = {};
	protected ref array<string> m_rifleUGLArray = {};
	protected ref array<string> m_carbinesArray = {};
	protected ref array<string> m_pistolsArray = {};
	
	protected ref array<Managed> m_WeaponSlotComponentArray = {};
	
	CRF_GearScriptEditorGamemodeComponent m_gearScriptEditor;
	SCR_CharacterInventoryStorageComponent m_CharacterInventory;
	
	ref EntitySpawnParams spawnParams;
	
	ref array<string> m_loadedArray = {};
	ref array<array<string>> m_finalizedLoadedArray = {};
	override void OnMenuOpen()
	{	
		super.OnMenuOpen();
		
		m_wRoot = GetRootWidget();
		m_hudRoot = FrameWidget.Cast(m_wRoot.FindWidget("GearScriptLayout"));
		
		m_gearScriptEditor = CRF_GearScriptEditorGamemodeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GearScriptEditorGamemodeComponent));
		
		if(!m_gearScriptEditor)
			{
				while(true)
				{
					if(m_hudRoot.GetChildren().GetName() == "FactionNameText")
					{
						m_hudRoot.GetChildren().SetEnabled(false);
						m_hudRoot.RemoveChild(m_hudRoot.GetChildren());
						break;
					}
					
					m_hudRoot.GetChildren().SetEnabled(false);
					m_hudRoot.RemoveChild(m_hudRoot.GetChildren());
				}
			return;
			}
		
		spawnParams = new EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
        spawnParams.Transform[3] = SCR_PlayerController.GetLocalControlledEntity().GetOrigin();
		
		m_CharacterInventory = SCR_CharacterInventoryStorageComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterInventoryStorageComponent));
		SCR_PlayerController.GetLocalControlledEntity().FindComponents(WeaponSlotComponent, m_WeaponSlotComponentArray);
		
		if(m_gearScriptEditor.GetGear())
		{
			m_gearScriptEditor.GetGear().Split("|", m_loadedArray, false);
			if(m_loadedArray.Get(0))
			m_loadedArray.Get(0).Split(":", m_helmetsArray, false);
			if(m_loadedArray.Get(1))
				m_loadedArray.Get(1).Split(":", m_shirtsArray, false);
			if(m_loadedArray.Get(2))
				m_loadedArray.Get(2).Split(":", m_armoredVestArray, false);
			if(m_loadedArray.Get(3))
				m_loadedArray.Get(3).Split(":", m_pantsArray, false);
			if(m_loadedArray.Get(4))
				m_loadedArray.Get(4).Split(":", m_bootsArray, false);
			if(m_loadedArray.Get(5))
				m_loadedArray.Get(5).Split(":", m_backpackArray, false);
			if(m_loadedArray.Get(6))
				m_loadedArray.Get(6).Split(":", m_vestArray, false);
			if(m_loadedArray.Get(7))
				m_loadedArray.Get(7).Split(":", m_handwearArray, false);
			if(m_loadedArray.Get(8))
				m_loadedArray.Get(8).Split(":", m_headArray, false);
			if(m_loadedArray.Get(9))
				m_loadedArray.Get(9).Split(":", m_eyesArray, false);
			if(m_loadedArray.Get(10))
				m_loadedArray.Get(10).Split(":", m_earsArray, false);
			if(m_loadedArray.Get(11))
				m_loadedArray.Get(11).Split(":", m_faceArray, false);
			if(m_loadedArray.Get(12))
				m_loadedArray.Get(12).Split(":", m_neckArray, false);
			if(m_loadedArray.Get(13))
				m_loadedArray.Get(13).Split(":", m_extra1Array, false);
			if(m_loadedArray.Get(14))
				m_loadedArray.Get(14).Split(":", m_extra2Array, false);
			if(m_loadedArray.Get(15))
				m_loadedArray.Get(15).Split(":", m_waistArray, false);
			if(m_loadedArray.Get(16))
				m_loadedArray.Get(16).Split(":", m_extra3Array, false);
			if(m_loadedArray.Get(17))
				m_loadedArray.Get(17).Split(":", m_extra4Array, false);
			if(m_loadedArray.Get(18))
				m_loadedArray.Get(18).Split(":", m_riflesArray, false);
			if(m_loadedArray.Get(19))
				m_loadedArray.Get(19).Split(":", m_rifleUGLArray, false);
			if(m_loadedArray.Get(20))
				m_loadedArray.Get(20).Split(":", m_carbinesArray, false);
			if(m_loadedArray.Get(21))
				m_loadedArray.Get(21).Split(":", m_pistolsArray, false);
			if(m_loadedArray.Get(22))
				m_loadedArray.Get(22).Split(":", m_ARPrefab, false);
			if(m_loadedArray.Get(23))
				m_loadedArray.Get(23).Split(":", m_MMGPrefab, false);
			if(m_loadedArray.Get(24))
				m_loadedArray.Get(24).Split(":", m_HMGPrefab, false);
			if(m_loadedArray.Get(25))
				m_loadedArray.Get(25).Split(":", m_ATPrefab, false);
			if(m_loadedArray.Get(26))
				m_loadedArray.Get(26).Split(":", m_MATPrefab, false);
			if(m_loadedArray.Get(27))
				m_loadedArray.Get(27).Split(":", m_HATPrefab, false);
			if(m_loadedArray.Get(28))
				m_loadedArray.Get(28).Split(":", m_AAPrefab, false);
			if(m_loadedArray.Get(29))
				m_loadedArray.Get(29).Split(":", m_sniperPrefab, false);
			RefreshClothing(true, m_helmetsArray, "HelmetArray");
			RefreshClothing(true, m_shirtsArray, "ShirtArray");
			RefreshClothing(true, m_armoredVestArray, "ArmoredVestArray");
			RefreshClothing(true, m_pantsArray, "PantsArray");
			RefreshClothing(true, m_bootsArray, "BootsArray");
			RefreshClothing(true, m_backpackArray, "BackpackArray");
			RefreshClothing(true, m_vestArray, "VestArray");
			RefreshClothing(true, m_handwearArray, "HandwearArray");
			RefreshClothing(true, m_headArray, "HeadArray");
			RefreshClothing(true, m_eyesArray, "EyesArray");
			RefreshClothing(true, m_earsArray, "EarsArray");
			RefreshClothing(true, m_faceArray, "FaceArray");
			RefreshClothing(true, m_neckArray, "NeckArray");
			RefreshClothing(true, m_extra1Array, "Extra1Array");
			RefreshClothing(true, m_extra2Array, "Extra2Array");
			RefreshClothing(true, m_waistArray, "WaistArray");
			RefreshClothing(true, m_extra3Array, "Extra3Array");
			RefreshClothing(true, m_extra4Array, "Extra4Array");
			RefreshRegularWeapon(true, m_riflesArray, "RifleArray");
			RefreshRegularWeapon(true, m_rifleUGLArray, "RifleUGLArray");
			RefreshRegularWeapon(true, m_carbinesArray, "CarbineArray");
			RefreshRegularWeapon(true, m_pistolsArray, "PistolArray");
			RefreshRegularWeapon(true, m_sniperPrefab, "SniperString");
			RefreshSpecWeapon(true, m_ARPrefab, "ARString");
			RefreshSpecWeapon(true, m_MMGPrefab, "MMGString");
			RefreshSpecWeapon(true, m_HMGPrefab, "HMGString");
			RefreshSpecWeapon(true, m_ATPrefab, "ATString");
			RefreshSpecWeapon(true, m_MATPrefab, "MATString");
			RefreshSpecWeapon(true, m_HATPrefab, "HATString");
			RefreshSpecWeapon(true, m_AAPrefab, "AAString");
			
			for(int i = 0; i < m_gearScriptEditor.GetCheckedBoxes().Count(); i++)
			{
				
				if(i == 0)
					CheckBoxWidget.Cast(m_hudRoot.FindWidget("LeadershipBinos")).SetChecked(m_gearScriptEditor.GetCheckedBoxes().Get(i));
				
				if(i == 1)
					CheckBoxWidget.Cast(m_hudRoot.FindWidget("AssistantBinos")).SetChecked(m_gearScriptEditor.GetCheckedBoxes().Get(i));
				
				if(i == 2)
					CheckBoxWidget.Cast(m_hudRoot.FindWidget("MedicFrags")).SetChecked(m_gearScriptEditor.GetCheckedBoxes().Get(i));
			}
			
			if(m_gearScriptEditor.GetFaction() == "Blufor")
			{
				TextWidget.Cast(m_hudRoot.FindWidget("BluforText")).SetText("((BLUFOR))");
				TextWidget.Cast(m_hudRoot.FindWidget("OpforText")).SetText("OPFOR");
				TextWidget.Cast(m_hudRoot.FindWidget("IndforText")).SetText("INDFOR");
			}
			
			if(m_gearScriptEditor.GetFaction() == "Opfor")
			{
				TextWidget.Cast(m_hudRoot.FindWidget("BluforText")).SetText("BLUFOR");
				TextWidget.Cast(m_hudRoot.FindWidget("OpforText")).SetText("((OPFOR))");
				TextWidget.Cast(m_hudRoot.FindWidget("IndforText")).SetText("INDFOR");
			}
			
			if(m_gearScriptEditor.GetFaction() == "Indfor")
			{
				TextWidget.Cast(m_hudRoot.FindWidget("BluforText")).SetText("BLUFOR");
				TextWidget.Cast(m_hudRoot.FindWidget("OpforText")).SetText("OPFOR");
				TextWidget.Cast(m_hudRoot.FindWidget("IndforText")).SetText("((INDFOR))");
			}
			
			EditBoxWidget.Cast(m_hudRoot.FindWidget("FileNameEditBox")).SetText(m_gearScriptEditor.GetFileName());
			EditBoxWidget.Cast(m_hudRoot.FindWidget("FactionEditBox")).SetText(m_gearScriptEditor.GetFactionName());
		}
		
		//Set Regular Weapons
		SCR_ButtonTextComponent rifleButton = SCR_ButtonTextComponent.GetButtonText("Rifle", m_wRoot);
		SCR_ButtonTextComponent rifleUGLButton = SCR_ButtonTextComponent.GetButtonText("RifleUGL", m_wRoot);
		SCR_ButtonTextComponent carbineButton = SCR_ButtonTextComponent.GetButtonText("Carbine", m_wRoot);
		SCR_ButtonTextComponent pistolButton = SCR_ButtonTextComponent.GetButtonText("Pistol", m_wRoot);
		
		//Add Regular Weapon on Click
		rifleButton.m_OnClicked.Insert(AddRifle);
		rifleUGLButton.m_OnClicked.Insert(AddRifleUGL);
		carbineButton.m_OnClicked.Insert(AddCarbine);
		pistolButton.m_OnClicked.Insert(AddPistol);
		
		//Remove Regular Weapon Buttons
		SCR_ButtonTextComponent removeRifleButton = SCR_ButtonTextComponent.GetButtonText("RifleRemove", m_wRoot);
		SCR_ButtonTextComponent removeRifleUGLButton = SCR_ButtonTextComponent.GetButtonText("RifleUGLRemove", m_wRoot);
		SCR_ButtonTextComponent removeCarbineButton = SCR_ButtonTextComponent.GetButtonText("CarbineRemove", m_wRoot);
		SCR_ButtonTextComponent removePistolButton = SCR_ButtonTextComponent.GetButtonText("PistolRemove", m_wRoot);
		
		//Remove Regular Weapon on Click
		removeRifleButton.m_OnClicked.Insert(RemoveRifle);
		removeRifleUGLButton.m_OnClicked.Insert(RemoveRifleUGL);
		removeCarbineButton.m_OnClicked.Insert(RemoveCarbine);
		removePistolButton.m_OnClicked.Insert(RemovePistol);
		
		//Clear Regular Weapon Buttons
		SCR_ButtonTextComponent clearRifleButton = SCR_ButtonTextComponent.GetButtonText("RifleClear", m_wRoot);
		SCR_ButtonTextComponent clearRifleUGLButton = SCR_ButtonTextComponent.GetButtonText("RifleUGLClear", m_wRoot);
		SCR_ButtonTextComponent clearCarbineButton = SCR_ButtonTextComponent.GetButtonText("CarbineClear", m_wRoot);
		SCR_ButtonTextComponent clearPistolButton = SCR_ButtonTextComponent.GetButtonText("PistolClear", m_wRoot);
		
		//Clear Weapon on Click
		clearRifleButton.m_OnClicked.Insert(ClearRifle);
		clearRifleUGLButton.m_OnClicked.Insert(ClearRifleUGL);
		clearCarbineButton.m_OnClicked.Insert(ClearCarbine);
		clearPistolButton.m_OnClicked.Insert(ClearPistol);
		
		//Set Spec Weapons
		SCR_ButtonTextComponent m_ARButton = SCR_ButtonTextComponent.GetButtonText("AR", m_wRoot);
		SCR_ButtonTextComponent m_MMGButton = SCR_ButtonTextComponent.GetButtonText("MMG", m_wRoot);
		SCR_ButtonTextComponent m_HMGButton = SCR_ButtonTextComponent.GetButtonText("HMG", m_wRoot);
		SCR_ButtonTextComponent m_ATButton = SCR_ButtonTextComponent.GetButtonText("AT", m_wRoot);
		SCR_ButtonTextComponent m_MATButton = SCR_ButtonTextComponent.GetButtonText("MAT", m_wRoot);
		SCR_ButtonTextComponent m_HATButton = SCR_ButtonTextComponent.GetButtonText("HAT", m_wRoot);
		SCR_ButtonTextComponent m_AAButton = SCR_ButtonTextComponent.GetButtonText("AA", m_wRoot);
		SCR_ButtonTextComponent m_sniperButton = SCR_ButtonTextComponent.GetButtonText("Sniper", m_wRoot);
		
		//Add Spec Weapons on Click
		m_ARButton.m_OnClicked.Insert(AddAR);
		m_MMGButton.m_OnClicked.Insert(AddMMG);
		m_HMGButton.m_OnClicked.Insert(AddHMG);
		m_ATButton.m_OnClicked.Insert(AddAT);
		m_MATButton.m_OnClicked.Insert(AddMAT);
		m_HATButton.m_OnClicked.Insert(AddHAT);
		m_AAButton.m_OnClicked.Insert(AddAA);
		m_sniperButton.m_OnClicked.Insert(AddSniper);
		
		//Remove weapons
		SCR_ButtonTextComponent m_RemoveARButton = SCR_ButtonTextComponent.GetButtonText("ARClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveMMGButton = SCR_ButtonTextComponent.GetButtonText("MMGClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveHMGButton = SCR_ButtonTextComponent.GetButtonText("HMGClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveATButton = SCR_ButtonTextComponent.GetButtonText("ATClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveMATButton = SCR_ButtonTextComponent.GetButtonText("MATClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveHATButton = SCR_ButtonTextComponent.GetButtonText("HATClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveAAButton = SCR_ButtonTextComponent.GetButtonText("AAClear", m_wRoot);
		SCR_ButtonTextComponent m_RemoveSniperButton = SCR_ButtonTextComponent.GetButtonText("SniperClear", m_wRoot);
		
		//Remove Spec Weapons on Click
		m_RemoveARButton.m_OnClicked.Insert(RemoveAR);
		m_RemoveMMGButton.m_OnClicked.Insert(RemoveMMG);
		m_RemoveHMGButton.m_OnClicked.Insert(RemoveHMG);
		m_RemoveATButton.m_OnClicked.Insert(RemoveAT);
		m_RemoveMATButton.m_OnClicked.Insert(RemoveMAT);
		m_RemoveHATButton.m_OnClicked.Insert(RemoveHAT);
		m_RemoveAAButton.m_OnClicked.Insert(RemoveAA);
		m_RemoveSniperButton.m_OnClicked.Insert(RemoveSniper);
      
		//Add Buttons
		SCR_ButtonTextComponent m_helemtsButton = SCR_ButtonTextComponent.GetButtonText("Helmet", m_wRoot);
		SCR_ButtonTextComponent m_shirtsButton = SCR_ButtonTextComponent.GetButtonText("Shirt", m_wRoot);
		SCR_ButtonTextComponent m_armoredVestButton = SCR_ButtonTextComponent.GetButtonText("ArmoredVest", m_wRoot);
		SCR_ButtonTextComponent m_pantsButton = SCR_ButtonTextComponent.GetButtonText("Pants", m_wRoot);
		SCR_ButtonTextComponent m_bootsButton = SCR_ButtonTextComponent.GetButtonText("Boots", m_wRoot);
		SCR_ButtonTextComponent m_backpackButton = SCR_ButtonTextComponent.GetButtonText("Backpack", m_wRoot);
		SCR_ButtonTextComponent m_vestButton = SCR_ButtonTextComponent.GetButtonText("Vest", m_wRoot);
		SCR_ButtonTextComponent m_handwearButton = SCR_ButtonTextComponent.GetButtonText("Handwear", m_wRoot);
		SCR_ButtonTextComponent m_headButton = SCR_ButtonTextComponent.GetButtonText("Head", m_wRoot);
		SCR_ButtonTextComponent m_eyesButton = SCR_ButtonTextComponent.GetButtonText("Eyes", m_wRoot);
		SCR_ButtonTextComponent m_earsButton = SCR_ButtonTextComponent.GetButtonText("Ears", m_wRoot);
		SCR_ButtonTextComponent m_faceButton = SCR_ButtonTextComponent.GetButtonText("Face", m_wRoot);
		SCR_ButtonTextComponent m_neckButton = SCR_ButtonTextComponent.GetButtonText("Neck", m_wRoot);
		SCR_ButtonTextComponent m_extra1Button = SCR_ButtonTextComponent.GetButtonText("Extra1", m_wRoot);
		SCR_ButtonTextComponent m_extra2Button = SCR_ButtonTextComponent.GetButtonText("Extra2", m_wRoot);
		SCR_ButtonTextComponent m_waistButton = SCR_ButtonTextComponent.GetButtonText("Waist", m_wRoot);
		SCR_ButtonTextComponent m_extra3Button = SCR_ButtonTextComponent.GetButtonText("Extra3", m_wRoot);
		SCR_ButtonTextComponent m_extra4Button = SCR_ButtonTextComponent.GetButtonText("Extra4", m_wRoot);
      
		//Add Buttons on Clicked
		m_helemtsButton.m_OnClicked.Insert(AddHelmet); 
		m_shirtsButton.m_OnClicked.Insert(AddShirt);
		m_armoredVestButton.m_OnClicked.Insert(AddArmoredVest); 
		m_pantsButton.m_OnClicked.Insert(AddPants);
		m_bootsButton.m_OnClicked.Insert(AddBoots); 
		m_backpackButton.m_OnClicked.Insert(AddBackpack);
		m_vestButton.m_OnClicked.Insert(AddVest); 
		m_handwearButton.m_OnClicked.Insert(AddHandwear);
		m_headButton.m_OnClicked.Insert(AddHead); 
		m_eyesButton.m_OnClicked.Insert(AddEyes);
		m_earsButton.m_OnClicked.Insert(AddEars); 
		m_faceButton.m_OnClicked.Insert(AddFace);
		m_neckButton.m_OnClicked.Insert(AddNeck); 
		m_extra1Button.m_OnClicked.Insert(AddExtra1);
		m_extra2Button.m_OnClicked.Insert(AddExtra2); 
		m_waistButton.m_OnClicked.Insert(AddWaist);
		m_extra3Button.m_OnClicked.Insert(AddExtra3); 
		m_extra4Button.m_OnClicked.Insert(AddExtra4);
		
		//Clear Buttons
		SCR_ButtonTextComponent helemtsClearButton = SCR_ButtonTextComponent.GetButtonText("HelmetClear", m_wRoot);
		SCR_ButtonTextComponent shirtsClearButton = SCR_ButtonTextComponent.GetButtonText("ShirtClear", m_wRoot);
		SCR_ButtonTextComponent armoredVestClearButton = SCR_ButtonTextComponent.GetButtonText("ArmoredVestClear", m_wRoot);
		SCR_ButtonTextComponent pantsClearButton = SCR_ButtonTextComponent.GetButtonText("PantsClear", m_wRoot);
		SCR_ButtonTextComponent bootsClearButton = SCR_ButtonTextComponent.GetButtonText("BootsClear", m_wRoot);
		SCR_ButtonTextComponent backpackClearButton = SCR_ButtonTextComponent.GetButtonText("BackpackClear", m_wRoot);
		SCR_ButtonTextComponent vestClearButton = SCR_ButtonTextComponent.GetButtonText("VestClear", m_wRoot);
		SCR_ButtonTextComponent handwearClearButton = SCR_ButtonTextComponent.GetButtonText("HandwearClear", m_wRoot);
		SCR_ButtonTextComponent headClearButton = SCR_ButtonTextComponent.GetButtonText("HeadClear", m_wRoot);
		SCR_ButtonTextComponent eyesClearButton = SCR_ButtonTextComponent.GetButtonText("EyesClear", m_wRoot);
		SCR_ButtonTextComponent earsClearButton = SCR_ButtonTextComponent.GetButtonText("EarsClear", m_wRoot);
		SCR_ButtonTextComponent faceClearButton = SCR_ButtonTextComponent.GetButtonText("FaceClear", m_wRoot);
		SCR_ButtonTextComponent neckClearButton = SCR_ButtonTextComponent.GetButtonText("NeckClear", m_wRoot);
		SCR_ButtonTextComponent extra1ClearButton = SCR_ButtonTextComponent.GetButtonText("Extra1Clear", m_wRoot);
		SCR_ButtonTextComponent extra2ClearButton = SCR_ButtonTextComponent.GetButtonText("Extra2Clear", m_wRoot);
		SCR_ButtonTextComponent waistClearButton = SCR_ButtonTextComponent.GetButtonText("WaistClear", m_wRoot);
		SCR_ButtonTextComponent extra3ClearButton = SCR_ButtonTextComponent.GetButtonText("Extra3Clear", m_wRoot);
		SCR_ButtonTextComponent extra4ClearButton = SCR_ButtonTextComponent.GetButtonText("Extra4Clear", m_wRoot);
		
		//Clear Buttons on Clicked
		helemtsClearButton.m_OnClicked.Insert(ClearHelmets);
		shirtsClearButton.m_OnClicked.Insert(ClearShirts);
		armoredVestClearButton.m_OnClicked.Insert(ClearArmoredVests);
		pantsClearButton.m_OnClicked.Insert(ClearPants);
		bootsClearButton.m_OnClicked.Insert(ClearBoots);
		backpackClearButton.m_OnClicked.Insert(ClearBackpacks);
		vestClearButton.m_OnClicked.Insert(ClearVests);
		handwearClearButton.m_OnClicked.Insert(ClearHandwear);
		headClearButton.m_OnClicked.Insert(ClearHead);
		eyesClearButton.m_OnClicked.Insert(ClearEyes);
		earsClearButton.m_OnClicked.Insert(ClearEars);
		faceClearButton.m_OnClicked.Insert(ClearFace);
		neckClearButton.m_OnClicked.Insert(ClearNeck);
		extra1ClearButton.m_OnClicked.Insert(ClearExtra1);
		extra2ClearButton.m_OnClicked.Insert(ClearExtra2);
		waistClearButton.m_OnClicked.Insert(ClearWaist);
		extra3ClearButton.m_OnClicked.Insert(ClearExtra3);
		extra4ClearButton.m_OnClicked.Insert(ClearExtra4);
		
		//Remove Buttons
		SCR_ButtonTextComponent helemtsRemoveButton = SCR_ButtonTextComponent.GetButtonText("HelmetRemove", m_wRoot);
		SCR_ButtonTextComponent shirtsRemoveButton = SCR_ButtonTextComponent.GetButtonText("ShirtRemove", m_wRoot);
		SCR_ButtonTextComponent armoredVestRemoveButton = SCR_ButtonTextComponent.GetButtonText("ArmoredVestRemove", m_wRoot);
		SCR_ButtonTextComponent pantsRemoveButton = SCR_ButtonTextComponent.GetButtonText("PantsRemove", m_wRoot);
		SCR_ButtonTextComponent bootsRemoveButton = SCR_ButtonTextComponent.GetButtonText("BootsRemove", m_wRoot);
		SCR_ButtonTextComponent backpackRemoveButton = SCR_ButtonTextComponent.GetButtonText("BackpackRemove", m_wRoot);
		SCR_ButtonTextComponent vestRemoveButton = SCR_ButtonTextComponent.GetButtonText("VestRemove", m_wRoot);
		SCR_ButtonTextComponent handwearRemoveButton = SCR_ButtonTextComponent.GetButtonText("HandwearRemove", m_wRoot);
		SCR_ButtonTextComponent headRemoveButton = SCR_ButtonTextComponent.GetButtonText("HeadRemove", m_wRoot);
		SCR_ButtonTextComponent eyesRemoveButton = SCR_ButtonTextComponent.GetButtonText("EyesRemove", m_wRoot);
		SCR_ButtonTextComponent earsRemoveButton = SCR_ButtonTextComponent.GetButtonText("EarsRemove", m_wRoot);
		SCR_ButtonTextComponent faceRemoveButton = SCR_ButtonTextComponent.GetButtonText("FaceRemove", m_wRoot);
		SCR_ButtonTextComponent neckRemoveButton = SCR_ButtonTextComponent.GetButtonText("NeckRemove", m_wRoot);
		SCR_ButtonTextComponent extra1RemoveButton = SCR_ButtonTextComponent.GetButtonText("Extra1Remove", m_wRoot);
		SCR_ButtonTextComponent extra2RemoveButton = SCR_ButtonTextComponent.GetButtonText("Extra2Remove", m_wRoot);
		SCR_ButtonTextComponent waistRemoveButton = SCR_ButtonTextComponent.GetButtonText("WaistRemove", m_wRoot);
		SCR_ButtonTextComponent extra3RemoveButton = SCR_ButtonTextComponent.GetButtonText("Extra3Remove", m_wRoot);
		SCR_ButtonTextComponent extra4RemoveButton = SCR_ButtonTextComponent.GetButtonText("Extra4Remove", m_wRoot);
		
		//Remove Buttons on Clicked
		helemtsRemoveButton.m_OnClicked.Insert(RemoveHelmet);
		shirtsRemoveButton.m_OnClicked.Insert(RemoveShirt);
		armoredVestRemoveButton.m_OnClicked.Insert(RemoveArmoredVest);
		pantsRemoveButton.m_OnClicked.Insert(RemovePants);
		bootsRemoveButton.m_OnClicked.Insert(RemoveBoots);
		backpackRemoveButton.m_OnClicked.Insert(RemoveBackpack);
		vestRemoveButton.m_OnClicked.Insert(RemoveVest);
		handwearRemoveButton.m_OnClicked.Insert(RemoveHandwear);
		headRemoveButton.m_OnClicked.Insert(RemoveHead);
		eyesRemoveButton.m_OnClicked.Insert(RemoveEyes);
		earsRemoveButton.m_OnClicked.Insert(RemoveEars);
		faceRemoveButton.m_OnClicked.Insert(RemoveFace);
		neckRemoveButton.m_OnClicked.Insert(RemoveNeck);
		extra1RemoveButton.m_OnClicked.Insert(RemoveExtra1);
		extra2RemoveButton.m_OnClicked.Insert(RemoveExtra2);
		waistRemoveButton.m_OnClicked.Insert(RemoveWaist);
		extra3RemoveButton.m_OnClicked.Insert(RemoveExtra3);
		extra4RemoveButton.m_OnClicked.Insert(RemoveExtra4);
		
		//Export
		SCR_ButtonTextComponent export = SCR_ButtonTextComponent.GetButtonText("Export", m_wRoot);
		export.m_OnClicked.Insert(Export);
		
		//Faction Buttons
		SCR_ButtonTextComponent bluforButton = SCR_ButtonTextComponent.GetButtonText("Blufor", m_wRoot);
		SCR_ButtonTextComponent ofporButton = SCR_ButtonTextComponent.GetButtonText("Opfor", m_wRoot);
		SCR_ButtonTextComponent indforButton = SCR_ButtonTextComponent.GetButtonText("Indfor", m_wRoot);
		
		//Faction Buttons on Clicked
		bluforButton.m_OnClicked.Insert(SelectBlufor);
		ofporButton.m_OnClicked.Insert(SelectOpfor);
		indforButton.m_OnClicked.Insert(SelectIndfor);
	}
	
	override void Action_CloseInventory()
	{
		super.Action_CloseInventory();
		m_gearScriptEditor = CRF_GearScriptEditorGamemodeComponent.Cast(GetGame().GetGameMode().FindComponent(CRF_GearScriptEditorGamemodeComponent));
		if(m_gearScriptEditor)
			SaveGear();
	}
	
	void SelectBlufor()
	{
		m_gearScriptEditor.SwitchFaction("Blufor");
		TextWidget bluforTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("BluforText"));
		TextWidget opforTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("OpforText"));
		TextWidget indofrTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("IndforText"));
		bluforTextWidget.SetText("((BLUFOR))");
		opforTextWidget.SetText("OPFOR");
		indofrTextWidget.SetText("INDFOR");
	}
	
	void SelectOpfor()
	{
		m_gearScriptEditor.SwitchFaction("Opfor");
		TextWidget bluforTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("BluforText"));
		TextWidget opforTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("OpforText"));
		TextWidget indofrTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("IndforText"));
		bluforTextWidget.SetText("BLUFOR");
		opforTextWidget.SetText("((OPFOR))");
		indofrTextWidget.SetText("INDFOR");
	}
	
	void SelectIndfor()
	{
		m_gearScriptEditor.SwitchFaction("Indfor");
		TextWidget bluforTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("BluforText"));
		TextWidget opforTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("OpforText"));
		TextWidget indofrTextWidget = TextWidget.Cast(m_hudRoot.FindWidget("IndforText"));
		bluforTextWidget.SetText("BLUFOR");
		opforTextWidget.SetText("OPFOR");
		indofrTextWidget.SetText("((INDFOR))");
	}
	
	
	void SaveGear()
	{
		ref array<array<string>> exportArray = {};
		ref array<bool> checkboxArray = {};
		exportArray.Insert(m_helmetsArray);
		exportArray.Insert(m_shirtsArray);
		exportArray.Insert(m_armoredVestArray);
		exportArray.Insert(m_pantsArray);
		exportArray.Insert(m_bootsArray);
		exportArray.Insert(m_backpackArray);
		exportArray.Insert(m_vestArray);
		exportArray.Insert(m_handwearArray);
		exportArray.Insert(m_headArray);
		exportArray.Insert(m_eyesArray);
		exportArray.Insert(m_earsArray);
		exportArray.Insert(m_faceArray);
		exportArray.Insert(m_neckArray);
		exportArray.Insert(m_extra1Array);
		exportArray.Insert(m_extra2Array);
		exportArray.Insert(m_waistArray);
		exportArray.Insert(m_extra3Array);
		exportArray.Insert(m_extra4Array);
		exportArray.Insert(m_riflesArray);
		exportArray.Insert(m_rifleUGLArray);
		exportArray.Insert(m_carbinesArray);
		exportArray.Insert(m_pistolsArray);
		exportArray.Insert(m_ARPrefab);
		exportArray.Insert(m_MMGPrefab);
		exportArray.Insert(m_HMGPrefab);
		exportArray.Insert(m_ATPrefab);
		exportArray.Insert(m_MATPrefab);
		exportArray.Insert(m_HATPrefab);
		exportArray.Insert(m_AAPrefab);
		exportArray.Insert(m_sniperPrefab);
		bool leaderBinoBox = CheckBoxWidget.Cast(m_hudRoot.FindWidget("LeadershipBinos")).IsChecked();
		bool assistantBinoBox = CheckBoxWidget.Cast(m_hudRoot.FindWidget("AssistantBinos")).IsChecked();
		bool medicFragsBox = CheckBoxWidget.Cast(m_hudRoot.FindWidget("MedicFrags")).IsChecked();
		checkboxArray.Insert(leaderBinoBox);
		checkboxArray.Insert(assistantBinoBox);
		checkboxArray.Insert(medicFragsBox);
		m_gearScriptEditor.SetFileName(EditBoxWidget.Cast(m_hudRoot.FindWidget("FileNameEditBox")).GetText());
		m_gearScriptEditor.SetFactionName(EditBoxWidget.Cast(m_hudRoot.FindWidget("FactionEditBox")).GetText());
		m_gearScriptEditor.SaveGear(exportArray, checkboxArray);
	}
	
	void Export()
	{
		ref array<array<string>> exportArray = {};
		ref array<bool> checkboxArray = {};
		exportArray.Insert(m_helmetsArray);
		exportArray.Insert(m_shirtsArray);
		exportArray.Insert(m_armoredVestArray);
		exportArray.Insert(m_pantsArray);
		exportArray.Insert(m_bootsArray);
		exportArray.Insert(m_backpackArray);
		exportArray.Insert(m_vestArray);
		exportArray.Insert(m_handwearArray);
		exportArray.Insert(m_headArray);
		exportArray.Insert(m_eyesArray);
		exportArray.Insert(m_earsArray);
		exportArray.Insert(m_faceArray);
		exportArray.Insert(m_neckArray);
		exportArray.Insert(m_extra1Array);
		exportArray.Insert(m_extra2Array);
		exportArray.Insert(m_waistArray);
		exportArray.Insert(m_extra3Array);
		exportArray.Insert(m_extra4Array);
		exportArray.Insert(m_riflesArray);
		exportArray.Insert(m_rifleUGLArray);
		exportArray.Insert(m_carbinesArray);
		exportArray.Insert(m_pistolsArray);
		exportArray.Insert(m_ARPrefab);
		exportArray.Insert(m_MMGPrefab);
		exportArray.Insert(m_HMGPrefab);
		exportArray.Insert(m_ATPrefab);
		exportArray.Insert(m_MATPrefab);
		exportArray.Insert(m_HATPrefab);
		exportArray.Insert(m_AAPrefab);
		exportArray.Insert(m_sniperPrefab);
		bool leaderBinoBox = CheckBoxWidget.Cast(m_hudRoot.FindWidget("LeadershipBinos")).IsChecked();
		bool assistantBinoBox = CheckBoxWidget.Cast(m_hudRoot.FindWidget("AssistantBinos")).IsChecked();
		bool medicFragsBox = CheckBoxWidget.Cast(m_hudRoot.FindWidget("MedicFrags")).IsChecked();
		checkboxArray.Insert(leaderBinoBox);
		checkboxArray.Insert(assistantBinoBox);
		checkboxArray.Insert(medicFragsBox);
		m_gearScriptEditor.SetFileName(EditBoxWidget.Cast(m_hudRoot.FindWidget("FileNameEditBox")).GetText());
		m_gearScriptEditor.SetFactionName(EditBoxWidget.Cast(m_hudRoot.FindWidget("FactionEditBox")).GetText());
		m_gearScriptEditor.ExportGearScript(exportArray, checkboxArray);
	}
	
	void RefreshClothing(bool isLoading, array<string> clothingArray, string widgetName)
	{	
		ref array<string> itemPrefabNames = {};
		ref array<string> displayStringArray = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget(widgetName));
		if(isLoading)
		{
			if(clothingArray.Count() == 0)
			{
				m_arrayWidget.SetText("");
				return;
			}
		}
		for(int g = 0; g < clothingArray.Count(); g++)
		{
			itemPrefabNames.Insert(clothingArray[g]);		
		}
		foreach(string prefab : itemPrefabNames)
		{
			array<string> itemStringArray = {};
			prefab.Split("/", itemStringArray, false);
			displayStringArray.Insert(itemStringArray[itemStringArray.Count()-1]);	
		}
		string displayString = SCR_StringHelper.Join(", ", displayStringArray, true);
		m_arrayWidget.SetText(displayString);
	}
	
	void RefreshRegularWeapon(bool isLoading, array<string> weaponsArray, string widgetName)
	{
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget(widgetName));
		if(isLoading)
		{
			if(weaponsArray.Count() == 0)
			{
				m_arrayWidget.SetText("");
				return;
			}
		}
		ref array<string> weaponsPrefabNames = {};
		int index = 0;
		for(int g = 0; g < weaponsArray.Count()/4; g++)
		{
			weaponsPrefabNames.Insert(weaponsArray[index]);	
			index = index + 4;
		}
		array<string> displayStringArray = {};
		foreach(string prefab : weaponsPrefabNames)
		{
			array<string> weaponStringArray = {};
			prefab.Split("/", weaponStringArray, false);
			displayStringArray.Insert(weaponStringArray[weaponStringArray.Count()-1]);					
		}
		string displayString = SCR_StringHelper.Join(", ", displayStringArray, true);
		m_arrayWidget.SetText(displayString);
	}
	
	void RefreshUGL(bool isLoading)
	{
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("RifleUGLArray"));
		if(isLoading)
		{
			if(m_rifleUGLArray.Count() == 0)
			{
				m_arrayWidget.SetText("");
				return;
			}
		}
		ref array<string> weaponsPrefabNames = {};
		ref array<string> GLMags = {};
		int GLIndex = 3;
		int prefabIndex = 0;
		for(int i = 1; i <= m_rifleUGLArray.Count()/5; i++)
		{
			weaponsPrefabNames.Insert(m_rifleUGLArray[prefabIndex]);
			GLMags.Insert(m_rifleUGLArray[GLIndex]);
			GLIndex = GLIndex + 5;
			prefabIndex = prefabIndex + 5;
		}
		array<string> displayStringArray = {};
		int index = 0;
		foreach(string prefab : weaponsPrefabNames)
		{
			array<string> weaponStringArray = {};
			prefab.Split("/", weaponStringArray, false);
			if(GLMags[index].Contains("VOG"))
			{
				displayStringArray.Insert(string.Format("%1 | GP25", weaponStringArray[weaponStringArray.Count()-1]));
			} else
			{
				displayStringArray.Insert(weaponStringArray[weaponStringArray.Count()-1]);
			}	
			index++;
		}
		string displayString = SCR_StringHelper.Join(", ", displayStringArray, true);
		m_arrayWidget.SetText(displayString);
	}
	
	void RefreshSpecWeapon(bool isLoading, array<string> specWeaponArray, string widgetName)
	{
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget(widgetName));
		if(isLoading)
		{
			if(specWeaponArray.Count() == 0)
			{
				m_arrayWidget.SetText("");
				return;
			}
		}
		array<string> weaponStringArray = {};
		string displayString;
		specWeaponArray[0].Split("/", weaponStringArray, false);
		displayString = (weaponStringArray[weaponStringArray.Count()-1]);
		m_arrayWidget.SetText(displayString);
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Rifles Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddRifle()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(300 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_riflesArray.Insert(weapon);
				m_riflesArray.Insert(magazine);
				m_riflesArray.Insert(numberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_riflesArray.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshRegularWeapon(false, m_riflesArray, "RifleArray");
				return;
			}
		}
	}
	void RemoveRifle()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("RifleInput"));
		int index = inputWidget.GetText().ToInt() - 1;
		for(int i = 0; i < 4; i++)
		{
			m_riflesArray.RemoveOrdered(index);
		}
		RefreshRegularWeapon(false, m_riflesArray, "RifleArray");
	}
	
	void ClearRifle()
	{
		m_riflesArray = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("RifleArray"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Pistols Members----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddPistol()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			if(weaponSlotComponent.GetWeaponSlotType() == "secondary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(60 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_pistolsArray.Insert(weapon);
				m_pistolsArray.Insert(magazine);
				m_pistolsArray.Insert(numberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_pistolsArray.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshRegularWeapon(false, m_pistolsArray, "PistolArray");
				return;
			}
		}
	}
	void RemovePistol()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("PistolInput"));
		int index = inputWidget.GetText().ToInt() - 1;
		for(int i = 0; i < 4; i++)
		{
			m_pistolsArray.RemoveOrdered(index);
		}
		RefreshRegularWeapon(false, m_pistolsArray, "PistolArray");
	}
	
	void ClearPistol()
	{
		m_pistolsArray = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("PistolArray"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Carbines Members---------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddCarbine()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(240 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_carbinesArray.Insert(weapon);
				m_carbinesArray.Insert(magazine);
				m_carbinesArray.Insert(numberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_carbinesArray.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshRegularWeapon(false, m_carbinesArray, "CarbineArray");
				return;
			}
		}
	}
	void RemoveCarbine()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("CarbineInput"));
		int index = inputWidget.GetText().ToInt() - 1;
		for(int i = 0; i < 4; i++)
		{
			m_carbinesArray.RemoveOrdered(index);
		}
		RefreshRegularWeapon(false, m_carbinesArray, "CarbineArray");
	}
	
	void ClearCarbine()
	{
		m_carbinesArray = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("CarbineArray"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------RifleUGL Members---------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddRifleUGL()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(300 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_rifleUGLArray.Insert(weapon);
				m_rifleUGLArray.Insert(magazine);
				m_rifleUGLArray.Insert(numberOfMags);
				ref array<BaseMuzzleComponent> muzzles = {};
				string GLMag = "";
				weaponSlotComponent.GetMuzzlesList(muzzles);
				foreach(BaseMuzzleComponent muzzle : muzzles)
				{
					if(muzzle.GetMuzzleType() == EMuzzleType.MT_UGLMuzzle)
					{
						GLMag = muzzle.GetMagazine().GetOwner().GetPrefabData().GetPrefabName();
					}
				}
				m_rifleUGLArray.Insert(GLMag);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_rifleUGLArray.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshUGL(false);
				return;
			}
		}
	}
	void RemoveRifleUGL()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("RifleUGLInput"));
		int index = inputWidget.GetText().ToInt() - 1;
		for(int i = 0; i < 5; i++)
		{
			m_rifleUGLArray.RemoveOrdered(index);
		}
		RefreshUGL(false);
	}
	
	void ClearRifleUGL()
	{
		m_rifleUGLArray = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("RifleUGLArray"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------AR Members---------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddAR()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_ARPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(800 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				string assistantNumberOfMags = string.ToString(Math.Round(600 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_ARPrefab.Insert(weapon);
				m_ARPrefab.Insert(magazine);
				m_ARPrefab.Insert(numberOfMags);
				m_ARPrefab.Insert(assistantNumberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_ARPrefab.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshSpecWeapon(false, m_ARPrefab, "ARString");
				
				return;
			}
		}
	}

	void RemoveAR()
	{
		m_ARPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ARString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------MMG Members--------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddMMG()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_MMGPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(800 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				string assistantNumberOfMags = string.ToString(Math.Round(600 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_MMGPrefab.Insert(weapon);
				m_MMGPrefab.Insert(magazine);
				m_MMGPrefab.Insert(numberOfMags);
				m_MMGPrefab.Insert(assistantNumberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_MMGPrefab.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshSpecWeapon(false, m_MMGPrefab, "MMGString");
				return;
			}
		}
	}
	
	void RemoveMMG()
	{
		m_MMGPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("MMGString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------HMG Members--------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddHMG()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_HMGPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(800 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				string assistantNumberOfMags = string.ToString(Math.Round(600 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_HMGPrefab.Insert(weapon);
				m_HMGPrefab.Insert(magazine);
				m_HMGPrefab.Insert(numberOfMags);
				m_HMGPrefab.Insert(assistantNumberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_HMGPrefab.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshSpecWeapon(false, m_HMGPrefab, "HMGString");
				return;
			}
		}
	}
	
	void RemoveHMG()
	{
		m_HMGPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HMGString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------AT Members---------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddAT()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_ATPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get((m_WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(!weaponSlotComponent.GetWeaponEntity())
						continue;
					
					string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
					string magazine;
					string numberOfMags;
					string assistantNumberOfMags;
					if(weaponSlotComponent.GetCurrentMuzzle().IsDisposable())
					{
						magazine = "none";
						numberOfMags = "none";
						assistantNumberOfMags= "";
					} else
					{
						magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
						numberOfMags = "2";
						assistantNumberOfMags = "3";
					}
					m_ATPrefab.Insert(weapon);
					m_ATPrefab.Insert(magazine);
					m_ATPrefab.Insert(numberOfMags);
					m_ATPrefab.Insert(assistantNumberOfMags);
					IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
					WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
					ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
					ref array<AttachmentSlotComponent> weaponAttachments = {};
					tempWeaponComponent.GetAttachments(tempWeaponAttachments);
					weaponSlotComponent.GetAttachments(weaponAttachments);
					string overridedAttachments = "";
					ref array<string> newAttachments = {};
					for(int g = 0; g < tempWeaponAttachments.Count(); g++)
					{
						string tempAttachmentString = "";
						string attachmentString = "";
						if(tempWeaponAttachments[g].GetAttachedEntity())
							tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(weaponAttachments[g].GetAttachedEntity())
							attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(tempAttachmentString != attachmentString)
							newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
					}
					overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
					m_ATPrefab.Insert(overridedAttachments);
					delete tempWeapon;
					RefreshSpecWeapon(false, m_ATPrefab, "ATString");
					return;
				}
			}
		}
	}
	
	void RemoveAT()
	{
		m_ATPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ATString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------MAT Members---------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddMAT()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_MATPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get((m_WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(!weaponSlotComponent.GetWeaponEntity())
						continue;
					
					string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
					string magazine;
					string numberOfMags;
					string assistantNumberOfMags;
					if(weaponSlotComponent.GetCurrentMuzzle().IsDisposable())
					{
						magazine = "none";
						numberOfMags = "none";
						assistantNumberOfMags = ""
					} else
					{
						magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
						numberOfMags = "3";
						assistantNumberOfMags = "3"
					}
					m_MATPrefab.Insert(weapon);
					m_MATPrefab.Insert(magazine);
					m_MATPrefab.Insert(numberOfMags);
					m_MATPrefab.Insert(assistantNumberOfMags);
					IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
					WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
					ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
					ref array<AttachmentSlotComponent> weaponAttachments = {};
					tempWeaponComponent.GetAttachments(tempWeaponAttachments);
					weaponSlotComponent.GetAttachments(weaponAttachments);
					string overridedAttachments = "";
					ref array<string> newAttachments = {};
					for(int g = 0; g < tempWeaponAttachments.Count(); g++)
					{
						string tempAttachmentString = "";
						string attachmentString = "";
						if(tempWeaponAttachments[g].GetAttachedEntity())
							tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(weaponAttachments[g].GetAttachedEntity())
							attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(tempAttachmentString != attachmentString)
							newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
					}
					overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
					m_MATPrefab.Insert(overridedAttachments);
					delete tempWeapon;
					RefreshSpecWeapon(false, m_MATPrefab, "MATString");
					return;
				}
			}
		}
	}
	
	void RemoveMAT()
	{
		m_MATPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("MATString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------HAT Members---------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddHAT()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_HATPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get((m_WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(!weaponSlotComponent.GetWeaponEntity())
						continue;
					
					string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
					string magazine;
					string numberOfMags;
					string assistantNumberOfMags;
					if(weaponSlotComponent.GetCurrentMuzzle().IsDisposable())
					{
						magazine = "none";
						numberOfMags = "none";
						assistantNumberOfMags = "none";
					} else
					{
						magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
						numberOfMags = "3";
						assistantNumberOfMags = "3";
					}
					m_HATPrefab.Insert(weapon);
					m_HATPrefab.Insert(magazine);
					m_HATPrefab.Insert(numberOfMags);
					m_HATPrefab.Insert(assistantNumberOfMags);
					IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
					WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
					ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
					ref array<AttachmentSlotComponent> weaponAttachments = {};
					tempWeaponComponent.GetAttachments(tempWeaponAttachments);
					weaponSlotComponent.GetAttachments(weaponAttachments);
					string overridedAttachments = "";
					ref array<string> newAttachments = {};
					for(int g = 0; g < tempWeaponAttachments.Count(); g++)
					{
						string tempAttachmentString = "";
						string attachmentString = "";
						if(tempWeaponAttachments[g].GetAttachedEntity())
							tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(weaponAttachments[g].GetAttachedEntity())
							attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(tempAttachmentString != attachmentString)
							newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
					}
					overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
					m_HATPrefab.Insert(overridedAttachments);
					delete tempWeapon;
					RefreshSpecWeapon(false, m_HATPrefab, "HATString");
					return;
				}
			}
		}
	}
	
	void RemoveHAT()
	{
		m_HATPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HATString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------AA Members---------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddAA()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_AAPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get((m_WeaponSlotComponentArray.Find(weaponSlotComponent) - 1))).GetWeaponSlotType() == "primary")
				{
					if(!weaponSlotComponent.GetWeaponEntity())
						continue;
					
					string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
					string magazine;
					string numberOfMags;
					string assistantNumberOfMags;
					if(weaponSlotComponent.GetCurrentMuzzle().IsDisposable())
					{
						magazine = "none";
						numberOfMags = "";
						assistantNumberOfMags = "none";
					} else
					{
						magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
						numberOfMags = "3";
						assistantNumberOfMags = "3"
					}
					m_AAPrefab.Insert(weapon);
					m_AAPrefab.Insert(magazine);
					m_AAPrefab.Insert(numberOfMags);
					m_AAPrefab.Insert(assistantNumberOfMags);
					IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
					WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
					ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
					ref array<AttachmentSlotComponent> weaponAttachments = {};
					tempWeaponComponent.GetAttachments(tempWeaponAttachments);
					weaponSlotComponent.GetAttachments(weaponAttachments);
					string overridedAttachments = "";
					ref array<string> newAttachments = {};
					for(int g = 0; g < tempWeaponAttachments.Count(); g++)
					{
						string tempAttachmentString = "";
						string attachmentString = "";
						if(tempWeaponAttachments[g].GetAttachedEntity())
							tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(weaponAttachments[g].GetAttachedEntity())
							attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
						if(tempAttachmentString != attachmentString)
							newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
					}
					overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
					m_AAPrefab.Insert(overridedAttachments);
					delete tempWeapon;
					RefreshSpecWeapon(false, m_AAPrefab, "AAString");
					return;
				}
			}
		}
	}
	
	void RemoveAA()
	{
		m_AAPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("AAString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Sniper Members--------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddSniper()
	{
		for(int i = 0; i < m_WeaponSlotComponentArray.Count(); i++)
		{
			WeaponSlotComponent weaponSlotComponent = WeaponSlotComponent.Cast(m_WeaponSlotComponentArray.Get(i));
			m_sniperPrefab = {};
			if(weaponSlotComponent.GetWeaponSlotType() == "primary")
			{
				if(!weaponSlotComponent.GetWeaponEntity())
					continue;
				
				string weapon = weaponSlotComponent.GetWeaponEntity().GetPrefabData().GetPrefabName();
				string magazine = weaponSlotComponent.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefabName();
				string numberOfMags = string.ToString(Math.Round(120 / weaponSlotComponent.GetCurrentMagazine().GetMaxAmmoCount()));
				m_sniperPrefab.Insert(weapon);
				m_sniperPrefab.Insert(magazine);
				m_sniperPrefab.Insert(numberOfMags);
				IEntity tempWeapon = GetGame().SpawnEntityPrefab(Resource.Load(weapon),GetGame().GetWorld(),spawnParams);
				WeaponComponent tempWeaponComponent = WeaponComponent.Cast(tempWeapon.FindComponent(WeaponComponent));
				ref array<AttachmentSlotComponent> tempWeaponAttachments = {};
				ref array<AttachmentSlotComponent> weaponAttachments = {};
				tempWeaponComponent.GetAttachments(tempWeaponAttachments);
				weaponSlotComponent.GetAttachments(weaponAttachments);
				string overridedAttachments = "";
				ref array<string> newAttachments = {};
				for(int g = 0; g < tempWeaponAttachments.Count(); g++)
				{
					string tempAttachmentString = "";
					string attachmentString = "";
					if(tempWeaponAttachments[g].GetAttachedEntity())
						tempAttachmentString = tempWeaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(weaponAttachments[g].GetAttachedEntity())
						attachmentString = weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName();
					if(tempAttachmentString != attachmentString)
						newAttachments.Insert(weaponAttachments[g].GetAttachedEntity().GetPrefabData().GetPrefabName());
				}
				overridedAttachments = SCR_StringHelper.Join(",", newAttachments, true);
				m_sniperPrefab.Insert(overridedAttachments);
				delete tempWeapon;
				RefreshRegularWeapon(false, m_sniperPrefab, "SniperString");
				return;
			}
		}
	}
	
	void RemoveSniper()
	{
		m_sniperPrefab = {};
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("SniperString"));
		m_arrayWidget.SetText("");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Helmet Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddHelmet() 
	{ 
		if(!m_CharacterInventory.Get(0))
			return;
		m_item = m_CharacterInventory.Get(0).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_helmetsArray.Insert(m_item);
		RefreshClothing(false, m_helmetsArray, "HelmetArray");
    }
	
	void RemoveHelmet()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HelmetInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_helmetsArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_helmetsArray.RemoveOrdered(index);
		RefreshClothing(false, m_helmetsArray, "HelmetArray");
	}
	
	void ClearHelmets()
	{
		m_helmetsArray = {};
		RefreshClothing(false, m_helmetsArray, "HelmetArray");
	}
	
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Shirt Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddShirt() 
	{ 
		if(!m_CharacterInventory.Get(1))
			return;
		m_item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_shirtsArray.Insert(m_item);
		RefreshClothing(false, m_shirtsArray, "ShirtArray");
    }
	
	void RemoveShirt()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("ShirtInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_shirtsArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_shirtsArray.RemoveOrdered(index);
		RefreshClothing(false, m_shirtsArray, "ShirtArray");
	}
	
	void ClearShirts()
	{
		m_shirtsArray = {};
		RefreshClothing(false, m_shirtsArray, "ShirtArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Armored Vests Members----------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddArmoredVest() 
	{ 
		if(!m_CharacterInventory.Get(2))
			return;
		m_item = m_CharacterInventory.Get(2).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_armoredVestArray.Insert(m_item);
		RefreshClothing(false, m_armoredVestArray, "ArmoredVestArray");
    }
	
	void RemoveArmoredVest()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("ArmoredVestInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_armoredVestArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_armoredVestArray.RemoveOrdered(index);
		RefreshClothing(false, m_armoredVestArray, "ArmoredVestArray");
	}
	
	void ClearArmoredVests()
	{
		m_armoredVestArray = {};
		RefreshClothing(false, m_armoredVestArray, "ArmoredVestArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Pants Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddPants() 
	{ 
		if(!m_CharacterInventory.Get(3))
			return;
		m_item = m_CharacterInventory.Get(3).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_pantsArray.Insert(m_item);
		RefreshClothing(false, m_pantsArray, "PantsArray");
    }
	
	void RemovePants()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("PantsInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_pantsArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_pantsArray.RemoveOrdered(index);
		RefreshClothing(false, m_pantsArray, "PantsArray");
	}
	
	void ClearPants()
	{
		m_pantsArray = {};
		RefreshClothing(false, m_pantsArray, "PantsArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Boots Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddBoots() 
	{ 
		if(!m_CharacterInventory.Get(4))
			return;
		m_item = m_CharacterInventory.Get(4).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_bootsArray.Insert(m_item);
		RefreshClothing(false, m_bootsArray, "BootsArray");
    }
	
	void RemoveBoots()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("BootsInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_bootsArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_bootsArray.RemoveOrdered(index);
		RefreshClothing(false, m_bootsArray, "BootsArray");
	}
	
	void ClearBoots()
	{
		m_bootsArray = {};
		RefreshClothing(false, m_bootsArray, "BootsArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Backpack Members---------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddBackpack() 
	{ 
		if(!m_CharacterInventory.Get(5))
			return;
		m_item = m_CharacterInventory.Get(5).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_backpackArray.Insert(m_item);
		RefreshClothing(false, m_backpackArray, "BackpackArray");
    }
	
	void RemoveBackpack()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("BackpackInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_backpackArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_backpackArray.RemoveOrdered(index);
		RefreshClothing(false, m_backpackArray, "BackpackArray");
	}
	
	void ClearBackpacks()
	{
		m_backpackArray = {};
		RefreshClothing(false, m_backpackArray, "BackpackArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Vest Members-------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddVest() 
	{ 
		if(!m_CharacterInventory.Get(6))
			return;
		m_item = m_CharacterInventory.Get(6).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_vestArray.Insert(m_item);
		RefreshClothing(false, m_vestArray, "VestArray");
    }
	
	void RemoveVest()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("VestInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_vestArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_vestArray.RemoveOrdered(index);
		RefreshClothing(false, m_vestArray, "VestArray");
	}
	
	void ClearVests()
	{
		m_vestArray = {};
		RefreshClothing(false, m_vestArray, "VestArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Handwear Members---------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddHandwear() 
	{ 
		if(!m_CharacterInventory.Get(7))
			return;
		m_item = m_CharacterInventory.Get(7).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_handwearArray.Insert(m_item);
		RefreshClothing(false, m_handwearArray, "HandwearArray");
    }
	
	void RemoveHandwear()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HandwearInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_handwearArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_handwearArray.RemoveOrdered(index);
		RefreshClothing(false, m_handwearArray, "HandwearArray");
	}
	
	void ClearHandwear()
	{
		m_handwearArray = {};
		RefreshClothing(false, m_handwearArray, "HandwearArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Head Members-------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
void AddHead() 
{ 
	if(!m_CharacterInventory.Get(8))
			return;
	m_item = m_CharacterInventory.Get(8).GetPrefabData().GetPrefabName();
	if(!m_item)
		return;
	m_headArray.Insert(m_item);
	RefreshClothing(false, m_headArray, "HeadArray");
}

void RemoveHead()
{
	EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HeadInput"));
	
	if(inputWidget.GetText().ToInt() == 0)
		return;
	
	if(inputWidget.GetText().ToInt() > m_headArray.Count())
		return;
	
	int index = inputWidget.GetText().ToInt() - 1;
	m_headArray.RemoveOrdered(index);
	RefreshClothing(false, m_headArray, "HeadArray");
}

void ClearHead()
{
	m_headArray = {};
	RefreshClothing(false, m_headArray, "HeadArray");
}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Eyes Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddEyes() 
	{ 
		if(!m_CharacterInventory.Get(9))
			return;
		m_item = m_CharacterInventory.Get(9).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_eyesArray.Insert(m_item);
		RefreshClothing(false, m_eyesArray, "EyesArray");
    }
	
	void RemoveEyes()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("EyesInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_eyesArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_eyesArray.RemoveOrdered(index);
		RefreshClothing(false, m_eyesArray, "EyesArray");
	}
	
	void ClearEyes()
	{
		m_eyesArray = {};
		RefreshClothing(false, m_eyesArray, "EyesArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Ears Members-------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddEars() 
	{ 
		if(!m_CharacterInventory.Get(10))
			return;
		m_item = m_CharacterInventory.Get(10).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_earsArray.Insert(m_item);
		RefreshClothing(false, m_earsArray, "EarsArray");
    }
	
	void RemoveEars()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("EarsInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_earsArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_earsArray.RemoveOrdered(index);
		RefreshClothing(false, m_earsArray, "EarsArray");
	}
	
	void ClearEars()
	{
		m_earsArray = {};
		RefreshClothing(false, m_earsArray, "EarsArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Face Members-------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddFace() 
	{ 
		if(!m_CharacterInventory.Get(11))
			return;
		m_item = m_CharacterInventory.Get(11).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_faceArray.Insert(m_item);
		RefreshClothing(false, m_faceArray, "FaceArray");
    }
	
	void RemoveFace()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("FaceInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_faceArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_faceArray.RemoveOrdered(index);
		RefreshClothing(false, m_faceArray, "FaceArray");
	}
	
	void ClearFace()
	{
		m_faceArray = {};
		RefreshClothing(false, m_faceArray, "FaceArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Neck Members-------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddNeck() 
	{ 
		if(!m_CharacterInventory.Get(12))
			return;
		m_item = m_CharacterInventory.Get(12).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_neckArray.Insert(m_item);
		RefreshClothing(false, m_neckArray, "NeckArray");
    }
	
	void RemoveNeck()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("NeckInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_neckArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_neckArray.RemoveOrdered(index);
		RefreshClothing(false, m_neckArray, "NeckArray");
	}
	
	void ClearNeck()
	{
		m_neckArray = {};
		RefreshClothing(false, m_neckArray, "NeckArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Extra1 Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddExtra1() 
	{ 
		if(!m_CharacterInventory.Get(13))
			return;
		m_item = m_CharacterInventory.Get(13).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_extra1Array.Insert(m_item);
		RefreshClothing(false, m_extra1Array, "Extra1Array");
    }
	
	void RemoveExtra1()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra1Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra1Array.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra1Array.RemoveOrdered(index);
		RefreshClothing(false, m_extra1Array, "Extra1Array");
	}
	
	void ClearExtra1()
	{
		m_extra1Array = {};
		RefreshClothing(false, m_extra1Array, "Extra1Array");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Extra2 Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddExtra2() 
	{ 
		if(!m_CharacterInventory.Get(14))
			return;
		m_item = m_CharacterInventory.Get(14).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_extra2Array.Insert(m_item);
		RefreshClothing(false, m_extra2Array, "Extra2Input");
    }
	
	void RemoveExtra2()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra2Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra2Array.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra2Array.RemoveOrdered(index);
		RefreshClothing(false, m_extra2Array, "Extra2Input");
	}
	
	void ClearExtra2()
	{
		m_extra2Array = {};
		RefreshClothing(false, m_extra2Array, "Extra2Input");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Waist Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddWaist() 
	{ 
		if(!m_CharacterInventory.Get(15))
			return;
		m_item = m_CharacterInventory.Get(15).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_waistArray.Insert(m_item);
		RefreshClothing(false, m_waistArray, "WaistArray");
    }
	
	void RemoveWaist()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("WaistInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_waistArray.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_waistArray.RemoveOrdered(index);
		RefreshClothing(false, m_waistArray, "WaistArray");
	}
	
	void ClearWaist()
	{
		m_waistArray = {};
		RefreshClothing(false, m_waistArray, "WaistArray");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Extra3 Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddExtra3() 
	{ 
		if(!m_CharacterInventory.Get(16))
			return;
		m_item = m_CharacterInventory.Get(16).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_extra3Array.Insert(m_item);
		RefreshClothing(false, m_extra3Array, "Extra3Array");
    }
	
	void RemoveExtra3()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra3Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra3Array.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra3Array.RemoveOrdered(index);
		RefreshClothing(false, m_extra3Array, "Extra3Array");
	}
	
	void ClearExtra3()
	{
		m_extra3Array = {};
		RefreshClothing(false, m_extra3Array, "Extra3Array");
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Extra4 Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddExtra4() 
	{ 
		if(!m_CharacterInventory.Get(17))
			return;
		m_item = m_CharacterInventory.Get(17).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_extra4Array.Insert(m_item);
		RefreshClothing(false, m_extra4Array, "Extra4Array");
    }
	
	void RemoveExtra4()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra4Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra4Array.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra4Array.RemoveOrdered(index);
		RefreshClothing(false, m_extra4Array, "Extra4Array");
	}
	
	void ClearExtra4()
	{
		m_extra4Array = {};
		RefreshClothing(false, m_extra4Array, "Extra4Array");
	}
}