//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "GameScripted/Insurgency", description: "Manages map markers for cache entities (defenders only)", color: "255 0 0 255")]
class CRF_InsurgencyCacheMarkerComponentClass: ScriptComponentClass {}

//------------------------------------------------------------------------------------------------
/**
 * Component that manages map markers for cache entities
 * Only visible to defenders, only when cache is active
 */
class CRF_InsurgencyCacheMarkerComponent: ScriptComponent
{
	[Attribute("{E4C78A5F544BF77A}UI/Textures/Editor/EditableEntities/Characters/EditableEntity_Random.edds", UIWidgets.ResourcePickerThumbnail, "Icon to use for cache marker", params: "edds imageset")]
	protected ResourceName m_sMarkerIcon;
	
	[Attribute("Cache", UIWidgets.EditBox, "Text label for the marker")]
	protected string m_sMarkerText;
	
	[Attribute("0 0 0", UIWidgets.EditBox, "Offset from cache position for marker")]
	protected vector m_vMarkerOffset;
	
	[Attribute("0", UIWidgets.Slider, "Update interval in seconds (0 = every frame)", params: "0 60 1")]
	protected float m_fUpdateInterval;
	
	[Attribute("100", UIWidgets.Slider, "Marker Z-order", params: "0 1000 1")]
	protected int m_iZOrder;
	
	[Attribute("255 0 0 255", UIWidgets.ColorPicker, "Marker color")]
	protected ref Color m_MarkerColor;
	
	protected CRF_InsDestructiveComponent m_CacheComponent;
	protected CRF_InsurgencyGamemodeManager m_InsurgencyGamemode;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Get references
		m_CacheComponent = CRF_InsDestructiveComponent.Cast(owner.FindComponent(CRF_InsDestructiveComponent));
		
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
	 * Register this cache's marker with the player controller
	 */
	protected void RegisterMarker()
	{
		if (!m_CacheComponent || !m_InsurgencyGamemode)
			return;
		
		// Only register on clients
		if (!Replication.IsClient())
			return;
		
		CRF_PlayerControllerManager playerMgr = CRF_PlayerControllerManager.GetInstance();
		if (!playerMgr)
			return;
		
		// Use existing AddScriptedMarker method with individual parameters
		// We'll encode the marker type and phase in the entity name for filtering
		string entityNameWithMetadata = string.Format("%1_CACHE_PHASE%2", 
			GetOwner().GetName(), 
			m_CacheComponent.GetCachePhase());
		
		playerMgr.AddScriptedMarker(
			entityNameWithMetadata,                    // Entity name with type/phase metadata
			m_vMarkerOffset.ToString(false),          // Offset as string
			(int)m_fUpdateInterval,                   // Update interval
			m_sMarkerText,                            // Marker text
			m_sMarkerIcon,                            // Icon path
			m_iZOrder,                                // Z-order
			m_MarkerColor.PackToInt()                 // Color as int
		);
		
		Print(string.Format("Cache marker registered: %1 (Phase %2)", m_sMarkerText, m_CacheComponent.GetCachePhase()), LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Check if marker should be visible to current player
	 */
	bool ShouldMarkerBeVisible()
	{
		if (!m_CacheComponent || !m_InsurgencyGamemode)
			return false;
		
		// Cache must be active
		if (!m_CacheComponent.IsCacheActive())
			return false;
		
		// Cache must not be destroyed
		if (m_CacheComponent.IsCacheDestroyed())
			return false;
		
		// Check if player is on defending team
		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!playerController)
			return false;
		
		SCR_PlayerFactionAffiliationComponent factionComp = SCR_PlayerFactionAffiliationComponent.Cast(
			playerController.FindComponent(SCR_PlayerFactionAffiliationComponent));
		if (!factionComp)
			return false;
		
		Faction playerFaction = factionComp.GetAffiliatedFaction();
		if (!playerFaction)
			return false;
		
		FactionKey playerFactionKey = playerFaction.GetFactionKey();
		
		// Only defenders should see cache markers
		return (playerFactionKey == m_InsurgencyGamemode.m_DefendingSide);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetCachePhase()
	{
		if (!m_CacheComponent)
			return 0;
		
		return m_CacheComponent.GetCachePhase();
	}
}