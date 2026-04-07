//Working with plane prefabs some need to scale up cause of modders being modders
class CRF_AirdropScalerClass: ScriptComponentClass
{

}

class CRF_AirdropScaler: ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		GenericEntity childPlane = GenericEntity.Cast(owner);
		childPlane.SetScale(2);
		childPlane.Update();
		childPlane.OnTransformReset();
	}
}