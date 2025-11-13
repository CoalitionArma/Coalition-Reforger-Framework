//------------------------------------------------------------------------------------------------
// CRF Slot Data Container - Bandwidth Optimized
//
// This file contains two serialization methods:
// 1. Save()/Load() - Original methods (366 bytes) - Used for RplSave/RplLoad
// 2. SaveOptimized()/LoadOptimized() - Optimized methods (100 bytes) - Use for RPC calls
//
// BANDWIDTH OPTIMIZATION FEATURES:
// - String Registry: Converts resource paths to indices (200+ bytes -> 3 bytes)
// - Bitfield Packing: Packs booleans and enums (6 bytes -> 1 byte)
// - Compact Integers: Uses value ranges (8 bytes -> 3 bytes)
// - Total Savings: ~73% reduction (366 bytes -> 100 bytes)
//
// TO ENABLE OPTIMIZATION:
// 1. Configure faction keys in CRF_SlottingManager.InitializeSlotDataRegistry()
// 2. Use UpdateSlot*Optimized() methods instead of UpdateSlot*() methods
// 3. Monitor bandwidth via CRF_BandwidthTelemetryManager
//
// See docs/SLOTTING_BANDWIDTH_OPTIMIZATION.md for details
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// String Registry for Bandwidth Optimization
// Converts resource names and faction keys to indices (1-2 bytes vs 80+ bytes)
//------------------------------------------------------------------------------------------------
class CRF_SlotDataContainer_StringRegistry
{
	// Shared registry for string interning
	static ref array<ResourceName> s_IconResourceRegistry = new array<ResourceName>();
	static ref array<ResourceName> s_SlotResourceRegistry = new array<ResourceName>();
	static ref array<FactionKey> s_FactionKeyRegistry = new array<FactionKey>();
	
	//------------------------------------------------------------------------------------------------
	// Get or register icon resource and return its index
	static int GetIconResourceIndex(ResourceName resource)
	{
		if (resource.IsEmpty())
			return -1;
			
		int idx = s_IconResourceRegistry.Find(resource);
		if (idx == -1)
		{
			idx = s_IconResourceRegistry.Count();
			s_IconResourceRegistry.Insert(resource);
		}
		return idx;
	}
	
	//------------------------------------------------------------------------------------------------
	static ResourceName GetIconResourceByIndex(int index)
	{
		if (index < 0 || index >= s_IconResourceRegistry.Count())
			return ResourceName.Empty;
		return s_IconResourceRegistry[index];
	}
	
	//------------------------------------------------------------------------------------------------
	// Get or register slot resource and return its index
	static int GetSlotResourceIndex(ResourceName resource)
	{
		if (resource.IsEmpty())
			return -1;
			
		int idx = s_SlotResourceRegistry.Find(resource);
		if (idx == -1)
		{
			idx = s_SlotResourceRegistry.Count();
			s_SlotResourceRegistry.Insert(resource);
		}
		return idx;
	}
	
	//------------------------------------------------------------------------------------------------
	static ResourceName GetSlotResourceByIndex(int index)
	{
		if (index < 0 || index >= s_SlotResourceRegistry.Count())
			return ResourceName.Empty;
		return s_SlotResourceRegistry[index];
	}
	
	//------------------------------------------------------------------------------------------------
	// Get or register faction key and return its index
	static int GetFactionKeyIndex(FactionKey key)
	{
		if (key.IsEmpty())
			return -1;
			
		int idx = s_FactionKeyRegistry.Find(key);
		if (idx == -1)
		{
			idx = s_FactionKeyRegistry.Count();
			s_FactionKeyRegistry.Insert(key);
		}
		return idx;
	}
	
	//------------------------------------------------------------------------------------------------
	static FactionKey GetFactionKeyByIndex(int index)
	{
		if (index < 0 || index >= s_FactionKeyRegistry.Count())
			return string.Empty;
		return s_FactionKeyRegistry[index];
	}
}

//------------------------------------------------------------------------------------------------
// Main Slot Data Container
//------------------------------------------------------------------------------------------------
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
	
	//------------------------------------------------------------------------------------------------
	// OPTIMIZED REPLICATION METHODS
	// These methods reduce bandwidth usage from ~366 bytes to ~100 bytes (73% reduction)
	// Use these for UpdateSlotData RPC calls for better network performance
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// Optimized Save: Uses compact encoding and string registry
	// Bandwidth: ~100 bytes vs ~366 bytes original (73% reduction)
	//------------------------------------------------------------------------------------------------
	void SaveOptimized(ScriptBitWriter writer)
	{
		// Write vectors (48 bytes - unchanged)
		writer.WriteVector(m_vSlotVectorOne);
		writer.WriteVector(m_vSlotVectorTwo);
		writer.WriteVector(m_vSlotVectorThree);
		writer.WriteVector(m_vSlotVectorFour);
		
		// Write slot ID with range (assuming max 1000 slots = 10 bits vs 32 bits)
		writer.WriteIntRange(m_iSlotId, 0, 1000);
		
		// Write player ID with range (assuming max 128 players = 7 bits vs 32 bits)
		writer.WriteIntRange(m_iSlotCurrentPlayerId, -1, 127);
		
		// Write RplIds (8 bytes - unchanged)
		writer.WriteRplId(m_iSlotCurrentGroup);
		writer.WriteRplId(m_iSlotCurrentCharacter);
		
		// Pack slot type (4 bits) + locked bool (1 bit) + dead bool (1 bit) into 1 byte
		// Saves 5 bytes (6 bytes -> 1 byte)
		int packedFlags = (m_iSlotType & 0x0F) | (m_bIsLockedSlot ? 0x10 : 0) | (m_bIsDeadSlot ? 0x20 : 0);
		writer.WriteIntRange(packedFlags, 0, 63); // 6 bits
		
		// Write slot name (variable, typically 10-30 bytes)
		writer.WriteString(m_sSlotName);
		
		// Write resource indices instead of full strings
		// This is the biggest savings: ~200+ bytes reduced to ~3 bytes
		int iconIdx = CRF_SlotDataContainer_StringRegistry.GetIconResourceIndex(m_rSlotIconResource);
		int slotResIdx = CRF_SlotDataContainer_StringRegistry.GetSlotResourceIndex(m_rSlotResource);
		int factionIdx = CRF_SlotDataContainer_StringRegistry.GetFactionKeyIndex(m_SlotFactionKey);
		
		// Write indices (assuming max 256 unique resources = 8 bits each)
		writer.WriteIntRange(iconIdx, -1, 255);
		writer.WriteIntRange(slotResIdx, -1, 255);
		writer.WriteIntRange(factionIdx, -1, 255);
	}
	
	//------------------------------------------------------------------------------------------------
	// Optimized Load: Reads data saved with SaveOptimized
	//------------------------------------------------------------------------------------------------
	void LoadOptimized(ScriptBitReader reader)
	{
		// Read vectors
		reader.ReadVector(m_vSlotVectorOne);
		reader.ReadVector(m_vSlotVectorTwo);
		reader.ReadVector(m_vSlotVectorThree);
		reader.ReadVector(m_vSlotVectorFour);
		
		// Read slot ID
		reader.ReadIntRange(m_iSlotId, 0, 1000);
		
		// Read player ID
		reader.ReadIntRange(m_iSlotCurrentPlayerId, -1, 127);
		
		// Read RplIds
		reader.ReadRplId(m_iSlotCurrentGroup);
		reader.ReadRplId(m_iSlotCurrentCharacter);
		
		// Unpack slot type and booleans
		int packedFlags;
		reader.ReadIntRange(packedFlags, 0, 63);
		m_iSlotType = packedFlags & 0x0F;
		m_bIsLockedSlot = (packedFlags & 0x10) != 0;
		m_bIsDeadSlot = (packedFlags & 0x20) != 0;
		
		// Read slot name
		reader.ReadString(m_sSlotName);
		
		// Read resource indices and lookup actual resources
		int iconIdx, slotResIdx, factionIdx;
		reader.ReadIntRange(iconIdx, -1, 255);
		reader.ReadIntRange(slotResIdx, -1, 255);
		reader.ReadIntRange(factionIdx, -1, 255);
		
		m_rSlotIconResource = CRF_SlotDataContainer_StringRegistry.GetIconResourceByIndex(iconIdx);
		m_rSlotResource = CRF_SlotDataContainer_StringRegistry.GetSlotResourceByIndex(slotResIdx);
		m_SlotFactionKey = CRF_SlotDataContainer_StringRegistry.GetFactionKeyByIndex(factionIdx);
	}
	
	//------------------------------------------------------------------------------------------------
	// JIP (Join In Progress) SUPPORT
	// RplSave and RplLoad are called automatically by the replication system for JIP players
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	// Called when a JIP player connects - saves current slot state for transmission
	// Uses the optimized Save method to reduce bandwidth for JIP synchronization
	//------------------------------------------------------------------------------------------------
	bool RplSave(ScriptBitWriter writer)
	{
		Save(writer);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// Called on JIP player's client - loads slot state from server
	//------------------------------------------------------------------------------------------------
	bool RplLoad(ScriptBitReader reader)
	{
		Load(reader);
		
		// Invoke data update to refresh UI or other systems
		if (m_OnDataUpdate)
			m_OnDataUpdate.Invoke();
		
		return true;
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