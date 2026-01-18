class CRF_SlotDataContainer
{		
	protected int m_iSlotId;
	protected int m_iSlotCurrentPlayerId;
	protected CRF_EGearRole m_SlotRole;
	protected CRF_EFactions m_SlotFaction;
	protected RplId m_iSlotCurrentGroup = RplId.Invalid();
	protected RplId m_iSlotCurrentCharacter = RplId.Invalid();
	protected bool m_bIsLockedSlot = false;
	protected bool m_bIsDeadSlot = false;
	
	// Invoker for data updates
	protected ref ScriptInvoker m_OnDataUpdate;
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Replaces or sets the internal CRF_SlotDataContainer record for the slot.
	 * If newData is non-null, the slot's data is updated with the provided instance.
	 *
	 * @param slotID: ID of the slot whose data should be updated.
	 * @param newData: Pointer/reference to the new CRF_SlotDataContainer to apply.
	 */
	void DataUpdate(CRF_SlotDataContainer newSlotData = null)
	{	
		if(newSlotData)	
		{
			SetSlotId(newSlotData.GetSlotId());	
			SetSlotCurrentPlayerId(newSlotData.GetSlotCurrentPlayerId());
			SetSlotCurrentGroup(newSlotData.GetSlotCurrentGroup());
			SetSlotCurrentCharacter(newSlotData.GetSlotCurrentCharacter());
			SetSlotFactionEnum(newSlotData.GetSlotFactionEnum());
			SetIsLockedSlot(newSlotData.GetIsLockedSlot());
			SetIsDeadSlot(newSlotData.GetIsDeadSlot());
			
			if (m_OnDataUpdate)
				m_OnDataUpdate.Invoke();
		};
	}
	
	//------------------------------------------------------------------------------------------------
	// SCRIPT INVOKER
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnDataUpdate()
	{
		if (!m_OnDataUpdate)
			m_OnDataUpdate = new ScriptInvoker();

		return m_OnDataUpdate;
	}
	
	//------------------------------------------------------------------------------------------------
	// SETTERS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void SetSlotId(int slotId)
	{
		m_iSlotId = slotId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentPlayerId(int playerId)
	{
		// Dirty flag check: only update if value actually changed
		if (m_iSlotCurrentPlayerId == playerId)
			return;
		
		m_iSlotCurrentPlayerId = playerId;
		
		if (playerId <= 0)
				CRF_SlottingManager.GetInstance().CleanupCharacterFromSlot(this);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentGroup(RplId groupRplId)
	{
		// Dirty flag check: only update if value actually changed
		if (m_iSlotCurrentGroup == groupRplId)
			return;
		
		m_iSlotCurrentGroup = groupRplId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentCharacter(RplId characterRplId)
	{
		// Dirty flag check: only update if value actually changed
		if (m_iSlotCurrentCharacter == characterRplId)
			return;
		
		m_iSlotCurrentCharacter = characterRplId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotFactionKey(FactionKey faction)
	{
		CRF_EFactions factionEnum;
		switch (faction)
		{
			case "BLUFOR" 	: factionEnum = CRF_EFactions.BLUFOR; break;
			case "OPFOR" 	: factionEnum = CRF_EFactions.OPFOR; break;
			case "INDFOR" 	: factionEnum = CRF_EFactions.INDFOR; break;
			case "CIV" 		: factionEnum = CRF_EFactions.CIV; break;
		}
		
		// Dirty flag check: only update if value actually changed
		if (m_SlotFaction == factionEnum)
			return;
		
		m_SlotFaction = factionEnum;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotFactionEnum(CRF_EFactions factionEnum)
	{
		// Dirty flag check: only update if value actually changed
		if (m_SlotFaction == factionEnum)
			return;
		
		m_SlotFaction = factionEnum;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsLockedSlot(bool lockedState)
	{
		// Dirty flag check: only update if value actually changed
		if (m_bIsLockedSlot == lockedState)
			return;
		
		m_bIsLockedSlot = lockedState;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsDeadSlot(bool deadState)
	{
		// Dirty flag check: only update if value actually changed
		if (m_bIsDeadSlot == deadState)
			return;
		
		m_bIsDeadSlot = deadState;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotRole(CRF_EGearRole role)
	{
		// Dirty flag check: only update if value actually changed
		if (m_SlotRole == role)
			return;
		
		m_SlotRole = role;
	}
	
	//------------------------------------------------------------------------------------------------
	// GETTERS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	int GetSlotId()
	{
		return m_iSlotId;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSlotCurrentPlayerId()
	{
		return m_iSlotCurrentPlayerId;
	}
	
	//------------------------------------------------------------------------------------------------
	RplId GetSlotCurrentGroup()
	{
		if(!m_iSlotCurrentGroup || m_iSlotCurrentGroup == RplId.Invalid())
			return RplId.Invalid();
		else
			return m_iSlotCurrentGroup;
	}
	
	//------------------------------------------------------------------------------------------------
	RplId GetSlotCurrentCharacter()
	{
		if(!m_iSlotCurrentCharacter || m_iSlotCurrentGroup == RplId.Invalid())
			return RplId.Invalid();
		else
			return m_iSlotCurrentCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	FactionKey GetSlotFactionKey()
	{
		FactionKey faction;
		switch (m_SlotFaction)
		{
			case CRF_EFactions.BLUFOR 	: faction = "BLUFOR"; break;
			case CRF_EFactions.OPFOR 	: faction = "OPFOR"; break;
			case CRF_EFactions.INDFOR 	: faction = "INDFOR"; break;
			case CRF_EFactions.CIV 		: faction = "CIV"; break;
		}
		
		return faction;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_EFactions GetSlotFactionEnum()
	{
		return m_SlotFaction;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIsLockedSlot()
	{
		return m_bIsLockedSlot;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIsDeadSlot()
	{
		return m_bIsDeadSlot;
	}
	
	string GetSlotName() 
	{
		string customSlottingName = CRF_GearscriptManager.GetInstance().GetCustomRoleName(GetSlotFactionKey(), m_SlotRole);
		
		if (customSlottingName.IsEmpty())
			return CRF_GamemodeManager.RolesConfig().FindRoleConfig(m_SlotRole).m_sRoleName;
		else
			return customSlottingName;
	}
	
	CRF_ESlotType GetSlotType() 
	{
		return CRF_GamemodeManager.RolesConfig().FindRoleConfig(m_SlotRole).m_SlottingType;
	}
	
	ResourceName GetSlotIconResource() 
	{
		return CRF_GamemodeManager.RolesConfig().FindRoleConfig(m_SlotRole).m_RoleIcon;
	}
	
	ResourceName GetSlotResource() 
	{
		ref CRF_RoleConfig config = CRF_GamemodeManager.RolesConfig().FindRoleConfig(m_SlotRole);
		ResourceName resource;
		
		switch (GetSlotFactionEnum())
		{
			case CRF_EFactions.BLUFOR : resource = config.m_BluforVariant; break;
			case CRF_EFactions.OPFOR : resource = config.m_OpforVariant; break;
			case CRF_EFactions.INDFOR : resource = config.m_IndforVariant; break;
			case CRF_EFactions.CIV : resource = config.m_CivVariant; break;
		}
		
		return resource;
	}
	
	//------------------------------------------------------------------------------------------------
	// REPLICATION STUFF
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// REPLICATION STUFF
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void Save(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iSlotId);
		writer.WriteInt(m_iSlotCurrentPlayerId);
		writer.WriteInt(m_SlotRole);
		writer.WriteInt(m_SlotFaction);
		writer.WriteRplId(m_iSlotCurrentGroup);
		writer.WriteRplId(m_iSlotCurrentCharacter);
		writer.WriteBool(m_bIsLockedSlot);
		writer.WriteBool(m_bIsDeadSlot);
	}
	
	void Load(ScriptBitReader reader)
	{
		reader.ReadInt(m_iSlotId);
		reader.ReadInt(m_iSlotCurrentPlayerId);
		reader.ReadInt(m_SlotRole);
		reader.ReadInt(m_SlotFaction);
		reader.ReadRplId(m_iSlotCurrentGroup);
		reader.ReadRplId(m_iSlotCurrentCharacter);
		reader.ReadBool(m_bIsLockedSlot);
		reader.ReadBool(m_bIsDeadSlot);
	}
	
	static bool Extract(CRF_SlotDataContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeBytes(instance.m_iSlotId, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentPlayerId, 4);
		snapshot.SerializeBytes(instance.m_SlotRole, 4);
		snapshot.SerializeBytes(instance.m_SlotFaction, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentGroup, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentCharacter, 4);
	    snapshot.SerializeBytes(instance.m_bIsLockedSlot, 4);
		snapshot.SerializeBytes(instance.m_bIsDeadSlot, 4);
	    return true;
	}
	
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, CRF_SlotDataContainer instance)
	{
		snapshot.SerializeBytes(instance.m_iSlotId, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentPlayerId, 4);
		snapshot.SerializeBytes(instance.m_SlotRole, 4);
		snapshot.SerializeBytes(instance.m_SlotFaction, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentGroup, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentCharacter, 4);
	    snapshot.SerializeBytes(instance.m_bIsLockedSlot, 4);
		snapshot.SerializeBytes(instance.m_bIsDeadSlot, 4);
	    return true;
	}
	
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);
	}
	
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);
	    return true;
	}
	
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
	    return lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4);
	}
	
	static bool PropCompare(CRF_SlotDataContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
	    return snapshot.Compare(instance.m_iSlotId, 4)
		&& snapshot.Compare(instance.m_iSlotCurrentPlayerId, 4)
		&& snapshot.Compare(instance.m_SlotRole, 4)
		&& snapshot.Compare(instance.m_SlotFaction, 4)
		&& snapshot.Compare(instance.m_iSlotCurrentGroup, 4)
		&& snapshot.Compare(instance.m_iSlotCurrentCharacter, 4)
		&& snapshot.Compare(instance.m_bIsLockedSlot, 4)
		&& snapshot.Compare(instance.m_bIsDeadSlot, 4);
	}
}