//------------------------------------------------------------------------------------------------
//! Injects the auto-generated "Mission Technicals" entry into the mission briefing list at display
//! time only. This intentionally does NOT touch gamemode.m_aMissionDescriptors - that's an
//! [Attribute]-backed field Workbench's property panels bind directly, and mutating it from script
//! (even at runtime, since EOnInit also fires in Workbench's entity preview) desyncs that binding
//! and corrupts the editor's display of the other, hand-authored descriptors.
modded class COA_MissionDescriptionUI
{
	//------------------------------------------------------------------------------------------------
	override void ShowList()
	{
		super.ShowList();

		InsertMissionTechnicalsDescriptor();
	}

	//------------------------------------------------------------------------------------------------
	protected void InsertMissionTechnicalsDescriptor()
	{
		if (!m_cListBoxComponent || !m_Gamemode)
			return;

		COA_MissionDescriptor descriptor = new COA_MissionDescriptor();
		descriptor.m_sTitle = "Mission Technicals";
		descriptor.m_sTextData = CRF_MissionTechnicalsGenerator.BuildText(m_Gamemode);
		descriptor.m_bShowForAnyFaction = true;

		m_cListBoxComponent.AddItem(
			descriptor.m_sTitle,
			null,
			"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
		);
		m_aActiveDescriptors.Insert(descriptor);
	}
}
