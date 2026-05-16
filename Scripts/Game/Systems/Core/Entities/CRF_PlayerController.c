class CRF_PlayerControllerClass : SCR_PlayerControllerClass
{
}

class CRF_PlayerController : SCR_PlayerController
{	
	//Grace period from respawn to use mini arsenal in ms
	static float GRACE_PERIOD_TIME = 60000;
	
	//Last time this character was respawned.
	float m_fTimeOfLastRespawn;

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ENTITY UPDATES
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Checks to see if grace period is up for a respawn to use the mini arsenal
	//! \return if the grace period is over and we have to hide the mini arsenal
	static bool IsGracePeriodOver()
	{
		CRF_PlayerController pc = CRF_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return false;

		float timeElapesed = GetGame().GetWorld().GetWorldTime() - pc.m_fTimeOfLastRespawn;
		if (timeElapesed > GRACE_PERIOD_TIME)
			return true;
		else 
			return false;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		
		if (from)
		{
			SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(from.FindComponent(SCR_CharacterControllerComponent));
			if (charController && charController.IsDead())
			{
				CRF_PlayerControllerManager pcm = CRF_PlayerControllerManager.GetInstance();
				if (pcm)
				{
					vector mat[4];
					from.GetTransform(mat);
					pcm.m_vPlayersLastDeath = mat;
				}
			};
		}
		
		if (!Replication.IsServer())
		{
			m_fTimeOfLastRespawn = GetGame().GetWorld().GetWorldTime();
	
			SCR_MapMarkerManagerComponent mapMarkerManager = SCR_MapMarkerManagerComponent.GetInstance();
			//Let the entity init before we update global markers (For faction check purposes)
			if (mapMarkerManager)
				GetGame().GetCallqueue().CallLater(mapMarkerManager.RequestGlobalMarkersRefresh, 1000, false);
			
			if (from)
			{
				CRF_BushMovementComponent bushComp = CRF_BushMovementComponent.Cast(from.FindComponent(CRF_BushMovementComponent));
				if (!bushComp)
					return;
				
				bushComp.UnregisterEntity();
			}
			
			if (to)
			{
				CRF_BushMovementComponent bushComp = CRF_BushMovementComponent.Cast(to.FindComponent(CRF_BushMovementComponent));
				if (!bushComp)
					return;
				
				bushComp.RegisterEntity();
			}
		}
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 CONNECTION EVENTS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Called when the player controller updates (typically whenever a player joins/rejoins)
	override protected void UpdateLocalPlayerController()
	{
		super.UpdateLocalPlayerController();
		
		if (RplSession.Mode() == RplMode.Dedicated || !CRF_Gamemode.GetInstance() || !CRF_PlayerControllerManager.GetInstance())
			return;
		
		CRF_PlayerControllerManager.GetInstance().InitilizePlayerControllerComp();
	}

	//------------------------------------------------------------------------------------------------
	//! Called when the player disconnects from the game
	//! Ensures settings are reset to their stored values
	override void DisconnectFromGame()
	{
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
		{
			// Call the parent implementation
			super.DisconnectFromGame();
			return;
		};

		// Get the CRF player controller comp
		CRF_PlayerSettingsManager playerSettingsManager = CRF_PlayerSettingsManager.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerSettingsManager)
			// Reset settings to previously stored values
			playerSettingsManager.ResetSettingsToStoredValues();
		
		super.DisconnectFromGame();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 RADIO EVENTS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void UpdateSettings()
	{
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (CRF_EntityHelper.IsSpectator(GetControlledEntity()))
			return;
		
		//Still trying to update with a spectator radio. No clue why this happens as theres the check above by whatever.
		foreach (IEntity radio: m_aRadios)
		{
			if (!radio)
				continue;
			
			if (!radio.GetPrefabData())
				continue;
			
			if (!radio.GetPrefabData().GetPrefabName())
				continue;
			
			if (radio.GetPrefabData().GetPrefabName() == "{13A97D10A827AE01}Prefabs/Items/Equipment/Radios/SpecRadioBag.et")
				return;
		}
		
		m_aRadioSettings.Clear();
		foreach (IEntity radio: m_aRadios)
		{
			if (!radio)
				continue;
			CVON_RadioComponent radioComp = CVON_RadioComponent.Cast(radio.FindComponent(CVON_RadioComponent));
			
			if (radioComp.m_sFactionKey != "" && radioComp.m_sFactionKey != factionMan.GetPlayerFaction(GetPlayerId()).GetFactionKey())
				return;
			ref CVON_RadioSettingObject setting = new CVON_RadioSettingObject();
			setting.m_sFreq = radioComp.m_sFrequency;
			setting.m_Stereo = radioComp.m_eStereo;
			setting.m_iVolume = radioComp.m_iVolume;
			m_aRadioSettings.Insert(setting);
		}
	}
}