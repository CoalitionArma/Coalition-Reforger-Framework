class CRF_InventoryUI : SCR_InventoryMenuUI
{
	protected Widget m_wRoot;
	protected FrameWidget m_hudRoot;
	TextWidget m_arrayWidget;
	string m_item;
	ref array<string> m_itemArray = {};
	
	protected SCR_ButtonTextComponent m_helemtsButton;
	protected SCR_ButtonTextComponent m_shirtsButton;
	protected SCR_ButtonTextComponent m_armoredVestButton;
	protected SCR_ButtonTextComponent m_pantsButton;
	protected SCR_ButtonTextComponent m_bootsButton;
	protected SCR_ButtonTextComponent m_backpackButton;
	protected SCR_ButtonTextComponent m_vestButton;
	protected SCR_ButtonTextComponent m_handwearButton;
	protected SCR_ButtonTextComponent m_headButton;
	protected SCR_ButtonTextComponent m_eyesButton;
	protected SCR_ButtonTextComponent m_earsButton;
	protected SCR_ButtonTextComponent m_faceButton;
	protected SCR_ButtonTextComponent m_neckButton;
	protected SCR_ButtonTextComponent m_extra1Button;
	protected SCR_ButtonTextComponent m_extra2Button;
	protected SCR_ButtonTextComponent m_waistButton;
	protected SCR_ButtonTextComponent m_extra3Button;
	protected SCR_ButtonTextComponent m_extra4Button;
	
	ref array<string> m_helmetsArrayDisplay = {};
	ref array<string> m_shirtsArrayDisplay = {};
	ref array<string> m_armoredVestArrayDisplay = {};
	ref array<string> m_pantsArrayDisplay = {};
	ref array<string> m_bootsArrayDisplay = {};
	ref array<string> m_backpackArrayDisplay = {};
	ref array<string> m_vestArrayDisplay = {};
	ref array<string> m_handwearArrayDisplay = {};
	ref array<string> m_headArrayDisplay = {};
	ref array<string> m_eyesArrayDisplay = {};
	ref array<string> m_earsArrayDisplay = {};
	ref array<string> m_faceArrayDisplay = {};
	ref array<string> m_neckArrayDisplay = {};
	ref array<string> m_extra1ArrayDisplay = {};
	ref array<string> m_extra2ArrayDisplay = {};
	ref array<string> m_waistArrayDisplay = {};
	ref array<string> m_extra3ArrayDisplay = {};
	ref array<string> m_extra4ArrayDisplay = {};
	
	ref array<ResourceName> m_helmetsArray = {};
	ref array<ResourceName> m_shirtsArray = {};
	ref array<ResourceName> m_armoredVestArray = {};
	ref array<ResourceName> m_pantsArray = {};
	ref array<ResourceName> m_bootsArray = {};
	ref array<ResourceName> m_backpackArray = {};
	ref array<ResourceName> m_vestArray = {};
	ref array<ResourceName> m_handwearArray = {};
	ref array<ResourceName> m_headArray = {};
	ref array<ResourceName> m_eyesArray = {};
	ref array<ResourceName> m_earsArray = {};
	ref array<ResourceName> m_faceArray = {};
	ref array<ResourceName> m_neckArray = {};
	ref array<ResourceName> m_extra1Array = {};
	ref array<ResourceName> m_extra2Array = {};
	ref array<ResourceName> m_waistArray = {};
	ref array<ResourceName> m_extra3Array = {};
	ref array<ResourceName> m_extra4Array = {};
		
	string m_helemtsArrayString;
	string m_shirtsArrayString;
	string m_armoredVestArrayString;
	string m_pantsArrayString;
	string m_bootsArrayString;
	string m_backpackArrayString;
	string m_vestArrayString;
	string m_handwearArrayString;
	string m_headArrayString;
	string m_eyesArrayString;
	string m_earsArrayString;
	string m_faceArrayString;
	string m_neckArrayString;
	string m_extra1ArrayString;
	string m_extra2ArrayString;
	string m_waistArrayString;
	string m_extra3ArrayString;
	string m_extra4ArrayString;
	
	
	SCR_CharacterInventoryStorageComponent m_CharacterInventory = SCR_CharacterInventoryStorageComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterInventoryStorageComponent));
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		m_wRoot = GetRootWidget();
		m_hudRoot = FrameWidget.Cast(m_wRoot.FindWidget("HudManagerLayout"));
		if( !Init() )
		{
			Action_CloseInventory();
			return;
		}

		GetGame().SetViewDistance(GetGame().GetMinimumViewDistance());

		ShowVicinity();

		m_bProcessInitQueue = true;

		if (m_pPreviewManager)
		{
			m_wPlayerRender = ItemPreviewWidget.Cast( m_widget.FindAnyWidget( "playerRender" ) );
			auto collection = m_StorageManager.GetAttributes();
			if (collection)
				m_PlayerRenderAttributes = PreviewRenderAttributes.Cast(collection.FindAttribute(SCR_CharacterInventoryPreviewAttributes));

			m_pCharacterWidgetHelper = SCR_InventoryCharacterWidgetHelper(m_wPlayerRender, GetGame().GetWorkspace() );
		}

		if( m_pNavigationBar )
			m_pNavigationBar.m_OnAction.Insert(OnAction);

		GetGame().GetInputManager().AddActionListener("Inventory_Drag", EActionTrigger.DOWN, Action_DragDown);
		GetGame().GetInputManager().AddActionListener("Inventory", EActionTrigger.DOWN, Action_CloseInventory);
		InitAttachmentSpinBox();
		OnInputDeviceIsGamepad(!GetGame().GetInputManager().IsUsingMouseAndKeyboard());
		GetGame().OnInputDeviceIsGamepadInvoker().Insert(OnInputDeviceIsGamepad);		
		
		SetAttachmentSpinBoxActive(m_bIsUsingGamepad);
		
		ResetHighlightsOnAvailableStorages();
		SetOpenStorage();
		UpdateTotalWeightText();
		
		InitializeCharacterHitZones();
		InitializeCharacterInformation();
		UpdateCharacterPreview();
		
		m_wRoot = GetRootWidget();
		
		Print("Custom inventory opened");
      
		//Add Buttons
		m_helemtsButton = SCR_ButtonTextComponent.GetButtonText("Helmet", m_wRoot);
		m_shirtsButton = SCR_ButtonTextComponent.GetButtonText("Shirt", m_wRoot);
		m_armoredVestButton = SCR_ButtonTextComponent.GetButtonText("ArmoredVest", m_wRoot);
		m_pantsButton = SCR_ButtonTextComponent.GetButtonText("Pants", m_wRoot);
		m_bootsButton = SCR_ButtonTextComponent.GetButtonText("Boots", m_wRoot);
		m_backpackButton = SCR_ButtonTextComponent.GetButtonText("Backpack", m_wRoot);
		m_vestButton = SCR_ButtonTextComponent.GetButtonText("Vest", m_wRoot);
		m_handwearButton = SCR_ButtonTextComponent.GetButtonText("Handwear", m_wRoot);
		m_headButton = SCR_ButtonTextComponent.GetButtonText("Head", m_wRoot);
		m_eyesButton = SCR_ButtonTextComponent.GetButtonText("Eyes", m_wRoot);
		m_earsButton = SCR_ButtonTextComponent.GetButtonText("Ears", m_wRoot);
		m_faceButton = SCR_ButtonTextComponent.GetButtonText("Face", m_wRoot);
		m_neckButton = SCR_ButtonTextComponent.GetButtonText("Neck", m_wRoot);
		m_extra1Button = SCR_ButtonTextComponent.GetButtonText("Extra1", m_wRoot);
		m_extra2Button = SCR_ButtonTextComponent.GetButtonText("Extra2", m_wRoot);
		m_waistButton = SCR_ButtonTextComponent.GetButtonText("Waist", m_wRoot);
		m_extra3Button = SCR_ButtonTextComponent.GetButtonText("Extra3", m_wRoot);
		m_extra4Button = SCR_ButtonTextComponent.GetButtonText("Extra4", m_wRoot);
      
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
		
	}
	
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Helmet Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddHelmet() 
	{ 
		m_item = m_CharacterInventory.Get(0).GetPrefabData().GetPrefabName();
		if(!m_item)
			return;
		m_helmetsArray.Insert(m_item);
		m_item.Split("/", m_itemArray, false);
		m_helmetsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HelmetArray"));
		m_helemtsArrayString = SCR_StringHelper.Join(",", m_helmetsArrayDisplay, true);
		m_arrayWidget.SetText(m_helemtsArrayString);
    }
	
	void RemoveHelmet()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HelmetInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_helmetsArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_helmetsArray.Remove(index);
		m_helmetsArrayDisplay.Remove(index);
		m_helemtsArrayString = SCR_StringHelper.Join(",", m_helmetsArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HelmetArray"));
		m_arrayWidget.SetText(m_helemtsArrayString);
	}
	
	void ClearHelmets()
	{
		m_helmetsArray = {};
		m_helmetsArrayDisplay = {};
		m_helemtsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HelmetArray"));
		m_arrayWidget.SetText(m_helemtsArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_shirtsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_shirtsArrayString = SCR_StringHelper.Join(",", m_shirtsArrayDisplay, true);
		m_arrayWidget.SetText(m_shirtsArrayString);
    }
	
	void RemoveShirt()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("ShirtInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_shirtsArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_shirtsArray.Remove(index);
		m_shirtsArrayDisplay.Remove(index);
		m_shirtsArrayString = SCR_StringHelper.Join(",", m_shirtsArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_arrayWidget.SetText(m_shirtsArrayString);
	}
	
	void ClearShirts()
	{
		m_shirtsArray = {};
		m_shirtsArrayDisplay = {};
		m_shirtsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_arrayWidget.SetText(m_shirtsArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_armoredVestArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ArmoredVestArray"));
		m_armoredVestArrayString = SCR_StringHelper.Join(",", m_armoredVestArrayDisplay, true);
		m_arrayWidget.SetText(m_armoredVestArrayString);
    }
	
	void RemoveArmoredVest()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("ArmoredVestInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_shirtsArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_armoredVestArray.Remove(index);
		m_armoredVestArrayDisplay.Remove(index);
		m_armoredVestArrayString = SCR_StringHelper.Join(",", m_armoredVestArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ArmoredVestArray"));
		m_arrayWidget.SetText(m_armoredVestArrayString);
	}
	
	void ClearArmoredVests()
	{
		m_armoredVestArray = {};
		m_armoredVestArrayDisplay = {};
		m_armoredVestArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ArmoredVestArray"));
		m_arrayWidget.SetText(m_armoredVestArrayString);
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
		m_bootsArray.Insert(m_item);
		m_item.Split("/", m_itemArray, false);
		m_pantsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("PantsArray"));
		m_pantsArrayString = SCR_StringHelper.Join(",", m_pantsArrayDisplay, true);
		m_arrayWidget.SetText(m_pantsArrayString);
    }
	
	void RemovePants()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("PantsInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_pantsArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_pantsArray.Remove(index);
		m_pantsArrayDisplay.Remove(index);
		m_pantsArrayString = SCR_StringHelper.Join(",", m_pantsArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("PantsArray"));
		m_arrayWidget.SetText(m_pantsArrayString);
	}
	
	void ClearPants()
	{
		m_pantsArray = {};
		m_pantsArrayDisplay = {};
		m_pantsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("PantsArray"));
		m_arrayWidget.SetText(m_pantsArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_bootsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("BootsArray"));
		m_bootsArrayString = SCR_StringHelper.Join(",", m_bootsArrayDisplay, true);
		m_arrayWidget.SetText(m_bootsArrayString);
    }
	
	void RemoveBoots()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("BootsInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_bootsArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_bootsArray.Remove(index);
		m_bootsArrayDisplay.Remove(index);
		m_bootsArrayString = SCR_StringHelper.Join(",", m_bootsArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("BootsArray"));
		m_arrayWidget.SetText(m_bootsArrayString);
	}
	
	void ClearBoots()
	{
		m_bootsArray = {};
		m_bootsArrayDisplay = {};
		m_bootsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("BootsArray"));
		m_arrayWidget.SetText(m_bootsArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_backpackArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("BackpackArray"));
		m_backpackArrayString = SCR_StringHelper.Join(",", m_backpackArrayDisplay, true);
		m_arrayWidget.SetText(m_backpackArrayString);
    }
	
	void RemoveBackpack()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("BackpackInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_backpackArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_backpackArray.Remove(index);
		m_backpackArrayDisplay.Remove(index);
		m_backpackArrayString = SCR_StringHelper.Join(",", m_backpackArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("BackpackArray"));
		m_arrayWidget.SetText(m_backpackArrayString);
	}
	
	void ClearBackpacks()
	{
		m_backpackArray = {};
		m_backpackArrayDisplay = {};
		m_backpackArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("BackpackArray"));
		m_arrayWidget.SetText(m_backpackArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_vestArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("VestArray"));
		m_vestArrayString = SCR_StringHelper.Join(",", m_vestArrayDisplay, true);
		m_arrayWidget.SetText(m_vestArrayString);
    }
	
	void RemoveVest()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("VestInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_vestArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_vestArray.Remove(index);
		m_vestArrayDisplay.Remove(index);
		m_vestArrayString = SCR_StringHelper.Join(",", m_vestArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("VestArray"));
		m_arrayWidget.SetText(m_vestArrayString);
	}
	
	void ClearVests()
	{
		m_vestArray = {};
		m_vestArrayDisplay = {};
		m_vestArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("VestArray"));
		m_arrayWidget.SetText(m_vestArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_handwearArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HandwearArray"));
		m_handwearArrayString = SCR_StringHelper.Join(",", m_handwearArrayDisplay, true);
		m_arrayWidget.SetText(m_handwearArrayString);
    }
	
	void RemoveHandwear()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HandwearInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_handwearArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_handwearArray.Remove(index);
		m_handwearArrayDisplay.Remove(index);
		m_handwearArrayString = SCR_StringHelper.Join(",", m_handwearArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HandwearArray"));
		m_arrayWidget.SetText(m_handwearArrayString);
	}
	
	void ClearHandwear()
	{
		m_handwearArray = {};
		m_handwearArrayDisplay = {};
		m_handwearArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HandwearArray"));
		m_arrayWidget.SetText(m_handwearArrayString);
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
	m_item.Split("/", m_itemArray, false);
	m_headArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
	m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HeadArray"));
	m_headArrayString = SCR_StringHelper.Join(",", m_headArrayDisplay, true);
	m_arrayWidget.SetText(m_headArrayString);
}

void RemoveHead()
{
	EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HeadInput"));
	
	if(inputWidget.GetText().ToInt() == 0)
		return;
	
	if(inputWidget.GetText().ToInt() > m_headArrayDisplay.Count())
		return;
	
	int index = inputWidget.GetText().ToInt() - 1;
	m_headArray.Remove(index);
	m_headArrayDisplay.Remove(index);
	m_headArrayString = SCR_StringHelper.Join(",", m_headArrayDisplay, true);
	m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HeadArray"));
	m_arrayWidget.SetText(m_headArrayString);
}

void ClearHead()
{
	m_headArray = {};
	m_headArrayDisplay = {};
	m_headArrayString = "";
	m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HeadArray"));
	m_arrayWidget.SetText(m_headArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_eyesArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("EyesArray"));
		m_eyesArrayString = SCR_StringHelper.Join(",", m_eyesArrayDisplay, true);
		m_arrayWidget.SetText(m_eyesArrayString);
    }
	
	void RemoveEyes()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("EyesInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_eyesArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_eyesArray.Remove(index);
		m_eyesArrayDisplay.Remove(index);
		m_eyesArrayString = SCR_StringHelper.Join(",", m_eyesArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("EyesArray"));
		m_arrayWidget.SetText(m_eyesArrayString);
	}
	
	void ClearEyes()
	{
		m_eyesArray = {};
		m_eyesArrayDisplay = {};
		m_eyesArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("EyesArray"));
		m_arrayWidget.SetText(m_eyesArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_earsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("EarsArray"));
		m_earsArrayString = SCR_StringHelper.Join(",", m_earsArrayDisplay, true);
		m_arrayWidget.SetText(m_earsArrayString);
    }
	
	void RemoveEars()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("EarsInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_earsArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_earsArray.Remove(index);
		m_earsArrayDisplay.Remove(index);
		m_earsArrayString = SCR_StringHelper.Join(",", m_earsArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("EarsArray"));
		m_arrayWidget.SetText(m_earsArrayString);
	}
	
	void ClearEars()
	{
		m_earsArray = {};
		m_earsArrayDisplay = {};
		m_earsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("EarsArray"));
		m_arrayWidget.SetText(m_earsArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_faceArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("FaceArray"));
		m_faceArrayString = SCR_StringHelper.Join(",", m_faceArrayDisplay, true);
		m_arrayWidget.SetText(m_faceArrayString);
    }
	
	void RemoveFace()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("FaceInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_faceArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_faceArray.Remove(index);
		m_faceArrayDisplay.Remove(index);
		m_faceArrayString = SCR_StringHelper.Join(",", m_faceArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("FaceArray"));
		m_arrayWidget.SetText(m_faceArrayString);
	}
	
	void ClearFace()
	{
		m_faceArray = {};
		m_faceArrayDisplay = {};
		m_faceArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("FaceArray"));
		m_arrayWidget.SetText(m_faceArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_neckArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("NeckArray"));
		m_neckArrayString = SCR_StringHelper.Join(",", m_neckArrayDisplay, true);
		m_arrayWidget.SetText(m_neckArrayString);
    }
	
	void RemoveNeck()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("NeckInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_neckArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_neckArray.Remove(index);
		m_neckArrayDisplay.Remove(index);
		m_neckArrayString = SCR_StringHelper.Join(",", m_neckArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("NeckArray"));
		m_arrayWidget.SetText(m_neckArrayString);
	}
	
	void ClearNeck()
	{
		m_neckArray = {};
		m_neckArrayDisplay = {};
		m_neckArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("NeckArray"));
		m_arrayWidget.SetText(m_neckArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_extra1ArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra1Array"));
		m_extra1ArrayString = SCR_StringHelper.Join(",", m_extra1ArrayDisplay, true);
		m_arrayWidget.SetText(m_extra1ArrayString);
    }
	
	void RemoveExtra1()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra1Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra1ArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra1Array.Remove(index);
		m_extra1ArrayDisplay.Remove(index);
		m_extra1ArrayString = SCR_StringHelper.Join(",", m_extra1ArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra1Array"));
		m_arrayWidget.SetText(m_extra1ArrayString);
	}
	
	void ClearExtra1()
	{
		m_extra1Array = {};
		m_extra1ArrayDisplay = {};
		m_extra1ArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra1Array"));
		m_arrayWidget.SetText(m_extra1ArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_extra2ArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra2Array"));
		m_extra2ArrayString = SCR_StringHelper.Join(",", m_extra2ArrayDisplay, true);
		m_arrayWidget.SetText(m_extra2ArrayString);
    }
	
	void RemoveExtra2()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra2Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra2ArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra2Array.Remove(index);
		m_extra2ArrayDisplay.Remove(index);
		m_extra2ArrayString = SCR_StringHelper.Join(",", m_extra2ArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra2Array"));
		m_arrayWidget.SetText(m_extra2ArrayString);
	}
	
	void ClearExtra2()
	{
		m_extra2Array = {};
		m_extra2ArrayDisplay = {};
		m_extra2ArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra2Array"));
		m_arrayWidget.SetText(m_extra2ArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_waistArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("WaistArray"));
		m_waistArrayString = SCR_StringHelper.Join(",", m_waistArrayDisplay, true);
		m_arrayWidget.SetText(m_waistArrayString);
    }
	
	void RemoveWaist()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("WaistInput"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_waistArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_waistArray.Remove(index);
		m_waistArrayDisplay.Remove(index);
		m_waistArrayString = SCR_StringHelper.Join(",", m_waistArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("WaistArray"));
		m_arrayWidget.SetText(m_waistArrayString);
	}
	
	void ClearWaist()
	{
		m_waistArray = {};
		m_waistArrayDisplay = {};
		m_waistArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("WaistArray"));
		m_arrayWidget.SetText(m_waistArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_extra3ArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra3Array"));
		m_extra3ArrayString = SCR_StringHelper.Join(",", m_extra3ArrayDisplay, true);
		m_arrayWidget.SetText(m_extra3ArrayString);
    }
	
	void RemoveExtra3()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra3Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra3ArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra3Array.Remove(index);
		m_extra3ArrayDisplay.Remove(index);
		m_extra3ArrayString = SCR_StringHelper.Join(",", m_extra3ArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra3Array"));
		m_arrayWidget.SetText(m_extra3ArrayString);
	}
	
	void ClearExtra3()
	{
		m_extra3Array = {};
		m_extra3ArrayDisplay = {};
		m_extra3ArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra3Array"));
		m_arrayWidget.SetText(m_extra3ArrayString);
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
		m_item.Split("/", m_itemArray, false);
		m_extra4ArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra4Array"));
		m_extra4ArrayString = SCR_StringHelper.Join(",", m_extra4ArrayDisplay, true);
		m_arrayWidget.SetText(m_extra4ArrayString);
    }
	
	void RemoveExtra4()
	{
		EditBoxWidget inputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("Extra4Input"));
		
		if(inputWidget.GetText().ToInt() == 0)
			return;
		
		if(inputWidget.GetText().ToInt() > m_extra4ArrayDisplay.Count())
			return;
		
		int index = inputWidget.GetText().ToInt() - 1;
		m_extra4Array.Remove(index);
		m_extra4ArrayDisplay.Remove(index);
		m_extra4ArrayString = SCR_StringHelper.Join(",", m_extra4ArrayDisplay, true);
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra4Array"));
		m_arrayWidget.SetText(m_extra4ArrayString);
	}
	
	void ClearExtra4()
	{
		m_extra4Array = {};
		m_extra4ArrayDisplay = {};
		m_extra4ArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("Extra4Array"));
		m_arrayWidget.SetText(m_extra4ArrayString);
	}
}