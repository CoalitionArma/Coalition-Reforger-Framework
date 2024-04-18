class CRF_GameMode_SearchAndDestroyComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_GameMode_SearchAndDestroyComponent: SCR_BaseGameModeComponent
{
	protected bool aSiteDestroyed, bSiteDestroyed = false;
	protected int sitesDestroyed = 0;
	protected IEntity aSite, bSite;
	IEntity aSiteTrigger, bSiteTrigger;
	
	override protected void OnWorldPostProcess(World world)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg("Initializing Search and Destroy",10);
		initBombSites();
	}
	
	void createMarkers()
	{
		// Create markers on each bombsite
		
	}
	
	void initBombSites()
	{
		aSiteTrigger = GetGame().GetWorld().FindEntityByName("aSiteTrigger");
		bSiteTrigger = GetGame().GetWorld().FindEntityByName("bSiteTrigger");
		vector aSiteSpawn = aSiteTrigger.GetOrigin();
		vector bSiteSpawn = bSiteTrigger.GetOrigin();
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = aSiteSpawn;
		
		// Spawn destructible "site" at each trigger point
		aSite = GetGame().SpawnEntityPrefab(Resource.Load("{451BE00A37C69679}Prefabs/Structures/Military/Radar/ApproachRadar_TPN19_01/ApproachRadar_TPN19_01_generator.et"),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = bSiteSpawn; // change to bsite
		aSite = GetGame().SpawnEntityPrefab(Resource.Load("{451BE00A37C69679}Prefabs/Structures/Military/Radar/ApproachRadar_TPN19_01/ApproachRadar_TPN19_01_generator.et"),GetGame().GetWorld(),spawnParams);
		// Create markers on each bomb site
		createMarkers();
	}
	
	void siteDestroyed(IEntity site) 
	{
		string siteName = "B Site";
		sitesDestroyed++;
		
		if (site == aSite)
		{
			siteName = "A Site";
		}
		
		// Show destruction message
		SCR_PopUpNotification.GetInstance().PopupMsg("Attackers have destroyed " + siteName,5);
		
		if (sitesDestroyed == 2)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("Attackers have destroyed both sites!",30,"Attacker victory!");
		}
	}
}