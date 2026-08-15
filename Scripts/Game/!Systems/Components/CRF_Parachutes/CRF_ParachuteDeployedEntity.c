class CRF_ParachuteDeployedEntityClass : GenericEntityClass {}

class CRF_ParachuteDeployedEntity : GenericEntity
{
	protected RplComponent m_RplComponent;
	protected Physics m_Physics;
	protected BaseCompartmentSlot m_CargoSlot;
	protected SCR_BaseCompartmentManagerComponent m_CompartmentManager;

	protected IEntity m_PilotCharacter;
	protected SCR_CompartmentAccessComponent m_PilotAccess;
	protected bool m_IsPilotAccessHooked;

	// Initial velocity set by player component at spawn
	protected vector m_InitialVelocity;
	protected bool m_VelocityApplied;

	// Weather and wind
	protected TimeAndWeatherManagerEntity m_WeatherManager;
	protected float m_WindDirDeg;
	protected float m_WindSpeed;

	// Steering: input state
	protected InputManager m_InputManager;
	protected bool m_ControlsEnabled;
	protected float m_fInputPitch;
	protected float m_fInputRoll;

	// Steering: input as received by authority from a remote owner
	protected float m_NetInputPitch;
	protected float m_NetInputRoll;
	protected float m_InputTimeSinceRecv;

	// Steering: input send throttling (owner -> server)
	protected float m_InputAccTime;
	protected float m_LastSentPitch;
	protected float m_LastSentRoll;

	// Steering: cached per-tick physics state
	protected vector m_vWorldTransform[4];
	protected vector m_vVelocity;
	protected vector m_vAngularVelocity;
	protected float m_fForwardSpeed;
	protected float m_fAccel;
	protected float m_fSmoothAccel;

	protected const float HEADING_LERP_RATE = 2.5;

	[Attribute("4.0", UIWidgets.Slider, "Max fall speed (m/s)", "1 20 0.1")]
	protected float m_MaxFallSpeed = 4.0;

	[Attribute("2.0", UIWidgets.Slider, "Drag strength to limit fall speed", "0 20 0.1")]
	protected float m_DragStrength = 2.0;

	// Steering settings
	[Attribute("60", UIWidgets.Slider, "Pitch torque", "1 200 1", category: "Steering")]
	protected float m_PitchTorque = 60.0;

	[Attribute("60", UIWidgets.Slider, "Roll torque", "1 200 1", category: "Steering")]
	protected float m_RollTorque = 60.0;

	[Attribute("200", UIWidgets.Slider, "Auto-level proportional gain", "0 500 1", category: "Steering")]
	protected float m_LevelPropGain = 200.0;

	[Attribute("20", UIWidgets.Slider, "Auto-level damping", "0 50 0.1", category: "Steering")]
	protected float m_LevelDampening = 20.0;

	[Attribute("2", UIWidgets.Slider, "Auto-level power", "0.1 3.0 0.1", category: "Steering")]
	protected float m_LevelPower = 2.0;

	[Attribute("45", UIWidgets.Slider, "Max turn rate (deg/s)", "0 90 1", category: "Steering")]
	protected float m_MaxTurnRate = 45.0;

	[Attribute("4.0", UIWidgets.Slider, "Turn proportional gain", "0 20 0.1", category: "Steering")]
	protected float m_TurnPropGain = 4.0;

	[Attribute("1.0", UIWidgets.Slider, "Turn damping", "0 10 0.1", category: "Steering")]
	protected float m_TurnDampening = 1.0;

	[Attribute("8", UIWidgets.Slider, "Min bank angle to turn (deg)", "0 45 1", category: "Steering")]
	protected float m_MinBankAngle = 8.0;

	[Attribute("0.1", UIWidgets.Slider, "Min pitch input to turn", "0 1 0.01", category: "Steering")]
	protected float m_MinPitchInput = 0.1;

	[Attribute("4.0", UIWidgets.Slider, "Glide accel, pitch down", "0 20 0.1", category: "Steering")]
	protected float m_GlideDownPitch = 4.0;

	[Attribute("5.0", UIWidgets.Slider, "Glide decel, pitch up", "0 20 0.1", category: "Steering")]
	protected float m_GlideUpPitch = 5.0;

	[Attribute("0.0", UIWidgets.Slider, "Min forward speed", "0 10 0.1", category: "Steering")]
	protected float m_MinForwardSpeed = 0.0;

	[Attribute("21.0", UIWidgets.Slider, "Max forward speed", "0 50 0.1", category: "Steering")]
	protected float m_MaxForwardSpeed = 21.0;

	[Attribute("20", UIWidgets.Slider, "Input send rate (hz)", "1 60 1", category: "Steering")]
	protected float m_InputSendHz = 20.0;

	[Attribute("0.01", UIWidgets.Slider, "Input change threshold", "0 0.5 0.01", category: "Steering")]
	protected float m_InputChangeThreshold = 0.01;

	[Attribute("0.30", UIWidgets.Slider, "Input timeout (s)", "0 2 0.05", category: "Steering")]
	protected float m_InputTimeoutSec = 0.30;

	// Flare settings
	[Attribute("10.0", UIWidgets.Slider, "Flare start height (m)", "0 50 1")]
	protected float m_FlareStartHeight = 10.0;

	[Attribute("1.0", UIWidgets.Slider, "Flare end height (m)", "0 10 0.1")]
	protected float m_FlareEndHeight = 1.0;

	[Attribute("10.0", UIWidgets.Slider, "Max flare deceleration (m/s²)", "0 30 0.5")]
	protected float m_MaxFlareDeceleration = 10.0;

	[Attribute("0.5", UIWidgets.Slider, "Ground detection extra offset (m)", "0 2 0.1")]
	protected float m_GroundCheckOffset = 0.5;

	protected bool m_HasLanded;

	// Network sync
	[Attribute("10", UIWidgets.Slider, "Network sync interval (hz)", "1 60 1")]
	protected float m_NetworkSyncHz = 10;
	protected float m_NetSendInterval;
	protected float m_NetAccTime;

	// Interpolation for non-owners
	protected vector m_TargetPos;
	protected vector m_TargetAngles;
	protected vector m_TargetVel;
	protected float m_InterpFactor;

	// --------------------------------------------------------------------------------------------
	// Initialization
	// --------------------------------------------------------------------------------------------

	void CRF_ParachuteDeployedEntity(IEntitySource src, IEntity parent)
	{
		// Entities spawned at runtime (e.g. via SpawnEntityPrefabEx) only receive EOnInit
		// if the INIT event is requested here. Without this, m_RplComponent/m_Physics are
		// never assigned, IsAuthority()/IsOwner() always return false, and the SIMULATE/
		// CONTACT/FRAME masks below never get registered either - which is why this never
		// worked once the chute was spawned on a dedicated server.
		SetEventMask(EntityEvent.INIT);
	}

	bool IsAuthority()
	{
		return m_RplComponent && m_RplComponent.Role() == RplRole.Authority;
	}

	bool IsOwner()
	{
		return m_RplComponent && m_RplComponent.IsOwner();
	}

	RplId GetRplId()
	{
		if (m_RplComponent)
			return m_RplComponent.Id();
		return RplId.Invalid();
	}

	override void EOnInit(IEntity owner)
	{
		if (SCR_Global.IsEditMode())
			return;

		m_RplComponent = RplComponent.Cast(FindComponent(RplComponent));
		m_Physics = GetPhysics();
		m_InputManager = GetGame().GetInputManager();
		m_WeatherManager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetTimeAndWeatherManager();

		m_CompartmentManager = SCR_BaseCompartmentManagerComponent.Cast(FindComponent(SCR_BaseCompartmentManagerComponent));
		if (m_CompartmentManager)
		{
			array<BaseCompartmentSlot> slots = {};
			m_CompartmentManager.GetCompartments(slots);
			foreach (BaseCompartmentSlot s : slots)
			{
				if (s && s.GetType() == ECompartmentType.CARGO)
				{
					m_CargoSlot = s;
					break;
				}
			}
		}

		if (IsAuthority() && m_Physics && m_InitialVelocity != vector.Zero && !m_VelocityApplied)
		{
			m_Physics.SetVelocity(m_InitialVelocity);
			m_VelocityApplied = true;
		}

		// Network sync interval
		m_NetSendInterval = 1.0 / Math.Max(m_NetworkSyncHz, 1.0);
		m_NetAccTime = 0;

		SetEventMask(EntityEvent.SIMULATE | EntityEvent.CONTACT | EntityEvent.FRAME); // FRAME is for interpolation
	}

	override void EOnDeactivate(IEntity owner)
	{
		UnhookPilotExit();
		DisableControls();
	}

	// --------------------------------------------------------------------------------------------
	// Pilot linking and exit hook
	// --------------------------------------------------------------------------------------------

	void SetPilotAndHook(IEntity pilot, SCR_CompartmentAccessComponent access)
	{
		m_PilotCharacter = pilot;
		m_PilotAccess = access;
		if (m_PilotAccess && !m_IsPilotAccessHooked)
		{
			m_PilotAccess.GetOnCompartmentLeft().Insert(OnPilotLeftCompartment);
			m_IsPilotAccessHooked = true;
		}
	}

	void SetInitialVelocity(vector vel)
	{
		m_InitialVelocity = vel;
		if (IsAuthority() && m_Physics && !m_VelocityApplied)
		{
			m_Physics.SetVelocity(vel);
			m_VelocityApplied = true;
		}
	}

	protected void UnhookPilotExit()
	{
		if (!m_IsPilotAccessHooked || !m_PilotAccess)
			return;

		m_PilotAccess.GetOnCompartmentLeft().Remove(OnPilotLeftCompartment);
		m_IsPilotAccessHooked = false;
	}

	protected void OnPilotLeftCompartment(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		RequestExit();
	}

	// --------------------------------------------------------------------------------------------
	// Simulation (runs on authority and owner client for prediction)
	// --------------------------------------------------------------------------------------------

	override void EOnSimulate(IEntity owner, float timeSlice)
	{
		// Skip if landed or no physics
		if (m_HasLanded || !m_Physics)
			return;

		// Run simulation on authority (server) AND on owner client for prediction
		// Non-owner clients rely on interpolation only
		if (!IsAuthority() && !IsOwner())
			return;

		if (!m_VelocityApplied && m_InitialVelocity != vector.Zero)
		{
			m_Physics.SetVelocity(m_InitialVelocity);
			m_VelocityApplied = true;
		}

		// Cache current transform/velocity - steering handlers below all read these
		GetWorldTransform(m_vWorldTransform);
		m_vVelocity = m_Physics.GetVelocity();
		m_vAngularVelocity = m_Physics.GetAngularVelocity();

		// Resolve pitch/roll input: the owner uses local player input directly (set by
		// SetPitch/SetRoll below); authority uses whatever a remote owner last reported
		// over RpcAsk_Input, and zeroes it out if that input goes stale.
		if (!IsOwner())
		{
			m_InputTimeSinceRecv += timeSlice;
			if (m_InputTimeSinceRecv > m_InputTimeoutSec)
			{
				m_fInputPitch = 0.0;
				m_fInputRoll = 0.0;
			}
			else
			{
				m_fInputPitch = Math.Clamp(m_NetInputPitch, -1.0, 1.0);
				m_fInputRoll = Math.Clamp(m_NetInputRoll, -1.0, 1.0);
			}
		}

		// Steering: canopy orientation, self-leveling, and bank-to-turn
		HandlePitch(timeSlice);
		HandleRoll(timeSlice);
		HandleAutoLevel(timeSlice);
		HandleBankTurn(timeSlice);

		// Forward glide speed from pitch input; updates horizontal velocity and
		// carries the existing vertical speed through to the drag/flare logic below
		HandleGlide(timeSlice);

		vector vel = m_Physics.GetVelocity();
		float downwardSpeed = -vel[1];

		// Limit downward speed with drag
		if (downwardSpeed > m_MaxFallSpeed)
		{
			float excess = downwardSpeed - m_MaxFallSpeed;
			float neededAccel = excess / timeSlice;
			neededAccel = Math.Min(neededAccel, m_DragStrength);
			float impulse = neededAccel * m_Physics.GetMass() * timeSlice;
			m_Physics.ApplyImpulse(vector.Up * impulse);
		}

		// Apply wind (only authority, wind is global)
		if (IsAuthority())
			HandleWeather(timeSlice);

		// Ground flare: aggressively slow descent when near ground
		HandleGroundFlare(timeSlice);

		// Ground detection
		vector pos = GetOrigin();
		float groundY = SCR_TerrainHelper.GetTerrainY(pos, null, true);
		float altitude = pos[1] - groundY;
		if (altitude < m_GroundCheckOffset)
		{
			if (m_CargoSlot && m_CargoSlot.IsOccupied())
				RequestExit();
		}

		// Network sync (only authority sends)
		if (IsAuthority())
		{
			m_NetAccTime += timeSlice;
			if (m_NetAccTime >= m_NetSendInterval)
			{
				m_NetAccTime = 0;
				SendSync();
			}
		}

		// Controls & input networking - owner client only. A listen-host that is both
		// owner and authority reads its own m_fInputPitch/Roll directly above and never
		// needs to send it anywhere; a remote owner must relay input to the server.
		if (IsOwner())
		{
			if (m_CargoSlot && m_CargoSlot.IsOccupied())
			{
				EnableControls();
				if (!IsAuthority())
					SendInputIfChanged(timeSlice);
			}
			else
			{
				DisableControls();
			}
		}
	}

	override void EOnContact(IEntity owner, IEntity other, Contact contact)
	{
		if (!IsAuthority() || m_HasLanded)
			return;
		if (m_CargoSlot && m_CargoSlot.IsOccupied())
			RequestExit();
	}

	void RequestExit()
	{
		if (m_HasLanded)
			return;
		m_HasLanded = true;
		DisableControls();

		// Small delay before exit to allow flare to finish
		GetGame().GetCallqueue().CallLater(DoRequestExit, 100, false);
	}

	void DoRequestExit()
	{
		float velocityAtExit = -m_Physics.GetVelocity()[1];
		Rpc(Rpc_RequestExit, velocityAtExit);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void Rpc_RequestExit(float velocityAtExit)
	{
		if (!m_CargoSlot)
			return;

		IEntity occupant = m_CargoSlot.GetOccupant();
		if (!occupant)
			return;

		PlayerManager pm = GetGame().GetPlayerManager();
		int playerId = pm.GetPlayerIdFromControlledEntity(occupant);
		if (playerId == 0)
			return;

		SCR_PlayerController pc = SCR_PlayerController.Cast(pm.GetPlayerController(playerId));
		if (!pc)
			return;

		CRF_ParachutePlayerComponent playerComp = CRF_ParachutePlayerComponent.Cast(pc.FindComponent(CRF_ParachutePlayerComponent));
		if (!playerComp)
			return;

		playerComp.Rpc_RequestExit(GetRplId(), velocityAtExit);
	}

	// --------------------------------------------------------------------------------------------
	// Aggressive Ground Flare
	// --------------------------------------------------------------------------------------------

	void HandleGroundFlare(float timeSlice)
	{
		vector pos = GetOrigin();
		float terrainY = SCR_TerrainHelper.GetTerrainY(pos, null, true);
		float height = pos[1] - terrainY;

		// Too high, no flare
		if (height > m_FlareStartHeight)
			return;

		// Normalized factor: 0 at start height, 1 at end height
		float t = 1.0 - (height - m_FlareEndHeight) / (m_FlareStartHeight - m_FlareEndHeight);
		t = Math.Clamp(t, 0.0, 1.0);

		// Apply upward impulse to kill downward velocity
		float decel = t * m_MaxFlareDeceleration;
		float impulse = decel * m_Physics.GetMass() * timeSlice;
		m_Physics.ApplyImpulse(vector.Up * impulse);
	}

	// --------------------------------------------------------------------------------------------
	// Steering
	// --------------------------------------------------------------------------------------------

	// Pitch with forward/back input
	protected void HandlePitch(float timeSlice)
	{
		vector axisWorld = VectorToParent(vector.Right);
		float torque = m_fInputPitch * m_PitchTorque;
		m_Physics.ApplyTorque(axisWorld * torque);
	}

	// Roll with left/right input
	protected void HandleRoll(float timeSlice)
	{
		vector axisWorld = VectorToParent(vector.Forward);
		float torque = -m_fInputRoll * m_RollTorque;
		m_Physics.ApplyTorque(axisWorld * torque);
	}

	// Converts bank angle + forward pull into a yaw rate, so banking the canopy turns it
	void HandleBankTurn(float timeSlice)
	{
		vector ang = Math3D.MatrixToAngles(m_vWorldTransform);
		float rollDeg = ang[2];

		if (Math.AbsFloat(rollDeg) < m_MinBankAngle)
			return;
		if (m_fInputPitch < m_MinPitchInput)
			return;

		float bankFactor = Math.Sin(rollDeg * Math.DEG2RAD);
		float pitchFactor = Math.Clamp(m_fInputPitch, 0, 1);
		float yawRateDes = bankFactor * pitchFactor * m_MaxTurnRate;

		vector yawAxisWorld = VectorToParent(vector.Up);
		float yawRateCur = vector.Dot(m_vAngularVelocity, yawAxisWorld) * Math.RAD2DEG;

		float rateError = yawRateDes - yawRateCur;
		float torque = rateError * m_TurnPropGain - yawRateCur * m_TurnDampening;

		m_Physics.ApplyTorque(yawAxisWorld * torque);
	}

	// Keeps the canopy roughly upright when there's no roll input fighting it
	void HandleAutoLevel(float timeSlice)
	{
		vector worldUp = vector.Up;
		vector actualUp = m_vWorldTransform[1];

		vector errorAxis = VecCross(actualUp, worldUp);
		float errorMag = errorAxis.Length();

		if (errorMag < 0.001)
			return;
		errorAxis.Normalize();

		float kpEff = m_LevelPropGain * Math.Pow(errorMag, m_LevelPower);
		float errorVel = vector.Dot(m_vAngularVelocity, errorAxis);

		vector correctiveTorque =
			errorAxis * (errorMag * kpEff) -
			errorAxis * (errorVel * m_LevelDampening);

		m_Physics.ApplyTorque(correctiveTorque);
	}

	vector VecCross(vector a, vector b)
	{
		return {
			a[1] * b[2] - a[2] * b[1],
			a[2] * b[0] - a[0] * b[2],
			a[0] * b[1] - a[1] * b[0]};
	}

	// Integrates forward glide speed from pitch input and steers heading toward the
	// canopy's nose. Sets full velocity (horizontal from glide, vertical carried over
	// from m_vVelocity) - the drag/flare logic that runs right after this call still
	// adjusts the vertical component via impulses on top of what this produces.
	void HandleGlide(float timeSlice)
	{
		vector forwardW = VectorToParent(vector.Forward);
		vector upW = vector.Up;

		float pitchDot = vector.Dot(forwardW, upW);
		float normPitch = Math.Sin(Math.Asin(-pitchDot));

		float forwardSpeed = m_fForwardSpeed;
		float verticalSpeed = m_vVelocity[1];

		if (normPitch > 0.03)
			m_fAccel = normPitch * m_GlideDownPitch;
		else if (Math.AbsFloat(normPitch) <= 0.03)
			m_fAccel = m_GlideDownPitch * 0.33;
		else
			m_fAccel = normPitch * m_GlideUpPitch;

		float smoothing = Math.Clamp(0.5 * timeSlice, 0.0, 0.04);
		m_fSmoothAccel += (m_fAccel - m_fSmoothAccel) * smoothing;

		float prevSpeed = forwardSpeed;
		forwardSpeed = Math.Clamp(forwardSpeed + m_fSmoothAccel * timeSlice, m_MinForwardSpeed, m_MaxForwardSpeed);

		if (m_fSmoothAccel < 0)
		{
			float maxDrop = 1.5 * timeSlice;
			float actual = prevSpeed - forwardSpeed;
			if (actual > maxDrop)
				forwardSpeed = prevSpeed - maxDrop;
		}

		if (normPitch < -0.12 && prevSpeed > m_MinForwardSpeed + 1.5)
		{
			float liftFactor = Math.Clamp(-normPitch, 0.0, 1.0);
			float liftConvert = Math.Min(prevSpeed - m_MinForwardSpeed, 2.0) * liftFactor * timeSlice * 0.8;
			verticalSpeed += liftConvert;
			forwardSpeed -= liftConvert * 0.7;
			forwardSpeed = Math.Max(forwardSpeed, m_MinForwardSpeed);
		}

		vector horizVel = m_vVelocity;
		horizVel[1] = 0;
		float horizLen = horizVel.Length();

		vector curDir;
		if (horizLen > 0.01)
			curDir = horizVel / horizLen;
		else
			curDir = forwardW.Normalized();

		vector targetDir = forwardW;
		targetDir[1] = 0;
		float tgtLen = targetDir.Length();
		if (tgtLen > 0.001)
			targetDir /= tgtLen;
		else
			targetDir = curDir;

		float lerpAlpha = Math.Clamp(HEADING_LERP_RATE * timeSlice, 0, 1);
		vector newDir = curDir + (targetDir - curDir) * lerpAlpha;
		newDir.Normalize();

		vector newHorizVel = newDir * forwardSpeed;
		vector newVel = {newHorizVel[0], verticalSpeed, newHorizVel[2]};

		m_Physics.SetVelocity(newVel);
		m_vVelocity = newVel;
		m_fForwardSpeed = forwardSpeed;
	}

	// --------------------------------------------------------------------------------------------
	// Steering: input capture (owner client) & networking (owner -> server)
	// --------------------------------------------------------------------------------------------

	void EnableControls()
	{
		if (!m_InputManager)
			m_InputManager = GetGame().GetInputManager();
		if (!m_InputManager)
			return;

		if (!m_InputManager.IsContextActive("CharacterMovementContext"))
			m_InputManager.ActivateContext("CharacterMovementContext");

		if (m_ControlsEnabled)
			return;

		// Only the owning client should read local input; a non-owner authority
		// (dedicated server) relies on RpcAsk_Input instead.
		if (IsAuthority() && !IsOwner())
			return;

		m_ControlsEnabled = true;

		// Character context (on-foot style)
		m_InputManager.AddActionListener("CharacterForward", EActionTrigger.VALUE, SetPitch);
		m_InputManager.AddActionListener("CharacterRight", EActionTrigger.VALUE, SetRoll);

		// Vehicle context fallback (some contexts stop emitting CharacterForward/Right)
		m_InputManager.AddActionListener("VehicleThrottle", EActionTrigger.VALUE, SetPitch);
		m_InputManager.AddActionListener("VehicleSteer", EActionTrigger.VALUE, SetRoll);
	}

	void DisableControls()
	{
		if (!m_ControlsEnabled)
			return;
		m_ControlsEnabled = false;

		if (!m_InputManager)
			return;

		m_InputManager.RemoveActionListener("CharacterForward", EActionTrigger.VALUE, SetPitch);
		m_InputManager.RemoveActionListener("CharacterRight", EActionTrigger.VALUE, SetRoll);
		m_InputManager.RemoveActionListener("VehicleThrottle", EActionTrigger.VALUE, SetPitch);
		m_InputManager.RemoveActionListener("VehicleSteer", EActionTrigger.VALUE, SetRoll);

		// Prevent "sticky" input from a listener removed mid-press
		m_fInputPitch = 0.0;
		m_fInputRoll = 0.0;
	}

	void SetPitch(float value = 0.0, EActionTrigger reason = 0, string actionName = string.Empty)
	{
		if (!m_ControlsEnabled)
			return;
		m_fInputPitch = value;
	}

	void SetRoll(float value = 0.0, EActionTrigger reason = 0, string actionName = string.Empty)
	{
		if (!m_ControlsEnabled)
			return;
		m_fInputRoll = value;
	}

	// Sends the owner's local input to the authoritative server so its simulation
	// (and everyone else who receives its broadcast sync) actually reflects steering.
	protected void SendInputIfChanged(float timeSlice)
	{
		m_InputAccTime += timeSlice;

		float pitch = Math.Clamp(m_fInputPitch, -1.0, 1.0);
		float roll = Math.Clamp(m_fInputRoll, -1.0, 1.0);

		bool changed = (Math.AbsFloat(pitch - m_LastSentPitch) > m_InputChangeThreshold) ||
					   (Math.AbsFloat(roll - m_LastSentRoll) > m_InputChangeThreshold);

		float sendInterval = 1.0 / Math.Max(m_InputSendHz, 1.0);
		if (m_InputAccTime >= sendInterval && (changed || m_InputAccTime >= sendInterval * 3.0))
		{
			m_InputAccTime = 0;
			m_LastSentPitch = pitch;
			m_LastSentRoll = roll;
			Rpc(RpcAsk_Input, pitch, roll);
		}
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	void RpcAsk_Input(float pitch, float roll)
	{
		if (!IsAuthority())
			return;
		if (!m_CargoSlot || !m_CargoSlot.IsOccupied())
			return;

		m_NetInputPitch = Math.Clamp(pitch, -1.0, 1.0);
		m_NetInputRoll = Math.Clamp(roll, -1.0, 1.0);
		m_InputTimeSinceRecv = 0.0;
	}

	// --------------------------------------------------------------------------------------------
	// Network Synchronization with Deletion Protection
	// --------------------------------------------------------------------------------------------

	void SendSync()
	{
		if (!IsAuthority())
			return;

		vector transform[4];
		GetWorldTransform(transform);
		vector vel = m_Physics.GetVelocity();
		vector angVel = m_Physics.GetAngularVelocity();

		Rpc(RpcDo_SyncMovement, transform, vel, angVel, m_WindDirDeg, m_WindSpeed);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	void RpcDo_SyncMovement(vector transform[4], vector vel, vector angVel, float windDirDeg, float windSpeed)
	{
		// Ignore if we are the authority or if the entity has already landed/deleted
		if (IsAuthority() || m_HasLanded || !m_Physics)
			return;

		m_WindDirDeg = windDirDeg;
		m_WindSpeed = windSpeed;

		// Simple interpolation: store target and apply linearly over time
		m_TargetPos = transform[3];
		m_TargetAngles = Math3D.MatrixToAngles(transform);
		m_TargetVel = vel;
		m_InterpFactor = 0; // will be increased each frame

		// If the difference is large, snap
		vector curPos = GetOrigin();
		if (vector.Distance(m_TargetPos, curPos) > 5.0)
		{
			SetWorldTransform(transform);
			m_Physics.SetVelocity(vel);
			m_Physics.SetAngularVelocity(angVel);
		}
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Skip if landed, deleted, or if we are the owner/authority
		if (m_HasLanded || !m_Physics || IsAuthority() || IsOwner())
			return;

		if (m_InterpFactor < 1.0)
		{
			m_InterpFactor += timeSlice * 5; // interpolation speed (5 per second)
			if (m_InterpFactor > 1.0) m_InterpFactor = 1.0;

			// Interpolate position and orientation
			vector curPos = GetOrigin();
			vector newPos = Lerp(curPos, m_TargetPos, m_InterpFactor);

			// Get current transform
			vector curTransform[4];
			GetWorldTransform(curTransform);
			vector curAng = Math3D.MatrixToAngles(curTransform);
			vector newAng = LerpAngles(curAng, m_TargetAngles, m_InterpFactor);

			vector newTransform[4];
			Math3D.AnglesToMatrix(newAng, newTransform);
			newTransform[3] = newPos;
			SetWorldTransform(newTransform);

			// Interpolate velocity
			vector curVel = m_Physics.GetVelocity();
			vector newVel = Lerp(curVel, m_TargetVel, m_InterpFactor);
			m_Physics.SetVelocity(newVel);
		}
	}

	vector Lerp(vector a, vector b, float t)
	{
		return a + (b - a) * t;
	}

	vector LerpAngles(vector a, vector b, float t)
	{
		vector result;
		for (int i = 0; i < 3; i++)
		{
			float diff = b[i] - a[i];
			if (diff > 180) diff -= 360;
			if (diff < -180) diff += 360;
			result[i] = a[i] + diff * t;
		}
		return result;
	}

	// --------------------------------------------------------------------------------------------
	// Wind Simulation
	// --------------------------------------------------------------------------------------------

	void HandleWeather(float timeSlice)
	{
		if (!m_WeatherManager)
			return;

		m_WindDirDeg = m_WeatherManager.GetWindDirection();
		m_WindSpeed = m_WeatherManager.GetWindSpeed();

		if (m_WindSpeed == 0)
			return;

		vector windDir = Vector(0, 0, 0);
		windDir[0] = m_WindDirDeg;
		vector windVector = windDir.AnglesToVector() * m_WindSpeed;

		float impulseStrength = m_WindSpeed * timeSlice * 2;
		m_Physics.ApplyImpulse(windVector * impulseStrength);
	}
}