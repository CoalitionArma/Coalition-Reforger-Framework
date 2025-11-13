class CRF_SlotDataContainer
{	
	protected vector m_vSlotVectorOne;
	protected vector m_vSlotVectorTwo;
	protected vector m_vSlotVectorThree;
	protected vector m_vSlotVectorFour;
	
	protected int m_iSlotId;
	protected int m_iSlotCurrentPlayerId;
	protected RplId m_iSlotCurrentGroup = RplId.Invalid();
	protected RplId m_iSlotCurrentCharacter = RplId.Invalid();
	protected CRF_ESlotType m_iSlotType = CRF_ESlotType.GENERAL_INFANTRY;
	
	protected string m_sSlotName;
	protected ResourceName m_rSlotIconResource;
	protected ResourceName m_rSlotResource;
	protected FactionKey m_SlotFactionKey;
	
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
			
			vector vec[4];
			newSlotData.GetSlotVector(vec);
			SetSlotVector(vec);
			
			SetSlotCurrentPlayerId(newSlotData.GetSlotCurrentPlayerId());
			SetSlotCurrentGroup(newSlotData.GetSlotCurrentGroup());
			SetSlotCurrentCharacter(newSlotData.GetSlotCurrentCharacter());
			SetSlotType(newSlotData.GetSlotType());
			SetSlotName(newSlotData.GetSlotName());
			SetSlotIcon(newSlotData.GetSlotIconResource());
			SetSlotResource(newSlotData.GetSlotResource());
			SetSlotFactionKey(newSlotData.GetSlotFactionKey());
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
	void SetSlotVector(vector tempVec[4])
	{
		m_vSlotVectorOne = tempVec[0];
		m_vSlotVectorTwo = tempVec[1];
		m_vSlotVectorThree = tempVec[2];
		m_vSlotVectorFour = tempVec[3];
	}	
	
	//------------------------------------------------------------------------------------------------
	void SetSlotId(int slotId)
	{
		m_iSlotId = slotId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentPlayerId(int playerId)
	{
		m_iSlotCurrentPlayerId = playerId;
		
		if (playerId <= 0)
				CRF_SlottingManager.GetInstance().CleanupCharacterFromSlot(this);
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentGroup(RplId groupRplId)
	{
		m_iSlotCurrentGroup = groupRplId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentCharacter(RplId characterRplId)
	{
		m_iSlotCurrentCharacter = characterRplId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotType(CRF_ESlotType slotType)
	{
		m_iSlotType = slotType;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotName(string name)
	{
		m_sSlotName = name;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotIcon(ResourceName icon)
	{
		m_rSlotIconResource = icon;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotResource(ResourceName resource)
	{
		m_rSlotResource = resource;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotFactionKey(FactionKey faction)
	{
		m_SlotFactionKey = faction;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsLockedSlot(bool lockedState)
	{
		m_bIsLockedSlot = lockedState;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsDeadSlot(bool deadState)
	{
		m_bIsDeadSlot = deadState;
	}
	
	//------------------------------------------------------------------------------------------------
	// GETTERS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void GetSlotVector(out vector vec[4])
	{
		vec[0] = m_vSlotVectorOne;
		vec[1] = m_vSlotVectorTwo;
		vec[2] = m_vSlotVectorThree;
		vec[3] = m_vSlotVectorFour;
	}
	
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
	CRF_ESlotType GetSlotType()
	{
		return m_iSlotType;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetSlotName()
	{
		if(!m_sSlotName)
			return "Invalid Name";
		else
			return m_sSlotName;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetSlotIconResource()
	{
		return m_rSlotIconResource;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetSlotResource()
	{
		return m_rSlotResource;
	}
	
	//------------------------------------------------------------------------------------------------
	FactionKey GetSlotFactionKey()
	{
		return m_SlotFactionKey;
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
	
	//------------------------------------------------------------------------------------------------
	// REPLICATION STUFF
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void Save(ScriptBitWriter writer)
	{
	    writer.WriteVector(m_vSlotVectorOne);
	    writer.WriteVector(m_vSlotVectorTwo);
	    writer.WriteVector(m_vSlotVectorThree);
	    writer.WriteVector(m_vSlotVectorFour);
	
		writer.WriteInt(m_iSlotId);
	    writer.WriteInt(m_iSlotCurrentPlayerId);
	    writer.WriteRplId(m_iSlotCurrentGroup);
	    writer.WriteRplId(m_iSlotCurrentCharacter);
	    writer.WriteInt(m_iSlotType);
	
	    writer.WriteString(m_sSlotName);
	    writer.WriteString(m_rSlotIconResource);
	    writer.WriteString(m_rSlotResource);
	    writer.WriteString(m_SlotFactionKey);
	
	    writer.WriteBool(m_bIsLockedSlot);
	    writer.WriteBool(m_bIsDeadSlot);
	}
	
	void Load(ScriptBitReader reader)
	{
	    reader.ReadVector(m_vSlotVectorOne);
	    reader.ReadVector(m_vSlotVectorTwo);
	    reader.ReadVector(m_vSlotVectorThree);
	    reader.ReadVector(m_vSlotVectorFour);
	
		reader.ReadInt(m_iSlotId);
	    reader.ReadInt(m_iSlotCurrentPlayerId);
	    reader.ReadRplId(m_iSlotCurrentGroup);
	    reader.ReadRplId(m_iSlotCurrentCharacter);
	    reader.ReadInt(m_iSlotType);
	
	    reader.ReadString(m_sSlotName);
	    reader.ReadString(m_rSlotIconResource);
	    reader.ReadString(m_rSlotResource);
	    reader.ReadString(m_SlotFactionKey);
	
	    reader.ReadBool(m_bIsLockedSlot);
	    reader.ReadBool(m_bIsDeadSlot);
	}
	
	static bool Extract(CRF_SlotDataContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
	    snapshot.SerializeBytes(instance.m_vSlotVectorOne, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorTwo, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorThree, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorFour, 12);
	
		snapshot.SerializeBytes(instance.m_iSlotId, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentPlayerId, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentGroup, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentCharacter, 4);
	    snapshot.SerializeBytes(instance.m_iSlotType, 4);
	
	    snapshot.SerializeString(instance.m_sSlotName);
	    snapshot.SerializeString(instance.m_rSlotIconResource);
	    snapshot.SerializeString(instance.m_rSlotResource);
	    snapshot.SerializeString(instance.m_SlotFactionKey);
	
	    snapshot.SerializeBytes(instance.m_bIsLockedSlot, 4);
	    snapshot.SerializeBytes(instance.m_bIsDeadSlot, 4);
	    return true;
	}
	
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, CRF_SlotDataContainer instance)
	{
	    snapshot.SerializeBytes(instance.m_vSlotVectorOne, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorTwo, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorThree, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorFour, 12);
	
		snapshot.SerializeBytes(instance.m_iSlotId, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentPlayerId, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentGroup, 4);
	    snapshot.SerializeBytes(instance.m_iSlotCurrentCharacter, 4);
	    snapshot.SerializeBytes(instance.m_iSlotType, 4);
	
	    snapshot.SerializeString(instance.m_sSlotName);
	    snapshot.SerializeString(instance.m_rSlotIconResource);
	    snapshot.SerializeString(instance.m_rSlotResource);
	    snapshot.SerializeString(instance.m_SlotFactionKey);
	
	    snapshot.SerializeBytes(instance.m_bIsLockedSlot, 4);
	    snapshot.SerializeBytes(instance.m_bIsDeadSlot, 4);
	    return true;
	}
	
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
	    snapshot.EncodeVector(packet);
		snapshot.EncodeVector(packet);
		snapshot.EncodeVector(packet);
		snapshot.EncodeVector(packet);
		
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeString(packet);
		
		snapshot.EncodeBool(packet);
		snapshot.EncodeBool(packet);
	}
	
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
	    snapshot.DecodeVector(packet);
		snapshot.DecodeVector(packet);
		snapshot.DecodeVector(packet);
		snapshot.DecodeVector(packet);
		
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeString(packet);
		
		snapshot.DecodeBool(packet);
		snapshot.DecodeBool(packet);
	
	    return true;
	}
	
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
	    return lhs.CompareSnapshots(rhs, 12)
	        && lhs.CompareSnapshots(rhs, 12)
	        && lhs.CompareSnapshots(rhs, 12)
	        && lhs.CompareSnapshots(rhs, 12)
			&& lhs.CompareSnapshots(rhs, 4)
	        && lhs.CompareSnapshots(rhs, 4)
	        && lhs.CompareSnapshots(rhs, 4)
	        && lhs.CompareSnapshots(rhs, 4)
	        && lhs.CompareSnapshots(rhs, 4)
	        && lhs.CompareStringSnapshots(rhs)
	        && lhs.CompareStringSnapshots(rhs)
	        && lhs.CompareStringSnapshots(rhs)
	        && lhs.CompareStringSnapshots(rhs)
	        && lhs.CompareSnapshots(rhs, 4)
	        && lhs.CompareSnapshots(rhs, 4);
	}
	
	static bool PropCompare(CRF_SlotDataContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
	    return snapshot.Compare(instance.m_vSlotVectorOne, 12)
	        && snapshot.Compare(instance.m_vSlotVectorTwo, 12)
	        && snapshot.Compare(instance.m_vSlotVectorThree, 12)
	        && snapshot.Compare(instance.m_vSlotVectorFour, 12)
			&& snapshot.Compare(instance.m_iSlotId, 4)
	        && snapshot.Compare(instance.m_iSlotCurrentPlayerId, 4)
	        && snapshot.Compare(instance.m_iSlotCurrentGroup, 4)
	        && snapshot.Compare(instance.m_iSlotCurrentCharacter, 4)
	        && snapshot.Compare(instance.m_iSlotType, 4)
	        && snapshot.CompareString(instance.m_sSlotName)
	        && snapshot.CompareString(instance.m_rSlotIconResource)
	        && snapshot.CompareString(instance.m_rSlotResource)
	        && snapshot.CompareString(instance.m_SlotFactionKey)
	        && snapshot.Compare(instance.m_bIsLockedSlot, 4)
	        && snapshot.Compare(instance.m_bIsDeadSlot, 4);
	}
}