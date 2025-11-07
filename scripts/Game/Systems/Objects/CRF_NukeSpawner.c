class CRF_NukeSpawnerClass: BaseGameTriggerEntityClass
{
	
}

class CRF_NukeSpawner: BaseGameTriggerEntity
{
	void SpawnNuke()
	{
		EntitySpawnParams params = new EntitySpawnParams();
		this.GetTransform(params.Transform);
		
		ParticleEffectEntity.Cast(GetGame().SpawnEntityPrefab(Resource.Load("{77C479ACA308A41E}Prefabs/Nuke/CRF_Nuke.et"), null, params));
		SCR_CameraShakeManagerComponent.AddCameraShake(
			1.0,      
			1.0,     
			0.05,              
			0.0,          
			3.0               
		);
	}
	
	void SpawnNukeSound()
	{
		EntitySpawnParams params = new EntitySpawnParams();
		this.GetTransform(params.Transform);
		
		IEntity sound = GetGame().SpawnEntityPrefab(Resource.Load("{6D887D33B0E6CB31}Prefabs/Nuke/CRF_NukeSound.et"), null, params);
		
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId: playerIds)
		{
			RplComponent rplComp = RplComponent.Cast(sound.FindComponent(RplComponent));
            RplIdentity rplIdentity = GetGame().GetPlayerManager().GetPlayerController(playerId).GetRplIdentity();
            rplComp.EnableStreamingConNode(rplIdentity, true);
		}
		
		GetGame().GetCallqueue().CallLater(CleanUpSoundObject, 30000, false, sound);
	}
	
	void CleanUpSoundObject(IEntity soundObject)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(soundObject);
	}

	override void EOnInit(IEntity owner)
	{
		GetGame().GetCallqueue().CallLater(SpawnNuke, 1500, false);
		
		#ifdef WORKBENCH
		SpawnNukeSound();
		#else
		if (System.IsConsoleApp())
			SpawnNukeSound();
		#endif
	}
}