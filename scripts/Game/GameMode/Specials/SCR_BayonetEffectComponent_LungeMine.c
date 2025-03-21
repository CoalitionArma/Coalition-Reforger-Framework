class SCR_BayonetEffectComponent_LungeMineClass : SCR_BayonetEffectComponentClass
{
}

class SCR_BayonetEffectComponent_LungeMine : SCR_BayonetEffectComponent
{	
	string exp = "{BC91A8CA354E845B}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge_Inst.et";
	
	//------------------------------------------------------------------------------------------------
	override void OnImpact(notnull IEntity other, float impulse, vector impactPosition, vector impactNormal, GameMaterial mat, vector velocityBefore = vector.Zero, vector velocityAfter = vector.Zero)
	{		

//		if (RplSession.Mode() == RplMode.Dedicated)
//		{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.Transform[3] = impactPosition;
		GetGame().SpawnEntityPrefab(Resource.Load(exp),GetGame().GetWorld(),spawnParams);
		
//		}
	}

}


