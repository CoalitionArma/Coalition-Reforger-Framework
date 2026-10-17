[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_VAAR_GamemodeComponentClass: SCR_BaseGameModeComponentClass
{

}
class CRF_VAAR_GamemodeComponent: SCR_BaseGameModeComponent
{
	//! Playback file format written by this component (see CRF_VAAR_DataContainer.c)
	static const int FORMAT_VERSION = 2;
	//! Header "sk": shot kinds are reliable. Recordings from 12-16 Oct 2026 lack it - they marked every bullet
	//! as a shell (bullets use ShellMoveComponent too), and the viewer corrects them.
	static const int SHOT_KIND_REVISION = 1;

	protected static CRF_VAAR_GamemodeComponent m_sInstance;

	// Vehicles register here from CRF_VAAR_VehicleTrakcerComponent - static so vehicles that initialise
	// before the game mode entity are not missed
	protected static ref array<IEntity> s_aTrackedVehicles = {};

	protected string m_sMissionName;
	protected string m_sFilePath;
	protected FileHandle m_AARFile;

	[RplProp()] bool m_bRecording;
	protected float m_fTimer = 0;
	protected int m_iCurrentFrame = 0;
	protected bool m_bWroteFrame;
	protected bool m_bClosed;		// file finished - nothing more may be appended

	[Attribute("0.5", "auto", "Seconds between recorded frames", category: "CRF Virtual AAR System")]
    protected float m_iRecordIntervals = 0.5;

	protected ref array<ref CRF_VAAR_ShotEvent> m_aShotsBuffer = {};
	protected ref array<ref CRF_VAAR_KillEvent> m_aKillsBuffer = {};

	// Last roster entry written per ID, so names/roles/factions are only written when they change
	protected ref map<int, string> m_mCharacterRoster = new map<int, string>();
	protected ref map<int, string> m_mVehicleRoster = new map<int, string>();

	// Thrown grenades, reported by the Lobby's COA_GrenadeTracker; each is followed until it explodes,
	// its smoke runs out or it times out
	protected ref array<ref CRF_VAAR_TrackedGrenade> m_aGrenades = {};
	protected ref array<ref CRF_VAAR_GrenadeThrowEvent> m_aGrenadeThrowsBuffer = {};
	protected int m_iNextGrenadeId = 1;

	// How long a thrown grenade stays on the playback: smoke while it smokes; explosive ones until they
	// explode (seen through RegisterShot, else the end of the fuse) - the exploded entity lingers
	protected static const float SMOKE_GRENADE_LIFETIME_MS = 40000;
	protected static const float EXPLOSIVE_GRENADE_LIFETIME_MS = 8000;	// no timer fuse (impact fuzes)
	protected static const float GRENADE_FUSE_MARGIN_MS = 250;

	// Map markers already written to the file: marker ID -> whether it was visible to its faction (shared)
	protected ref map<int, bool> m_mRecordedMarkers = new map<int, bool>();

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (RplSession.Mode() != RplMode.Dedicated)
			return;

		SetEventMask(owner, EntityEvent.FRAME);
		COA_GrenadeTracker.GetOnGrenadeThrown().Insert(OnGrenadeThrown);

		GetGame().GetCallqueue().CallLater(InitilizeAAR, 8000, false);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (RplSession.Mode() != RplMode.Dedicated)
			return;

		if (!m_bRecording)
			return;

		m_fTimer += timeSlice;
		if (m_fTimer < m_iRecordIntervals)
			return;

		// Carry the overshoot into the next frame so frames stay m_iRecordIntervals apart on average -
		// resetting to 0 made every frame a server tick long and the playback clock drifted behind
		m_fTimer -= m_iRecordIntervals;
		if (m_fTimer > m_iRecordIntervals * 10)
			m_fTimer = 0; // a long hitch - don't burst-write the backlog

		RecordFrame();
		m_iCurrentFrame++;
	}

	// Handle closing out the AAR at game end
	//------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);

		if (RplSession.Mode() != RplMode.Dedicated)
			return;

		// Close out the aar with a delay to make sure everything was recorded (CallLater takes milliseconds)
		GetGame().GetCallqueue().CallLater(CloseVAAR, (int)(m_iRecordIntervals * 1000) + 100);
	}

	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		if (RplSession.Mode() != RplMode.Dedicated)
			return;

		RegisterCharacterDeath(instigatorContextData);
	}


	// Singleton instance
	//------------------------------------------------------------------------------------
	void CRF_VAAR_GamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
    {
        m_sInstance = this;
    }

	void ~CRF_VAAR_GamemodeComponent()
	{
		if (m_sInstance == this)
		{
			m_sInstance = null;
			s_aTrackedVehicles.Clear();
		}

		COA_GrenadeTracker.GetOnGrenadeThrown().Remove(OnGrenadeThrown);
	}

	//! Frame being recorded (0.5 s each), or -1 when not recording - lets incident markers point into the playback
	int GetCurrentFrame()
	{
		if (!m_bRecording)
			return -1;
		return m_iCurrentFrame;
	}

	//------------------------------------------------------------------------------------
	static CRF_VAAR_GamemodeComponent GetInstance()
    {
        return m_sInstance;
    }

	string GetLogFilePath()
	{
		return m_sFilePath;
	}

	protected static const int MAX_AAR_INIT_RETRIES = 450; // 450 * 8s = 1 hour max
	protected int m_iAARInitAttempts = 0;

	// Wait for the game to go live, then start recording
	//------------------------------------------------------------------------------------
	protected void InitilizeAAR()
	{
		if (COA_Gamemode.GetInstance().m_GamemodeState != COA_EGamemodeState.GAME || COA_SafestartManager.GetInstance().GetSafestartStatus())
		{
			m_iAARInitAttempts++;
			if (m_iAARInitAttempts < MAX_AAR_INIT_RETRIES)
				GetGame().GetCallqueue().CallLater(InitilizeAAR, 8000, false);
			return;
		}

		// An admin may have started it by hand already
		if (m_AARFile)
			return;

		if (!OpenRecordingFile())
			return;

		m_bRecording = true;
		Replication.BumpMe();

		Print("[CRF_VAAR] Recording Started");
		COA_RplBroadcastManager.GetInstance().BroadcastAdminChatMessage("[CRF_VAAR] Recording Started");
	}

	// Create the playback file and write its header
	//------------------------------------------------------------------------------------
	protected bool OpenRecordingFile()
	{
		Print("[CRF_VAAR] Initilizing AAR System");

		m_sMissionName = string.Format("%1_%2", GetGame().GetMissionName(), System.GetUnixTime());
		m_sFilePath = string.Format("$profile:vaar/AAR_Log_%1.json", m_sMissionName);

		m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);
		if (!m_AARFile)
		{
			Print("[CRF_VAAR] ERROR: Could not create AAR file: " + m_sFilePath, LogLevel.ERROR);
			return false;
		}

		string mission = m_sMissionName;
		mission.Replace("\"", "'");
		string world = GetGame().GetWorldFile();
		world.Replace("\"", "'");
		world.Replace("\\", "/");

		m_AARFile.WriteLine(string.Format("{ \"v\": %1, \"sk\": %6, \"mission\": \"%2\", \"interval\": %3, \"start\": %4, \"world\": \"%5\", \"frames\": [",
			FORMAT_VERSION, mission, m_iRecordIntervals, System.GetUnixTime(), world, SHOT_KIND_REVISION));

		m_bWroteFrame = false;
		m_mCharacterRoster.Clear();
		m_mVehicleRoster.Clear();
		m_aGrenades.Clear();
		m_aGrenadeThrowsBuffer.Clear();
		m_iNextGrenadeId = 1;
		m_mRecordedMarkers.Clear();
		return true;
	}

	// Write frame to json file containing positions, shots fired & events
	//------------------------------------------------------------------------------------
	protected void RecordFrame()
	{
		if (!m_AARFile && !m_sFilePath.IsEmpty())
			m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);
		if (!m_AARFile)
			return;

		// Grab all the players and AI currently in the world
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		CRF_VAAR_Frame frame = new CRF_VAAR_Frame();
		frame.ts = m_iCurrentFrame;

		foreach (CRF_VAAR_ShotEvent shot : m_aShotsBuffer)
		{
			frame.s.Insert(shot);
		}
		m_aShotsBuffer.Clear();

		foreach (CRF_VAAR_KillEvent killEvent : m_aKillsBuffer)
		{
			frame.k.Insert(killEvent);
		}
		m_aKillsBuffer.Clear();

		foreach (CRF_VAAR_GrenadeThrowEvent grenadeThrow : m_aGrenadeThrowsBuffer)
		{
			frame.gt.Insert(grenadeThrow);
		}
		m_aGrenadeThrowsBuffer.Clear();

		RecordMarkerChanges(frame.m);

		// Characters
		foreach (AIAgent agent : agents)
		{
			IEntity character = agent.GetControlledEntity();
			if (!character)
				continue;

			// Don't track spectator entities
			if (COA_EntityHelper.IsSpectator(character))
				continue;

			// Cast the character itself - its root parent is the vehicle while it's in one, and v1 dropped
			// everyone riding in a vehicle from the playback
			SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character);
			if (!chimeraCharacter)
				continue;

			int characterID = GetID(character);
			string characterName = GetCharacterName(character);
			int characterRole = GetRole(character);
			int characterFaction = GetFaction(character);
			bool isPlayer = GetPlayerID(character) != 0;

			string rosterKey = string.Format("%1|%2|%3", characterName, characterRole, characterFaction);
			string knownKey;
			if (!m_mCharacterRoster.Find(characterID, knownKey) || knownKey != rosterKey)
			{
				m_mCharacterRoster.Set(characterID, rosterKey);
				frame.r.Insert(new CRF_VAAR_CharacterInfo(characterID, characterName, characterRole, characterFaction, isPlayer));
			}

			int state = 0;
			vector characterPos = character.GetOrigin();
			if (chimeraCharacter.IsInVehicle())
			{
				state |= CRF_EVAARCharacterState.IN_VEHICLE;
				IEntity vehicleIn = CompartmentAccessComponent.GetVehicleIn(character);
				if (vehicleIn)
					characterPos = vehicleIn.GetOrigin();
			}

			CharacterControllerComponent controller = chimeraCharacter.GetCharacterController();
			if (controller && controller.IsUnconscious())
				state |= CRF_EVAARCharacterState.UNCONSCIOUS;

			// Direction the character is looking
			vector characterAim = vector.Forward;
			AimingComponent headAiming = chimeraCharacter.GetHeadAimingComponent();
			if (headAiming)
				characterAim = headAiming.GetAimingDirectionWorld();

			frame.c.Insert(new CRF_VAAR_CharacterSnapshot(characterID, characterPos, characterAim, state));
		}

		// Vehicles
		BaseWorld world = GetGame().GetWorld();
		foreach (IEntity vehicle : s_aTrackedVehicles)
		{
			if (!vehicle)
				continue;

			int vehicleID = Replication.FindItemId(vehicle);
			int vehicleType = GetVehicleType(vehicle);
			string vehicleName = GetFriendlyName(vehicle);

			string vehicleKey = string.Format("%1|%2", vehicleName, vehicleType);
			string knownVehicleKey;
			if (!m_mVehicleRoster.Find(vehicleID, knownVehicleKey) || knownVehicleKey != vehicleKey)
			{
				m_mVehicleRoster.Set(vehicleID, vehicleKey);
				frame.vr.Insert(new CRF_VAAR_VehicleInfo(vehicleID, vehicleName, vehicleType));
			}

			vector vehiclePos = vehicle.GetOrigin();

			// Height above the ground (or the sea) for aircraft, so the viewer can show who's flying
			int altitude = 0;
			if (world && (vehicleType == CRF_EVAARVehicleType.HELICOPTER || vehicleType == CRF_EVAARVehicleType.PLANE))
			{
				float ground = Math.Max(world.GetSurfaceY(vehiclePos[0], vehiclePos[2]), 0);
				altitude = (int)Math.Round(vehiclePos[1] - ground);
				if (altitude < 0)
					altitude = 0;
			}

			CRF_VAAR_VehicleSnapshot vehicleSnapshot = new CRF_VAAR_VehicleSnapshot(vehicleID, vehiclePos, vehicle.GetYawPitchRoll(), GetFaction(vehicle), GetHealth(vehicle), altitude);
			GetVehicleOccupants(vehicle, vehicleSnapshot.oc, vehicleSnapshot.os);
			frame.v.Insert(vehicleSnapshot);
		}

		// Thrown grenades
		if (world)
		{
			float now = world.GetWorldTime();
			for (int grenadeIndex = m_aGrenades.Count() - 1; grenadeIndex >= 0; grenadeIndex--)
			{
				CRF_VAAR_TrackedGrenade tracked = m_aGrenades[grenadeIndex];
				if (!tracked.m_Entity || now > tracked.m_fExpireAt)
				{
					m_aGrenades.Remove(grenadeIndex);
					continue;
				}

				// Still in someone's hand (or inventory) - not on the map yet
				if (tracked.m_Entity.GetRootParent() != tracked.m_Entity)
					continue;

				frame.g.Insert(new CRF_VAAR_GrenadeSnapshot(tracked.m_iId, tracked.m_Entity.GetOrigin(), tracked.m_iType));
			}
		}

		JsonSaveContext jsonHelper = new JsonSaveContext();
		if (!jsonHelper)
			return;

		jsonHelper.WriteValue("frame", frame);

		// Comma before every frame after the first one written (not the first frame index - a skipped
		// frame 0 used to leave a leading comma and an unreadable file)
		if (m_bWroteFrame)
			m_AARFile.WriteLine("," + jsonHelper.SaveToString());
		else
			m_AARFile.WriteLine(jsonHelper.SaveToString());
		m_bWroteFrame = true;
	}

	// Handle closing out the AAR at game end
	//------------------------------------------------------------------------------------
	void CloseVAAR()
	{
		if (m_sFilePath.IsEmpty() || m_bClosed)
			return;

		// Flush the kills and shots from the last moments
		if (m_bRecording)
			RecordFrame();

		m_bRecording = false;
		Replication.BumpMe();

		if (!m_AARFile)
			m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);

		if (!m_AARFile)
		{
			Print("[CRF_VAAR] ERROR: Could not open AAR file to close recording: " + m_sFilePath, LogLevel.ERROR);
			return;
		}

		m_AARFile.WriteLine("] }");
		m_AARFile.Close();
		m_AARFile = null;
		m_bClosed = true;

		Print("[CRF_VAAR] Recording Saved");
		COA_RplBroadcastManager.GetInstance().BroadcastAdminChatMessage("[CRF_VAAR] Recording Saved");
	}

	// MAP MARKERS
	//------------------------------------------------------------------------------------
	//! Compares the server's map markers with the ones already written and records what changed. The server
	//! holds every marker a player places as public (CRF shareable markers included, shared or not) and the
	//! mission's own; private markers never leave the player's machine, so they can't be recorded.
	protected void RecordMarkerChanges(notnull array<ref CRF_VAAR_MarkerEvent> events)
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return;

		map<int, SCR_MapMarkerBase> current = new map<int, SCR_MapMarkerBase>();
		foreach (SCR_MapMarkerBase marker : markerManager.GetStaticMarkers())
		{
			if (marker && marker.GetMarkerID() != -1)
				current.Set(marker.GetMarkerID(), marker);
		}
		foreach (SCR_MapMarkerBase marker : markerManager.GetDisabledMarkers())
		{
			if (marker && marker.GetMarkerID() != -1)
				current.Set(marker.GetMarkerID(), marker);
		}

		// Removed
		array<int> removed = {};
		foreach (int markerId, bool wasShared : m_mRecordedMarkers)
		{
			if (!current.Contains(markerId))
				removed.Insert(markerId);
		}
		foreach (int markerId : removed)
		{
			m_mRecordedMarkers.Remove(markerId);
			events.Insert(new CRF_VAAR_MarkerEvent(CRF_EVAARMarkerEvent.REMOVED, markerId));
		}

		// Shared state is only worked out when some marker could still change it
		set<int> sharedIds;
		foreach (int markerId, SCR_MapMarkerBase marker : current)
		{
			bool known;
			if (!m_mRecordedMarkers.Find(markerId, known) || !known)
			{
				sharedIds = GetSharedMarkerIds(markerManager);
				break;
			}
		}

		// Placed, or shared since the last frame
		foreach (int markerId, SCR_MapMarkerBase marker : current)
		{
			bool wasShared;
			bool recorded = m_mRecordedMarkers.Find(markerId, wasShared);
			if (recorded && wasShared)
				continue;

			bool isShared = IsMarkerShared(marker, sharedIds);
			if (!recorded)
			{
				m_mRecordedMarkers.Set(markerId, isShared);
				events.Insert(CreateMarkerPlacedEvent(marker, markerManager, isShared));
				continue;
			}

			if (isShared)
			{
				m_mRecordedMarkers.Set(markerId, true);
				events.Insert(new CRF_VAAR_MarkerEvent(CRF_EVAARMarkerEvent.SHARED, markerId));
			}
		}
	}

	//------------------------------------------------------------------------------------
	//! Markers a player has shared with teammates (CRF shareable markers): shared to the whole faction during
	//! safestart, or handed to the players standing nearby
	protected set<int> GetSharedMarkerIds(SCR_MapMarkerManagerComponent markerManager)
	{
		set<int> sharedIds = new set<int>();
		foreach (int markerId, string factionKey : markerManager.m_aGlobalMarkers)
		{
			sharedIds.Insert(markerId);
		}
		foreach (int playerId, array<int> received : markerManager.m_MarkersSharedReference)
		{
			if (!received)
				continue;
			foreach (int markerId : received)
			{
				sharedIds.Insert(markerId);
			}
		}
		return sharedIds;
	}

	//------------------------------------------------------------------------------------
	//! Whether a marker's whole faction can see it. Only player markers of a faction with shareable markers
	//! start out hidden from teammates.
	protected bool IsMarkerShared(SCR_MapMarkerBase marker, set<int> sharedIds)
	{
		if (marker.GetMarkerOwnerID() <= 0)
			return true;

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		Faction ownerFaction = SCR_FactionManager.SGetPlayerFaction(marker.GetMarkerOwnerID());
		if (!gamemode || !ownerFaction || !gamemode.DoesFactionShareMarker(ownerFaction.GetFactionKey()))
			return true;

		return sharedIds && sharedIds.Contains(marker.GetMarkerID());
	}

	//------------------------------------------------------------------------------------
	protected CRF_VAAR_MarkerEvent CreateMarkerPlacedEvent(SCR_MapMarkerBase marker, SCR_MapMarkerManagerComponent markerManager, bool isShared)
	{
		CRF_VAAR_MarkerEvent markerEvent = new CRF_VAAR_MarkerEvent(CRF_EVAARMarkerEvent.PLACED, marker.GetMarkerID());

		int pos[2];
		marker.GetWorldPos(pos);
		markerEvent.x = pos[0] * 10;
		markerEvent.z = pos[1] * 10;
		markerEvent.r = marker.GetRotation();
		markerEvent.n = marker.GetCustomText();
		markerEvent.f = GetMarkerFaction(marker);
		if (isShared)
			markerEvent.s = 1;

		int ownerId = marker.GetMarkerOwnerID();
		if (ownerId > 0)
		{
			markerEvent.o = ownerId;
			markerEvent.on = GetGame().GetPlayerManager().GetPlayerName(ownerId);
		}

		markerEvent.t = CRF_EVAARMarkerKind.OTHER;
		markerEvent.c = 0xFFFFFF;

		SCR_MapMarkerConfig config = markerManager.GetMarkerConfig();
		if (!config)
			return markerEvent;

		SCR_EMapMarkerType markerType = marker.GetType();
		if (markerType == SCR_EMapMarkerType.PLACED_CUSTOM)
		{
			markerEvent.t = CRF_EVAARMarkerKind.CUSTOM;
			SCR_MapMarkerEntryPlaced placedConfig = SCR_MapMarkerEntryPlaced.Cast(config.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_CUSTOM));
			if (placedConfig)
			{
				markerEvent.c = ColorToSRGBInt(placedConfig.GetColorEntry(marker.GetColorEntry()));
				ResourceName imageset, imagesetGlow;
				string quad;
				if (placedConfig.GetIconEntry(marker.GetIconEntry(), imageset, imagesetGlow, quad))
					markerEvent.ic = quad;
			}
		}
		else if (markerType == SCR_EMapMarkerType.PLACED_MILITARY)
		{
			markerEvent.t = CRF_EVAARMarkerKind.MILITARY;
			FillMilitarySymbol(markerEvent, marker, SCR_MapMarkerEntryMilitary.Cast(config.GetMarkerEntryConfigByType(SCR_EMapMarkerType.PLACED_MILITARY)));
		}

		return markerEvent;
	}

	//------------------------------------------------------------------------------------
	//! Identity, colour and "Dimension - Type" label of a NATO symbol marker, read the way
	//! SCR_MapMarkerEntryMilitary.InitClientSettings does
	protected void FillMilitarySymbol(CRF_VAAR_MarkerEvent markerEvent, SCR_MapMarkerBase marker, SCR_MapMarkerEntryMilitary militaryConfig)
	{
		if (!militaryConfig)
			return;

		int configId = marker.GetMarkerConfigID();
		SCR_MarkerMilitaryFactionEntry factionEntry = militaryConfig.GetFactionEntry(configId % SCR_MapMarkerEntryMilitary.FACTION_DETERMINATOR);
		if (factionEntry)
		{
			markerEvent.mi = factionEntry.GetFactionIdentity();
			markerEvent.c = ColorToSRGBInt(factionEntry.GetColor());
		}

		array<string> labels = {};
		SCR_MarkerMilitaryDimension dimension = militaryConfig.GetDimensionEntry(configId / SCR_MapMarkerEntryMilitary.FACTION_DETERMINATOR);
		if (dimension)
			labels.Insert(WidgetManager.Translate(dimension.GetTranslation()));

		int typeFlags = marker.GetFlags();
		array<ref SCR_MarkerMilitaryType> types = militaryConfig.GetMilitaryTypes();
		if (typeFlags != 0 && types)
		{
			foreach (SCR_MarkerMilitaryType type : types)
			{
				if (typeFlags & type.GetType())
					labels.Insert(WidgetManager.Translate(type.GetTranslation()));
			}
		}

		string label;
		foreach (string part : labels)
		{
			if (part.IsEmpty())
				continue;
			if (!label.IsEmpty())
				label += " - ";
			label += part;
		}
		markerEvent.ml = label;
	}

	//------------------------------------------------------------------------------------
	//! Faction a marker is restricted to, else its author's faction (0 for mission markers everyone sees)
	protected int GetMarkerFaction(SCR_MapMarkerBase marker)
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (factionManager && marker.GetMarkerFactionFlags() != 0)
		{
			array<Faction> factions = {};
			factionManager.GetFactionsList(factions);
			foreach (Faction faction : factions)
			{
				if (marker.IsFaction(factionManager.GetFactionIndex(faction)))
					return FactionKeyToCode(faction.GetFactionKey());
			}
		}

		if (marker.GetMarkerOwnerID() > 0)
		{
			Faction ownerFaction = SCR_FactionManager.SGetPlayerFaction(marker.GetMarkerOwnerID());
			if (ownerFaction)
				return FactionKeyToCode(ownerFaction.GetFactionKey());
		}

		return 0;
	}

	//------------------------------------------------------------------------------------
	protected int FactionKeyToCode(string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR" : {return 1;}
			case "OPFOR" : {return 2;}
			case "INDFOR" : {return 3;}
			case "CIV" : {return 4;}
		}
		return 0;
	}

	//------------------------------------------------------------------------------------
	//! Marker configs hold linear colours; the website draws sRGB
	protected int ColorToSRGBInt(Color color)
	{
		if (!color)
			return 0xFFFFFF;
		return (LinearToSRGBByte(color.R()) << 16) | (LinearToSRGBByte(color.G()) << 8) | LinearToSRGBByte(color.B());
	}

	//------------------------------------------------------------------------------------
	protected int LinearToSRGBByte(float linear)
	{
		linear = Math.Clamp(linear, 0, 1);
		float srgb;
		if (linear <= 0.0031308)
			srgb = linear * 12.92;
		else
			srgb = 1.055 * Math.Pow(linear, 1 / 2.4) - 0.055;
		return (int)Math.Clamp(Math.Round(srgb * 255), 0, 255);
	}

	// HELPERS
	//------------------------------------------------------------------------------------
	protected string GetFriendlyName(IEntity entity)
	{
		if (!entity)
			return "Unknown";

	    SCR_EditableEntityComponent editableComponent = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
	    if (editableComponent)
	    {
	        SCR_UIInfo uiInfo = editableComponent.GetInfo();
	        if (uiInfo)
	        {
	             string name = uiInfo.GetName();

				return WidgetManager.Translate(name);
	        }
	    }

		return "Unknown";
	}

	//------------------------------------------------------------------------------------
	protected int GetPlayerID(IEntity character)
	{
		if (!character)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		return playerManager.GetPlayerIdFromControlledEntity(character);
	}

	//------------------------------------------------------------------------------------
	protected string GetCharacterName(IEntity character)
	{
		if (!character)
			return "Unknown";

		int playerID = GetPlayerID(character);
		if (playerID != 0)
			return GetGame().GetPlayerManager().GetPlayerName(playerID);

		return "AI";
	}

	//------------------------------------------------------------------------------------
	//! Player ID for players, RplId for everything else - used for every ID in the file
	protected int GetID(IEntity entity)
	{
		if (!entity)
			return 0;

		int playerID = GetPlayerID(entity);
		if (playerID != 0)
			return playerID;

		return Replication.FindItemId(entity);
	}

	//------------------------------------------------------------------------------------
	protected int GetVehicleType(IEntity vehicle)
	{
		// Aircraft first - the vehicle type attribute has no value for them
		if (vehicle.FindComponent(VehicleHelicopterSimulation))
			return CRF_EVAARVehicleType.HELICOPTER;
		if (vehicle.FindComponent(VehicleFixedWingSimulation))
			return CRF_EVAARVehicleType.PLANE;

		Vehicle vehicleEntity = Vehicle.Cast(vehicle);
		if (!vehicleEntity)
			return CRF_EVAARVehicleType.UNKNOWN;

		bool tracked = vehicle.FindComponent(VehicleTrackedSimulation) != null;

		EVehicleType type = vehicleEntity.m_eVehicleType; // Refactor is planned by devs for this
		if (type == EVehicleType.APC)
			return CRF_EVAARVehicleType.APC;
		if (type == EVehicleType.CAR)
			return CRF_EVAARVehicleType.CAR;
		if (type == EVehicleType.TRUCK)
			return CRF_EVAARVehicleType.TRUCK;
		if (type == EVehicleType.MORTAR)
			return CRF_EVAARVehicleType.MORTAR;
		// v1 drew these as helicopters
		if (type == EVehicleType.FUEL_TRUCK || type == EVehicleType.COMM_TRUCK || type == EVehicleType.SUPPLY_TRUCK || type == EVehicleType.REPAIR || type == EVehicleType.ARSENAL)
			return CRF_EVAARVehicleType.SUPPORT;

		// No type set: tracked vehicles are armour, the viewer refines the rest by name
		if (tracked)
			return CRF_EVAARVehicleType.TANK;

		return CRF_EVAARVehicleType.UNKNOWN;
	}

	//------------------------------------------------------------------------------------
	protected int GetFaction(IEntity entity)
	{
		if (!entity)
			return 0;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return 0;

		Faction faction = factionComponent.GetAffiliatedFaction();
		if (!faction)
			return 0;

		switch(faction.GetFactionKey())
		{
			case "BLUFOR" : {return 1;}
			case "OPFOR" : {return 2;}
			case "INDFOR" : {return 3;}
			case "CIV" : {return 4;}
		}

		return 0;
	}

	//------------------------------------------------------------------------------------
	//! 0-100, 0 once destroyed
	protected int GetHealth(IEntity vehicle)
	{
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
		if (!damageManager)
			return 100;

		if (damageManager.GetState() == EDamageState.DESTROYED)
			return 0;

		int health = (int)Math.Round(damageManager.GetHealthScaled() * 100);
		if (health < 1)
			return 1;
		if (health > 100)
			return 100;
		return health;
	}

	//------------------------------------------------------------------------------------
	protected COA_EGearRole GetRole(IEntity entity)
	{
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!COA_RoleHelper.IsValidGearscriptResource(prefab))
			return COA_EGearRole.RIFLEMAN;

		return COA_RoleHelper.ResourceToRole(prefab);
	}

	//------------------------------------------------------------------------------------
	protected void GetVehicleOccupants(IEntity vehicle, notnull array<int> occupantIds, notnull array<int> seats)
	{
		BaseCompartmentManagerComponent compartmentManager = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!compartmentManager)
			return;

		array<BaseCompartmentSlot> compartments = {};
		compartmentManager.GetCompartments(compartments);

		foreach (BaseCompartmentSlot slot : compartments)
		{
			IEntity occupant = slot.GetOccupant();
			if (!occupant)
				continue;

			int seat = CRF_EVAARSeat.PASSENGER;
			switch (slot.GetType())
			{
				case ECompartmentType.PILOT : {seat = CRF_EVAARSeat.DRIVER; break;}
				case ECompartmentType.TURRET : {seat = CRF_EVAARSeat.GUNNER; break;}
			}

			occupantIds.Insert(GetID(occupant));
			seats.Insert(seat);
		}
	}

	//------------------------------------------------------------------------------------
	//! What a shot came from, by the projectile's movement component. Bullets move with ShellMoveComponent
	//! too; what sets shells (mortar, cannon HE) apart is a trigger component that sets off their warhead -
	//! a bullet's hit effects live in its move component instead.
	protected int GetShotKind(IEntity projectile)
	{
		if (!projectile)
			return CRF_EVAARShotKind.BULLET;

		// Warheads sit under the projectile, so check the whole projectile
		IEntity root = projectile.GetRootParent();
		array<IEntity> parts = {projectile};
		if (root && root != projectile)
			parts.Insert(root);

		bool isShell;
		bool hasTrigger;
		foreach (IEntity part : parts)
		{
			if (part.FindComponent(GrenadeMoveComponent))
				return CRF_EVAARShotKind.GRENADE;
			if (part.FindComponent(MissileMoveComponent))
				return CRF_EVAARShotKind.ROCKET;
			if (part.FindComponent(ShellMoveComponent))
				isShell = true;
			if (part.FindComponent(BaseTriggerComponent))
				hasTrigger = true;
		}

		if (isShell && hasTrigger)
			return CRF_EVAARShotKind.SHELL;

		return CRF_EVAARShotKind.BULLET;
	}

	//------------------------------------------------------------------------------------
	//! Weapon in the killer's hands, or the vehicle they were in
	protected string GetKillWeapon(IEntity killer)
	{
		if (!killer)
			return "";

		ChimeraCharacter character = ChimeraCharacter.Cast(killer);
		if (!character)
		{
			// Killed by a vehicle or static weapon directly
			string ownerName = GetFriendlyName(killer.GetRootParent());
			if (ownerName == "Unknown")
				return "";
			return ownerName;
		}

		if (character.IsInVehicle())
		{
			IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(character);
			string vehicleName = GetFriendlyName(vehicle);
			if (vehicleName != "Unknown")
				return vehicleName;
		}

		BaseWeaponManagerComponent weaponManager = character.GetWeaponManager();
		if (!weaponManager)
			return "";

		BaseWeaponComponent weapon = weaponManager.GetCurrentWeapon();
		if (!weapon)
			return "";

		UIInfo info = weapon.GetUIInfo();
		if (!info)
			return "";

		return WidgetManager.Translate(info.GetName());
	}

	//------------------------------------------------------------------------------------
	protected vector GetMapPosition(IEntity entity)
	{
		IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(entity);
		if (vehicle)
			return vehicle.GetOrigin();
		return entity.GetOrigin();
	}

	// GETTERS
	//------------------------------------------------------------------------------------
	bool IsRecording()
	{
		return m_bRecording;
	}

	// SETTERS
	//------------------------------------------------------------------------------------
	static void RegisterVehicle(IEntity vehicle)
	{
		if (vehicle && !s_aTrackedVehicles.Contains(vehicle))
			s_aTrackedVehicles.Insert(vehicle);
	}
	//------------------------------------------------------------------------------------
	static void UnregisterVehicle(IEntity vehicle)
	{
		s_aTrackedVehicles.RemoveItem(vehicle);
	}
	//------------------------------------------------------------------------------------
	void RegisterShot(IEntity shooter, IEntity projectile, float hitX, float hitZ)
	{
		vector start = GetMapPosition(shooter);
		int shooterID = GetID(shooter);

		// Deduplicate: OnEffect can fire twice per bullet (e.g. two effects on weapon).
		// Skip if the last buffered shot has identical scaled integer coordinates.
		int newSx = (int)Math.Round(start[0] * 10);
		int newSz = (int)Math.Round(start[2] * 10);
		int newHx = (int)Math.Round(hitX * 10);
		int newHz = (int)Math.Round(hitZ * 10);

		if (!m_aShotsBuffer.IsEmpty())
		{
			CRF_VAAR_ShotEvent last = m_aShotsBuffer[m_aShotsBuffer.Count() - 1];
			if (last.si == shooterID && last.sx == newSx && last.sz == newSz && last.hx == newHx && last.hz == newHz)
				return;
		}

		m_aShotsBuffer.Insert(new CRF_VAAR_ShotEvent(shooterID, start, hitX, hitZ, GetShotKind(projectile)));

		// A thrown grenade going off - take it off the playback now
		if (projectile && !m_aGrenades.IsEmpty())
		{
			IEntity projectileRoot = projectile.GetRootParent();
			foreach (CRF_VAAR_TrackedGrenade tracked : m_aGrenades)
			{
				if (tracked.m_iType == CRF_EVAARGrenadeType.EXPLOSIVE && tracked.m_Entity && (tracked.m_Entity == projectile || tracked.m_Entity == projectileRoot))
					tracked.m_fExpireAt = 0;
			}
		}
	}
	//------------------------------------------------------------------------------------
	void RegisterCharacterDeath(notnull SCR_InstigatorContextData instigatorContextData)
	{
		if (!m_bRecording)
			return;

		IEntity killerEntity = instigatorContextData.GetKillerEntity();
		IEntity targetEntity = instigatorContextData.GetVictimEntity();
		if (!targetEntity)
			return;

		// Skip self-kills
		if (killerEntity && killerEntity == targetEntity)
			return;

		vector targetPos = GetMapPosition(targetEntity);
		int distance = -1;
		if (killerEntity)
			distance = (int)Math.Round(vector.Distance(GetMapPosition(killerEntity), targetPos));

		m_aKillsBuffer.Insert(new CRF_VAAR_KillEvent(
			GetID(killerEntity), GetID(targetEntity),
			GetCharacterName(killerEntity), GetCharacterName(targetEntity),
			GetFaction(killerEntity), GetFaction(targetEntity),
			GetKillWeapon(killerEntity), distance, targetPos));
	}
	//------------------------------------------------------------------------------------
	//! COA_GrenadeTracker: a character threw a grenade - follow it on the playback until it's gone
	protected void OnGrenadeThrown(IEntity thrower, IEntity grenade, int grenadeType)
	{
		if (!m_bRecording || !grenade)
			return;

		foreach (CRF_VAAR_TrackedGrenade tracked : m_aGrenades)
		{
			if (tracked.m_Entity == grenade)
				return;
		}

		int recordedType = CRF_EVAARGrenadeType.EXPLOSIVE;
		float lifetime = EXPLOSIVE_GRENADE_LIFETIME_MS;
		if (grenadeType == COA_EGrenadeType.SMOKE)
		{
			recordedType = CRF_EVAARGrenadeType.SMOKE;
			lifetime = SMOKE_GRENADE_LIFETIME_MS;
		}
		else
		{
			// Thrown as it leaves the hand, so it goes off by the end of its fuse
			TimerTriggerComponent fuse = TimerTriggerComponent.Cast(grenade.FindComponent(TimerTriggerComponent));
			if (fuse && fuse.GetTimer() > 0)
				lifetime = fuse.GetTimer() * 1000 + GRENADE_FUSE_MARGIN_MS;
		}

		CRF_VAAR_TrackedGrenade newGrenade = new CRF_VAAR_TrackedGrenade();
		newGrenade.m_Entity = grenade;
		newGrenade.m_iId = m_iNextGrenadeId;
		newGrenade.m_iType = recordedType;
		newGrenade.m_fExpireAt = GetGame().GetWorld().GetWorldTime() + lifetime;
		m_aGrenades.Insert(newGrenade);

		m_aGrenadeThrowsBuffer.Insert(new CRF_VAAR_GrenadeThrowEvent(m_iNextGrenadeId, GetID(thrower), recordedType));
		m_iNextGrenadeId++;
	}

	//------------------------------------------------------------------------------------
	void ToggleRecording()
	{
		// Frames appended after the closing "] }" would make the whole file unreadable
		if (m_bClosed)
			return;

		m_bRecording = !m_bRecording;

		// Started by an admin before the automatic start - open the file now
		if (m_bRecording && !m_AARFile && m_sFilePath.IsEmpty() && !OpenRecordingFile())
			m_bRecording = false;

		Replication.BumpMe();
	}
}

//------------------------------------------------------------------------------------
//! A thrown grenade the recorder is following
class CRF_VAAR_TrackedGrenade
{
	IEntity m_Entity;		// weak: null once the grenade is deleted (exploded, smoke finished)
	int m_iId;
	int m_iType;			// CRF_EVAARGrenadeType
	float m_fExpireAt;		// world time (ms) to stop following it regardless
}
