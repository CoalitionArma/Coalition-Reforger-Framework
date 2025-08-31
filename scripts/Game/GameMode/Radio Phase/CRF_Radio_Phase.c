[ComponentEditorProps(category: "Script Component", description: "")]
class CRF_RadioPhaseManagerClass: ScriptComponentClass
{
	
}

class CRF_RadioPhaseManager: ScriptComponent
{
	[Attribute("BLUFOR", "auto", "The side what interacts")]
	FactionKey interactingSide;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnInteracting;
	
	[Attribute("false", "auto", "The object with the script")]
	bool respawnAll;
		
	void fireTrigger()
	{

		if (respawnInteracting)
		{
			CRF_RplToAuthorityManager.GetInstance().RespawnFaction(interactingSide, false);
		}
		
		if (respawnAll)
		{
			array<FactionKey> factionKeys = {"OPFOR", "BLUFOR", "INDFOR", "CIV"};
			
			foreach (FactionKey key : factionKeys)
			{
				CRF_RplToAuthorityManager.GetInstance().RespawnFaction(key, false);
			}
		}
	}
	
}