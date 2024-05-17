[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RushGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RushGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side assaulting the bomb sites")]
	FactionKey attackingSide;
	
	[Attribute("USSR", "auto", "The side deffending the bomb sites")]
	FactionKey defendingSide;
	
	[Attribute("{1260D6425C37BDF2}Prefabs/Props/Military/Generators/GeneratorMilitary_USSR_01/CRF_Rush_Site.et", "auto", "The object to spawn as a Rush site")]
	string bombSitePrefab;
	
	[RplProp(onRplName: "ShowMessage")]
	string m_sMessageContent;
	
	[RplProp(onRplName: "PlaySound")]
	string m_SoundString;
	
	[RplProp(onRplName: "SiteDestroyedClient")]
	string m_sDestroyedBombSiteString;
	
	// Rush physical sites 
	IEntity w1s1, w1s2, w2s1, w2s2, w3s1, w3s2;
	
	// Game logic 
	bool site1Planted = false;
	bool site2Planted = false;
	bool countDownActive = false;
	bool wave1Clear = false;
	bool wave2Clear = false;
	int sitesDestroyed = 0;
	
	// CONSTANTS 
	int BOMBSITETIMER = 60;
	int site1Timer = BOMBSITETIMER;
	int site2Timer = BOMBSITETIMER;
	
	override protected void OnWorldPostProcess(World world)
	{
		if (!GetGame().InPlayMode()) 
			return;
	
		InitRushSites();
		PrintFormat("[CRF] w1s1: %1",w1s1.GetName());
	}
	
	void InitRushSites()
	{
		w1s1 = GetGame().FindEntity("CRF_Rush_Site_w1s1");
		w1s2 = GetGame().FindEntity("CRF_Rush_Site_w1s2");
		w2s1 = GetGame().FindEntity("CRF_Rush_Site_w2s1");
		w2s2 = GetGame().FindEntity("CRF_Rush_Site_w2s2");
		w3s1 = GetGame().FindEntity("CRF_Rush_Site_w3s1");
		w3s2 = GetGame().FindEntity("CRF_Rush_Site_w3s2");
	}
	
	// Checks to ensure sites can be planted at
	bool VerifySites(string bombSite)
	{
		if (!wave1Clear)
		{
			if (bombSite == w2s1.GetName() || bombSite == w2s2.GetName() || bombSite == w3s1.GetName() || bombSite == w3s2.GetName())
				return false;
		} else if (wave1Clear && !wave2Clear) {
			if (bombSite == w3s1.GetName() || bombSite == w3s2.GetName())
				return false;
		}
		
		return true;
	}
	
	// TODO 
	void ToggleBombPlanted(string bombSite, bool togglePlanted) 
	{
		m_SoundString = "{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav";
		if (togglePlanted && VerifySites(bombSite)) // bomb planted
		{
			if (bombSite == w1s1.GetName() || bombSite == w2s1.GetName() || bombSite == w3s1.GetName()) {
				site1Planted = true;
				m_sMessageContent = "Attackers have placed a bomb at site 1!";
			} else {
				site2Planted = true;
				m_sMessageContent = "Attackers have placed a bomb at site 2!";
			};
		
			// Spawn countdown thread
			GetGame().GetCallqueue().CallLater(StartCountdown, 1000, true, bombSite);
		} else { // bomb defused
			// Remove countdown
			GetGame().GetCallqueue().Remove(StartCountdown);
			
			if (bombSite == w1s1.GetName() || bombSite == w2s1.GetName() || bombSite == w3s1.GetName()) {
				site1Planted = false;
				m_sMessageContent = "Defenders have defused the bomb site 1!";
			} else {
				site2Planted = false;
				m_sMessageContent = "Defenders have defused the bomb site 2!";
			};
		};
		
		Replication.BumpMe();
	}
	
	// TODO LOOP, every 1 sec
	void StartCountdown(string bombSite)
	{
		// Check if defused
		if (!countDownActive) {
			site1Planted = false;
			site2Planted = false;
			return;
		}
		
		// Show message
		if (site1Planted) {
			site1Timer--;
			m_sMessageContent = SCR_FormatHelper.FormatTime(site1Timer);
		} else {
			site2Timer--;
			m_sMessageContent = SCR_FormatHelper.FormatTime(site2Timer);
		};
		
		// Bomb goes off
		if (site1Timer <= 0 || site2Timer <= 0) 
		{
			if (site1Planted) {
				m_sMessageContent = "SITE 1 DESTROYED!";
				site1Timer = 60;
			} else {
				m_sMessageContent = "SITE 2 DESTROYED!";
				site2Timer = 60;
			};
			
			// Remove timer
			GetGame().GetCallqueue().Remove(StartCountdown);
			countDownActive = false;
			
			// Destroy site
			SiteDestroyed(bombSite);
			
			site1Planted = false;
			site2Planted = false;
		}
		
		Replication.BumpMe();
	}
	
	void SiteDestroyed(string bombSite) 
	{	
		sitesDestroyed++;
		IEntity bombSiteToBlowwwwwww;
		
		// Determine bomb site since we cant pass entities in rpc 
		switch (bombSite) 
		{
			case "CRF_Rush_Site_w1s1":
				bombSiteToBlowwwwwww = w1s1;
				break;
			case "CRF_Rush_Site_w1s2":	
				bombSiteToBlowwwwwww = w1s2;
				break;
			case "CRF_Rush_Site_w2s1":
				bombSiteToBlowwwwwww = w2s1;
				break;
			case "CRF_Rush_Site_w2s2":	
				bombSiteToBlowwwwwww = w2s2;
				break;
			case "CRF_Rush_Site_w3s1":
				bombSiteToBlowwwwwww = w3s1;
				break;
			case "CRF_Rush_Site_w3s1":
				bombSiteToBlowwwwwww = w3s2;
				break;
			default: {}
		}
		
		// TODO: Fix || Spawn explosion at site
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = bombSiteToBlowwwwwww.GetOrigin();
		
		// Delete entity
		delete bombSiteToBlowwwwwww;
	
		GetGame().SpawnEntityPrefab(Resource.Load("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et"),GetGame().GetWorld(),spawnParams);
		
		if (sitesDestroyed == 6)
		{
			m_sMessageContent = "Attackers have destroyed all Rush sites! Attacker victory!";
			m_SoundString = "{349D4D7CC242131D}Sounds/Music/Ingame/Samples/Jingles/MU_EndCard_Drums.wav";
		}
		
		Replication.BumpMe();
	}
	
	void ShowMessage()
	{
		array<string> messageSplitArray = {};
		string stasisStr = m_sMessageContent;
		stasisStr.Split("╣", messageSplitArray, false);

		string mainMessage = messageSplitArray[0];
		string time = messageSplitArray[1];
		string subMessage = messageSplitArray[2];
		
		if(!mainMessage.IsEmpty() && !time.IsEmpty())
			SCR_PopUpNotification.GetInstance().PopupMsg(mainMessage, time.ToFloat(), subMessage);
	}
	
	void PlaySound()
	{
		AudioSystem.PlaySound(m_SoundString);
	}

	// TODO
	void SiteDestroyedClient() 
	{
		/*IEntity destroyedBombSiteEntity = null;
		
		if(m_sDestroyedBombSiteString == "SiteA")
			destroyedBombSiteEntity = aSite;
		else
			destroyedBombSiteEntity = bSite;
		
		// Spawn explosion at site
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = destroyedBombSiteEntity.GetOrigin();
	
		GetGame().SpawnEntityPrefab(Resource.Load("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et"),GetGame().GetWorld(),spawnParams);
		// Delete entity
		delete destroyedBombSiteEntity;*/
	}
}

