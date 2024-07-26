[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_AASGameModeComponentClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_AASGameModeComponent: SCR_BaseGameModeComponent
{
	[Attribute("US", "auto", "Faction 1")]
	FactionKey factionOne;
	
	[Attribute("USSR", "auto", "Faction 2")]
	FactionKey factionTwo;
	
	
	
}
