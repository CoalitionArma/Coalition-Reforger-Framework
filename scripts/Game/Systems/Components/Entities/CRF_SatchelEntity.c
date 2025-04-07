class CRF_SatchelEntityClass : GenericEntityClass {}

class CRF_SatchelEntity : GenericEntity
{
	void CRF_SatchelEntity(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.VISIBLE);
	}
	
	//------------------------------------------------------------------------------------------------
	override event protected void EOnVisible(IEntity owner, int frameNumber)
	{	
		super.EOnVisible(owner, frameNumber);
		
		switch(GetPrefabData().GetPrefabName())
		{
			case "{55641F140F92FDCD}Prefabs/Weapons/Explosives/Explosive_Satchels/BLUFOR_Satchel.et" : {SetScale(2.250); break;}
			case "{8D3FD56CC5B2A5FF}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/BLUFOR_Throwable_Satchel.et" : {SetScale(2.250); break;}
			case "{2E5BFBBBC0AD79CE}Prefabs/Weapons/Explosives/Explosive_Satchels/OPFOR_Satchel.et" : {SetScale(2.250); break;}
			case "{4E2DA37BD38AF5E6}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/OPFOR_Throwable_Satchel.et" : {SetScale(2.250); break;}
			case "{00515E7AAAFD9CB6}Prefabs/Weapons/Explosives/Explosive_Satchels/INDFOR_Satchel.et" : {SetScale(3.050); break;}
			case "{40C84C5C9B134008}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/INDFOR_Throwable_Satchel.et" : {SetScale(3.050); break;}
		};
	}
}