modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Returns true if the entity is at or near the water surface
	static bool ACE_Carrying_IsCharacterInWater(notnull IEntity entity)
	{
		vector pos = entity.GetOrigin();
		// Check if the entity's origin is already submerged
		if (ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), pos))
			return true;
		// Also check 1m below origin to detect characters floating at the water surface,
		// whose root origin may sit just above the water line
		pos[1] = pos[1] - 1.0;
		return ChimeraWorldUtils.TryGetWaterSurfaceSimple(GetGame().GetWorld(), pos);
	}

	//------------------------------------------------------------------------------------------------
	//! Override: Allow carrying/dragging unconscious characters in water even when ragdoll is active.
	//! On land, ragdoll ends quickly once settled. In water, buoyancy keeps ragdoll active indefinitely,
	//! so we must skip the ragdoll block when the casualty is in water.
	override protected bool ACE_Carrying_CanCarryOrDragCasualty(SCR_ChimeraCharacter casualty, out string cannotPerformReason)
	{
		if (!ACE_CanCarry(casualty, cannotPerformReason))
			return false;

		SCR_CharacterControllerComponent casualtyCharController = SCR_CharacterControllerComponent.Cast(casualty.GetCharacterController());
		if (!casualtyCharController || casualtyCharController.GetLifeState() != ECharacterLifeState.INCAPACITATED)
			return false;

		// Trying to carry while unit is ragdolling will break things on land, but in water the
		// ragdoll persists indefinitely (floating physics never settle), so we allow it there.
		if (casualtyCharController.GetAnimationComponent().IsRagdollActive() && !ACE_Carrying_IsCharacterInWater(casualty))
		{
			cannotPerformReason = "#AR-UserActionUnavailable";
			return false;
		}

		return true;
	}
}
