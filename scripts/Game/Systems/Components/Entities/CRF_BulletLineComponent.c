class CRF_BulletLineComponentClass: ScriptComponentClass
{
}

class CRF_BulletLineComponent: ScriptComponent
{
	vector m_vStartingPos;
	vector m_aPoints[500] = {};
	int m_iCurrentPoint = 0;
	ShellMoveComponent m_ShellMoveComponent;
	vector m_vInitialPosition = "0 0 0";
	int m_iColor;
	SCR_PlayerController m_PlayerController;
	
	override void OnPostInit(IEntity owner)
	{
		#ifdef ENABLE_DIAG
		#else
		if (System.IsConsoleApp())
			return;
		#endif
		SetEventMask(owner, EntityEvent.FRAME | EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		m_PlayerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		GetPos();
		m_ShellMoveComponent = ShellMoveComponent.Cast(owner.FindComponent(ShellMoveComponent));
	}
	
	void GetPos()
	{
		m_vStartingPos = GetOwner().GetOrigin();
	}
	
	float m_fTimeBuffer = 1;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_PlayerController.m_bIsBulletTrackingEnabled)
			return;
		vector origin = owner.GetOrigin();
		if (origin == "0 0 0")
			return;
		
		//Set the intial position of the bullet
		//This is cause by the time the bullet is registered not at the debug zone it's already like 50m away from the player.
		//This sets it to the players location intially.
		if (m_vInitialPosition == "0 0 0")
		{
			if (!m_ShellMoveComponent.GetInstigator())
				return;
			
			if (!m_ShellMoveComponent.GetInstigator().GetInstigatorEntity())
				return;
			
			m_vInitialPosition = m_ShellMoveComponent.GetInstigator().GetInstigatorEntity().GetOrigin();
			m_vInitialPosition[1] = m_vInitialPosition[1] + 1.5;
			SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			m_iColor = factionMan.GetPlayerFaction(m_ShellMoveComponent.GetInstigator().GetInstigatorPlayerID()).GetFactionColor().PackToInt();
			DrawBullet(owner, m_vInitialPosition);
		}
		else
			DrawBullet(owner, origin);
		m_fTimeBuffer += timeSlice;
			
	}
	
	void DrawBullet(IEntity owner, vector origin)
	{
		if (m_fTimeBuffer >= 0.01)
		{
			m_aPoints[m_iCurrentPoint] = origin;
			m_iCurrentPoint++;
			m_fTimeBuffer = 0;
		}
		
		if (m_iCurrentPoint + 1 > 2)
			Shape.CreateLines(m_iColor, ShapeFlags.ONCE, m_aPoints, m_iCurrentPoint);
	}
	
	void ~CRF_BulletLineComponent()
	{
		#ifdef ENABLE_DIAG
		#else
		if (System.IsConsoleApp())
			return;
		#endif
		
		if (!GetGame().GetWorld())
			return;
		
		if (!m_PlayerController.m_bIsBulletTrackingEnabled)
			return;
		
		Shape line;
		if (m_iCurrentPoint + 1 > 2)
			line = Shape.CreateLines(m_iColor, ShapeFlags.VISIBLE, m_aPoints, m_iCurrentPoint);
		
		if (line)
			GetGame().GetCallqueue().CallLater(DeleteLine, 500, false, line, 500);
	}
	
	void DeleteLine(Shape line, int delay)
	{
		//We check every half second to see if the player turned off bullet tracking.
		//This is because the object this is tied to has been destroyed so we not longer have acced to EONFrame.
		if (delay >= 3000 || !SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_bIsBulletTrackingEnabled)
		{
			delete line;
			return;
		}
		
		delay += 500;
		//Small safety net to avoid disaster
		if (delay < 3000)
			GetGame().GetCallqueue().CallLater(DeleteLine, delay, false, line, delay);
	}
}