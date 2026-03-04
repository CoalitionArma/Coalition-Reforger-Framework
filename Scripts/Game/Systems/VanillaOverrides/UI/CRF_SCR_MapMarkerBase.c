modded class SCR_MapMarkerBase
{
	bool m_bIsShared = false;
	
	override void SetVisible(bool state)
	{
		//Nuclear option, cause this is fucking called in a million places
		if (!m_bIsShared && m_wRoot)
		{
			m_wRoot.SetVisible(false);
			return;
		}
		
		if (m_wRoot)
			m_wRoot.SetVisible(state);
	}
	
	override void SetUpdateDisabled(bool state)
	{
		if (m_bIsBlocked && !state)
			SetUpdateDisabled(true);
		
		m_bIsUpdateDisabled = state;
		
		//Nuclear option, cause this is fucking called in a million places
		if (!m_bIsShared && m_wRoot)
		{
			m_wRoot.SetVisible(false);
			return;
		}
		
		if (m_wRoot && m_wRoot.IsVisible() == state)
			m_wRoot.SetVisible(!state);
	}
	
	//------------------------------------------------------------------------------------------------
	override static bool Extract(SCR_MapMarkerBase instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeInt(instance.m_iPosWorldX);
		snapshot.SerializeInt(instance.m_iPosWorldY);
		snapshot.SerializeInt(instance.m_iMarkerID);
		snapshot.SerializeInt(instance.m_iMarkerOwnerID);
		snapshot.SerializeInt(instance.m_iFlags);
		snapshot.SerializeInt(instance.m_iConfigID);
		snapshot.SerializeInt(instance.m_iFactionFlags);
		snapshot.SerializeBytes(instance.m_iRotation, 2);
		snapshot.SerializeBytes(instance.m_eType, 1);
		snapshot.SerializeBytes(instance.m_iColorEntry, 1);
		snapshot.SerializeBytes(instance.m_iIconEntry, 2);
		snapshot.SerializeString(instance.m_sCustomText);
		snapshot.SerializeBool(instance.m_bIsTimestampVisible);
		snapshot.SerializeBytes(instance.m_Timestamp, 8);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, SCR_MapMarkerBase instance)
	{
		snapshot.SerializeInt(instance.m_iPosWorldX);
		snapshot.SerializeInt(instance.m_iPosWorldY);
		snapshot.SerializeInt(instance.m_iMarkerID);
		snapshot.SerializeInt(instance.m_iMarkerOwnerID);
		snapshot.SerializeInt(instance.m_iFlags);
		snapshot.SerializeInt(instance.m_iConfigID);
		snapshot.SerializeInt(instance.m_iFactionFlags);
		snapshot.SerializeBytes(instance.m_iRotation, 2);
		snapshot.SerializeBytes(instance.m_eType, 1);
		snapshot.SerializeBytes(instance.m_iColorEntry, 1);
		snapshot.SerializeBytes(instance.m_iIconEntry, 2);
		snapshot.SerializeString(instance.m_sCustomText);
		snapshot.SerializeBool(instance.m_bIsTimestampVisible);
		snapshot.SerializeBytes(instance.m_Timestamp, 8);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.Serialize(packet, SERIALIZED_BYTES);
		snapshot.EncodeString(packet);
		snapshot.EncodeBool(packet); // m_bIsTimestampVisible
		snapshot.Serialize(packet, 8); // m_Timestamp
	}

	//------------------------------------------------------------------------------------------------
	override static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.Serialize(packet, SERIALIZED_BYTES);
		snapshot.DecodeString(packet);
		snapshot.DecodeBool(packet); // m_bIsTimestampVisible
		snapshot.Serialize(packet, 8); // m_Timestamp
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs , ScriptCtx ctx)
	{
		return lhs.CompareSnapshots(rhs, SERIALIZED_BYTES)	// m_iPosWorldX(4) + m_iPosWorldY(4) + m_iMarkerID(4) + m_iMarkerOwnerID(4) + m_iFlags(4) + m_iConfigID(4) + m_iFactionFlags(4) + m_iRotation(2) + m_eType(1) + m_iColorEntry(1) + m_iIconEntry(2)
			&& lhs.CompareStringSnapshots(rhs) // m_sCustomText
			&& lhs.CompareSnapshots(rhs, 4 + 8); // m_bIsTimestampVisible + m_Timestamp
	}

	//------------------------------------------------------------------------------------------------
	override static bool PropCompare(SCR_MapMarkerBase instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
		return snapshot.CompareInt(instance.m_iPosWorldX)
			&& snapshot.CompareInt(instance.m_iPosWorldY) 
			&& snapshot.CompareInt(instance.m_iMarkerID)
			&& snapshot.CompareInt(instance.m_iMarkerOwnerID)
			&& snapshot.CompareInt(instance.m_iFlags)
			&& snapshot.CompareInt(instance.m_iConfigID)
			&& snapshot.CompareInt(instance.m_iFactionFlags)
			&& snapshot.Compare(instance.m_iRotation, 2)
			&& snapshot.Compare(instance.m_eType, 1)
			&& snapshot.Compare(instance.m_iColorEntry, 1)
			&& snapshot.Compare(instance.m_iIconEntry, 2)
			&& snapshot.CompareString(instance.m_sCustomText)
			&& snapshot.CompareBool(instance.m_bIsTimestampVisible)
			&& snapshot.Compare(instance.m_Timestamp, 8);
	}
}