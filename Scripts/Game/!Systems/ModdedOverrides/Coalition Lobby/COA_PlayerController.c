modded class COA_PlayerController
{
	// Set once by CRF_COA_PlayerRplToOwnerManager.RpcDo_SetJoinInProgress if (and only if) this
	// player's connection was audited while the round was already in COA_EGamemodeState.GAME - the
	// only real signal for "this player is joining in progress." Consumed (cleared) the first time
	// it's acted on in OnControlledEntityChanged below, so it can't fire again on a later respawn.
	protected bool m_bIsJoinInProgress = false;

	//------------------------------------------------------------------------------------------------
	void SetIsJoinInProgress(bool isJip)
	{
		m_bIsJoinInProgress = isJip;
	}

    //------------------------------------------------------------------------------------------------
	//! Opens the JIP forward deploy menu - see the OnControlledEntityChanged gate below for when this fires.
	protected void OpenJIPForwardDeployMenu()
	{
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_JIPForwardDeployMenu);
	}

	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		vanilla.OnControlledEntityChanged(from, to);
		
		if (from)
		{
			SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(from.FindComponent(SCR_CharacterControllerComponent));
			if (charController && charController.IsDead())
			{
				COA_PlayerControllerManager manager = COA_PlayerControllerManager.GetInstance();
				if (manager)
				{
					vector mat[4];
					from.GetTransform(mat);
					manager.m_vPlayersLastDeath = mat;
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

			//Offer JIP (Join In Progress) forward deploy exactly once, on this player's first live-
			//character possession after a genuine join-in-progress (see SetIsJoinInProgress) - not on
			//every spawn/respawn, not for everyone at once at round start, and not when a GM possesses
			//an AI unit via Zeus (neither of those is this player joining). SafeStart must also have
			//ended, and the mission must allow JIP (Disable JIP off).
			//Delayed slightly so it doesn't fight the character loading screen for the top menu slot.
			if (m_bIsJoinInProgress && to && !COA_EntityHelper.IsSpectator(to))
			{
				COA_Gamemode gamemode = COA_Gamemode.GetInstance();
				COA_SafestartManager safestartManager = COA_SafestartManager.GetInstance();
				if (gamemode && safestartManager && !gamemode.m_bLockUnusedSlots && !safestartManager.GetSafestartStatus())
				{
					GetGame().GetCallqueue().CallLater(OpenJIPForwardDeployMenu, 500, false);
					m_bIsJoinInProgress = false;
				}
			}

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
}