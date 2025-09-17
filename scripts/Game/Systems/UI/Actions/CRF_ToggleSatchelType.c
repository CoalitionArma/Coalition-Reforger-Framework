//------------------------------------------------------------------------------------------------
class CRF_ToggleSatchelType : ScriptedUserAction
{	
	static const ResourceName SATCHEL_BLUFOR = "{55641F140F92FDCD}Prefabs/Weapons/Explosives/Explosive_Satchels/BLUFOR_Satchel.et";
	static const ResourceName SATCHEL_OPFOR = "{2E5BFBBBC0AD79CE}Prefabs/Weapons/Explosives/Explosive_Satchels/OPFOR_Satchel.et";
	static const ResourceName SATCHEL_INDFOR = "{00515E7AAAFD9CB6}Prefabs/Weapons/Explosives/Explosive_Satchels/INDFOR_Satchel.et";
	static const ResourceName TROWABLE_SATCHEL_BLUFOR = "{8D3FD56CC5B2A5FF}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/BLUFOR_Throwable_Satchel.et";
	static const ResourceName TROWABLE_SATCHEL_OPFOR = "{4E2DA37BD38AF5E6}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/OPFOR_Throwable_Satchel.et";
	static const ResourceName TROWABLE_SATCHEL_INDFOR = "{40C84C5C9B134008}Prefabs/Weapons/Explosives/Explosive_Satchels/Throwable/INDFOR_Throwable_Satchel.et";
	
	protected IEntity m_iSatchelEntity;
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Called when object is initialized and registered to actions manager
	 */
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		m_iSatchelEntity = pOwnerEntity;
	};
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Execute the satchel toggle action when the player activates it
	 * @param pOwnerEntity The entity that owns this action (the object being interacted with)
	 * @param pUserEntity The player entity performing the action
	 */
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		super.PerformAction(pOwnerEntity, pUserEntity);
		
		if (!m_iSatchelEntity)
			return;
		
		ResourceName satchelResource = m_iSatchelEntity.GetPrefabData().GetPrefabName();
		RplComponent satchelRplComp = RplComponent.Cast(m_iSatchelEntity.FindComponent(RplComponent));
		
		if (!satchelRplComp || satchelResource.IsEmpty())
			return;
		
		switch (satchelResource)
		{
			case SATCHEL_BLUFOR : {satchelResource = TROWABLE_SATCHEL_BLUFOR; break;}
			case SATCHEL_OPFOR : {satchelResource = TROWABLE_SATCHEL_OPFOR; break;}
			case SATCHEL_INDFOR : {satchelResource = TROWABLE_SATCHEL_INDFOR; break;}
			case TROWABLE_SATCHEL_BLUFOR : {satchelResource = SATCHEL_BLUFOR; break;}
			case TROWABLE_SATCHEL_OPFOR : {satchelResource = SATCHEL_OPFOR; break;}
			case TROWABLE_SATCHEL_INDFOR : {satchelResource = SATCHEL_INDFOR; break;}
		};
		
		CRF_RplToAuthorityManager.GetInstance().AddItem(SCR_PlayerController.GetLocalPlayerId(), satchelResource, false);
		CRF_RplToAuthorityManager.GetInstance().RemoveItem(SCR_PlayerController.GetLocalPlayerId(), satchelRplComp.Id(), false);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * If overridden and true is returned, outName is returned when BaseUserAction.GetActionName is called.
	 * If not overridden or false is returned the default value from UIInfo is taken (or empty string if no UI info exists)
	 */
	override bool GetActionNameScript(out string outName) 
	{ 
		if (!m_iSatchelEntity)
			return false;
		
		ResourceName satchelResource = m_iSatchelEntity.GetPrefabData().GetPrefabName();
		
		if (satchelResource == TROWABLE_SATCHEL_BLUFOR || satchelResource == TROWABLE_SATCHEL_OPFOR || satchelResource == TROWABLE_SATCHEL_INDFOR)
		{
			outName = "Convert Satchel To Non-Throwable";
			return true; 
		} else if (satchelResource == SATCHEL_BLUFOR || satchelResource == SATCHEL_OPFOR || satchelResource == SATCHEL_INDFOR)
		{
			outName = "Convert Satchel To Throwable";
			return true; 
		}
		
		return false; 
	};
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Indicates that this action only affects the local player and doesn't need to be synchronized
	 */
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	/**
	 * Indicates whether this action should be broadcast to other clients
	 */
	override bool CanBroadcastScript()
	{
		return false;
	}
};
