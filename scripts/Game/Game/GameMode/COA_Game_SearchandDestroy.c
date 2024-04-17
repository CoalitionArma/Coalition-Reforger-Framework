class CRF_GameMode_SearchAndDestroyClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_GameMode_SearchAndDestroyComponent: SCR_BaseGameModeComponent
{
	protected bool aSiteDestroyed, bSiteDestroyed = false;
	protected int sitesDestroyed = 0;
	protected IEntity aSite, bSite;
	protected EntitySpawnParams spawnParams;
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Only run on in-game post init
		if (!GetGame().InPlayMode()) 
		{
			return;
		}
		
		SCR_PopUpNotification.GetInstance().PopupMsg("Starting Search and Destroy",10);
	}
	
	void createMarkers()
	{
		// Create markers on each bombsite
	}
	
	void createBombSites()
	{
		// Spawn destructible "site" at each trigger point
		GetGame().SpawnEntity(,GetGame().GetWorld(),);
		// Create markers on each bomb site
		createMarkers();
		// Attach actions to each bomb site
		
	}
	
	void siteDestroyed() 
	{
		sitesDestroyed++;
		if (sitesDestroyed == 2)
	}
}