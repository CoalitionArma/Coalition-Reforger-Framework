class CRF_ObjectSpawnerClass: BaseGameTriggerEntityClass
{

}

class CRF_ObjectSpawner: BaseGameTriggerEntity
{
	[Attribute("", desc: "Object that will spawn in.", category: "CRF Object Spawning", params: "et")] 
	ResourceName m_rObject;
	
	IEntity m_eObject;
	
	override void EOnInit(IEntity owner)
	{
		if (m_rObject.IsEmpty())
		{
			Print(string.Format("No Object set on %1", this), LogLevel.ERROR);
			return;
		}
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		vector entityLocation[4];
		this.GetTransform(entityLocation);
		EntitySpawnParams params = CRF_EntityHelper.CreateSpawnParams(entityLocation);
		
		m_eObject = GetGame().SpawnEntityPrefab(Resource.Load(m_rObject), GetGame().GetWorld(), params);
	}
	
	#ifdef WORKBENCH
	override bool _WB_OnKeyChanged(BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if (key == "m_rObject")
			SCR_EntityHelper.DeleteEntityAndChildren(m_eObject);
		return false;
	}
	
	override event void _WB_SetTransform(inout vector mat[4], IEntitySource src)
	{
		if (m_eObject)
		{
			vector pos[4];
			this.GetTransform(pos);
			UpdateObjectPos(pos);
		}	
	}
	
	void UpdateObjectPos(vector pos[4])
	{
		if(!m_eObject)
			return;
		m_eObject.SetTransform(pos);
		m_eObject.Update();
	}
	#endif
}