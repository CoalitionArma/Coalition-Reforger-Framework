enum CRF_ESlotType
{
	REGULAR = 0,
	LEADERORMEDIC,
	SPECIALTY,
}

class CRF_SlotDataContainer
{
	vector m_vSlotVectorOne;
	vector m_vSlotVectorTwo;
	vector m_vSlotVectorThree;
	vector m_vSlotVectorFour;
	
	int m_iSlotCurrentPlayerId = 0;
	RplId m_iSlotCurrentGroup = RplId.Invalid();
	RplId m_iSlotCurrentCharacter = RplId.Invalid();
	CRF_ESlotType m_iSlotType = CRF_ESlotType.REGULAR;
	
	string m_sSlotName = "";
	ResourceName m_rSlotIconResource = "";
	ResourceName m_rSlotResource = "";
	FactionKey m_SlotFactionKey = "";
	
	bool m_bIsLockedSlot = false;
	bool m_bIsDeadSlot = false;
	
	//------------------------------------------------------------------------------------------------
	bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteVector(m_vSlotVectorOne);
		writer.WriteVector(m_vSlotVectorTwo);
		writer.WriteVector(m_vSlotVectorThree);
		writer.WriteVector(m_vSlotVectorFour);
		
		writer.WriteInt(m_iSlotCurrentPlayerId);
		writer.WriteInt(m_iSlotCurrentGroup);
		writer.WriteInt(m_iSlotCurrentCharacter);
		writer.WriteInt(m_iSlotType);
		
		writer.WriteString(m_sSlotName);
		writer.WriteString(m_rSlotIconResource);
		writer.WriteString(m_rSlotResource);
		writer.WriteString(m_SlotFactionKey);
		
		writer.WriteBool(m_bIsLockedSlot);
		writer.WriteBool(m_bIsDeadSlot);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadVector(m_vSlotVectorOne);
		reader.ReadVector(m_vSlotVectorTwo);
		reader.ReadVector(m_vSlotVectorThree);
		reader.ReadVector(m_vSlotVectorFour);
		
		reader.ReadInt(m_iSlotCurrentPlayerId);
		reader.ReadInt(m_iSlotCurrentGroup);
		reader.ReadInt(m_iSlotCurrentCharacter);
		reader.ReadInt(m_iSlotType);
		
		reader.ReadString(m_sSlotName);
		reader.ReadString(m_rSlotIconResource);
		reader.ReadString(m_rSlotResource);
		reader.ReadString(m_SlotFactionKey);
		
		reader.ReadBool(m_bIsLockedSlot);
		reader.ReadBool(m_bIsDeadSlot);
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Extract(CRF_SlotDataContainer instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeVector(instance.m_vSlotVectorOne);
		snapshot.SerializeVector(instance.m_vSlotVectorTwo);
		snapshot.SerializeVector(instance.m_vSlotVectorThree);
		snapshot.SerializeVector(instance.m_vSlotVectorFour);
		
		snapshot.SerializeInt(instance.m_iSlotCurrentPlayerId);
		snapshot.SerializeInt(instance.m_iSlotCurrentGroup);
		snapshot.SerializeInt(instance.m_iSlotCurrentCharacter);
		snapshot.SerializeInt(instance.m_iSlotType);
		
		snapshot.SerializeString(instance.m_sSlotName);
		snapshot.SerializeString(instance.m_rSlotIconResource);
		snapshot.SerializeString(instance.m_rSlotResource);
		snapshot.SerializeString(instance.m_SlotFactionKey);
		
		snapshot.SerializeBool(instance.m_bIsLockedSlot);
		snapshot.SerializeBool(instance.m_bIsDeadSlot);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, CRF_SlotDataContainer instance)
	{
		snapshot.SerializeVector(instance.m_vSlotVectorOne);
		snapshot.SerializeVector(instance.m_vSlotVectorTwo);
		snapshot.SerializeVector(instance.m_vSlotVectorThree);
		snapshot.SerializeVector(instance.m_vSlotVectorFour);
		
		snapshot.SerializeInt(instance.m_iSlotCurrentPlayerId);
		snapshot.SerializeInt(instance.m_iSlotCurrentGroup);
		snapshot.SerializeInt(instance.m_iSlotCurrentCharacter);
		snapshot.SerializeInt(instance.m_iSlotType);
		
		snapshot.SerializeString(instance.m_sSlotName);
		snapshot.SerializeString(instance.m_rSlotIconResource);
		snapshot.SerializeString(instance.m_rSlotResource);
		snapshot.SerializeString(instance.m_SlotFactionKey);
		
		snapshot.SerializeBool(instance.m_bIsLockedSlot);
		snapshot.SerializeBool(instance.m_bIsDeadSlot);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	static bool PropCompare(CRF_SlotDataContainer instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.Compare(instance.m_vSlotVectorOne, 12)
			&& snapshot.Compare(instance.m_vSlotVectorTwo, 12)
			&& snapshot.Compare(instance.m_vSlotVectorThree, 12)
			&& snapshot.Compare(instance.m_vSlotVectorFour, 12)
			&& snapshot.CompareInt(instance.m_iSlotCurrentPlayerId)
			&& snapshot.CompareInt(instance.m_iSlotCurrentGroup)
			&& snapshot.CompareInt(instance.m_iSlotCurrentCharacter)
			&& snapshot.CompareInt(instance.m_iSlotType)
			&& snapshot.CompareString(instance.m_sSlotName)
			&& snapshot.CompareString(instance.m_rSlotIconResource)
			&& snapshot.CompareString(instance.m_rSlotResource)
			&& snapshot.CompareString(instance.m_SlotFactionKey)
			&& snapshot.CompareBool(instance.m_bIsLockedSlot)
			&& snapshot.CompareBool(instance.m_bIsDeadSlot);
	}
}