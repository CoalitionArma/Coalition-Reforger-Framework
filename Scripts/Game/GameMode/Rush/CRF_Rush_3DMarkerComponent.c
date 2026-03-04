//------------------------------------------------------------------------------------
// CRF_Rush_3DMarkerComponent: 3D HUD marker component for MCOM sites in Rush gamemode
// Creates floating letters (A/B) above MCOM entities that are visible through walls
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "Game Mode Component", description: "3D HUD marker for MCOM sites showing letter indicators")]
class CRF_Rush_3DMarkerComponentClass: ScriptComponentClass
{
	
}

class CRF_Rush_3DMarkerComponent: ScriptComponent
{
	//===================================================================================
	// ATTRIBUTES AND PROPERTIES
	//===================================================================================
	
	[Attribute("A", desc: "Letter to display for this MCOM (A or B)")]
	string m_sMarkerLetter;
	
	[Attribute("1 0 0 1", UIWidgets.ColorPicker, desc: "Color of the 3D marker")]
	ref Color m_MarkerColor;
	
	[Attribute("128.0", desc: "Maximum size of the marker icon")]
	float m_fMaxIconSize;
	
	[Attribute("32.0", desc: "Minimum size of the marker icon")]
	float m_fMinIconSize;
	
	[Attribute("2000.0", desc: "Maximum distance at which marker is visible")]
	float m_fMaxDistance;
	
	[Attribute("5.0", desc: "Minimum distance for maximum size")]
	float m_fMinDistance;
	
	//===================================================================================
	// RUNTIME VARIABLES
	//===================================================================================
	
	// UI References
	protected Widget m_wMarkerRoot;
	protected TextWidget m_wMarkerText;
	protected PanelWidget m_wMarkerBackground;
	protected bool m_bIsInitialized = false;
	
	// State tracking
	protected vector m_vWorldPosition;
	protected bool m_bIsVisible = true;
	protected IEntity m_Owner;
	
	// Flashing functionality
	protected bool m_bIsFlashing = false;
	protected float m_fFlashTimer = 0.0;
	protected float m_fFlashInterval = 0.4; // Flash every 0.4 seconds for faster, more urgent effect
	protected bool m_bFlashState = true; // Current flash state (visible/hidden)
	protected ref Color m_FlashColor;
	
	// Server state buffering (for when server sends data before UI is ready)
	protected bool m_bServerDataPending = false;
	protected string m_sServerLetter;
	protected ref Color m_ServerColor;
	protected bool m_bServerVisible;
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	/**
	 * Initialize the component when entity is ready
	 */
	override void OnPostInit(IEntity owner)
	{
		m_Owner = owner;
		SetEventMask(owner, EntityEvent.FRAME);
		
		// Always auto-initialize on server, but wait for server command on clients
		if (Replication.IsServer())
		{
			// Server: Delayed initialization to ensure UI is ready
			GetGame().GetCallqueue().CallLater(InitializeMarkerUI, 1000, false);
		}
		else
		{
			// Client: Wait longer to ensure server has time to send initialization
			// Also auto-initialize as fallback in case server command fails
			GetGame().GetCallqueue().CallLater(InitializeMarkerUI, 3000, false);
			
			// Additional retry mechanism for clients
			GetGame().GetCallqueue().CallLater(CheckInitializationStatus, 5000, false);
		}
	}
	
	/**
	 * Initialize the 3D marker UI
	 */
	protected void InitializeMarkerUI()
	{
		// Find or create the main HUD root
		Widget hudRoot = GetGame().GetWorkspace().FindWidget("HudRoot");
		if (!hudRoot)
			hudRoot = GetGame().GetWorkspace();
		
		if (!hudRoot)
			return;
		
		// Create marker layout from inline definition
		m_wMarkerRoot = CreateMarkerWidget(hudRoot);
		if (!m_wMarkerRoot)
			return;
		
		// Configure the marker
		if (m_wMarkerText) {
			m_wMarkerText.SetText(m_sMarkerLetter);
			m_wMarkerText.SetColor(Color.White);
		}
			
		if (m_wMarkerBackground)
			m_wMarkerBackground.SetColor(m_MarkerColor);
		
		m_bIsInitialized = true;
		
		// Apply any pending server data
		if (m_bServerDataPending)
			ApplyServerData();
	}
	
	/**
	 * Create the marker widget structure programmatically
	 * @param parent Parent widget to attach to
	 * @return Created marker root widget
	 */
	protected Widget CreateMarkerWidget(Widget parent)
	{
		// Create root frame widget for proper FrameSlot manipulation
		Widget root = GetGame().GetWorkspace().CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, parent);
		if (!root)
			return null;
		
		// Create background panel for better visibility
		Widget background = GetGame().GetWorkspace().CreateWidget(WidgetType.PanelWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, root);
		if (background)
		{
			m_wMarkerBackground = PanelWidget.Cast(background);
			if (m_wMarkerBackground)
			{
				// Set background color with some transparency
				m_wMarkerBackground.SetColor(m_MarkerColor);
				m_wMarkerBackground.SetOpacity(0.8);
			}
			
			// Position background to fill the root widget completely
			FrameSlot.SetAnchor(background, 0.0, 0.0);
			FrameSlot.SetAlignment(background, 0.0, 0.0);
			FrameSlot.SetSize(background, m_fMaxIconSize, m_fMaxIconSize);
			FrameSlot.SetPos(background, 0, 0);
		}
		
		// Create text widget
		Widget text = GetGame().GetWorkspace().CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, root);
		if (text)
		{
			m_wMarkerText = TextWidget.Cast(text);
			if (m_wMarkerText)
			{
				m_wMarkerText.SetText(m_sMarkerLetter);
				// Use default text color - no need to set explicitly
			}
			
			// Center the text within the root widget
			FrameSlot.SetAnchor(text, 0.5, 0.5);
			FrameSlot.SetAlignment(text, 0.5, 0.5);
			FrameSlot.SetSize(text, m_fMaxIconSize, m_fMaxIconSize);
			FrameSlot.SetPos(text, 0, 0);
		}
		
		// Set initial size and position for the root widget
		FrameSlot.SetSize(root, m_fMaxIconSize, m_fMaxIconSize);
		FrameSlot.SetPos(root, -10000, -10000); // Position off-screen initially until proper positioning

		// Ensure the root widget is visible but with reasonable z-order
		root.SetOpacity(0.0); // Start invisible until properly positioned
		root.SetZOrder(100); // Lower z-order to avoid blocking other UI elements
		
		return root;
	}
	
	//===================================================================================
	// UPDATE LOOP
	//===================================================================================
	
	/**
	 * Update marker position and visibility every frame
	 */
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Always allow frame updates if initialized and has root widget
		// Don't skip based on visibility - let UpdateMarkerDisplay handle that
		if (!m_bIsInitialized || !m_wMarkerRoot)
		{
			return;
		}
		
		UpdateFlashing(timeSlice);
		UpdateWorldPosition();
		
		// Only update display if marker should be visible
		if (m_bIsVisible)
		{
			UpdateMarkerDisplay();
		}
		else
		{
			// Hide the marker if it should not be visible
			if (m_wMarkerRoot)
			{
				m_wMarkerRoot.SetOpacity(0.0);
			}
		}
	}
	
	/**
	 * Update the world position from the owner entity
	 */
	protected void UpdateWorldPosition()
	{
		// Use the entity's origin as the base position
		m_vWorldPosition = m_Owner.GetOrigin();
		
		// Try to get a more accurate center position using physics geometry
		Physics physics = m_Owner.GetPhysics();
		if (physics)
		{
			// Get the center of mass if available
			vector centerOfMass = physics.GetCenterOfMass();
			if (centerOfMass != vector.Zero)
			{
				// Transform center of mass to world coordinates
				vector worldCenterOfMass = m_Owner.CoordToParent(centerOfMass);
				m_vWorldPosition = worldCenterOfMass;
			}
		}
		
		// If physics didn't work, try using visual bounds
		if (m_vWorldPosition == m_Owner.GetOrigin())
		{
			vector mins, maxs;
			m_Owner.GetBounds(mins, maxs);
			// Calculate the center of the bounding box in local coordinates
			vector localCenter = (mins + maxs) * 0.5;
			// Transform to world coordinates
			m_vWorldPosition = m_Owner.CoordToParent(localCenter);
		}
		
		// Offset above the MCOM for better visibility
		m_vWorldPosition[1] = m_vWorldPosition[1] + 2.0;
        m_vWorldPosition[0] = m_vWorldPosition[0] + 1; // Adjusted for better visibility
	}
	
	/**
	 * Update flashing animation
	 * @param timeSlice Time since last frame
	 */
	protected void UpdateFlashing(float timeSlice)
	{
		if (!m_bIsFlashing)
			return;
		
		m_fFlashTimer += timeSlice;
		
		if (m_fFlashTimer >= m_fFlashInterval)
		{
			m_fFlashTimer = 0.0;
			m_bFlashState = !m_bFlashState;
			
			// Simply change the text color based on flash state
			if (m_wMarkerText)
			{
				if (m_bFlashState && m_FlashColor)
				{
					// Flash state: use flash color
					m_wMarkerText.SetColor(m_FlashColor);
				}
				else
				{
					// Normal state: use original color (white for text)
					m_wMarkerText.SetColor(Color.White);
				}
			}
		}
	}
	
	/**
	 * Update marker display based on camera position and distance
	 */
	protected void UpdateMarkerDisplay()
	{
		// Hide marker if map is open
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		// Project 3D world position to 2D screen coordinates
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(m_vWorldPosition, GetGame().GetWorld());
		
		// Get camera position and calculate distance
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (!camera)
		{
			m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		vector cameraPosition = camera.GetOrigin();
		float distance = vector.Distance(cameraPosition, m_vWorldPosition);
		
		// Hide if behind camera or too far away
		if (screenPosition[2] < 0 || distance > m_fMaxDistance)
		{
			m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		// Calculate icon size based on distance
		float sizeFactor = Math.InverseLerp(m_fMaxDistance, m_fMinDistance, distance);
		float iconSize = Math.Lerp(m_fMinIconSize, m_fMaxIconSize, sizeFactor);
		
		// Update widget position and size
		FrameSlot.SetPos(m_wMarkerRoot, screenPosition[0] - iconSize * 0.5, screenPosition[1] - iconSize * 0.5);
		FrameSlot.SetSize(m_wMarkerRoot, iconSize, iconSize);
		
		// Set opacity based on distance (closer = more opaque)
		float opacity = Math.Clamp(sizeFactor, 0.3, 1.0);
		m_wMarkerRoot.SetOpacity(opacity);
	}
	
	/**
	 * Set the marker color
	 * @param color The color to use
	 */
	void SetMarkerColor(Color color)
	{
		m_MarkerColor = color;
		if (m_wMarkerBackground)
			m_wMarkerBackground.SetColor(color);
	}
	
	/**
	 * Set the marker letter (A or B) and update UI if initialized
	 * @param letter The letter to display
	 */
	void SetMarkerLetter(string letter)
	{
		m_sMarkerLetter = letter;
		
		// Update the UI text widget if it's already been created
		if (m_wMarkerText)
		{
			m_wMarkerText.SetText(letter);
		}
	}
	
	/**
	 * Show or hide the marker
	 * @param visible True to show, false to hide
	 */
	void SetVisible(bool visible)
	{
		m_bIsVisible = visible;
		
		if (m_wMarkerRoot)
		{
			if (visible)
				m_wMarkerRoot.SetOpacity(1.0);
			else
				m_wMarkerRoot.SetOpacity(0.0);
		}
	}
	
	/**
	 * Check if marker is currently visible
	 * @return True if visible
	 */
	bool IsVisible()
	{
		return m_bIsVisible;
	}
	
	/**
	 * Start flashing the marker with a specific color
	 * @param flashColor The color to flash with
	 */
	void StartFlashing(Color flashColor)
	{
		m_bIsFlashing = true;
		m_FlashColor = flashColor;
		m_fFlashTimer = 0.0;
		m_bFlashState = true;
		
		// Ensure marker is visible when flashing starts
		if (!m_bIsVisible)
		{
			SetVisible(true);
		}
		
		// Immediately apply flash color to text
		if (m_wMarkerText && m_FlashColor)
		{
			m_wMarkerText.SetColor(m_FlashColor);
		}
	}
	
	/**
	 * Stop flashing and return to normal color
	 */
	void StopFlashing()
	{
		m_bIsFlashing = false;
		m_fFlashTimer = 0.0;
		m_bFlashState = true;
		
		// Return text to normal color (white)
		if (m_wMarkerText)
		{
			m_wMarkerText.SetColor(Color.White);
		}
	}
	
	/**
	 * Check if marker is currently flashing
	 * @return True if flashing
	 */
	bool IsFlashing()
	{
		return m_bIsFlashing;
	}
	
	/**
	 * Initialize marker from server command (for multiplayer replication)
	 * @param letter The marker letter (A or B)
	 * @param color The marker color
	 * @param visible Whether the marker should be visible
	 */
	void InitializeMarkerFromServer(string letter, Color color, bool visible)
	{
		// Store server data for later application
		m_sServerLetter = letter;
		m_ServerColor = color;
		m_bServerVisible = visible;
		m_bServerDataPending = true;
		
		// If UI is ready, apply immediately
		if (m_bIsInitialized)
		{
			ApplyServerData();
		}
		else
		{
			// UI not ready, ensure initialization happens
			GetGame().GetCallqueue().CallLater(InitializeMarkerUI, 100, false);
		}
	}
	
	/**
	 * Apply server data to UI elements
	 */
	protected void ApplyServerData()
	{
		if (!m_bServerDataPending)
			return;
		
		// Update properties first
		m_sMarkerLetter = m_sServerLetter;
		m_MarkerColor = m_ServerColor;
		
		// Update UI elements
		if (m_wMarkerText)
		{
			m_wMarkerText.SetText(m_sServerLetter);
		}
		if (m_wMarkerBackground)
		{
			m_wMarkerBackground.SetColor(m_ServerColor);
		}
		
		// Set visibility last - this will update m_bIsVisible and widget opacity
		SetVisible(m_bServerVisible);
		
		// Clear pending flag
		m_bServerDataPending = false;
	}
	
	//===================================================================================
	// CLEANUP
	//===================================================================================
	
	/**
	 * Clean up when component is deleted
	 */
	override void OnDelete(IEntity owner)
	{
		if (m_wMarkerRoot)
		{
			m_wMarkerRoot.RemoveFromHierarchy();
			m_wMarkerRoot = null;
		}
	}
	
	/**
	 * Check if initialization completed properly (fallback for clients)
	 */
	protected void CheckInitializationStatus()
	{
		// Only run on clients
		if (Replication.IsServer())
			return;
		
		if (!m_bIsInitialized || !m_wMarkerRoot)
		{
			InitializeMarkerUI();
		}
	}
}
