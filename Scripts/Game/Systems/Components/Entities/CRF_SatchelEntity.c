class CRF_SatchelEntityClass : GenericEntityClass {}

class CRF_SatchelEntity : GenericEntity
{
	// Scale constants for different types of satchels
	protected const float BLUFOR_OPFOR_SCALE = 2.250;
	protected const float INDFOR_SCALE = 3.050;
	
	void CRF_SatchelEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.VISIBLE);
	}
	
	//------------------------------------------------------------------------------------------------
	override event protected void EOnVisible(IEntity owner, int frameNumber)
	{	
		super.EOnVisible(owner, frameNumber);
		
		string prefabName = GetPrefabData().GetPrefabName();
		float scaleValue = GetScaleForPrefab(prefabName);
		
		if (scaleValue > 0)
		{
			SetScale(scaleValue);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected float GetScaleForPrefab(string prefabName)
	{
		// BLUFOR and OPFOR satchels
		if (prefabName == "{55641F140F92FDCD}Prefabs/Weapons/Explosives/Explosive_Satchels/BLUFOR_Satchel.et" ||
			prefabName == "{8D3FD56CC5B2A5FF}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/BLUFOR_Throwable_Satchel.et" ||
			prefabName == "{2E5BFBBBC0AD79CE}Prefabs/Weapons/Explosives/Explosive_Satchels/OPFOR_Satchel.et" ||
			prefabName == "{4E2DA37BD38AF5E6}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/OPFOR_Throwable_Satchel.et")
		{
			return BLUFOR_OPFOR_SCALE;
		}
		
		// INDFOR satchels
		if (prefabName == "{00515E7AAAFD9CB6}Prefabs/Weapons/Explosives/Explosive_Satchels/INDFOR_Satchel.et" ||
			prefabName == "{40C84C5C9B134008}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/INDFOR_Throwable_Satchel.et")
		{
			return INDFOR_SCALE;
		}
		
		// Unknown prefab
		return 0;
	}
}