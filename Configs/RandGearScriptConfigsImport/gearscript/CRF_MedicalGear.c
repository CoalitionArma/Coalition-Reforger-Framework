[BaseContainerProps(configRoot: true), BaseContainerCustomTitleField("RoleSpecificGear")]
class CRF_MedicalGear
{
	[Attribute(category: "Equipment")]
	ResourceName m_BandagePrefab;
	[Attribute(category: "Equipment")]
	ResourceName m_TourniquetPrefab;
	[Attribute(category: "Equipment")]
	ResourceName m_MorphinePrefab;
	[Attribute(category: "Equipment")]
	ResourceName m_EpiPrefab;
	[Attribute(category: "Equipment")]
	ResourceName m_SalinePrefab;
	[Attribute(category: "Equipment")]
	ResourceName m_MedicalKitPrefab;
	
	[Attribute(defvalue: "2", params: "0 30", category: "Settings")]
	int m_GruntBandageCount;
	[Attribute(defvalue: "12", params: "0 30", category: "Settings")]
	int m_MedicBandageCount;
	
	[Attribute(defvalue: "1", params: "0 30", category: "Settings")]
	int m_GruntTourniqueCount;
	[Attribute(defvalue: "4", params: "0 30", category: "Settings")]
	int m_MedicTourniqueCount;
	
	[Attribute(defvalue: "0", params: "0 30", category: "Settings")]
	int m_GruntMorphineCount;
	[Attribute(defvalue: "6", params: "0 30", category: "Settings")]
	int m_MedicMorphineCount;
	
	[Attribute(defvalue: "0", params: "0 30", category: "Settings")]
	int m_GruntEpiCount;
	[Attribute(defvalue: "6", params: "0 30", category: "Settings")]
	int m_MedicEpiCount;
	
	[Attribute(defvalue: "0", params: "0 30", category: "Settings")]
	int m_GruntSalineCount;
	[Attribute(defvalue: "5", params: "0 30", category: "Settings")]
	int m_MedicSalineCount;
}
