class CRF_ParachutePlayerComponentClass : ScriptComponentClass {}

class CRF_ParachutePlayerComponent : ScriptComponent
{
	// Core references
	protected RplComponent m_RplComponent;
	protected InputManager m_InputManager;
	protected SCR_PlayerController m_PlayerController;

	// Current pilot character and its compartment access
	protected IEntity m_ControlledCharacter;
	protected SCR_CompartmentAccessComponent m_CharacterCompartmentAccess;

	// Backpack item (local reference)
	protected CRF_ParachuteBackpackComponent m_BackpackComponent;

	// Deployed parachute (local reference)
	protected IEntity m_DeployedChuteEntity;

	// Replicated deployment state
	[RplProp(condition: RplCondition.OwnerOnly, onRplName: "OnDeployStateChanged")]
	protected bool m_IsDeployed = false;

	[RplProp(condition: RplCondition.OwnerOnly, onRplName: "OnDeployStateChanged")]
	protected RplId m_DeployedChuteRplId = RplId.Invalid();

	[RplProp(condition: RplCondition.OwnerOnly)]
	protected int m_ChuteSlotId = -1;

	// Deployment parameters
	[Attribute("8.0", UIWidgets.Slider, "Safe radius for deployment", "0 100 1")]
	protected float m_SafeRadius;

	[Attribute("10.0", UIWidgets.Slider, "Minimum altitude to deploy", "0 100 1")]
	protected float m_MinimumAltitude;

	// Query scratch (used in deployment checks)
	protected bool m_ObstacleFound;
	protected IEntity m_QueryCharacter;

	// Retry limits
	protected const int MAX_RESOLVE_RETRIES = 10;
	protected const int MAX_ENTER_RETRIES = 10;
	protected const int MAX_OWNERSHIP_RETRIES = 10;
	protected int m_ResolveRetries;
	protected int m_EnterRetries;
	protected int m_OwnershipRetries;

	// --------------------------------------------------------------------------------------------
	// Initialization & Cleanup
	// --------------------------------------------------------------------------------------------

	bool IsAuthority()
	{
		return m_RplComponent && m_RplComponent.Role() == RplRole.Authority;
	}

	bool IsOwner()
	{
		return m_RplComponent && m_RplComponent.IsOwner();
	}

	bool IsChuteOwner()
	{
		if (!m_DeployedChuteEntity)
			return false;
		RplComponent rpl = RplComponent.Cast(m_DeployedChuteEntity.FindComponent(RplComponent));
		return rpl && rpl.IsOwner();
	}

	override void OnPostInit(IEntity owner)
	{
		if (SCR_Global.IsEditMode())
			return;
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		if (SCR_Global.IsEditMode())
			return;

		m_RplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		m_PlayerController = SCR_PlayerController.Cast(owner);
		if (!m_PlayerController)
		{
			Print("CRF_ParachutePlayerComponent: No SCR_PlayerController found on owner!", LogLevel.ERROR);
			return;
		}

		m_PlayerController.m_OnControlledEntityChanged.Insert(OnControlledCharacterChanged);
		m_PlayerController.m_OnDestroyed.Insert(OnPlayerDestroyed);
		m_InputManager = GetGame().GetInputManager();

		OnControlledCharacterChanged(null, m_PlayerController.GetControlledEntity());
		EnableDeploymentInput();
	}

	override void OnDelete(IEntity owner)
	{
		if (m_PlayerController)
		{
			m_PlayerController.m_OnControlledEntityChanged.Remove(OnControlledCharacterChanged);
			m_PlayerController.m_OnDestroyed.Remove(OnPlayerDestroyed);
		}
		if (m_InputManager)
			m_InputManager.RemoveActionListener("CharacterJump", EActionTrigger.DOWN, OnJumpPressed);
	}

	protected void EnableDeploymentInput()
	{
		if (!m_InputManager || !m_PlayerController)
			return;
		if (m_PlayerController.GetPlayerId() != SCR_PlayerController.GetLocalPlayerId())
			return;
		m_InputManager.AddActionListener("CharacterJump", EActionTrigger.DOWN, OnJumpPressed);
	}

	protected void OnPlayerDestroyed(Instigator killer, IEntity killerEntity)
	{
		if (IsAuthority() && m_DeployedChuteEntity)
			DeleteChuteEntity(m_DeployedChuteEntity);
	}

	// --------------------------------------------------------------------------------------------
	// Character / Inventory Tracking
	// --------------------------------------------------------------------------------------------

	void OnControlledCharacterChanged(IEntity from, IEntity to)
	{
		SCR_ChimeraCharacter pilot = SCR_ChimeraCharacter.Cast(to);
		if (!pilot)
		{
			m_ControlledCharacter = null;
			m_CharacterCompartmentAccess = null;
			m_BackpackComponent = null;
			return;
		}

		m_ControlledCharacter = to;
		m_CharacterCompartmentAccess = SCR_CompartmentAccessComponent.Cast(to.FindComponent(SCR_CompartmentAccessComponent));
		RefreshBackpackReference();

		// If authority and a chute is deployed while changing character, clean up immediately.
		if (IsAuthority() && m_IsDeployed)
		{
			DeleteChuteEntity(m_DeployedChuteEntity);
			m_DeployedChuteEntity = null;
			m_IsDeployed = false;
			m_DeployedChuteRplId = RplId.Invalid();
			Replication.BumpMe();
		}
	}

	// Recursively search inventory for the first parachute backpack component
	protected CRF_ParachuteBackpackComponent FindParachuteInInventory(IEntity container)
	{
		if (!container)
			return null;

		CRF_ParachuteBackpackComponent comp = CRF_ParachuteBackpackComponent.Cast(container.FindComponent(CRF_ParachuteBackpackComponent));
		if (comp)
			return comp;

		SCR_InventoryStorageManagerComponent invMgr = SCR_InventoryStorageManagerComponent.Cast(container.FindComponent(SCR_InventoryStorageManagerComponent));
		if (invMgr)
		{
			array<IEntity> items = {};
			invMgr.GetItems(items, EStoragePurpose.PURPOSE_ANY);
			foreach (IEntity item : items)
			{
				comp = FindParachuteInInventory(item);
				if (comp)
					return comp;
			}
		}

		return null;
	}

	protected void RefreshBackpackReference()
	{
		m_BackpackComponent = null;
		IEntity character = GetControlledCharacter();
		if (!character)
			return;

		m_BackpackComponent = FindParachuteInInventory(character);
	}

	protected IEntity GetControlledCharacter()
	{
		if (m_ControlledCharacter && SCR_ChimeraCharacter.Cast(m_ControlledCharacter))
			return m_ControlledCharacter;
		if (!m_PlayerController)
			return null;
		IEntity ent = m_PlayerController.GetControlledEntity();
		if (SCR_ChimeraCharacter.Cast(ent))
			return ent;
		return null;
	}

	// --------------------------------------------------------------------------------------------
	// Deployment Checks & Input
	// --------------------------------------------------------------------------------------------

	protected bool CanDeployParachute(IEntity character, CRF_ParachuteBackpackComponent backpack)
	{
		if (!character || !backpack)
			return false;
		if (m_IsDeployed)
			return false;
		if (backpack.IsUsed())
			return false;

		SCR_ChimeraCharacter pawn = SCR_ChimeraCharacter.Cast(character);
		if (!pawn || pawn.IsInVehicle())
			return false;

		// Check if player is falling
		CharacterAnimationComponent animComp = CharacterAnimationComponent.Cast(character.FindComponent(CharacterAnimationComponent));
		if (!animComp || !animComp.PhysicsIsFalling())
			return false;

		// Obstacle check – ignore entities without physics (triggers, etc.)
		m_QueryCharacter = character;
		m_ObstacleFound = false;
		GetGame().GetWorld().QueryEntitiesBySphere(
			pawn.GetOrigin(),
			m_SafeRadius,
			_IsObstacleFound,
			null,
			EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.STATIC);
		if (m_ObstacleFound)
			return false;

		// Altitude check
		float terrainY = SCR_TerrainHelper.GetTerrainY(pawn.GetOrigin(), null, true);
		float altitude = pawn.GetOrigin()[1] - terrainY;
		if (altitude < m_MinimumAltitude)
			return false;

		return true;
	}

	bool _IsObstacleFound(IEntity other)
	{
		if (!other)
			return true;
		if (other == m_QueryCharacter)
			return true;

		IEntity parent = other.GetParent();
		while (parent)
		{
			if (parent == m_QueryCharacter)
				return true;
			parent = parent.GetParent();
		}
		
		if (!other.GetPhysics())
			return true;

		m_ObstacleFound = true;
		return false;
	}

	void OnJumpPressed()
	{
		IEntity character = GetControlledCharacter();
		if (!character)
			return;

		RefreshBackpackReference();

		if (!m_BackpackComponent)
			return;
		if (!CanDeployParachute(character, m_BackpackComponent))
			return;

		if (IsAuthority())
			Rpc_RequestDeploy();
		else
			Rpc(Rpc_RequestDeploy);
	}

	// --------------------------------------------------------------------------------------------
	// Server-side Deployment
	// --------------------------------------------------------------------------------------------

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void Rpc_RequestDeploy()
	{
		if (m_IsDeployed)
			return;

		IEntity character = GetControlledCharacter();
		if (!character)
			return;

		Physics characterPhysics = character.GetPhysics();
		vector initialVelocity = vector.Zero;
		if (characterPhysics)
			initialVelocity = characterPhysics.GetVelocity();

		CRF_ParachuteBackpackComponent backpack = FindBackpackOnServer(character);
		if (!backpack)
			return;

		if (!CanDeployParachute(character, backpack))
			return;

		ResourceName prefab = backpack.GetParachutePrefab();
		if (prefab == "")
			return;

		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		character.GetWorldTransform(sp.Transform);

		IEntity spawned = GetGame().SpawnEntityPrefabEx(prefab, false, GetGame().GetWorld(), sp);
		if (!spawned)
			return;

		backpack.SetUsed();

		CRF_ParachuteDeployedEntity chuteEntity = CRF_ParachuteDeployedEntity.Cast(spawned);
		if (!chuteEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			return;
		}

		m_DeployedChuteEntity = chuteEntity;
		m_BackpackComponent = backpack;

		TransferChuteOwnership(chuteEntity);

		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(chuteEntity.FindComponent(BaseCompartmentManagerComponent));
		if (!bcm)
			return;

		array<BaseCompartmentSlot> slots = {};
		bcm.GetCompartments(slots);
		BaseCompartmentSlot pilotSlot = null;
		foreach (BaseCompartmentSlot s : slots)
		{
			if (s && s.GetType() == ECompartmentType.CARGO)
			{
				pilotSlot = s;
				break;
			}
		}
		if (!pilotSlot)
			return;

		chuteEntity.SetPilotAndHook(character, m_CharacterCompartmentAccess);
		chuteEntity.SetInitialVelocity(initialVelocity);

		RplComponent spawnedRpl = RplComponent.Cast(chuteEntity.FindComponent(RplComponent));
		if (spawnedRpl)
			m_DeployedChuteRplId = spawnedRpl.Id();
		m_ChuteSlotId = pilotSlot.GetCompartmentSlotID();
		m_IsDeployed = true;
		Replication.BumpMe();

		GetGame().GetCallqueue().CallLater(QueueEnterChute, 50, false, m_DeployedChuteRplId, m_ChuteSlotId);
	}

	protected void QueueEnterChute(RplId chuteId, int slotId)
	{
		if (IsChuteOwner())
			Rpc_InitializeClientChute(chuteId, slotId);
	}

	protected CRF_ParachuteBackpackComponent FindBackpackOnServer(IEntity characterEntity)
	{
		return FindParachuteInInventory(characterEntity);
	}

	protected void TransferChuteOwnership(IEntity chute)
	{
		if (!chute)
			return;
		RplComponent rpl = RplComponent.Cast(chute.FindComponent(RplComponent));
		if (!rpl || !m_PlayerController)
			return;
		rpl.GiveExt(m_PlayerController.GetRplIdentity(), true);
	}

	// --------------------------------------------------------------------------------------------
	// Owner Client: Resolve and Enter the Parachute
	// --------------------------------------------------------------------------------------------

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void Rpc_InitializeClientChute(RplId chuteId, int slotId)
	{
		m_DeployedChuteRplId = chuteId;
		m_ChuteSlotId = slotId;
		m_IsDeployed = true;
		m_DeployedChuteEntity = null;
		m_ResolveRetries = 0;
		GetGame().GetCallqueue().CallLater(TryFindChuteEntity, 50, false);
	}

	void OnDeployStateChanged()
	{
		if (!GetGame().InPlayMode())
			return;
		if (!m_IsDeployed)
		{
			m_DeployedChuteEntity = null;
			return;
		}
		m_ResolveRetries = 0;
		GetGame().GetCallqueue().CallLater(TryFindChuteEntity, 50, false);
	}

	protected void TryFindChuteEntity()
	{
		if (!m_IsDeployed || m_DeployedChuteEntity || m_DeployedChuteRplId == RplId.Invalid())
			return;

		Managed instance = Replication.FindItem(m_DeployedChuteRplId);
		RplComponent rplComp = RplComponent.Cast(instance);
		if (!rplComp)
		{
			if (m_ResolveRetries < MAX_RESOLVE_RETRIES)
			{
				m_ResolveRetries++;
				RetryFindChute();
			}
			return;
		}

		IEntity chute = rplComp.GetEntity();
		if (!chute)
		{
			if (m_ResolveRetries < MAX_RESOLVE_RETRIES)
			{
				m_ResolveRetries++;
				RetryFindChute();
			}
			return;
		}

		m_DeployedChuteEntity = chute;

		CRF_ParachuteDeployedEntity chuteEntity = CRF_ParachuteDeployedEntity.Cast(chute);
		if (!chuteEntity)
			return;

		IEntity character = GetControlledCharacter();
		if (character)
		{
			chuteEntity.SetPilotAndHook(character, m_CharacterCompartmentAccess);
		}

		m_EnterRetries = 0;
		TryEnterChute();
	}

	protected void RetryFindChute()
	{
		GetGame().GetCallqueue().CallLater(TryFindChuteEntity, 50, false);
	}

	protected void TryEnterChute()
	{
		if (!GetGame().InPlayMode() || !m_DeployedChuteEntity || m_ChuteSlotId < 0)
			return;

		IEntity character = GetControlledCharacter();
		if (!character)
			return;

		if (!m_CharacterCompartmentAccess)
			m_CharacterCompartmentAccess = SCR_CompartmentAccessComponent.Cast(character.FindComponent(SCR_CompartmentAccessComponent));
		if (!m_CharacterCompartmentAccess)
			return;

		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(m_DeployedChuteEntity.FindComponent(BaseCompartmentManagerComponent));
		if (!bcm)
		{
			if (m_EnterRetries < MAX_ENTER_RETRIES)
			{
				m_EnterRetries++;
				RetryEnterChute();
			}
			return;
		}

		BaseCompartmentSlot slot = bcm.FindCompartment(m_ChuteSlotId);
		if (!slot)
		{
			if (m_EnterRetries < MAX_ENTER_RETRIES)
			{
				m_EnterRetries++;
				RetryEnterChute();
			}
			return;
		}

		if (slot.IsOccupied())
		{
			if (slot.GetOccupant() == character)
			{
				m_OwnershipRetries = 0;
				WaitForChuteOwnership();
				return;
			}
			if (m_EnterRetries < MAX_ENTER_RETRIES)
			{
				m_EnterRetries++;
				RetryEnterChute();
			}
			return;
		}

		if (slot.IsGetInLocked() || !slot.IsCompartmentAccessible())
		{
			if (m_EnterRetries < MAX_ENTER_RETRIES)
			{
				m_EnterRetries++;
				RetryEnterChute();
			}
			return;
		}

		if (character.GetParent() == m_DeployedChuteEntity)
		{
			m_OwnershipRetries = 0;
			WaitForChuteOwnership();
			return;
		}

		bool ok = m_CharacterCompartmentAccess.GetInVehicle(m_DeployedChuteEntity, slot, true, 0, ECloseDoorAfterActions.INVALID, true);
		if (!ok)
		{
			if (m_EnterRetries < MAX_ENTER_RETRIES)
			{
				m_EnterRetries++;
				RetryEnterChute();
			}
			return;
		}

		m_OwnershipRetries = 0;
		WaitForChuteOwnership();
	}

	protected void RetryEnterChute()
	{
		GetGame().GetCallqueue().CallLater(TryEnterChute, 50, false);
	}

	protected void WaitForChuteOwnership()
	{
		if (!m_DeployedChuteEntity)
			return;
		if (IsChuteOwner())
		{
			// No controls to enable – just return
			return;
		}
		if (m_OwnershipRetries < MAX_OWNERSHIP_RETRIES)
		{
			m_OwnershipRetries++;
			GetGame().GetCallqueue().CallLater(WaitForChuteOwnership, 50, false);
		}
	}

	// --------------------------------------------------------------------------------------------
	// Landing & Safe Deletion (Server)
	// --------------------------------------------------------------------------------------------

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void Rpc_RequestExit(RplId chuteId, float velocityAtExit)
	{
		if (!IsAuthority() || !m_IsDeployed || chuteId != m_DeployedChuteRplId)
			return;

		IEntity character = GetControlledCharacter();
		if (character)
		{
			// 1. Place player safely on ground with zero velocity
			PlacePlayerSafelyOnGround(character);

			// 2. Ask player to exit the vehicle (teleport)
			if (m_CharacterCompartmentAccess)
				m_CharacterCompartmentAccess.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);

			// 3. After teleport, ensure they are still grounded
			PlacePlayerSafelyOnGround(character);
		}

		// Start checking for empty compartment before deletion
		GetGame().GetCallqueue().CallLater(CheckAndDeleteIfEmpty, 200, false, m_DeployedChuteEntity);
	}

	// Helper to place player safely on ground with zero velocity
	void PlacePlayerSafelyOnGround(IEntity player)
	{
		if (!player) return;

		vector pos = player.GetOrigin();
		float terrainY = SCR_TerrainHelper.GetTerrainY(pos, null, true);
		pos[1] = terrainY + 0.5; // .5 meters above terrain for safety
		player.SetOrigin(pos);

		Physics phys = player.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}

	protected void CheckAndDeleteIfEmpty(IEntity chute)
	{
		if (!chute)
			return;

		BaseCompartmentManagerComponent bcm = BaseCompartmentManagerComponent.Cast(chute.FindComponent(BaseCompartmentManagerComponent));
		if (bcm)
		{
			BaseCompartmentSlot slot = bcm.FindCompartment(m_ChuteSlotId);
			if (slot && slot.IsOccupied())
			{
				GetGame().GetCallqueue().CallLater(CheckAndDeleteIfEmpty, 200, false, chute);
				return;
			}
		}

		DeleteChuteEntity(chute);
		m_DeployedChuteEntity = null;
		m_IsDeployed = false;
		m_DeployedChuteRplId = RplId.Invalid();
		Replication.BumpMe();
		Rpc(Rpc_ConfirmChuteCleared);
	}

	void DeleteChuteEntity(IEntity chute)
	{
		if (chute)
			SCR_EntityHelper.DeleteEntityAndChildren(chute);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void Rpc_ConfirmChuteCleared()
	{
		m_IsDeployed = false;
		m_DeployedChuteRplId = RplId.Invalid();
		m_ChuteSlotId = -1;
		m_DeployedChuteEntity = null;
	}
}