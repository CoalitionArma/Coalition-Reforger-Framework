enum CRF_ESlotType
{
	REGULAR = 0,
	LEADERORMEDIC,
	SPECIALTY,
}

class CRF_SlotData
{
	vector m_vSlotVector[4];
	
	ResourceName m_rSlotResource;
	
	int m_iSlotCurrentPlayerId;
	
	FactionKey m_SlotFactionKey;
	
	RplId m_iSlotCurrentGroup;
	
	RplId m_iSlotCurrentCharacter;
	
	ref CRF_SlotUIData m_SlotUIData;
	
	/*!
		Serialize this class using provided ScriptBitWriter.
	*/
	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteResourceName(m_rSlotResource);
		writer.WriteInt(m_iSlotCurrentPlayerId);
		writer.WriteString(m_SlotFactionKey);
		writer.WriteRplId(m_iSlotCurrentGroup);
		writer.WriteRplId(m_iSlotCurrentCharacter);
		writer.Write(m_SlotUIData, 64);
		
		return true;
	}
	
	/*!
		Deserialize this class using provided ScriptBitWriter.
	*/
    bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadResourceName(m_rSlotResource);
		reader.ReadInt(m_iSlotCurrentPlayerId);
		reader.ReadString(m_SlotFactionKey);
		reader.ReadRplId(m_iSlotCurrentGroup);
		reader.ReadRplId(m_iSlotCurrentCharacter);
		reader.Read(m_SlotUIData, 64);
		
		return true;
	}	
	
	//################################################################################################
	//! Codec methods
	//------------------------------------------------------------------------------------------------
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet) 
	{
		snapshot.Serialize(packet, 12);
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot) 
	{
		return snapshot.Serialize(packet, 12);
	}
	
	//------------------------------------------------------------------------------------------------
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx) 
	{		
		return lhs.CompareSnapshots(rhs, 12);
	}

	//------------------------------------------------------------------------------------------------
	static bool PropCompare(CRF_SlotData prop, SSnapSerializerBase snapshot, ScriptCtx ctx) 
	{
		return snapshot.Compare(prop.m_rSlotResource, 4)
			&& snapshot.Compare(prop.m_iSlotCurrentPlayerId, 4)
			&& snapshot.Compare(prop.m_SlotFactionKey, 4)
			&& snapshot.Compare(prop.m_iSlotCurrentGroup, 4)
			&& snapshot.Compare(prop.m_iSlotCurrentCharacter, 4)
			&& snapshot.Compare(prop.m_SlotUIData, 24);
	}
			
	//------------------------------------------------------------------------------------------------
	static bool Extract(CRF_SlotData prop, ScriptCtx ctx, SSnapSerializerBase snapshot) 
	{
		snapshot.SerializeBytes(prop.m_rSlotResource, 4);
		snapshot.SerializeBytes(prop.m_iSlotCurrentPlayerId, 4);
		snapshot.SerializeBytes(prop.m_SlotFactionKey, 4);
		snapshot.SerializeBytes(prop.m_iSlotCurrentGroup, 4);
		snapshot.SerializeBytes(prop.m_iSlotCurrentCharacter, 4);
		snapshot.SerializeBytes(prop.m_SlotUIData, 24);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, CRF_SlotData prop) 
	{
		snapshot.SerializeBytes(prop.m_rSlotResource, 4);
		snapshot.SerializeBytes(prop.m_iSlotCurrentPlayerId, 4);
		snapshot.SerializeBytes(prop.m_SlotFactionKey, 4);
		snapshot.SerializeBytes(prop.m_iSlotCurrentGroup, 4);
		snapshot.SerializeBytes(prop.m_iSlotCurrentCharacter, 4);
		snapshot.SerializeBytes(prop.m_SlotUIData, 8);
		return true;
	}
}

class CRF_SlotUIData
{
	string m_sSlotName;

	ResourceName m_rSlotIconResource;
	
	CRF_ESlotType m_iSlotType;
	
	bool m_bIsLockedSlot;
	
	bool m_bIsDeadSlot;
}

class CRF_SlottingManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_SlottingManager : SCR_BaseGameModeComponent
{
	
	// INT in this map works on a "ID" based system where a ID is generated for every slot that is created in the AddSlot function bellow.
	// CRF_SlotData is then stored in this map for further use by the relevant systems or to be updated later when applicable.
	protected ref map<int, CRF_SlotData> m_mSlotsMap = new map<int, CRF_SlotData>;
	
	// Cannot replicate maps, so we use this array to replicate all map keys (in correllation with the map data array bellow).
	// Is also a really easy way to update clients the slots map has changed.
	[RplProp(onRplName: "UpdateClientSlotsMap")]
	protected ref array<int> m_aSlotsKey = {}; 
	
	// Cannot replicate maps, so we use this array to replicate all map data (in correllation with the map key array above).
	[RplProp()]
	protected ref array<CRF_SlotData> m_aSlotsData = {}; 
	
	// Latest Slot ID that was used to create a slot
	protected int m_iLatestSlotID;
	
	// Script Invoker for all your invoker needs
	protected ref ScriptInvoker m_OnSlottingUpdate;
	
	//------------------------------------------------------------------------------------------------
	static CRF_SlottingManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_SlottingManager.Cast(gameMode.FindComponent(CRF_SlottingManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSlottingUpdate()
	{
		if (!m_OnSlottingUpdate)
			m_OnSlottingUpdate = new ScriptInvoker();

		return m_OnSlottingUpdate;
	}
	
	//Updates all players the slotting information has changed
	//------------------------------------------------------------------------------------------------
	void SlottingChangesUpdate()
	{
		// Create a temp array so we arent broadcasting for each change to m_aPlayerArray.
		protected ref array<int> tempSlotsKey = {};
		protected ref array<CRF_SlotData> tempSlotsData = {};

		// Fill tempSlotsKey/tempSlotsData with all keys and values in m_mSlotsMap.
		for (int i = 0; i < m_mSlotsMap.Count(); i++)
		{
			int key = m_mSlotsMap.GetKey(i);
			CRF_SlotData value = m_mSlotsMap.Get(key);
			
			tempSlotsKey.Insert(key);
			tempSlotsData.Insert(value);
		};

		// Replicate m_aSlotsKey/m_aSlotsData to all clients.
		m_aSlotsKey = tempSlotsKey;
		m_aSlotsData = tempSlotsData;
		Replication.BumpMe();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateClientSlotsMap()
	{
		if(m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
		
		foreach (int i, int slotID : m_aSlotsKey)
			m_mSlotsMap.Set(slotID, m_aSlotsData.Get(i));
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotData GetSlotData(int slotId)
	{
		return m_mSlotsMap.Get(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	map<int,CRF_SlotData> GetSlotMap()
	{
		return m_mSlotsMap;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		array<int> tempArray = {};
		
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentGroup == rplId)
				tempArray.Insert(slotID);
		}
		
		return tempArray;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotData GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentCharacter == rplId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPlayerSlotID(int playerId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotData GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		return SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(GetPlayerSlotData(playerId).m_iSlotCurrentGroup)).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	IEntity GetPlayerSlotCharacter(int playerId)
	{
		return IEntity.Cast(RplComponent.Cast(Replication.FindItem(GetPlayerSlotData(playerId).m_iSlotCurrentCharacter)).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId)
	{
		return GetGame().GetFactionManager().GetFactionByKey(GetPlayerSlotData(playerId).m_SlotFactionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPlayerSlotResource(int playerId)
	{
		return GetPlayerSlotData(playerId).m_rSlotResource;
	}
	
	//------------------------------------------------------------------------------------------------
	void GetPlayerSlotVector(int playerId, out vector vec[])
	{
		vec = GetPlayerSlotData(playerId).m_vSlotVector;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetCharacterSlotID(IEntity entity)
	{
		RplId rplId = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
		
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentCharacter == rplId)
				return slotID;
		}
		
		return 0;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		CRF_SlotData slotData = GetPlayerSlotData(playerId);
		if(slotData)
		{
			CRF_SlotUIData slotUiData = slotData.m_SlotUIData;
			return slotUiData.m_bIsDeadSlot;
		} else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		CRF_SlotData slotData = m_mSlotsMap.Get(slotId);
		CRF_SlotUIData slotUIData = slotData.m_SlotUIData;
		slotUIData.m_bIsLockedSlot = input;
		
		SlottingChangesUpdate();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotData slotData = m_mSlotsMap.Get(slotId);
		CRF_SlotUIData slotUIData = slotData.m_SlotUIData;
		slotUIData.m_bIsDeadSlot = input;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		CRF_SlotData slotData = m_mSlotsMap.Get(slotId);
		slotData.m_iSlotCurrentPlayerId = playerId;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId groupId)
	{
		CRF_SlotData slotData = m_mSlotsMap.Get(slotId);
		slotData.m_iSlotCurrentGroup = groupId;
		m_mSlotsMap.Set(slotId, slotData);
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		CRF_SlotData slotData = m_mSlotsMap.Get(slotId);
		slotData.m_rSlotResource = resource;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		CRF_SlotData slotData = m_mSlotsMap.Get(slotId);
		slotData.m_iSlotCurrentCharacter = charId;
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void LockAllOpenSlots()
	{
		foreach (int slotID, CRF_SlotData slotData : m_mSlotsMap)
		{
			if(slotData.m_iSlotCurrentPlayerId != 0)
				continue;
			else
				slotData.m_SlotUIData.m_bIsLockedSlot = true;
		}
		
		SlottingChangesUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void AddPlayableEntity(IEntity entity)
	{
		if(RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter));
		
		if(!playableCharComp || !playableCharComp.IsPlayable())
			return;
		
		SCR_EditableCharacterComponent editableCharComp = SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent));
		SCR_AIGroup group = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(entity.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
		
		CRF_SlotData slotData = new CRF_SlotData;
		
		if(group)
		{
			slotData.m_iSlotCurrentGroup = RplComponent.Cast(group.FindComponent(RplComponent)).Id();
			slotData.m_SlotFactionKey = group.GetFaction().GetFactionKey();
		} else 
			slotData.m_SlotFactionKey = SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent)).GetAffiliatedFactionKey();
		
		entity.GetWorldTransform(slotData.m_vSlotVector);
		slotData.m_rSlotResource = entity.GetPrefabData().GetPrefabName();
		slotData.m_iSlotCurrentCharacter = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
		
		CRF_SlotUIData slotUIData = new CRF_SlotUIData;
		
		if (!playableCharComp.GetName().IsEmpty())
			slotUIData.m_sSlotName = playableCharComp.GetName();
		else
			slotUIData.m_sSlotName = editableCharComp.GetDisplayName();	
		
		slotUIData.m_rSlotIconResource = editableCharComp.GetInfo().GetIconPath();
		slotUIData.m_iSlotType = playableCharComp.GetSlottingRole();
		
		slotData.m_SlotUIData = slotUIData;
		
		m_iLatestSlotID++;
		m_mSlotsMap.Set(m_iLatestSlotID, slotData);
		
		SlottingChangesUpdate();
		
		if(CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}