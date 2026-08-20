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
