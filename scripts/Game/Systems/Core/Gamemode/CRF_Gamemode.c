class CRF_GamemodeClass : SCR_BaseGameModeClass
{
}

enum CRF_GamemodeState
{
	INITIAL,
	SLOTTING,
	GAME,
	AAR
}

enum CRF_SlottingState
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
	int m_GamemodeState = CRF_GamemodeState.INITIAL;

	[RplProp()]
	int m_SlottingState = CRF_SlottingState.LEADERSANDMEDICS;

	//Slot ID given to an entity
	[RplProp()]
	ref array<int> m_aSlots = {};

	//Slot name of entity
	[RplProp()]
	ref array<string> m_aSlotNames = {};

	//Player that is slotted in the entities name
	[RplProp()]
	ref array<string> m_aSlotPlayerNames = {};

	//Slot icon off of UI Info
	[RplProp()]
	ref array<ResourceName> m_aSlotIcons = {};

	[RplProp()]
	ref array<ResourceName> m_aSlotPrefabs = {};

	//Entities slot type, leader, specialty, everyone
	[RplProp()]
	ref array<int> m_aEntitySlotTypes = {};

	//Is the entity dead or alive, needed as entities pop in and out of streamable.
	[RplProp()]
	ref array<bool> m_aEntityDeathStatus = {};

	[RplProp()]
	ref array<RplId> m_aCharacters = {};

	[RplProp()]
	ref array<string> m_aCharacterNames = {};

	//RplId of entities that are playable
	[RplProp()]
	ref array<RplId> m_aEntitySlots = {};

	//Stores the group ID for each slot, so I can reference what group a slot is in. CAUSE THERE IS NO WAY TO DO THAT ON THE CLIENT.
	[RplProp()]
	ref array<RplId> m_aPlayerGroupIDs = {};

	//Communicates change across all clients so they can refresh their slots in the UI
	[RplProp()]
	int m_iSlotChanges = 0;

	//Is a group locked
	[RplProp()]
	ref array<bool> m_aGroupLockedStatus = {};

	//Stores SCR_AIGroup RplId, CAUSE YOU CAN'T FUCKING GRAB NON PLAYABLE GROUPS BOHEMIA
	[RplProp()]
	ref array<RplId> m_aGroupRplIDs = {};

	//Stores the playable group created whenever an AI group is created in the editor
	[RplProp()]
	ref array<RplId> m_aActivePlayerGroupsIDs = {};

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
	protected ref array<CRF_GamemodeManager> m_aAdditionalCRFGamemodeManagers = {};
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
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		GetGame().GetCallqueue().CallLater(LogCharacter, 500, false, entity);
	}

	//------------------------------------------------------------------------------------------------
	void LogCharacter(IEntity entity)
	{
		#ifdef WORKBENCH
		if (!SCR_ChimeraCharacter.Cast(entity))
				return;
			m_aCharacters.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
			if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)))
			{
				if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName())
					m_aCharacterNames.Insert(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName());
				else
					m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			}
			else
				m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			Replication.BumpMe();
		#else
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			if (!SCR_ChimeraCharacter.Cast(entity))
				return;
			m_aCharacters.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
			if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)))
			{
				if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName())
					m_aCharacterNames.Insert(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName());
				else
					m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			}
			else
				m_aCharacterNames.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetDisplayName());
			Replication.BumpMe();
		}
		#endif
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		array<Managed> additionalComponents = {};
		int count = owner.FindComponents(CRF_GamemodeManager, additionalComponents);
		m_aAdditionalCRFGamemodeManagers.Clear();
		for (int i = 0; i < count; i++)
		{
			CRF_GamemodeManager comp = CRF_GamemodeManager.Cast(additionalComponents[i]);
			m_aAdditionalCRFGamemodeManagers.Insert(comp);
		}

		if (m_bRespawnEnabled)
			CRF_RespawnManager.GetInstance().InitilizeRespawns();

		SCR_AIGroup.GetOnPlayerAdded().Insert(OnPlayerJoinedGroup);
		SCR_AIGroup.GetOnPlayerRemoved().Insert(OnPlayerLeftGroup);
		
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
		if (m_bRespawnEnabled && entity.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et" && m_GamemodeState != CRF_GamemodeState.AAR)
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
		if (m_GamemodeState == CRF_GamemodeState.GAME)
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

	//Initializes group into the replicated arrays
	//------------------------------------------------------------------------------------------------
	void AddGroup(SCR_AIGroup group)
	{
		m_aGroupRplIDs.Insert(RplComponent.Cast(group.FindComponent(RplComponent)).Id());
		m_aGroupLockedStatus.Insert(false);
		SCR_AIGroup newGroup = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(group.GetFaction());
		newGroup.SetCanDeleteIfNoPlayer(false);
		newGroup.SetMaxMembers(12);
		m_aActivePlayerGroupsIDs.Insert(RplComponent.Cast(newGroup.FindComponent(RplComponent)).Id());
		Replication.BumpMe();
	}

	//Sets the group to be locked
	//------------------------------------------------------------------------------------------------
	void SetGroupLockedStatus(int index, bool input)
	{
		m_aGroupLockedStatus.Set(index, input);
	}

	//Sets slot to player or removes him from it
	//------------------------------------------------------------------------------------------------
	void SetSlot(int index, int playerId)
	{
		if (playerId > 0)
		{
			SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(FactionAffiliationComponent.Cast(RplComponent.Cast(Replication.FindItem(m_aEntitySlots.Get(index))).GetEntity().FindComponent(FactionAffiliationComponent)).GetAffiliatedFaction());
			m_aSlotPlayerNames.Set(index, GetGame().GetPlayerManager().GetPlayerName(playerId));
		}
		else
		{
			if (m_aSlots.Get(index) > 0)
			{
				SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(m_aSlots.Get(index)).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));
				m_aSlotPlayerNames.Set(index, "");
			}
		}
		m_aSlots.Set(index, playerId);
		m_iSlotChanges++;
		Replication.BumpMe();
	}

	//Sets if an entity is dead or not in the array
	//------------------------------------------------------------------------------------------------
	void SetDeathState(IEntity entity, bool input)
	{
		if (entity.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
		{
			m_aEntityDeathStatus.Set(m_aEntitySlots.Find(RplComponent.Cast(entity.FindComponent(RplComponent)).Id()), input);
			m_iSlotChanges++;
			Replication.BumpMe();
		};
	}

	//Initializes playable entities and adds their values into the replicated arrays
	//------------------------------------------------------------------------------------------------
	int AddPlayableEntity(IEntity entity)
	{
		int index = m_aSlots.Insert(0);
		m_aEntitySlots.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
		m_aPlayerGroupIDs.Insert(RplComponent.Cast(SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(entity.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup()).FindComponent(RplComponent)).Id());
		m_aSlotNames.Insert(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName());
		m_aSlotPrefabs.Insert(entity.GetPrefabData().GetPrefabName());
		m_aSlotIcons.Insert(SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent)).GetInfo().GetIconPath());
		m_aEntityDeathStatus.Insert(false);
		m_aSlotPlayerNames.Insert("");

		if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).IsLeader())
			m_aEntitySlotTypes.Insert(0);
		else if (CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).IsSpecialty())
			m_aEntitySlotTypes.Insert(1);
		else
			m_aEntitySlotTypes.Insert(2);

		if (m_aSlots.Count() == 1)
			entity.GetTransform(m_vGenericSpawn);

		return index;

		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void RemovePlayableEntity(RplId entityID)
	{
		if (!Replication.FindItem(entityID) || SCR_PossessingManagerComponent.GetInstance().GetIdFromMainEntity(RplComponent.Cast(Replication.FindItem(entityID)).GetEntity()) != 0)
			return;

		int index = m_aEntitySlots.Find(entityID);
		m_aSlots.RemoveOrdered(index);
		m_aPlayerGroupIDs.RemoveOrdered(index);
		m_aSlotNames.RemoveOrdered(index);
		m_aSlotIcons.RemoveOrdered(index);
		m_aSlotPrefabs.RemoveOrdered(index);
		m_aEntityDeathStatus.RemoveOrdered(index);
		m_aSlotPlayerNames.RemoveOrdered(index);
		m_aEntitySlotTypes.RemoveOrdered(index);
		m_aEntitySlots.RemoveOrdered(index);

		SCR_EntityHelper.DeleteEntityAndChildren(RplComponent.Cast(Replication.FindItem(entityID)).GetEntity());

		m_iSlotChanges++;

		Replication.BumpMe();
	}

	//Puts the player into an entity when they connect
	//------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
//		if(m_aSlots.Find(playerId) == -1)
//			EnterSpectator(playerId);

		if (m_aSlots.Find(playerId) != -1)
		{
			m_iSlotChanges++;
			Replication.BumpMe();
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
			m_iSlotChanges++;
			Replication.BumpMe();
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
		if ((m_GamemodeState == CRF_GamemodeState.AAR || m_GamemodeState == CRF_GamemodeState.GAME) && !overriden)
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

			if (m_GamemodeState == CRF_GamemodeState.AAR)
				EnterAAR();
		}
		else
			CRF_PlayerControllerComponent.GetInstance().OpenCurrentStateMenu();
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerJoinedGroup(SCR_AIGroup aiGroup, int playerId)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			IEntity currentLeaderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(aiGroup.GetLeaderID());
			if (!currentLeaderEntity)
				return;

			if (!CRF_Library.IsSquadLeaderRole(currentLeaderEntity))
			{
				IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
				if (!player)
					return;

				if (CRF_Library.IsSquadLeaderRole(player))
				{
					SCR_GroupsManagerComponent.GetInstance().SetGroupLeader(aiGroup.GetGroupID(), playerId);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	void OnPlayerLeftGroup(SCR_AIGroup aiGroup, int playerId)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
		{
			IEntity currentLeaderEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(aiGroup.GetLeaderID());
			if (!currentLeaderEntity)
				return;

			if (!CRF_Library.IsSquadLeaderRole(currentLeaderEntity))
			{
				array<int> groupMembers = aiGroup.GetPlayerIDs();

				foreach (int member : groupMembers)
				{
					IEntity memberEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(member);
					if (!memberEntity)
						return;

					if (CRF_Library.IsTeamLeaderRole(memberEntity))
					{
						SCR_GroupsManagerComponent.GetInstance().SetGroupLeader(aiGroup.GetGroupID(), member);
						break;
					}
				}
			}
		}
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
