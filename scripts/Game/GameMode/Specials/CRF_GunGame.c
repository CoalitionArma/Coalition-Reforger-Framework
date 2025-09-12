class CRF_GunGameClass: SCR_BaseGameModeComponentClass
{

}

[BaseContainerProps()]
class CRF_GunGameContainer
{
	[Attribute()] ResourceName m_sWeapon;
	[Attribute()] ResourceName m_sMagazines;
	[Attribute()] int m_iAmountOfMagazines;
	[Attribute()] int m_iAmountOfKillsToLevelUp;
}

class GunGameMedalContainer
{
	ResourceName m_sMedalImage;
	string m_sMedalText;
}

class CRF_GungameStart: ChimeraMenuBase
{
}

class CRF_GunGameEnd: ChimeraMenuBase
{
	override void OnMenuOpen()
	{
		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		GetRootWidget().SetOpacity(0);
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		// End Game Animations
		if (GetRootWidget().GetOpacity() < 1)
			GetRootWidget().SetOpacity(GetRootWidget().GetOpacity() + tDelta);
		
		if (GetRootWidget().GetOpacity() > 1)
			GetRootWidget().SetOpacity(1);
			
	}
	
	void Action_Exit()
	{
		// Note: Opening pause menu instead of directly exiting the game
		// because players often accidentally exit the game
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}
	
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
}

class CRF_GunGame: SCR_BaseGameModeComponent
{
	[Attribute()] ref array<ref CRF_GunGameContainer> m_aGunLevels;
	
	//Current Level Player is at
	[RplProp()] ref array<int> m_aLevels = {};
	
	//How many kills at this level they have
	[RplProp()] ref array<int> m_aKillsThisLevel = {};
	
	//How many total kills they have
	[RplProp()] ref array<int> m_aKills = {};
	
	//Used to reference the index of the above arrays
	[RplProp()] ref array<int> m_aPlayers = {};
	
	//Used to relay to other clients the game is over
	[RplProp()] bool m_bIsGameOver = false;
	
	// Internal flag to prevent redundant replication updates
	protected bool m_bSuppressReplication = false;
	
	//Stores spawnpoints so if two players spawn at once they don't spawn on the same point
	ref array<int> m_aSpawnPointBuffer = {};
	
	//Just stores this for reference
	MenuBase m_GameOverMenu;
	
	
	//Medals
	//Current kill streak
	int m_iKillStreak = 0;
	
	//Last time you got a medal for your kill streak
	int m_iLastKillStreak = 0;
	
	//Buffer so we can have our medal animations and queue these up
	ref array<ref GunGameMedalContainer> m_aMedals = {};
	
	//Used to signify if we need to display the next medal
	bool m_bIsMedalDisplaying = false;
	
	//How many people you've killed in a 5 second span since the first kill in this buffer
	int m_iKillsBuffer = 0;
	
	//Did you go prone within the last three seconds
	bool m_bIsDropshot = false;
	
	//How many times you've died since your last kill
	int m_iComebackCounter = 0;
	
	//Just so if someone gets humiliated at 10 points and goes back down to 0 you don't think you're the first kill again
	bool m_bFirstKill = true;
	
	//Who killed you last
	int m_iRevengePlayer = -1;
	
	//Time since your last weapon, huh good naming
	float m_fTimeSinceLastWeapon = 0;
	
	// What was your old weapon, used to pull out the new one
	IEntity m_eOldWeapon = null;
	
	//How many kills do you need to win
	int m_iKillsToWin = 0;
	
	//The level your client is tracking
	int m_iLocalLevel = 0;
	
	//Trackers for HUD components
	Widget m_wKillIcon;
	Widget m_wHUD;
	Widget m_wHitmarker;
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME | EntityEvent.INIT);
		foreach (CRF_GunGameContainer level: m_aGunLevels)
		{
			m_iKillsToWin += level.m_iAmountOfKillsToLevelUp;
		}
	}
	
	//Disables going uncon for that true COD feel
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		SCR_GameModeHealthSettings.Cast(GetGame().GetGameMode().FindComponent(SCR_GameModeHealthSettings)).SetUnconsciousnessPermitted(false);
	}
	
	//Finds a spawnpoint with no players around it, or if none available picks a random one.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	vector GetSpawnPoint()
	{
		int amountOfSpawns = 0;
		for (int i = 0; i < 128; i++)
		{
			IEntity spawnPoint = GetGame().GetWorld().FindEntityByName("spawnpoint" + i.ToString());
			if (!spawnPoint)
				break;
			
			amountOfSpawns++;
			
			//Sees if theres a player within 25m
			if (!GetGame().GetWorld().QueryEntitiesBySphere(spawnPoint.GetOrigin(), 25, SpawnPointCallBack, null))
				continue;
			
			if (m_aSpawnPointBuffer.Contains(i))
				continue;
			
			m_aSpawnPointBuffer.Insert(i);
			GetGame().GetCallqueue().CallLater(RemoveSpawnFromBuffer, 500, false, i);
			
			return spawnPoint.GetOrigin();
		}
		
		RandomGenerator random = new RandomGenerator();
		int randomSpawn = random.RandInt(0, amountOfSpawns);
		IEntity randomSpawnpoint = GetGame().GetWorld().FindEntityByName("spawnpoint" + randomSpawn);
		return randomSpawnpoint.GetOrigin();
	}
	
	//Just used to allow players to spawn at that spawn point again
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RemoveSpawnFromBuffer(int spawn)
	{
		m_aSpawnPointBuffer.RemoveItem(spawn);
	}
	
	//CB for query by sphere
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	bool SpawnPointCallBack(IEntity entity)
	{
		if (ChimeraCharacter.Cast(entity))
		{
			//Is this character dead
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (damageManager)
			{
				if (damageManager.GetState() == EDamageState.DESTROYED)
					return true;
				else
					return false;
			}
		}
			
		return true;
	}
	
	//Process all data on what to do when a player is killed on client and server.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		ProcessKillClient(instigatorContextData);
		super.OnControllableDestroyed(instigatorContextData);
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif
		
		int killerId = instigatorContextData.GetInstigator().GetInstigatorPlayerID();
		#ifdef WORKBENCH
		#else
		if (killerId == -1)
			return;
		#endif
		
		int index = m_aPlayers.Find(killerId);
		if (index == -1)
			return;
		bool suicide = instigatorContextData.GetKillerPlayerID() == instigatorContextData.GetVictimPlayerID();
		
		if (instigatorContextData.GetKillerEntity())
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(instigatorContextData.GetKillerEntity());
			SCR_MeleeComponent meleeComp = SCR_MeleeComponent.Cast(character.FindComponent(SCR_MeleeComponent));
			
			if (meleeComp.GetMeleeStarted())
				DemotePlayer(instigatorContextData.GetVictimPlayerID());
		}
		
		if (suicide)
		{
			DemotePlayer(instigatorContextData.GetVictimPlayerID());
		}
		else
		{
			int currentKills = m_aKills.Get(index);
			currentKills++;
			
			int currentKillsThisLevel = m_aKillsThisLevel.Get(index);
			currentKillsThisLevel++;
			
			currentKillsThisLevel = Math.ClampInt(currentKillsThisLevel, 0, 100);
			currentKills = Math.ClampInt(currentKills, 0, 100);
			
			// Check if game is won before updating stats
			bool gameWon = (currentKills == m_iKillsToWin);
			
			// Use batch update to set all changes in one replication call
			if (gameWon)
			{
				BatchUpdatePlayerStats(index, currentKills, currentKillsThisLevel, -1, true);
				GameOver();
				return;
			}
			else
			{
				BatchUpdatePlayerStats(index, currentKills, currentKillsThisLevel);
				NewWeaponCheck(killerId);
			}
		}
		
		vector dubugSpawnPointVector[4];
		GetGame().GetWorld().FindEntityByName("debugSpawnpoint").GetWorldTransform(dubugSpawnPointVector);
		
		GetGame().GetCallqueue().CallLater(RespawnPlayerDelayed, 5000, false, instigatorContextData.GetVictimPlayerID(), dubugSpawnPointVector[0], dubugSpawnPointVector[1], dubugSpawnPointVector[2], dubugSpawnPointVector[3]);
	}
	
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	/**
	* Can't use static vectors in callLater, so we just use this container method to act as a holder for the call later  
	* @param playerId ID of the player to initialize
	* @param locationZero Position 0 in the world vector to spawn the player
	* @param locationOne Position 1 in the world vector to spawn the player
	* @param locationTwo Position 2 in the world vector to spawn the player
	* @param locationThree Position 3 in the world vector to spawn the player
	*/
	void RespawnPlayerDelayed(int playerId, vector locationZero, vector locationOne, vector locationTwo, vector locationThree)
	{
		vector location[4];
		
		location[0] = locationZero;
		location[1] = locationOne;
		location[2] = locationTwo;
		location[3] = locationThree;
		
		RespawnPlayer(playerId, location);
	}
	
	
	//Called by server to tell clients to draw the gameover screen.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void GameOver()
	{
		#ifdef WORKBENCH
		RpcDo_BroadcastGameOver();
		#else
		Rpc(RpcDo_BroadcastGameOver);
		#endif
	}
	
	//Yep does what it says it does
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_BroadcastGameOver()
	{
		GetGame().GetMenuManager().CloseAllMenus();
		m_GameOverMenu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_GunGameEnd);
		Widget menuWidget = m_GameOverMenu.GetRootWidget();
		ref array<int> winners = GetWinners();
		
		if (winners.Get(0) != -1)
			TextWidget.Cast(menuWidget.FindWidget("First")).SetText("1st. " + GetGame().GetPlayerManager().GetPlayerName(winners.Get(0)));
		else
			menuWidget.FindWidget("First").SetVisible(false);
		
		if (winners.Get(1) != -1)
			TextWidget.Cast(menuWidget.FindWidget("Second")).SetText("2nd. " + GetGame().GetPlayerManager().GetPlayerName(winners.Get(1)));
		else
			menuWidget.FindWidget("Second").SetVisible(false);
		
		if (winners.Get(2) != -1)
			TextWidget.Cast(menuWidget.FindWidget("Third")).SetText("3rd. " + GetGame().GetPlayerManager().GetPlayerName(winners.Get(2)));
		else
			menuWidget.FindWidget("Third").SetVisible(false);
		
		if (winners.Contains(SCR_PlayerController.GetLocalPlayerId()))
			menuWidget.FindWidget("Victory").SetVisible(true);
		else
			menuWidget.FindWidget("Defeat").SetVisible(true);
	}
	
	//Gets Top 3 players.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	array<int> GetWinners()
	{
	    array<int> winners = {};
	    array<int> sortedKills = {};
	    array<int> sortedPlayers = {};
	    
	    sortedKills.Copy(m_aKills);
	    sortedPlayers.Copy(m_aPlayers);
	    
	    // Simple selection sort for top 3
	    for (int i = 0; i < 3 && i < sortedKills.Count(); i++)
	    {
	        int maxIndex = i;
	        for (int j = i + 1; j < sortedKills.Count(); j++)
	        {
	            if (sortedKills[j] > sortedKills[maxIndex])
	                maxIndex = j;
	        }
	
	        // Swap kills
	        int tempKill = sortedKills[i];
	        sortedKills[i] = sortedKills[maxIndex];
	        sortedKills[maxIndex] = tempKill;
	
	        // Swap players to keep indexing consistent
	        int tempPlayer = sortedPlayers[i];
	        sortedPlayers[i] = sortedPlayers[maxIndex];
	        sortedPlayers[maxIndex] = tempPlayer;
	
	        winners.Insert(sortedPlayers[i]);
	    }
		
		while (winners.Count() < 3)
	    {
	        winners.Insert(-1);
	    }
	
		Print(winners);
	    return winners;
	}
	
	//Called when a player is killed by melee or suicide to drop them down to the previous level.	
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void DemotePlayer(int playerId)
	{
		int index = m_aPlayers.Find(playerId);
		int levels = m_aLevels.Get(index);
		int kills = m_aKills.Get(index);
		int currentKillsThisLevel = m_aKillsThisLevel.Get(index);
		levels--;
		levels = Math.ClampInt(levels, 0, 100);
		CRF_GunGameContainer lastLevel = m_aGunLevels.Get(levels);
		kills -= currentKillsThisLevel + lastLevel.m_iAmountOfKillsToLevelUp;
		kills = Math.ClampInt(kills, 0, 100);
		
		// Use batch update to set all demotion changes in one replication call
		BatchUpdatePlayerStats(index, kills, 0, levels);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Silent setters for batch operations - update arrays without triggering replication
	*/
	protected void SetPlayerKillsSilent(int playerIndex, int kills)
	{
		if (playerIndex >= 0 && playerIndex < m_aKills.Count())
			m_aKills.Set(playerIndex, Math.ClampInt(kills, 0, 100));
	}
	
	protected void SetPlayerKillsThisLevelSilent(int playerIndex, int kills)
	{
		if (playerIndex >= 0 && playerIndex < m_aKillsThisLevel.Count())
			m_aKillsThisLevel.Set(playerIndex, Math.ClampInt(kills, 0, 100));
	}
	
	protected void SetPlayerLevelSilent(int playerIndex, int level)
	{
		if (playerIndex >= 0 && playerIndex < m_aLevels.Count())
			m_aLevels.Set(playerIndex, Math.ClampInt(level, 0, 100));
	}
	
	protected void SetGameOverSilent(bool gameOver)
	{
		m_bIsGameOver = gameOver;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Batch update multiple player statistics in a single replication call
	* @param playerIndex Index of the player in the arrays
	* @param kills Optional new total kills value
	* @param killsThisLevel Optional new kills this level value
	* @param level Optional new level value
	* @param gameOver Optional game over state
	*/
	void BatchUpdatePlayerStats(int playerIndex, int kills = -1, int killsThisLevel = -1, int level = -1, bool gameOver = false)
	{
		bool anyChanged = false;
		m_bSuppressReplication = true;
		
		if (kills >= 0)
		{
			SetPlayerKillsSilent(playerIndex, kills);
			anyChanged = true;
		}
		
		if (killsThisLevel >= 0)
		{
			SetPlayerKillsThisLevelSilent(playerIndex, killsThisLevel);
			anyChanged = true;
		}
		
		if (level >= 0)
		{
			SetPlayerLevelSilent(playerIndex, level);
			anyChanged = true;
		}
		
		if (gameOver != m_bIsGameOver)
		{
			SetGameOverSilent(gameOver);
			anyChanged = true;
		}
		
		m_bSuppressReplication = false;
		if (anyChanged)
			Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	* Batch initialize multiple new players to minimize replication calls
	* @param playerIds Array of player IDs to initialize
	*/
	void BatchInitializePlayers(array<int> playerIds)
	{
		bool anyAdded = false;
		m_bSuppressReplication = true;
		
		foreach (int playerId : playerIds)
		{
			if (m_aPlayers.Contains(playerId))
				continue;
				
			m_aPlayers.Insert(playerId);
			m_aLevels.Insert(0);
			m_aKills.Insert(0);
			m_aKillsThisLevel.Insert(0);
			anyAdded = true;
		}
		
		m_bSuppressReplication = false;
		if (anyAdded)
			Replication.BumpMe();
	}
	
	//Adds medal to the queue
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AddMedal(string medalImage, string medalText)
	{
		GunGameMedalContainer medalContainer = new GunGameMedalContainer();
		medalContainer.m_sMedalImage = medalImage;
		medalContainer.m_sMedalText = medalText;
		m_aMedals.Insert(medalContainer);
	}
	
	//Ran only on the client, process kills on the client
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	float m_fKillIconTimer = 0;
	void ProcessKillClient(notnull SCR_InstigatorContextData instigatorContextData)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif
		
		// We died
		if (SCR_PlayerController.GetLocalPlayerId() == instigatorContextData.GetVictimPlayerID())
		{
			m_iLastKillStreak = 0;
			m_iKillStreak = 0;
			m_iComebackCounter++;
			m_iRevengePlayer = instigatorContextData.GetKillerPlayerID();
			return;
		}
		
		// Used because for some fuck ass reason on dedicated servers after a death you get spammed with useless OnControllableDestroyed with no victims.
		#ifdef WORKBENCH
		#else
		if (instigatorContextData.GetVictimPlayerID() == 0)
			return;
		#endif
			
		//Not our kill
		if (SCR_PlayerController.GetLocalPlayerId() != instigatorContextData.GetKillerPlayerID())
			return;
		
		//Used because if you're scoped in with a sniper and kill someone the PIP may stay on your screen, COMPLETELY BROKEN on workbench, works fine in Dedi.
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity()).FindComponent(BaseWeaponManagerComponent));
		IEntity currentWeapon;
		IEntity currentSight;
		if (weaponMan.GetCurrentWeapon())
		{
			currentWeapon = weaponMan.GetCurrentWeapon().GetOwner();
			if(weaponMan.GetCurrentWeapon().GetSights())
				currentSight = weaponMan.GetCurrentWeapon().GetSights().GetOwner();
		}
		
		if (currentWeapon)
		{
			if (currentSight)
			{
				if (currentSight.FindComponent(SCR_2DPIPSightsComponent))
				{
					SCR_2DPIPSightsComponent pipComp = SCR_2DPIPSightsComponent.Cast(currentSight.FindComponent(SCR_2DPIPSightsComponent));
					pipComp.SetPIPEnabled(false);
				}
			}
		}
		
		if (m_iKillsBuffer == 0)
		{
			m_iKillsBuffer++;
			GetGame().GetCallqueue().CallLater(KillBuffer, 3000, false);
		}
		else
			m_iKillsBuffer++;
		
		m_iKillStreak++;

		if (m_bIsDropshot)
			AddMedal("{CA7E93826F34955D}UI/layouts/HUD/GunGame/Medals/Dropshot_Medal_BOII.edds", "DROPSHOT");
		
		if (SCR_DamageManagerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_DamageManagerComponent)).GetHealthScaled() < 0.5)
			AddMedal("{E6F99A749F738983}UI/layouts/HUD/GunGame/Medals/Survivor_Medal_BOII.edds", "SURVIVOR");
		
		foreach (int kills: m_aKills)
		{
			if (kills != 0)
				m_bFirstKill = false;
		}
		
		if (m_bFirstKill)
			AddMedal("{D2DCE578823A673C}UI/layouts/HUD/GunGame/Medals/FirstBlood_Medal_BOII.edds", "FIRST BLOOD");
		
		if (m_iComebackCounter >= 3)
			AddMedal("{4542DF8DC79326E8}UI/layouts/HUD/GunGame/Medals/Comeback_Medal_BOII.edds", "COMEBACK");
		
		m_iComebackCounter = 0;
		
		if (vector.Distance(SCR_PlayerController.GetLocalControlledEntity().GetOrigin(), instigatorContextData.GetVictimEntity().GetOrigin()) > 100)
			AddMedal("{33B404E44435E0D2}UI/layouts/HUD/GunGame/Medals/Long_Shot_Medal_BOII.edds", "LONGSHOT");
		
		if (m_iRevengePlayer >= 0)
			if (m_iRevengePlayer == instigatorContextData.GetVictimPlayerID())
			{
				m_iRevengePlayer = 0;
				AddMedal("{7E9CE5535464B327}UI/layouts/HUD/GunGame/Medals/Revenge_Medal_BOII.edds", "REVENGE");
			}			
		
		if (m_fTimeSinceLastWeapon < 3)
			AddMedal("{B49603B706100DD1}UI/layouts/HUD/GunGame/Medals/Gunslinger_Medal_BOII.edds", "GUNSLINGER");
		
		ChimeraCharacter character = ChimeraCharacter.Cast(instigatorContextData.GetKillerEntity());
		SCR_MeleeComponent meleeComp = SCR_MeleeComponent.Cast(character.FindComponent(SCR_MeleeComponent));
			
		//Process Melee kill, if its the 1st place player give Regicide medal.
		if (meleeComp.GetMeleeStarted())
		{
			ref array<int> winners = GetWinners();
			if (winners.Get(0) != -1)
			{
				if (winners.Get(0) == instigatorContextData.GetVictimPlayerID())
					AddMedal("{B260408040852BC0}UI/layouts/HUD/GunGame/Medals/Regicide_Medal_BOII.edds", "REGICIDE");
				else
					AddMedal("{9F05837619EA7FBF}UI/layouts/HUD/GunGame/Medals/Humiliation_Medal_BOII.edds", "HUMILIATION");
			}
			else
				AddMedal("{9F05837619EA7FBF}UI/layouts/HUD/GunGame/Medals/Humiliation_Medal_BOII.edds", "HUMILIATION");
		}
		
		if (m_wKillIcon)
		{
			m_fKillIconTimer = 0;
			delete m_wKillIcon;
		}
			
		m_wKillIcon = GetGame().GetWorkspace().CreateWidgets("{320C29C0FF5CFE13}UI/layouts/HUD/GunGame/KillPopup.layout");
		m_wKillIcon.SetOpacity(m_fKillIconTimer);
	}
	
	//Processes kills within 5 seconds of first kill medals.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void KillBuffer()
	{
		switch (m_iKillsBuffer)
		{
			case 2:
			{
				AddMedal("{519F97C70AAF9A10}UI/layouts/HUD/GunGame/Medals/Double_Kill_Medal_BOII.edds", "DOUBLE KILL");
				break;
			}
			case 3:
			{
				AddMedal("{1540DF20A3139634}UI/layouts/HUD/GunGame/Medals/Triple_Kill_Medal_BOII.edds", "TRIPLE KILL");
				break;
			}
			case 4:
			{
				AddMedal("{EA7AF4839D44C547}UI/layouts/HUD/GunGame/Medals/Fury_Kill_Medal_BOII.edds", "FURY KILL");
				break;
			}
			case 5:
			{
				AddMedal("{4035442B5BA76CCD}UI/layouts/HUD/GunGame/Medals/Frenzy_Kill_Medal_BOII.edds", "FRENZY KILL");
				break;
			}
			case 6:
			{
				AddMedal("{E44B8417629F2206}UI/layouts/HUD/GunGame/Medals/Super_Kill_Medal_BOII.edds", "SUPER KILL");
				break;
			}
			case 7:
			{
				AddMedal("{E2298E57903099B7}UI/layouts/HUD/GunGame/Medals/Mega_Kill_Medal_BOII.edds", "MEGA KILL");
				break;
			}
			case 8:
			{
				AddMedal("{6130F3009D109913}UI/layouts/HUD/GunGame/Medals/Ultra_Kill_Medal_BOII.edds", "ULTRA KILL");
				break;
			}
		}
		m_iKillsBuffer = 0;
	}
	
	//Need a delay for this, for entity processing reasons(magik).
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId, vector spawn[4])
	{
		CRF_RespawnManager.GetInstance().RespawnPlayer(playerId, spawn);
	}
	
	//Member variable to manage how long its been since you went prone.
	float m_fDropShotTimer = 0;
	//Client updates.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif
		
		//Client Updates
		CheckIfWeaponEquipped();
		UpdateKillIcon(timeSlice);
		UpdateHUD(timeSlice);
		UpdateKillStreak(timeSlice);
		UpdateCurrentWeapon(timeSlice);
		UpdateDropShot(timeSlice);
		
		//When going into spectator after death you may sometimes not get the game over screen
		if (m_bIsGameOver)
			if (!m_GameOverMenu)
				RpcDo_BroadcastGameOver();
		
		//I didn't want to make a whole update method for just this
		if (m_wHitmarker)
			m_wHitmarker.SetOpacity(m_wHitmarker.GetOpacity() - (timeSlice * 2));
	}
	
	//Updates time since you went prone.
	//Used for medals
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateDropShot(float timeSlice)
	{
		if (!SCR_PlayerController.GetLocalControlledEntity())
			return;
		if (!SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent))
			return;
		if (SCR_CharacterControllerComponent.Cast(SCR_PlayerController.GetLocalControlledEntity().FindComponent(SCR_CharacterControllerComponent)).GetStance() == ECharacterStance.PRONE)
		{
			m_fDropShotTimer += timeSlice;
		}
		else
			m_fDropShotTimer = 0;
		
		if (m_fDropShotTimer > 0 && m_fDropShotTimer < 3)
		{
			m_bIsDropshot = true;
		}
		else
			m_bIsDropshot = false;
	}
	
	//used for giving out GunSlinger medal
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateCurrentWeapon(float timeSlice)
	{
		m_fTimeSinceLastWeapon += timeSlice;
	}
	
	//Member variable used during the medal animations
	float m_fMedalTimer = 0;
	//Used to give out medals and killstreaks.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateKillStreak(float timeSlice)
	{
		//What killstreak to give out.
		switch (m_iKillStreak)
		{
			case 5: 
			{
				if (m_iLastKillStreak >= 5)
					break;
				
				m_iLastKillStreak = 5;
				AddMedal("{A7E3DD72B28AAC03}UI/layouts/HUD/GunGame/Medals/5_Streak_Medal_BOII.edds", "BLOODTHIRSTY");
				break;
			}
			case 10: 
			{
				if (m_iLastKillStreak >= 10)
					break;
				
				m_iLastKillStreak = 10;
				AddMedal("{3EF5BAF914A00485}UI/layouts/HUD/GunGame/Medals/10_Streak_Medal_BOII.edds", "MERCILESS");
				break;
			}
			case 15: 
			{
				if (m_iLastKillStreak >= 15)
					break;
				
				m_iLastKillStreak = 15;
				AddMedal("{611C42BCC4301F64}UI/layouts/HUD/GunGame/Medals/15_Streak_Medal_BOII.edds", "RUTHLESS");
				break;
			}
			case 20: 
			{
				if (m_iLastKillStreak >= 20)
					break;
				
				m_iLastKillStreak = 20;
				AddMedal("{C7D988E772230FF8}UI/layouts/HUD/GunGame/Medals/20_Streak_Medal_BOII.edds", "RELENTLESS");
				break;
			}
			case 25: 
			{
				if (m_iLastKillStreak >= 25)
					break;
				
				m_iLastKillStreak = 25;
				AddMedal("{983070A2A2B31419}UI/layouts/HUD/GunGame/Medals/25_Streak_Medal_BOII.edds", "BRUTAL");
				break;
			}
			case 30: 
			{
				if (m_iLastKillStreak >= 30)
					break;
				
				m_iLastKillStreak = 30;
				AddMedal("{90C266ED505DF6D3}UI/layouts/HUD/GunGame/Medals/30_Streak_Medal_BOII.edds", "UNSTOPPABLE");
				break;
			}
		}
		
		//No medals to give
		if (m_aMedals.Count() == 0)
			return;
		
		//No HUD to draw to
		if (!m_wHUD)
			return;
		
		ImageWidget medalImage = ImageWidget.Cast(m_wHUD.FindWidget("MedalImage"));
		TextWidget medalText = TextWidget.Cast(m_wHUD.FindWidget("MedalText"));
		//If no medal displaying lets display one.
		if (!m_bIsMedalDisplaying)
		{
			medalImage.LoadImageTexture(0, m_aMedals.Get(0).m_sMedalImage);
			medalImage.SetImage(0);
			medalImage.SetOpacity(0);
			medalText.SetText(m_aMedals.Get(0).m_sMedalText);
			medalText.SetOpacity(0);
			AudioSystem.PlaySound("{A3D993FCC6520D36}Sounds/GunGame/MedalRevealShine.wav");
		}
		
		m_bIsMedalDisplaying = true;
		
		//Just handles the medal animation and removing the current medal from the medal buffer
		m_fMedalTimer += timeSlice * 2;
		if (m_fMedalTimer < 1)
		{
			medalImage.SetOpacity(m_fMedalTimer);
			medalText.SetOpacity(m_fMedalTimer);
		}
		else if (m_fMedalTimer >= 1 && m_fMedalTimer <= 6)
		{
			medalImage.SetOpacity(6 - m_fMedalTimer);
			medalText.SetOpacity(6 - m_fMedalTimer);
		}
		else if (m_fMedalTimer > 6)
		{
			m_fMedalTimer = 0;
			m_bIsMedalDisplaying = false;
			m_aMedals.RemoveOrdered(0);
			medalImage.SetOpacity(0);
			medalText.SetOpacity(0);
		}
	}
	
	//Updates the lower left HUD with your score and the highest score thats not you.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateHUD(float timeSlice)
	{
		if (!GetGame().GetPlayerController())
			return;
		
		if (!SCR_PlayerController.GetLocalControlledEntity())
			return;
		
		if (SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" && m_wHUD)
		{
			delete m_wHUD;
			return;
		}
		
		if (SCR_PlayerController.GetLocalControlledEntity().GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			return;
		
		if (!m_wHUD)
		{
			m_wHUD = GetGame().GetWorkspace().CreateWidgets("{CB30D25E7BC3ADDB}UI/layouts/HUD/GunGame/GunGameHUD.layout");
		}
		
		int index = m_aPlayers.Find(SCR_PlayerController.GetLocalPlayerId());
		if (index == -1)
			return;
		
		TextWidget.Cast(m_wHUD.FindWidget("YourScore")).SetText((m_aKills.Get(index) * 10).ToString());
		TextWidget.Cast(m_wHUD.FindWidget("NextHighest")).SetText((FindNextHighestKills() * 10).ToString());
	}
	
	//Sorts through all the players kills and finds the highest one thats not you.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	int FindNextHighestKills()
	{
		int index = m_aPlayers.Find(SCR_PlayerController.GetLocalPlayerId());
		
		int highestKills = 0;
		foreach (int kills: m_aKills)
		{
			if (m_aKills.Find(kills) == index)
				continue;
			
			if (kills > highestKills)
				highestKills = kills;
		}
		
		return highestKills;
	}
	
	//Updates the +10 points in the center and if you leveled up gives you the gun promotion text as well.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateKillIcon(float timeSlice)
	{
		if (!m_wKillIcon)
			return;
		
		m_fKillIconTimer += timeSlice * 4;
		
		int index = m_aPlayers.Find(SCR_PlayerController.GetLocalPlayerId());
		if (m_iLocalLevel != m_aLevels.Get(index))
		{
			m_iLocalLevel = m_aLevels.Get(index);
			m_wKillIcon.FindWidget("Promotion").SetOpacity(1);
		}
		
		if (m_fKillIconTimer <= 1)
			m_wKillIcon.SetOpacity(m_fKillIconTimer);
		else
			m_wKillIcon.SetOpacity(8 - m_fKillIconTimer);
		
		if (m_fKillIconTimer >= 9)
		{
			m_fKillIconTimer = 0;
			delete m_wKillIcon;
		}
	}
	
	//Used to pull out weapon if its not the previous weapon we had
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void CheckIfWeaponEquipped()
	{
		IEntity entity = SCR_PlayerController.GetLocalControlledEntity();
		if (!entity)
			return;
		
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(entity).FindComponent(BaseWeaponManagerComponent));
		if (!weaponMan)
			return;
		
		ref array<WeaponSlotComponent> outSlots = {};
		weaponMan.GetWeaponsSlots(outSlots);
		
		IEntity currentWeapon;
		foreach (WeaponSlotComponent slot: outSlots)
		{
			if (!slot.GetWeaponEntity())
				continue;
			
			currentWeapon = slot.GetWeaponEntity();
		}
		
		CharacterControllerComponent charContComp = CharacterControllerComponent.Cast(entity.FindComponent(CharacterControllerComponent));
		if (m_eOldWeapon == currentWeapon)
			return;
		
		m_eOldWeapon = currentWeapon;
		m_fTimeSinceLastWeapon = 0;
		
		charContComp.TryEquipRightHandItem(currentWeapon, EEquipItemType.EEquipTypeWeapon, true);
	}
	
	//Server Only, teleports player to a spawnpoint, easiest method to keep this just one object.
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif
		
		GetGame().GetCallqueue().CallLater(SpawnCheck, 1000, false, entity);
	}
	
	//We have to broadcast this, cause???
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_TeleportPlayer(int playerId, vector spawn)
	{
		SCR_Global.TeleportPlayer(playerId, spawn, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
	}
	
	//Delay to allow the spawned entity proper time to initialize
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SpawnCheck(IEntity entity)
	{
		if (!entity)
			return;
		
		if (entity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			return;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		
		//Teleports player inside map and to a valid spawnpoint
		vector spawn = GetSpawnPoint();
		SCR_Global.TeleportPlayer(playerId, spawn, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
		Rpc(RpcDo_TeleportPlayer, playerId, spawn);
		
		int index = m_aPlayers.Find(playerId);
		if (index == -1)
			return;
		
		//Gives out the weapon for the current level the player is at.
		int level = m_aLevels.Get(index);
		ref CRF_GunGameContainer gunLevel = m_aGunLevels.Get(level);
		
		if (!ChimeraCharacter.Cast(entity))
			return;
		
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(entity).FindComponent(BaseWeaponManagerComponent));
		if (!weaponMan)
			return;
		
		ref array<WeaponSlotComponent> outSlots = {};
		weaponMan.GetWeaponsSlots(outSlots);
	
		SCR_InventoryStorageManagerComponent storageManagerComponent = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		storageManagerComponent.TrySpawnPrefabToStorage(gunLevel.m_sWeapon, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);
		
		for (int i = 1; i < gunLevel.m_iAmountOfMagazines; i++)
		{
			storageManagerComponent.TrySpawnPrefabToStorage(gunLevel.m_sMagazines, null, -1, EStoragePurpose.PURPOSE_ANY);
		}
	}
	
	//Checks to see if we need to hand out a new weapon
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void NewWeaponCheck(int playerId)
	{
		int index = m_aPlayers.Find(playerId);
		if (index == -1)
			return;
		
		int level = m_aLevels.Get(index);
		ref CRF_GunGameContainer gunLevel = m_aGunLevels.Get(level);
		
		//Just checks if somehow we got demoted with no death
		int currentKillsAtThisLevel = m_aKillsThisLevel.Get(index);
		if (currentKillsAtThisLevel == -1)
		{
			level--;
			level = Math.ClampInt(level, 0, 100);
			
			// Use batch update for demotion
			BatchUpdatePlayerStats(index, -1, 0, level);
			NewLevel(playerId);
			return;
		}
		
		//Don't have the kills to level up yet
		if (gunLevel.m_iAmountOfKillsToLevelUp >  currentKillsAtThisLevel)
			return;
		
		//Doing this to prevent complicated shit with previous levels mags
		SCR_InventoryStorageManagerComponent storageManagerComponent = SCR_InventoryStorageManagerComponent.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).FindComponent(SCR_InventoryStorageManagerComponent));
		ref array<IEntity> items = {};
		storageManagerComponent.GetItems(items);
		foreach (IEntity item: items)
		{
			if (item.GetPrefabData().GetPrefabName() == m_aGunLevels.Get(level).m_sMagazines)
				SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
		level++;
		
		// Use batch update for level up
		BatchUpdatePlayerStats(index, -1, 0, level);
		NewLevel(playerId);
	}
	
	//Used to hand out the new levels weapon
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void NewLevel(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(player).FindComponent(BaseWeaponManagerComponent));
		if (!weaponMan)
			return;
		
		IEntity currentWeapon;
		if (weaponMan.GetCurrentWeapon())
			currentWeapon = weaponMan.GetCurrentWeapon().GetOwner();
		
		if (currentWeapon)
			GetGame().GetCallqueue().CallLater(DeleteWeapon, 100, false, currentWeapon);
			
		
		int index = m_aPlayers.Find(playerId);
		if (index == -1)
			return;
		
		int level = m_aLevels.Get(index);
		if (level > m_aGunLevels.Count() - 1)
			return;
		CRF_GunGameContainer gunLevel = m_aGunLevels.Get(level);
		GetGame().GetCallqueue().CallLater(NewLevelAddWeapon, 300, false, player, gunLevel.m_sWeapon, gunLevel.m_sMagazines, gunLevel.m_iAmountOfMagazines);
	}
	
	//Need time to disable PIP if using a scope
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void DeleteWeapon(IEntity weapon)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(weapon);
	}
	
	//Need time for the previous weapon to be deleted
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void NewLevelAddWeapon(IEntity player, ResourceName weapon, ResourceName magazine, int amount)
	{
		SCR_InventoryStorageManagerComponent storageManagerComponent = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		storageManagerComponent.TrySpawnPrefabToStorage(weapon, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);
		
		for (int i = 1; i < amount; i++)
		{
			storageManagerComponent.TrySpawnPrefabToStorage(magazine, null, -1, EStoragePurpose.PURPOSE_ANY);
		}
	}
	
	//Initializes the player in the array
	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		//Hmm maybe it runs on clients, who knows
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif
		
		if (m_aPlayers.Contains(playerId))
			return;
		
		m_aPlayers.Insert(playerId);
		m_aLevels.Insert(0);
		m_aKills.Insert(0);
		m_aKillsThisLevel.Insert(0);
		if (!m_bSuppressReplication)
			Replication.BumpMe();
	}
}