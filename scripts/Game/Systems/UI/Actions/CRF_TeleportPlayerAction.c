//------------------------------------------------------------------------------------------------
class CRF_TeleportPlayerAction : ScriptedUserAction
{	
	// Name of the object the player will be teleported to
	[Attribute("", "auto", "Object we are teleporting to", category: "CRF Teleport")]
	string m_sObjectNameToTeleportTo;
	
	// Optional gearscript to apply to the player after teleportation
	[Attribute("", "auto", "Gearscript given to the teleported entity on the other side", category: "CRF Teleport")]
	ResourceName m_sGearscriptToSet;
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Execute the teleport action when the player activates it
	 * @param pOwnerEntity The entity that owns this action (the object being interacted with)
	 * @param pUserEntity The player entity performing the action
	 */
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		// Validate necessary entities exist
		if (!pOwnerEntity)
		{
			Print("CRF_TeleportPlayerAction: Owner entity is null", LogLevel.ERROR);
			return;
		}
		
		if (!pUserEntity)
		{
			Print("CRF_TeleportPlayerAction: User entity is null", LogLevel.ERROR);
			return;
		}
		
		// Handle teleportation if a target is specified
		if (!m_sObjectNameToTeleportTo.IsEmpty())
		{
			// Find the destination object in the world
			IEntity teleportObject = GetGame().GetWorld().FindEntityByName(m_sObjectNameToTeleportTo);
			
			if (!teleportObject)
			{
				Print("CRF_TeleportPlayerAction: Target teleport object not found: " + m_sObjectNameToTeleportTo, LogLevel.ERROR);
				return;
			}
			
			// Get destination position and orientation
			vector teleportPosition = teleportObject.GetOrigin();
			vector teleportYawPitchRoll = teleportObject.GetYawPitchRoll();
		
			// Find a safe spot near the destination
			vector finalSpawnLocation = vector.Zero;
			SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, teleportPosition, 3);
			
			// Perform the teleportation
			SCR_Global.TeleportLocalPlayer(finalSpawnLocation, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
			
			// Set the player's orientation to match the target
			IEntity localEntity = SCR_PlayerController.GetLocalControlledEntity();
			if (localEntity)
			{
				localEntity.SetYawPitchRoll(teleportYawPitchRoll);
			}
		}

		// Handle gear assignment if specified
		if (!m_sGearscriptToSet.IsEmpty())
		{
			int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
			CRF_RplToAuthorityManager.GetInstance().ResetGear(localPlayerId, m_sGearscriptToSet, false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Indicates that this action only affects the local player and doesn't need to be synchronized
	 * @return Always returns true since teleportation is handled locally
	 */
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Indicates whether this action should be broadcast to other clients
	 * @return Always returns false since teleportation is handled locally
	 */
	override bool CanBroadcastScript()
	{
		return false;
	}
};
