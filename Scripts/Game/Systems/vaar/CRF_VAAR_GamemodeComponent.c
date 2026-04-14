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
	
	protected bool m_bRecording;
	protected float m_fTimer = 0;
	
	[Attribute("0.5", "auto", "Recording intervals in milliseconds", category: "CRF Virtual AAR System")]
    protected const float m_iRecordIntervals = 0.5;
	
	protected ref array<ref CRF_VAAR_ShotEvent> m_aShotsBuffer = {};
	protected ref array<ref CRF_VAAR_Event> m_aEventsBuffer = {};
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		//if (RplSession.Mode() != RplMode.Dedicated)
		//	return;
		
		SetEventMask(owner, EntityEvent.FRAME);
		
		GetGame().GetCallqueue().CallLater(InitilizeAAR, 4000, true); // Why ;(
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		//if (RplSession.Mode() != RplMode.Dedicated)
		//	return;
		
		m_fTimer += timeSlice;
		if (m_fTimer >= m_iRecordIntervals && m_bRecording)
        {
           RecordFrame();
            m_fTimer = 0;
        }
	}
	
	// Does this work??
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		super.OnGameModeEnd(data);
		
		//if (RplSession.Mode() != RplMode.Dedicated)
		//	return;
		
		// Close out the aar with a delay to make sure everything was recorded
		GetGame().GetCallqueue().CallLater(CloseVAAR, m_iRecordIntervals + 0.1);
	}
	
		
	// Singleton instance
	//------------------------------------------------------------------------------------
	void CRF_VAAR_GamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
    {
        m_sInstance = this;
    }
	
	static CRF_VAAR_GamemodeComponent GetInstance()
    {
        return m_sInstance;
    }
	
	// Setup frame json file for recording
	//------------------------------------------------------------------------------------
	protected void InitilizeAAR()
	{
		if (CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME || CRF_SafestartManager.GetInstance().GetSafestartStatus())
			return;
		
		GetGame().GetCallqueue().Remove(InitilizeAAR);
		
		Print("[CRF_VAAR] Initilizing AAR System");
		
		// Set mission name
		m_sMissionName = string.Format("%1_%2", GetGame().GetMissionName(), System.GetUnixTime());
		
		// Create AAR File
		m_sFilePath = string.Format("$profile:AAR_Log_%1.json", m_sMissionName);
		
		// Write Mission Details to file
		m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);
		if (!m_AARFile)
			return;
		
		m_AARFile.WriteLine(string.Format("{ \"mission\": \"%1\", \"frames\": [", m_sMissionName));
		
		m_bRecording = true;
		
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
		// Would it be better to store these in an array with onplayerconnected or onaispawned?
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		
		CRF_VAAR_Frame frame = new CRF_VAAR_Frame();
		
		// Timestamp for the frame based on in game time
		frame.Timestamp = GetGame().GetWorld().GetWorldTime();
		
		// Record shots fired into the frame
		foreach (CRF_VAAR_ShotEvent shot : m_aShotsBuffer)
		{
			frame.Shots.Insert(shot);
		}

		m_aShotsBuffer.Clear();
		
		// Record events into the frame
		foreach (CRF_VAAR_Event aarEvent : m_aEventsBuffer)
		{
			frame.Events.Insert(aarEvent);
		}

		m_aEventsBuffer.Clear();
		
		// Record entity positions into the frame
		foreach (AIAgent agent : agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			// Collect info
			RplId entityID = Replication.FindId(entity);
			string entityName = entity.GetName(); // This doesnt work...
			vector entityPos = entity.GetOrigin();

			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity.GetRootParent());
			if (!character)
				continue;
			
			vector entityAim = character.GetHeadAimingComponent().GetAimingDirectionWorld();
			
			CRF_VAAR_EntitySnapshot snapshot = new CRF_VAAR_EntitySnapshot(entityID, entityName, entityPos, entityAim);
			
			frame.Entities.Insert(snapshot);
		}
		
		// TODO: Track vehicles and players in them
		
		// Convert the frame to json format
		SCR_JsonSaveContext jsonHelper = new SCR_JsonSaveContext();
		if (!jsonHelper)
			return;
		
		// Convert the frame to json
		jsonHelper.WriteValue("frame", frame);
		
		// Write the frame to the aar file
		m_AARFile.WriteLine(jsonHelper.ExportToString() + ",");
	}
	
	// Handle closing out the AAR at game end
	//------------------------------------------------------------------------------------
	void CloseVAAR()
	{
		// Stop frame recording
		m_bRecording = false;
		
		// Add a blank frame and close out the json file so it valid
		if (!m_AARFile)
			m_AARFile = FileIO.OpenFile(m_sFilePath, FileMode.APPEND);
		
		m_AARFile.WriteLine(string.Format("{{\"frame\": {{\"Timestamp\": 0, \"Entities\": []}}}}"));
		m_AARFile.WriteLine("] }");
		m_AARFile.Close();
		m_AARFile = null;
		
		Print("[CRF_VAAR] Recording Saved");
		CRF_RplBroadcastManager.GetInstance().BroadcastAdminChatMessage("[CRF_VAAR] Recording Saved");
	}
	
	// Handle a shot being fired
	//------------------------------------------------------------------------------------
	void RegisterShot(IEntity shooter, IEntity projectile, float hitX, float hitZ)
	{
		// Collect info on the projectile
		vector start = shooter.GetOrigin();
		RplId shooterID = Replication.FindId(shooter); // Should use rpl component
		
		// Store shot in buffer
		m_aShotsBuffer.Insert(new CRF_VAAR_ShotEvent(shooterID, start, hitX, hitZ));
	}
	
	// Handle event
	//------------------------------------------------------------------------------------
	void RegisterEvent(CRF_VAAR_EEventTypes type, RplId target = RplId.Invalid(), RplId instgator = RplId.Invalid())
	{
		// TODO: No events are actually being tracked this just place holder
		// Store event in buffer
		m_aEventsBuffer.Insert(new CRF_VAAR_Event(type, target, instgator))
	}
	
	// GETTERS
	//------------------------------------------------------------------------------------
	bool IsRecording()
	{
		return m_bRecording;
	}
}