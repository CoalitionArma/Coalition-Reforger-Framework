//------------------------------------------------------------------------------------
// CRF_CTF_FlagComponent: Marks an entity as a physical Capture The Flag flag.
//
// Placed by the mission maker directly in the world (baked entity, not runtime-spawned)
// on a flag pole / banner prefab. Supports two roster roles, chosen via m_eOwningFaction:
//  - A faction's home flag (Double Flag mode) - owning faction is BLUFOR or OPFOR.
//  - The single neutral flag (Neutral Flag mode) - owning faction left as "NONE".
//
// All authoritative state changes (pickup, drop, return) happen on the server via
// TryPickUp()/Drop()/ReturnToBase(); clients only ever read the replicated state and
// mirror it visually. While carried, every machine (including the server) independently
// follows the carrier's own already-replicated transform each frame - no extra network
// traffic is needed to keep the flag glued to the carrier's back.
//------------------------------------------------------------------------------------

class CRF_CTF_FlagComponentClass : ScriptComponentClass
{
}

class CRF_CTF_FlagComponent : ScriptComponent
{
	//===================================================================================
	// STATE CONSTANTS
	//===================================================================================

	static const int STATE_AT_BASE = 0;
	static const int STATE_CARRIED = 1;
	static const int STATE_DROPPED = 2;

	//===================================================================================
	// ATTRIBUTES
	//===================================================================================

	[Attribute("", UIWidgets.ComboBox, desc: "Faction this flag belongs to. Leave empty for the single Neutral Flag mode flag.", enums: {ParamEnum("", "NONE (Neutral flag)"), ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR")})]
	protected FactionKey m_eOwningFaction;

	[Attribute("Flag", UIWidgets.EditBox, desc: "Display name used in pickup/drop/capture notifications.")]
	protected string m_sFlagDisplayName;

	[Attribute("Spine2", UIWidgets.EditBox, desc: "Name of the carrier's skeleton bone the flag follows while carried. Leave empty to follow the carrier's root transform instead.")]
	protected string m_sCarryBoneName;

	[Attribute("-0.15 0.05 -0.18", UIWidgets.Coords, desc: "Local offset (right, up, forward) from the carry bone the flag is held at.")]
	protected vector m_vCarryOffset;

	[Attribute("1", UIWidgets.CheckBox, desc: "Rotate the flag with the carry bone. Disable to keep the flag pole upright while carried.")]
	protected bool m_bApplyCarrierRotation;

	//===================================================================================
	// REPLICATED STATE
	//===================================================================================

	[RplProp(onRplName: "OnCarryStateReplicated")]
	protected int m_iFlagState = STATE_AT_BASE;

	[RplProp()]
	protected int m_iCarrierPlayerId = 0;

	[RplProp(onRplName: "OnCarryStateReplicated")]
	protected vector m_vDroppedPosition;

	[RplProp()]
	protected vector m_vDroppedAngles;

	//===================================================================================
	// LOCAL STATE
	//===================================================================================

	protected vector m_mHomeTransform[4];
	protected float m_fDroppedAtMs = 0;

	protected static ref array<CRF_CTF_FlagComponent> m_aRegisteredFlags = {};

	//===================================================================================
	// INITIALIZATION
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (!m_aRegisteredFlags.Contains(this))
			m_aRegisteredFlags.Insert(this);

		string factionLabel = m_eOwningFaction;
		if (factionLabel.IsEmpty())
			factionLabel = "NONE (neutral)";
		Print(string.Format("[CRF_CTF] Flag '%1' (Owning Faction: %2) registered. RplComponent present: %3", m_sFlagDisplayName, factionLabel, owner.FindComponent(RplComponent) != null), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Cache the mission maker's placed transform as "home" for return-to-base.
		owner.GetTransform(m_mHomeTransform);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (m_iFlagState == STATE_CARRIED)
			UpdateCarriedTransform();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		m_aRegisteredFlags.RemoveItem(this);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_CTF_FlagComponent()
	{
		if (m_aRegisteredFlags)
			m_aRegisteredFlags.RemoveItem(this);
	}

	//===================================================================================
	// JIP REPLICATION
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! RplSave and RplLoad must stay symmetric - add fields to both, in the same order.
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_iFlagState);
		writer.WriteInt(m_iCarrierPlayerId);
		writer.WriteVector(m_vDroppedPosition);
		writer.WriteVector(m_vDroppedAngles);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool RplLoad(ScriptBitReader reader)
	{
		reader.ReadInt(m_iFlagState);
		reader.ReadInt(m_iCarrierPlayerId);
		reader.ReadVector(m_vDroppedPosition);
		reader.ReadVector(m_vDroppedAngles);

		ApplyStateVisualsLocal();

		return true;
	}

	//===================================================================================
	// SERVER-AUTHORITATIVE STATE CHANGES
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Attempts to pick up the flag on behalf of a player. Caller (the gamemode manager) is
	//! responsible for faction/ownership rules - this only guards against a race where two
	//! pickup requests land in the same tick.
	//! \param[in] playerId The player picking up the flag
	//! \return True if the pickup was applied
	bool TryPickUp(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0 || m_iFlagState == STATE_CARRIED)
			return false;

		m_iFlagState = STATE_CARRIED;
		m_iCarrierPlayerId = playerId;
		Replication.BumpMe();

		ApplyStateVisualsLocal();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Drops the flag at a world position, e.g. where its carrier died or disconnected.
	//! \param[in] atPosition World position to drop the flag at
	//! \param[in] atAngles Yaw/pitch/roll to drop the flag at
	void Drop(vector atPosition, vector atAngles)
	{
		if (!Replication.IsServer() || m_iFlagState != STATE_CARRIED)
			return;

		m_iFlagState = STATE_DROPPED;
		m_iCarrierPlayerId = 0;
		m_vDroppedPosition = atPosition;
		m_vDroppedAngles = atAngles;
		m_fDroppedAtMs = GetGame().GetWorld().GetWorldTime();
		Replication.BumpMe();

		ApplyStateVisualsLocal();
	}

	//------------------------------------------------------------------------------------------------
	//! Returns the flag to its mission-placed home position, e.g. after a capture or timeout.
	void ReturnToBase()
	{
		if (!Replication.IsServer())
			return;

		m_iFlagState = STATE_AT_BASE;
		m_iCarrierPlayerId = 0;
		Replication.BumpMe();

		ApplyStateVisualsLocal();
	}

	//===================================================================================
	// VISUALS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! Fired on clients when the replicated carry state changes.
	protected void OnCarryStateReplicated()
	{
		ApplyStateVisualsLocal();
	}

	//------------------------------------------------------------------------------------------------
	//! Snaps the flag entity to the correct transform for its current state, on this machine only.
	protected void ApplyStateVisualsLocal()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		switch (m_iFlagState)
		{
			case STATE_AT_BASE:
			{
				owner.SetTransform(m_mHomeTransform);
				WakePhysics(owner);
				break;
			}

			case STATE_DROPPED:
			{
				vector rotMat[3];
				Math3D.AnglesToMatrix(m_vDroppedAngles, rotMat);
				vector dropMat[4];
				dropMat[0] = rotMat[0];
				dropMat[1] = rotMat[1];
				dropMat[2] = rotMat[2];
				dropMat[3] = m_vDroppedPosition;
				owner.SetTransform(dropMat);
				WakePhysics(owner);
				break;
			}

			case STATE_CARRIED:
			{
				UpdateCarriedTransform();
				break;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void WakePhysics(IEntity owner)
	{
		Physics physics = owner.GetPhysics();
		if (physics)
			physics.SetActive(1);
	}

	//------------------------------------------------------------------------------------------------
	//! Follows the carrier's bone (or root transform) every frame while carried. Runs
	//! independently on every machine from the carrier's own already-replicated transform,
	//! so no per-frame network traffic is required to keep the flag attached.
	protected void UpdateCarriedTransform()
	{
		IEntity owner = GetOwner();
		if (!owner || m_iCarrierPlayerId <= 0)
			return;

		IEntity carrier = GetGame().GetPlayerManager().GetPlayerControlledEntity(m_iCarrierPlayerId);
		if (!carrier)
			return;

		vector mat[4];
		carrier.GetTransform(mat);

		if (!m_sCarryBoneName.IsEmpty())
		{
			Animation carrierAnimation = carrier.GetAnimation();
			if (carrierAnimation)
			{
				int boneIndex = carrierAnimation.GetBoneIndex(m_sCarryBoneName);
				if (boneIndex != -1)
				{
					vector boneMat[4];
					carrierAnimation.GetBoneMatrix(boneIndex, boneMat);
					Math3D.MatrixMultiply4(mat, boneMat, mat);
				}
			}
		}

		vector offsetWorld = mat[0] * m_vCarryOffset[0] + mat[1] * m_vCarryOffset[1] + mat[2] * m_vCarryOffset[2];
		mat[3] = mat[3] + offsetWorld;

		if (!m_bApplyCarrierRotation)
		{
			mat[0] = "1 0 0";
			mat[1] = "0 1 0";
			mat[2] = "0 0 1";
		}

		owner.SetTransform(mat);
	}

	//===================================================================================
	// ACCESSORS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	bool IsAtBase()
	{
		return m_iFlagState == STATE_AT_BASE;
	}

	//------------------------------------------------------------------------------------------------
	bool IsCarried()
	{
		return m_iFlagState == STATE_CARRIED;
	}

	//------------------------------------------------------------------------------------------------
	bool IsDropped()
	{
		return m_iFlagState == STATE_DROPPED;
	}

	//------------------------------------------------------------------------------------------------
	bool IsNeutral()
	{
		return m_eOwningFaction.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	int GetCarrierId()
	{
		return m_iCarrierPlayerId;
	}

	//------------------------------------------------------------------------------------------------
	FactionKey GetOwningFaction()
	{
		return m_eOwningFaction;
	}

	//------------------------------------------------------------------------------------------------
	string GetDisplayName()
	{
		return m_sFlagDisplayName;
	}

	//------------------------------------------------------------------------------------------------
	//! \return World time (ms) the flag was last dropped, for auto-return timers
	float GetDroppedAtMs()
	{
		return m_fDroppedAtMs;
	}

	//===================================================================================
	// REGISTRY
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	static array<CRF_CTF_FlagComponent> GetRegisteredFlags()
	{
		array<CRF_CTF_FlagComponent> flags = {};
		foreach (CRF_CTF_FlagComponent flag : m_aRegisteredFlags)
		{
			if (flag)
				flags.Insert(flag);
		}

		return flags;
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] factionKey Faction to find the home flag for
	//! \return That faction's home flag, or null if none is registered (e.g. Neutral Flag mode)
	static CRF_CTF_FlagComponent GetFlagForFaction(FactionKey factionKey)
	{
		if (factionKey.IsEmpty())
			return null;

		foreach (CRF_CTF_FlagComponent flag : m_aRegisteredFlags)
		{
			if (flag && flag.GetOwningFaction() == factionKey)
				return flag;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! \return The single neutral flag, or null if none is registered (e.g. Double Flag mode)
	static CRF_CTF_FlagComponent GetNeutralFlag()
	{
		foreach (CRF_CTF_FlagComponent flag : m_aRegisteredFlags)
		{
			if (flag && flag.IsNeutral())
				return flag;
		}

		return null;
	}
}
