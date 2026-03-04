class CRF_GarbageManagerClass : ScriptComponentClass {}

class CRF_GarbageManager : ScriptComponent
{		
	ref array<IEntity> m_aDeadBodies = {};

	//------------------------------------------------------------------------------------------------
	void CleanUpBodies()
	{
		array<IEntity> bodiesToRemove = new array<IEntity>();
		bodiesToRemove.Reserve(m_aDeadBodies.Count()); // Pre-allocate capacity for performance
		
		foreach (IEntity body: m_aDeadBodies)
		{
			if (!body)
				continue;
			
			if (!GetGame().GetWorld().QueryEntitiesBySphere(body.GetOrigin(), 30, CleanUpBodyCallback, null))
				continue;

			bodiesToRemove.Insert(body);
		}
		
		int delay = 1;
		foreach (IEntity body: bodiesToRemove)
		{
			m_aDeadBodies.RemoveItem(body);
			//Lets not delete 100s of entities in one frame now
			GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 100 * delay, false, body);
			delay++;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CleanUpBodyCallback(IEntity entity)
	{
		if (ChimeraCharacter.Cast(entity))
		{
			//Is this character dead
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (damageManager)
			{
				if (damageManager.GetState() == EDamageState.DESTROYED)
					return true;
				else
					return false;
			}
		}
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	static protected CRF_GarbageManager m_sInstance;
	void CRF_GarbageManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_GarbageManager GetInstance()
	{
		return m_sInstance;
	}
}