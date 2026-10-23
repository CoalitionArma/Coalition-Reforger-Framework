//------------------------------------------------------------------------------------------------
//! The spectator map (COA_SpectatorMapOverlay) over a replay: the same pins, headings, names by zoom,
//! vehicles with their crew count, camera marker and hover card - fed by the replay's soldiers instead of
//! the Lobby's slots. Clicking a soldier follows them, a body or a vehicle's crew too. Kill skulls follow
//! the playback clock, so a seek moves them with it. Opened by CRF_ReplayHUDMenu (the map key).
//!
//! COA_SpecMapUnit.m_iPlayerId holds the soldier's recording ID here (replays have no player IDs).
class CRF_ReplayMapOverlay : COA_SpectatorMapOverlay
{
	protected static const float CREW_RADIUS = 12;		// a soldier recorded in a vehicle rides the nearest one within this

	protected ref map<int, ref COA_SpecMapUnit> m_mReplayUnits = new map<int, ref COA_SpecMapUnit>();
	protected ref map<int, ref COA_SpecMapVehicle> m_mReplayVehicles = new map<int, ref COA_SpecMapVehicle>();
	protected ref map<int, ref CRF_ReplayKillMarker> m_mReplayKills = new map<int, ref CRF_ReplayKillMarker>();

	//------------------------------------------------------------------------------------------------
	override void Close()
	{
		foreach (int unitId, COA_SpecMapUnit unit : m_mReplayUnits)
		{
			if (unit.m_wRoot)
				unit.m_wRoot.RemoveFromHierarchy();
		}
		m_mReplayUnits.Clear();

		foreach (int vehicleId, COA_SpecMapVehicle vehicle : m_mReplayVehicles)
		{
			if (vehicle.m_wRoot)
				vehicle.m_wRoot.RemoveFromHierarchy();
		}
		m_mReplayVehicles.Clear();

		foreach (int killId, CRF_ReplayKillMarker kill : m_mReplayKills)
		{
			if (kill.m_wRoot)
				kill.m_wRoot.RemoveFromHierarchy();
		}
		m_mReplayKills.Clear();

		super.Close();
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice)
	{
		if (!m_bOpen || !m_MapEntity)
			return;

		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (!player)
			return;

		m_fRefreshTimer += timeSlice;
		if (m_fRefreshTimer >= REFRESH_INTERVAL)
		{
			m_fRefreshTimer = 0;
			RefreshReplay(player);
		}

		// Replays have no squads: zoomed out, everyone stays a dot
		float ppu = m_Workspace.DPIUnscale(m_MapEntity.GetCurrentZoom());
		int detail = 1;
		if (ppu < ZOOM_SQUADS)
			detail = 0;
		else if (ppu >= ZOOM_NAMES)
			detail = 2;

		IEntity followed;
		CRF_ReplayCharProxy followedProxy = player.GetCharacters().Get(player.GetFollowedId());
		if (player.GetFollowedId() != 0 && followedProxy)
			followed = followedProxy.m_Entity;

		foreach (int unitId, COA_SpecMapUnit unit : m_mReplayUnits)
		{
			PositionUnit(unit, detail, followed);
		}

		foreach (int vehicleId, COA_SpecMapVehicle vehicle : m_mReplayVehicles)
		{
			PositionVehicle(vehicle, detail, followed);
		}

		UpdateCamera(followed);
		UpdateReplayKills(player);
		PositionTooltip();
	}

	// ── Data ─────────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void RefreshReplay(CRF_ReplayPlayer player)
	{
		map<int, ref CRF_ReplayCharProxy> characters = player.GetCharacters();
		foreach (int unitId, COA_SpecMapUnit unit : m_mReplayUnits)
		{
			unit.m_bSeen = false;
		}

		foreach (int id, CRF_ReplayCharProxy proxy : characters)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(proxy.m_Entity);
			if (!character)
				continue;

			Faction faction = character.GetFaction();
			if (faction && !ShouldShowFaction(faction))
				continue;

			COA_SpecMapUnit current = m_mReplayUnits.Get(id);
			if (!current || current.m_Character != character)
			{
				if (current && current.m_wRoot)
					current.m_wRoot.RemoveFromHierarchy();

				current = CreateReplayUnit(id, character);
				if (!current)
					continue;
			}

			current.m_bSeen = true;
			current.m_iPlayerId = id;
			current.m_Faction = faction;
			current.m_bAlive = !proxy.m_bKilled;
			current.m_bUncon = current.m_bAlive && proxy.m_bUnconscious;

			CRF_ReplayCharInfo info = player.GetCharInfo(id);
			current.m_sName = "Unknown";
			current.m_sRole = "";
			if (info)
			{
				if (!info.n.IsEmpty())
					current.m_sName = info.n;

				current.m_sRole = CRF_ReplayOverlay.RoleName(info.r);
			}

			if (current.m_bAlive)
				current.m_Pin.SetLabel(current.m_sName);
			else
				current.m_Pin.SetLabel("† " + current.m_sName);
		}

		array<int> stale = {};
		foreach (int unitId, COA_SpecMapUnit unit : m_mReplayUnits)
		{
			if (!unit.m_bSeen || !unit.m_Character)
				stale.Insert(unitId);
		}

		foreach (int staleId : stale)
		{
			COA_SpecMapUnit staleUnit = m_mReplayUnits.Get(staleId);
			if (staleUnit.m_wRoot)
				staleUnit.m_wRoot.RemoveFromHierarchy();
			m_mReplayUnits.Remove(staleId);
		}

		RebuildReplayVehicles(player);

		foreach (int unitId, COA_SpecMapUnit unit : m_mReplayUnits)
		{
			StyleUnit(unit);
		}

		RefreshTooltip();
	}

	//------------------------------------------------------------------------------------------------
	protected COA_SpecMapUnit CreateReplayUnit(int id, SCR_ChimeraCharacter character)
	{
		COA_SpecMapPin pin;
		Widget root = CreatePin(COA_ESpecMapPinKind.UNIT, Z_UNIT, pin);
		if (!root)
			return null;

		COA_SpecMapUnit unit = new COA_SpecMapUnit();
		unit.m_iPlayerId = id;
		unit.m_Character = character;
		unit.m_wRoot = root;
		unit.m_Pin = pin;
		pin.Init(root, COA_ESpecMapPinKind.UNIT, unit, this);
		m_mReplayUnits.Set(id, unit);
		return unit;
	}

	//------------------------------------------------------------------------------------------------
	//! Recordings don't list a vehicle's crew: soldiers recorded in a vehicle ride the nearest one
	protected void RebuildReplayVehicles(CRF_ReplayPlayer player)
	{
		foreach (int vehicleId, COA_SpecMapVehicle vehicle : m_mReplayVehicles)
		{
			vehicle.m_bSeen = false;
			vehicle.m_aCrew.Clear();
		}

		map<int, ref CRF_ReplayCharProxy> characters = player.GetCharacters();
		map<int, ref CRF_ReplayVehicleProxy> vehicles = player.GetVehicles();
		foreach (int unitId, COA_SpecMapUnit unit : m_mReplayUnits)
		{
			unit.m_Vehicle = null;
			CRF_ReplayCharProxy proxy = characters.Get(unitId);
			if (!proxy || !proxy.m_bHidden || !unit.m_bAlive || !unit.m_Character)
				continue;

			vector position = unit.m_Character.GetOrigin();
			bool found;
			int nearestId;
			float nearest = CREW_RADIUS * CREW_RADIUS;
			foreach (int candidateId, CRF_ReplayVehicleProxy candidate : vehicles)
			{
				if (!candidate.m_Entity || candidate.m_bWrecked)
					continue;

				float distance = vector.DistanceSq(position, candidate.m_Entity.GetOrigin());
				if (distance < nearest)
				{
					nearest = distance;
					nearestId = candidateId;
					found = true;
				}
			}

			if (!found)
				continue;

			CRF_ReplayVehicleProxy ridden = vehicles.Get(nearestId);
			COA_SpecMapVehicle crewed = m_mReplayVehicles.Get(nearestId);
			if (!crewed || crewed.m_Entity != ridden.m_Entity)
			{
				if (crewed && crewed.m_wRoot)
					crewed.m_wRoot.RemoveFromHierarchy();

				crewed = CreateReplayVehicle(player, nearestId, ridden, unit.m_Faction);
				if (!crewed)
					continue;
			}

			crewed.m_bSeen = true;
			crewed.m_aCrew.Insert(unit);
			unit.m_Vehicle = crewed;
		}

		array<int> stale = {};
		foreach (int vehicleId, COA_SpecMapVehicle vehicle : m_mReplayVehicles)
		{
			if (!vehicle.m_bSeen || !vehicle.m_Entity)
			{
				stale.Insert(vehicleId);
				continue;
			}

			vehicle.m_Pin.SetBadge(vehicle.m_aCrew.Count().ToString());
		}

		foreach (int staleId : stale)
		{
			COA_SpecMapVehicle staleVehicle = m_mReplayVehicles.Get(staleId);
			if (staleVehicle.m_wRoot)
				staleVehicle.m_wRoot.RemoveFromHierarchy();
			m_mReplayVehicles.Remove(staleId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected COA_SpecMapVehicle CreateReplayVehicle(CRF_ReplayPlayer player, int id, CRF_ReplayVehicleProxy proxy, Faction faction)
	{
		COA_SpecMapPin pin;
		Widget root = CreatePin(COA_ESpecMapPinKind.VEHICLE, Z_VEHICLE, pin);
		if (!root)
			return null;

		COA_SpecMapVehicle vehicle = new COA_SpecMapVehicle();
		vehicle.m_Entity = proxy.m_Entity;
		vehicle.m_wRoot = root;
		vehicle.m_Pin = pin;
		vehicle.m_Faction = faction;
		vehicle.m_sName = CRF_ReplayOverlay.VehicleName(player, proxy);
		pin.Init(root, COA_ESpecMapPinKind.VEHICLE, vehicle, this);

		SCR_UIInfo info;
		SCR_EditableVehicleComponent editable = SCR_EditableVehicleComponent.Cast(proxy.m_Entity.FindComponent(SCR_EditableVehicleComponent));
		if (editable)
			info = editable.GetInfo();

		pin.SetColors(FactionColor(faction), s_Ring);
		pin.SetIconFromInfo(info, ATLAS, "Car");
		pin.SetIconColor(s_White);
		pin.SetLabel(vehicle.m_sName);

		m_mReplayVehicles.Set(id, vehicle);
		return vehicle;
	}

	// ── Kills ────────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! A pulse when new, then a skull fading out over KILL_LIFETIME seconds of playback
	protected void UpdateReplayKills(CRF_ReplayPlayer player)
	{
		foreach (int markerId, CRF_ReplayKillMarker marker : m_mReplayKills)
		{
			marker.m_bSeen = false;
		}

		array<CRF_ReplayKillEntry> kills = {};
		player.GetRecentKills(KILL_LIFETIME, kills);
		float now = player.GetTimeSeconds();
		BaseWorld world = GetGame().GetWorld();
		foreach (CRF_ReplayKillEntry entry : kills)
		{
			CRF_ReplayKill kill = entry.m_Kill;
			if (kill.x == 0 && kill.z == 0)
				continue;

			CRF_ReplayKillMarker killMarker = m_mReplayKills.Get(entry.m_iIndex);
			if (!killMarker)
			{
				COA_SpecMapPin pin;
				Widget root = CreatePin(COA_ESpecMapPinKind.KILL, Z_KILL, pin);
				if (!root)
					continue;

				pin.Init(root, COA_ESpecMapPinKind.KILL, null, this);
				pin.SetColors(CRF_ReplayOverlay.KillColor(player, kill), s_Ring);
				pin.SetIconImage(ATLAS, "Dead");
				pin.SetIconColor(s_White);
				pin.SetLabel("");
				pin.SetLabelShown(false);

				killMarker = new CRF_ReplayKillMarker();
				killMarker.m_wRoot = root;
				killMarker.m_Pin = pin;
				float x = kill.x / 10.0, z = kill.z / 10.0;
				killMarker.m_vPosition = Vector(x, world.GetSurfaceY(x, z), z);
				m_mReplayKills.Set(entry.m_iIndex, killMarker);
			}

			killMarker.m_bSeen = true;
			float age = Math.Max(0, now - entry.m_iFrame * player.GetFrameInterval());
			float pulse = Math.Max(0, 1 - age);
			killMarker.m_Pin.Layout(13 + 12 * pulse, false, true);
			killMarker.m_wRoot.SetOpacity(Math.Clamp(1.2 - age / KILL_LIFETIME, 0.2, 1));
			PlacePin(killMarker.m_Pin, killMarker.m_vPosition);
		}

		array<int> gone = {};
		foreach (int oldId, CRF_ReplayKillMarker oldMarker : m_mReplayKills)
		{
			if (oldMarker.m_bSeen)
				continue;

			if (oldMarker.m_wRoot)
				oldMarker.m_wRoot.RemoveFromHierarchy();
			gone.Insert(oldId);
		}

		foreach (int goneId : gone)
		{
			m_mReplayKills.Remove(goneId);
		}
	}

	// ── Hover card & clicks ──────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! Recorded state - no slots, and no health in the recording
	override protected void FillUnitTooltip(COA_SpecMapUnit unit)
	{
		string state = "Click to follow";
		if (!unit.m_bAlive)
		{
			state = "Killed in action  ·  click to go there";
			CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
			CRF_ReplayKill death;
			if (player)
				death = player.GetDeathOf(unit.m_iPlayerId);

			if (death && !death.kn.IsEmpty() && death.kn != death.tn)
				state = "Killed by " + death.kn;
		}
		else if (unit.m_bUncon)
		{
			state = "Unconscious  ·  click to follow";
		}

		SetTooltip(unit.m_sName, unit.m_sRole, 0, false, state, "", FactionColor(unit.m_Faction));
	}

	//------------------------------------------------------------------------------------------------
	//! The recorded health, not the posed vehicle's
	override protected void FillVehicleTooltip(COA_SpecMapVehicle vehicle)
	{
		string body;
		foreach (COA_SpecMapUnit crew : vehicle.m_aCrew)
		{
			if (!body.IsEmpty())
				body += "<br/>";
			body += crew.m_sName;
			if (crew.m_bUncon)
				body += "  <color rgba='206,103,17,255'>unconscious</color>";
		}

		float health = 1;
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
		{
			foreach (int id, CRF_ReplayVehicleProxy proxy : player.GetVehicles())
			{
				if (proxy.m_Entity == vehicle.m_Entity)
				{
					health = proxy.m_iHealth / 100.0;
					break;
				}
			}
		}

		string sub = string.Format("%1 aboard", vehicle.m_aCrew.Count());
		SetTooltip(vehicle.m_sName, sub, health, true, "Click to follow the crew", body, FactionColor(vehicle.m_Faction));
	}

	//------------------------------------------------------------------------------------------------
	//! A soldier or a vehicle's crew: close the map and follow them; a body: fly the free camera there
	override void OnPinClicked(COA_SpecMapPin pin)
	{
		if (!pin)
			return;

		COA_SpecMapUnit unit = COA_SpecMapUnit.Cast(pin.m_Owner);
		COA_SpecMapVehicle vehicle = COA_SpecMapVehicle.Cast(pin.m_Owner);

		bool follow;
		int followId;
		vector position;
		if (unit && unit.m_Character)
		{
			if (unit.m_bAlive)
			{
				follow = true;
				followId = unit.m_iPlayerId;
			}
			else
			{
				position = unit.m_Character.GetOrigin();
			}
		}
		else if (vehicle)
		{
			foreach (COA_SpecMapUnit crew : vehicle.m_aCrew)
			{
				if (crew.m_Character && crew.m_bAlive)
				{
					follow = true;
					followId = crew.m_iPlayerId;
					break;
				}
			}
		}

		// Deferred: closing the map deletes this pin's widgets
		if (follow)
			GetGame().GetCallqueue().Call(CRF_ReplayHUDMenu.FollowFromMap, followId);
		else if (position != vector.Zero)
			GetGame().GetCallqueue().Call(CRF_ReplayHUDMenu.MoveCameraFromMap, position);
	}
}
