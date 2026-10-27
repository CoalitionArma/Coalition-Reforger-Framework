[EntityEditorProps(category: "GameScripted/Misc", description: "")]
class Quack_GenerticTeleporterEntityClass : GenericEntityClass
{
}

class Quack_GenerticTeleporterEntity : GenericEntity
{
	//------------------------------------------------------------------------------------------------
	override protected void EOnFrame(IEntity owner, float timeSlice)
	{
		// remove if unused
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		// remove if unused
	}

	//------------------------------------------------------------------------------------------------
	void Quack_GenerticTeleporterEntity(IEntitySource src, IEntity parent)
	{
		// SetEventMask(EntityEvent.INIT | EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	void ~Quack_GenerticTeleporterEntity()
	{
		// remove if unused
	}
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
