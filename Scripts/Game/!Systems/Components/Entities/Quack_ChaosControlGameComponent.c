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
		 
//		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
//		gamemode.SetGameState(COA_EGamemodeState.AAR);
	}
		
	//------------------------------------------------------------------------------------------------
	void addnumber()
	{
		RitualSitesActivations ++;
		if(RitualSitesActivations < 7)
		{
			SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
			gamemode.SetGameState(COA_EGamemodeState.AAR);
		}
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
	
	void Movetriggers()
	{
	}
	
}

class Quack_RitualUserAction : ScriptedUserAction
{
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		//Add 1 token to ritual activations then check if the win conditions have been met 
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		Quack_chaosControlGameComponent GameComp = Quack_chaosControlGameComponent.Cast(gamemode.FindComponent(Quack_chaosControlGameComponent));
		GameComp.addnumber();
	}
}
