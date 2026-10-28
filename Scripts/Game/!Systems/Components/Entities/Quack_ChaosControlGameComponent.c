[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class Quack_chaosControlGameComponentClass : ScriptComponentClass
{
}

class Quack_chaosControlGameComponent : ScriptComponent
{
	[Attribute(defvalue: "10", desc: "Wait in seconds before check", params: "0 59 1", category: "Post Briefing")]
	protected float m_fCheckPeriod;
	
	[Attribute(defvalue: "30", desc: "Wait in seconds", params: "0 59 1", category: "Wait time")]
	protected float m_iSeconds;
	
	[Attribute(defvalue: "1", desc: "Wait in Minutes", params: "0 45 1", category: "Wait time")]
	protected float m_iMinutes;
	
	protected float m_fCheckDelay;
	protected bool key1;
	protected bool key2;
	int RitualSitesActivations = 0;
	// Need an array of locations, perferably 8 to randomly teleport players to; as well as two known locations. 
	
	//------------------------------------------------------------------------------------------------	
	// Wait for the safe start to end before starting the countdown.
	protected void WaitGameStart(IEntity owner)
	{
		float TotalDuration = (m_iSeconds * 1000) + (m_iMinutes * 60000);
		if (isGameRunning() && isSafeStartOff())
		{
			Print("Safe start is off. Timer has been started");
			ClearEventMask(owner, EntityEvent.FRAME);
			GetGame().GetCallqueue().CallLater(SkullActivator, TotalDuration, false);
		}
		else
			Print("Timer is waiting for game to start.")
	}
	
	//------------------------------------------------------------------------------------------------	
	protected bool isSafeStartOff()
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if(!gamemode)
		{
			Print("No game mode found",LogLevel.ERROR);
			return false;
		}
		COA_SafestartManager SafeStarChecker = COA_SafestartManager.Cast(gamemode.FindComponent(COA_SafestartManager));
		if(!SafeStarChecker)
		{
			Print("No Safe start manager found",LogLevel.ERROR);
			return false;
		}
		
		bool SafeStart = SafeStarChecker.GetSafestartStatus();
		if(SafeStart)
			return false;
		
		return true;
	}

	//------------------------------------------------------------------------------------------------	
	protected bool isGameRunning()
	{
		SCR_BaseGameMode checker = SCR_BaseGameMode.Cast(GetGame().GetGameMode());;
		if (!checker)
			return false;
		
		return checker.IsRunning();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void SkullActivator()
	{
		//Here the class will will activate the skulls by finding the SCR_BaseInteractiveLightcomponent and turning them on.
		IEntity skull1 = GetGame().GetWorld().FindEntityByName("skully1");
		IEntity skull2 = GetGame().GetWorld().FindEntityByName("skully2");
		
		if(!skull1)
		{
			Print("No skull1, check name");
			return;
		}
		
		SCR_BaseInteractiveLightComponent skull1comp = SCR_BaseInteractiveLightComponent.Cast(skull1.FindComponent(SCR_BaseInteractiveLightComponent));
		if(skull1comp)
		skull1comp.ToggleLight(!skull1comp.IsOn());
		
		if(!skull2)
		{
			Print("No skull2, check name");
			return;
		}
		SCR_BaseInteractiveLightComponent skull2comp = SCR_BaseInteractiveLightComponent.Cast(skull2.FindComponent(SCR_BaseInteractiveLightComponent));
		if(skull2comp)
		skull2comp.ToggleLight(!skull2comp.IsOn());

		key1 = true;
		key2 = true;
	}
		
	//------------------------------------------------------------------------------------------------
	// Increases the number of ritual sites completed and spits out a message depending on how the game is going.
	void addnumber()
	{
		RitualSitesActivations ++;
		if (RitualSitesActivations < 2)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg(
			string.Format("The cult has completed %1 ritual out of %2", RitualSitesActivations, 8), 5);
		}
		
		else if (RitualSitesActivations <= 7)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg(
			string.Format("The cult has completed %1 rituals out of %2", RitualSitesActivations, 8), 5);
		}
		
		else if(RitualSitesActivations > 7)
		{
			SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			SCR_PopUpNotification.GetInstance().PopupMsg("The Awakening Ritual is complete!", 5, "The Elder Gods have awoken!");
			// Play ending music here.
			// gamemode.SetGameState(COA_EGamemodeState.AAR);
		}
		
		Print(RitualSitesActivations);
	}
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		m_fCheckDelay -= timeSlice;
		if (m_fCheckDelay <= 0)
		{
			m_fCheckDelay = m_fCheckPeriod;
			WaitGameStart(owner);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		float TotalDuration = (m_iSeconds * 1000) + (m_iMinutes * 60000);

		SetEventMask(owner, EntityEvent.FRAME);
	}
}

class Quack_TriggerStormUserAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// teleport player
	}
	
	//Activate triggers with flags
	
}

class Quack_RitualUserAction : ScriptedUserAction
{
	bool ActivationSwitch = 1;
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		// Add 1 token to ritual activations then check if the win conditions have been met 
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		Quack_chaosControlGameComponent GameComp = Quack_chaosControlGameComponent.Cast(gamemode.FindComponent(Quack_chaosControlGameComponent));
		GameComp.addnumber();
		// Add a glow to the skull by calling the SCR_BaseInteractiveLightComponent toggle light method. 
		SCR_BaseInteractiveLightComponent ritlight = SCR_BaseInteractiveLightComponent.Cast(pOwnerEntity.FindComponent(SCR_BaseInteractiveLightComponent));
		if(ritlight)
		ritlight.ToggleLight(!ritlight.IsOn());
		// Prevent characters from conducting the ritual again.
		ActivationSwitch = 0;
		
	}
	
	//------------------------------------------------------------------------------------------------
	// Determines if a character has the ability to conduct the ritual by checking for the wrench (ritual totem) or if the ritual has been completed on this site before.
	override bool CanBeShownScript(IEntity user) 
	{ 
		if(!CharacterHasWrench(user) || (ActivationSwitch == 0))
		{	
			return false;
		}
		return true;
	};
	
	//------------------------------------------------------------------------------------------------
	// Only characters that have the ritual "totem" may conduct the ritual.
	// This method checks the character's inventory for the wrench (which will be our stand in for the totem).
	bool CharacterHasWrench(IEntity characterEntity)
	{
	    if (!characterEntity)
	        return false;
	
	    SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(characterEntity.FindComponent(SCR_InventoryStorageManagerComponent));
	    if (!inventoryManager)
	        return false;
	
	    ResourceName targetPrefab = "{33B2DFDCD0EBA3DB}Prefabs/Items/Equipment/Kits/RepairKit_01/RepairKit_01_wrench.et";
		// Do I need the ref keyword here?
	    array<IEntity> items = new array<IEntity>();
	    inventoryManager.GetItems(items);
	
	    // Iterate through the character's inventory until we find the wrench.
		foreach (IEntity item : items)
	    {
	        if (!item)
	            continue;
	
	        EntityPrefabData prefabData = item.GetPrefabData();
	        if (prefabData)
	        {
	            ResourceName itemPrefabName = prefabData.GetPrefabName();
	            if (itemPrefabName == targetPrefab)
	            {
					return true;
	            }
	        }
	    }
	
	    return false;
	}
}
