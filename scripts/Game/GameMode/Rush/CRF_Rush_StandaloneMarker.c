//------------------------------------------------------------------------------------
// CRF_Rush_StandaloneMarker: Standalone 3D HUD marker for MCOM sites
// Creates floating letters (A/B) above MCOM entities without requiring entity components
//------------------------------------------------------------------------------------

class CRF_Rush_StandaloneMarker
{
	//===================================================================================
	// PROPERTIES
	//===================================================================================
	
	// Configuration
	protected string m_sMarkerLetter = "A";
	protected ref Color m_MarkerColor;
	protected float m_fMaxIconSize = 128.0;
	protected float m_fMinIconSize = 32.0;
	protected float m_fMaxDistance = 2000.0;
	protected float m_fMinDistance = 5.0;
	
	// UI References
	protected Widget m_wMarkerRoot;
	protected TextWidget m_wMarkerText;
	protected PanelWidget m_wMarkerBackground;
	protected bool m_bIsInitialized = false;
	
	// State tracking
	protected vector m_vWorldPosition;
	protected bool m_bIsVisible = true;
	protected IEntity m_TargetEntity;
	
	// Flashing functionality
	protected bool m_bIsFlashing = false;
	protected float m_fFlashTimer = 0.0;
	protected float m_fFlashInterval = 0.4;
	protected bool m_bFlashState = true;
	protected ref Color m_FlashColor;
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	/**
	 * Constructor for entity-based markers
	 * @param targetEntity The entity to track
	 * @param letter The marker letter (A or B)
	 * @param color The marker color
	 */
	void CRF_Rush_StandaloneMarker(IEntity targetEntity, string letter, Color color)
	{
		m_TargetEntity = targetEntity;
		m_sMarkerLetter = letter;
		m_MarkerColor = color;
		
		// Delayed initialization to ensure UI is ready
		GetGame().GetCallqueue().CallLater(InitializeMarkerUI, 1000, false);
	}
	
	/**
	 * Constructor for position-based markers (for clients without entity references)
	 * @param position The world position for the marker
	 * @param letter The marker letter (A or B)
	 * @param color The marker color
	 */
	void CRF_Rush_StandaloneMarkerPositional(vector position, string letter, Color color)
	{
		m_vWorldPosition = position;
		m_sMarkerLetter = letter;
		m_MarkerColor = color;
		m_TargetEntity = null; // No entity reference for positional markers
		
		// Delayed initialization to ensure UI is ready
		GetGame().GetCallqueue().CallLater(InitializeMarkerUI, 1000, false);
	}
	
	/**
	 * Set the world position for this marker (used for positional markers)
	 * @param position The world position to set
	 */
	void SetWorldPosition(vector position)
	{
		m_vWorldPosition = position;
	}
	
	/**
	 * Initialize the 3D marker UI
	 */
	protected void InitializeMarkerUI()
	{
		// For positional markers, we don't need an entity
		if (!m_TargetEntity && m_vWorldPosition == "0 0 0")
		{
			return;
		}
		
		// Find or create the main HUD root
		Widget hudRoot = GetGame().GetWorkspace().FindWidget("HudRoot");
		if (!hudRoot)
		{
			hudRoot = GetGame().GetWorkspace();
		}
		
		if (!hudRoot)
		{
			return;
		}
		
		// Create marker layout
		m_wMarkerRoot = CreateMarkerWidget(hudRoot);
		if (!m_wMarkerRoot)
		{
			return;
		}
		
		// Configure the marker
		if (m_wMarkerText)
		{
			m_wMarkerText.SetText(m_sMarkerLetter);
			m_wMarkerText.SetColor(Color.White);
		}
			
		if (m_wMarkerBackground)
		{
			m_wMarkerBackground.SetColor(m_MarkerColor);
		}
		
		m_bIsInitialized = true;
	}
	
	/**
	 * Create the marker widget structure programmatically
	 * @param parent Parent widget to attach to
	 * @return Created marker root widget
	 */
	protected Widget CreateMarkerWidget(Widget parent)
	{
		// Create root frame widget
		Widget root = GetGame().GetWorkspace().CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, parent);
		if (!root)
		{
			return null;
		}
		
		// Create background panel
		Widget background = GetGame().GetWorkspace().CreateWidget(WidgetType.PanelWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, root);
		if (background)
		{
			m_wMarkerBackground = PanelWidget.Cast(background);
			if (m_wMarkerBackground)
			{
				m_wMarkerBackground.SetColor(m_MarkerColor);
				m_wMarkerBackground.SetOpacity(0.8);
			}
			
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
			}
			
			FrameSlot.SetAnchor(text, 0.5, 0.5);
			FrameSlot.SetAlignment(text, 0.5, 0.5);
			FrameSlot.SetSize(text, m_fMaxIconSize, m_fMaxIconSize);
			FrameSlot.SetPos(text, 0, 0);
		}
		
		// Set initial size and properties
		FrameSlot.SetSize(root, m_fMaxIconSize, m_fMaxIconSize);
		FrameSlot.SetPos(root, 100, 100);
		root.SetOpacity(1.0);
		root.SetZOrder(1000);
		root.SetVisible(true);
		
		return root;
	}
	
	//===================================================================================
	// UPDATE METHODS
	//===================================================================================
	
	/**
	 * Update marker position and visibility (called from gamemode frame update)
	 */
	void UpdateMarker()
	{
		if (!m_bIsInitialized || !m_wMarkerRoot)
			return;
		
		// Update position (works for both entity-based and positional markers)
		UpdateWorldPosition();
		
		if (m_bIsVisible)
		{
			UpdateMarkerDisplay();
		}
		else
		{
			m_wMarkerRoot.SetOpacity(0.0);
		}
	}
	
	/**
	 * Update flashing animation
	 * @param timeSlice Time since last frame
	 */
	void UpdateFlashing(float timeSlice)
	{
		if (!m_bIsFlashing)
			return;
		
		m_fFlashTimer += timeSlice;
		
		if (m_fFlashTimer >= m_fFlashInterval)
		{
			m_fFlashTimer = 0.0;
			m_bFlashState = !m_bFlashState;
			
			if (m_wMarkerText)
			{
				if (m_bFlashState && m_FlashColor)
				{
					m_wMarkerText.SetColor(m_FlashColor);
				}
				else
				{
					m_wMarkerText.SetColor(Color.White);
				}
			}
		}
	}
	
	/**
	 * Update the world position from the target entity or use fixed position
	 */
	protected void UpdateWorldPosition()
	{
		// If we have an entity, update position from entity
		if (m_TargetEntity)
		{
			m_vWorldPosition = m_TargetEntity.GetOrigin();
			
			// Try to get center position using bounds
			vector mins, maxs;
			m_TargetEntity.GetBounds(mins, maxs);
			vector localCenter = (mins + maxs) * 0.5;
			m_vWorldPosition = m_TargetEntity.CoordToParent(localCenter);
			
			// Offset above the MCOM for better visibility
			m_vWorldPosition[1] = m_vWorldPosition[1] + 2.0;
			m_vWorldPosition[0] = m_vWorldPosition[0] + 1.0;
		}
		// If no entity, m_vWorldPosition was set in constructor and remains fixed
	}
	
	/**
	 * Update marker display based on camera position and distance
	 */
	protected void UpdateMarkerDisplay()
	{
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
		
		// Calculate scale and opacity based on distance
		float distanceScale = 1.0 - ((distance - m_fMinDistance) / (m_fMaxDistance - m_fMinDistance));
		if (distanceScale > 1.0)
			distanceScale = 1.0;
		if (distanceScale < 0.1)
			distanceScale = 0.1;
		
		// Calculate final size and opacity
		float finalSize = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * distanceScale;
		float finalOpacity = 0.5 + (0.5 * distanceScale);
		
		// Center the marker precisely on the projected screen position
		float halfSize = finalSize * 0.5;
		float centerX = screenPosition[0] - halfSize;
		float centerY = screenPosition[1] - halfSize;
		
		// Update marker position
		FrameSlot.SetPos(m_wMarkerRoot, centerX, centerY);
		FrameSlot.SetSize(m_wMarkerRoot, finalSize, finalSize);
		
		// Update background
		if (m_wMarkerBackground)
		{
			FrameSlot.SetSize(m_wMarkerBackground, finalSize, finalSize);
			FrameSlot.SetPos(m_wMarkerBackground, 0, 0);
		}
		
		// Update text
		if (m_wMarkerText)
		{
			FrameSlot.SetSize(m_wMarkerText, finalSize, finalSize);
			FrameSlot.SetPos(m_wMarkerText, 0, 0);
		}
		
		// Set opacity and z-order
		m_wMarkerRoot.SetOpacity(finalOpacity);
		m_wMarkerRoot.SetZOrder(Math.Floor(screenPosition[2] * -10000));
	}
	
	//===================================================================================
	// PUBLIC INTERFACE
	//===================================================================================
	
	/**
	 * Set the marker letter (A or B)
	 * @param letter The letter to display
	 */
	void SetMarkerLetter(string letter)
	{
		m_sMarkerLetter = letter;
		if (m_wMarkerText)
			m_wMarkerText.SetText(letter);
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
	 * Show or hide the marker
	 * @param visible True to show, false to hide
	 */
	void SetVisible(bool visible)
	{
		m_bIsVisible = visible;
		
		if (m_wMarkerRoot)
		{
			if (visible)
			{
				m_wMarkerRoot.SetOpacity(1.0);
			}
			else
			{
				m_wMarkerRoot.SetOpacity(0.0);
			}
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
		
		// Immediately apply flash color
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
		
		// Return text to normal color
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
		// Update properties
		m_sMarkerLetter = letter;
		m_MarkerColor = color;
		
		// Update UI elements if already initialized
		if (m_bIsInitialized)
		{
			if (m_wMarkerText)
			{
				m_wMarkerText.SetText(letter);
			}
			if (m_wMarkerBackground)
			{
				m_wMarkerBackground.SetColor(color);
			}
		}
		
		// Set visibility
		SetVisible(visible);
	}
	
	//===================================================================================
	// CLEANUP
	//===================================================================================
	
	/**
	 * Clean up when marker is deleted
	 */
	void Cleanup()
	{
		if (m_wMarkerRoot)
		{
			m_wMarkerRoot.RemoveFromHierarchy();
			m_wMarkerRoot = null;
		}
		m_wMarkerText = null;
		m_wMarkerBackground = null;
		m_TargetEntity = null;
	}
}
