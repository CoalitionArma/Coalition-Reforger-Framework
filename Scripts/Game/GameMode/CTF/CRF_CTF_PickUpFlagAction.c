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

		if (pOwnerEntity)
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
		if (rplManager)
			rplManager.RequestCTFFlagPickup(playerId, rplComponent.Id());
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_FlagComponent || !user)
			return false;

		if (m_FlagComponent.IsCarried())
			return false;

		FactionKey userFaction = GetUserFactionKey(user);
		if (userFaction.IsEmpty())
			return false;

		FactionKey flagFaction = m_FlagComponent.GetOwningFaction();

		// A docked home flag can only be stolen by the opposing faction - your own
		// team's flag isn't "picked up" until an enemy has taken it out of base.
		if (m_FlagComponent.IsAtBase() && !flagFaction.IsEmpty() && userFaction == flagFaction)
			return false;

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
