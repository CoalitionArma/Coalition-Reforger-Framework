class CRF_SpectatorLabelIconCharacter : CRF_SpectatorLabelIcon
{
	//------------------------------------------------------------------------------------------------
	// Class member variables
	//------------------------------------------------------------------------------------------------
	
	// Character state
	protected bool m_bDead = false;
	protected bool m_bWounded = false;
	protected float m_fClickIgnoreTime;
	
	// Spectator menu reference (set by SetIconForEntity so MMB can call SelectSpecCursorTPP directly)
	protected CRF_SpectatorMenu m_SpectatorMenu;
	
	// Character components
	protected SCR_ChimeraCharacter m_eChimeraCharacter;
	protected SCR_CharacterControllerComponent m_ControllerComponent;
	protected SCR_EditableCharacterComponent m_EditableCharacterComponent;
	protected CRF_Gamemode m_Gamemode;
	
	// UI widgets
	protected ImageWidget m_wSpectatorLabelIconBackground;
	protected ImageWidget m_wSpectatorLabelIconCircle;
	protected ImageWidget m_wSpectatorLabelIconWounded;
	protected ImageWidget m_wSpectatorLabelIconCircleSmall;
	protected OverlayWidget m_wOverlayCircle;
	
	// Resources and appearance
	protected ResourceName m_rIconImageSet = "{F3A9B47F55BE8D2B}UI/Textures/Icons/PS_Atlas_x64.imageset";
	protected ref Color m_cDeadColor = Color.Gray;
	
	protected static const float CHAR_ICON_FADE_NEAR = 250.0; // fully opaque closer than this
	protected static const float CHAR_ICON_FADE_FAR  = 500.0; // fully transparent beyond this
	
	//------------------------------------------------------------------------------------------------
	// Widget initialization
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		// Find and cache all widget references
		m_wSpectatorLabelIconBackground = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconBackground"));
		m_wSpectatorLabelIconCircle = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconCircle"));
		m_wSpectatorLabelIconWounded = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconWounded"));
		m_wSpectatorLabelIconCircleSmall = ImageWidget.Cast(w.FindAnyWidget("SpectatorLabelIconCircleSmall"));
		m_wOverlayCircle = OverlayWidget.Cast(w.FindAnyWidget("OverlayCircle"));
		
		// Get game mode instance
		m_Gamemode = CRF_Gamemode.GetInstance();
		
		super.HandlerAttached(w);
	}
	
	//------------------------------------------------------------------------------------------------
	// Returns the button component from the label
	//------------------------------------------------------------------------------------------------
	SCR_ButtonTextComponent GetButton()
	{
		if (!m_wRoot)
			return null;
			
		Widget buttonWidget = m_wRoot.FindAnyWidget("LabelButton");
		if (!buttonWidget)
			return null;
			
		return SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
	}

	//------------------------------------------------------------------------------------------------
	// Store a reference to the spectator menu so OnMMBClicked can call SelectSpecCursorTPP
	//------------------------------------------------------------------------------------------------
	void SetSpectatorMenu(CRF_SpectatorMenu menu)
	{
		m_SpectatorMenu = menu;
	}

	//------------------------------------------------------------------------------------------------
	// Called when the player middle-clicks this icon.
	// Bypasses cursor hit-testing by using the already-known m_eEntity directly.
	//------------------------------------------------------------------------------------------------
	void OnMMBClicked()
	{
		if (m_SpectatorMenu && m_eEntity)
			m_SpectatorMenu.SelectSpecCursorTPP(m_eEntity);
	}

	//------------------------------------------------------------------------------------------------
	// Called when the player left-clicks this icon (FPP / helmet cam).
	// Bypasses cursor hit-testing by using the already-known m_eEntity directly.
	//------------------------------------------------------------------------------------------------
	void OnLMBClicked()
	{
		if (m_SpectatorMenu && m_eEntity)
			m_SpectatorMenu.SelectSpecCursorFPP(m_eEntity);
	}

	//------------------------------------------------------------------------------------------------
	// Override Update to apply far-distance fade-out on character icons
	//------------------------------------------------------------------------------------------------
	override void Update()
	{
		if (!m_eEntity)
			return;
		
		// Let the base class handle world-position projection, distance measurement,
		// far-distance hard-culling, label visibility, icon sizing and z-ordering.
		super.Update();
		
		// Soft-fade icons as the camera pulls away, so they dissolve smoothly rather
		// than popping out at the hard-cull distance.
		if (m_fDistanceToIcon >= CHAR_ICON_FADE_FAR)
		{
			m_wRoot.SetOpacity(0.0);
			return;
		}
		
		if (m_fDistanceToIcon > CHAR_ICON_FADE_NEAR)
		{
			float farOpacity = 1.0 - (m_fDistanceToIcon - CHAR_ICON_FADE_NEAR) / (CHAR_ICON_FADE_FAR - CHAR_ICON_FADE_NEAR);
			m_wRoot.SetOpacity(farOpacity);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Set up the entity associated with this label
	//------------------------------------------------------------------------------------------------
	override void SetEntity(IEntity entity, string boneName)
	{
		super.SetEntity(entity, boneName);
		
		if (!entity)
			return;
		
		// Get character and its components
		m_eChimeraCharacter = SCR_ChimeraCharacter.Cast(entity);
		if (!m_eChimeraCharacter)
			return;
			
		m_ControllerComponent = SCR_CharacterControllerComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_CharacterControllerComponent));
		m_EditableCharacterComponent = SCR_EditableCharacterComponent.Cast(m_eChimeraCharacter.FindComponent(SCR_EditableCharacterComponent));
		
		// Set faction colors
		SetupFactionColors();
		
		// Special setup for spectator
		SetupSpectatorView();
		
		// Load character icon
		LoadCharacterIcon();
	}
	
	//------------------------------------------------------------------------------------------------
	// Setup faction-based colors
	//------------------------------------------------------------------------------------------------
	protected void SetupFactionColors()
	{
		if (!m_eChimeraCharacter)
			return;
			
		SCR_Faction faction = SCR_Faction.Cast(m_eChimeraCharacter.GetFaction());
		if (!faction)
			return;
			
		// Apply faction colors to UI elements
		m_wSpectatorLabelIcon.SetColor(faction.GetOutlineFactionColor());
		m_wSpectatorLabelIconBackground.SetColor(faction.GetFactionColor());
		m_wSpectatorLabelIconCircle.SetColor(faction.GetOutlineFactionColor());
		m_wSpectatorLabelIconCircleSmall.SetColor(faction.GetFactionColor());
	}
	
	//------------------------------------------------------------------------------------------------
	// Configure special display settings for spectators
	//------------------------------------------------------------------------------------------------
	protected void SetupSpectatorView()
	{
		if (!m_EditableCharacterComponent)
			return;
			
		if (CRF_GamemodeManager.IsSpectator(m_EditableCharacterComponent.GetOwner()))
		{
			// Apply spectator-specific settings
			m_fMaxIconSize = 20;
			m_fMinIconOpacity = 1;
			m_fMaxIconOpacity = 1;
			m_fMaxIconDistance = 425;
			m_fMaxLabelDistance = 425;
			
			// Hide spectator-specific UI elements
			m_wSpectatorLabelText.SetVisible(false);
			m_wSpectatorLabelIconBackground.SetVisible(false);
			m_wSpectatorLabelIconCircle.SetVisible(false);
			m_wSpectatorLabelIconCircleSmall.SetVisible(false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Load the character's icon
	//------------------------------------------------------------------------------------------------
	protected void LoadCharacterIcon()
	{
		if (!m_EditableCharacterComponent || !m_wSpectatorLabelIcon)
			return;
			
		SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
		if (uiInfo)
			m_wSpectatorLabelIcon.LoadImageTexture(0, uiInfo.GetIconPath());
	}
	
	//------------------------------------------------------------------------------------------------
	// Update the label with current character information
	//------------------------------------------------------------------------------------------------
	override void UpdateLabel()
	{
		super.UpdateLabel();
		
		// Update player name text
		UpdatePlayerName();
		
		// Handle visibility based on distance
		UpdateDistanceVisibility();
		
		// Update character state (wounded/dead)
		UpdateCharacterState();
	}
	
	//------------------------------------------------------------------------------------------------
	// Update the player name displayed on the label
	//------------------------------------------------------------------------------------------------
	protected void UpdatePlayerName()
	{
		if (!m_eEntity || !m_wSpectatorLabelText)
			return;

		bool isDead = !CheckIfEntityAlive(m_eEntity);

		RplComponent rplComponent = RplComponent.Cast(m_eEntity.FindComponent(RplComponent));
		if (!rplComponent)
			return;
			
		CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotDataFromCharacter(rplComponent.Id());
		
		string displayName = "";
		
		int playerId = 0;
		if (slotData)
			playerId = slotData.GetSlotCurrentPlayerId();
			
		if (playerId > 0)
		{
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (!playerName.IsEmpty())
				displayName = playerName;
			else
				displayName = "DISCONNECTED PLAYER";
		}
		else 
		{
			if (slotData)
				displayName = slotData.GetSlotName();
		}
		
		if (isDead && !displayName.IsEmpty())
			displayName = "† " + displayName;
		
		m_wSpectatorLabelText.SetText(displayName);
		
		if (isDead)
			m_wSpectatorLabelText.SetOpacity(0.3);
		else
			m_wSpectatorLabelText.SetOpacity(1.0);
	}
	
	/**
	 * Check if the provided entity is considered "alive"
	 * @param entity - Entity to check
	 */
	protected bool CheckIfEntityAlive(IEntity entity)
	{
		// Get ChimeraCharacter so we can pull the controller
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return false;
	
		// Get the controller from the character
		CharacterControllerComponent controller = character.GetCharacterController();
	
		// If the character is a valid character and is not dead then return that this guy ain't dead
		if (controller && controller.GetLifeState() != ECharacterLifeState.DEAD)
			return true;
		else 
			return false;
	}
	
	//------------------------------------------------------------------------------------------------
	// Update visibility based on distance
	//------------------------------------------------------------------------------------------------
	protected void UpdateDistanceVisibility()
	{
		if (!m_wOverlayCircle || !m_wSpectatorLabelIcon)
			return;
			
		bool isVisible = m_fDistanceToIcon <= m_fMaxLabelDistance;
		m_wOverlayCircle.SetVisible(isVisible);
		m_wSpectatorLabelIcon.SetVisible(isVisible);
	}
	
	//------------------------------------------------------------------------------------------------
	// Update character state (wounded/dead) and associated UI
	//------------------------------------------------------------------------------------------------
	protected void UpdateCharacterState()
	{
		if (!m_EditableCharacterComponent || !m_ControllerComponent)
			return;
			
		// If character is already marked as dead, no need to update
		if (m_bDead && m_EditableCharacterComponent.GetVisibleSelf())
			return;
			
		if (m_EditableCharacterComponent.GetVisibleSelf())
		{
			// Handle wounded state
			bool isUnconscious = m_ControllerComponent.IsUnconscious();
			if (!m_bWounded && isUnconscious)
			{
				m_wSpectatorLabelIconWounded.SetVisible(true);
				m_bWounded = true;
			}
			else if (m_bWounded && !isUnconscious)
			{
				// Reset to normal state from wounded
				SCR_UIInfo uiInfo = m_EditableCharacterComponent.GetInfo();
				if (uiInfo)
					m_wSpectatorLabelIcon.LoadImageTexture(0, uiInfo.GetIconPath());
					
				m_wSpectatorLabelIconWounded.SetVisible(false);
				m_bWounded = false;
			}
			
			// Handle dead state
			if (m_ControllerComponent.IsDead()) 
			{
				// Apply visual changes for dead state
				m_wSpectatorLabelIcon.SetOpacity(0.9);
				m_wSpectatorLabelBackground.SetOpacity(0.9);
				m_wSpectatorLabelIconBackground.SetColor(m_cDeadColor);
				m_wSpectatorLabelIconCircleSmall.SetColor(m_cDeadColor);
				m_wSpectatorLabelIconBackground.SetOpacity(0.2);
				m_wSpectatorLabelIconWounded.SetVisible(false);
				m_wSpectatorLabelIconCircleSmall.SetOpacity(0.95);
				m_bDead = true;
			}
		}
		else 
		{
			// Entity not visible - reduce opacity
			if (m_wRoot)
				m_wRoot.SetOpacity(0.5);
		}
	}
}