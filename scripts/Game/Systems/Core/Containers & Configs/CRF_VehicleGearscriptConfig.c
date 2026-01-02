[BaseContainerProps(configRoot:true)]
class CRF_VehicleGearscriptConfig
{
	[Attribute("1200")] int m_iAmountOfBulletsRifles;
	[Attribute("600")] int m_iAmountOfBulletsRifleUGLs;
	[Attribute("600")] int m_iAmountOfBulletsCarbines;
	[Attribute("200")] int m_iAmountOfBulletsPistols;
	[Attribute("2400")] int m_iAmountOfBulletsAR;
	[Attribute("3000")] int m_iAmountOfBulletsMMG;
	[Attribute("600")] int m_iAmountOfBulletsHMG;
	
	[Attribute("4")] int m_iAmountOfDisposables;
	[Attribute("8")] int m_iAmountOfRocketsAT;
	[Attribute("8")] int m_iAmountOfRocketsMAT;
	[Attribute("2")] int m_iAmountOfRocketsAA;
	
	[Attribute("120")] int m_iAmountOfBulletsSniper;
	
	[Attribute("8")] int m_iAmountOfGrenades;
	[Attribute("16")] int m_iAmountOfSmokeGrenades;
	
	[Attribute("20")] int m_iAmountOfHEGLs;
	[Attribute("40")] int m_iAmountOfSmokeGLs;
}

[BaseContainerProps()]
class CRF_VehicleGearscriptOverride
{
	[Attribute("0", UIWidgets.SearchComboBox, enums: ParamEnumArray.FromEnum(CRF_EVehicleGearScriptType))]
	CRF_EVehicleGearScriptType m_VehicleAmmoType;
	
	[Attribute()] 
	int m_iAmountOfBullets;
}

[BaseContainerProps()]
class CRF_VehicleGearScriptAdditionalItem
{
	[Attribute(uiwidget: "resourcePickerThumbnail", params: "et")] 
	ResourceName m_Prefab;
	
	[Attribute("1")] 
	int m_iAmountOfItemSupplyTruck;
	
	[Attribute("1")] 
	int m_iAmountOfItemRegularVehicle;
}

[BaseContainerProps()]
class CRF_VehicleGearScriptLoadout
{
	[Attribute("300")] int m_iAmountofAutoCannonAmmo;
	[Attribute("1200")] int m_iAmountofMachineGunAmmo;
}