class CRF_RaidItemComponentClass: ScriptComponentClass
{
}

class CRF_RaidItemComponent: ScriptComponent
{
	[Attribute("10", UIWidgets.EditBox, "Supply value this object contributes to the raid pool.")]
	int m_iSupplyValue;

	[Attribute("Target", UIWidgets.EditBox, "Label displayed on the tactical map marker.")]
	string m_sItemName;

	protected CRF_RaidGamemodeComponent m_RaidGamemode;
	protected SCR_DamageManagerComponent m_DamageManager;
	protected ref SCR_MapMarkerBase m_Marker;
	protected bool m_bPointsGiven = false;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

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

		m_RaidGamemode.RegisterRaidItem(this);

		m_DamageManager = SCR_DamageManagerComponent.Cast(owner.FindComponent(SCR_DamageManagerComponent));
		if (!m_DamageManager)
		{
			Print("[CRF_RaidItem] ERROR: No SCR_DamageManagerComponent on " + owner, LogLevel.ERROR);
			return;
		}

		m_DamageManager.GetOnDamageStateChanged().Insert(OnDamageStateChanged);

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager && CRF_RaidGamemodeComponent.GetEnableMapMarkers())
		{
			vector origin = owner.GetOrigin();
			m_Marker = new SCR_MapMarkerBase();
			m_Marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
			m_Marker.SetIconEntry(SCR_EScenarioFrameworkMarkerCustom.DESTROY2);
			m_Marker.SetCustomText(m_sItemName + " (" + m_iSupplyValue.ToString() + ")");
			m_Marker.SetColorEntry(SCR_EScenarioFrameworkMarkerCustomColor.OPFOR);
			m_Marker.SetWorldPos(origin[0], origin[2]);
			markerManager.InsertStaticMarker(m_Marker, false, true);
		}
	}

	protected void OnDamageStateChanged(EDamageState state)
	{
		if (state == EDamageState.DESTROYED)
			GivePoints();
	}

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
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager && m_Marker)
			markerManager.RemoveStaticMarker(m_Marker);

		m_Marker = null;
	}

	int GetSupplyValue()
	{
		return m_iSupplyValue;
	}

	override void OnDelete(IEntity owner)
	{
		if (m_DamageManager)
			m_DamageManager.GetOnDamageStateChanged().Remove(OnDamageStateChanged);

		// OnDelete runs while the entity lifecycle is valid. Avoid gameplay and
		// manager calls from the destructor, where teardown order is undefined.
		if (!m_bPointsGiven && m_RaidGamemode && GetGame() && GetGame().GetWorld())
			GivePoints();
		else
			RemoveMarker();

		if (m_RaidGamemode)
			m_RaidGamemode.UnregisterRaidItem(this);

		m_DamageManager = null;
		m_RaidGamemode = null;
		super.OnDelete(owner);
	}

	void ~CRF_RaidItemComponent()
	{
		m_DamageManager = null;
		m_RaidGamemode = null;
		m_Marker = null;
	}
}
