modded class GC_SuppressionSystem
{
	//------------------------------------------------------------------------------------------------
	//! Override: Prevents projectiles from being registered for suppression when the local player is a spectator
	override void RegisterProjectile(IEntity projectile)
	{
		IEntity localEntity = GetGame().GetPlayerController().GetControlledEntity();
		if (localEntity && COA_EntityHelper.IsSpectator(localEntity))
			return;

		super.RegisterProjectile(projectile);
	}

	//------------------------------------------------------------------------------------------------
	//! Override: Prevents suppression from being applied when the local player is a spectator
	override protected void AddSuppression(float suppression)
	{
		IEntity localEntity = GetGame().GetPlayerController().GetControlledEntity();
		if (localEntity && COA_EntityHelper.IsSpectator(localEntity))
			return;

		super.AddSuppression(suppression);
	}

	//------------------------------------------------------------------------------------------------
	//! Override: Resets suppression immediately when the controlled entity changes (death, respawn, spectator transition)
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		SetEnabled(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Override: Ensures any lingering suppression is cleared while the player has no valid entity or is in spectator
	override protected void OnUpdate(WorldSystemPoint point)
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			super.OnUpdate(point);
			return;
		}

		IEntity localEntity = pc.GetControlledEntity();
		if (!localEntity || COA_EntityHelper.IsSpectator(localEntity))
		{
			if (GetAmount() > 0)
				SetEnabled(true);
			return;
		}

		super.OnUpdate(point);
	}
}
