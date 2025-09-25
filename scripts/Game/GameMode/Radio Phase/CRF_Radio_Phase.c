[ComponentEditorProps(category: "Script Component", description: "")]
class CRF_RadioPhaseManagerClass: ScriptComponentClass
{
	
}

class CRF_RadioPhaseManager: ScriptComponent
{
	[Attribute("BLUFOR", "auto", "The side what interacts")]
	FactionKey interactingSide;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnBlufor;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnOpfor;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnIndfor;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnCiv;
		
	void fireTrigger()
	{
		
		if (respawnBlufor)
		{
			CRF_RplToAuthorityManager.GetInstance().RespawnFaction("BLUFOR", false);
		}
		
		if (respawnOpfor)
		{
			CRF_RplToAuthorityManager.GetInstance().RespawnFaction("OPFOR", false);
		}
		
		if (respawnIndfor)
		{
			CRF_RplToAuthorityManager.GetInstance().RespawnFaction("INDFOR", false);
		}
		
		if (respawnCiv)
		{
			CRF_RplToAuthorityManager.GetInstance().RespawnFaction("CIV", false);
		}
	}
	
}