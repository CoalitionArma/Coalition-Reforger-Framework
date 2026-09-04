//------------------------------------------------------------------------------------
// CRF_CTF_PickUpFlagAction: Pickup interaction for CRF_CTF_FlagComponent.
// Attach to the flag prefab's UserActionsComponent. Visibility mirrors the gamemode's
// pickup rules for immediate UI feedback; the actual state change is re-validated
// server-side in CRF_CTFGamemodeManager.TryPickUpFlag() before it takes effect.
//------------------------------------------------------------------------------------

class CRF_CTF_PickUpFlagAction : ScriptedUserAction
{
	protected CRF_CTF_FlagComponent m_FlagComponent;

	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		if (!GetGame().InPlayMode())
			return;

		if (!pOwnerEntity)
			return;

		m_FlagComponent = CRF_CTF_FlagComponent.Cast(pOwnerEntity.FindComponent(CRF_CTF_FlagComponent));
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);

		if (!m_FlagComponent || !pOwnerEntity || !pUserEntity)
			return;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (playerId <= 0)
			return;

		RplComponent rplComponent = RplComponent.Cast(pOwnerEntity.FindComponent(RplComponent));
		if (!rplComponent)
			return;

		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!rplManager)
			return;

		rplManager.RequestCTFFlagPickup(playerId, rplComponent.Id());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		string reason;
		return EvaluateVisibility(user, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[out] reason Why the result came out the way it did
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
