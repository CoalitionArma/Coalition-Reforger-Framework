[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_LinearAASGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_LinearAASGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "The side designated as blufor, this faction will have control of the beginning half of the total zones at game start. \n\n In Example: If the total zones were [A, B, C, D, E] this faction would have control of [A, B] at game start", category: "Linear AAS Faction Settings")]
	FactionKey m_BluforSide;
	
	[Attribute("USA", "auto", "Nickname for the side designated as blufor", category: "Linear AAS Faction Settings")]
	string m_sBluforSideNickname;
	
	[Attribute("USSR", "auto", "The side designated as opfor, this faction will have control of the last half of the total zones at game start. \n\n In Example: If the total zones were [A, B, C, D, E] this faction would have control of [D, E] at game start", category: "Linear AAS Faction Settings")]
	FactionKey m_OpforSide;
	
	[Attribute("USSR", "auto", "Nickname for the side designated as opfor", category: "Linear AAS Faction Settings")]
	string m_sOpforSideNickname;
	
	[Attribute("", UIWidgets.EditBox, desc: "Array of all zone object names, !MAKE SURE ALL OBJECTS LISTED ARE INVINCIBLE!", category: "Linear AAS Zone Settings")]
	ref array<string> m_aZoneObjectNames;
	
	[Attribute("30", "auto", "[Seconds] The amount of time it takes to cap a zone", category: "Linear AAS Zone Settings")]
	int m_iZoneCaptureTime;
	
	[Attribute("600", "auto", "[Seconds] Time until the frontline zones are unlocked after being locked, recommend you stick to on of these: [5 minutes, 10 minutes, 15 minutes, 20 minutes]", category: "Linear AAS Zone Settings")]
	int m_iZoneUnlockTime;
	
	[Attribute("5", "auto", "Min number of players needed to cap a zone", category: "Linear AAS Zone Settings")]
	int m_iMinNumberOfPlayersNeeded;
	
	[Attribute("600", "auto", "[Seconds] When all zones are captured by a side, it'll take this set time to declare that side a victor", category: "Linear AAS Zone Settings")]
	int m_iTimeToWin;
	
	[Attribute("900", "auto", "[Seconds] Time until the middle zone is unlocked", category: "Linear AAS Zone Settings")]
	int m_iInitialTime;
	
	// - All players within a zones range
	ref array<SCR_ChimeraCharacter> m_aAllPlayersWithinZoneRange = new array<SCR_ChimeraCharacter>;
	
	[RplProp(onRplName: "UpdateClients")]
	string m_sHudMessage;
	
	[RplProp(onRplName: "PlaySound")]
	string m_sSoundString;
	
	[RplProp(onRplName: "UpdateClients")]
	ref array<string> m_aZonesStatus = new array<string>;
	
	bool m_bGameStarted = false;
	bool m_bAnyZonesBeingCaptured = false;
	int m_iWhichZoneCaptureIsInProgress = -1;
	int m_iZoneUnlockTimeIteratorInt = m_iZoneUnlockTime + 1;
	int m_iInitialTimeIteratorInt = m_iInitialTime;
	
	//------------------------------------------------------------------------------------------------

	// override/static functions

	//------------------------------------------------------------------------------------------------

	static CRF_LinearAASGameModeComponent GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_LinearAASGameModeComponent.Cast(gameMode.FindComponent(CRF_LinearAASGameModeComponent));
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		GetGame().GetCallqueue().CallLater(CheckAddInitialMarkers, 1000, true);

		//--- Server only
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		SetupZoneStatus();
		GetGame().GetCallqueue().CallLater(UpdateZones, 1000, true);
		GetGame().GetCallqueue().CallLater(StartGame, 1000, true);
	}
	
	//------------------------------------------------------------------------------------------------

	// Linear AAS functions

	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void CheckAddInitialMarkers()
	{
		// Create markers on each bomb site
		CRF_GameModePlayerComponent gameModePlayerComponent = CRF_GameModePlayerComponent.GetInstance();
		if (!gameModePlayerComponent) 
			return;
		
		gameModePlayerComponent.UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, m_BluforSide, m_OpforSide);
		
		GetGame().GetCallqueue().Remove(CheckAddInitialMarkers);
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
		CRF_SafestartGameModeComponent safestart = CRF_SafestartGameModeComponent.GetInstance();
		if(safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning())
			return;
		
		int zoneIndex = ((m_aZonesStatus.Count()-1)/2);
		string zoneText = ConvertIndexToZoneString(zoneIndex);
		
		if(m_iInitialTimeIteratorInt != 0)
		{
			m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", "N/A", m_iInitialTimeIteratorInt, "N/A"));
			m_sHudMessage = string.Format("%1 Unlocked: %2", zoneText, SCR_FormatHelper.FormatTime(m_iInitialTimeIteratorInt));
			m_iInitialTimeIteratorInt = m_iInitialTimeIteratorInt - 1;
			Replication.BumpMe();
			return;
		};
		
		m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", "N/A", "Unlocked", "N/A"));
		m_sHudMessage = string.Format("%1 Unlocked!", zoneText);
		m_sSoundString = m_sHudMessage;
		PlaySound();
		m_bGameStarted = true;
		UpdateClients();
		Replication.BumpMe();
		
		GetGame().GetCallqueue().Remove(StartGame);
		
		GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void UpdateZones()
	{
		CRF_SafestartGameModeComponent safestart = CRF_SafestartGameModeComponent.GetInstance();
		if(safestart.GetSafestartStatus() || !SCR_BaseGameMode.Cast(GetGame().GetGameMode()).IsRunning() || !m_bGameStarted)
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
			
			GetGame().GetWorld().QueryEntitiesBySphere(zone.GetOrigin(), 75, ProcessEntity, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT); // get all entitys withing a 50m radius around the zone
			
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
				UpdateClients();
				Replication.BumpMe();
			};
		}
		
		if(!m_bAnyZonesBeingCaptured)
			m_iWhichZoneCaptureIsInProgress = -1;
		
		if (m_sHudMessage == string.Format("%1 Victory!", m_sBluforSideNickname) || m_sHudMessage == string.Format("%1 Victory!", m_sOpforSideNickname))
		{
			foreach(int index, string zoneName : m_aZoneObjectNames)
			{
				string status = m_aZonesStatus[index];
			
				array<string> zoneStatusArray = {};
				status.Split(":", zoneStatusArray, false);
		
				m_aZonesStatus.Set(index, string.Format("%1:%2:%3", zoneStatusArray[0], "Locked", zoneStatusArray[0]));
			}
			
			UpdateClients();
			Replication.BumpMe();
			return;
		}
		
		
		if(zonesCapturedBlufor == m_aZoneObjectNames.Count() && zonesFullyCapturedBlufor == m_aZoneObjectNames.Count())
		{
			m_iTimeToWin = m_iTimeToWin - 1;
			
			if(m_iTimeToWin <= 0)
			{
				m_sHudMessage = string.Format("%1 Victory!", m_sBluforSideNickname);
				m_sSoundString = m_sHudMessage;
				PlaySound();
			} else {
				m_sHudMessage = string.Format("%1 Victory In: %2", m_sBluforSideNickname, SCR_FormatHelper.FormatTime(m_iTimeToWin));
			};

			Replication.BumpMe();
			return;
		};
		
		if(zonesCapturedOpfor == m_aZoneObjectNames.Count() && zonesFullyCapturedOpfor == m_aZoneObjectNames.Count())
		{
			m_iTimeToWin = m_iTimeToWin - 1;

			if(m_iTimeToWin <= 0)
			{
				m_sHudMessage = string.Format("%1 Victory!", m_sOpforSideNickname);
				m_sSoundString = m_sHudMessage;
				PlaySound();
			} else {
				m_sHudMessage = string.Format("%1 Victory In: %2", m_sOpforSideNickname, SCR_FormatHelper.FormatTime(m_iTimeToWin));
			};

			Replication.BumpMe();
			return;
		};
		
		if(zonesFullyCapturedBlufor != m_aZoneObjectNames.Count() && zonesFullyCapturedOpfor != m_aZoneObjectNames.Count())
			m_iTimeToWin = 180;
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
		 
		m_iWhichZoneCaptureIsInProgress = zoneIndex;
		m_aZonesStatus.Set(zoneIndex, string.Format("%1:%2:%3", side, zoneState.ToInt() + 1, zoneFactionStored));
		
		UpdateClients();
		Replication.BumpMe();
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
		m_sSoundString = m_sHudMessage;
		PlaySound();
		
		Replication.BumpMe();
		GetGame().GetCallqueue().CallLater(ResetMessage, 7250);
		
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
		
		GetGame().GetCallqueue().CallLater(UnlockZoneDelay, 7650, false, zoneIndex, zoneIndexToChange); // need to allow players to read m_sHudMessage
		UpdateClients();
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void UnlockZoneDelay(int zoneIndex, int zoneIndexTwo)
	{	
		GetGame().GetCallqueue().CallLater(UnlockZone, 1000, true, zoneIndex, zoneIndexTwo);
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
				if(zoneText != zoneTextTwo)
					m_sHudMessage = zoneText + " & " + zoneTextTwo + " are now unlocked!";
				else
					m_sHudMessage = zoneText + " is now unlocked!";
			
				m_sSoundString = m_sHudMessage;
				GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
				PlaySound();
			};
			
			UpdateClients();
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(UnlockZone);
			return;
		};

		if (checkIfTimerArray.Count() <= 3)
		{
			switch (m_iZoneUnlockTimeIteratorInt)
			{
				case 1200: {
					if(zoneText != zoneTextTwo)
						m_sHudMessage = zoneText + " & " + zoneTextTwo + " unlock in 20 minute(s)";
					else
						m_sHudMessage = zoneText + " unlocks in 20 minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sSoundString = m_sHudMessage;
					PlaySound();
					break;
				};
				case 900: {
				if(zoneText != zoneTextTwo)
						m_sHudMessage = zoneText + " & " + zoneTextTwo + " unlock in 15 minute(s)";
					else
						m_sHudMessage = zoneText + " unlocks in 15 minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sSoundString = m_sHudMessage;
					PlaySound();
					break;
				};
				case 600: {
					if(zoneText != zoneTextTwo)
						m_sHudMessage = zoneText + " & " + zoneTextTwo + " unlock in 10 minute(s)";
					else
						m_sHudMessage = zoneText + " unlocks in 10 minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sSoundString = m_sHudMessage;
					PlaySound();
					break;
				};
				case 300: {
					if(zoneText != zoneTextTwo)
						m_sHudMessage = zoneText + " & " + zoneTextTwo + " unlock in 5 minute(s)";
					else
						m_sHudMessage = zoneText + " unlocks in 5 minute(s)";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sSoundString = m_sHudMessage;
					PlaySound();
					break;
				}
				case 60: {
					if(zoneText != zoneTextTwo)
						m_sHudMessage = zoneText + " & " + zoneTextTwo + " unlock in 1 minute!";
					else
						m_sHudMessage = zoneText + " unlocks in 1 minute";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sSoundString = m_sHudMessage;
					PlaySound();
					break;
				}
				case 15: {
					if(zoneText != zoneTextTwo)
						m_sHudMessage = zoneText + " & " + zoneTextTwo + " unlock in 15 seconds!";
					else
						m_sHudMessage = zoneText + " unlocks in 15 seconds!";
					GetGame().GetCallqueue().CallLater(ResetMessage, 10000);
					m_sSoundString = m_sHudMessage;
					PlaySound();
					break;
				}
			}
			Replication.BumpMe();
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
			m_sHudMessage = "";
			Replication.BumpMe();
		};
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateClients()
	{
		CRF_GameModePlayerComponent.GetInstance().UpdateMapMarkers(m_aZonesStatus, m_aZoneObjectNames, m_BluforSide, m_OpforSide);
	}
	
	//------------------------------------------------------------------------------------------------
	void PlaySound()
	{
		AudioSystem.PlaySound("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav");
	};
};
