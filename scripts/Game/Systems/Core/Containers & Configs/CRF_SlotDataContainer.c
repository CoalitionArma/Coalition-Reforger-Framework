class CRF_SlotDataContainer
{		
	protected int m_iSlotId;
	protected int m_iSlotCurrentPlayerId;
	protected CRF_EGearRole m_SlotRole;
	protected CRF_EFactions m_SlotFaction;
	protected RplId m_iSlotCurrentGroup = RplId.Invalid();
	protected RplId m_iSlotCurrentCharacter = RplId.Invalid();
	protected bool m_bIsLockedSlot = false;
	
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
			SetSlotFactionKey(newSlotData.GetSlotFactionKey());
			SetIsLockedSlot(newSlotData.GetIsLockedSlot());
			
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
	void SetSlotFactionKey(CRF_EFactions faction)
	{
		// Dirty flag check: only update if value actually changed
		if (m_SlotFaction == faction)
			return;
		
		m_SlotFaction = faction;
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
	CRF_EFactions GetSlotFactionKey()
	{
		return m_SlotFaction;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIsLockedSlot()
	{
		return m_bIsLockedSlot;
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
	        && snapshot.Compare(instance.m_bIsLockedSlot, 4);
	}
}