modded class SCR_SeizingComponent
{
	//------------------------------------------------------------------------------------------------
	// Helper method to find entity name by searching common naming patterns in the world
	// Since IEntity doesn't have GetName(), we need to search for known names
	protected string FindEntityNameInWorld(IEntity entity)
	{
		if (!entity)
			return "";
		
		// Common objective naming patterns to search for
		array<string> commonNames = {
			"OBJ_A", "OBJ_B", "OBJ_C", "OBJ_D", "OBJ_E",
			"ObjectiveA", "ObjectiveB", "ObjectiveC", "ObjectiveD", "ObjectiveE",
			"zone_a", "zone_b", "zone_c", "zone_d", "zone_e",
			"Alpha", "Beta", "Charlie", "Delta", "Echo",
			"Capture_A", "Capture_B", "Capture_C", "Capture_D"
		};
		
		// Try to find this entity by checking common names
		foreach (string name : commonNames)
		{
			IEntity foundEntity = GetGame().GetWorld().FindEntityByName(name);
			if (foundEntity == entity)
				return name;
		}
		
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	override void NotifyPlayerInRadius(notnull SCR_Faction faction)
	{
		//TODO - Get faction and capture point info so it can be used in all game modes
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
		{
			// Get the entity this component is attached to
			IEntity ownerEntity = GetOwner();
			string hintTitle = "OBJECTIVE CAPTURED";
			string hintMessage = "The next OBJ is now active!";
			
			// Get prefab name or search for entity name in world
			if (ownerEntity)
			{
				// Get parent's parent entity (the actual objective entity with the name)
				// Hierarchy: OBJ_A -> MilitaryBaseLogic -> CapturePoint (this component)
				IEntity parentEntity = ownerEntity.GetParent();
				IEntity targetEntity = ownerEntity; // Default to owner if no parent
				
				if (parentEntity)
				{
					PrintFormat("[CRF_SeizingComponent] Found parent entity");
					IEntity grandParentEntity = parentEntity.GetParent();
					
					if (grandParentEntity)
					{
						targetEntity = grandParentEntity;
						PrintFormat("[CRF_SeizingComponent] Using grandparent entity (objective root)");
					}
					else
					{
						targetEntity = parentEntity;
						PrintFormat("[CRF_SeizingComponent] Using parent entity (no grandparent)");
					}
				}
				else
				{
					PrintFormat("[CRF_SeizingComponent] No parent, using owner entity");
				}
				
				// Try to get the prefab name
				string prefabName = targetEntity.GetPrefabData().GetPrefabName();
				PrintFormat("[CRF_SeizingComponent] Prefab name: %1", prefabName);
				
				// Try to find this entity by iterating through named entities in the world
				// This is a workaround since entities don't have a GetName() method
				string foundName = FindEntityNameInWorld(targetEntity);
				PrintFormat("[CRF_SeizingComponent] Entity name in world: %1", foundName);
				
				// Use whichever name is more useful
				string identifierName = foundName;
				if (identifierName.IsEmpty())
					identifierName = prefabName;
				
				// Customize hint based on entity name or prefab
				if (identifierName.Contains("OBJ_A") || identifierName.Contains("ObjectiveA") || identifierName.Contains("zone_a") || identifierName.Contains("Alpha"))
				{
					hintTitle = "OBJECTIVE A CAPTURED";
					hintMessage = "Objective A has been captured by " + faction.GetFactionName() + "!";
				}
				else if (identifierName.Contains("OBJ_B") || identifierName.Contains("ObjectiveB") || identifierName.Contains("zone_b") || identifierName.Contains("Beta"))
				{
					hintTitle = "OBJECTIVE B CAPTURED";
					hintMessage = "Objective B has been captured by " + faction.GetFactionName() + "!";
				}
				else if (identifierName.Contains("OBJ_C") || identifierName.Contains("ObjectiveC") || identifierName.Contains("zone_c") || identifierName.Contains("Charlie"))
				{
					hintTitle = "OBJECTIVE C CAPTURED";
					hintMessage = "Objective C has been captured by " + faction.GetFactionName() + "!";
				}
				else if (identifierName.Contains("OBJ_D") || identifierName.Contains("ObjectiveD") || identifierName.Contains("zone_d") || identifierName.Contains("Delta"))
				{
					hintTitle = "OBJECTIVE D CAPTURED";
					hintMessage = "Objective D has been captured by " + faction.GetFactionName() + "!";
				}
				else
				{
					// Default message with faction info
					hintTitle = "OBJECTIVE CAPTURED BY " + faction.GetFactionKey();
					if (!foundName.IsEmpty())
						hintMessage = string.Format("'%1' has been captured!", foundName);
					else
						hintMessage = "The next OBJ is now active!";
				}
			}
			
			hintManager.ShowCustomHint(hintMessage, hintTitle, 10);
		};
		
		CRF_RespawnManager rm = CRF_RespawnManager.GetInstance();
		if (rm)
			rm.RespawnAllSides();
		
		super.NotifyPlayerInRadius(faction);
	}
}