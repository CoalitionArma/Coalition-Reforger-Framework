class CRF_VehicleSpawnerClass: ScriptComponentClass
{

}

class CRF_VehicleSpawner: ScriptComponent
{
	[Attribute("")]
	ResourceName m_rVehicle;
	
	protected IEntity m_eVehicle;
	protected IEntity m_eOldVehicle;
	
	override void OnPostInit(IEntity owner)
	{
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(SpawnVehicle, 3000, false);
	}
	
	void SpawnVehicle()
	{
		if(m_rVehicle == "")
			return;
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		GetOwner().GetTransform(params.Transform);
		m_eVehicle = GetGame().SpawnEntityPrefab(Resource.Load(m_rVehicle), GetGame().GetWorld(), params);
	}
	
	#ifdef WORKBENCH
	override void _WB_OnInit(IEntity owner, inout vector mat[4], IEntitySource src)
	{
		SpawnVehicle();
	}
	
	override bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if (key == "m_rVehicle")
			SCR_EntityHelper.DeleteEntityAndChildren(m_eVehicle);
		return false;
	}
	
	override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if(!m_rVehicle && m_eVehicle)
			delete m_eVehicle;
		
		if(m_rVehicle && !m_eVehicle)
			SpawnVehicle();
		
		if(m_eVehicle)
		{
			vector pos[4];
			GetOwner().GetTransform(pos);
			UpdateVehiclePos(pos);
		}
			
			
	}
	
	void UpdateVehiclePos(vector pos[4])
	{
		if(!m_eVehicle)
			return;
		m_eVehicle.SetTransform(pos);
		m_eVehicle.Update();
	}
	#endif
	
	void ~CRF_VehicleSpawner()
	{
		#ifdef WORKBENCH
				SCR_EntityHelper.DeleteEntityAndChildren(m_eVehicle);
		#endif
	}
}