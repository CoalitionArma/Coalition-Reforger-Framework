//------------------------------------------------------------------------------------------------
[BaseContainerProps(configRoot: true), SCR_BaseContainerCustomTitleFields({"m_sCallsign"}, "%1")]
class CRF_SlottingGroup
{		
	protected IEntitySource m_eOwnerSrc;
	protected WorldEditorAPI m_WorldAPI;
	
	[Attribute()]
	LocalizedString m_sCallsign;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EFlagType))]
	CRF_EFlagType m_FlagType;
	
	[Attribute()]
	ref CRF_SlottingSpawnPoint m_sSpawnpoint;
	
	[Attribute(uiwidget: UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EGearRole))]
	ref array<ref CRF_EGearRole> m_aSlots;
	
	protected void SetSlotsArray()
	{
		array<ref ContainerIdPathEntry> path;
		Print(m_eOwnerSrc.GetComponentCount());
		Print(m_eOwnerSrc);
		Print(m_WorldAPI);
		
		if (!m_eOwnerSrc)
			return;
		
		if (!m_WorldAPI.CreateObjectVariableMember(m_eOwnerSrc, path, "m_iTimeToRespawn", "CRF_Gamemode"))
			return;

		path.Insert(new ContainerIdPathEntry("m_iTimeToRespawn"));

		if (!m_WorldAPI.SetVariableValue(m_eOwnerSrc, path, "m_iTimeToRespawn", "999"))
			return;
				
		//m_WorldAPI.SetVariableValue(m_eOwnerSrc, null, "m_iTimeToRespawn", "69");
		/*
		int count = ownerSrc.GetComponentCount();
		for (int i = 0; i < count; i++)
		{
			IEntityComponentSource componentContainer = ownerSrc.GetComponent(i);
			if (componentContainer.GetClassName() == "MeshObject")
			{
				string remapOptions;
				componentContainer.Get("Materials", remapOptions);
				array<string> parts = {};
				remapOptions.Split(";", parts, true);
				foreach (int idx, string part : parts)
				{
					array<string> partParts = {};
					part.Split(",", partParts, true);

					array<ref ContainerIdPathEntry> path = {new ContainerIdPathEntry("RHS_Armament_TurretDestructionComponent")};
					api.CreateObjectArrayVariableMember(ownerSrc, path, "m_aMaterialsRemap", "RHS_Armament_MaterialRemapContainer", idx);
					path = {new ContainerIdPathEntry("RHS_Armament_TurretDestructionComponent"), new ContainerIdPathEntry("m_aMaterialsRemap", idx)};
					api.SetVariableValue(ownerSrc, path, "m_sMaterialName", partParts[0]);
					api.SetVariableValue(ownerSrc, path, "m_sMaterialPath", partParts[1]);
				}
				api.UpdateSelectionGui();
				return;
			}
		}
		*/
		
	}
	
	void CRF_SlottingGroup()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		m_WorldAPI = worldEditor.GetApi();
		m_eOwnerSrc = m_WorldAPI.GetSelectedEntity(0);
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps()]
class CRF_SlottingSpawnPoint
{		
	[Attribute("0", uiwidget: UIWidgets.ComboBox, enums: {ParamEnum("DEFAULT", "0"), ParamEnum("CUSTOM", "1")})]
	int m_bStartingSpawnPoint;
	
	[Attribute("")]
	ref PointInfo m_CustomPosition;
}

[BaseContainerProps()]
class CRF_SlottingGroup_AirCrew : CRF_SlottingGroup
{
	void CRF_SlottingGroup_AirCrew(IEntitySource src, IEntity parent)
	{
		SetSlotsArray();
	}
}