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

	[Attribute("4.0", UIWidgets.Slider, "Max fall speed (m/s)", "1 20 0.1")]
	protected float m_MaxFallSpeed = 4.0;

	[Attribute("2.0", UIWidgets.Slider, "Drag strength to limit fall speed", "0 20 0.1")]
	protected float m_DragStrength = 2.0;

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

		SetEventMask(EntityEvent.SIMULATE);
		SetEventMask(EntityEvent.CONTACT);
		SetEventMask(EntityEvent.FRAME); // for interpolation
	}

	override void EOnDeactivate(IEntity owner)
	{
		UnhookPilotExit();
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