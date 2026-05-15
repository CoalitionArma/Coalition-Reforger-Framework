[ComponentEditorProps(category: "GameScripted/Misc", description: "")]
class Quack_SpawnerComponentClass : ScriptComponentClass
{
}

/****************************************************************************************
* This component will call the CRF RespawnAllSides() method after a determined amount of time. The timer can be set in the world editor.  
* The component should be added to the GameMode entity.
/****************************************************************************************/

class Quack_SpawnerComponent : ScriptComponent
{
	[Attribute(defvalue: "10", desc: "Wait in seconds before check", params: "0 59 1", category: "Post Briefing")]
	protected float m_fCheckPeriod;
	
	[Attribute(defvalue: "30", desc: "Wait in seconds", params: "0 59 1", category: "Wait time")]
	protected float m_iSeconds;
	
	[Attribute(defvalue: "22", desc: "Wait in Minutes", params: "0 45 1", category: "Wait time")]
	protected float m_iMinutes;
	
	protected float m_fCheckDelay;
	
	//------------------------------------------------------------------------------------------------	
	protected void WaitGameStart(IEntity owner)
	{
		float TotalDuration = (m_iSeconds * 1000) + (m_iMinutes * 60000);
		if (isGameRunning() && isSafeStartOff())
		{
			Print("Safe start is off. Timer has been started");
			ClearEventMask(owner, EntityEvent.FRAME);
			GetGame().GetCallqueue().CallLater(AllRepawner, TotalDuration, false);
		}
		else
			Print("Timer is waiting for game to start.")
	}
	
	//------------------------------------------------------------------------------------------------	
	protected bool isSafeStartOff()
	{
		SCR_BaseGameMode CRF_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if(!CRF_GameMode)
		{
			Print("No game mode found",LogLevel.ERROR);
			return false;
		}
		CRF_SafestartManager SafeStarChecker = CRF_SafestartManager.Cast(CRF_GameMode.FindComponent(CRF_SafestartManager));
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
	protected void AllRepawner()
	{
		SCR_BaseGameMode CRF_GameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if(!CRF_GameMode)
		{
			Print("No game mode found",LogLevel.ERROR);
		}
		
		CRF_RespawnManager RspMng = CRF_RespawnManager.Cast(CRF_GameMode.FindComponent(CRF_RespawnManager));
		RspMng.RespawnAllSides()
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
		Print("EOnInit");
	    Print(owner);
		Print("Checking for vehicle");	
		
		float TotalDuration = (m_iSeconds * 1000) + (m_iMinutes * 60000);

		SetEventMask(owner, EntityEvent.FRAME);


	}
}
