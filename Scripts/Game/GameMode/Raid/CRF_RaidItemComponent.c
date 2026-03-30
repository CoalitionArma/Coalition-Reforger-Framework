class CRF_RaidItemComponentClass: ScriptComponentClass
{
}
 
class CRF_RaidItemComponent: ScriptComponent
{
	// ------------------------------------------------------------------ attrs
	[Attribute("10", UIWidgets.EditBox, "Supply value this object contributes to the raid pool.")]
	int m_iSupplyValue;
 
	[Attribute("Target", UIWidgets.EditBox, "Label displayed on the tactical map marker.")]
	string m_sItemName;
 
	// ----------------------------------------------------------------- state
	protected CRF_RaidGamemodeComponent	m_RaidGamemode;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected ref SCR_MapMarkerBase		m_Marker;
	protected bool						m_bPointsGiven = false;
 
	// --------------------------------------------------------------- init
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
 
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
 
		// Server-side only
		#ifndef WORKBENCH
		if (!System.IsConsoleApp())
			return;
		#endif
 
		m_RaidGamemode = CRF_RaidGamemodeComponent.GetInstance();
		if (!m_RaidGamemode)
		{
			Print("[CRF_RaidItem] ERROR: CRF_RaidGamemodeComponent not found in world.", LogLevel.ERROR);
			return;
		}
 
		// Register with the manager so it can total up available supply
		m_RaidGamemode.RegisterRaidItem(this);
 
		// Hook damage state
		m_DamageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
		{
			Print("[CRF_RaidItem] ERROR: No SCR_DamageManagerComponent on " + owner, LogLevel.ERROR);
			return;
		}
		m_DamageManager.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
 
		// Place a map marker so attackers can see the target
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerMan)
		{
			vector origin = owner.GetOrigin();
			m_Marker = new SCR_MapMarkerBase();
			m_Marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			m_Marker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.DESTROY2);
			m_Marker.SetCustomText(m_sItemName + " (" + m_iSupplyValue.ToString() + ")");
			m_Marker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.OPFOR);
			m_Marker.SetWorldPos(origin[0], origin[2]);
			markerMan.InsertStaticMarker(m_Marker, false, true);
		}
	}
 
	// ---------------------------------------------------------- event handlers
	protected void OnDamageStateChanged(EDamageState state)
	{
		if (state != EDamageState.DESTROYED)
			return;
 
		GivePoints();
	}
 
	// --------------------------------------------------------------- helpers
	protected void GivePoints()
	{
		if (m_bPointsGiven)
			return;
 
		m_bPointsGiven = true;
 
		if (m_RaidGamemode)
			m_RaidGamemode.OnItemDestroyed(this);
 
		RemoveMarker();
	}
 
	protected void RemoveMarker()
	{
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerMan && m_Marker)
			markerMan.RemoveStaticMarker(m_Marker);
	}
 
	// ---------------------------------------------------------- public api
	int GetSupplyValue()
	{
		return m_iSupplyValue;
	}
 
	// ---------------------------------------------------------------- dtor
	void ~CRF_RaidItemComponent()
	{
		if (!GetGame() || !GetGame().GetWorld())
			return;
 
		#ifndef WORKBENCH
		if (!System.IsConsoleApp())
			return;
		#endif
	}
}