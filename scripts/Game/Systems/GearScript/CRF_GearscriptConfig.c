[BaseContainerProps(configRoot: true)]
class CRF_GearScriptConfig
{
	[Attribute("")]
	ref CRF_GearScript m_GearScript;
	
	[Attribute("")]
	ref CRF_WeaponGearScript m_WeaponGearScript;
	
}

[BaseContainerProps()]
class CRF_weaponClass
{
	[Attribute(defvalue: "", category: "Gear")]
	ResourceName weapon;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> attachments;
	
}

[BaseContainerProps()]
class CRF_GearScript
{
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_headgear;

	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_shirts;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_armoredVest;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_pants;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_boots;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_backpack;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_vest;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_handwear;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_head;

	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_eyes;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_ears;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_face;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_neck;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_extra;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_extra2;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_waist;
	
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_extra3;
		
	[Attribute(defvalue: "", category: "Gear")]
	ref array<ResourceName> m_extra4;
	
}
[BaseContainerProps()]
class CRF_WeaponGearScript
{
	[Attribute(defvalue: "", category: "Weapons")]
	ref CRF_weaponClass m_rifle;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref CRF_weaponClass m_rifleUGL;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref CRF_weaponClass m_carbine;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref CRF_weaponClass m_AR;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref CRF_weaponClass m_MMG;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref array<ResourceName> m_HMG;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref array<ResourceName> m_AT;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref array<ResourceName> m_MAT;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref array<ResourceName> m_HAT;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref array<ResourceName> m_AA;
	
	[Attribute(defvalue: "", category: "Weapons")]
	ref array<ResourceName> m_pistol;
}
