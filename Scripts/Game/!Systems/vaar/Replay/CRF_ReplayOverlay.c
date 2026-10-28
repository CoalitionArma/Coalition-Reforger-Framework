//------------------------------------------------------------------------------------------------
//! Replay soldiers get COA's spectator marker (SpectatorLabelIconCharacter) - a replay has no slots, so the
//! name and the unconscious state come from the recording instead
modded class COA_SpectatorLabelIconCharacter
{
	protected bool m_bReplayTag;
	protected string m_sReplayName;
	protected bool m_bReplayUnconscious;

	//------------------------------------------------------------------------------------------------
	void SetReplayName(string name)
	{
		m_bReplayTag = true;
		if (name == m_sReplayName)
			return;

		m_sReplayName = name;
		m_fNextNameRefresh = 0;
	}

	//------------------------------------------------------------------------------------------------
	void SetReplayUnconscious(bool unconscious)
	{
		m_bReplayUnconscious = unconscious;
	}

	//------------------------------------------------------------------------------------------------
	override protected void UpdatePlayerName()
	{
		if (!m_bReplayTag)
		{
			super.UpdatePlayerName();
			return;
		}

		if (!m_wSpectatorLabelText)
			return;

		string displayName = m_sReplayName;
		if (m_bDead && !displayName.IsEmpty())
			displayName = "† " + displayName;

		m_wSpectatorLabelText.SetText(displayName);
		if (m_bDead)
			m_wSpectatorLabelText.SetOpacity(0.55);
		else
			m_wSpectatorLabelText.SetOpacity(1.0);
	}

	//------------------------------------------------------------------------------------------------
	//! Posed bodies are never really unconscious - the recording says when they were
	override protected void UpdateCharacterState()
	{
		if (!m_bReplayTag)
		{
			super.UpdateCharacterState();
			return;
		}

		if (!m_ControllerComponent)
			return;

		bool dead = m_ControllerComponent.IsDead();
		bool wounded = !dead && m_bReplayUnconscious;
		if (dead == m_bDead && wounded == m_bWounded)
			return;

		m_bDead = dead;
		m_bWounded = wounded;
		m_fNextNameRefresh = 0;
		ApplyStateColors();
	}
}

//------------------------------------------------------------------------------------------------
modded class COA_SpecHoverCard
{
	//------------------------------------------------------------------------------------------------
	//! A card the caller fills in (the replay, whose soldiers have no slots)
	void ShowCustom(Managed subject, string title, string sub, float health, bool showHealth, string state, string body, Color accent, float anchorX, float anchorY)
	{
		if (!subject || !m_wRoot)
			return;

		if (NeedsRefresh(subject))
			Fill(title, sub, health, showHealth, state, body, accent);

		Place(anchorX, anchorY);
	}
}

//------------------------------------------------------------------------------------------------
class CRF_ReplayTag : Managed
{
	int m_iId;
	IEntity m_Entity;
	Widget m_wRoot;
	COA_SpectatorLabelIconCharacter m_Icon;

	//------------------------------------------------------------------------------------------------
	void OnClicked()
	{
		CRF_ReplayPlayer player = CRF_ReplayPlayer.GetInstance();
		if (player)
			player.Follow(m_iId);
	}
}

//------------------------------------------------------------------------------------------------
class CRF_ReplayVehicleMarker : Managed
{
	int m_iId;
	IEntity m_Entity;
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;
	Faction m_Faction;
	ref array<int> m_aCrew = {};
	float m_fScreenX;
	float m_fScreenY;
}

//------------------------------------------------------------------------------------------------
class CRF_ReplayKillMarker
{
	Widget m_wRoot;
	COA_SpecMapPin m_Pin;
	vector m_vPosition;
	bool m_bSeen;
}

//------------------------------------------------------------------------------------------------
//! The spectator's in-world markers over a replay: a name tag on every soldier (click to follow, hover for
//! a card), occupied vehicles with their crew count, and a fading skull where someone died in the last
//! KILL_SECONDS of playback. Same widgets as COA_SpectatorMenu / COA_SpecWorldMarkers. Owned by CRF_ReplayHUDMenu.
class CRF_ReplayOverlay
{
	protected static const ResourceName TAG_LAYOUT = "{68625BAD23CEE68F}UI/Spectator/SpectatorLabelIconCharacter.layout";
	protected static const ResourceName PIN_LAYOUT = "{6A75150000000001}UI/Spectator/COA_SpecMapPin.layout";
	protected static const ResourceName ATLAS = "{F3A9B47F55BE8D2B}UI/Images/SpecImages/Spec_Atlas_x64.imageset";

	protected static const float VEHICLE_REFRESH_MS = 500;
	protected static const float CREW_RADIUS = 12;			// a soldier recorded in a vehicle rides the nearest one within this
	protected static const float VEHICLE_MAX_DISTANCE = 1200;
	protected static const float VEHICLE_MIN_DISTANCE = 6;
	protected static const float VEHICLE_SIZE_NEAR = 26;
	protected static const float VEHICLE_SIZE_FAR = 18;
	protected static const float VEHICLE_ABOVE_ROOF = 1.2;

	protected static const float KILL_SECONDS = 20;			// of playback
	protected static const float KILL_MAX_DISTANCE = 800;

	protected static ref Color s_Ring = new Color(0.0033, 0.0037, 0.0052, 0.92);
	protected static ref Color s_White = new Color(0.8632, 0.8879, 0.9301, 1);

	protected Widget m_wParent;
	protected bool m_bVisible = true;
	protected ref map<int, ref CRF_ReplayTag> m_mTags = new map<int, ref CRF_ReplayTag>();
	protected ref map<int, ref CRF_ReplayVehicleMarker> m_mVehicles = new map<int, ref CRF_ReplayVehicleMarker>();
	protected ref map<int, ref CRF_ReplayKillMarker> m_mKills = new map<int, ref CRF_ReplayKillMarker>();
	protected ref COA_SpecHoverCard m_HoverCard;
	protected float m_fNextVehicleRefresh;

	//------------------------------------------------------------------------------------------------
	void CRF_ReplayOverlay(notnull Widget parent)
	{
		m_wParent = parent;
		m_HoverCard = new COA_SpecHoverCard(parent);
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_ReplayOverlay()
	{
		Clear();
	}

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		foreach (int id, CRF_ReplayTag tag : m_mTags)
		{
			if (tag.m_wRoot)
				tag.m_wRoot.RemoveFromHierarchy();
		}
		m_mTags.Clear();

		foreach (int vehicleId, CRF_ReplayVehicleMarker vehicle : m_mVehicles)
		{
			if (vehicle.m_wRoot)
				vehicle.m_wRoot.RemoveFromHierarchy();
		}
		m_mVehicles.Clear();

		foreach (int killId, CRF_ReplayKillMarker kill : m_mKills)
		{
			if (kill.m_wRoot)
				kill.m_wRoot.RemoveFromHierarchy();
		}
		m_mKills.Clear();

		if (m_HoverCard)
			m_HoverCard.Hide();
	}

	//------------------------------------------------------------------------------------------------
	//! Hide UI: everything goes, and comes back as it was
	void SetVisible(bool visible)
	{
		m_bVisible = visible;
		if (!visible)
			Clear();
	}

	//------------------------------------------------------------------------------------------------
	void Update(CRF_ReplayPlayer player)
	{
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (!m_bVisible || !player || !cameraManager || !cameraManager.CurrentCamera())
		{
			m_HoverCard.Hide();
			return;
		}

		CRF_ReplayTag hoveredTag = UpdateTags(player);

		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fNextVehicleRefresh)
		{
			m_fNextVehicleRefresh = now + VEHICLE_REFRESH_MS;
			RefreshVehicles(player);
		}

		vector cameraPos = cameraManager.CurrentCamera().GetOrigin();
		Widget underCursor = WidgetManager.GetWidgetUnderCursor();
		CRF_ReplayVehicleMarker hoveredVehicle;
		foreach (int id, CRF_ReplayVehicleMarker vehicle : m_mVehicles)
		{
			if (PositionVehicle(vehicle, cameraPos, underCursor))
				hoveredVehicle = vehicle;
		}

		UpdateKills(player, cameraPos);
		UpdateHoverCard(player, hoveredTag, hoveredVehicle);
	}

	// ── Soldiers ─────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! \return the tag under the cursor, or null
	protected CRF_ReplayTag UpdateTags(CRF_ReplayPlayer player)
	{
		map<int, ref CRF_ReplayCharProxy> characters = player.GetCharacters();

		// Gone, or a new body for the same soldier (respawned, or back before a death after a seek)
		array<int> stale = {};
		foreach (int id, CRF_ReplayTag tag : m_mTags)
		{
			CRF_ReplayCharProxy current = characters.Get(id);
			if (!current || !tag.m_Entity || current.m_Entity != tag.m_Entity)
				stale.Insert(id);
		}

		foreach (int staleId : stale)
		{
			RemoveTag(staleId);
		}

		bool firstPerson = player.GetCameraMode() != CRF_EReplayCameraMode.ORBIT;
		int followed = player.GetFollowedId();
		CRF_ReplayTag hovered;
		foreach (int id, CRF_ReplayCharProxy proxy : characters)
		{
			if (!proxy.m_Entity)
				continue;

			CRF_ReplayTag tag = m_mTags.Get(id);
			if (!tag)
			{
				tag = CreateTag(proxy);
				if (!tag)
					continue;
			}

			string name;
			CRF_ReplayCharInfo info = player.GetCharInfo(id);
			if (info)
				name = info.n;

			tag.m_Icon.SetReplayName(name);
			tag.m_Icon.SetReplayUnconscious(proxy.m_bUnconscious);
			tag.m_Icon.Update();

			// Inside a vehicle (its marker counts the crew), or the head the camera is in
			if (proxy.m_bHidden || (firstPerson && id == followed))
			{
				tag.m_wRoot.SetOpacity(0);
				tag.m_wRoot.SetEnabled(false);
				continue;
			}

			if (tag.m_Icon.IsHoveredNow())
				hovered = tag;
		}

		return hovered;
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayTag CreateTag(CRF_ReplayCharProxy proxy)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(TAG_LAYOUT, m_wParent);
		if (!root)
			return null;

		COA_SpectatorLabelIconCharacter icon = COA_SpectatorLabelIconCharacter.Cast(root.FindHandler(COA_SpectatorLabelIconCharacter));
		if (!icon)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		CRF_ReplayTag tag = new CRF_ReplayTag();
		tag.m_iId = proxy.m_iId;
		tag.m_Entity = proxy.m_Entity;
		tag.m_wRoot = root;
		tag.m_Icon = icon;

		icon.SetEntity(proxy.m_Entity, "Spine3");
		SCR_ButtonTextComponent button = icon.GetButton();
		if (button)
			button.m_OnClicked.Insert(tag.OnClicked);

		m_mTags.Set(proxy.m_iId, tag);
		return tag;
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveTag(int id)
	{
		CRF_ReplayTag tag = m_mTags.Get(id);
		if (tag && tag.m_wRoot)
			tag.m_wRoot.RemoveFromHierarchy();

		m_mTags.Remove(id);
	}

	// ── Vehicles ─────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! Recordings don't list a vehicle's crew: soldiers recorded in a vehicle ride the nearest one
	protected void RefreshVehicles(CRF_ReplayPlayer player)
	{
		map<int, ref CRF_ReplayVehicleProxy> vehicles = player.GetVehicles();
		foreach (int id, CRF_ReplayVehicleMarker marker : m_mVehicles)
		{
			marker.m_aCrew.Clear();
			marker.m_Faction = null;
		}

		foreach (int charId, CRF_ReplayCharProxy proxy : player.GetCharacters())
		{
			if (!proxy.m_bHidden || proxy.m_bKilled || !proxy.m_Entity)
				continue;

			vector position = proxy.m_Entity.GetOrigin();
			bool found;
			int nearestId;
			float nearest = CREW_RADIUS * CREW_RADIUS;
			foreach (int vehicleId, CRF_ReplayVehicleProxy vehicle : vehicles)
			{
				if (!vehicle.m_Entity || vehicle.m_bWrecked)
					continue;

				float distance = vector.DistanceSq(position, vehicle.m_Entity.GetOrigin());
				if (distance < nearest)
				{
					nearest = distance;
					nearestId = vehicleId;
					found = true;
				}
			}

			if (!found)
				continue;

			CRF_ReplayVehicleProxy ridden = vehicles.Get(nearestId);
			CRF_ReplayVehicleMarker crewed = m_mVehicles.Get(nearestId);
			if (!crewed || crewed.m_Entity != ridden.m_Entity)
			{
				RemoveVehicle(nearestId);
				crewed = CreateVehicle(player, ridden);
				if (!crewed)
					continue;
			}

			crewed.m_aCrew.Insert(charId);
			if (!crewed.m_Faction)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(proxy.m_Entity);
				if (character)
					crewed.m_Faction = character.GetFaction();
			}
		}

		array<int> stale = {};
		foreach (int markerId, CRF_ReplayVehicleMarker vehicleMarker : m_mVehicles)
		{
			if (vehicleMarker.m_aCrew.IsEmpty() || !vehicleMarker.m_Entity)
			{
				stale.Insert(markerId);
				continue;
			}

			vehicleMarker.m_Pin.SetBadge(vehicleMarker.m_aCrew.Count().ToString());
		}

		foreach (int staleId : stale)
		{
			RemoveVehicle(staleId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_ReplayVehicleMarker CreateVehicle(CRF_ReplayPlayer player, CRF_ReplayVehicleProxy proxy)
	{
		COA_SpecMapPin pin;
		Widget root = CreatePin(pin);
		if (!root)
			return null;

		CRF_ReplayVehicleMarker marker = new CRF_ReplayVehicleMarker();
		marker.m_iId = proxy.m_iId;
		marker.m_Entity = proxy.m_Entity;
		marker.m_wRoot = root;
		marker.m_Pin = pin;
		pin.Init(root, COA_ESpecMapPinKind.VEHICLE, marker, null);

		SCR_UIInfo info;
		SCR_EditableVehicleComponent editable = SCR_EditableVehicleComponent.Cast(proxy.m_Entity.FindComponent(SCR_EditableVehicleComponent));
		if (editable)
			info = editable.GetInfo();

		pin.SetColors(COA_SpecHoverCard.FactionColor(null), s_Ring);
		pin.SetIconFromInfo(info, ATLAS, "Car");
		pin.SetIconColor(s_White);
		pin.SetLabel(VehicleName(player, proxy));
		pin.SetLabelShown(false);
		pin.Layout(VEHICLE_SIZE_NEAR, false, true);

		m_mVehicles.Set(proxy.m_iId, marker);
		return marker;
	}

	//------------------------------------------------------------------------------------------------
	protected void RemoveVehicle(int id)
	{
		CRF_ReplayVehicleMarker marker = m_mVehicles.Get(id);
		if (marker && marker.m_wRoot)
			marker.m_wRoot.RemoveFromHierarchy();

		m_mVehicles.Remove(id);
	}

	//------------------------------------------------------------------------------------------------
	//! \return true when the cursor is on it
	protected bool PositionVehicle(CRF_ReplayVehicleMarker marker, vector cameraPos, Widget underCursor)
	{
		if (!marker.m_Entity || !marker.m_wRoot)
			return false;

		vector boundsMin, boundsMax;
		marker.m_Entity.GetWorldBounds(boundsMin, boundsMax);
		vector top = marker.m_Entity.GetOrigin();
		top[1] = boundsMax[1] + VEHICLE_ABOVE_ROOF;

		float distance = vector.Distance(cameraPos, top);
		vector screen = GetGame().GetWorkspace().ProjWorldToScreen(top, GetGame().GetWorld());
		bool show = screen[2] > 0 && distance < VEHICLE_MAX_DISTANCE && distance > VEHICLE_MIN_DISTANCE;

		marker.m_Pin.SetShown(show);
		marker.m_wRoot.SetEnabled(show);
		if (!show)
			return false;

		float t = Math.Clamp((distance - 20) / (VEHICLE_MAX_DISTANCE - 20), 0, 1);
		marker.m_Pin.Layout(VEHICLE_SIZE_NEAR + (VEHICLE_SIZE_FAR - VEHICLE_SIZE_NEAR) * t, false, true);

		bool hovered = IsUnder(underCursor, marker.m_wRoot);
		marker.m_Pin.SetLabelShown(hovered);
		if (hovered)
			marker.m_Pin.SetColors(COA_SpecHoverCard.FactionColor(marker.m_Faction), s_White);
		else
			marker.m_Pin.SetColors(COA_SpecHoverCard.FactionColor(marker.m_Faction), s_Ring);

		marker.m_fScreenX = screen[0];
		marker.m_fScreenY = screen[1];
		marker.m_Pin.SetScreenPos(screen[0], screen[1]);
		marker.m_wRoot.SetZOrder(screen[2] * -10000);
		return hovered;
	}

	// ── Kills ────────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	//! A pulse when new, then a skull fading out over KILL_SECONDS of playback - a seek moves them with it
	protected void UpdateKills(CRF_ReplayPlayer player, vector cameraPos)
	{
		foreach (int id, CRF_ReplayKillMarker marker : m_mKills)
		{
			marker.m_bSeen = false;
		}

		array<CRF_ReplayKillEntry> kills = {};
		player.GetRecentKills(KILL_SECONDS, kills);
		float now = player.GetTimeSeconds();
		BaseWorld world = GetGame().GetWorld();
		foreach (CRF_ReplayKillEntry entry : kills)
		{
			CRF_ReplayKill kill = entry.m_Kill;
			if (kill.x == 0 && kill.z == 0)
				continue;

			CRF_ReplayKillMarker killMarker = m_mKills.Get(entry.m_iIndex);
			if (!killMarker)
			{
				COA_SpecMapPin pin;
				Widget root = CreatePin(pin);
				if (!root)
					continue;

				pin.Init(root, COA_ESpecMapPinKind.KILL, null, null);
				pin.SetColors(KillColor(player, kill), s_Ring);
				pin.SetIconImage(ATLAS, "Dead");
				pin.SetIconColor(s_White);
				pin.SetLabel("");
				pin.SetLabelShown(false);

				killMarker = new CRF_ReplayKillMarker();
				killMarker.m_wRoot = root;
				killMarker.m_Pin = pin;
				float x = kill.x / 10.0, z = kill.z / 10.0;
				killMarker.m_vPosition = Vector(x, world.GetSurfaceY(x, z) + 1.0, z);
				m_mKills.Set(entry.m_iIndex, killMarker);
			}

			killMarker.m_bSeen = true;
			vector screen = GetGame().GetWorkspace().ProjWorldToScreen(killMarker.m_vPosition, world);
			bool show = screen[2] > 0 && vector.Distance(cameraPos, killMarker.m_vPosition) < KILL_MAX_DISTANCE;
			killMarker.m_Pin.SetShown(show);
			if (!show)
				continue;

			float age = Math.Max(0, now - entry.m_iFrame * player.GetFrameInterval());
			float pulse = Math.Max(0, 1 - age);
			killMarker.m_Pin.Layout(14 + 12 * pulse, false, true);
			killMarker.m_wRoot.SetOpacity(Math.Clamp(1.2 - age / KILL_SECONDS, 0.15, 1));
			killMarker.m_Pin.SetScreenPos(screen[0], screen[1]);
			killMarker.m_wRoot.SetZOrder(screen[2] * -10000 - 2);
		}

		array<int> gone = {};
		foreach (int markerId, CRF_ReplayKillMarker oldMarker : m_mKills)
		{
			if (oldMarker.m_bSeen)
				continue;

			if (oldMarker.m_wRoot)
				oldMarker.m_wRoot.RemoveFromHierarchy();
			gone.Insert(markerId);
		}

		foreach (int goneId : gone)
		{
			m_mKills.Remove(goneId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The victim's faction colour (their body's, else the recorded side's)
	static Color KillColor(CRF_ReplayPlayer player, CRF_ReplayKill kill)
	{
		CRF_ReplayCharProxy victim;
		if (kill.ti != 0)
			victim = player.GetCharacters().Get(kill.ti);

		if (victim)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(victim.m_Entity);
			if (character && character.GetFaction())
				return character.GetFaction().GetFactionColor();
		}

		return SideColor(kill.tf);
	}

	//------------------------------------------------------------------------------------------------
	//! Recorded side (1 BLUFOR, 2 OPFOR, 3 INDFOR, 4 CIV)
	static Color SideColor(int side)
	{
		switch (side)
		{
			case 1: return Color.FromSRGBA(41, 104, 201, 255);
			case 2: return Color.FromSRGBA(198, 40, 40, 255);
			case 3: return Color.FromSRGBA(46, 140, 64, 255);
			case 4: return Color.FromSRGBA(128, 64, 160, 255);
		}

		return COA_SpecHoverCard.FactionColor(null);
	}

	// ── Hover card ───────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected void UpdateHoverCard(CRF_ReplayPlayer player, CRF_ReplayTag tag, CRF_ReplayVehicleMarker vehicle)
	{
		if (tag && tag.m_wRoot)
		{
			CRF_ReplayCharProxy proxy = player.GetCharacters().Get(tag.m_iId);
			CRF_ReplayCharInfo info = player.GetCharInfo(tag.m_iId);
			string name = "Unknown";
			string sub;
			if (info)
			{
				if (!info.n.IsEmpty())
					name = info.n;

				sub = RoleName(info.r);
			}

			string state = "Click to follow";
			if (proxy && proxy.m_bKilled)
			{
				state = "Killed in action";
				CRF_ReplayKill death = player.GetDeathOf(tag.m_iId);
				if (death && !death.kn.IsEmpty() && death.kn != death.tn)
					state = "Killed by " + death.kn;
			}
			else if (proxy && proxy.m_bUnconscious)
			{
				state = "Unconscious";
			}
			else if (player.GetFollowedId() == tag.m_iId)
			{
				state = "Following";
			}

			Faction faction;
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(tag.m_Entity);
			if (character)
				faction = character.GetFaction();

			m_HoverCard.ShowCustom(tag, name, sub, 0, false, state, "", COA_SpecHoverCard.FactionColor(faction), FrameSlot.GetPosX(tag.m_wRoot), FrameSlot.GetPosY(tag.m_wRoot));
			return;
		}

		if (vehicle)
		{
			string body;
			foreach (int crewId : vehicle.m_aCrew)
			{
				CRF_ReplayCharInfo crewInfo = player.GetCharInfo(crewId);
				if (!crewInfo || crewInfo.n.IsEmpty())
					continue;

				if (!body.IsEmpty())
					body += "<br/>";
				body += crewInfo.n;
			}

			float health = 1;
			CRF_ReplayVehicleProxy vehicleProxy = player.GetVehicles().Get(vehicle.m_iId);
			if (vehicleProxy)
				health = vehicleProxy.m_iHealth / 100.0;

			m_HoverCard.ShowCustom(vehicle, VehicleName(player, vehicleProxy), string.Format("%1 aboard", vehicle.m_aCrew.Count()), health, true, "", body,
				COA_SpecHoverCard.FactionColor(vehicle.m_Faction), vehicle.m_fScreenX, vehicle.m_fScreenY);
			return;
		}

		m_HoverCard.Hide();
	}

	//------------------------------------------------------------------------------------------------
	//! The recorded name, else the prefab's
	static string VehicleName(CRF_ReplayPlayer player, CRF_ReplayVehicleProxy proxy)
	{
		if (!proxy)
			return "Vehicle";

		CRF_ReplayVehInfo info = player.GetVehicleInfo(proxy.m_iId);
		if (info && !info.n.IsEmpty())
			return info.n;

		if (proxy.m_Entity)
			return COA_SpecHoverCard.VehicleName(proxy.m_Entity);

		return "Vehicle";
	}

	//------------------------------------------------------------------------------------------------
	//! COA_EGearRole as words: AUTOMATIC_RIFLEMAN -> Automatic rifleman
	static string RoleName(int role)
	{
		if (role <= 0)
			return "";

		string name = typename.EnumToString(COA_EGearRole, role);
		if (name.IsEmpty())
			return "";

		name.Replace("_", " ");
		name.ToLower();
		string first = name.Substring(0, 1);
		first.ToUpper();
		return first + name.Substring(1, name.Length() - 1);
	}

	// ── Helpers ──────────────────────────────────────────────────────────────────────────────────
	//------------------------------------------------------------------------------------------------
	protected Widget CreatePin(out COA_SpecMapPin pin)
	{
		Widget root = GetGame().GetWorkspace().CreateWidgets(PIN_LAYOUT, m_wParent);
		if (!root)
			return null;

		Widget hitArea = root.FindAnyWidget("HitArea");
		if (hitArea)
			pin = COA_SpecMapPin.Cast(hitArea.FindHandler(COA_SpecMapPin));

		if (!pin)
		{
			root.RemoveFromHierarchy();
			return null;
		}

		return root;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsUnder(Widget underCursor, Widget root)
	{
		while (underCursor)
		{
			if (underCursor == root)
				return true;

			underCursor = underCursor.GetParent();
		}

		return false;
	}
}
