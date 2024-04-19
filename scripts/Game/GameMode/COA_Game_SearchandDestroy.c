class CRF_GameMode_SearchAndDestroyComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_GameMode_SearchAndDestroyComponent: SCR_BaseGameModeComponent
{
	[Attribute("US")]
	FactionKey attackingSide;
	
	[Attribute("USSR")]
	FactionKey defendingSide;
	
	protected bool aSiteDestroyed, bSiteDestroyed = false;
	protected int sitesDestroyed = 0;
	IEntity aSite, bSite;
	
	IEntity aSiteTrigger, bSiteTrigger;
	AudioSystem as;
	protected int BOMBTIMER = 120;
	bool aSitePlanted = false;
	bool bSitePlanted = false;
	bool countDownActive = false;
	vector aSiteSpawn;
	vector bSiteSpawn;
	
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
		{
			return;
		}
	
		initBombSites();
	}
	
	/*void createMarkers()
	// No easy way to do this so we use PS marker for now
	{
		// Create markers on each bombsite
		
	}*/
	
	void initBombSites()
	{
		SCR_PopUpNotification.GetInstance().PopupMsg("Initializing Search and Destroy",10);
		aSiteTrigger = GetGame().GetWorld().FindEntityByName("aSiteTrigger");
		bSiteTrigger = GetGame().GetWorld().FindEntityByName("bSiteTrigger");
		aSiteSpawn = aSiteTrigger.GetOrigin();
		bSiteSpawn = bSiteTrigger.GetOrigin();
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = aSiteSpawn;
		
		// Spawn destructible "site" at each trigger point
		aSite = GetGame().SpawnEntityPrefab(Resource.Load("{451BE00A37C69679}Prefabs/Structures/Military/Radar/ApproachRadar_TPN19_01/ApproachRadar_TPN19_01_generator.et"),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = bSiteSpawn; // change to bsite
		bSite = GetGame().SpawnEntityPrefab(Resource.Load("{451BE00A37C69679}Prefabs/Structures/Military/Radar/ApproachRadar_TPN19_01/ApproachRadar_TPN19_01_generator.et"),GetGame().GetWorld(),spawnParams);
		// Create markers on each bomb site
		// createMarkers();
	}
	
	// Acts as a loop method spawned via calllater, every 1 sec
	void startCountdown(IEntity bombSitePlanted)
	{
		// Check if defused
		if (!countDownActive) 
		{
			aSitePlanted = false;
			bSitePlanted = false;
			return;
		}
		
		// Subtract 1 sec
		BOMBTIMER--;
		string timetoshow = SCR_FormatHelper.FormatTime(BOMBTIMER);
		// Show message
		if (aSitePlanted)
			SCR_PopUpNotification.GetInstance().PopupMsg("Bomb Planted", 1, "A SITE: " + timetoshow);
		else
			SCR_PopUpNotification.GetInstance().PopupMsg("Bomb Planted", 1, "B SITE: " + timetoshow);
		
		// Bomb goes off
		if (BOMBTIMER <= 0) {
			if (aSitePlanted) {
				SCR_PopUpNotification.GetInstance().PopupMsg("A SITE DESTROYED!");
			} else {
				SCR_PopUpNotification.GetInstance().PopupMsg("B SITE DESTROYED!");
			}
				
			
			// Remove timer
			GetGame().GetCallqueue().Remove(startCountdown);
			countDownActive = false;
			// Destroy site
			siteDestroyed(bombSitePlanted);
			// Reset Timer
			BOMBTIMER = 120;
			aSitePlanted = false;
			bSitePlanted = false;
		}
	}
	
	void siteDestroyed(IEntity bombSitePlanted) 
	{
		sitesDestroyed++;
		
		// Spawn explosion at site
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = bombSitePlanted.GetOrigin();
	
		GetGame().SpawnEntityPrefab(Resource.Load("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et"),GetGame().GetWorld(),spawnParams);
		// Delete entity
		delete bombSitePlanted;
		
		if (sitesDestroyed == 2)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("Attackers have destroyed both sites!",30,"Attacker victory!");
			as.PlaySound("{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav");
		}
		
		countDownActive = false;
		aSitePlanted = false;
		bSitePlanted = false;
	}
}