class CRF_SnowballComponentClass: ScriptComponentClass
{
}

class CRF_SnowballComponent: ScriptComponent
{
	GrenadeMoveComponent m_Grenade;
	IEntity m_Player;
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		
		SetEventMask(owner, EntityEvent.FRAME | EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		m_Grenade = GrenadeMoveComponent.Cast(owner.FindComponent(GrenadeMoveComponent));
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		GetGame().GetWorld().QueryEntitiesBySphere(owner.GetOrigin(), 0.1, ExplodeCheck, null);
	}
	
	bool ExplodeCheck(IEntity entity)
	{
		if (GetOwner().GetParent())
			return true;
		
		if (ChimeraCharacter.Cast(entity))
		{
			if (!m_Player)
				m_Player = entity;
			
			if (m_Player == entity)
				return true;
			
			//Is this character dead
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(entity);
			if (damageManager)
			{
				if (damageManager.GetState() == EDamageState.DESTROYED)
					return true;
				else
					Explode(entity);
			}
		}
			
		return true;
	}
	
	void Explode(IEntity owner)
	{
		EntitySpawnParams params = new EntitySpawnParams();
		GetOwner().GetTransform(params.Transform);
		GetGame().SpawnEntityPrefab(Resource.Load("{821879B003FB1E17}Prefabs/Weapons/Warheads/Explosions/Snowball_Explosion.et"), null, params);
		SCR_EntityHelper.DeleteEntityAndChildren(GetOwner());
	}
}