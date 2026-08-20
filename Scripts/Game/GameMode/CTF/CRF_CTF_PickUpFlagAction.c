//------------------------------------------------------------------------------------
// CRF_CTF_PickUpFlagAction: Pickup interaction for CRF_CTF_FlagComponent.
// Attach to the flag prefab's UserActionsComponent. Visibility mirrors the gamemode's
// pickup rules for immediate UI feedback; the actual state change is re-validated
// server-side in CRF_CTFGamemodeManager.TryPickUpFlag() before it takes effect.
//------------------------------------------------------------------------------------

class CRF_CTF_PickUpFlagAction : ScriptedUserAction
{
	protected CRF_CTF_FlagComponent m_FlagComponent;

	// Throttles CanBeShownScript logging to state transitions only - the interaction system
	// calls it continuously while a player is in range, so logging every call would flood
	// the log without adding information.
	protected string m_sLastLoggedVisibilityReason = "";

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode())
		{
			Print("[CRF_CTF] PickUpFlagAction.Init: skipped, GetGame().InPlayMode() is false on this machine.", LogLevel.WARNING);
			return;
		}

		if (!pOwnerEntity)
		{
			Print("[CRF_CTF] PickUpFlagAction.Init: no owner entity passed in.", LogLevel.WARNING);
			return;
		}

		m_FlagComponent = CRF_CTF_FlagComponent.Cast(pOwnerEntity.FindComponent(CRF_CTF_FlagComponent));
		if (!m_FlagComponent)
			Print(string.Format("[CRF_CTF] PickUpFlagAction.Init: entity '%1' has no CRF_CTF_FlagComponent - this action will never be shown.", pOwnerEntity.GetName()), LogLevel.WARNING);
		else
			Print(string.Format("[CRF_CTF] PickUpFlagAction.Init: bound to flag '%1' on entity '%2'.", m_FlagComponent.GetDisplayName(), pOwnerEntity.GetName()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		if (!m_FlagComponent || !pOwnerEntity || !pUserEntity)
		{
			Print("[CRF_CTF] PickUpFlagAction REJECTED: missing flag component/owner/user entity.", LogLevel.WARNING);
			return;
		}

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (playerId <= 0)
		{
			Print(string.Format("[CRF_CTF] PickUpFlagAction REJECTED: GetPlayerIdFromControlledEntity returned %1 for the acting entity.", playerId), LogLevel.WARNING);
			return;
		}

		RplComponent rplComponent = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		if (!rplComponent)
		{
			Print(string.Format("[CRF_CTF] PickUpFlagAction REJECTED: flag entity '%1' has no RplComponent - add one to the flag prefab, pickups cannot work without it.", pOwnerEntity.GetName()), LogLevel.WARNING);
			return;
		}

		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!rplManager)
		{
			Print("[CRF_CTF] PickUpFlagAction REJECTED: COA_PlayerRplToAuthorityManager.GetInstance() returned null.", LogLevel.WARNING);
			return;
		}

		Print(string.Format("[CRF_CTF] PickUpFlagAction: requesting pickup for playerId=%1, flag RplId=%2.", playerId, rplComponent.Id()), LogLevel.NORMAL);
		rplManager.RequestCTFFlagPickup(playerId, rplComponent.Id());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		string reason;
		bool result = EvaluateVisibility(user, reason);

		if (reason != m_sLastLoggedVisibilityReason)
		{
			m_sLastLoggedVisibilityReason = reason;
			Print(string.Format("[CRF_CTF] PickUpFlagAction.CanBeShownScript -> %1 (%2)", result, reason), LogLevel.NORMAL);
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! \param[out] reason Why the result came out the way it did, for CanBeShownScript's logging
	protected bool EvaluateVisibility(IEntity user, out string reason)
	{
		if (!m_FlagComponent)
		{
			reason = "no CRF_CTF_FlagComponent bound - Init() never found one on this entity";
			return false;
		}

		if (!user)
		{
			reason = "no user entity passed in";
			return false;
		}

		if (m_FlagComponent.IsCarried())
		{
			reason = "flag is already carried";
			return false;
		}

		FactionKey userFaction = GetUserFactionKey(user);
		if (userFaction.IsEmpty())
		{
			reason = "user has no FactionAffiliationComponent / no affiliated faction";
			return false;
		}

		FactionKey flagFaction = m_FlagComponent.GetOwningFaction();

		// A docked home flag can only be stolen by the opposing faction - your own
		// team's flag isn't "picked up" until an enemy has taken it out of base.
		if (m_FlagComponent.IsAtBase() && !flagFaction.IsEmpty() && userFaction == flagFaction)
		{
			reason = string.Format("user faction %1 matches this docked home flag's owning faction", userFaction);
			return false;
		}

		reason = string.Format("visible (user faction %1)", userFaction);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	//------------------------------------------------------------------------------------------------
	protected FactionKey GetUserFactionKey(IEntity user)
	{
		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(user.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return "";

		Faction faction = affiliation.GetAffiliatedFaction();
		if (!faction)
			return "";

		return faction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
