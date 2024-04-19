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
	protected IEntity aSite, bSite;
	
	protected IEntity aSiteTrigger, bSiteTrigger;
	protected int aSiteTimer = 120;
	protected int bSiteTimer = 120;
	protected vector aSiteSpawn;
	protected vector bSiteSpawn;
	
	[RplProp(onRplName: "ShowMessage")]
	string m_sMessageContent = "";
	
	[RplProp(onRplName: "PlaySound")]
	string m_SoundString = "";
	
	[RplProp()]
	bool aSitePlanted = false;
	
	[RplProp()]
	bool bSitePlanted = false;
	
	[RplProp()]
	EntityID aSiteID = null;
	
	[RplProp()]
	EntityID bSiteID = null;
	
	[RplProp()]
	bool countDownActive = false;
	
	//------------------------------------------------------------------------------------------------
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
		{
			return;
		}
		
		// Debugging in workbench
		//GetGame().GetCallqueue().CallLater(Debugging, 250, true);
	
		if (Replication.IsServer())
			initBombSites();
	}
	
	/*void createMarkers()
	// No easy way to do this so we use PS marker for now
	{
		// Create markers on each bombsite
		
	}*/
	
	//------------------------------------------------------------------------------------------------
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
		aSiteID = aSite.GetID();
		
		spawnParams.Transform[3] = bSiteSpawn; // change to bsite
		bSite = GetGame().SpawnEntityPrefab(Resource.Load("{451BE00A37C69679}Prefabs/Structures/Military/Radar/ApproachRadar_TPN19_01/ApproachRadar_TPN19_01_generator.et"),GetGame().GetWorld(),spawnParams);
		bSiteID = bSite.GetID();
		
		Replication.BumpMe();
		
		// Create markers on each bomb site
		// createMarkers();
	}
	
	// Debugging in workbench
	//------------------------------------------------------------------------------------------------
	/*
	protected string m_sStoredMessageContent;
	void Debugging() {
		if (m_sStoredMessageContent != m_sMessageContent) {
			m_sStoredMessageContent = m_sMessageContent;
			Print(m_sMessageContent);
		};
	}
	*/

	// Acts as a loop method spawned via calllater, every 1 sec
	//------------------------------------------------------------------------------------------------
	void startCountdown(IEntity bombSitePlanted)
	{
		// Check if defused
		if (!countDownActive) 
		{
			aSitePlanted = false;
			bSitePlanted = false;
			return;
		}
		
		// Show message
		if (aSitePlanted) {
			aSiteTimer--;
			m_sMessageContent = "Bomb Planted╣1╣A SITE: " + SCR_FormatHelper.FormatTime(aSiteTimer);
		} else {
			bSiteTimer--;
			m_sMessageContent = "Bomb Planted╣1╣B SITE: " + SCR_FormatHelper.FormatTime(bSiteTimer);
		};
		
		// Bomb goes off
		if (aSiteTimer <= 0 || bSiteTimer <= 0) 
		{
			if (aSitePlanted) {
				m_sMessageContent = "A SITE DESTROYED!╣15╣";
				aSiteTimer = 120;
			} else {
				m_sMessageContent = "B SITE DESTROYED!╣15╣";
				bSiteTimer = 120;
			};
			
			// Remove timer
			GetGame().GetCallqueue().Remove(startCountdown);
			countDownActive = false;
			// Destroy site
			siteDestroyed(bombSitePlanted);
			aSitePlanted = false;
			bSitePlanted = false;
		}
		
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
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
			m_sMessageContent = "Attackers have destroyed both sites!╣30╣Attacker victory!";
			m_SoundString = "{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav";
		}
		
		countDownActive = false;
		aSitePlanted = false;
		bSitePlanted = false;
		
		Replication.BumpMe();
	}
	
	void ToggleBombPlanted(EntityID entityID, bool togglePlanted) {
		if (!togglePlanted) 
		{
			GetGame().GetCallqueue().Remove(startCountdown);
			countDownActive = false;
			if (aSitePlanted) {
				aSitePlanted = false;
				m_sMessageContent = "Deffenders have defused the bomb at A!╣10╣";
			} else {
				bSitePlanted = false;
				m_sMessageContent = "Deffenders have defused the bomb at B!╣10╣";
			};
		} else {
			// Set which site is planted
			countDownActive = true;
			if (!aSite.IsDeleted() && entityID == aSite.GetID()) {
				aSitePlanted = true;
				m_sMessageContent = "Attackers have placed a bomb at A!╣10╣";
			} else {
				bSitePlanted = true;
				m_sMessageContent = "Attackers have placed a bomb at B!╣10╣";
			};
		
			// Spawn countdown thread
			GetGame().GetCallqueue().CallLater(startCountdown, 1000, true, GetGame().GetWorld().FindEntityByID(entityID));
		};
		
		Replication.BumpMe();
	}
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	// Locality needs verified for workbench and local server hosting
	void ShowMessage()
	{
		array<string> messageSplitArray = {};
		m_sMessageContent.Split("╣", messageSplitArray, false);

		string mainMessage = messageSplitArray[0];
		string time = messageSplitArray[1];
		string subMessage = messageSplitArray[2];

		SCR_PopUpNotification.GetInstance().PopupMsg(mainMessage, time.ToFloat(), subMessage);
	};
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	// Locality needs verified for workbench and local server hosting
	void PlaySound()
	{
		AudioSystem.PlaySound(m_SoundString);
	};
}