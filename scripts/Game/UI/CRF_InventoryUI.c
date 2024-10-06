class CRF_InventoryUI : SCR_InventoryMenuUI
{
	protected Widget m_wRoot;
	protected FrameWidget m_hudRoot;
	TextWidget m_arrayWidget;
	string m_item;
	
	protected SCR_ButtonTextComponent m_helemtsButton;
	protected SCR_ButtonTextComponent m_shirtsButton;
	protected SCR_ButtonTextComponent m_armoredVestButton;
	protected SCR_ButtonTextComponent m_pantsButton;
	protected SCR_ButtonTextComponent m_bootsButton;
	protected SCR_ButtonTextComponent m_backpackButton;
	
	ref array<string> m_itemArray = {};
	ref array<string> m_helmetsArrayDisplay = {};
	ref array<string> m_shirtsArrayDisplay = {};
	ref array<string> m_armoredVestArrayDisplay = {};
	ref array<string> m_pantsArrayDisplay = {};
	ref array<string> m_bootsArrayDisplay = {};
	ref array<string> m_backpackArrayDisplay = {};
	
	ref array<ResourceName> m_helmetsArray = {};
	ref array<ResourceName> m_shirtsArray = {};
	ref array<ResourceName> m_armoredVestArray = {};
	ref array<ResourceName> m_pantsArray = {};
	ref array<ResourceName> m_bootsArray = {};
	ref array<ResourceName> m_backpackArray = {};
	
	string m_helemtsArrayString;
	string m_shirtsArrayString;
	string m_armoredVestArrayString;
	string m_pantsArrayString;
	string m_bootsArrayString;
	string m_backpackArrayString;
	
	
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
      
		//Add Buttons on Clicked
		m_helemtsButton.m_OnClicked.Insert(AddHelmet); 
		m_shirtsButton.m_OnClicked.Insert(AddShirt);
		m_armoredVestButton.m_OnClicked.Insert(AddArmoredVest); 
		m_pantsButton.m_OnClicked.Insert(AddPants);
		m_bootsButton.m_OnClicked.Insert(AddBoots); 
		m_backpackButton.m_OnClicked.Insert(AddBackpack);
		
		//Clear Buttons
		SCR_ButtonTextComponent helemtsClearButton = SCR_ButtonTextComponent.GetButtonText("HelmetClear", m_wRoot);
		SCR_ButtonTextComponent shirtsClearButton = SCR_ButtonTextComponent.GetButtonText("ShirtClear", m_wRoot);
		SCR_ButtonTextComponent armoredVestClearButton = SCR_ButtonTextComponent.GetButtonText("ArmoredVestClear", m_wRoot);
		SCR_ButtonTextComponent pantsClearButton = SCR_ButtonTextComponent.GetButtonText("PantsClear", m_wRoot);
		SCR_ButtonTextComponent bootsClearButton = SCR_ButtonTextComponent.GetButtonText("BootsClear", m_wRoot);
		SCR_ButtonTextComponent backpackClearButton = SCR_ButtonTextComponent.GetButtonText("BackpackClear", m_wRoot);
		
		//Clear Buttons on Clicked
		helemtsClearButton.m_OnClicked.Insert(ClearHelmets);
		shirtsClearButton.m_OnClicked.Insert(ClearShirts);
		armoredVestClearButton.m_OnClicked.Insert(ClearArmoredVests);
		pantsClearButton.m_OnClicked.Insert(ClearPants);
		bootsClearButton.m_OnClicked.Insert(ClearBoots);
		backpackClearButton.m_OnClicked.Insert(ClearBackpacks);
		
		//Remove Buttons
		SCR_ButtonTextComponent helemtsRemoveButton = SCR_ButtonTextComponent.GetButtonText("HelmetRemove", m_wRoot);
		SCR_ButtonTextComponent shirtsRemoveButton = SCR_ButtonTextComponent.GetButtonText("ShirtRemove", m_wRoot);
		SCR_ButtonTextComponent armoredVestRemoveButton = SCR_ButtonTextComponent.GetButtonText("ArmoredVestRemove", m_wRoot);
		SCR_ButtonTextComponent pantsRemoveButton = SCR_ButtonTextComponent.GetButtonText("PantsRemove", m_wRoot);
		SCR_ButtonTextComponent bootsRemoveButton = SCR_ButtonTextComponent.GetButtonText("BootsRemove", m_wRoot);
		SCR_ButtonTextComponent backpackRemoveButton = SCR_ButtonTextComponent.GetButtonText("BackpackRemove", m_wRoot);
		
		//Remove Buttons on Clicked
		helemtsRemoveButton.m_OnClicked.Insert(RemoveHelmet);
		shirtsRemoveButton.m_OnClicked.Insert(RemoveShirt);
		
	}
	
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Helmet Members-----------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddHelmet() 
	{ 
		m_item = m_CharacterInventory.Get(0).GetPrefabData().GetPrefabName();
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
		m_item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
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
		m_item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
		m_shirtsArray.Insert(m_item);
		m_item.Split("/", m_itemArray, false);
		m_shirtsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_shirtsArrayString = SCR_StringHelper.Join(",", m_shirtsArrayDisplay, true);
		m_arrayWidget.SetText(m_shirtsArrayString);
    }
	
	void RemoveArmoredVest()
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
	
	void ClearArmoredVests()
	{
		m_shirtsArray = {};
		m_shirtsArrayDisplay = {};
		m_shirtsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_arrayWidget.SetText(m_shirtsArrayString);
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Pants Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddPants() 
	{ 
		m_item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
		m_shirtsArray.Insert(m_item);
		m_item.Split("/", m_itemArray, false);
		m_shirtsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_shirtsArrayString = SCR_StringHelper.Join(",", m_shirtsArrayDisplay, true);
		m_arrayWidget.SetText(m_shirtsArrayString);
    }
	
	void RemovePants()
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
	
	void ClearPants()
	{
		m_shirtsArray = {};
		m_shirtsArrayDisplay = {};
		m_shirtsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_arrayWidget.SetText(m_shirtsArrayString);
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Boots Members------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddBoots() 
	{ 
		m_item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
		m_shirtsArray.Insert(m_item);
		m_item.Split("/", m_itemArray, false);
		m_shirtsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_shirtsArrayString = SCR_StringHelper.Join(",", m_shirtsArrayDisplay, true);
		m_arrayWidget.SetText(m_shirtsArrayString);
    }
	
	void RemoveBoots()
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
	
	void ClearBoots()
	{
		m_shirtsArray = {};
		m_shirtsArrayDisplay = {};
		m_shirtsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_arrayWidget.SetText(m_shirtsArrayString);
	}
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------Backpack Members---------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------------------------------------
	void AddBackpack() 
	{ 
		m_item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
		m_shirtsArray.Insert(m_item);
		m_item.Split("/", m_itemArray, false);
		m_shirtsArrayDisplay.Insert(m_itemArray.Get(m_itemArray.Count() - 1));
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_shirtsArrayString = SCR_StringHelper.Join(",", m_shirtsArrayDisplay, true);
		m_arrayWidget.SetText(m_shirtsArrayString);
    }
	
	void RemoveBackpack()
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
	
	void ClearBackpacks()
	{
		m_shirtsArray = {};
		m_shirtsArrayDisplay = {};
		m_shirtsArrayString = "";
		m_arrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtArray"));
		m_arrayWidget.SetText(m_shirtsArrayString);
	}
}