//------------------------------------------------------------------------------------
// CRF_RushPlantBombAction: Plant bomb action for Rush gamemode
// Allows attacking team to plant bombs on MCOM sites in the active zone
//------------------------------------------------------------------------------------

class CRF_RushPlantBombAction : ScriptedUserAction
{	
	protected SCR_FactionManager m_FactionManager;
	protected CRF_RushGamemodeManager m_RushGamemode;
	
	/**
	 * Initialize the action with necessary manager references
	 * @param pOwnerEntity The entity that owns this action
	 * @param pManagerComponent The action manager component
	 */
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode()) 
		{
			return;
		}
		
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_RushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
	}
	
	/**
	 * Perform the plant bomb action
	 * @param pOwnerEntity The MCOM entity being interacted with
	 * @param pUserEntity The player performing the action
	 */
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || !pUserEntity || !m_RushGamemode)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(pUserEntity);
		if (!character)
			return;
		
		// Determine which MCOM is being planted
		string mcomIdentifier = GetMCOMIdentifier(pOwnerEntity);
		if (mcomIdentifier.IsEmpty())
			return;
		
		// Send RPC to stop planting sound since action completed successfully
		CRF_RplToAuthorityManager.GetInstance().StopRushPlantingSound();
		
		// Send plant command to authority (this will trigger bomb ticking sound)
		CRF_RplToAuthorityManager.GetInstance().ToggleRushMCOMPlanted(mcomIdentifier, true);
		
		// Clear the planting MCOM since action completed
		if (m_RushGamemode)
		{
			m_RushGamemode.SetPlantingMCOM("");
		}
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);
		
		// Determine which MCOM is being planted and notify the gamemode
		IEntity ownerEntity = GetOwner();
		if (ownerEntity && m_RushGamemode)
		{
			string mcomIdentifier = GetMCOMIdentifier(ownerEntity);
			if (!mcomIdentifier.IsEmpty())
			{
				// Set which MCOM is being planted for sound placement
				m_RushGamemode.SetPlantingMCOM(mcomIdentifier);
			}
		}
		
		// Send RPC to start planting sound globally
		CRF_RplToAuthorityManager.GetInstance().StartRushPlantingSound();
	}
	
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.OnActionCanceled(pOwnerEntity, pUserEntity);
		
		// Clear the planting MCOM since action was cancelled
		if (m_RushGamemode)
		{
			m_RushGamemode.SetPlantingMCOM("");
		}
		
		// Send RPC to stop planting sound globally when action is canceled
		CRF_RplToAuthorityManager.GetInstance().StopRushPlantingSound();
	}
	
	/**
	 * Check if the action can be shown to the user
	 * @param user The user entity
	 * @return True if action should be visible
	 */
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_FactionManager || !m_RushGamemode)
		{
			return false;
		}
		
		if (!user)
		{
			return false;
		}
		
		FactionKey userFactionKey;
		
		// Try method 1: FactionAffiliationComponent (preferred)
		FactionAffiliationComponent userAffiliation = FactionAffiliationComponent.Cast(user.FindComponent(FactionAffiliationComponent));
		if (userAffiliation)
		{
			Faction userFaction = userAffiliation.GetAffiliatedFaction();
			if (userFaction)
			{
				userFactionKey = userFaction.GetFactionKey();
			}
		}
		
		// Try method 2: Local player faction (fallback)
		if (userFactionKey.IsEmpty())
		{
			Faction localPlayerFaction = m_FactionManager.GetLocalPlayerFaction();
			if (localPlayerFaction)
			{
				userFactionKey = localPlayerFaction.GetFactionKey();
			}
		}
		
		// If still no faction found, fail
		if (userFactionKey.IsEmpty())
		{
			return false;
		}
		
		FactionKey attackingFactionKey = m_RushGamemode.GetAttackingSide();
		
		// Only show to attacking faction
		if (userFactionKey != attackingFactionKey)
		{
			return false;
		}
		
		// Check if this MCOM is in the active zone
		IEntity ownerEntity = GetOwner();
		if (!ownerEntity)
		{
			return false;
		}
		
		string mcomIdentifier = GetMCOMIdentifier(ownerEntity);
		if (mcomIdentifier.IsEmpty())
		{
			return false;
		}
		
		// Only show for MCOMs in the current active zone
		if (!m_RushGamemode.IsMCOMInActiveZone(mcomIdentifier))
		{
			return false;
		}
		
		return true;
	}	
	
	/**
	 * Check if the action can be performed
	 * @param user The user entity
	 * @return True if action can be performed
	 */
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_RushGamemode)
		{
			return false;
		}
		
		// Don't allow planting if countdown is already active (only one bomb at a time)
		if (m_RushGamemode.IsCountdownActive())
		{
			return false;
		}
		
		// Check if this MCOM is in the active zone
		IEntity ownerEntity = GetOwner();
		if (!ownerEntity)
		{
			return false;
		}
		
		string mcomIdentifier = GetMCOMIdentifier(ownerEntity);
		if (mcomIdentifier.IsEmpty())
		{
			return false;
		}
		
		// Only allow planting in the current active zone
		if (!m_RushGamemode.IsMCOMInActiveZone(mcomIdentifier))
		{
			return false;
		}
		
		return true;
	}	
	
	/**
	 * Get the MCOM identifier from the entity ID
	 * @param mcomEntity The MCOM entity
	 * @return MCOM identifier string
	 */
	protected string GetMCOMIdentifier(IEntity mcomEntity)
	{
		if (!mcomEntity || !m_RushGamemode)
			return "";
		
		EntityID entityID = mcomEntity.GetID();
		
		// Check against all MCOM entity IDs
		if (entityID == m_RushGamemode.GetMCOMEntityID("Zone1Alpha"))
			return "Zone1Alpha";
		if (entityID == m_RushGamemode.GetMCOMEntityID("Zone1Beta"))
			return "Zone1Beta";
		if (entityID == m_RushGamemode.GetMCOMEntityID("Zone2Alpha"))
			return "Zone2Alpha";
		if (entityID == m_RushGamemode.GetMCOMEntityID("Zone2Beta"))
			return "Zone2Beta";
		if (entityID == m_RushGamemode.GetMCOMEntityID("Zone3Alpha"))
			return "Zone3Alpha";
		if (entityID == m_RushGamemode.GetMCOMEntityID("Zone3Beta"))
			return "Zone3Beta";
		
		return "";
	}
	
	/**
	 * This action has local effects only
	 * @return True
	 */
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	/**
	 * This action should not be broadcast
	 * @return False
	 */
	override bool CanBroadcastScript()
	{
		return false;
	}
}
