//------------------------------------------------------------------------------------------------
//! Tank Battle: the third person camera in a mode tank (CRF_TankBattleGamemodeComponent, Camera settings)
//! - further back (Camera Distance Scale), higher (Camera Extra Height) and wider (Camera Extra FOV)
//! - looking at where the gun points (Camera Look At Aim): vanilla looks along the gun, so from
//!   above and behind it the aim point sits behind the tank. Here the view is turned, through the
//!   camera's pivot, at the gun's aim point (CRF_TankBattleGamemodeComponent.GetLocalGunRay), so the
//!   aim point is in the middle of the screen at any range: the engine puts the camera m_fDistance
//!   back from the pivot along its view. The aim distance is smoothed, so the view doesn't jump when
//!   the gun sweeps from the horizon onto something close. Free look (Alt) is left alone.
//! Outside a mode tank nothing changes. The camera is registered by type in CharacterCameraSet
//! (RegisterCameraCreator), not built from a config, so a plain modded class is enough.
modded class CharacterCamera3rdPersonTurret
{
	protected static const float TANK_MIN_AIM_DISTANCE = 25;	//!< m: closer aim points are treated as this far
	protected static const float TANK_AIM_SMOOTHING = 0.12;	//!< per update, towards the traced distance

	protected bool m_bTankAimCamera;
	protected float m_fTankAimDistance = -1;

	//------------------------------------------------------------------------------------------------
	override void InitCameraData()
	{
		super.InitCameraData();

		m_bTankAimCamera = CRF_TankBattleGamemodeComponent.IsCameraLookAtAim(m_OwnerCharacter);
		m_fTankAimDistance = -1;

		float scale = CRF_TankBattleGamemodeComponent.GetCameraDistanceScale(m_OwnerCharacter);
		if (scale > 1)
		{
			m_fDist_Desired *= scale;
			m_fDist_Min *= scale;
			m_fDist_Max *= scale;
			m_fCamDist = m_fDist_Desired;
			m_fCamDistTarget = m_fDist_Desired;
			m_fZoomTargetWidth = m_fCamDist * 2 * Math.Tan(0.5 * m_fFOV * Math.DEG2RAD);
		}

		m_fHeight += CRF_TankBattleGamemodeComponent.GetCameraExtraHeight(m_OwnerCharacter);

		// Vanilla tilts the view after aiming it (third person angle, vehicle pitch) and shifts it with
		// speed (aimpoint curve): either would move the aim point off the middle of the screen
		if (m_bTankAimCamera)
		{
			m_fAngleThirdPerson = 0;
			m_fPitchFactor = 0;
			m_pCameraAimpointData = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	override float GetBaseFOV()
	{
		return super.GetBaseFOV() + CRF_TankBattleGamemodeComponent.GetCameraExtraFov(m_OwnerCharacter);
	}

	//------------------------------------------------------------------------------------------------
	override void CalculateLookAngles(vector vehicleAngles, vector characterAngles, out ScriptedCameraItemResult pOutResult)
	{
		super.CalculateLookAngles(vehicleAngles, characterAngles, pOutResult);

		if (!m_bTankAimCamera || !m_OwnerVehicle)
			return;

		if (m_ControllerComponent && m_ControllerComponent.IsFreeLookEnabled())
			return;

		vector origin;
		vector direction;
		float distance;
		if (!CRF_TankBattleGamemodeComponent.GetCameraAimRay(m_OwnerCharacter, origin, direction, distance))
			return;

		if (m_fTankAimDistance < 0)
			m_fTankAimDistance = distance;
		else
			m_fTankAimDistance = Math.Lerp(m_fTankAimDistance, distance, TANK_AIM_SMOOTHING);

		vector aimPoint = origin + direction * Math.Max(m_fTankAimDistance, TANK_MIN_AIM_DISTANCE);

		// The pivot as vanilla's OnUpdate places it: the camera centre (vehicle space), raised by the height
		vector vehicleTransform[4];
		m_OwnerVehicle.GetTransform(vehicleTransform);
		vector pivot = m_vCameraCenter.Multiply4(vehicleTransform) + vehicleTransform[1] * m_fHeight;

		vector look = aimPoint - pivot;
		if (look.LengthSq() < 1)
			return;

		vector angles = look.VectorToAngles();
		angles[2] = 0;
		m_vLastCameraAngles = angles;
		Math3D.AnglesToMatrix(angles - characterAngles, pOutResult.m_CameraTM);
	}
}
