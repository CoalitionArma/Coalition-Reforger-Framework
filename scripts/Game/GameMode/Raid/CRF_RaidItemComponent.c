class CRF_RaidItemComponentClass: ScriptComponentClass
{
}

class CRF_RaidItemComponent: ScriptComponent
{
	[Attribute("10")] int m_iPointsEarnedWhenDestroyed;
	[Attribute("Ammo Pallet", desc: "What the marker will say on the map")] string m_sItemName;
	CRF_RaidGamemodeComponent m_RaidGamemode;
	SCR_DamageManagerComponent m_DestructionComp;
	ref SCR_MapMarkerBase m_Marker;
	bool m_bHasPointsBeenGiven = false;
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		m_RaidGamemode = CRF_RaidGamemodeComponent.GetInstance();
		m_DestructionComp = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!m_DestructionComp)
			Print("[CRF RAID ERROR] NO DESTRUCTION COMPONENT ON " + owner);
		m_DestructionComp.GetOnDamageStateChanged().Insert(OnDamageStateChanged);
		SCR_MapMarkerManagerComponent markerMan = SCR_MapMarkerManagerComponent.GetInstance();
		vector origin = GetOwner().GetOrigin();
		m_Marker = new SCR_MapMarkerBase();
		m_Marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		m_Marker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.DESTROY2);
		m_Marker.SetCustomText(m_sItemName + ": (" + m_iPointsEarnedWhenDestroyed.ToString() + ")");
		m_Marker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.OPFOR);
		m_Marker.SetWorldPos(origin[0], origin[2]);
		markerMan.InsertStaticMarker(m_Marker, false, true);
	}
	
	void OnDamageStateChanged(EDamageState state)
	{
		if (state == EDamageState.DESTROYED)
		{
			m_RaidGamemode.OnObjectDestroyed(m_iPointsEarnedWhenDestroyed);
			m_bHasPointsBeenGiven = true;
			SCR_MapMarkerManagerComponent.GetInstance().RemoveStaticMarker(m_Marker);
		}
	}
	
	void ~CRF_RaidItemComponent()
	{
		if (!GetGame().GetWorld())
			return;
		
		#ifdef WORKBENCH
		#else
		if (!System.IsConsoleApp())
			return;
		#endif
		if (m_bHasPointsBeenGiven)
			return;
		m_RaidGamemode.OnObjectDestroyed(m_iPointsEarnedWhenDestroyed);
		m_bHasPointsBeenGiven = true;
		SCR_MapMarkerManagerComponent.GetInstance().RemoveStaticMarker(m_Marker);
	}
}