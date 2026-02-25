class CRF_CameraManagerClass : ScriptComponentClass
{
}

class CRF_CameraManager : ScriptComponent
{
	IEntity m_eCamera;                      // Stores local camera entity for spectator mode
	protected vector m_vStoredCameraPos[4];   // Stores camera transform between sessions
	
	protected static CRF_CameraManager m_sInstance;
	
	protected bool m_bCameraOnRails;
	protected bool m_bTPPMode = false; // True = third-person, false = first-person (helmet cam)
	
	// Orbit camera state (TPP mode)
	protected float m_fOrbitYaw    = 0.0;   // Accumulated horizontal orbit angle (degrees)
	protected float m_fOrbitPitch  = 20.0;  // Accumulated vertical orbit angle (degrees, clamped 5–80)
	protected float m_fOrbitRadius = 4.0;   // Distance from entity in meters (clamped 1.5–20)
	
	protected IEntity m_eCameraEntity;
	protected vector m_vCameraOrbitPoint;
	protected float m_vCameraOrbitDistance;
	protected float m_vCameraOrbitHeight;
	protected PolylineShapeEntity m_CameraPolyLine;
	
	//------------------------------------------------------------------------------------------------
	// STATIC ACCESSORS
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the instance of this component from the player controller
	 * @return CRF_CameraManager - The camera manager component instance or null if unavailable
	 */
	
	void CRF_CameraManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_CameraManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	// METHODS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/*!
	 * Updates stored camera position for persistence between sessions
	 * @param cameraPosToStore - Array of 4 vectors representing camera transform
	 */
	void UpdateStoredCameraPos(vector cameraPosToStoreOne, vector cameraPosToStoreTwo, vector cameraPosToStoreThree, vector cameraPosToStoreFour)
	{
		m_vStoredCameraPos[0] = cameraPosToStoreOne;
		m_vStoredCameraPos[1] = cameraPosToStoreTwo;
		m_vStoredCameraPos[2] = cameraPosToStoreThree;
		m_vStoredCameraPos[3] = cameraPosToStoreFour;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsEntity(IEntity entity, bool tpp = false)
	{
		if (m_eCamera) {
			ClearCameraOnRailsVariables();
			m_eCameraEntity = entity;
			m_bTPPMode = tpp;
			// Initialise orbit yaw to face behind the entity so camera starts at its back
			if (tpp && entity)
			{
				vector angles = entity.GetAngles();
				// angles[0] = yaw (world Y-rotation). Offset 180° so we're behind.
				m_fOrbitYaw = angles[0] + 180.0;
				m_fOrbitPitch = 20.0;
			}
			InitalizeCameraOnRails();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsOrbit(vector point, float distance, float height)
	{	
		if (m_eCamera) {
			ClearCameraOnRailsVariables();
			m_vCameraOrbitPoint = point;
			m_vCameraOrbitDistance = distance;
			m_vCameraOrbitHeight = height;
			InitalizeCameraOnRails();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetCameraOnRailsPolyline(PolylineShapeEntity poly)
	{
		if (m_eCamera) {
			ClearCameraOnRailsVariables();
			m_CameraPolyLine = poly;
			InitalizeCameraOnRails();
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitalizeCameraOnRails()
	{
		m_bCameraOnRails = true;
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void ClearCameraOnRailsVariables()
	{
		m_eCameraEntity = null;
		m_CameraPolyLine = null;
		m_vCameraOrbitPoint = vector.Zero;
		m_vCameraOrbitDistance = 0;
		m_vCameraOrbitHeight = 0;
		m_bTPPMode = false;
		m_fOrbitYaw        = 0.0;
		m_fOrbitPitch      = 20.0;
		m_fOrbitRadius     = 4.0;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	    super.EOnFrame(owner, timeSlice);
		
		if (!m_bCameraOnRails || !m_eCamera)
		{
			ClearCameraOnRailsVariables();
			ClearEventMask(owner, EntityEvent.FRAME);
			return;
		} else {
			switch (true) 
			{
				case (m_vCameraOrbitPoint != vector.Zero) : FrameUpdateOrbit(); break;
				case (m_eCameraEntity) : 
				{
					if (m_bTPPMode) FrameUpdateEntityTPP(timeSlice);
					else FrameUpdateEntity();
					break;
				}
				case (m_CameraPolyLine) : FrameUpdatePolyline(); break;
			}
		};
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	 * Initilizes players if they have a valid spectator entity
	 */
	void InitilizeSpecCamera()
	{
		vector cameraPos[4];
		cameraPos = SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_vPlayersLastDeath;
		
		// Use provided death position if available
		if (CRF_GamemodeManager.IsValidSpawnVector(cameraPos[3])) {
			cameraPos[3][1] = cameraPos[3][1] + 1.5; // Elevate camera slightly above death position
		}
		// Use stored camera position if available
		else if (CRF_GamemodeManager.IsValidSpawnVector(m_vStoredCameraPos[3])) {
			cameraPos = m_vStoredCameraPos;
		} 
		// Fallback to generic spawn position
		else {
			cameraPos[3] = CRF_Gamemode.GetInstance().GetGenericSpawn();
		}
			
		// Set up camera entity
		EntitySpawnParams cameraSpawnParams = new EntitySpawnParams();
		cameraSpawnParams.TransformMode = ETransformMode.WORLD;
		cameraSpawnParams.Transform = cameraPos;

		// Spawn or reposition camera
		if (!m_eCamera)
			m_eCamera = GetGame().SpawnEntityPrefab(Resource.Load("{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et"), GetGame().GetWorld(), cameraSpawnParams);
		else
			m_eCamera.SetWorldTransform(cameraPos);
		
		// Level camera horizon
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
		
		// Switch to spectator camera
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	};
	
	//------------------------------------------------------------------------------------------------
	protected void FrameUpdateEntity()
	{
		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(m_eCameraEntity.FindComponent(SlotManagerComponent));
		if (!slotComp)
			return;
		
		// Get the first-person camera slot
		EntitySlotInfo cameraPoint = slotComp.GetSlotByName("CRF_FPP");
		if (!cameraPoint)
			return;
		
		// Get transform and modify it to be slightly behind and to the right of the player
		vector transform[4];
		cameraPoint.GetTransform(transform);
		
		// Calculate offset position (0.5m back, 0.3m right for over-shoulder view)
		vector offsetPosition = transform[3] - (transform[2] * 0.5) + (transform[0] * 0.3);
		transform[3] = offsetPosition;
		
		// Apply transform to spectator camera
		m_eCamera.SetTransform(transform);
	};
	
	//------------------------------------------------------------------------------------------------
	/*!
	 * Third-person orbit camera: orbits around the entity using mouse look input.
	 * Rotation only occurs while RMB (ManualCameraRotate) is held.
	 * ManualCameraRotateYaw / ManualCameraRotatePitch provide angular deltas (degrees/s at timeSlice=1).
	 * Pitch is clamped 5–80 degrees. Radius is fixed at m_fOrbitRadius.
	 * Scroll wheel while RMB is held (ManualCameraSpeedAdjust) zooms the orbit radius in/out.
	 * Radius is clamped 1.5–20 meters.
	 */
	protected void FrameUpdateEntityTPP(float timeSlice)
	{
		if (!m_eCameraEntity || !m_eCamera)
			return;

		InputManager im = GetGame().GetInputManager();

		// Only rotate/zoom while RMB is held (ManualCameraRotate action)
		if (im.GetActionValue("ManualCameraRotate") != 0)
		{
			float rawYaw   = im.GetActionValue("ManualCameraRotateYaw");
			float rawPitch = im.GetActionValue("ManualCameraRotatePitch");

			// Values are angular deltas; scale by timeSlice for frame-rate independence.
			// Speed constant matches SCR_RotateManualCameraComponent default (90 deg/s).
			const float SPEED = 90.0;
			m_fOrbitYaw   += rawYaw   * SPEED * timeSlice;
			m_fOrbitPitch  = Math.Clamp(m_fOrbitPitch - rawPitch * SPEED * timeSlice, 5.0, 80.0);

			// Scroll wheel while RMB held: ManualCameraSpeedAdjust gives a signed
			// per-frame impulse (positive = scroll up). Multiply radius by it to zoom.
			float zoomInput = im.GetActionValue("ManualCameraSpeedAdjust");
			if (zoomInput != 0)
			{
				// zoomInput is a rate value; scale so one notch (~1.0 unit) moves radius by 1 m/s
				const float ZOOM_SPEED = 8.0;
				m_fOrbitRadius = Math.Clamp(m_fOrbitRadius - zoomInput * ZOOM_SPEED * timeSlice, 1.5, 20.0);
			}
		}

		// --- Build camera position on sphere around entity -------------------
		// Pivot at entity head height
		vector entityPos = m_eCameraEntity.GetOrigin() + Vector(0, 1.0, 0);

		float yawRad   = m_fOrbitYaw   * Math.DEG2RAD;
		float pitchRad = m_fOrbitPitch * Math.DEG2RAD;

		float cosPitch = Math.Cos(pitchRad);
		vector offset = Vector(
			m_fOrbitRadius * cosPitch * Math.Sin(yawRad),
			m_fOrbitRadius * Math.Sin(pitchRad),
			m_fOrbitRadius * cosPitch * Math.Cos(yawRad)
		);
		vector camPos = entityPos + offset;

		// --- Build look-at transform -----------------------------------------
		vector lookDir = vector.Direction(camPos, entityPos);
		lookDir.Normalize();

		vector worldUp = Vector(0, 1, 0);
		vector right   = lookDir * worldUp; // forward × up (cross product)
		right.Normalize();
		vector up = right * lookDir; // right × forward
		up.Normalize();

		vector camTransform[4];
		camTransform[0] = right;
		camTransform[1] = up;
		camTransform[2] = lookDir;
		camTransform[3] = camPos;
		m_eCamera.SetTransform(camTransform);
	}

	//------------------------------------------------------------------------------------------------
	protected void FrameUpdateOrbit()
	{
	
	}
	
	//------------------------------------------------------------------------------------------------
	protected void FrameUpdatePolyline()
	{
	
	}
}