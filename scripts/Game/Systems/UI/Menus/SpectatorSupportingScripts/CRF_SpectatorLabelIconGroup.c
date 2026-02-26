/**
 * UI component that displays a floating NATO group icon above a group's average position
 * in spectator mode. Shows the group's NATO symbol shape and name label.
 */
class CRF_SpectatorLabelIconGroup : CRF_SpectatorLabelIcon
{
	//------------------------------------------------------------------------------------------------
	// Class member variables
	//------------------------------------------------------------------------------------------------
	
	// Group reference
	protected SCR_AIGroup m_Group;
	
	// UI widgets
	protected ImageWidget m_wGroupNATOIcon;
	protected ImageWidget m_wGroupNATOBackground;
	
	// Cached raw centroid (before height offset)
	protected vector m_vRawCentroid;
	
	// Group display settings
	protected static const float GROUP_ICON_HEIGHT_OFFSET_MIN = 6.0;
	protected static const float GROUP_ICON_HEIGHT_OFFSET_MAX = 65.0;
	
	// Fade-out when the camera is very close to the group — individual character icons
	// are clearly visible at short range, so the group symbol becomes redundant noise.
	protected static const float GROUP_ICON_FADE_START = 20.0; 
	protected static const float GROUP_ICON_FADE_END   = 10.0; 
	
	// Icon sizing — group icons are larger than individual character icons so they are
	// easily visible from a distance but don't overwhelm individual unit icons.
	// scaledIconSize drives the HEIGHT; width is derived from the NATO 3:2 aspect ratio.
	protected static const float GROUP_MAX_ICON_SIZE = 52.0;
	protected static const float GROUP_MIN_ICON_SIZE = 24.0;
	protected static const float GROUP_ICON_ASPECT_RATIO = 2.0;
	
	//------------------------------------------------------------------------------------------------
	// Widget initialization
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		// Find group-specific widget references
		m_wGroupNATOIcon = ImageWidget.Cast(w.FindAnyWidget("GroupNATOIcon"));
		m_wGroupNATOBackground = ImageWidget.Cast(w.FindAnyWidget("GroupNATOBackground"));
		
		super.HandlerAttached(w);
	}
	
	//------------------------------------------------------------------------------------------------
	// Set the group associated with this label
	//------------------------------------------------------------------------------------------------
	void SetGroup(SCR_AIGroup group)
	{
		m_Group = group;
		
		if (!group)
			return;
		
		// Apply group-specific icon size — larger than individual character icons
		m_fMaxIconSize = GROUP_MAX_ICON_SIZE;
		m_fMinIconSize = GROUP_MIN_ICON_SIZE;
		
		// Set faction colors
		SetupFactionColors();
		
		// Load group NATO icon
		LoadGroupIcon();
		
		// Set group name
		SetGroupName();
	}
	
	//------------------------------------------------------------------------------------------------
	// Get the group reference
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetGroup()
	{
		return m_Group;
	}
	
	//------------------------------------------------------------------------------------------------
	// Setup faction-based colors for the icon
	//------------------------------------------------------------------------------------------------
	protected void SetupFactionColors()
	{
		if (!m_Group)
			return;
		
		SCR_Faction faction = SCR_Faction.Cast(m_Group.GetFaction());
		if (!faction)
			return;
		
		// Hide the base circle icon — the NATO overlay replaces it entirely
		if (m_wSpectatorLabelIcon)
			m_wSpectatorLabelIcon.SetVisible(false);
		
		// Match the character icon color scheme:
		// GetFactionColor()        = solid fill color (used for background/icon body)
		// GetOutlineFactionColor() = outline/highlight color
		if (m_wGroupNATOIcon)
			m_wGroupNATOIcon.SetColor(faction.GetFactionColor());
		
		// Hide the separate background box — the NATO imageset images already contain the
		// correct background shape (rectangle, circle, etc.) baked into each image
		if (m_wGroupNATOBackground)
			m_wGroupNATOBackground.SetVisible(false);
	}
	
	//------------------------------------------------------------------------------------------------
	// Load the group's NATO flag icon
	//------------------------------------------------------------------------------------------------
	protected void LoadGroupIcon()
	{
		if (!m_Group || !m_wGroupNATOIcon)
			return;
		
		SCR_Faction faction = SCR_Faction.Cast(m_Group.GetFaction());
		if (!faction)
			return;
		
		ResourceName imageSet = faction.GetGroupFlagImageSet();
		string imageName = m_Group.GetGroupFlag();
		
		if (imageSet != "" && imageName != "")
			m_wGroupNATOIcon.LoadImageFromSet(0, imageSet, imageName);
	}
	
	//------------------------------------------------------------------------------------------------
	// Set the group name text
	//------------------------------------------------------------------------------------------------
	protected void SetGroupName()
	{
		if (!m_Group || !m_wSpectatorLabelText)
			return;
		
		string groupName = m_Group.GetCustomName();
		if (groupName.IsEmpty())
			groupName = m_Group.GetCustomNameWithOriginal();
		
		m_wSpectatorLabelText.SetText(groupName);
		m_wSpectatorLabelText.SetExactFontSize(20);
	}
	
	//------------------------------------------------------------------------------------------------
	// Override SetEntity - groups don't track a single entity
	//------------------------------------------------------------------------------------------------
	override void SetEntity(IEntity entity, string boneName)
	{
		// Groups don't track a single entity directly
		// World position is calculated from group member centroids
	}
	
	//------------------------------------------------------------------------------------------------
	// Override Update to calculate group centroid position
	//------------------------------------------------------------------------------------------------
	override void Update()
	{
		if (!m_Group)
			return;
		
		// Calculate group centroid from alive members
		vector centroid;
		int aliveCount;
		if (!CalculateGroupCentroid(centroid, aliveCount))
		{
			// No alive members, hide
			if (m_wRoot)
				m_wRoot.SetOpacity(0.0);
			return;
		}
		
		// Calculate distance from camera to the raw centroid (before height offset)
		vector cameraPosition = GetGame().GetCameraManager().CurrentCamera().GetOrigin();
		m_fDistanceToIcon = vector.Distance(cameraPosition, centroid);
		
		// Scale the height offset with distance so the icon floats higher when zoomed out
		float distanceFraction = Math.Clamp(m_fDistanceToIcon / m_fMaxIconDistance, 0.0, 1.0);
		float heightOffset = GROUP_ICON_HEIGHT_OFFSET_MIN + (GROUP_ICON_HEIGHT_OFFSET_MAX - GROUP_ICON_HEIGHT_OFFSET_MIN) * distanceFraction;
		
		// Cache the raw centroid for line drawing before applying the icon height offset
		m_vRawCentroid = centroid;
		
		// Raise the icon above the centroid by the distance-scaled offset
		centroid[1] = centroid[1] + heightOffset;
		m_vWorldPosition = centroid;
		
		// Project 3D world position to 2D screen coordinates
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(m_vWorldPosition, GetGame().GetWorld());
		
		// Hide if behind camera or too far away
		if (screenPosition[2] < 0 || m_fDistanceToIcon > m_fMaxIconDistance)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		
		// Fade out when the camera is very close — individual character icons are
		// already clearly readable at short range so the group symbol adds clutter.
		if (m_fDistanceToIcon <= GROUP_ICON_FADE_END)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		float closeOpacity = 1.0;
		if (m_fDistanceToIcon < GROUP_ICON_FADE_START)
			closeOpacity = (m_fDistanceToIcon - GROUP_ICON_FADE_END) / (GROUP_ICON_FADE_START - GROUP_ICON_FADE_END);
		
		// Icon is visible
		m_wRoot.SetOpacity(closeOpacity);
		
		// Update label content
		UpdateLabel();
		
		// Update label visibility based on distance
		UpdateLabelVisibility();
		
		// Update icon size and position based on distance
		UpdateIconAppearance(screenPosition);
		
		// Handle forced name display (when mouse hovers)
		if (m_bForceShowName)
		{
			m_wSpectatorLabel.SetOpacity(1.0);
			m_wRoot.SetZOrder(100);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Calculate the centroid position from alive group members.
	// Uses the slot map to find player-controlled characters, because human players are not
	// returned by SCR_AIGroup.GetAgents() (which only yields AI agents).
	// Also falls back to GetAgents() for any pure-AI members.
	//------------------------------------------------------------------------------------------------
	protected bool CalculateGroupCentroid(out vector centroid, out int aliveCount)
	{
		centroid = vector.Zero;
		aliveCount = 0;
		
		// Get the group's replication ID so we can match it against slot data
		RplComponent groupRpl = RplComponent.Cast(m_Group.FindComponent(RplComponent));
		if (!groupRpl)
			return false;
		
		RplId groupRplId = groupRpl.Id();
		
		// --- Player-controlled characters via slot map ---
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (slottingManager)
		{
			map<int, ref CRF_SlotDataContainer> slotMap = slottingManager.GetSlotMap();
			if (slotMap)
			{
				foreach (int slotId, CRF_SlotDataContainer slotData : slotMap)
				{
					if (!slotData || slotData.GetSlotCurrentGroup() != groupRplId)
						continue;
					
					RplId charRplId = slotData.GetSlotCurrentCharacter();
					if (charRplId == RplId.Invalid())
						continue;
					
					RplComponent charRpl = RplComponent.Cast(Replication.FindItem(charRplId));
					if (!charRpl)
						continue;
					
					IEntity entity = charRpl.GetEntity();
					if (!entity)
						continue;
					
					// Check alive state
					SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
					if (controller && controller.IsDead())
						continue;
					
					centroid = centroid + entity.GetOrigin();
					aliveCount++;
				}
			}
		}
		
		// --- AI-controlled characters via GetAgents() (pure-AI groups) ---
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		
		foreach (AIAgent agent : agents)
		{
			IEntity entity = agent.GetControlledEntity();
			if (!entity)
				continue;
			
			ChimeraCharacter character = ChimeraCharacter.Cast(entity);
			if (!character)
				continue;
			
			CharacterControllerComponent controller = character.GetCharacterController();
			if (!controller || controller.GetLifeState() == ECharacterLifeState.DEAD)
				continue;
			
			// Avoid double-counting if this AI entity was already added via slot map
			bool alreadyCounted = false;
			RplComponent entityRpl = RplComponent.Cast(entity.FindComponent(RplComponent));
			if (entityRpl && slottingManager)
			{
				CRF_SlotDataContainer slotData = slottingManager.GetSlotDataFromCharacter(entityRpl.Id());
				if (slotData && slotData.GetSlotCurrentGroup() == groupRplId)
					alreadyCounted = true;
			}
			
			if (!alreadyCounted)
			{
				centroid = centroid + entity.GetOrigin();
				aliveCount++;
			}
		}
		
		if (aliveCount == 0)
			return false;
		
		centroid = centroid * (1.0 / aliveCount);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	// Update the label - refresh group name (may change during play)
	//------------------------------------------------------------------------------------------------
	override void UpdateLabel()
	{
		super.UpdateLabel();
		SetGroupName();
	}
	
	//------------------------------------------------------------------------------------------------
	// Override icon appearance to also size the NATO overlay widget
	//------------------------------------------------------------------------------------------------
	override protected void UpdateIconAppearance(vector screenPosition)
	{
		// Calculate distance scale factor (1.0 when close, 0.0 when far)
		float distanceScale = 1.0 - ((m_fDistanceToIcon - m_fMinIconDistance) / (m_fMaxIconDistance - m_fMinIconDistance));
		
		// Clamp scale to maximum of 1.0
		if (distanceScale > 1.0)
			distanceScale = 1.0;
		
		// Calculate icon size based on distance
		float scaledIconHeight = m_fMinIconSize + (m_fMaxIconSize - m_fMinIconSize) * distanceScale;
		float scaledIconWidth = scaledIconHeight * GROUP_ICON_ASPECT_RATIO;
		
		// Get label dimensions
		float labelSizeX, labelSizeY;
		m_wSpectatorLabel.GetScreenSize(labelSizeX, labelSizeY);
		float labelSizeXD = GetGame().GetWorkspace().DPIUnscale(labelSizeX);
		
		// Position and size the NATO overlay icon, preserving its natural 3:2 aspect ratio
		Widget natoOverlay = m_wRoot.FindAnyWidget("GroupNATOOverlay");
		if (natoOverlay)
		{
			FrameSlot.SetSize(natoOverlay, scaledIconWidth, scaledIconHeight);
			FrameSlot.SetPos(natoOverlay, -scaledIconWidth / 2, -scaledIconHeight / 2);
			
			// Also resize the image widget itself — its hardcoded Size in the layout
			// won't stretch automatically to fill the overlay container
			if (m_wGroupNATOIcon)
				m_wGroupNATOIcon.SetSize(scaledIconWidth, scaledIconHeight);
		}
		
		// Position the label above the icon
		FrameSlot.SetPos(m_wSpectatorLabel, -labelSizeXD / 2, -scaledIconHeight);
		
		// Position the root widget at the screen position
		FrameSlot.SetPos(m_wRoot, screenPosition[0], screenPosition[1]);
		
		// Set z-order based on distance (closer objects appear on top), but behind character icons
		m_wRoot.SetZOrder(screenPosition[2] * -10000 - 1);
	}
}
