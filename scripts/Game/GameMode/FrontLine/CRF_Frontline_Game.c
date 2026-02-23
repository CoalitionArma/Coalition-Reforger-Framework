[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_FrontlineGamemodeManagerClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_FrontlineGamemodeManager: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side designated as blufor, this faction will have control of the beginning half of the total zones at game start. \n\n In Example: If the total zones were [A, B, C, D, E] this faction would have control of [A, B] at game start", category: "Frontline Faction Settings")]
	FactionKey m_BluforSide;
	
	[Attribute("USA", "auto", "Nickname for the side designated as blufor", category: "Frontline Faction Settings")]
	string m_sBluforSideNickname;
	
	[Attribute("USSR", "auto", "The side designated as opfor, this faction will have control of the last half of the total zones at game start. \n\n In Example: If the total zones were [A, B, C, D, E] this faction would have control of [D, E] at game start", category: "Frontline Faction Settings")]
	FactionKey m_OpforSide;
	
	[Attribute("USSR", "auto", "Nickname for the side designated as opfor", category: "Frontline Faction Settings")]
	string m_sOpforSideNickname;
	
	[Attribute("", UIWidgets.EditBox, desc: "Array of all zone object names, !MAKE SURE ALL OBJECTS LISTED ARE INVINCIBLE!", category: "Frontline Zone Settings")]
	ref array<string> m_aZoneObjectNames;
	
	[Attribute("30", "auto", "[Seconds] The amount of time it takes to cap a zone", category: "Frontline Zone Settings")]
	int m_iZoneCaptureTime;
	
	[Attribute("600", "auto", "[Seconds] Time until the frontline zones are unlocked after being locked, recommend you stick to on of these: [5 minutes, 10 minutes, 15 minutes, 20 minutes]", category: "Frontline Zone Settings")]
	int m_iZoneUnlockTime;
	
	[Attribute("5", "auto", "Min number of players needed to cap a zone", category: "Frontline Zone Settings")]
	int m_iMinNumberOfPlayersNeeded;
	
	[Attribute("600", "auto", "[Seconds] When all zones are captured by a side, it'll take this set time to declare that side a victor", category: "Frontline Zone Settings")]
	int m_iTimeToWin;
	
	[Attribute("900", "auto", "[Seconds] Time until the middle zone is unlocked", category: "Frontline Zone Settings")]
	int m_iInitialTime;
	
	// - All players within a zones range
	ref array<SCR_ChimeraCharacter> m_aAllPlayersWithinZoneRange = new array<SCR_ChimeraCharacter>;
	
	// RPC-based state management - no more BumpMe needed
	protected string m_sHudMessage;
	protected string m_sRadioSoundString;
	protected string m_sAlertSoundString;
	protected ref array<string> m_aZonesStatus = new array<string>;
	
	[RplProp()]
	bool m_bGameStarted = false;
	
	bool m_bAnyZonesBeingCaptured = false;
	int m_iWhichZoneCaptureIsInProgress = -1;
	int m_iZoneUnlockTimeIteratorInt = m_iZoneUnlockTime + 1;
	int m_iTimeToWinIteratorInt = m_iTimeToWin;
	int m_iInitialTimeIteratorInt = m_iInitialTime;
	
	bool m_bStartGame = false;
	bool m_bInitialMarkerCheck = false;
	bool m_bZoneUpdateChecks = false;
	int m_iZoneIndex;
	int m_iZoneIndex2;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------

	// Instance of this component (this method only works if you KNOW there will only ever be one instance of this component) 
	protected static CRF_FrontlineGamemodeManager s_Instance;
	
	//------------------------------------------------------------------------------------------------
	void CRF_FrontlineGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!s_Instance)
			s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_FrontlineGamemodeManager GetInstance()
	{
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	// Public getters for protected variables
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	string GetHudMessage()
	{
		return m_sHudMessage;
	}
	
	//------------------------------------------------------------------------------------------------
	array<string> GetZonesStatus()
	{
		return m_aZonesStatus;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FIXEDFRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		// Initialize the array on all machines (needed for RPC updates to work)
		SetupZoneStatus();
		
		// Server-only initialization
		if (RplSession.Mode() == RplMode.Client)
			return;
	}
	
	float m_fUpdateBuffer = 0;
	override void EOnFixedFrame(IEntity owner, float timeSlice)
	{
		super.EOnFixedFrame(owner, timeSlice);
		if (m_fUpdateBuffer >= 1)
		{
			m_fUpdateBuffer = 0;
			
			if (!m_bInitialMarkerCheck)
				CheckAddInitialMarkers();
			if (RplSession.Mode() == RplMode.Client)
				return;
			UpdateZones();
			if (!m_bStartGame)
				StartGame();
			if (m_bZoneUpdateChecks)
				UnlockZone(m_iZoneIndex, m_iZoneIndex2);
		}
		m_fUpdateBuffer += timeSlice;
		
	}
	
	//------------------------------------------------------------------------------------------------
	// RPC Methods for efficient network updates
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateHudMessage(string message)
	{
		m_sHudMessage = message;
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateZoneStatus(int index, string status)
	{
		if (index < 0 || index >= m_aZonesStatus.Count())
			return;
			
		m_aZonesStatus.Set(index, status);
		UpdateClients();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateMultipleZones(array<int> indices, array<string> statuses)
	{
		if (indices.Count() != statuses.Count())
			return;
			
		for (int i = 0; i < indices.Count(); i++)
		{
			int index = indices[i];
			if (index >= 0 && index < m_aZonesStatus.Count())
				m_aZonesStatus.Set(index, statuses[i]);
		}
		UpdateClients();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayRadioSound(string message)
	{
		m_sHudMessage = message;
		PlayRadioSound();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_PlayAlertSound(string message)
	{
		m_sHudMessage = message;
		PlayAlertSound();
	}
	
	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ClearMessages()
	{
		m_sHudMessage = "";
		m_sAlertSoundString = "";
	}

	//------------------------------------------------------------------------------------------------

	// Frontline functions

	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void CheckAddInitialMarkers()
	{
		// Create markers on each bomb site
		CRF_PlayerControllerManager gameModePlayerComponent = CRF_PlayerControllerManager.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		gameModePlayerComponent.UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, m_BluforSide, m_OpforSide);
		
		m_bInitialMarkerCheck = true;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetupZoneStatus()
	{
		int totalZones = m_aZoneObjectNames.Count() - 1;
		
		foreach(int index, string names : m_aZoneObjectNames)
		{
			string faction = "N/A";
			
			if (index < (totalZones/2))
				faction = m_BluforSide;
			
			if (index > (totalZones/2))
				faction = m_OpforSide;
		
			m_aZonesStatus.Insert(string.Format("%1:%2:%3", faction, "Locked", faction));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void StartGame()
	{
		if(CRF_SafestartManager.GetInstance().GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			return;
		
		int zoneIndex = ((m_aZonesStatus.Count()-1)/2);
		string zoneText = ConvertIndexToZoneString(zoneIndex);
		
		if(m_iInitialTimeIteratorInt != 0)
		{
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", "N/A", m_iInitialTimeIteratorInt, "N/A"));
			string message = string.Format("%1 Unlocks In: %2", zoneText, SCR_FormatHelper.FormatTime(m_iInitialTimeIteratorInt));
			m_iInitialTimeIteratorInt = m_iInitialTimeIteratorInt - 1;
			
			Rpc(RpcDo_UpdateZoneStatus, zoneIndex, string.Format("%1:%2:%3", "N/A", m_iInitialTimeIteratorInt, "N/A"));
			Rpc(RpcDo_UpdateHudMessage, message);
			return;
		};
		
		m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", "N/A", "Unlocked", "N/A"));
		string unlockMessage = string.Format("%1 Unlocked!", zoneText);
		m_bGameStarted = true;
		
		Rpc(RpcDo_UpdateZoneStatus, zoneIndex, string.Format("%1:%2:%3", "N/A", "Unlocked", "N/A"));
		Rpc(RpcDo_PlayRadioSound, unlockMessage);
		Replication.BumpMe(); // Only for m_bGameStarted flag
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
		
		m_bStartGame = true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateZones()
	{
		if(CRF_SafestartManager.GetInstance().GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning() || !m_bGameStarted)
			return;
		
		int zonesCapturedBlufor;
		int zonesCapturedOpfor;
		int zonesFullyCapturedBlufor;
		int zonesFullyCapturedOpfor;
		
		m_bAnyZonesBeingCaptured = false;
		
		foreach(int index, string zoneName : m_aZoneObjectNames)
		{
			m_aAllPlayersWithinZoneRange.Clear();
			
			IEntity zone = GetGame().GetWorld().FindEntityByName(zoneName);
			
			if(!zone)
				continue;
			
			GetGame().GetWorld().QueryEntitiesBySphere(zone.GetOrigin(), 75, ProcessEntity, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT); // get all entitys within a 150m radius around the zone
			
			float bluforInZone = 0;
			float opforInZone = 0;
			
			foreach(SCR_ChimeraCharacter player : m_aAllPlayersWithinZoneRange) 
			{
				if(player.GetFactionKey() == m_BluforSide)
					bluforInZone = bluforInZone + 1;
				
				if(player.GetFactionKey() == m_OpforSide)
					opforInZone = opforInZone + 1;
			}
			
			string status = m_aZonesStatus[index];
			
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
			
			FactionKey zoneFaction = zoneStatusArray[0];
			string zoneState = zoneStatusArray[1];
			FactionKey zoneFactionStored = zoneStatusArray[2];
			
			if(zoneFaction == m_BluforSide)
				zonesCapturedBlufor = zonesCapturedBlufor + 1;
			
			if(zoneFaction == m_OpforSide)
				zonesCapturedOpfor = zonesCapturedOpfor + 1;
			
			if(zoneFactionStored == m_BluforSide)
				zonesFullyCapturedBlufor = zonesFullyCapturedBlufor + 1;
		
			if(zoneFactionStored == m_OpforSide)
				zonesFullyCapturedOpfor = zonesFullyCapturedOpfor + 1;
			
			if (bluforInZone >= m_iMinNumberOfPlayersNeeded && opforInZone < (bluforInZone/2))
			{
				CheckStartCaptureZone(index, m_BluforSide, zoneFaction, zoneState, zoneFactionStored);
				continue;
			};
			
			if (opforInZone >= m_iMinNumberOfPlayersNeeded && bluforInZone < (opforInZone/2)) 
			{
				CheckStartCaptureZone(index, m_OpforSide, zoneFaction, zoneState, zoneFactionStored);
				continue;
			};
			
			if(zoneState.ToInt() != 0)  
			{
				m_aZonesStatus.Set(index, string.Format("%1:%2:%3", zoneFactionStored, "Unlocked", zoneFactionStored));
				Rpc(RpcDo_UpdateZoneStatus, index, string.Format("%1:%2:%3", zoneFactionStored, "Unlocked", zoneFactionStored));
			};
		}
		
		if(!m_bAnyZonesBeingCaptured)
			m_iWhichZoneCaptureIsInProgress = -1;
		
		if (m_sHudMessage == string.Format("%1 Victory!", m_sBluforSideNickname) || m_sHudMessage == string.Format("%1 Victory!", m_sOpforSideNickname))
		{
			array<int> indices = {};
			array<string> statuses = {};
			
			foreach(int index, string zoneName : m_aZoneObjectNames)
			{
				string status = m_aZonesStatus[index];
			
				array<string> zoneStatusArray = {};
				status.Split(":", zoneStatusArray, false);
		
				m_aZonesStatus.Set(index, string.Format("%1:%2:%3", zoneStatusArray[0], "Locked", zoneStatusArray[0]));
				indices.Insert(index);
				statuses.Insert(string.Format("%1:%2:%3", zoneStatusArray[0], "Locked", zoneStatusArray[0]));
			}
			
			Rpc(RpcDo_UpdateMultipleZones, indices, statuses);
			return;
		}
		
		
		if(zonesCapturedBlufor == m_aZoneObjectNames.Count() && zonesFullyCapturedBlufor == m_aZoneObjectNames.Count())
		{
			m_iTimeToWinIteratorInt = m_iTimeToWinIteratorInt - 1;
			
			string victoryMessage;
			if(m_iTimeToWinIteratorInt <= 0)
			{
				victoryMessage = string.Format("%1 Victory!", m_sBluforSideNickname);
				m_sHudMessage = victoryMessage;
				Rpc(RpcDo_PlayRadioSound, victoryMessage);
			} else {
				victoryMessage = string.Format("%1 Victory In: %2", m_sBluforSideNickname, SCR_FormatHelper.FormatTime(m_iTimeToWinIteratorInt));
				m_sHudMessage = victoryMessage;
				Rpc(RpcDo_UpdateHudMessage, victoryMessage);
			};

			return;
		};
		
		if(zonesCapturedOpfor == m_aZoneObjectNames.Count() && zonesFullyCapturedOpfor == m_aZoneObjectNames.Count())
		{
			m_iTimeToWinIteratorInt = m_iTimeToWinIteratorInt - 1;

			string victoryMessage;
			if(m_iTimeToWinIteratorInt <= 0)
			{
				victoryMessage = string.Format("%1 Victory!", m_sOpforSideNickname);
				m_sHudMessage = victoryMessage;
				Rpc(RpcDo_PlayRadioSound, victoryMessage);
			} else {
				victoryMessage = string.Format("%1 Victory In: %2", m_sOpforSideNickname, SCR_FormatHelper.FormatTime(m_iTimeToWinIteratorInt));
				m_sHudMessage = victoryMessage;
				Rpc(RpcDo_UpdateHudMessage, victoryMessage);
			};

			return;
		};
		
		if(zonesFullyCapturedBlufor != m_aZoneObjectNames.Count() && zonesFullyCapturedOpfor != m_aZoneObjectNames.Count())
			m_iTimeToWinIteratorInt = m_iTimeToWin;
	};
	
	//------------------------------------------------------------------------------------------------
	protected bool ProcessEntity(IEntity processEntity)
	{
		SCR_ChimeraCharacter playerCharacter = SCR_ChimeraCharacter.Cast(processEntity);
		if (!playerCharacter) 
			return true;

		int processEntityID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(playerCharacter);
		if (!processEntityID) 
			return true;

		m_aAllPlayersWithinZoneRange.Insert(playerCharacter);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void CheckStartCaptureZone(int zoneIndex, FactionKey side, FactionKey zoneFaction, string zoneState, FactionKey zoneFactionStored)
	{	
		if (zoneState == "Locked" || (zoneFaction == side && zoneState == "Unlocked") || ((m_iWhichZoneCaptureIsInProgress != zoneIndex) && (m_iWhichZoneCaptureIsInProgress != -1))) 
			return;
		
		m_bAnyZonesBeingCaptured = true;
		
		if(zoneState.ToInt() >= m_iZoneCaptureTime) // Zone officially captured
		{
			ZoneCaptured(zoneIndex, side);
			m_iWhichZoneCaptureIsInProgress = -1;
			return;
		};
		
		if (zoneState.ToInt() == 1)
		{
			string nickname;
			switch(side)
			{
				case m_BluforSide : { nickname = m_sBluforSideNickname; break;}; //Blufor
				case m_OpforSide  : { nickname = m_sOpforSideNickname;  break;}; //Opfor
			}
			
			string alertMessage = nickname + " Is Capturing " + ConvertIndexToZoneString(zoneIndex) + "!";
			m_sHudMessage = alertMessage;
			Rpc(RpcDo_PlayAlertSound, alertMessage);
			
			GetGame().GetCallqueue().CallLater(ResetMessage, 8500);
		};
		 
		m_iWhichZoneCaptureIsInProgress = zoneIndex;
		m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, zoneState.ToInt() + 1, zoneFactionStored));
		
		Rpc(RpcDo_UpdateZoneStatus, zoneIndex, string.Format("%1:%2:%3", side, zoneState.ToInt() + 1, zoneFactionStored));
	};
	
	//------------------------------------------------------------------------------------------------
	void ZoneCaptured(int zoneIndex, FactionKey side)
	{
		string zoneText = ConvertIndexToZoneString(zoneIndex);
		string nickname;
		int zoneIndexToChange;
		int zoneIndexBehind;
			
		switch(side)
		{
			case m_BluforSide : { nickname = m_sBluforSideNickname; zoneIndexToChange = zoneIndex + 1; zoneIndexBehind = zoneIndex - 1; break;}; //Blufor
			case m_OpforSide  : { nickname = m_sOpforSideNickname;  zoneIndexToChange = zoneIndex - 1; zoneIndexBehind = zoneIndex + 1; break;}; //Opfor
		}
		
		m_sHudMessage = string.Format("%1 Captured %2!", nickname, zoneText);
		m_sRadioSoundString = m_sHudMessage;
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 7250);
		
		// Respawn all players when zone is captured
		CRF_RespawnManager respawnManager = CRF_RespawnManager.GetInstance();
		if (respawnManager)
			respawnManager.RespawnAllSides();
		
		if((zoneIndex == (m_aZoneObjectNames.Count() - 1) && side == m_BluforSide) || (zoneIndex == 0 && side == m_OpforSide))
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, "Unlocked", side));
		else
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, "Locked", side));
		
		if(zoneIndexBehind <= (m_aZoneObjectNames.Count() - 1) && zoneIndexBehind >= 0)
		{
			string zoneBehindToChange = m_aZonesStatus.Get(zoneIndexBehind);
		
			array<string> zoneStatusBehindArray = {};
			zoneBehindToChange.Split(":", zoneStatusBehindArray, false);
		
			m_aZonesStatus.Set(zoneIndexBehind, string.Format("%1:%2:%3", zoneStatusBehindArray[0], "Locked", zoneStatusBehindArray[2]));
		};
		
		if(zoneIndexToChange > (m_aZoneObjectNames.Count() - 1) || zoneIndexToChange < 0)
			return;
		
		string zoneStatusToChange = m_aZonesStatus.Get(zoneIndexToChange);
		
		array<string> zoneStatusToChangeArray = {};
		zoneStatusToChange.Split(":", zoneStatusToChangeArray, false);
		
		m_aZonesStatus.Set(zoneIndexToChange, string.Format("%1:%2:%3", zoneStatusToChangeArray[0], "Locked", zoneStatusToChangeArray[2]));
		
		// Batch update all zone changes
		array<int> indices = {zoneIndex, zoneIndexToChange};
		
		string capturedZoneStatus;
		if ((zoneIndex == (m_aZoneObjectNames.Count() - 1) && side == m_BluforSide) || (zoneIndex == 0 && side == m_OpforSide))
			capturedZoneStatus = "Unlocked";
		else
			capturedZoneStatus = "Locked";
		
		array<string> statuses = {
			string.Format("%1:%2:%3", side, capturedZoneStatus, side),
			string.Format("%1:%2:%3", zoneStatusToChangeArray[0], "Locked", zoneStatusToChangeArray[2])
		};
		
		if(zoneIndexBehind <= (m_aZoneObjectNames.Count() - 1) && zoneIndexBehind >= 0)
		{
			indices.Insert(zoneIndexBehind);
			string zoneBehindToChange = m_aZonesStatus.Get(zoneIndexBehind);
			array<string> zoneStatusBehindArray = {};
			zoneBehindToChange.Split(":", zoneStatusBehindArray, false);
			statuses.Insert(string.Format("%1:%2:%3", zoneStatusBehindArray[0], "Locked", zoneStatusBehindArray[2]));
			m_aZonesStatus.Set(zoneIndexBehind, string.Format("%1:%2:%3", zoneStatusBehindArray[0], "Locked", zoneStatusBehindArray[2]));
		};
		
		Rpc(RpcDo_UpdateMultipleZones, indices, statuses);
		
		GetGame().GetCallqueue().CallLater(UnlockZoneDelay, 7650, false, zoneIndex, zoneIndexToChange); // need to allow players to read m_sHudMessage
	}
	
	//------------------------------------------------------------------------------------------------
	void UnlockZoneDelay(int zoneIndex, int zoneIndexTwo)
	{	
		m_iZoneIndex = zoneIndex;
		m_iZoneIndex2 = zoneIndexTwo;
		m_bZoneUpdateChecks = true;
	};
	
	//------------------------------------------------------------------------------------------------
	void UnlockZone(int zoneIndex, int zoneIndexTwo)
	{	
		if (m_iZoneUnlockTimeIteratorInt > 0) 
			m_iZoneUnlockTimeIteratorInt = m_iZoneUnlockTimeIteratorInt - 1;

		array<string> checkIfTimerArray = {};
		m_sHudMessage.Split(":", checkIfTimerArray, true);
		
		string zoneText = ConvertIndexToZoneString(zoneIndex);
		string zoneTextTwo = ConvertIndexToZoneString(zoneIndexTwo);
		
		if(m_iZoneUnlockTimeIteratorInt <= 0)
		{
			m_iZoneUnlockTimeIteratorInt = m_iZoneUnlockTime + 1;
			
			string status = m_aZonesStatus[zoneIndex];
			array<string> zoneStatusArray = {};
			status.Split(":", zoneStatusArray, false);
		
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", zoneStatusArray[2], "Unlocked", zoneStatusArray[2]));
		
			string statusTwo = m_aZonesStatus[zoneIndexTwo];
			array<string> zoneStatusTwoArray = {};
			statusTwo.Split(":", zoneStatusTwoArray, false);
		
			m_aZonesStatus.Set(zoneIndexTwo, string.Format("%1:%2:%3", zoneStatusTwoArray[2], "Unlocked", zoneStatusTwoArray[2]));
			 
			if (checkIfTimerArray.Count() <= 3)
			{
				string unlockMessage;
				if(zoneText != zoneTextTwo)
					unlockMessage = zoneText + " & " + zoneTextTwo + " Are Now Unlocked!";
				else
					unlockMessage = zoneText + " Is Now Unlocked!";
			
				m_sHudMessage = unlockMessage;
				Rpc(RpcDo_PlayRadioSound, unlockMessage);
				GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
			};
			
			// Update both zones
			array<int> indices = {zoneIndex, zoneIndexTwo};
			array<string> statuses = {
				string.Format("%1:%2:%3", zoneStatusArray[2], "Unlocked", zoneStatusArray[2]),
				string.Format("%1:%2:%3", zoneStatusTwoArray[2], "Unlocked", zoneStatusTwoArray[2])
			};
			Rpc(RpcDo_UpdateMultipleZones, indices, statuses);
			
			m_bZoneUpdateChecks = false;
			return;
		};

		if (checkIfTimerArray.Count() <= 3)
		{
			string timerMessage;
			switch (m_iZoneUnlockTimeIteratorInt)
			{
				case 1200: {
					if(zoneText != zoneTextTwo)
						timerMessage = zoneText + " & " + zoneTextTwo + " Unlock In 20 Minute(s)";
					else
						timerMessage = zoneText + " Unlocks In 20 Minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sHudMessage = timerMessage;
					Rpc(RpcDo_PlayRadioSound, timerMessage);
					break;
				};
				case 900: {
				if(zoneText != zoneTextTwo)
						timerMessage = zoneText + " & " + zoneTextTwo + " Unlock In 15 Minute(s)";
					else
						timerMessage = zoneText + " Unlocks In 15 Minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sHudMessage = timerMessage;
					Rpc(RpcDo_PlayRadioSound, timerMessage);
					break;
				};
				case 600: {
					if(zoneText != zoneTextTwo)
						timerMessage = zoneText + " & " + zoneTextTwo + " Unlock In 10 Minute(s)";
					else
						timerMessage = zoneText + " Unlocks In 10 Minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sHudMessage = timerMessage;
					Rpc(RpcDo_PlayRadioSound, timerMessage);
					break;
				};
				case 300: {
					if(zoneText != zoneTextTwo)
						timerMessage = zoneText + " & " + zoneTextTwo + " Unlock In 5 Minute(s)";
					else
						timerMessage = zoneText + " Unlocks In 5 Minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sHudMessage = timerMessage;
					Rpc(RpcDo_PlayRadioSound, timerMessage);
					break;
				}
				case 60: {
					if(zoneText != zoneTextTwo)
						timerMessage = zoneText + " & " + zoneTextTwo + " Unlock In 1 Minute!";
					else
						timerMessage = zoneText + " Unlocks In 1 Minute";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sHudMessage = timerMessage;
					Rpc(RpcDo_PlayRadioSound, timerMessage);
					break;
				}
				case 15: {
					if(zoneText != zoneTextTwo)
						timerMessage = zoneText + " & " + zoneTextTwo + " Unlock In 15 seconds!";
					else
						timerMessage = zoneText + " Unlocks In 15 Seconds!";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sHudMessage = timerMessage;
					Rpc(RpcDo_PlayRadioSound, timerMessage);
					break;
				}
			}
		};
	}
	
	//------------------------------------------------------------------------------------------------
	string ConvertIndexToZoneString(int index)
	{
		string zoneText;
		switch(index)
		{
			case 0  : {zoneText = "Zone A"; break;};
			case 1  : {zoneText = "Zone B"; break;};
			case 2  : {zoneText = "Zone C"; break;};
			case 3  : {zoneText = "Zone D"; break;};
			case 4  : {zoneText = "Zone E"; break;};
			case 5  : {zoneText = "Zone F"; break;};
			case 6  : {zoneText = "Zone G"; break;};
			case 7  : {zoneText = "Zone H"; break;};
			case 8  : {zoneText = "Zone I"; break;};
			case 9  : {zoneText = "Zone J"; break;};
			case 10 : {zoneText = "Zone K"; break;};
			case 11 : {zoneText = "Zone L"; break;};
			case 12 : {zoneText = "Zone M"; break;};
			case 13 : {zoneText = "Zone N"; break;};
			case 14 : {zoneText = "Zone O"; break;};
			case 15 : {zoneText = "Zone P"; break;};
			case 16 : {zoneText = "Zone Q"; break;};
			case 17 : {zoneText = "Zone R"; break;};
			case 18 : {zoneText = "Zone S"; break;};
			case 19 : {zoneText = "Zone T"; break;};
			case 20 : {zoneText = "Zone U"; break;};
			case 21 : {zoneText = "Zone V"; break;};
			case 22 : {zoneText = "Zone W"; break;};
			case 23 : {zoneText = "Zone X"; break;};
			case 24 : {zoneText = "Zone Y"; break;};
			case 25 : {zoneText = "Zone Z"; break;};
		}
		return zoneText;
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetMessage()
	{
		array<string> checkIfTimerArray = {};
		m_sHudMessage.Split(":", checkIfTimerArray, true);
		
		if (checkIfTimerArray.Count() <= 3)
		{
			Rpc(RpcDo_ClearMessages);
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateClients()
	{
		CRF_PlayerControllerManager instance = CRF_PlayerControllerManager.GetInstance();
		if (!instance)
		{
			Print("[CRF_Frontline] WARNING: CRF_PlayerControllerManager instance not found, skipping UpdateMapMarkers", LogLevel.WARNING);
			return;
		}
		
		instance.UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, m_BluforSide, m_OpforSide);
	}
	
	//------------------------------------------------------------------------------------------------
	void PlayRadioSound()
	{
		AudioSystem.PlaySound("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav");
	};
	
	//------------------------------------------------------------------------------------------------
	void PlayAlertSound()
	{
		AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
	};
};
