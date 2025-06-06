[BaseContainerProps(configRoot: true)]
class CRF_MarkerMakerConfig
{
	[Attribute(defvalue: "{CD85ADE9E0F54679}PrefabsEditable/Markers/EditableMarker.et", params: "et")]
	ResourceName m_sManualMarkerPrefab;
	
	[Attribute("")]
	ref CRF_ManualMarkerConfig m_mManualMarkerConfig;
	
	[Attribute("")]
	ref array<ref CRF_FactionManualMarkerConfig> m_mFactionsManualMarkerConfig;
	
	[Attribute("")]
	ref array<ref CRF_VehicleManualMarkerConfig> m_mVehiclesManualMarkerConfig;
}

[BaseContainerProps()]
class CRF_FactionManualMarkerConfig
{
	[Attribute("")]
	FactionKey m_sFactionKey;
	
	[Attribute("")]
	ref CRF_ManualMarkerConfig m_mManualMarkerConfig;
}

[BaseContainerProps()]
class CRF_VehicleManualMarkerConfig
{
	[Attribute("")]
	EVehicleType m_iVehicleType;
	
	[Attribute("")]
	ref CRF_ManualMarkerConfig m_mManualMarkerConfig;
}

[BaseContainerProps()]
class CRF_ManualMarkerConfig
{
	[Attribute("{D17288006833490F}UI/Textures/Icons/icons_wrapperUI-32.imageset")]
	ResourceName m_sImageSet;
	[Attribute("")]
	ResourceName m_sImageSetGlow;
	[Attribute("1 1 1 1", UIWidgets.ColorPicker)]
	ref Color m_MarkerColor;
	[Attribute("empty")]
	string m_sQuadName;
	[Attribute("5.0")]
	float m_fWorldSize;
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline)]
	string m_sDescription;
	[Attribute("true")]
	bool m_bUseWorldScale;
	[Attribute("")]
	ref array<FactionKey> m_aVisibleForFactions;
	[Attribute("")]
	bool m_bVisibleForEmptyFaction;
	[Attribute("0", UIWidgets.ComboBox, "", "", ParamEnumArray.FromEnum(SCR_EGameModeState))]
	ref array<SCR_EGameModeState> m_aHideOnGameModeStates;
	[Attribute("")]
	int m_iZOrder;
	[Attribute("")]
	float m_fMinSize;
}