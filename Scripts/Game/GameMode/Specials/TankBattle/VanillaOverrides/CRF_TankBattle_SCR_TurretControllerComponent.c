//------------------------------------------------------------------------------------------------
//! Tank Battle: vanilla turns a turret on W/A/S/D too (TurretRotate*, in TurretContext). The driver
//! controls the main gun without that context (CRF_TankBattleGamemodeComponent.AttachMainTurret), so
//! W/A/S/D only drive; this zeroes those keys as well, in case they still reach the turret.
//! The mouse (TurretAim*) still aims.
modded class SCR_TurretControllerComponent
{
	//------------------------------------------------------------------------------------------------
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		super.OnPrepareControls(owner, am, dt, player);

		if (!player || !CRF_TankBattleGamemodeComponent.IsLocalTankTurret(this))
			return;

		am.SetActionValue("TurretRotateUp", 0);
		am.SetActionValue("TurretRotateDown", 0);
		am.SetActionValue("TurretRotateLeft", 0);
		am.SetActionValue("TurretRotateRight", 0);
	}
}
