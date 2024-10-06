class CRF_InventoryUI : SCR_InventoryMenuUI
{
	protected Widget m_wRoot;
	protected FrameWidget m_hudRoot;
	protected SCR_ButtonTextComponent m_HelemtsButton;
	protected SCR_ButtonTextComponent m_ShirtsButton;
	TextWidget m_ArrayWidget;
	string m_Item;
	
	ref array<string> m_ItemArray = {};
	ref array<string> m_HelmetsArrayDisplay = {};
	ref array<string> m_ShirtsArrayDisplay = {};
	
	ref array<ResourceName> m_HelmetsArray = {};
	ref array<ResourceName> m_shirtsArray = {};
	
	string m_HelemtsArrayString;
	string m_ShirtsArrayString;
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
		m_HelemtsButton = SCR_ButtonTextComponent.GetButtonText("Helmets", m_wRoot);
		m_ShirtsButton = SCR_ButtonTextComponent.GetButtonText("Shirts", m_wRoot);
      
		//Add Buttons on Clicked
		m_HelemtsButton.m_OnClicked.Insert(AddHelmet); 
		m_ShirtsButton.m_OnClicked.Insert(AddShirt);
		
		//Clear Buttons
		SCR_ButtonTextComponent HelemtsClearButton = SCR_ButtonTextComponent.GetButtonText("HelmetsClear", m_wRoot);
		SCR_ButtonTextComponent ShirtsClearButton = SCR_ButtonTextComponent.GetButtonText("ShirtsClear", m_wRoot);
		
		//Clear Buttons on Clicked
		HelemtsClearButton.m_OnClicked.Insert(ClearHelmets);
		ShirtsClearButton.m_OnClicked.Insert(ClearShirts);
		
		//Remove Buttons
		SCR_ButtonTextComponent HelemtsRemoveButton = SCR_ButtonTextComponent.GetButtonText("HelmetRemove", m_wRoot);
		SCR_ButtonTextComponent ShirtsRemoveButton = SCR_ButtonTextComponent.GetButtonText("ShirtRemove", m_wRoot);
		
		//Remove Buttons on Clicked
		HelemtsRemoveButton.m_OnClicked.Insert(RemoveHelmet);
		ShirtsRemoveButton.m_OnClicked.Insert(RemoveShirt);
	}
	
	void AddHelmet() 
	{ 
		m_Item = m_CharacterInventory.Get(0).GetPrefabData().GetPrefabName();
		m_HelmetsArray.Insert(m_Item);
		m_Item.Split("/", m_ItemArray, false);
		m_HelmetsArrayDisplay.Insert(m_ItemArray.Get(m_ItemArray.Count() - 1));
		m_ArrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HelmetsArray"));
		m_HelemtsArrayString = SCR_StringHelper.Join(",", m_HelmetsArrayDisplay, true);
		m_ArrayWidget.SetText(m_HelemtsArrayString);
    }
	
	void RemoveHelmet()
	{
		EditBoxWidget InputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("HelmetInput"));
		
		if(InputWidget.GetText().ToInt() == 0)
			return;
		
		if(InputWidget.GetText().ToInt() > m_HelmetsArrayDisplay.Count())
			return;
		
		int index = InputWidget.GetText().ToInt() - 1;
		m_HelmetsArray.Remove(index);
		m_HelmetsArrayDisplay.Remove(index);
		m_HelemtsArrayString = SCR_StringHelper.Join(",", m_HelmetsArrayDisplay, true);
		m_ArrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HelmetsArray"));
		m_ArrayWidget.SetText(m_HelemtsArrayString);
	}
	
	void ClearHelmets()
	{
		m_HelmetsArray = {};
		m_HelmetsArrayDisplay = {};
		m_HelemtsArrayString = "";
		m_ArrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("HelmetsArray"));
		m_ArrayWidget.SetText(m_HelemtsArrayString);
	}
	
	void AddShirt() 
	{ 
		m_Item = m_CharacterInventory.Get(1).GetPrefabData().GetPrefabName();
		m_shirtsArray.Insert(m_Item);
		m_Item.Split("/", m_ItemArray, false);
		m_ShirtsArrayDisplay.Insert(m_ItemArray.Get(m_ItemArray.Count() - 1));
		m_ArrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtsArray"));
		m_ShirtsArrayString = SCR_StringHelper.Join(",", m_ShirtsArrayDisplay, true);
		m_ArrayWidget.SetText(m_ShirtsArrayString);
    }
	
	void RemoveShirt()
	{
		EditBoxWidget InputWidget = EditBoxWidget.Cast(m_hudRoot.FindWidget("ShirtInput"));
		
		if(InputWidget.GetText().ToInt() == 0)
			return;
		
		if(InputWidget.GetText().ToInt() > m_ShirtsArrayDisplay.Count())
			return;
		
		int index = InputWidget.GetText().ToInt() - 1;
		m_shirtsArray.Remove(index);
		m_ShirtsArrayDisplay.Remove(index);
		m_ShirtsArrayString = SCR_StringHelper.Join(",", m_ShirtsArrayDisplay, true);
		m_ArrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtsArray"));
		m_ArrayWidget.SetText(m_ShirtsArrayString);
	}
	
	void ClearShirts()
	{
		m_shirtsArray = {};
		m_ShirtsArrayDisplay = {};
		m_ShirtsArrayString = "";
		m_ArrayWidget = TextWidget.Cast(m_hudRoot.FindWidget("ShirtsArray"));
		m_ArrayWidget.SetText(m_ShirtsArrayString);
	}
}