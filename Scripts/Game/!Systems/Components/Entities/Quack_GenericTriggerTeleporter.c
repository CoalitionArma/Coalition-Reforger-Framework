[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class Quack_GenericTriggerTeleporterEntityClass : BaseGameTriggerEntityClass
{
}

class Quack_GenericTriggerTeleporterEntity : BaseGameTriggerEntity
{
	[Attribute("", UIWidgets.Coords, "Coordinates to where the user will teleport to", category: "Area",)]
	protected vector m_coords;
	
	[Attribute(defvalue: "1", desc: "Whether or not the character will teleport to the exact coordinates or a within the general area", category: "Area")]
	protected bool m_bUseExactCoordinates;
	
	ref array<IEntity> playersInside = {};
	
	//------------------------------------------------------------------------------------------------
	override protected void OnActivate(IEntity ent)
	{
		Print("Something is inside the teleporter");
		GetGame().GetCallqueue().CallLater(popPlayerArray, 1000, false);
		
	}
	
	protected void teleportEnt()
	{
		foreach(IEntity playerInField : playersInside)
		{
			playerInField.SetOrigin(m_coords);
		}
	}
	
	protected void teleportgen()
	{
		foreach(IEntity playerInField : playersInside)
		{
			vector finalSpawnLocation = vector.Zero;
			SCR_WorldTools.FindEmptyTerrainPosition(finalSpawnLocation, m_coords, 3);
			playerInField.SetOrigin(finalSpawnLocation);
			Print(finalSpawnLocation)
		}
	}
	
	override protected void OnDeactivate(IEntity ent)
	{
		Print("Something has left.")
	}
	
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
			Print("we're mooving! moooooo!");
		}
	}
	//------------------------------------------------------------------------------------------------
	/*
	override void EOnActivate(IEntity owner)
	{
		super.EOnActivate(owner);
		SetEventMask(owner, EntityEvent.FRAME);
	}
 
	override void EOnDeactivate(IEntity owner)
	{
		super.EOnDeactivate(owner);
		ClearEventMask(owner, EntityEvent.FRAME);
	}
	*/
}
