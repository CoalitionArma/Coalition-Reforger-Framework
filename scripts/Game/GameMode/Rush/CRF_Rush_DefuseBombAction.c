//------------------------------------------------------------------------------------
// CRF_RushDefuseBombAction: Defuse bomb action for Rush gamemode
// Allows defending team to defuse planted bombs on MCOM sites
//------------------------------------------------------------------------------------

class CRF_RushDefuseBombAction : ScriptedUserAction
{	
	protected SCR_FactionManager m_FactionManager;
	protected CRF_RushGamemodeManager m_RushGamemode;
	protected EntityID m_EntityID;
	
	/**
	 * Initialize the action with necessary manager references
	 * @param pOwnerEntity The entity that owns this action
	 * @param pManagerComponent The action manager component
	 */
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent) 
	{
		if (!GetGame().InPlayMode()) 
			return;
		
		if (pOwnerEntity)
			m_EntityID = pOwnerEntity.GetID();
		
		m_FactionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		m_RushGamemode = CRF_RushGamemodeManager.Cast(GetGame().GetGameMode().FindComponent(CRF_RushGamemodeManager));
	}
	
	/**
	 * Perform the defuse bomb action
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
		
		// Determine which MCOM is being defused
		string mcomIdentifier = GetMCOMIdentifier(pOwnerEntity);
		if (mcomIdentifier.IsEmpty())
			return;

		// Stop the defuse sound since action completed successfully
		CRF_RplToAuthorityManager.GetInstance().StopRushDefuseSound();

		// Explicitly stop bomb ticking sound before defusing
		CRF_RplToAuthorityManager.GetInstance().StopRushBombTickingSound();

		// Send defuse command to authority (this will handle stopping bomb ticking sound)
		CRF_RplToAuthorityManager.GetInstance().ToggleRushMCOMPlanted(mcomIdentifier, false);
		
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
	
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);
		
		// Send RPC to start defuse sound globally
		CRF_RplToAuthorityManager.GetInstance().StartRushDefuseSound();
	}
	
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.OnActionCanceled(pOwnerEntity, pUserEntity);
		
		// Send RPC to stop defuse sound globally when action is canceled
		CRF_RplToAuthorityManager.GetInstance().StopRushDefuseSound();
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
		
		// Get user's faction affiliation component
		FactionAffiliationComponent userAffiliation = FactionAffiliationComponent.Cast(user.FindComponent(FactionAffiliationComponent));
		if (!userAffiliation)
		{
			return false;
		}
		
		// Get user's faction
		Faction userFaction = userAffiliation.GetAffiliatedFaction();
		if (!userFaction)
		{
			return false;
		}
		
		// Get user's faction key
		FactionKey userFactionKey = userFaction.GetFactionKey();
		FactionKey defendingFactionKey = m_RushGamemode.GetDefendingSide();
		
		// Only show to defending faction
		if (userFactionKey != defendingFactionKey)
		{
			return false;
		}
		
		// Only show if countdown is active
		if (!m_RushGamemode.IsCountdownActive())
		{
			return false;
		}
		
		// Only show if this specific MCOM is planted
		string mcomIdentifier = GetMCOMIdentifier(GetOwner());
		if (mcomIdentifier.IsEmpty())
		{
			return false;
		}
		
		bool isPlanted = m_RushGamemode.GetMCOMPlantedStatus(mcomIdentifier);
		
		return isPlanted;
	}
	
	/**
	 * Check if the action can be performed
	 * @param user The user entity
	 * @return True if action can be performed
	 */
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_RushGamemode)
			return false;
		
		// Can only defuse if countdown is active
		if (!m_RushGamemode.IsCountdownActive())
			return false;
		
		// Check if this specific MCOM is planted
		string mcomIdentifier = GetMCOMIdentifier(GetOwner());
		if (mcomIdentifier.IsEmpty())
			return false;
		
		return m_RushGamemode.GetMCOMPlantedStatus(mcomIdentifier);
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
		
		// Check against all MCOM entity IDs - try new sequential system first
		if (entityID == m_RushGamemode.GetMCOMEntityID("MCOMA"))
			return "MCOMA";
		if (entityID == m_RushGamemode.GetMCOMEntityID("MCOMB"))
			return "MCOMB";
		if (entityID == m_RushGamemode.GetMCOMEntityID("MCOMC"))
			return "MCOMC";
		if (entityID == m_RushGamemode.GetMCOMEntityID("MCOMD"))
			return "MCOMD";
		if (entityID == m_RushGamemode.GetMCOMEntityID("MCOME"))
			return "MCOME";
		if (entityID == m_RushGamemode.GetMCOMEntityID("MCOMF"))
			return "MCOMF";
		
		// Legacy compatibility - check old identifiers
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
