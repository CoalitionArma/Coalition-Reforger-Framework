//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_SpawnPointData
{	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ATTRIBUTES SET MANUALLY
//=============================================================================================================================================================================================================================================================================================================================================================

	[Attribute("true", UIWidgets.CheckBox,"Is this spawn point active by default")]
	protected bool m_bSpawnPointActive;
	
	[Attribute("Base", "auto", "Nickname for the respawn point")]
	protected string m_sSpawnPointName;
	
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFactions))]
	protected CRF_EFactions m_iSpawnPointFaction;
	
	[Attribute("10", "auto", "How far away in meters we spawn people from the spawn point (Keep in mind this is RADIUS, not diameter)")]
	protected int m_iSpawnPointRadius;
	
	[Attribute("true", "auto", "If this Spawn Point does a safety check before spawning a entity (prevents clipping into buildings/terrain)")]
	protected bool m_bSpawnPointSafetyCheck;
	
	[Attribute("true", "auto", "If this Spawn Point conforms to the terrain before spawning the entity (prevents players spawning too high off the ground)")]
	protected bool m_bSpawnPointConformsToTerrain;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RUNTIME VARIABLES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	protected int m_iSpawnPointId;
	protected RplId m_iSpawnPointEntity = RplId.Invalid();
	protected bool m_bIsTemp;
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 UPDATE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Replaces or sets the internal CRF_SpawnPointData record for the id.
	//! If newSpawnData is non-null, the spawns data is updated with the provided instance.
	//! \param[in] newSpawnData: Pointer/reference to the new CRF_SpawnPointData to apply.
	void DataUpdate(CRF_SpawnPointData newSpawnData = null)
	{	
		if(newSpawnData)	
		{
			SetSpawnPointActive(newSpawnData.GetIsActiveSpawnPoint());	
			SetSpawnPointName(newSpawnData.GetSpawnPointName());	
			SetSpawnPointFaction(newSpawnData.GetSpawnPointFaction());
			SetSpawnPointId(newSpawnData.GetSpawnPointId());
			SetSpawnPointEntity(newSpawnData.GetSpawnPointEntity());
		};
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 SETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointActive(bool isActive)
	{
		m_bSpawnPointActive = isActive;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointName(string spawnPointName)
	{
		m_sSpawnPointName = spawnPointName;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointFaction(CRF_EFactions spawnPointFaction)
	{
		m_iSpawnPointFaction = spawnPointFaction;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointId(int spawnId)
	{
		m_iSpawnPointId = spawnId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointEntity(RplId entityId)
	{
		m_iSpawnPointEntity = entityId;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSpawnPointToTemp()
	{
		m_bIsTemp = true;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 GETTERS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	bool GetIsActiveSpawnPoint()
	{
		return m_bSpawnPointActive;
	}
	
	//------------------------------------------------------------------------------------------------
	string GetSpawnPointName()
	{
		return m_sSpawnPointName;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_EFactions GetSpawnPointFaction()
	{
		return m_iSpawnPointFaction;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSpawnPointRadius()
	{
		return m_iSpawnPointRadius;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIfSpawnPointSafetyCheck()
	{
		return m_bSpawnPointSafetyCheck;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIfSpawnPointConformsToTerrain()
	{
		return m_bSpawnPointConformsToTerrain;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetSpawnPointId()
	{
		return m_iSpawnPointId;
	}
	
	//------------------------------------------------------------------------------------------------
	RplId GetSpawnPointEntity()
	{
		return m_iSpawnPointEntity;
	}
	
	//------------------------------------------------------------------------------------------------
	bool GetIsTempSpawnPoint()
	{
		return m_bIsTemp;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 REPLICATION METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void Save(ScriptBitWriter writer)
	{
		writer.WriteBool(m_bSpawnPointActive);
		writer.WriteString(m_sSpawnPointName);
		writer.WriteInt(m_iSpawnPointFaction);
		writer.WriteInt(m_iSpawnPointId);
		writer.WriteRplId(m_iSpawnPointEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	void Load(ScriptBitReader reader)
	{
		reader.ReadBool(m_bSpawnPointActive);
		reader.ReadString(m_sSpawnPointName);
		reader.ReadInt(m_iSpawnPointFaction);
		reader.ReadInt(m_iSpawnPointId);
		reader.ReadRplId(m_iSpawnPointEntity);
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Extract(CRF_SpawnPointData instance, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.SerializeBytes(instance.m_bSpawnPointActive, 4);
		snapshot.SerializeString(instance.m_sSpawnPointName);
		snapshot.SerializeBytes(instance.m_iSpawnPointFaction, 4);
		snapshot.SerializeBytes(instance.m_iSpawnPointId, 4);
		snapshot.SerializeBytes(instance.m_iSpawnPointEntity, 4);
	    return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Inject(SSnapSerializerBase snapshot, ScriptCtx ctx, CRF_SpawnPointData instance)
	{
		snapshot.SerializeBytes(instance.m_bSpawnPointActive, 4);
		snapshot.SerializeString(instance.m_sSpawnPointName);
		snapshot.SerializeBytes(instance.m_iSpawnPointFaction, 4);
		snapshot.SerializeBytes(instance.m_iSpawnPointId, 4);
		snapshot.SerializeBytes(instance.m_iSpawnPointEntity, 4);
	    return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static void Encode(SSnapSerializerBase snapshot, ScriptCtx ctx, ScriptBitSerializer packet)
	{
		snapshot.EncodeInt(packet);
		snapshot.EncodeString(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
		snapshot.EncodeInt(packet);
	}
	
	//------------------------------------------------------------------------------------------------
	static bool Decode(ScriptBitSerializer packet, ScriptCtx ctx, SSnapSerializerBase snapshot)
	{
		snapshot.DecodeInt(packet);
		snapshot.DecodeString(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
		snapshot.DecodeInt(packet);
	    return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool SnapCompare(SSnapSerializerBase lhs, SSnapSerializerBase rhs, ScriptCtx ctx)
	{
	    return lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareStringSnapshots(rhs)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4)
		&& lhs.CompareSnapshots(rhs, 4);
	}
	
	//------------------------------------------------------------------------------------------------
	static bool PropCompare(CRF_SpawnPointData instance, SSnapSerializerBase snapshot, ScriptCtx ctx)
	{
	    return snapshot.Compare(instance.m_bSpawnPointActive, 4)
		&& snapshot.CompareString(instance.m_sSpawnPointName)
		&& snapshot.Compare(instance.m_iSpawnPointFaction, 4)
		&& snapshot.Compare(instance.m_iSpawnPointId, 4)
		&& snapshot.Compare(instance.m_iSpawnPointEntity, 4);
	}
}