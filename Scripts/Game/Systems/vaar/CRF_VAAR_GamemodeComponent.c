[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_VAAR_GamemodeComponentClass: SCR_BaseGameModeComponentClass
{
	
}
class CRF_VAAR_GamemodeComponent: SCR_BaseGameModeComponent
{
	protected static CRF_VAAR_GamemodeComponent m_sInstance;
		
	protected string m_sMissionName;
	protected string m_sFilePath;
	protected FileHandle m_AARFile;
	
	[RplProp()] bool m_bRecording;
	protected float m_fTimer = 0;
	protected int m_iCurrentFrame = 0;
	
	[Attribute("0.5", "auto", "Recording intervals in milliseconds", category: "CRF Virtual AAR System")]
    protected float m_iRecordIntervals = 0.5;
	
	protected ref array<ref CRF_VAAR_ShotEvent> m_aShotsBuffer = {};
	protected ref array<ref CRF_VAAR_KillEvent> m_aKillsBuffer = {};
	protected ref array<IEntity> m_aTrackedVehicle = {};
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		SetEventMask(owner, EntityEvent.FRAME);
		
		GetGame().GetCallqueue().CallLater(InitilizeAAR, 8000, false);
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		m_fTimer += timeSlice;
		if (m_fTimer >= m_iRecordIntervals && m_bRecording)
        {
           RecordFrame();
			m_iCurrentFrame++;
            m_fTimer = 0;
        }
	}
	
	// Handle closing out the AAR at game end
	//------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		
		if (RplSession.Mode() != RplMode.Dedicated)
			return;
		
		// Close out the aar with a delay to make sure everything was recorded
		GetGame().GetCallqueue().CallLater(CloseVAAR, m_iRecordIntervals + 0.1);
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
			m_sInstance = null;
	}

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

	// Setup frame json file for recording
	//------------------------------------------------------------------------------------
	protected void InitilizeAAR()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME || CRF_SafestartManager.GetInstance().GetSafestartStatus())
		{
			m_iAARInitAttempts++;
			if (m_iAARInitAttempts < MAX_AAR_INIT_RETRIES)
				GetGame().GetCallqueue().CallLater(InitilizeAAR, 8000, false);
			return;
		}
		
		Print("[CRF_VAAR] Initilizing AAR System");
		
		// Set mission name
		m_sMissionName = string.Format("%1_%2", GetGame().GetMissionName(), System.GetUnixTime());
		
		// Create AAR File
		m_sFilePath = string.Format("$profile:vaar/AAR_Log_%1.json", m_sMissionName);
		
		// Write Mission Details to file
		m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);
		if (!m_AARFile)
			return;
		
		m_AARFile.WriteLine(string.Format("{ \"mission\": \"%1\", \"frames\": [", m_sMissionName));
		
		m_bRecording = true;
		Replication.BumpMe();
		
		Print("[CRF_VAAR] Recording Started");
		CRF_RplBroadcastManager.GetInstance().BroadcastAdminChatMessage("[CRF_VAAR] Recording Started");
	}
	
	// Write frame to json file containing positions, shots fired & events
	//------------------------------------------------------------------------------------
	protected void RecordFrame()
	{
		if (!m_AARFile)
			m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);		
	
		// Grab all the players and AI currently in the world
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		CRF_VAAR_Frame frame = new CRF_VAAR_Frame();
		
		// Timestamp for the frame based on in game time
		frame.ts = m_iCurrentFrame;
		
		// Record shots fired into the frame
		foreach (CRF_VAAR_ShotEvent shot : m_aShotsBuffer)
		{
			frame.s.Insert(shot);
		}

		m_aShotsBuffer.Clear();
		
		// Record kills events into the frame
		foreach (CRF_VAAR_KillEvent killEvent : m_aKillsBuffer)
		{
			frame.k.Insert(killEvent);
		}

		m_aKillsBuffer.Clear();
		
		// Record character positions into the frame
		foreach (AIAgent agent : agents)
		{
			IEntity character = agent.GetControlledEntity();
			if (!character)
				continue;
			
			// Don't track spectator entities
			if (CRF_EntityHelper.IsSpectator(character))
				continue;

			// Collect info
			string characterName = GetCharacterName(character);
			RplId characterID = GetID(character);
			vector characterPos = character.GetOrigin();
			CRF_EGearRole characterRole = GetRole(character);
			int characterFaction = GetFaction(character);
			
			SCR_ChimeraCharacter chimeraCharacter = SCR_ChimeraCharacter.Cast(character.GetRootParent());
			if (!chimeraCharacter)
				continue;
			
			// Get Direction the character is aiming
			vector characterAim = chimeraCharacter.GetHeadAimingComponent().GetAimingDirectionWorld();
			
			CRF_VAAR_CharacterSnapshot characterSnapshot = new CRF_VAAR_CharacterSnapshot(characterID, characterName, characterPos, characterAim, characterRole, characterFaction);
			
			frame.c.Insert(characterSnapshot);
		}
		
		foreach(IEntity vehicle : m_aTrackedVehicle)
		{
			if (!vehicle)
				continue;

			// Collect info
			string vehicleName = GetFriendlyName(vehicle);
			RplId vehicleID = Replication.FindItemId(vehicle);
			vector vehiclePos = vehicle.GetOrigin();
			vector vehicleYaw = vehicle.GetYawPitchRoll();
			int vehicleType = GetVehicleType(vehicle);
			int vehicleFaction = GetFaction(vehicle);
			
			array<string> vehicleOccupants = {};
			GetVehicleOcuupants(vehicle, vehicleOccupants);
			
			CRF_VAAR_VehicleSnapshot vehicleSnapshot = new CRF_VAAR_VehicleSnapshot(vehicleID, vehicleName, vehiclePos, vehicleYaw, vehicleType, vehicleFaction, vehicleOccupants);
			
			frame.v.Insert(vehicleSnapshot);
		}
		
		// Convert the frame to json format
		JsonSaveContext jsonHelper = new JsonSaveContext();
		if (!jsonHelper)
			return;
		
		// Convert the frame to json
		jsonHelper.WriteValue("frame", frame);
		
		// Write the frame to the aar file — comma before each frame after the first
		if (m_iCurrentFrame > 0)
			m_AARFile.WriteLine("," + jsonHelper.SaveToString());
		else
			m_AARFile.WriteLine(jsonHelper.SaveToString());
	}
	
	// Handle closing out the AAR at game end
	//------------------------------------------------------------------------------------
	void CloseVAAR()
	{
		// Stop frame recording
		m_bRecording = false;
		Replication.BumpMe();
		
		// Close out the json file
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
		
		Print("[CRF_VAAR] Recording Saved");
		CRF_RplBroadcastManager.GetInstance().BroadcastAdminChatMessage("[CRF_VAAR] Recording Saved");
	}

	// HELPERS
	//------------------------------------------------------------------------------------
	protected string GetFriendlyName(IEntity entity)
	{
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
	protected string GetCharacterName(IEntity character)
	{
		if (!character)
			return "Unknown";

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "AI";

		int playerID = playerManager.GetPlayerIdFromControlledEntity(character);
		if (playerID != 0)
			return playerManager.GetPlayerName(playerID);
			
		return "AI";
	}
	
	//------------------------------------------------------------------------------------
	protected int GetID(IEntity character)
	{
		if (!character)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		int playerID = 0;
		if (playerManager)
			playerID = playerManager.GetPlayerIdFromControlledEntity(character);

		if (playerID != 0)
			return playerID;
			
		return Replication.FindItemId(character);
	}
	
	//------------------------------------------------------------------------------------
	protected int GetVehicleType(IEntity vehicle)
	{
		Vehicle vehicleEntity = Vehicle.Cast(vehicle);
		if (!vehicleEntity)
			return 0;

		int type = vehicleEntity.m_eVehicleType; // Refactor is planned by devs for this
		
		switch(type)
		{
			case EVehicleType.APC : {return 1;}
			case EVehicleType.CAR : {return 2;}
			case EVehicleType.TRUCK : {return 3;}
			//case EVehicleType.TANK : {return 4;}
			case EVehicleType.MORTAR : {return 5;}
			default : {return 6;} // WHY NO SPECIFIC TYPE FOR HELICOPTERS!?
		}
		
		return 0;
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
	protected CRF_EGearRole GetRole(IEntity entity)
	{	
		ResourceName prefab = entity.GetPrefabData().GetPrefabName();
		if (!CRF_RoleHelper.IsValidGearscriptResource(prefab))
			return CRF_EGearRole.RIFLEMAN;
		
		return CRF_RoleHelper.ResourceToRole(prefab);
	}
	
	
	//------------------------------------------------------------------------------------
	protected void GetVehicleOcuupants(IEntity vehicle, out array<string> occupants)
	{	
		BaseCompartmentManagerComponent compartmentManager = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!compartmentManager)
			return;
		
		array<BaseCompartmentSlot> compartments = {};
		compartmentManager.GetCompartments(compartments);
		
		foreach(BaseCompartmentSlot slot : compartments)
		{
			IEntity occupant = slot.GetOccupant();
			if (!occupant)
				continue;
			
			string occupantName = GetCharacterName(occupant);
			string slotName = "Passenger";
			
			UIInfo slotInfo = slot.GetUIInfo();
			if (slotInfo)
				slotName = WidgetManager.Translate(slotInfo.GetName());
			
			occupants.Insert(string.Format("%1: %2", slotName, occupantName));
		}
	}
	
	// GETTERS
	//------------------------------------------------------------------------------------
	bool IsRecording()
	{
		return m_bRecording;
	}
	
	// SETTERS
	//------------------------------------------------------------------------------------
	void RegisterVehicle(IEntity vehicle)
	{
		if (vehicle && !m_aTrackedVehicle.Contains(vehicle))
			m_aTrackedVehicle.Insert(vehicle);
	}
	//------------------------------------------------------------------------------------
	void UnregisterVehicle(IEntity vehicle)
	{
		m_aTrackedVehicle.RemoveItem(vehicle);
	}
	//------------------------------------------------------------------------------------
	void RegisterShot(IEntity shooter, IEntity projectile, float hitX, float hitZ)
	{
		// Collect info on the projectile
		vector start = shooter.GetOrigin();
		RplId shooterID = Replication.FindItemId(shooter);
		
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
		
		// Store shot in buffer
		m_aShotsBuffer.Insert(new CRF_VAAR_ShotEvent(shooterID, start, hitX, hitZ));
	}
	//------------------------------------------------------------------------------------
	void RegisterCharacterDeath(notnull SCR_InstigatorContextData instigatorContextData)
	{
		// Collect some info
		IEntity killerEntity = instigatorContextData.GetKillerEntity();
		IEntity targetEntity = instigatorContextData.GetVictimEntity();
		
		// Skip self-kills
		if (killerEntity && targetEntity && killerEntity == targetEntity)
			return;
		
		string killerName = GetCharacterName(killerEntity);
		string targetName = GetCharacterName(targetEntity);
		
		int killerFaction = GetFaction(killerEntity);
		int targetFaction = GetFaction(targetEntity);
		
		// Add to event buffer
		m_aKillsBuffer.Insert(new CRF_VAAR_KillEvent(targetName, killerName, targetFaction, killerFaction));
	}
	//------------------------------------------------------------------------------------
	void ToggleRecording()
	{
		m_bRecording = !m_bRecording;
		Replication.BumpMe();
	}
}
