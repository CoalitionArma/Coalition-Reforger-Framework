[ComponentEditorProps(category: "Game Mode Component", description: "")]
class CRF_RadioPhaseManagerClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_RadioPhaseManager: SCR_BaseGameModeComponent
{
	[Attribute("BLUFOR", "auto", "The side what interacts")]
	FactionKey interactingSide;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnInteracting;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnAll;
	
	void fireTrigger()
	{
		if (respawnInteracting || respawnAll)
		{
			respawn();
		}
	}
	
	
	void respawn()
	{
		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		
		if (respawnAll)
		{
			if (RplSession.Mode() == RplMode.Dedicated) {
				rm.RespawnAllSides();
				return;
			}
		}
		
		if (respawnInteracting)
		{
			if (RplSession.Mode() == RplMode.Dedicated) {
				rm.RespawnSide(interactingSide);
				return;
			}
		}
	}
}