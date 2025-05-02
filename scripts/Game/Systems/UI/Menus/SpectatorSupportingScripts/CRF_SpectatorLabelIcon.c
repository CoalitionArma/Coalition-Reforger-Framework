/**
 * UI component that displays an icon and label for an entity in spectator mode.
 * Handles automatic scaling and opacity based on distance to the camera.
 */
class CRF_SpectatorLabelIcon : SCR_ScriptedWidgetComponent
{	
	// Icon size configuration
	[Attribute("42.0")]
	protected float m_fMaxIconSize;
	[Attribute("4.0")]
	protected float m_fMinIconSize;
	
	// Entity reference and position tracking
	IEntity m_eEntity;
	vector m_vWorldPosition;
	protected TNodeId m_iBoneIndex;
	
	// Widget references
	ImageWidget m_wSpectatorLabelIcon;
	ButtonWidget m_wLabelButton;
	OverlayWidget m_wSpectatorLabelBackground;
	RichTextWidget m_wSpectatorLabelText;
	PanelWidget m_wSpectatorLabel;
	
	// Icon display distance configuration
	protected float m_fMaxIconDistance = 800.0; // Icons disappear beyond this distance
	protected float m_fMinIconDistance = 10.0;  // Distance at which icon reaches max size
	
	// Icon opacity configuration
	protected float m_fMaxIconOpacity = 1.0;
	protected float m_fMinIconOpacity = 0.8;
	
	// Label display distance configuration
	protected float m_fMaxLabelDistance = 50.0; // Labels fade out beyond this distance
	protected float m_fMinLabelDistance = 10.0; // Labels are fully visible closer than this
	
	// Runtime state
	protected float m_fDistanceToIcon;
	protected bool m_bForceShowName;
	
	/**
	 * Gets the world position of the tracked entity.
	 */
	vector GetWorldPosition()
	{
		return m_vWorldPosition;
	}
	
	/**
	 * Initializes widget references when this handler is attached.
	 */
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wSpectatorLabelIcon = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIcon"));
		m_wLabelButton = ButtonWidget.Cast(w.FindAnyWidget("LabelButton"));
		m_wSpectatorLabelBackground = OverlayWidget.Cast(w.FindAnyWidget("SpectatorLabelBackground"));
		m_wSpectatorLabelText = RichTextWidget.Cast(w.FindAnyWidget("SpectatorLabelText"));
		m_wSpectatorLabel = PanelWidget.Cast(w.FindAnyWidget("SpectatorLabel"));
	}
	
	/**
	 * Shows the entity name when mouse hovers over the icon.
	 */
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_bForceShowName = true;
		return true;
	}
	
	/**
	 * Hides the entity name when mouse leaves the icon.
	 */
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_bForceShowName = false;
		return true;
	}
	
	/**
	 * Sets the entity to track and optionally a specific bone to follow.
	 * @param entity The entity to track
	 * @param boneName Optional bone name to track instead of entity origin
	 */
	void SetEntity(IEntity entity, string boneName)
	{
		m_eEntity = entity;
		
		// If a bone name is provided, get its index for tracking
		if (boneName != "")
		{
			m_iBoneIndex = entity.GetAnimation().GetBoneIndex(boneName);
		}
	}
	
	/**
	 * Updates the position, size, and visibility of the icon and label.
	 * Called every frame when the widget is active.
	 */
	void Update()
	{
		// Skip processing if entity is invalid
		if (!m_eEntity)
			return;
			
		// Calculate world position (either entity origin or specific bone position)
		UpdateWorldPosition();
		
		// Project 3D world position to 2D screen coordinates
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(m_vWorldPosition, GetGame().GetWorld());
		
		// Calculate distance from camera to entity
		vector cameraPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin();
		m_fDistanceToIcon = vector.Distance(cameraPosition, m_vWorldPosition);
		
		// Hide if behind camera or too far away
		if (screenPosition[2] < 0 || m_fDistanceToIcon > m_fMaxIconDistance)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		
		// Icon is visible, set full opacity for the root widget
		m_wRoot.SetOpacity(1.0);
		
		// Update label content via derived class implementation
		UpdateLabel();
		
		// Update label visibility based on distance
		UpdateLabelVisibility();
		
		// Update icon size and position based on distance
		UpdateIconAppearance(screenPosition);
		
		// Handle forced name display (when mouse hovers)
		if (m_bForceShowName)
		{
			m_wSpectatorLabel.SetOpacity(1.0);
			m_wRoot.SetZOrder(100); // Bring to front
		}
	}
	
	/**
	 * Updates the world position of the entity, accounting for bone positioning if specified.
	 */
	protected void UpdateWorldPosition()
	{
		m_vWorldPosition = m_eEntity.GetOrigin();
		
		// If tracking a specific bone, calculate its world position
		if (m_iBoneIndex > 0)
		{
			vector entityMatrix[4];
			m_eEntity.GetTransform(entityMatrix);
			
			vector boneMatrix[4];
			m_eEntity.GetAnimation().GetBoneMatrix(m_iBoneIndex, boneMatrix);
			
			vector resultMatrix[4];
			Math3D.MatrixMultiply4(entityMatrix, boneMatrix, resultMatrix);
			
			m_vWorldPosition = resultMatrix[3]; // Position component of the matrix
		}
	}
	
	/**
	 * Updates the label's visibility and opacity based on distance.
	 */
	protected void UpdateLabelVisibility()
	{
		if (m_fDistanceToIcon > m_fMaxLabelDistance)
		{
			m_wSpectatorLabel.SetOpacity(0.0);
		}
		else
		{
			// Calculate opacity based on distance (1.0 when close, fading to 0.0)
			float opacityLabel = 1.0 - ((m_fDistanceToIcon - m_fMinLabelDistance) / (m_fMaxLabelDistance - m_fMinLabelDistance));
			
			// Clamp opacity to maximum of 1.0
			if (opacityLabel > 1.0)
				opacityLabel = 1.0;
				
			m_wSpectatorLabel.SetOpacity(opacityLabel);
		}
	}
	
	/**
	 * Updates the icon's size, opacity, and position based on distance.
	 */
	protected void UpdateIconAppearance(vector screenPosition)
	{
		// Calculate distance scale factor (1.0 when close, 0.0 when far)
		float distanceScale = 1.0 - ((m_fDistanceToIcon - m_fMinIconDistance) / (m_fMaxIconDistance - m_fMinIconDistance));
		
		// Clamp scale to maximum of 1.0
		if (distanceScale > 1.0)
			distanceScale = 1.0;
			
		// Calculate icon size and opacity based on distance
		float scaledIconSize = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * distanceScale;
		float scaledIconOpacity = m_fMinIconOpacity + (m_fMaxIconOpacity - m_fMinIconOpacity) * distanceScale;
		
		// Get label dimensions
		float labelSizeX, labelSizeY;
		m_wSpectatorLabel.GetScreenSize(labelSizeX, labelSizeY);
		float labelSizeXD = GetGame().GetWorkspace().DPIUnscale(labelSizeX);
		float labelSizeYD = GetGame().GetWorkspace().DPIUnscale(labelSizeY);
		
		// Position and size the icon
		FrameSlot.SetSize(m_wSpectatorLabelIcon, scaledIconSize, scaledIconSize);
		FrameSlot.SetPos(m_wSpectatorLabelIcon, -scaledIconSize/2, -scaledIconSize/2);
		
		// Position and size the button if it exists
		if (m_wLabelButton)
		{
			FrameSlot.SetSize(m_wLabelButton, scaledIconSize, scaledIconSize);
			FrameSlot.SetPos(m_wLabelButton, -scaledIconSize/2, -scaledIconSize/2);
		}
		
		// Position the label above the icon
		FrameSlot.SetPos(m_wSpectatorLabel, -labelSizeXD/2, -scaledIconSize);
		
		// Position the root widget at the screen position
		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);
		
		// Set z-order based on distance (closer objects appear on top)
		m_wRoot.SetZOrder(screenPosition[2] * -10000);
	}
	
	/**
	 * Updates the label content. Meant to be overridden by derived classes.
	 */
	void UpdateLabel()
	{
		// Implementation in derived classes
	}
}