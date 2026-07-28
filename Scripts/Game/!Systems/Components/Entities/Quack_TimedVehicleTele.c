[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class Quack_TimedVehicleTeleComponentClass : ScriptComponentClass
{
}

class Quack_TimedVehicleTeleComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Coords, "Coodinates to where the user will teleport to", "", )]
	protected vector m_vCoords;
	
	[Attribute(defvalue: "10", desc: "Wait in seconds before check", params: "0 59 1", category: "Post Briefing")]
	protected float m_fCheckPeriod;
	
	[Attribute(defvalue: "30", desc: "Wait in seconds", params: "0 59 1", category: "Wait time")]
	protected float m_iSeconds;
	
	[Attribute(defvalue: "0", desc: "Wait in Minutes", params: "0 15 1", category: "Wait time")]
	protected float m_iMinutes;
	
	protected float m_fCheckDelay;
	
	//------------------------------------------------------------------------------------------------	
	protected void WaitGameStart(IEntity owner)
	{
		float TotalDuration = (m_iSeconds * 1000) + (m_iMinutes * 60000);
		if (isGameRunning() && isSafeStartOff())
		{
			Print("Safe start is off. The Vehicle will be teleported");
			ClearEventMask(owner, EntityEvent.FRAME);
			GetGame().GetCallqueue().CallLater(TeleVehicle, TotalDuration, false, owner);
		}
		else
			Print("The vehicle is waiting for the game to start")	
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
	protected void TeleVehicle(IEntity owner)
	{
			IEntity Vic = IEntity.Cast(owner);
			Vic.SetOrigin(m_vCoords);
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
	override void EOnInit(IEntity owner)
	{
		// remove if unused
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		Print("EOnInit");
	    Print(owner);
		Print("Checking for vehicle");	
		
		float TotalDuration = (m_iSeconds * 1000) + (m_iMinutes * 60000);

		SetEventMask(owner, EntityEvent.FRAME);


	}

	//------------------------------------------------------------------------------------------------
}
