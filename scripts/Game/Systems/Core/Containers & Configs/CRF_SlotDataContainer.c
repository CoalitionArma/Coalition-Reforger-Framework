class CRF_SlotDataContainer
{
	protected vector m_vSlotVectorOne;
	protected vector m_vSlotVectorTwo;
	protected vector m_vSlotVectorThree;
	protected vector m_vSlotVectorFour;
	
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
	// SCRIPT INVOKER
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void InvokeDataUpdate()
	{
		if (m_OnDataUpdate)
			m_OnDataUpdate.Invoke();
	}
	
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
		
		InvokeDataUpdate();
	}	
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentPlayerId(int playerId)
	{
		m_iSlotCurrentPlayerId = playerId;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentGroup(RplId groupRplId)
	{
		m_iSlotCurrentGroup = groupRplId;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentCharacter(RplId characterRplId)
	{
		m_iSlotCurrentCharacter = characterRplId;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotType(CRF_ESlotType slotType)
	{
		m_iSlotType = slotType;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotName(string name)
	{
		m_sSlotName = name;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotIcon(ResourceName icon)
	{
		m_rSlotIconResource = icon;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotResource(ResourceName resource)
	{
		m_rSlotResource = resource;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotFactionKey(FactionKey faction)
	{
		m_SlotFactionKey = faction;
	
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsLockedSlot(bool lockedState)
	{
		m_bIsLockedSlot = lockedState;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsDeadSlot(bool deadState)
	{
		m_bIsDeadSlot = deadState;
		
		InvokeDataUpdate();
	}
	
	//------------------------------------------------------------------------------------------------
	// SILENT SETTERS - No InvokeDataUpdate() calls for batch operations
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentPlayerIdSilent(int playerId)
	{
		m_iSlotCurrentPlayerId = playerId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentGroupSilent(RplId groupRplId)
	{
		m_iSlotCurrentGroup = groupRplId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotCurrentCharacterSilent(RplId characterRplId)
	{
		m_iSlotCurrentCharacter = characterRplId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotResourceSilent(ResourceName resource)
	{
		m_rSlotResource = resource;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSlotNameSilent(string name)
	{
		m_sSlotName = name;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsLockedSlotSilent(bool lockedState)
	{
		m_bIsLockedSlot = lockedState;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetIsDeadSlotSilent(bool deadState)
	{
		m_bIsDeadSlot = deadState;
	}
	
	//------------------------------------------------------------------------------------------------
	// BATCH UPDATE METHOD - Single InvokeDataUpdate() call for multiple property changes
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	bool BatchUpdateSlotData(int playerId = -1, RplId groupId = RplId.Invalid(), RplId charId = RplId.Invalid(), 
	                        ResourceName resource = "", string name = "", bool isLocked = false, bool isDead = false)
	{
		bool hasChanges = false;
		
		// Only update values that are different from defaults or current values
		if (playerId != -1 && m_iSlotCurrentPlayerId != playerId)
		{
			m_iSlotCurrentPlayerId = playerId;
			hasChanges = true;
		}
		
		if (groupId != RplId.Invalid() && m_iSlotCurrentGroup != groupId)
		{
			m_iSlotCurrentGroup = groupId;
			hasChanges = true;
		}
		
		if (charId != RplId.Invalid() && m_iSlotCurrentCharacter != charId)
		{
			m_iSlotCurrentCharacter = charId;
			hasChanges = true;
		}
		
		if (!resource.IsEmpty() && m_rSlotResource != resource)
		{
			m_rSlotResource = resource;
			hasChanges = true;
		}
		
		if (!name.IsEmpty() && m_sSlotName != name)
		{
			m_sSlotName = name;
			hasChanges = true;
		}
		
		if (m_bIsLockedSlot != isLocked)
		{
			m_bIsLockedSlot = isLocked;
			hasChanges = true;
		}
		
		if (m_bIsDeadSlot != isDead)
		{
			m_bIsDeadSlot = isDead;
			hasChanges = true;
		}
		
		// Only trigger replication if something actually changed
		if (hasChanges)
			InvokeDataUpdate();
			
		return hasChanges;
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
	int GetSlotCurrentPlayerId()
	{
		if(!m_iSlotCurrentPlayerId)
			return 0;
		else
			return m_iSlotCurrentPlayerId;
	}
	
	//------------------------------------------------------------------------------------------------
	RplId GetSlotCurrentGroup()
	{
		if(!m_iSlotCurrentGroup)
			return RplId.Invalid();
		else
			return m_iSlotCurrentGroup;
	}
	
	//------------------------------------------------------------------------------------------------
	RplId GetSlotCurrentCharacter()
	{
		if(!m_iSlotCurrentCharacter)
			return RplId.Invalid();
		else
			return m_iSlotCurrentCharacter;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_ESlotType GetSlotType()
	{
		if(!m_iSlotType)
			return CRF_ESlotType.GENERAL_INFANTRY;
		else
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
		if(!m_rSlotIconResource)
			return "";
		else
			return m_rSlotIconResource;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetSlotResource()
	{
		if(!m_rSlotResource)
			return "";
		else
			return m_rSlotResource;
	}
	
	//------------------------------------------------------------------------------------------------
	FactionKey GetSlotFactionKey()
	{
		if(!m_SlotFactionKey)
			return "";
		else
			return m_SlotFactionKey;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIsLockedSlot()
	{
		if(!m_bIsLockedSlot)
			return false;
		else
			return m_bIsLockedSlot;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIsDeadSlot()
	{
		if(!m_bIsDeadSlot)
			return false;
		else
			return m_bIsDeadSlot;
	}
	
	//------------------------------------------------------------------------------------------------
	// REPLICATION STUFF
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	bool RplSave(ScriptBitWriter writer)
	{
	    // Vectors = 3 * 32 bits = 96
	    writer.Write(m_vSlotVectorOne, 96);
	    writer.Write(m_vSlotVectorTwo, 96);
	    writer.Write(m_vSlotVectorThree, 96);
	    writer.Write(m_vSlotVectorFour, 96);
	
	    // Integers = 32 bits
	    writer.Write(m_iSlotCurrentPlayerId, 32);
	    writer.Write(m_iSlotCurrentGroup, 32);
	    writer.Write(m_iSlotCurrentCharacter, 32);
	    writer.Write(m_iSlotType, 32);
	
	    // Strings use the proto directly (cannot replace with raw Write)
	    writer.WriteString(m_sSlotName);
	    writer.WriteString(m_rSlotIconResource);
	    writer.WriteString(m_rSlotResource);
	    writer.WriteString(m_SlotFactionKey);
	
	    // Bools = 1 bit
	    writer.Write(m_bIsLockedSlot, 1);
	    writer.Write(m_bIsDeadSlot, 1);
	
	    return true;
	}
	
	bool RplLoad(ScriptBitReader reader)
	{
	    reader.Read(m_vSlotVectorOne, 96);
	    reader.Read(m_vSlotVectorTwo, 96);
	    reader.Read(m_vSlotVectorThree, 96);
	    reader.Read(m_vSlotVectorFour, 96);
	
	    reader.Read(m_iSlotCurrentPlayerId, 32);
	    reader.Read(m_iSlotCurrentGroup, 32);
	    reader.Read(m_iSlotCurrentCharacter, 32);
	    reader.Read(m_iSlotType, 32);
	
	    reader.ReadString(m_sSlotName);
	    reader.ReadString(m_rSlotIconResource);
	    reader.ReadString(m_rSlotResource);
	    reader.ReadString(m_SlotFactionKey);
	
	    reader.Read(m_bIsLockedSlot, 1);
	    reader.Read(m_bIsDeadSlot, 1);
	
	    return true;
	}
	
	static bool Extract(CRF_SlotDataContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
	    snapshot.SerializeBytes(instance.m_vSlotVectorOne, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorTwo, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorThree, 12);
	    snapshot.SerializeBytes(instance.m_vSlotVectorFour, 12);
	
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