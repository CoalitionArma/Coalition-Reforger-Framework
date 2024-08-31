[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_SearchAndDestroyGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_SearchAndDestroyGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side assaulting the bomb sites")]
	FactionKey attackingSide;
	
	[Attribute("USSR", "auto", "The side deffending the bomb sites")]
	FactionKey defendingSide;
	
	[Attribute("{3E562E27A2B86F47}Prefabs/Structures/CRF_Bomb.et", "auto", "The object to spawn as a bomb")]
	string bombSitePrefab;
	
	protected bool aSiteDestroyed, bSiteDestroyed = false;
	protected int sitesDestroyed = 0;
	protected IEntity aSite, bSite;
	EntityID aSiteID, bSiteID = null;
	
	protected SCR_PopUpNotification m_PopUpNotification = null;
	protected IEntity aSiteTrigger, bSiteTrigger;
	protected int aSiteTimer = 120;
	protected int bSiteTimer = 120;
	protected vector aSiteSpawn;
	protected vector bSiteSpawn;
	protected vector aSiteYawPitchRoll;
	protected vector bSiteYawPitchRoll;
	
	[RplProp(onRplName: "ShowMessage")]
	string m_sMessageContent;
	protected string m_sStoredMessageContent;
	
	[RplProp(onRplName: "PlaySound")]
	string m_SoundString;
	
	[RplProp(onRplName: "SiteDestroyedClient")]
	string m_sDestroyedBombSiteString;
	
	[RplProp()]
	bool aSitePlanted = false;
	
	[RplProp()]
	bool bSitePlanted = false;
	
	[RplProp()]
	bool countDownActive = false;
	
	//------------------------------------------------------------------------------------------------
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
			return;
	
		InitBombSites();
	}
	
	//------------------------------------------------------------------------------------------------
	void InitBombSites()
	{
		aSiteTrigger = GetGame().GetWorld().FindEntityByName("aSiteTrigger");
		bSiteTrigger = GetGame().GetWorld().FindEntityByName("bSiteTrigger");
		aSiteSpawn = aSiteTrigger.GetOrigin();
		aSiteYawPitchRoll = aSiteTrigger.GetYawPitchRoll();
		bSiteSpawn = bSiteTrigger.GetOrigin();
		bSiteYawPitchRoll = bSiteTrigger.GetYawPitchRoll();
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = aSiteSpawn;
		
		// Spawn destructible "site" at each trigger point
		aSite = GetGame().SpawnEntityPrefab(Resource.Load(bombSitePrefab),GetGame().GetWorld(),spawnParams);
		aSite.SetYawPitchRoll(aSiteYawPitchRoll);
		aSiteID = aSite.GetID();
		
		spawnParams.Transform[3] = bSiteSpawn; // change to bsite
		bSite = GetGame().SpawnEntityPrefab(Resource.Load(bombSitePrefab),GetGame().GetWorld(),spawnParams);
		bSite.SetYawPitchRoll(bSiteYawPitchRoll);
		bSiteID = bSite.GetID();
		
		GetGame().GetCallqueue().CallLater(CheckAddMarkers, 1, true);
	}
	
	
	void CheckAddMarkers()
	{
		// Create markers on each bomb site
		CRF_GameModePlayerComponent gameModePlayerComponent = CRF_GameModePlayerComponent.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		gameModePlayerComponent.AddScriptedMarker("aSiteTrigger", "0 0 0", 1, "Bomb Site A", "{21A2A457BD0E42C1}UI\Objectives\A.edds", 50, ARGB(255, 225, 225, 225));
		gameModePlayerComponent.AddScriptedMarker("bSiteTrigger", "0 0 0", 1, "Bomb Site B", "{7F4A8D140283CCCE}UI\Objectives\B.edds", 50, ARGB(255, 225, 225, 225));
		
		GetGame().GetCallqueue().Remove(CheckAddMarkers);
	}
	

	// Acts as a loop method spawned via calllater, every 1 sec
	//------------------------------------------------------------------------------------------------
	void StartCountdown()
	{
		// Check if defused
		if (!countDownActive) {
			aSitePlanted = false;
			bSitePlanted = false;
			return;
		}
		
		// Show message
		if (aSitePlanted) {
			aSiteTimer--;
			m_sMessageContent = SCR_FormatHelper.FormatTime(aSiteTimer);
		} else {
			bSiteTimer--;
			m_sMessageContent = SCR_FormatHelper.FormatTime(bSiteTimer);
		};
		
		// Bomb goes off
		if (aSiteTimer <= 0 || bSiteTimer <= 0) 
		{
			if (aSitePlanted) {
				m_sMessageContent = "SITE A DESTROYED!|15|";
				aSiteTimer = 120;
			} else {
				m_sMessageContent = "SITE B DESTROYED!|15|";
				bSiteTimer = 120;
			};
			
			// Remove timer
			GetGame().GetCallqueue().Remove(StartCountdown);
			countDownActive = false;
			
			// Destroy site
			SiteDestroyed();
			
			aSitePlanted = false;
			bSitePlanted = false;
		}
		
		Replication.BumpMe();
		ShowMessage();
	}
	
	//------------------------------------------------------------------------------------------------
	void SiteDestroyed() 
	{
		sitesDestroyed++;
		
		IEntity bombSitePlanted = null;
		
		if (aSitePlanted) {
			m_sDestroyedBombSiteString = "SiteA";
			bombSitePlanted = aSite;
		} else {
			m_sDestroyedBombSiteString = "SiteB";
			bombSitePlanted = bSite;
		};
		
		SiteDestroyedClient();
		
		if (sitesDestroyed == 2)
		{
			m_sMessageContent = "Attackers Have Destroyed Both Sites!|60|Attacker Victory!";
			m_SoundString = "{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav";
		}
		
		Replication.BumpMe();
		ShowMessage();
	}
	
	//------------------------------------------------------------------------------------------------
	void ToggleBombPlanted(string sitePlanted, bool togglePlanted) 
	{
		m_SoundString = "{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav";
		if (!togglePlanted) 
		{
			GetGame().GetCallqueue().Remove(StartCountdown);
			countDownActive = false;
			if (aSitePlanted) {
				aSitePlanted = false;
				m_sMessageContent = "Defenders Have Defused a Bomb at Site A!|15|";
			} else {
				bSitePlanted = false;
				m_sMessageContent = "Defenders Have Defused a Bomb at Site B!|15|";
			};
		} else {
			// Set which site is planted
			countDownActive = true;
			if (!aSite.IsDeleted() && sitePlanted == "SiteA") {
				aSitePlanted = true;
				m_sMessageContent = "Attackers Have Placed a Bomb at Site A!|15|";
			} else {
				bSitePlanted = true;
				m_sMessageContent = "Attackers Have Placed a Bomb at Site B!|15|";
			};
		
			// Spawn countdown thread
			GetGame().GetCallqueue().CallLater(StartCountdown, 1000, true);
		};
		
		Replication.BumpMe();
		ShowMessage();
	}
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	// Locality needs verified for workbench and local server hosting
	void SiteDestroyedClient() 
	{
		IEntity destroyedBombSiteEntity = null;
		
		CRF_GameModePlayerComponent gameModePlayerComponent = CRF_GameModePlayerComponent.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		if(m_sDestroyedBombSiteString == "SiteA") {
			gameModePlayerComponent.RemoveScriptedMarker("aSiteTrigger", "0 0 0", 1, "Bomb Site A", "{21A2A457BD0E42C1}UI\Objectives\A.edds", 50, ARGB(255, 225, 225, 225));
			destroyedBombSiteEntity = aSite;
		} else {
			gameModePlayerComponent.RemoveScriptedMarker("bSiteTrigger", "0 0 0", 1, "Bomb Site B", "{7F4A8D140283CCCE}UI\Objectives\B.edds", 50, ARGB(255, 225, 225, 225));
			destroyedBombSiteEntity = bSite;
		};
		
		// Spawn explosion at site
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = destroyedBombSiteEntity.GetOrigin();
	
		GetGame().SpawnEntityPrefab(Resource.Load("{A79737B19CBFD5EC}Prefabs/Systems/Explosions/Wrapper_Bomb_Large.et"),GetGame().GetWorld(),spawnParams);
		// Delete entity
		delete destroyedBombSiteEntity;
	}
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	// Locality needs verified for workbench and local server hosting
	void ShowMessage()
	{
		if (m_sMessageContent == m_sStoredMessageContent)
			return;
		
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		//m_PopUpNotification = SCR_PopUpNotification.GetInstance();
		
		m_sStoredMessageContent = m_sMessageContent;
		
		array<string> messageSplitArray = {};
		string stasisStr = m_sMessageContent;
		stasisStr.Split("|", messageSplitArray, false);
		
		if(messageSplitArray.Count() == 1)
			return;

		string mainMessage = messageSplitArray[0];
		string time = messageSplitArray[1];
		string subMessage = messageSplitArray[2];
		
		m_PopUpNotification.PopupMsg(mainMessage, time.ToFloat(), subMessage);
	};
	
	// Called from server to all clients
	//------------------------------------------------------------------------------------------------
	// Locality needs verified for workbench and local server hosting
	void PlaySound()
	{
		AudioSystem.PlaySound(m_SoundString);
	};
}
