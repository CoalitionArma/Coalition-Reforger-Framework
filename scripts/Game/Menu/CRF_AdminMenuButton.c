class CRF_AdminMenuButton: SCR_ButtonTextComponent
{

//	bool IsAdmin()
//	{
//		// check if player is admin?
//		PlayerController pc = GetGame().GetPlayerController();
//		if (!pc)
//			return false;
//		
//		ServerAdminTools_PlayerControllerComponent adminToolsPCComponent = ServerAdminTools_PlayerControllerComponent.Cast(pc.FindComponent(ServerAdminTools_PlayerControllerComponent));
//		if (!adminToolsPCComponent)
//			return false;
//		
//		return adminToolsPCComponent.IsLocalPlayerAdmin();
//	};
	
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		if (SCR_Global.IsEditMode())
			return;
		
		OpenAdminMenu();
	};
	
	void OpenAdminMenu()
	{	
	}
};