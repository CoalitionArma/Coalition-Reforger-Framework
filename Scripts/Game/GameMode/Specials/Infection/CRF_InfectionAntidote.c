//------------------------------------------------------------------------------------
// Antidote crates for CRF_InfectionGamemodeComponent (prefab: Prefabs/Props/Infection/CRF_AntidoteCrate.et).
// Every crate draws from the mission's one shared pool of doses. A survivor injects themselves at a
// crate; the server checks they are at a real crate and decides what the dose does.
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! Marks an entity as an antidote crate, so the server can check a request really came from one
[ComponentEditorProps(category: "CRF/Infection", description: "Antidote crate for the infection special. Add CRF_InfectionAntidoteAction to the entity's actions.")]
class CRF_InfectionAntidoteStationClass : ScriptComponentClass {}

class CRF_InfectionAntidoteStation : ScriptComponent
{
	protected static ref array<IEntity> s_aStations = {};

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (GetGame().InPlayMode())
			s_aStations.Insert(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		s_aStations.RemoveItem(owner);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! A crate within a couple of metres of pos
	static bool IsStationAt(vector pos)
	{
		foreach (IEntity station : s_aStations)
		{
			if (station && vector.Distance(station.GetOrigin(), pos) < 2)
				return true;
		}

		return false;
	}
}

//------------------------------------------------------------------------------------------------
//! "Inject antidote (N left)" - survivors only
class CRF_InfectionAntidoteAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!pOwnerEntity || pUserEntity != SCR_PlayerController.GetLocalControlledEntity())
			return;

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (authorityManager)
			authorityManager.RequestInfectionAntidote(pOwnerEntity.GetOrigin());

		super.PerformAction(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		CRF_InfectionGamemodeComponent infection = CRF_InfectionGamemodeComponent.GetInstance();
		if (!infection || !user)
			return false;

		// Survivors only - the turned have no use for it
		Faction faction = COA_SlottingManager.GetInstance().GetPlayerSlotFaction(SCR_PlayerController.GetLocalPlayerId(), true);
		return faction && faction.GetFactionKey() == infection.GetSurvivorFaction();
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		CRF_InfectionGamemodeComponent infection = CRF_InfectionGamemodeComponent.GetInstance();
		if (!infection || infection.GetAntidoteDoses() <= 0)
		{
			SetCannotPerformReason("No doses left");
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		CRF_InfectionGamemodeComponent infection = CRF_InfectionGamemodeComponent.GetInstance();
		if (!infection)
			return false;

		outName = string.Format("Inject antidote (%1 left)", infection.GetAntidoteDoses());
		return true;
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
