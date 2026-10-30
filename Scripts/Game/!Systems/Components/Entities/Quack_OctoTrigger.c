[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class Quack_OctoTriggerEntityClass : BaseGameTriggerEntityClass
{
}
/****************************************************************************************
* This trigger entity will take the positions of 8 named entities on the map and convert them into position vectors.
* The vector data will then be used to randomly teleport players to one of the 8 locations each. 
* You can use exact coordinates if you're trying to teleport into a narrow area or have the system try to find an empty piece of terrain if you don't 
* want the players to bunch up into each other.
* The trigger should remain dorment until the keys condition within the ChoasControlGameComponent component has be set to 'true'.
/****************************************************************************************/
class Quack_OctoTriggerEntity : BaseGameTriggerEntity
{
	[Attribute("", UIWidgets.Coords, "Coordinates to where the user will teleport to", category: "Area",)]
	protected vector m_coords;
	
	[Attribute("", desc: "Name of entity for target 1", category: "Targets",)]
	protected string m_sTarget1;
	
	[Attribute("", desc: "Name of entity for target 2", category: "Targets",)]
	protected string m_sTarget2;
	
	[Attribute("", desc: "Name of entity for target 3", category: "Targets",)]
	protected string m_sTarget3;
	
	[Attribute("", desc: "Name of entity for target 4", category: "Targets",)]
	protected string m_sTarget4;
	
	[Attribute("", desc: "Name of entity for target 5", category: "Targets",)]
	protected string m_sTarget5;
	
	[Attribute("", desc: "Name of entity for target 6", category: "Targets",)]
	protected string m_sTarget6;
	
	[Attribute("", desc: "Name of entity for target 7", category: "Targets",)]
	protected string m_sTarget7;
	
	[Attribute("", desc: "Name of entity for target 8", category: "Targets",)]
	protected string m_sTarget8;
	
	[Attribute(defvalue: "1", desc: "Whether or not the character will teleport to the exact coordinates or a within the general area", category: "Area")]
	protected bool m_bUseExactCoordinates;
	
	ref array<IEntity> playersInside = {};
	ref array<string> Targets = {m_sTarget1, m_sTarget2, m_sTarget3, m_sTarget4, m_sTarget5, m_sTarget6, m_sTarget7, m_sTarget8};
		
	//------------------------------------------------------------------------------------------------
	override protected void OnActivate(IEntity ent)
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if(!gamemode)
		{
			Print("No game mode found",LogLevel.ERROR);
			return;
		}
		// Check if the 'key' variable within the ChoasControlGameComponent component exists.
		Quack_chaosControlGameComponent VariableCheck = Quack_chaosControlGameComponent.Cast(gamemode.FindComponent(Quack_chaosControlGameComponent));
		if(!VariableCheck)
			return;	

		int keycheck = VariableCheck.keys;
		Print(keycheck);
		// Check if the variable is set to true. and if it is, start flinging players across the play area.
		if(keycheck != true)
			return;
		
		Print("Something is inside the teleporter");
		GetGame().GetCallqueue().CallLater(popPlayerArray, 1000, false);
	}

	//------------------------------------------------------------------------------------------------
	// Teleport players into the exact location by choosing a random location.	
	protected void teleportEnt()
	{
		foreach(IEntity playerInField : playersInside)
		{
			int randomElement = Math.RandomInt(0, 8);		
			string TargetObj = Targets[randomElement];
			IEntity teleportObject = GetGame().GetWorld().FindEntityByName(TargetObj);
			vector telePos = teleportObject.GetOrigin();
			
			playerInField.SetOrigin(telePos);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Teleport players into a general area if it's safe by choosing a random location.	
	protected void teleportgen()
	{
		foreach(IEntity playerInField : playersInside)
		{
			int randomElement = Math.RandomInt(0, 8);
						
			string TargetObj = Targets[randomElement];
			IEntity teleportObject = GetGame().GetWorld().FindEntityByName(TargetObj);
			vector telePos = teleportObject.GetOrigin();
			
			vector finalSpawnLocation = vector.Zero;
			SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, telePos, 3);
			playerInField.SetOrigin(finalSpawnLocation);
			Print(finalSpawnLocation)
		}
	}

	//------------------------------------------------------------------------------------------------	
	override protected void OnDeactivate(IEntity ent)
	{
		Print("Something has left.")
	}
	
	//------------------------------------------------------------------------------------------------
	protected void popPlayerArray()
	{
		QueryEntitiesInside();
        int Count = GetEntitiesInside(playersInside);    
        Print(Count);
		if (m_bUseExactCoordinates)
		{
			teleportEnt();
			Print("Using exact coordinates")
		}
		else
		{
			teleportgen();
			Print("Searching to safe areas");
		}
	}
}
