#ifdef WORKBENCH
//! Reports which CRF gamemode logic components are attached to COA_Lobby, for the synopsis'
//! "Game Mode Components" section. A mission can stack more than one (e.g. Rally + Attrition),
//! which is why this is a list rather than the single authored COA_EGamemode label.
//!
//! This mods the generator rather than the config plugin on purpose: Workbench instantiates plugins
//! from the class carrying WorkbenchPluginAttribute, so a modded COA_MissionConfigurationPlugin is
//! never instantiated, while a modded plain script class like this one works normally.
modded class COA_MissionSynopsisGenerator
{
	//------------------------------------------------------------------------------------------------
	protected override void GetActiveGameModeComponents(IEntitySource entitySource, notnull array<string> activeComponents)
	{
		super.GetActiveGameModeComponents(entitySource, activeComponents);

		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_AttritionGamemodeComponent) >= 0)
			activeComponents.Insert("Attrition");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_FrontlineGamemodeManager) >= 0)
			activeComponents.Insert("Frontline");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_HighValueTargetGamemodeManager) >= 0)
			activeComponents.Insert("High Value Target");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_InsurgencyGamemodeManager) >= 0)
			activeComponents.Insert("Insurgency");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_RaidGamemodeComponent) >= 0)
			activeComponents.Insert("Raid");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_RallyGamemodeComponent) >= 0)
			activeComponents.Insert("Rally");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_RushGamemodeManager) >= 0)
			activeComponents.Insert("Rush");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_SearchAndDestroyGamemodeManager) >= 0)
			activeComponents.Insert("Search And Destroy");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_LooterGamemodeComponent) >= 0)
			activeComponents.Insert("Looter");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_PropHuntGamemode) >= 0)
			activeComponents.Insert("Prop Hunt");
		if (SCR_BaseContainerTools.FindComponentIndex(entitySource, CRF_SupplyExtractionGamemodeManager) >= 0)
			activeComponents.Insert("Supply Extraction");
	}
}
#endif
