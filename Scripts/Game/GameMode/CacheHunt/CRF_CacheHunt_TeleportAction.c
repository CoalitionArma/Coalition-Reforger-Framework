//------------------------------------------------------------------------------------
// CRF_CacheHunt_TeleportAction: Fast-travel between the defender home flag and the flag
// serving a cache.
//
// The flag pole prefab carries one instance of this action per possible cache (5), each
// with its own slot index. Which of them are visible depends on the flag the action sits
// on:
//  - On the defender home flag, slot N leads to cache N and is only shown while that
//    cache is still standing.
//  - On a cache flag, only slot 0 is shown and it leads back to the home flag.
//
// The action is defender-only, and disables itself while attackers are near either end
// of the trip. The proximity state is computed on the server and replicated onto the
// flag components, so this action only reads an already-authoritative flag.
//------------------------------------------------------------------------------------

class CRF_CacheHunt_TeleportAction: ScriptedUserAction
{
	[Attribute("0", UIWidgets.Slider, desc: "Which cache this action leads to when it sits on the defender home flag. Ignored on cache flags, where only slot 0 is used", params: "0 4 1", category: "CRF Cache Hunt")]
	protected int m_iSlotIndex;

	protected IEntity m_FlagEntity;
	protected CRF_CacheHunt_FlagComponent m_OwnFlag;
	protected string m_sCurrentActionName = "Teleport";

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);

		if (!GetGame().InPlayMode())
			return;

		m_FlagEntity = pOwnerEntity;
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		CRF_CacheHuntGamemodeManager gamemode = CRF_CacheHuntGamemodeManager.GetInstance();
		if (!gamemode)
			return;

		vector destinationPosition;
		vector destinationYawPitchRoll;
		if (!gamemode.GetFlagTransform(GetDestinationIndex(), destinationPosition, destinationYawPitchRoll))
		{
			Print("[CRF_CacheHunt] Teleport destination no longer exists.", LogLevel.WARNING);
			return;
		}

		// Land next to the flag rather than inside it
		vector finalSpawnLocation = vector.Zero;
		if (!SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, destinationPosition, TELEPORT_CLEARANCE_RADIUS))
			finalSpawnLocation = destinationPosition;

		SCR_Global.TeleportLocalPlayer(finalSpawnLocation, SCR_EPlayerTeleportedReason.FAST_TRAVEL);

		IEntity localEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (localEntity)
			localEntity.SetYawPitchRoll(destinationYawPitchRoll);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_CacheHuntGamemodeManager gamemode = CRF_CacheHuntGamemodeManager.GetInstance();
		if (!gamemode || !gamemode.AreTeleportFlagsEnabled())
			return false;

		// Only the side that owns the caches gets to use them as a fast-travel network
		if (!gamemode.IsDefender(user))
			return false;

		CRF_CacheHunt_FlagComponent ownFlag = GetOwnFlag();
		if (!ownFlag || ownFlag.GetCacheIndex() == CRF_CacheHunt_FlagComponent.UNASSIGNED_INDEX)
			return false;

		// A cache flag only ever offers the single trip back home
		if (!ownFlag.IsHomeFlag())
			return m_iSlotIndex == 0 && gamemode.HasHomeFlagDestination();

		// The home flag hides slots whose cache is destroyed or was never created
		return gamemode.HasCacheFlagDestination(m_iSlotIndex);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		CRF_CacheHuntGamemodeManager gamemode = CRF_CacheHuntGamemodeManager.GetInstance();
		CRF_CacheHunt_FlagComponent ownFlag = GetOwnFlag();

		if (!gamemode || !ownFlag)
		{
			SetCannotPerformReason(NO_DESTINATION_REASON);
			return false;
		}

		int destinationIndex = GetDestinationIndex();
		vector position;
		vector angles;
		if (!gamemode.GetFlagTransform(destinationIndex, position, angles))
		{
			SetCannotPerformReason(NO_DESTINATION_REASON);
			return false;
		}

		if (gamemode.AreEnemiesNearFlag(ownFlag.GetCacheIndex()))
		{
			SetCannotPerformReason(ENEMIES_HERE_REASON);
			return false;
		}

		if (gamemode.AreEnemiesNearFlag(destinationIndex))
		{
			SetCannotPerformReason(ENEMIES_THERE_REASON);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! \return Cache index this action travels to, or HOME_FLAG_INDEX when heading home
	protected int GetDestinationIndex()
	{
		CRF_CacheHunt_FlagComponent ownFlag = GetOwnFlag();
		if (ownFlag && !ownFlag.IsHomeFlag())
			return CRF_CacheHuntGamemodeManager.HOME_FLAG_INDEX;

		return m_iSlotIndex;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		CRF_CacheHunt_FlagComponent ownFlag = GetOwnFlag();
		if (ownFlag && !ownFlag.IsHomeFlag())
			m_sCurrentActionName = "Teleport to Main Spawn";
		else
			m_sCurrentActionName = string.Format("Teleport to %1", CRF_CacheHuntGamemodeManager.GetCacheDisplayName(m_iSlotIndex));

		outName = m_sCurrentActionName;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Teleporting only moves the local player, so there is nothing to replicate.
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}

	//===================================================================================
	// HELPERS
	//===================================================================================

	//------------------------------------------------------------------------------------------------
	//! The flag component on the entity this action is attached to.
	protected CRF_CacheHunt_FlagComponent GetOwnFlag()
	{
		if (m_OwnFlag)
			return m_OwnFlag;

		if (!m_FlagEntity)
			return null;

		m_OwnFlag = CRF_CacheHunt_FlagComponent.Cast(m_FlagEntity.FindComponent(CRF_CacheHunt_FlagComponent));
		if (!m_OwnFlag)
		{
			// The action may sit on a child of the flag pole
			IEntity root = m_FlagEntity.GetRootParent();
			if (root)
				m_OwnFlag = CRF_CacheHunt_FlagComponent.Cast(root.FindComponent(CRF_CacheHunt_FlagComponent));
		}

		return m_OwnFlag;
	}

	//===================================================================================
	// CONSTANTS
	//===================================================================================

	protected static const float TELEPORT_CLEARANCE_RADIUS	= 3;
	protected static const string ENEMIES_HERE_REASON		= "Enemies nearby - teleport disabled";
	protected static const string ENEMIES_THERE_REASON		= "Enemies at the destination - teleport disabled";
	protected static const string NO_DESTINATION_REASON		= "No teleport destination available";
}
