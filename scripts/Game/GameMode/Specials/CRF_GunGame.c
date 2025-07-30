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
	[Attribute()] ref array<string> m_sSpawnNames;
	[RplProp()] ref array<int> m_aLevels = {};
	[RplProp()] ref array<int> m_aKillsThisLevel = {};
	[RplProp()] ref array<int> m_aKills = {};
	[RplProp()] ref array<int> m_aPlayers = {};
	[RplProp()] bool m_bIsGameOver = false;
	MenuBase m_GameOverMenu;
	
	//Medals
	int m_iKillStreak = 0;
	int m_iLastKillStreak = 0;
	ref array<ref GunGameMedalContainer> m_aMedals = {};
	bool m_bIsMedalDisplaying = false;
	
	int m_iKillsBuffer = 0;
	
	bool m_bIsDropshot = false;
	
	int m_iComebackCounter = 0;
	
	bool m_bFirstKill = true;
	
	int m_iRevengePlayer = -1;
	
	float m_fTimeSinceLastWeapon = 0;
	IEntity m_eOldWeapon = null;
	
	int m_iKillsToWin = 0;
	int m_iLocalLevel = 0;
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
	
	override void EOnInit(IEntity owner)
	{
		SCR_GameModeHealthSettings.Cast(GetGame().GetGameMode().FindComponent(SCR_GameModeHealthSettings)).SetUnconsciousnessPermitted(false);
	}
	
	vector GetSpawnPoint()
	{
		int amountOfSpawns = 0;
		for (int i = 0; i < 128; i++)
		{
			IEntity spawnPoint = GetGame().GetWorld().FindEntityByName("spawnpoint" + i.ToString());
			if (!spawnPoint)
				break;
			
			amountOfSpawns++;
			
			if (!GetGame().GetWorld().QueryEntitiesBySphere(spawnPoint.GetOrigin(), 25, SpawnPointCallBack, null))
				continue;
			
			return spawnPoint.GetOrigin();
		}
		
		RandomGenerator random = new RandomGenerator();
		int randomSpawn = random.RandInt(0, amountOfSpawns);
		IEntity randomSpawnpoint = GetGame().GetWorld().FindEntityByName("spawnpoint" + randomSpawn);
		return randomSpawnpoint.GetOrigin();
	}
	
	bool SpawnPointCallBack(IEntity entity)
	{
		if (ChimeraCharacter.Cast(entity))
		{
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
		if (killerId == -1)
			return;
		
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
			
			m_aKills.Set(index, currentKills);
			m_aKillsThisLevel.Set(index, currentKillsThisLevel);
			NewWeaponCheck(killerId);
			Replication.BumpMe();
			if (m_aKills.Get(index) == m_iKillsToWin)
			{
				GameOver();
				m_bIsGameOver = true;
				Replication.BumpMe();
				return;
			}
		}
		
		GetGame().GetCallqueue().CallLater(RespawnPlayer, 5000, false, instigatorContextData.GetVictimPlayerID(), GetSpawnPoint());
	}
	
	void GameOver()
	{
		#ifdef WORKBENCH
		RpcDo_BroadcastGameOver();
		#else
		Rpc(RpcDo_BroadcastGameOver);
		#endif
	}
	
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
	
	array<int> GetWinners()
	{
		ref array<int> winners = {};
		int first = -1;
		int second = -1;
		int third = -1;
		for (int i = 0; i < m_aKills.Count(); i++)
		{
			if (m_aKills.Get(i) > first)
			{
				first = m_aPlayers.Get(i);
			}
			else if (m_aKills.Get(i) > second)
			{
				second = m_aPlayers.Get(i);
			}
			else if (m_aKills.Get(i) > third)
			{
				third = m_aPlayers.Get(i);
			}
		}
		winners.Insert(first);
		winners.Insert(second);
		winners.Insert(third);
		return winners;
	}
	
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
		m_aKillsThisLevel.Set(index, 0);
		kills = Math.ClampInt(kills, 0, 100);
		m_aKills.Set(index, kills);
		m_aLevels.Set(index, levels);
		Replication.BumpMe();
	}
	
	void AddMedal(string medalImage, string medalText)
	{
		GunGameMedalContainer medalContainer = new GunGameMedalContainer();
		medalContainer.m_sMedalImage = medalImage;
		medalContainer.m_sMedalText = medalText;
		m_aMedals.Insert(medalContainer);
	}
	
	float m_fKillIconTimer = 0;
	void ProcessKillClient(notnull SCR_InstigatorContextData instigatorContextData)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif
		
		if (SCR_PlayerController.GetLocalPlayerId() == instigatorContextData.GetVictimPlayerID())
		{
			m_iLastKillStreak = 0;
			m_iKillStreak = 0;
			m_iComebackCounter++;
			m_iRevengePlayer = instigatorContextData.GetKillerPlayerID();
			return;
		}
		
		if (instigatorContextData.GetVictimPlayerID() == 0)
			return;
			
		if (SCR_PlayerController.GetLocalPlayerId() != instigatorContextData.GetKillerPlayerID())
			return;
		
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
	
	void RespawnPlayer(int playerId, vector spawn)
	{
		CRF_RespawnManager.GetInstance().RespawnPlayer(playerId, spawn);
	}
	
	int m_iBeepTimer = 10;
	float m_fDropShotTimer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() != RplMode.Client)
			return;
		#endif
		
		CheckIfWeaponEquipped();
		UpdateKillIcon(timeSlice);
		UpdateHUD(timeSlice);
		UpdateKillStreak(timeSlice);
		UpdateCurrentWeapon(timeSlice);
		UpdateDropShot(timeSlice);
		
		if (m_bIsGameOver)
			if (!m_GameOverMenu)
				RpcDo_BroadcastGameOver();
		
		if (m_wHitmarker)
			m_wHitmarker.SetOpacity(m_wHitmarker.GetOpacity() - (timeSlice * 2));
	}
	
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
	
	void UpdateCurrentWeapon(float timeSlice)
	{
		m_fTimeSinceLastWeapon += timeSlice;
	}
	
	float m_fMedalTimer = 0;
	void UpdateKillStreak(float timeSlice)
	{
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
		
		if (m_aMedals.Count() == 0)
			return;
		
		if (!m_wHUD)
			return;
		
		ImageWidget medalImage = ImageWidget.Cast(m_wHUD.FindWidget("MedalImage"));
		TextWidget medalText = TextWidget.Cast(m_wHUD.FindWidget("MedalText"));
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
	
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		#ifdef WORKBENCH
		#else
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif
		
		GetGame().GetCallqueue().CallLater(SpawnCheck, 500, false, entity);
	}
	
	void SpawnCheck(IEntity entity)
	{
		if (!entity)
			return;
		
		if (entity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			return;
		
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(entity);
		
		int index = m_aPlayers.Find(playerId);
		if (index == -1)
			return;
		
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
	
	void NewWeaponCheck(int playerId)
	{
		int index = m_aPlayers.Find(playerId);
		if (index == -1)
			return;
		
		int level = m_aLevels.Get(index);
		ref CRF_GunGameContainer gunLevel = m_aGunLevels.Get(level);
		
		int currentKillsAtThisLevel = m_aKillsThisLevel.Get(index);
		if (currentKillsAtThisLevel == -1)
		{
			m_aKillsThisLevel.Set(index, 0);
			level--;
	
			level = Math.ClampInt(level, 0, 100);
			
			m_aLevels.Set(index, level);
			Replication.BumpMe();
			NewLevel(playerId);
			return;
		}
		if (gunLevel.m_iAmountOfKillsToLevelUp >  currentKillsAtThisLevel)
			return;
		
		m_aKillsThisLevel.Set(index, 0);
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
		m_aLevels.Set(index, level);
		Replication.BumpMe();
		NewLevel(playerId);
	}
	
	void NewLevel(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!player)
			return;
		
		BaseWeaponManagerComponent weaponMan = BaseWeaponManagerComponent.Cast(ChimeraCharacter.Cast(player).FindComponent(BaseWeaponManagerComponent));
		if (!weaponMan)
			return;
		
		IEntity currentWeapon = weaponMan.GetCurrentWeapon().GetOwner();
		if (!currentWeapon)
			return;
		
		SCR_EntityHelper.DeleteEntityAndChildren(currentWeapon);
		
		int index = m_aPlayers.Find(playerId);
		if (index == -1)
			return;
		
		int level = m_aLevels.Get(index);
		if (level > m_aGunLevels.Count() - 1)
			return;
		CRF_GunGameContainer gunLevel = m_aGunLevels.Get(level);
		GetGame().GetCallqueue().CallLater(NewLevelAddWeapon, 200, false, player, gunLevel.m_sWeapon, gunLevel.m_sMagazines, gunLevel.m_iAmountOfMagazines);
	}
	
	void NewLevelAddWeapon(IEntity player, ResourceName weapon, ResourceName magazine, int amount)
	{
		SCR_InventoryStorageManagerComponent storageManagerComponent = SCR_InventoryStorageManagerComponent.Cast(player.FindComponent(SCR_InventoryStorageManagerComponent));
		storageManagerComponent.TrySpawnPrefabToStorage(weapon, null, -1, EStoragePurpose.PURPOSE_WEAPON_PROXY);
		
		for (int i = 1; i < amount; i++)
		{
			storageManagerComponent.TrySpawnPrefabToStorage(magazine, null, -1, EStoragePurpose.PURPOSE_ANY);
		}
	}
	
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
		Replication.BumpMe();
	}
}