modded enum ChimeraMenuPreset 
{ 
  CRF_GearscriptUI
}

class CRF_Gearscript_OverlayClass: ChimeraMenuBase 
{
	protected Widget m_wRoot;
	protected SCR_ButtonTextComponent comp;
	IEntity myCallerEntity; // (Added) this variable will be used only to show how to transfer parameters to the GUI. 
	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen() 
	{ 
		m_wRoot = GetRootWidget();
      
		// SendHint 
		comp = SCR_ButtonTextComponent.GetButtonText("SendHint", m_wRoot);
      
		if (comp) 
		{
			GetGame().GetWorkspace().SetFocusedWidget(comp.GetRootWidget()); 
			comp.m_OnClicked.Insert(myCustomFunction); 
		} 
	} 
 
	// myCustomFunction called when SendHint Button is pressed 
	void myCustomFunction() 
	{ 
		SCR_HintManagerComponent.GetInstance().ShowCustomHint("This executes when pressing SendHint Button (Using OnClick)", "MY GUI", 3.0);
    }
}