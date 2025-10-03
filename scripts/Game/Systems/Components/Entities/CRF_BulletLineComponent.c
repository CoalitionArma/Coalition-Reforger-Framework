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
		#ifdef WORKBENCH
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
		if (!m_PlayerController.m_bIsBulletTrackingEnabled || !CRF_GamemodeManager.IsSpectator(m_PlayerController.GetControlledEntity()))
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
			
			if (owner.GetPrefabData().GetPrefabName().Contains("Spall"))
			{
				m_vInitialPosition = origin;
			}
			else
			{
				m_vInitialPosition = m_ShellMoveComponent.GetInstigator().GetInstigatorEntity().GetOrigin();
				m_vInitialPosition[1] = m_vInitialPosition[1] + 1.5;
			}
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
		#ifdef WORKBENCH
		#else
		if (System.IsConsoleApp())
			return;
		#endif
		
		if (!GetGame().GetWorld())
			return;
		
		if (!GetGame().GetPlayerController())
			return;
		
		if (!SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_bIsBulletTrackingEnabled || !CRF_GamemodeManager.IsSpectator(SCR_PlayerController.GetLocalControlledEntity()))
			return;
		
		Shape line;
		if (m_iCurrentPoint + 1 > 2)
		{
			line = Shape.CreateLines(m_iColor, ShapeFlags.VISIBLE, m_aPoints, m_iCurrentPoint);
			ref CRF_BulletTracerContainer container = new CRF_BulletTracerContainer();
			container.m_Line = line;
			container.m_fTimeAlive = 3;
			
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_aActiveTraces.Insert(container);
		}
	}
}