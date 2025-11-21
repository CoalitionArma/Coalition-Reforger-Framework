class CRF_BulletTracerContainer
{
	ref Shape m_Line;
	float m_fTimeAlive;
}

modded class SCR_PlayerController
{
	bool m_bIsBulletTrackingEnabled = false;
	ref array<ref CRF_BulletTracerContainer> m_aActiveTraces = {};
	bool m_bIsListeningToSpec = false;
	vector m_vPlayersLastDeath[4];
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		
		super.EOnFrame(owner, timeSlice);
		ref array<ref CRF_BulletTracerContainer> bulletsToDelete = {};
		foreach (ref CRF_BulletTracerContainer bullet: m_aActiveTraces)
		{
			bullet.m_fTimeAlive -= timeSlice;
			if (bullet.m_fTimeAlive <= 0 || !m_bIsBulletTrackingEnabled)
				bulletsToDelete.Insert(bullet);
		}
		if (bulletsToDelete.Count() > 0)
		{
			foreach (ref CRF_BulletTracerContainer bullet: bulletsToDelete)
			{
				delete bullet.m_Line;
				m_aActiveTraces.RemoveItem(bullet);
			}
		}
	}
	
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		if (from)
		{
			SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(from.FindComponent(SCR_CharacterControllerComponent));
			if (charController.IsDead())
				from.GetTransform(m_vPlayersLastDeath);
		}
	}
	
	/**
	 * Called when the player controller updates (typically whenever a player joins/rejoins)
	 */
	override protected void UpdateLocalPlayerController()
	{
		
		super.UpdateLocalPlayerController();
		
		if (RplSession.Mode() == RplMode.Dedicated || !CRF_Gamemode.GetInstance() || !CRF_PlayerControllerManager.GetInstance())
			return;
		
		CRF_PlayerControllerManager.GetInstance().InitilizePlayerControllerComp();
	}

	/**
	 * Called when the player disconnects from the game
	 * Ensures settings are reset to their stored values
	 */
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
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		
		// Can't do things if the pc comp doesnt exist
		if (playerControllerComp)
			// Reset settings to previously stored values
			playerControllerComp.ResetSettingsToStoredValues();
		
		super.DisconnectFromGame();
	}
	
	void InitializeRadioFromServer()
	{
		Rpc(RpcDo_InitializeRadioFromServer);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_InitializeRadioFromServer()
	{
		GetGame().GetCallqueue().CallLater(InitializeRadios, 500, false, GetLocalControlledEntity());
	}
	
	override void UpdateSettings()
	{
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (CRF_GamemodeManager.IsSpectator(GetControlledEntity()))
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
	
	void ForwardDeployRequestRejected()
	{
		Rpc(RpcDo_ForwardDeployRequestRejected);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_ForwardDeployRequestRejected()
	{
		SCR_NotificationsComponent.GetInstance().SendLocal(SCR_NotificationsComponent.SendLocal(ENotification.FASTTRAVEL_PLAYER_LOCATION_WRONG));
	}
	
	void TeleportLocalPlayer(vector location)
	{
		Rpc(RpcDo_TeleportLocalPlayer, location);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	void RpcDo_TeleportLocalPlayer(vector location)
	{
		SCR_Global.TeleportPlayer(GetPlayerId(), location);
	}
}