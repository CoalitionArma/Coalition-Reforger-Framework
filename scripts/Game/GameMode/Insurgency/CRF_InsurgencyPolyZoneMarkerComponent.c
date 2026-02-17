//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Insurgency", description: "Manages map markers for polyzone areas (attackers only)", color: "0 128 255 255")]
class CRF_InsurgencyPolyZoneMarkerComponentClass: ScriptComponentClass {}

//------------------------------------------------------------------------------------------------
/**
 * Component that manages map markers for polyzone search areas
 * Only visible to attackers, only when phase is active and revealed
 */
class CRF_InsurgencyPolyZoneMarkerComponent: ScriptComponent
{
	[Attribute("{94280F76021DA29A}UI/Textures/Editor/EditableEntities/Triggers/EditableEntity_Trigger.edds", UIWidgets.ResourcePickerThumbnail, "Icon to use for zone marker", params: "edds imageset")]
	protected ResourceName m_sMarkerIcon;
	
	[Attribute("Search Area", UIWidgets.EditBox, "Text label for the marker")]
	protected string m_sMarkerText;
	
	[Attribute("", UIWidgets.EditBox, "Description shown when zone is revealed (optional)")]
	protected string m_sMarkerDescription;
	
	[Attribute("0 0 0", UIWidgets.EditBox, "Offset from zone center for marker")]
	protected vector m_vMarkerOffset;
	
	[Attribute("0", UIWidgets.Slider, "Update interval in seconds (0 = static)", params: "0 60 1")]
	protected float m_fUpdateInterval;
	
	[Attribute("50", UIWidgets.Slider, "Marker Z-order", params: "0 1000 1")]
	protected int m_iZOrder;
	
	[Attribute("0 128 255 255", UIWidgets.ColorPicker, "Marker color")]
	protected ref Color m_MarkerColor;
	
	protected CRF_InsurgencyPolyZone m_PolyZoneComponent;
	protected CRF_InsurgencyGamemodeManager m_InsurgencyGamemode;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Get references
		m_PolyZoneComponent = CRF_InsurgencyPolyZone.Cast(owner.FindComponent(CRF_InsurgencyPolyZone));
		
		if (Replication.IsClient() || Replication.IsServer())
		{
			m_InsurgencyGamemode = CRF_InsurgencyGamemodeManager.GetInstance();
		}
		
		// Register marker when game starts
		if (GetGame().InPlayMode())
		{
			GetGame().GetCallqueue().CallLater(RegisterMarker, 1000, false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Register this polyzone's marker with the player controller
	 */
	protected void RegisterMarker()
	{
		if (!m_PolyZoneComponent || !m_InsurgencyGamemode)
			return;
		
		// Only register on clients
		if (!Replication.IsClient())
			return;
		
		CRF_PlayerControllerManager playerMgr = CRF_PlayerControllerManager.GetInstance();
		if (!playerMgr)
			return;
		
		// Build marker text with description if provided
		string displayText = m_sMarkerText;
		if (m_sMarkerDescription != "")
		{
			displayText = string.Format("%1\n%2", m_sMarkerText, m_sMarkerDescription);
		}
		
		// Use existing AddScriptedMarker method with individual parameters
		// We'll encode the marker type and phase in the entity name for filtering
		string entityNameWithMetadata = string.Format("%1_POLYZONE_PHASE%2", 
			GetOwner().GetName(), 
			m_PolyZoneComponent.GetVisiblePhase());
		
		playerMgr.AddScriptedMarker(
			entityNameWithMetadata,                    // Entity name with type/phase metadata
			m_vMarkerOffset.ToString(false),          // Offset as string
			(int)m_fUpdateInterval,                   // Update interval
			displayText,                              // Marker text (with description)
			m_sMarkerIcon,                            // Icon path
			m_iZOrder,                                // Z-order
			m_MarkerColor.PackToInt()                 // Color as int
		);
		
		Print(string.Format("PolyZone marker registered: %1 (Phase %2)", m_sMarkerText, m_PolyZoneComponent.GetVisiblePhase()), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Get center position of the polyzone for marker placement
	 */
	protected vector GetZoneCenterPosition()
	{
		// Get zone bounds and calculate center
		IEntity owner = GetOwner();
		if (!owner)
			return vector.Zero;
		
		// For now, just use entity origin
		// In a more advanced implementation, you could calculate the actual polygon center
		return owner.GetOrigin();
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if marker should be visible to current player
	 */
	bool ShouldMarkerBeVisible()
	{
		if (!m_PolyZoneComponent || !m_InsurgencyGamemode)
			return false;
		
		// Use the same visibility logic as the polyzone itself
		// This ensures marker visibility matches zone visibility
		return m_PolyZoneComponent.IsCurrentVisibility();
	}
	
	//------------------------------------------------------------------------------------------------
	int GetZonePhase()
	{
		if (!m_PolyZoneComponent)
			return 0;
		
		return m_PolyZoneComponent.GetVisiblePhase();
	}
}