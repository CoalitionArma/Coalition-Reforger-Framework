class CRF_ParachuteBackpackComponentClass : ScriptComponentClass {}

class CRF_ParachuteBackpackComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Parachute prefab to spawn", "et")]
	protected ResourceName m_ParachutePrefab;

	[RplProp(onRplName: "OnUsedChanged")]
	protected bool m_IsUsed = false;

	ResourceName GetParachutePrefab() { return m_ParachutePrefab; }
	bool IsUsed() { return m_IsUsed; }

	void SetUsed()
	{
		if (!Replication.IsServer()) return;
		if (m_IsUsed) return;
		m_IsUsed = true;
		Replication.BumpMe();
	}

	void OnUsedChanged() { /* optional UI update */ }

	override void EOnInit(IEntity owner)
	{
		if (m_ParachutePrefab == "")
			Print("ERROR: CRF_ParachuteBackpackComponent missing prefab.");
	}
}