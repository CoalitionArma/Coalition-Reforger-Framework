class CRF_GamemodeClass : SCR_BaseGameModeClass
{
}

enum CRF_EGamemodeState
{
	INITIAL,
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
	int m_GamemodeState = CRF_EGamemodeState.INITIAL;

	[RplProp()]
	int m_SlottingState = CRF_ESlottingState.LEADERSANDMEDICS;

	//Just stores a generic spawnpoint for players to spawn the spectator cam on. Cause of entities being streamable and such.
	[RplProp()]
	vector m_vGenericSpawn[4];

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
		if (entity.GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			delay = 0;

		// If respawn is enabled
		if (m_bRespawnEnabled && entity.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" && m_GamemodeState != CRF_EGamemodeState.AAR)
		{
			string faction = SCR_FactionManager.SGetPlayerFaction(playerId).GetFactionKey();

			if (CRF_RespawnManager.GetInstance().TicketsRemaining(faction))
			{
				// Remove tickets if used
				CRF_RespawnManager.GetInstance().SubtractTicket(faction);

				// Put them in death screen/timer screen
				GetGame().GetCallqueue().CallLater(CRF_RplBroadcastManager.GetInstance().SendRespawnScreen, (delay + 150), false, playerId);
			}
		}

		//Throw em into spectator
		GetGame().GetCallqueue().CallLater(EnterSpectator, delay, false, playerId, entity);
	}

	//------------------------------------------------------------------------------------------------
	void EnterSpectator(int playerId, IEntity entity = null)
	{
		IEntity initialEntity = GetGame().SpawnEntityPrefab(Resource.Load("{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et"), GetGame().GetWorld());
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));

		GetGame().GetCallqueue().CallLater(pc.SetInitialMainEntity, 250, false, initialEntity);

		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerId);
		if (currentGroup)
			currentGroup.RemovePlayer(playerId);
		
		SCR_CharacterDamageManagerComponent damManager = SCR_CharacterDamageManagerComponent.Cast(initialEntity.FindComponent(SCR_CharacterDamageManagerComponent)); 
		if(damManager)
			damManager.EnableDamageHandling(false);
		
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));

		vector cameraPos[4];
		if (m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			if (m_aSlots.Find(playerId) != -1 && entity != null)
			{
				entity.GetWorldTransform(cameraPos);
				cameraPos[3][1] = cameraPos[3][1] + 1.5;
			} else
				cameraPos[3] = m_vGenericSpawn[3];
		} else
			cameraPos[3] = "0 10000 0";

		CRF_RplBroadcastManager.GetInstance().SendSpecClientInit(playerId, cameraPos);
	}

	//Called to enter the actual game, just puts the player into a slot or spectator.
	//------------------------------------------------------------------------------------------------
	void InitilizePlayer(int playerId)
	{
		if (m_aSlots.Find(playerId) == -1 
			|| GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" 
			|| m_aEntityDeathStatus.Get(m_aSlots.Find(playerId))) {
				EnterSpectator(playerId);
				return;
		}

		// WHAT THE FUCK IS THISSSSSSSSSSSSSSS
		RplId oldGroup = RplId.Invalid();
		if (GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			oldGroup = m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aEntitySlots.Find(RplComponent.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId).FindComponent(RplComponent)).Id()))));

		SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId)).SetInitialMainEntity(RplComponent.Cast(Replication.FindItem(m_aEntitySlots.Get(m_aSlots.Find(playerId)))).GetEntity());

		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerId)))))).GetEntity()).GetFaction());

		int groupId = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerId)))))).GetEntity()).GetGroupID();

		if (oldGroup != RplId.Invalid())
		{
			if (oldGroup != m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerId)))))
			{
				SCR_GroupsManagerComponent.GetInstance().AddPlayerToGroup(groupId, playerId);
				SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
			}
		} else {
			SCR_GroupsManagerComponent.GetInstance().AddPlayerToGroup(groupId, playerId);
			SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(playerId).RequestJoinGroup(groupId);
		}
		
		CRF_RplBroadcastManager.GetInstance().InitilizePlayer(playerId);
	}

	//Puts the player into an entity when they connect
	//------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);

		if (m_aSlots.Find(playerId) != -1)
		{
			CRF_GamemodeManager.GetInstance().SlottingChangesUpdate();
		}
	}

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
		if (m_aSlots.Find(playerId) != -1)
		{
			CRF_GamemodeManager.GetInstance().SlottingChangesUpdate();
		}
	}

	//Advances the slotting state
	//------------------------------------------------------------------------------------------------
	void AdvanceSlottingState()
	{
		m_SlottingState += 1;
		m_iSlotChanges++;
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
	void OnGamemodeStateChanged()
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
	void EnterAAR()
	{
		array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach (int player : players)
		{
			if (!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;

			if (GetGame().GetPlayerManager().GetPlayerControlledEntity(player).GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
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

	protected void OnDataReceived(SCR_PlayerData playerData)
	{
		m_PlayerData = playerData;
		m_PlayerData.CalculateStatsChange();
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
