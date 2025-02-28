class CRF_GamemodeClass: SCR_BaseGameModeClass
{
};

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
	ref array<string> m_aVONChannels = {"Deafen|", "Global|"};
	
	[RplProp()]
	ref array<int> m_aPlayersRegistedVON = {};
	
	[RplProp()]
	int m_iChannelChanges = 0;
	
	[RplProp()]
	int m_SlottingState = CRF_SlottingState.LEADERSANDMEDICS;
	
	//Stores when a player is talking
	[RplProp()]
	ref array<int> m_aPlayersTalking = {};
	
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
	
	[Attribute("0", "auto", "Useful for when your planning on using custom respawn triggers.", category: "CRF Gamemode Respawn")]
	bool m_bDisbleRespawnTimer;

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
	
	[RplProp(onRplName: "UpdateRespawnTimer")]
	int m_iRespawnWaveCurrentTime;

	IEntity m_eGamemodeEntity;
	protected ref ScriptInvoker m_OnStateChanged;
	protected ref array<CRF_GamemodeComponent> m_aAdditionalCRFGamemodeComponents = {};
	protected ref array<IEntity> m_aRespawnPoints = {};
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	static CRF_Gamemode GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_Gamemode.Cast(gameMode);
		else
			return null;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		GetGame().GetCallqueue().CallLater(LogCharacter, 500, false, entity);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void LogCharacter(IEntity entity)
	{
		#ifdef WORKBENCH
		if(!SCR_ChimeraCharacter.Cast(entity))
				return;
			m_aCharacters.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
			if(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)))
			{
				if(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName())
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
			if(!SCR_ChimeraCharacter.Cast(entity))
				return;
			m_aCharacters.Insert(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
			if(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)))
			{
				if(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).GetName())
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
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_eGamemodeEntity = owner;
		
		array<Managed> additionalComponents = new array<Managed>();
		int count = owner.FindComponents(CRF_GamemodeComponent, additionalComponents);
		m_aAdditionalCRFGamemodeComponents.Clear();
		for (int i = 0; i < count; i++)
		{
			CRF_GamemodeComponent comp = CRF_GamemodeComponent.Cast(additionalComponents[i]);
			m_aAdditionalCRFGamemodeComponents.Insert(comp);
		}
		
		if (m_bRespawnEnabled)
			InitilizeRespawns()
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		
		// If respawn is enabled
		if (m_bRespawnEnabled) {
			string faction = SCR_FactionManager.SGetPlayerFaction(playerId).GetFactionKey();
			
			if (TicketsRemaining(faction)) {
				// Remove tickets if used
				SubtractTicket(faction);
				
				// Put them in death screen/timer screen
				GetGame().GetCallqueue().CallLater(SendRespawnScreen, 250, false, playerId);
			}
		}
			
		//Throw em into spectator
		GetGame().GetCallqueue().CallLater(SetPlayerSpectator, 100, false, playerId, playerEntity);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetPlayerSpectator(int playerId, IEntity playerEntity)
	{
		if(playerEntity.GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			SpawnInitialEntity(playerId);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SpawnInitialEntity(int playerID)
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = "0 10000 0";
		IEntity initialEntity = GetGame().SpawnEntityPrefab(Resource.Load("{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et"),GetGame().GetWorld(),spawnParams);
		GetGame().GetCallqueue().CallLater(SetPlayerEntity, 200, false, initialEntity, playerID);
		SCR_AIGroup currentGroup = SCR_GroupsManagerComponent.GetInstance().GetPlayerGroup(playerID);
		if(currentGroup)
			currentGroup.RemovePlayer(playerID);
		SCR_CharacterDamageManagerComponent.Cast(initialEntity.FindComponent(SCR_CharacterDamageManagerComponent)).EnableDamageHandling(false);
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerID).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));
		return;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetCameraPos(int playerID, vector cameraPos[4])
	{
		RpcDo_SetCameraPos(playerID, cameraPos);
		Rpc(RpcDo_SetCameraPos, playerID, cameraPos);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	void RpcDo_SetCameraPos(int playerID, vector cameraPos[4])
	{	
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		if(!player)
			return;
		vector transform[4];
		player.GetWorldTransform(transform);
		transform[3] = cameraPos[3];

		BaseGameEntity playerEntity = BaseGameEntity.Cast(player); 
        
        if(!playerEntity)
            return;
        
        float scale = playerEntity.GetScale();
		playerEntity.Teleport(transform);
        playerEntity.SetWorldTransform(transform);
        playerEntity.SetScale(scale);
        playerEntity.Update();
        playerEntity.OnTransformReset();

		Physics phys = player.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}
	
	//Called to enter the actual game, just puts the player into a slot or spectator.
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void EnterGame(int playerID)
	{
		if(m_aSlots.Find(playerID) == -1 && GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			SpawnInitialEntity(playerID);
		
		if(m_aSlots.Find(playerID) == -1)
			return;
		
		if(m_aEntityDeathStatus.Get(m_aSlots.Find(playerID)))
		{
			SpawnInitialEntity(playerID);
			return;
		}
			
		IEntity oldEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
		RplId oldGroup = RplId.Invalid();
		if(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID).GetPrefabData().GetPrefabName() != "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
			oldGroup = m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aEntitySlots.Find(RplComponent.Cast(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID).FindComponent(RplComponent)).Id()))));
		SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerID)).SetInitialMainEntity(RplComponent.Cast(Replication.FindItem(m_aEntitySlots.Get(m_aSlots.Find(playerID)))).GetEntity());
		SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerID).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerID)))))).GetEntity()).GetFaction());
		if(oldGroup != RplId.Invalid())
		{
			if(oldGroup != m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerID)))))
				SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerID)))))).GetEntity()).AddPlayer(playerID);
		}
		else
			SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerID)))))).GetEntity()).AddPlayer(playerID);
	}
	
	//Initializes group into the replicated arrays
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AddGroup(SCR_AIGroup group)
	{
		m_aGroupRplIDs.Insert(RplComponent.Cast(group.FindComponent(RplComponent)).Id());
		m_aGroupLockedStatus.Insert(false);
		SCR_AIGroup newGroup = SCR_GroupsManagerComponent.GetInstance().CreateNewPlayableGroup(group.GetFaction());
		newGroup.SetCanDeleteIfNoPlayer(false);
		m_aActivePlayerGroupsIDs.Insert(RplComponent.Cast(newGroup.FindComponent(RplComponent)).Id());
		Replication.BumpMe();
	}
	
	//Sets the group to be locked
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetGroupLockedStatus(int index, bool input)
	{
		m_aGroupLockedStatus.Set(index, input);
	}
	
	//Sets slot to player or removes him from it
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetSlot(int index, int playerID)
	{
		if(playerID > 0)
		{
			SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(playerID).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(FactionAffiliationComponent.Cast(RplComponent.Cast(Replication.FindItem(m_aEntitySlots.Get(index))).GetEntity().FindComponent(FactionAffiliationComponent)).GetAffiliatedFaction());
			m_aSlotPlayerNames.Set(index, GetGame().GetPlayerManager().GetPlayerName(playerID));
		}
		else
		{
			if(m_aSlots.Get(index) > 0)
			{
				SCR_PlayerFactionAffiliationComponent.Cast(GetGame().GetPlayerManager().GetPlayerController(m_aSlots.Get(index)).FindComponent(SCR_PlayerFactionAffiliationComponent)).RequestFaction(GetGame().GetFactionManager().GetFactionByKey("SPEC"));
				m_aSlotPlayerNames.Set(index, "");
			}
		}
		m_aSlots.Set(index, playerID);
		m_iSlotChanges++;
		Replication.BumpMe();
	}
	
	//Sets if an entity is dead or not in the array
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetDeathState(IEntity entity, bool input)
	{
		m_aEntityDeathStatus.Set(m_aEntitySlots.Find(RplComponent.Cast(entity.FindComponent(RplComponent)).Id()), input);
		m_iSlotChanges++;
		Replication.BumpMe();
	}
	
	//Initializes playable entities and adds their values into the replicated arrays
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
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
		if(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).IsLeader())
			m_aEntitySlotTypes.Insert(0);
		else if(CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter)).IsSpecialty())
			m_aEntitySlotTypes.Insert(1);
		else
			m_aEntitySlotTypes.Insert(2);
		
		if(m_aSlots.Count() == 1)
			entity.GetTransform(m_vGenericSpawn);

		return index;
		
		Replication.BumpMe();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RemovePlayableEntity(RplId entityID)
	{
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
		if(Replication.FindItem(entityID))
			SCR_EntityHelper.DeleteEntityAndChildren(RplComponent.Cast(Replication.FindItem(entityID)).GetEntity());
		m_iSlotChanges++;
		Replication.BumpMe();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Should only ever be ran on the server
	void RespawnPlayerRplId(int playerID, string prefab, vector position, RplId groupID)
	{
		if(RplSession.Mode() != RplMode.Dedicated)
		{
			Print("ONLY RUN RespawnPlayer ON SERVER");
			return;
		}
		EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.TransformMode = ETransformMode.WORLD;
		vector finalSpawnLocation = vector.Zero;
		SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, position, 10);
        spawnParams.Transform[3] = finalSpawnLocation;
		IEntity newEntity = GetGame().SpawnEntityPrefab(Resource.Load(prefab),GetGame().GetWorld(),spawnParams);
		GetGame().GetCallqueue().CallLater(RespawnPlayerRplIdDelay, 100, false, playerID, groupID, newEntity);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RespawnPlayerRplIdDelay(int playerID, RplId groupID, IEntity newEntity)
	{
		SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aGroupRplIDs.Get(m_aActivePlayerGroupsIDs.Find(RplComponent.Cast(playerGroup.FindComponent(RplComponent)).Id())))).GetEntity());
		aiGroup.AddAIEntityToGroup(newEntity);
		int index = AddPlayableEntity(newEntity);
		SetSlot(m_aSlots.Find(playerID), -2);
		SetSlot(index, playerID);
		Rpc(RpcDo_EnterGame, playerID);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Respawn Ticket System
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void InitilizeRespawns()
	{
		m_iRespawnWaveCurrentTime = m_iTimeToRespawn;
		
		if (m_bWaveRespawn && RplSession.Mode() == RplMode.Dedicated)
		{
			GetGame().GetCallqueue().CallLater(UpdateRespawnTimer, 1000, true);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	bool TicketsRemaining(string faction)
	{
		bool result = false;
		switch (faction)
		{
			case "BLUFOR" : {
				if (m_iBLUFORTickets > 0 || m_iBLUFORTickets == -1) 
					result = true;
				break;
			};
			case "OPFOR" : {
				if (m_iOPFORTickets > 0 || m_iOPFORTickets == -1)
					result = true;
				break;
			}
			case "INDFOR" : {
				if (m_iINDFORTickets > 0 || m_iINDFORTickets == -1)
					result = true;
				break;
			}
			case "CIV" : {
				if (m_iCIVTickets > 0 || m_iCIVTickets == -1)
					result = true;
				break;
			}
		}
		return result;
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SubtractTicket(string faction)
	{
		switch (faction)
		{
			case "BLUFOR" : {
				if (m_iBLUFORTickets > 0 && m_iBLUFORTickets != -1 && !CRF_GamemodeComponent.GetInstance().GetSafestartStatus())
					m_iBLUFORTickets = m_iBLUFORTickets - 1;
					break;
			}
			case "OPFOR" : {
				if (m_iOPFORTickets > 0 && m_iOPFORTickets != -1 && !CRF_GamemodeComponent.GetInstance().GetSafestartStatus())
					m_iOPFORTickets = m_iOPFORTickets - 1;
					break;
			}
			case "INDFOR" : {
				if (m_iINDFORTickets > 0 && m_iINDFORTickets != -1 && !CRF_GamemodeComponent.GetInstance().GetSafestartStatus())
					m_iINDFORTickets = m_iINDFORTickets - 1;	
					break;
			}
			case "CIV" : {
				if (m_iCIVTickets > 0 && m_iCIVTickets != -1 && !CRF_GamemodeComponent.GetInstance().GetSafestartStatus())
					m_iCIVTickets = m_iCIVTickets - 1;
					break;
			}
		}
		Replication.BumpMe();
	}
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UpdateRespawnTimer()
	{
		if (m_GamemodeState != CRF_GamemodeState.GAME && !m_bDisbleRespawnTimer)
			return;	
		
		if (m_iRespawnWaveCurrentTime == 0)
		{
			m_iRespawnWaveCurrentTime = m_iTimeToRespawn;
		}
		m_iRespawnWaveCurrentTime--;
		Replication.BumpMe();
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RegisterRespawnPoint(IEntity respawnPoint)
	{
		m_aRespawnPoints.Insert(respawnPoint);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void UnRegisterRespawnPoint(IEntity respawnPoint)
	{
		if(m_aRespawnPoints.Find(respawnPoint) != -1)
			m_aRespawnPoints.Remove(m_aRespawnPoints.Find(respawnPoint));
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SendRespawnScreen(int playerId)
	{
		Rpc(RpcDo_SendRespawnScreen, playerId)	
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRespawnScreen(int playerId)
	{
		if(SCR_PlayerController.GetLocalPlayerId() != playerId)
			return;
		
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();

		GetGame().GetMenuManager().CloseAllMenus();
		MenuBase respawnMenu = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_RespawnMenu);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RespawnSide(string faction)
	{
		array<int> allPlayers = {};
		GetGame().GetPlayerManager().GetAllPlayers(allPlayers);
		
		foreach(int player : allPlayers)
		{
			if(!m_aSlots.Contains(player))
				continue;
			
			RplId groupID = m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(player))));
			SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aGroupRplIDs.Get(m_aActivePlayerGroupsIDs.Find(RplComponent.Cast(playerGroup.FindComponent(RplComponent)).Id())))).GetEntity());
			string playerFactionKey = aiGroup.GetFaction().GetFactionKey();
			
			// Make sure the player is still in that faction 
			if (playerFactionKey != faction)
				continue;
			
			// If tickets are enabled by MM
			if (TicketsRemaining(faction)) { // Always true if tickets > 0 or tickets == -1
				RespawnPlayer(player);
				SubtractTicket(faction); // only subtract if tickets > 0
			}
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RespawnPlayer(int playerId)
	{
		Rpc(RpcDo_RespawnPlayer, playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcDo_RespawnPlayer(int playerID)
	{
		if (RplSession.Mode() != RplMode.Dedicated)
		{
			Print("ONLY RUN RespawnSide ON SERVER");
			return; 
		}
		
		if (SCR_FactionManager.SGetPlayerFaction(playerID).GetFactionKey() == "SPEC")
		{	
			string respawnPrefab = CRF_GamemodeComponent.GetInstance().ReturnPlayerGearScriptsMapValue(playerID, "GSR");
			
			RplId groupID = m_aActivePlayerGroupsIDs.Get(m_aGroupRplIDs.Find(m_aPlayerGroupIDs.Get(m_aSlots.Find(playerID))));
			SCR_AIGroup playerGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(groupID)).GetEntity());
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(m_aGroupRplIDs.Get(m_aActivePlayerGroupsIDs.Find(RplComponent.Cast(playerGroup.FindComponent(RplComponent)).Id())))).GetEntity());
			string faction = aiGroup.GetFaction().GetFactionKey();
			
			if(respawnPrefab.IsEmpty())
			{
				switch(faction)
				{
					case "BLUFOR" 	: {respawnPrefab = "{6F99DE8453E6B423}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Rifleman_P.et"; 	break;}
					case "OPFOR"  	: {respawnPrefab = "{FC0904D71EF8DB6A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Rifleman_P.et";   	break;}
					case "INDFOR" 	: {respawnPrefab = "{A303C25424BC7149}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Rifleman_P.et";	break;}
					case "CIV" 		: {respawnPrefab = "{2046F9D64B1221F1}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_1SG_P.et";				break;}
				}
			}
			
			vector finalSpawnLocation = vector.Zero;
			EntitySpawnParams spawnParams = new EntitySpawnParams();
			spawnParams.TransformMode = ETransformMode.WORLD;
			
			vector spawnLocation = vector.Zero;
			IEntity newEntity = GetGame().SpawnEntityPrefab(Resource.Load(respawnPrefab),GetGame().GetWorld(),spawnParams);
			
			foreach(IEntity spawnPoint : m_aRespawnPoints)
			{
				if(!spawnPoint || CRF_RespawnPointComponent.Cast(spawnPoint.FindComponent(CRF_RespawnPointComponent)).m_sRespawnPointFaction != faction || !CRF_RespawnPointComponent.Cast(spawnPoint.FindComponent(CRF_RespawnPointComponent)).m_bActiveRespawnPoint || spawnLocation != vector.Zero)
					continue;
				
				spawnLocation = spawnPoint.GetOrigin();
			};
			
			Resource resource = Resource.Load(respawnPrefab);
			SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, spawnLocation, 10);
			RespawnPlayerRplId(playerID, respawnPrefab, finalSpawnLocation, groupID);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_EnterGame(int playerID)
	{
		if(SCR_PlayerController.GetLocalPlayerId() != playerID)
			return;
		
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).EnterGame(playerID);
	}
	
	//Puts the player into an entity when they connect
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
//		if(m_aSlots.Find(playerId) == -1)
//			SpawnInitialEntity(playerId);

		if(m_aSlots.Find(playerId) != -1)
		{
			m_iSlotChanges++;
			Replication.BumpMe();
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		m_OnPlayerDisconnected.Invoke(playerId, cause, timeout);
		
		// RespawnSystemComponent is not a SCR_BaseGameModeComponent, so for now we have to
		// propagate these events manually. 
		if (IsMaster())
			m_pRespawnSystemComponent.OnPlayerDisconnected_S(playerId, cause, timeout);

		foreach (SCR_BaseGameModeComponent comp : m_aAdditionalGamemodeComponents)
		{
			comp.OnPlayerDisconnected(playerId, cause, timeout);
		}
		
		m_OnPostCompPlayerDisconnected.Invoke(playerId, cause, timeout);
		//Updates connection status
		if(m_aSlots.Find(playerId) != -1)
		{
			m_iSlotChanges++;
			Replication.BumpMe();
		}
	}
	
	// Rush game mode respawn - no tickets
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RushRespawnPlayers()
	{
		// Stubbed for old mission support 
		RespawnSide("BLUFOR");
		RespawnSide("INDFOR");
		RespawnSide("OPFOR");
	}
	
	//Sets if the player is talking for UI purposes
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetPlayerTalking(int playerID)
	{
		if(m_aPlayersTalking.Find(playerID) != -1)
			return;
		
		m_aPlayersTalking.Insert(playerID);
		Replication.BumpMe();
	}
	
	//Sets if the player is talking for UI purposes
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RemovePlayerTalking(int playerID)
	{
		m_aPlayersTalking.RemoveItem(playerID);
		Replication.BumpMe();
	}
	
	//Advances the slotting state
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AdvanceSlottingState()
	{
		m_SlottingState += 1;
		m_iSlotChanges++;
		Replication.BumpMe();
	}
	
	//Opens the menu on the player
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void OpenMenu()
	{
		//Close any menu that wriggles its way in
		MenuBase topMenu = GetGame().GetMenuManager().GetTopMenu();
		if (topMenu)
			topMenu.Close();
		GetGame().GetMenuManager().CloseAllMenus();
		//Opens menu based on current game state : )
		switch(m_GamemodeState)
		{
			case CRF_GamemodeState.INITIAL: 	{GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_PreviewMenu);											break;}
			case CRF_GamemodeState.SLOTTING:	{GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_SlottingMenu);											break;}
			case CRF_GamemodeState.GAME: 		{SCR_PlayerController.Cast(GetGame().GetPlayerController()).EnterGame(SCR_PlayerController.GetLocalPlayerId());		break;}
			case CRF_GamemodeState.AAR: 		{GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_AARMenu);												break;}
		}
		if(m_GamemodeState != CRF_GamemodeState.GAME)
		{
			BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
			if(SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_iFPS)
				video.Get("MaxFps", SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_iFPS);
			video.Set("MaxFps", 30);
			GetGame().UserSettingsChanged();
			if(SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_iAudioSetting)
				SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
			AudioSystem.SetMasterVolume(AudioSystem.SFX, 0);
		}
	}
	
	
	//Sets the players entity, just used for a delay while the entity spawns
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetPlayerEntity(IEntity entity, int playerId)
	{
		IEntity oldEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId)).SetInitialMainEntity(entity);	
	}

	//Advances the overall gamemode state
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AdvanceGamemodeState()
	{
		if(m_GamemodeState == CRF_GamemodeState.AAR)
			return;
		
		m_GamemodeState += 1;
		Replication.BumpMe();
		OnGamemodeStateChanged();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnStateChanged()
	{
		if (!m_OnStateChanged)
			m_OnStateChanged = new ScriptInvoker();

		return m_OnStateChanged;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void OnGamemodeStateChanged()
	{
		if(RplSession.Mode() == RplMode.Dedicated)
		{
			if (m_OnStateChanged)
				m_OnStateChanged.Invoke(m_GamemodeState);
			
			foreach (CRF_GamemodeComponent component : m_aAdditionalCRFGamemodeComponents)
				component.OnGamemodeStateChanged();
			
			if(m_GamemodeState == CRF_GamemodeState.AAR)
				EnterAAR();
		}
		else
			OpenMenu();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void EnterAAR()
	{
		ref array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach(int player: players)
		{
			if(!GetGame().GetPlayerManager().IsPlayerConnected(player))
				continue;
			
			if(GetGame().GetPlayerManager().GetPlayerControlledEntity(player).GetPrefabData().GetPrefabName() == "{59886ECB7BBAF5BC}Prefabs/Characters/CRF_InitialEntity.et")
				continue;
			
			GetGame().GetCallqueue().CallLater(SpawnInitialEntity, 500, false, player);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void SetChannel(int index, string inputString, bool channelCreation)
	{
		m_aVONChannels.Set(index, inputString);
		if (!channelCreation)
		{
			foreach(string channel: m_aVONChannels)
			{
				ref array<string> channelSplit = {};
				channel.Split("|", channelSplit, true);
				if (channelSplit.Count() == 1 && m_aVONChannels.Find(channel) > 1)
					m_aVONChannels.RemoveOrdered(m_aVONChannels.Find(channel));
			}
		}
		m_iChannelChanges++;
		Replication.BumpMe();
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	bool IsPlayerInAnyChannel(int playerId, out int channelId)
	{
		bool isInChannel = false;
		channelId = -1;
		foreach(string channel: m_aVONChannels)
		{
			if(isInChannel)
				break;
			ref array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			ref array<string> players = {};
			if (channelSplit.Count() == 1)
				continue;
			channelSplit.Get(1).Split(",", players, true);
			foreach(string player: players)
			{
				if	(player == "")
					continue;
				if (player.ToInt() == playerId)
				{
					channelId = m_aVONChannels.Find(channel);
					isInChannel = true;
					break;
				}
			}
		}
		return isInChannel;	
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void AddPlayerToChannel(int playerId, int channelIndex, bool channelCreation)
	{
		int index;
		if (IsPlayerInAnyChannel(playerId, index))
			RemovePlayerFromAnyChannel(playerId, channelCreation);
		ref array<string> channelSplit = {};
		m_aVONChannels.Get(channelIndex).Split("|", channelSplit, true);
		ref array<string> players = {};
		if (channelSplit.Count() > 1)
			channelSplit.Get(1).Split(",", players, true);
		players.Insert(playerId.ToString());
		if (channelSplit.Count() > 1)
			channelSplit.Set(1, SCR_StringHelper.Join("," , players));
		else
			channelSplit.Insert(SCR_StringHelper.Join("," , players));
		SetChannel(channelIndex, SCR_StringHelper.Join("|", channelSplit), channelCreation);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void RemovePlayerFromAnyChannel(int playerId, bool channelCreation)
	{
		int index;
		if(!IsPlayerInAnyChannel(playerId, index))
			return;
		ref array<string> channelSplit = {};
		m_aVONChannels.Get(index).Split("|", channelSplit, true);
		ref array<string> players = {};
		if (channelSplit.Count() > 1)
			channelSplit.Get(1).Split(",", players, true);
		players.RemoveOrdered(players.Find(playerId.ToString()));
		if (channelSplit.Count() > 1)
			channelSplit.Set(1, SCR_StringHelper.Join("," , players));
		else
			channelSplit.Insert(SCR_StringHelper.Join("," , players));
		SetChannel(index, SCR_StringHelper.Join("|", channelSplit), channelCreation);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	bool IsPlayerInChannel(int playerId, int index)
	{
		ref array<string> channelSplit = {};
		m_aVONChannels.Get(index).Split("|", channelSplit, true);
		ref array<string> players = {};
		if (channelSplit.Count() == 1)
			return false;
		else
			channelSplit.Get(1).Split(",", players, true);
		if (players.Contains(playerId.ToString()))
			return true;
		else
			return false;	
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	int CreateChannel(string name, int playerId)
	{
		int index = m_aVONChannels.Insert(name + "|");
		AddPlayerToChannel(playerId, index, true);
		m_iChannelChanges++;
		Replication.BumpMe();
		return index;
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	int GetChannel(int playerId)
	{
		foreach(string channel: m_aVONChannels)
		{
			ref array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			ref array<string> players = {};
			if (channelSplit.Count() == 1)
				continue;
			else
				channelSplit.Get(1).Split(",", players, true);
			if (players.Contains(playerId.ToString()))
				return m_aVONChannels.Find(channel);
			else
				continue;
		}
		return 1;
	}
	
	void RequestToJoinChannel(int channel, int requestId)
	{
		ref array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach(int player: players)
		{
			if (IsPlayerInChannel(player, channel))
				Rpc(RpcDo_SendRequest, player, requestId, channel);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_SendRequest(int playerId, int requestId, int channel)
	{
		if (playerId != SCR_PlayerController.GetLocalPlayerId())
			return;
		
		CRF_SpectatorMenuUI specMenu = CRF_SpectatorMenuUI.Cast(GetGame().GetMenuManager().GetTopMenu());
		
		if (!specMenu)
			return;
		Widget compWidget = GetGame().GetWorkspace().CreateWidgets("{49490337615BA9B8}UI/Listbox/VONChannelRequestListBox.layout", specMenu.m_wRoot.FindAnyWidget("Requests"));
		specMenu.m_aRequest.Insert(compWidget);
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(compWidget.FindHandler(CRF_ListBoxElementComponent));
		comp.m_iPlayerId = requestId;
		comp.m_iChannelId = channel;
		comp.GetAccept().m_OnClicked.Insert(Accept);
		comp.GetDeny().m_OnClicked.Insert(Deny);
		FrameSlot.SetPosX(compWidget.FindAnyWidget("ButtonAnim"), 500);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Accept()
	{
		if (!WidgetManager.GetWidgetUnderCursor())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent())
			return;		
		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent().FindHandler(CRF_ListBoxElementComponent));
		SCR_PlayerController.Cast(GetGame().GetPlayerController()).Accept(comp.m_iPlayerId, comp.m_iChannelId);
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	void Deny()
	{
		if (!WidgetManager.GetWidgetUnderCursor())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent())
			return;
		else if (!WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent())
			return;		
		CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(WidgetManager.GetWidgetUnderCursor().GetParent().GetParent().GetParent().GetParent().GetParent().FindHandler(CRF_ListBoxElementComponent));

		ref array<int> players = {};
		GetGame().GetPlayerManager().GetAllPlayers(players);
		foreach(int player: players)
		{
			if (IsPlayerInChannel(player, comp.m_iChannelId))
				Rpc(RpcDo_Deny, player, comp.m_iPlayerId);
		}
	}
	
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_Deny(int playerId, int requestId)
	{
		if (playerId != SCR_PlayerController.GetLocalPlayerId())
			return;
		
		CRF_SpectatorMenuUI specMenu = CRF_SpectatorMenuUI.Cast(GetGame().GetMenuManager().GetTopMenu());
		
		if (!specMenu)
			return;
		
		foreach (Widget request: specMenu.m_aRequest)
		{
			CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(request.FindHandler(CRF_ListBoxElementComponent));
			if (!comp)
				continue;
			
			if (requestId == comp.m_iPlayerId)
			{
				comp.GetAccept().m_OnClicked.Clear();
				comp.GetDeny().m_OnClicked.Clear();
				comp.m_bDeleteRequest = true;
				return;			
			}
		}
	}
}

//Ditto the RL Devs WHY IS THIS HARDCODED
modded class SCR_ManualCamera
{
	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	override protected bool IsDisabledByMenu()
	{
		if (!m_MenuManager) return false;
		
		if (m_MenuManager.IsAnyDialogOpen()) return true;
		
		MenuBase topMenu = m_MenuManager.GetTopMenu();
		
		// WHY IT'S HARDCODED?
		return topMenu && (!topMenu.IsInherited(EditorMenuUI) && !topMenu.IsInherited(CRF_SpectatorMenuUI));
	}
};