modded class GC_SuppressionSystem
{
	//------------------------------------------------------------------------------------------------
	//! Override: Prevents projectiles from being registered for suppression when the local player is a spectator
	override void RegisterProjectile(IEntity projectile)
	{
		IEntity localEntity = GetGame().GetPlayerController().GetControlledEntity();
		if (localEntity && CRF_EntityHelper.IsSpectator(localEntity))
			return;

		super.RegisterProjectile(projectile);
	}

	//------------------------------------------------------------------------------------------------
	//! Override: Prevents suppression from being applied when the local player is a spectator
	override protected void AddSuppression(float suppression)
	{
		IEntity localEntity = GetGame().GetPlayerController().GetControlledEntity();
		if (localEntity && CRF_EntityHelper.IsSpectator(localEntity))
			return;

		super.AddSuppression(suppression);
	}
}
