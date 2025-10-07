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
	 * Called when the entity controlled by this player controller changes
	 * @param from The previous entity being controlled
	 * @param to The new entity being controlled
	 */
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{	
		// Check if gamemode instance exists, if not, exit early
		if (!CRF_Gamemode.GetInstance())
		{
			// Call the parent implementation
			super.OnControlledEntityChanged(from, to);
			return;
		};
		
		// Get the CRF player controller comp
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();

		// Handle race condition: If player is being assigned initial entity when they should have a playable character
		if (to && to.GetPrefabData().GetPrefabName() == CRF_GamemodeManager.GetSpectatorResource() && 
			CRF_Gamemode.GetInstance().m_GamemodeState == CRF_EGamemodeState.GAME)
		{
			int playerId = GetPlayerId();
			CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
			
			// Check if this player should have a proper character instead of initial entity
			if (slottingManager && slottingManager.IsPlayerInASlot(playerId) && !slottingManager.IsPlayerConsideredDead(playerId))
			{
				// Request re-initialization from server to fix race condition
				CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
				if (rplManager)
				{
					GetGame().GetCallqueue().CallLater(rplManager.RequestInitilizePlayer, 250, false, playerId);
				}
			}
		}

		// Call the parent implementation
		super.OnControlledEntityChanged(from, to);
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
		GetGame().GetCallqueue().CallLater(InitializeRadios, 2000, false, GetLocalControlledEntity());
	}
}