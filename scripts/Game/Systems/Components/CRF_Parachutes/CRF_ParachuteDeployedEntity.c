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

	[Attribute("5.0", UIWidgets.Slider, "Max fall speed (m/s)", "1 20 0.1")]
	protected float m_MaxFallSpeed;

	[Attribute("2.0", UIWidgets.Slider, "Drag strength to limit fall speed", "0 20 0.1")]
	protected float m_DragStrength;

	[Attribute("0.5", UIWidgets.Slider, "Ground detection extra offset (m)", "0 2 0.1")]
	protected float m_GroundCheckOffset;

	protected bool m_HasLanded;

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

	// --------------------------------------------------------------------------------------------
	// Initialization
	// --------------------------------------------------------------------------------------------

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

		SetEventMask(EntityEvent.SIMULATE);
		SetEventMask(EntityEvent.CONTACT);
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
	// Simulation
	// --------------------------------------------------------------------------------------------

	override void EOnSimulate(IEntity owner, float timeSlice)
	{
		if (!IsAuthority() || m_HasLanded || !m_Physics)
			return;

		if (!m_VelocityApplied && m_InitialVelocity != vector.Zero)
		{
			m_Physics.SetVelocity(m_InitialVelocity);
			m_VelocityApplied = true;
		}

		vector vel = m_Physics.GetVelocity();
		float downwardSpeed = -vel[1];

		if (downwardSpeed > m_MaxFallSpeed)
		{
			float excess = downwardSpeed - m_MaxFallSpeed;
			float neededAccel = excess / timeSlice;
			neededAccel = Math.Min(neededAccel, m_DragStrength);
			float impulse = neededAccel * m_Physics.GetMass() * timeSlice;
			m_Physics.ApplyImpulse(vector.Up * impulse);
		}

		HandleWeather(timeSlice);

		vector pos = GetOrigin();
		float groundY = SCR_TerrainHelper.GetTerrainY(pos, null, true);
		if (pos[1] - groundY < m_GroundCheckOffset)
		{
			if (m_CargoSlot && m_CargoSlot.IsOccupied())
				RequestExit();
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