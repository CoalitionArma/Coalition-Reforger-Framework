class CRF_GamemodeClass : SCR_BaseGameModeClass
{
}

enum CRF_EGamemodeState
{
	BRIEFING,
	SLOTTING,
	GAME,
	AAR
}

enum CRF_ESlottingState
{
	LEADERSANDMEDICS,
	SPECIALTIES,
	EVERYONE
}

[BaseContainerProps()]
class CRF_MissionDescriptor
{
	[Attribute("")]
	string m_sTitle;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sTextData;

	[Attribute("")]
	ref array<string> m_aFactionKeys;

	[Attribute("")]
	bool m_bShowForAnyFaction;
}

class CRF_Gamemode : SCR_BaseGameMode
{
	[RplProp(onRplName: "OnGamemodeStateChanged")]
	int m_GamemodeState = CRF_EGamemodeState.BRIEFING;

	[RplProp()]
	int m_SlottingState = CRF_ESlottingState.LEADERSANDMEDICS;

	[Attribute("45", "auto", "Mission Time (set to -1 to disable)", category: "CRF Gamemode General")]
	int m_iTimeLimitMinutes;

	[Attribute("false", "auto", "Only works with BLUFOR, OPFOR, INDFOR. Players will hear enemy radio chatter but may not talk on the enemies net", category: "CRF Gamemode General")]
	bool m_bAllowEspionage;

	[Attribute("false", "auto", "Enables AI autonomy while in GAME state", category: "CRF Gamemode General")]
	bool EnableAIInGameState;

	[Attribute("true", "auto", "Should we delete all JIP slots after SafeStart turns off? COOP = FALSE", category: "CRF Gamemode General")]
	bool m_bDeleteJIPSlots;

	[Attribute("true", "auto", "If safestart turns on instantly after the lobby screen.", category: "CRF Gamemode General")]
	bool m_bSafestartInstantlyEnabled;

	//Descriptions on the left in briefing
	[Attribute("", category: "CRF Gamemode General")]
	ref	array<ref CRF_MissionDescriptor> m_aMissionDescriptors;

	//This just is what is auto set in the slotting UI for ratio calculation
	[Attribute("1", "auto", "", category: "CRF Gamemode Slotting")]
	int m_iFactionOneRatio;

	//This just is what is auto set in the slotting UI for ratio calculation
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Gamemode Slotting")]
	string m_sFactionOneKey;

	//This just is what is auto set in the slotting UI for ratio calculation
	[Attribute("1", "auto", "", category: "CRF Gamemode Slotting")]
	int m_iFactionTwoRatio;

	//This just is what is auto set in the slotting UI for ratio calculation
	[Attribute("", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("", ""), ParamEnum("BLU", "BLU"), ParamEnum("OPF", "OPF"), ParamEnum("IND", "IND"), ParamEnum("CIV", "CIV")}, category: "CRF Gamemode Slotting")]
	string m_sFactionTwoKey;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all blufor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_BLUFORGearScriptSettings;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all opfor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_OPFORGearScriptSettings;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all indfor players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_INDFORGearScriptSettings;

	[Attribute("", UIWidgets.Auto, desc: "Gearscript applied to all civ players", category: "CRF Gamemode Gearscript")]
	ref CRF_GearScriptContainer m_CIVILIANGearScriptSettings;

	// Respawn Settings
	[Attribute("0", "auto", "", category: "CRF Gamemode Respawn")]
	bool m_bRespawnEnabled;

	[Attribute("0", "auto", "", category: "CRF Gamemode Respawn")]
	bool m_bWaveRespawn;

	[Attribute("300", UIWidgets.EditBox, "Time To Respawn in Seconds", category: "CRF Gamemode Respawn")]
	int m_iTimeToRespawn;

	[Attribute("-1", UIWidgets.EditBox, "Amount of BLUFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iBLUFORTickets;

	[Attribute("-1", UIWidgets.EditBox, "Amount of OPFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iOPFORTickets;

	[Attribute("-1", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iINDFORTickets;

	[Attribute("-1", UIWidgets.EditBox, "Amount of INDFOR Tickets. 0 = disabled/-1 = unlimited", category: "CRF Gamemode Respawn"), RplProp()]
	int m_iCIVTickets;

	//Just stores a generic spawnpoint for players to spawn the spectator cam on. Cause of entities being streamable and such.
	[RplProp()]
	vector m_vGenericSpawn[4];
	
	protected ref ScriptInvoker m_OnStateChanged;
	protected static ref SCR_PlayerData m_PlayerData;

	//------------------------------------------------------------------------------------------------
	static CRF_Gamemode GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_Gamemode.Cast(gameMode);
		else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if (RplSession.Mode() == RplMode.Dedicated)
			CRF_ModeratorConfig.LoadConfig();
	}
	
	//Advances the slotting state
	//------------------------------------------------------------------------------------------------
	void AdvanceSlottingState()
	{
		m_SlottingState += 1;
		Replication.BumpMe();
	}

	//Advances the overall gamemode state
	//------------------------------------------------------------------------------------------------
	void AdvanceGamemodeState(bool overriden = false)
	{
		if ((m_GamemodeState == CRF_EGamemodeState.AAR || m_GamemodeState == CRF_EGamemodeState.GAME) && !overriden)
			return;

		m_GamemodeState += 1;
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}

	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnStateChanged()
	{
		if (!m_OnStateChanged)
			m_OnStateChanged = new ScriptInvoker();

		return m_OnStateChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	protected override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		
		int delay = 150;
		if(m_GamemodeState != CRF_EGamemodeState.GAME)
		{
			entity.GetTransform(m_vGenericSpawn);
			delay = 2000;
		}
		
		// Early return conditions
		if (GetGame().InPlayMode() && RplSession.Mode() != RplMode.Client && entity && entity.GetPrefabData())
			// Schedule gear setup with delay
			GetGame().GetCallqueue().CallLater(CRF_GearscriptManager.GetInstance().SetupAddGearToEntity, delay, false, entity, entity.GetPrefabData().GetPrefabName());
	}

	//------------------------------------------------------------------------------------------------
	protected override void OnControllableDestroyed(IEntity entity, IEntity killerEntity, notnull Instigator instigator)
	{
		super.OnControllableDestroyed(entity, killerEntity, instigator);

		if (RplSession.Mode() == RplMode.Client)
			return;

		SCR_InstigatorContextData instigatorContextData = new SCR_InstigatorContextData(-1, entity, killerEntity, instigator);

		int playerId = instigatorContextData.GetVictimPlayerID();
		
		if (playerId <= 0 || instigatorContextData.GetVictimCharacterControlType() == SCR_ECharacterControlType.POSSESSED_AI)
			return;

		int delay = 2000;
		if (CRF_GamemodeManager.IsSpectator(entity))
			delay = 0;

		// If respawn is enabled
		if (m_bRespawnEnabled && !CRF_GamemodeManager.IsSpectator(entity) && m_GamemodeState != CRF_EGamemodeState.AAR)
		{
			string faction = SCR_FactionManager.SGetPlayerFaction(playerId).GetFactionKey();

			if (CRF_RespawnManager.GetInstance().TicketsRemaining(faction))
			{
				// Remove tickets if used
				CRF_RespawnManager.GetInstance().SubtractTicket(faction);

				// Put them in death screen/timer screen
				GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().SendRespawnScreen, (delay + 150), false, playerId);
			} else
				CRF_SlottingManager.GetInstance().UpdateSlotDeathState(CRF_SlottingManager.GetInstance().GetCharacterSlotID(entity), true);
		} else
			CRF_SlottingManager.GetInstance().UpdateSlotDeathState(CRF_SlottingManager.GetInstance().GetCharacterSlotID(entity), true);

		//Throw em into spectator
		GetGame().GetCallqueue().CallLater(CRF_GamemodeManager.GetInstance().EnterSpectator, delay, false, playerId, entity);
	}

	//Puts the player into an entity when they connect
	//------------------------------------------------------------------------------------------------
	protected override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);
		
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		if(m_GamemodeState == CRF_EGamemodeState.BRIEFING || m_GamemodeState == CRF_EGamemodeState.SLOTTING || m_GamemodeState == CRF_EGamemodeState.AAR)
			CRF_GamemodeManager.GetInstance().InitilizePlayer(iPlayerID);
		
		string playerIdentity = GetGame().GetBackendApi().GetPlayerIdentityId(iPlayerID);
		
		if (!playerIdentity.IsEmpty() && CRF_ModeratorConfig.IsModerator(playerIdentity))
			GetGame().GetCallqueue().CallLater(CRF_GamemodeManager.GetInstance().SetPlayerModerator, 5000, false, iPlayerID);
	};
	
	//------------------------------------------------------------------------------------------------
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);

		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually.
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);
		//Updates connection status
		if (CRF_SlottingManager.GetInstance().IsPlayerInASlot(SCR_PlayerController.GetLocalPlayerId()))
		{
			CRF_SlottingManager.GetInstance().SlottingChangesUpdate();
		}
	}
	
	protected void OnDataReceived(SCR_PlayerData playerData)
	{
		m_PlayerData = playerData;
		m_PlayerData.CalculateStatsChange();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGamemodeStateChanged()
	{
		if (RplSession.Mode() == RplMode.Dedicated || RplSession.Mode() == RplMode.Listen)
		{
			if (m_OnStateChanged)
				m_OnStateChanged.Invoke();

			if (m_GamemodeState == CRF_EGamemodeState.AAR)
				EnterAAR();
		}
		else
			CRF_PlayerControllerComponent.GetInstance().OpenCurrentStateMenu();
	}

	//------------------------------------------------------------------------------------------------
	protected void EnterAAR()
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach (int player : players)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;

			if (CRF_GamemodeManager.IsSpectator(GetGame().GetPlayerManager().GetPlayerControlledEntity(player)))
				continue;

			HitZone defaultHitZone = SCR_CharacterDamageManagerComponent.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(player).FindComponent(SCR_CharacterDamageManagerComponent)).GetDefaultHitZone();
			
			if(defaultHitZone)
				defaultHitZone.SetHealth(0);

			// Log player data
			if (!m_PlayerData)
			{
				SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
				if (!dataCollector)
				{
					Print ("SCR_CareerEndScreenUI: No data collector was found.", LogLevel.ERROR);
					return;
				}
		
				m_PlayerData = dataCollector.GetPlayerData(player, false);
		
				//If there's still no player data, we wait for the invoker on data received to let us now that we got the instance through rpl
				if (!m_PlayerData)
				{
					SCR_DataCollectorCommunicationComponent communicationComponent = SCR_DataCollectorCommunicationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(player).FindComponent(SCR_DataCollectorCommunicationComponent));
					if (communicationComponent)
						communicationComponent.GetOnDataReceived().Insert(OnDataReceived);
				}
				else if (!m_PlayerData.IsDataProgressionReady())
					m_PlayerData.CalculateStatsChange();
			}
		}
	}
}

//Ditto the RL Devs WHY IS THIS HARDCODED
modded class SCR_ManualCamera
{
	//------------------------------------------------------------------------------------------------
	override protected bool IsDisabledByMenu()
	{
		if (!m_MenuManager)
			return false;

		if (m_MenuManager.IsAnyDialogOpen())
			return true;

		MenuBase topMenu = m_MenuManager.GetTopMenu();

		// WHY IT'S HARDCODED?
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(CRF_SpectatorMenuUI));
	}
}
