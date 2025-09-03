class CRF_SpectatorMenu: ChimeraMenuBase
{
	//=================================================================================================
	// PROPERTIES
	//=================================================================================================
	
	// UI Widgets
	Widget m_wRoot;                                // Root widget of the menu
	protected FrameWidget m_wMapFrame;                       // Frame widget for the map
	protected Widget m_wPlayerSlotWidget;                    // Widget for player slots
	protected Widget m_wBluforButton;                        // Button for Blufor faction
	protected Widget m_wOpforButton;                         // Button for Opfor faction
	protected Widget m_wIndforButton;                        // Button for Indfor faction
	protected Widget m_wCivButton;                           // Button for Civilian faction
	protected Widget m_wSlotSelector;                        // Widget for selecting slots
	protected FrameWidget m_wFrameSlots;                     // Frame for displaying slots
	protected FrameWidget m_wFrameChannels;                  // Frame for displaying VON channels
	protected CRF_ListboxComponent m_wPlayerSlots;           // Listbox component for player slots
	protected CRF_ListboxComponent m_wVONChannels;           // Listbox component for VON channels
	
	// Game-related components
	protected CRF_Gamemode m_Gamemode;                       // Reference to the gamemode instance
	protected CRF_MenuManager m_MenuManager;                 // Reference to the menu manager
	protected SCR_ChatPanel m_ChatPanel;                     // Reference to the chat panel
	protected SCR_MapEntity m_MapEntity;                     // Reference to the map entity
	protected Faction m_fSelectedFaction;                    // Currently selected faction
	protected IEntity m_eSpecEntity;                         // Entity being spectated
	
	// Spectator tracking
	protected ref array<RplId> m_aEntityIcons = {};          // Array of entity IDs with icons
	protected ref array<Widget> m_aSpectatorWidgets = {};    // Array of spectator UI widgets
	protected ref array<ref CRF_SpectatorLabelIconCharacter> m_aSpectatorIcons = {}; // Array of spectator icons
	
	// Faction counters
	protected int m_iBluforSlots = 0;                        // Total Blufor slots
	protected int m_iOpforSlots = 0;                         // Total Opfor slots
	protected int m_iIndforSlots = 0;                        // Total Indfor slots
	protected int m_iCivSlots = 0;                           // Total Civilian slots
	protected int m_iAliveBluforSlots = 0;                   // Alive Blufor slots
	protected int m_iAliveOpforSlots = 0;                    // Alive Opfor slots
	protected int m_iAliveIndforSlots = 0;                   // Alive Indfor slots
	protected int m_iAliveCivSlots = 0;                      // Alive Civilian slots
	
	// State tracking
	protected bool m_bIsMapOpened = false;                   // Flag indicating if map is open
	protected bool m_bFPPEntityValidityCheck;                // Flag for first-person perspective validity
	protected int m_iLocalChannelUpdates = 0;                // Counter for local channel updates
	protected bool m_bHideUi = false;                        // Flag indicating if UI is hidden
	ref array<Widget> m_aRequest = {};            			  // Array of request widgets
	protected bool m_bFrameEventRegistered = false;          // Flag to track if frame event is registered
	
	bool m_bNVGActivated = false;             				  // NVG activation state for spectator
	
	// Main timer elements
	protected TextWidget m_wTimer;
	protected ImageWidget m_wBackground;

	// Game state references
	protected CRF_SafestartManager m_SafestartManager;
	protected string m_sStoredServerWorldTime;
	protected string m_sServerWorldTime;
	protected SCR_PopUpNotification m_PopUpNotification = null;
	
	//=================================================================================================
	// MENU LIFECYCLE METHODS
	//=================================================================================================
	
	/**
	 * Called when the menu is opened
	 * Sets up UI elements, registers action listeners, and initializes slots
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Initialize HUD visibility
		SCR_HUDManagerComponent hudManager = GetGame().GetHUDManager();
		if (hudManager) 
		{
			hudManager.SetVisible(true);
			hudManager.SetVisibleLayers(EHudLayers.ALWAYS_TOP);
		}
		
		// Initialize widget references
		m_wRoot = GetRootWidget();
		Widget wChatPanel = m_wRoot.FindAnyWidget("ChatPanel");
		if (wChatPanel)
			m_ChatPanel = SCR_ChatPanel.Cast(wChatPanel.FindHandler(SCR_ChatPanel));
		
		// Get game components
		m_Gamemode = CRF_Gamemode.GetInstance();
		m_MenuManager = CRF_MenuManager.GetInstance();
		
		// Initialize UI components
		m_wMapFrame = FrameWidget.Cast(m_wRoot.FindAnyWidget("MapFrame"));
		m_wPlayerSlotWidget = m_wRoot.FindAnyWidget("PlayerSlots");
		m_wPlayerSlots = CRF_ListboxComponent.Cast(m_wPlayerSlotWidget.FindHandler(CRF_ListboxComponent));
		m_wVONChannels = CRF_ListboxComponent.Cast(m_wRoot.FindAnyWidget("VONChannels").FindHandler(CRF_ListboxComponent));
		
		// Register input action listeners
		RegisterActionListeners();
		
		// Initialize slots
		InitSlots();
		
		// Select default faction based on availability
		SelectDefaultFaction();
		
		// Initialize faction buttons
		InitFactionButtons();
		
		// Initialize VON (Voice Over Network)
		if (!CVON_VONGameModeComponent.GetInstance())
			InitVON();
		
		// Update slots and register for slot updates
		UpdateSlots();
		CRF_SlottingManager.GetInstance().GetOnSlottingUpdate().Insert(UpdateSlots);
		
		// Update player icons and spectator UI
		//UpdatePlayerIcons();
		GetGame().GetCallqueue().CallLater(UpdatePlayerIcons, 1000, true);
		
		// Get game system references
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		
		// Find and cast main timer widgets
		m_wTimer = TextWidget.Cast(m_wRoot.FindWidget("timeLeftTimer"));
		m_wBackground = ImageWidget.Cast(m_wRoot.FindWidget("timeLeftBackground"));

		// Get notification system reference
		m_PopUpNotification = SCR_PopUpNotification.GetInstance();
	}
	
	/**
	 * Registers all action listeners for the menu
	 */
	protected void RegisterActionListeners()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			inputManager.AddActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
			inputManager.AddActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
		}
		inputManager.AddActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
		inputManager.AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
		inputManager.AddActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
		inputManager.AddActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
		inputManager.AddActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
		inputManager.AddActionListener("EditorToggleUI", EActionTrigger.DOWN, HideUI);
		inputManager.AddActionListener("CRF_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
	}
	
	/**
	 * Toggles night vision goggles in spectator mode
	 */
	void ToggleNVGs()
	{
		m_bNVGActivated = !m_bNVGActivated;

		if (m_bNVGActivated)
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{0AD0A1ADEBCF893F}Assets/Items/Equipment/NVG/pvs14/data/SpecNVGFilm.emat", true);
		else
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);
	}
	
	/**
	 * forces night vision goggles off in spectator mode
	 */
	void ForceNVGsOff()
	{
		SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);
	}
	
	/**
	 * Initializes faction buttons and their click handlers
	 */
	protected void InitFactionButtons()
	{
		m_wBluforButton = m_wRoot.FindAnyWidget("BLUSelectButton");
		m_wOpforButton = m_wRoot.FindAnyWidget("OPFSelectButton");
		m_wIndforButton = m_wRoot.FindAnyWidget("INDSelectButton");
		m_wCivButton = m_wRoot.FindAnyWidget("CIVSelectButton");
		m_wFrameSlots = FrameWidget.Cast(m_wRoot.FindAnyWidget("FrameSlots"));
		m_wSlotSelector = m_wRoot.FindAnyWidget("SlotSelector");
		m_wFrameChannels = FrameWidget.Cast(m_wRoot.FindAnyWidget("VONSlots"));
		
		// Register faction button click handlers
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wBluforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionBlufor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wOpforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionOpfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wIndforButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionIndfor);
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wCivButton).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(SelectFactionCiv);
		
		// Register create channel button click handler
		SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("CreateChannel")).FindHandler(SCR_ButtonTextComponent)).m_OnClicked.Insert(CreateChannel);
	}
	
	/**
	 * Select default faction based on availability
	 */
	protected void SelectDefaultFaction()
	{
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		
		if(slottingManager.IsFactionValid("BLUFOR"))
			SelectFactionBlufor();
		else if(slottingManager.IsFactionValid("OPFOR"))
			SelectFactionOpfor();
		else if(slottingManager.IsFactionValid("INDFOR"))
			SelectFactionIndfor();
		else if(slottingManager.IsFactionValid("CIV"))
			SelectFactionCiv();
	}
	
	protected void InitVON()
	{
		// Initialize VON with a slight delay to ensure proper setup
		GetGame().GetCallqueue().Call(Action_VONon);
		GetGame().GetCallqueue().Call(Action_VONOff);
	}
	
	/**
	 * Updates the compass UI based on camera orientation
	 */
	void UpdateCompass()
	{
		// Get camera yaw angle
		float yaw = -CRF_PlayerControllerManager.GetInstance().m_eCamera.GetYawPitchRoll()[0];
		float yawFloat = -yaw;
		
		// Convert negative angles to 0-360 range
		if (yawFloat < 0) 
			yawFloat = 360 - Math.AbsFloat(yawFloat);
		
		// Update compass widget position based on camera orientation
		FrameSlot.SetOffsets(
			FrameWidget.Cast(m_wRoot.FindAnyWidget("CompassFrameMoveable")), 
			-1090 - 1880 * (yawFloat / 360), 
			-63, 
			-2750 + 1880 * (yawFloat / 360), 
			-995
		);
	}
	
	/**
	 * Called every frame to update the menu
	 * @param tDelta - Time since last frame
	 */
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);

		// Update compass
		UpdateCompass();
		
		// Ensure map context is active when map is open
		if (m_MapEntity)
			GetGame().GetInputManager().ActivateContext("MapContext");
		
		// Update VON channels if needed and handle radio frequency updates
		if(m_iLocalChannelUpdates != m_MenuManager.m_iChannelChanges)
		{
			UpdateChannel();
			// Radio frequency is updated within UpdateChannel() method
		}
		
		// Handle spectator camera
		UpdateSpectatorCamera(tDelta);
		
		// Process channel requests
		ProcessChannelRequests(tDelta);
		
		// Update UI panel visibility based on cursor position
		UpdateUIPanelVisibility(tDelta);
		
		// Update icons
		UpdateIcons();
		
		// Update chat if available
		if (m_ChatPanel)
			m_ChatPanel.OnUpdateChat(tDelta);
		
		// Set kill feed type to dead local
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent));
		sender.SetKillFeedTypeDeadLocal();
		
		UpdateTimer();
	}
	
	/**
	 * Handle spectator camera updates
	 * @param tDelta - Time since last frame
	 */
	protected void UpdateSpectatorCamera(float tDelta)
	{
		CRF_PlayerControllerManager playerControllerComp = CRF_PlayerControllerManager.GetInstance();
		
		if (m_eSpecEntity)
		{
			InputManager im = GetGame().GetInputManager();
			
			// Check if user is trying to control camera manually or if entity is a spectator
			bool isManualControl = 
				im.GetActionValue("ManualCameraMoveLateral") != 0 || 
				im.GetActionValue("ManualCameraMoveVertical") != 0 || 
				im.GetActionValue("ManualCameraMoveLongitudinal") != 0 || 
				im.GetActionValue("ManualCameraRotate") != 0 || 
				CRF_GamemodeManager.IsSpectator(m_eSpecEntity);
				
			if (isManualControl)
			{
				// Reset spectator entity and unregister frame event
				m_eSpecEntity = null;
				m_bFPPEntityValidityCheck = false;
				UnregisterFrameEvent();
				
				// Reset camera angle after leaving FPP
				vector mat = playerControllerComp.m_eCamera.GetAngles();
				playerControllerComp.m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
			}
			else
			{
				// Register frame event for smooth camera tracking if not already registered
				if (!m_bFrameEventRegistered)
				{
					RegisterFrameEvent();
				}
				m_bFPPEntityValidityCheck = true;
			}
		} 
		else if(!m_eSpecEntity && m_bFPPEntityValidityCheck)
		{
			// Reset camera roll when not spectating and unregister frame event
			vector mat = playerControllerComp.m_eCamera.GetAngles();
			playerControllerComp.m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
			UnregisterFrameEvent();
		}
	}
	
	/**
	 * Registers the frame event for smooth spectator camera tracking
	 */
	protected void RegisterFrameEvent()
	{
		if (!m_bFrameEventRegistered)
		{
			IEntity specEntity = SCR_PlayerController.GetLocalMainEntity();
			
			if (!CRF_GamemodeManager.IsSpectator(specEntity))
				return;
			
			CRF_PlayableCharacter playableChar = CRF_PlayableCharacter.Cast(specEntity.FindComponent(CRF_PlayableCharacter));
			playableChar.SetCameraUpdateEnabled(true, m_eSpecEntity);
			
			m_bFrameEventRegistered = true;
		}
	}
	
	/**
	 * Unregisters the frame event for spectator camera tracking
	 */
	protected void UnregisterFrameEvent()
	{
		if (m_bFrameEventRegistered)
		{
			IEntity specEntity = SCR_PlayerController.GetLocalMainEntity();
			
			if (!CRF_GamemodeManager.IsSpectator(specEntity))
				return;
			
			CRF_PlayableCharacter playableChar = CRF_PlayableCharacter.Cast(specEntity.FindComponent(CRF_PlayableCharacter));
			playableChar.SetCameraUpdateEnabled(false, null);
			
			m_bFrameEventRegistered = false;
		}
	}
	
	/**
	 * Process channel join requests
	 * @param tDelta - Time since last frame
	 */
	protected void ProcessChannelRequests(float tDelta)
	{
		// Process each request widget
		for (int i = m_aRequest.Count() - 1; i >= 0; i--)
		{
			Widget request = m_aRequest[i];
			CRF_ListBoxElementComponent comp = CRF_ListBoxElementComponent.Cast(request.FindHandler(CRF_ListBoxElementComponent));
			
			// Check if player has joined the channel
			if (m_MenuManager.IsPlayerInChannel(comp.m_iPlayerId, comp.m_iChannelId))
			{
				request.RemoveFromHierarchy();
				m_aRequest.RemoveOrdered(i);
				continue;
			}
			
			// Handle request deletion animation
			if (comp.m_bDeleteRequest)
			{
				Widget buttonAnim = request.FindAnyWidget("ButtonAnim");
				float posX = FrameSlot.GetPosX(buttonAnim);
				
				if (posX > 500)
				{
					request.RemoveFromHierarchy();
					m_aRequest.RemoveOrdered(i);
					continue;
				}
				
				FrameSlot.SetPosX(buttonAnim, posX + tDelta * 2000);
				continue;
			}
			else if (FrameSlot.GetPosX(request.FindAnyWidget("ButtonAnim")) > 0)
			{
				// Handle request appearance animation
				Widget buttonAnim = request.FindAnyWidget("ButtonAnim");
				float posX = FrameSlot.GetPosX(buttonAnim);
				
				if (posX - tDelta * 2000 > 0)
					FrameSlot.SetPosX(buttonAnim, posX - tDelta * 2000);
				else 
					FrameSlot.SetPosX(buttonAnim, 0);
			}
			
			// Update request timer
			comp.GetProgress().SetCurrent(comp.GetProgress().GetCurrent() - tDelta);
			if (comp.GetProgress().GetCurrent() <= 0)
			{
				request.RemoveFromHierarchy();
				m_aRequest.RemoveOrdered(i);
			}
		}
	}
	
	/**
	 * Update player icons in the spectator UI
	 */
	protected void UpdatePlayerIcons()
	{
		array<RplId> comparisonRplIds = {};
		IEntity localMainEnt = SCR_PlayerController.GetLocalMainEntity();
		
		//------------------------------------------------------------------------------------------------
		// ALL SLOT-BASED CHARACTERS
		//------------------------------------------------------------------------------------------------
		
		map<int, CRF_SlotDataContainer> slotMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		if (slotMap && !slotMap.IsEmpty())
		{
			foreach (int slotId, CRF_SlotDataContainer slotData : slotMap)
			{		
				RplId slotRplId = slotData.GetSlotCurrentCharacter();
				
				if(slotRplId != RplId.Invalid() && Replication.FindItem(slotRplId))
				{
					IEntity entity = RplComponent.Cast(Replication.FindItem(slotRplId)).GetEntity();
					
					if(entity && entity != localMainEnt)
					{
						comparisonRplIds.Insert(slotRplId);
						SetIconForEntity(entity, slotRplId);
					};
				};
			};
		};
		
		//------------------------------------------------------------------------------------------------
		// ALL SPECTATORS
		//------------------------------------------------------------------------------------------------
		
		PlayerManager playermanager = GetGame().GetPlayerManager();
		
		if (playermanager)
		{
			array<int> arrayplayerIds = {};
			
			playermanager.GetAllPlayers(arrayplayerIds);
			
			foreach (int playerId : arrayplayerIds)
			{
				IEntity playerEntity = playermanager.GetPlayerControlledEntity(playerId);
				
				if (playerEntity && CRF_GamemodeManager.IsSpectator(playerEntity) && playerEntity != localMainEnt)
				{
					RplId playerRplId = RplComponent.Cast(playerEntity.FindComponent(RplComponent)).Id();
					comparisonRplIds.Insert(playerRplId);
					SetIconForEntity(playerEntity, playerRplId);
				};
			};
		};
		
		//------------------------------------------------------------------------------------------------
		// ALL AI
		//------------------------------------------------------------------------------------------------
		
		AIWorld aiworld = GetGame().GetAIWorld();
		
		if (aiworld)
		{
			array<AIAgent> arrayAIAgents = {};
			
			GetGame().GetAIWorld().GetAIAgents(arrayAIAgents);
			
			foreach (AIAgent aiAgent : arrayAIAgents)
			{
				IEntity aiEntity = aiAgent.GetControlledEntity();
				
				SCR_ChimeraCharacter aiCharacter = SCR_ChimeraCharacter.Cast(aiEntity);
				
				if(aiCharacter && aiCharacter != localMainEnt)
				{
					RplId aiRplId = RplComponent.Cast(aiCharacter.FindComponent(RplComponent)).Id();
					comparisonRplIds.Insert(aiRplId);
					SetIconForEntity(aiCharacter, aiRplId);
				};
			};
		};
		
		//------------------------------------------------------------------------------------------------
		// CLEAR ICONS THAT DONT EXIST
		//------------------------------------------------------------------------------------------------
		
		array<int> indexesToDelete = {};
		
		foreach (RplId rplId : m_aEntityIcons)
		{
			if(!rplId || !rplId.IsValid() || !comparisonRplIds.Contains(rplId))
			{
				int index = m_aEntityIcons.Find(rplId);
				
				if(index != -1)
					indexesToDelete.Insert(index);
			}
		};
		
		foreach (int index : indexesToDelete)
		{
			m_aEntityIcons.RemoveOrdered(index);
			delete m_aSpectatorWidgets.Get(index);
			m_aSpectatorWidgets.RemoveOrdered(index);
			m_aSpectatorIcons.RemoveOrdered(index);
		}
	}
	
	/**
	 * Set the icon for the provided entity
	 * @param entity - Entity to pass along to the icon
	 * @param entityId - EntityId to use to insert into the icon arrays
	 */
	protected void SetIconForEntity(IEntity entity, RplId entityId)
	{
		// Skip if icon already exists
		if (m_aEntityIcons.Contains(entityId))
			return;
		
		// Create new spectator icon
		Widget spectatorIconWidget = GetGame().GetWorkspace().CreateWidgets(
			"{68625BAD23CEE68F}UI/Spectator/SpectatorLabelIconCharacter.layout", 
			FrameWidget.Cast(GetRootWidget().FindAnyWidget("IconsFrame"))
		);
		
		CRF_SpectatorLabelIconCharacter spectatorIcon = CRF_SpectatorLabelIconCharacter.Cast(
			spectatorIconWidget.FindHandler(CRF_SpectatorLabelIconCharacter)
		);
		
		// If the character is alive and not a spectator, let spectators spectate them
		if (CheckIfEntityAlive(entity) && !CRF_GamemodeManager.IsSpectator(entity))
			spectatorIcon.GetButton().m_OnClicked.Insert(SelectSpecCursor);
		
		spectatorIcon.SetEntity(entity, "Spine3");
		
		// Store references to the icon
		m_aEntityIcons.Insert(entityId);
		m_aSpectatorIcons.Insert(spectatorIcon);
		m_aSpectatorWidgets.Insert(spectatorIconWidget);
	};
	
	/**
	 * Check if the provided entity is considered "alive"
	 * @param entity - Entity to check
	 */
	protected bool CheckIfEntityAlive(IEntity entity)
	{
		SCR_CharacterControllerComponent controllerComponent = SCR_CharacterControllerComponent.Cast(entity.FindComponent(SCR_CharacterControllerComponent));
	
		// If the character is a valid character and is not dead then return that this guy ain't dead
		if (controllerComponent && !controllerComponent.IsDead())
			return true;
		else 
			return false;
	}
	
	/**
	 * Update UI panel visibility based on cursor position
	 * @param tDelta - Time since last frame
	 */
	protected void UpdateUIPanelVisibility(float tDelta)
	{
		// Get cursor position
		int x, y;
		WidgetManager.GetMousePos(x, y);
		y = GetGame().GetWorkspace().DPIUnscale(y);
		
		// Get screen size
		float sX, sY;
		m_wRoot.GetScreenSize(sX, sY);
		
		// Update slots panel visibility
		float leftSlotX = FrameSlot.GetPosX(m_wFrameSlots);
		float leftSlotY = FrameSlot.GetPosY(m_wFrameSlots);
		
		if (x <= leftSlotX + 220 && y >= leftSlotY && y <= leftSlotY + 450)
		{
			// Expand slots panel when cursor is over it
			leftSlotX += tDelta * 2400.0;
			if (leftSlotX > 0)
				leftSlotX = 0;
			
			FrameSlot.SetPosX(m_wFrameSlots, leftSlotX);
			m_wRoot.FindAnyWidget("SliderBGL").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowL").SetVisible(false);
		}
		else
		{
			// Collapse slots panel when cursor moves away
			leftSlotX -= tDelta * 2400.0;
			if (leftSlotX < -200)
				leftSlotX = -200;
			
			FrameSlot.SetPosX(m_wFrameSlots, leftSlotX);
			m_wRoot.FindAnyWidget("SliderBGL").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowL").SetVisible(true);
		}
		
		// Update VON channels panel visibility
		float leftVONX = FrameSlot.GetPosX(m_wFrameChannels);
		float leftVONY = FrameSlot.GetPosY(m_wFrameChannels);
		
		if (x >= leftVONX -20 + sX && y >= leftVONY && y <= leftVONY + 450)
		{
			// Expand VON panel when cursor is over it
			leftVONX -= tDelta * 2400.0;
			if (leftVONX < -220)
				leftVONX = -220;
			
			FrameSlot.SetPosX(m_wFrameChannels, leftVONX);
			m_wRoot.FindAnyWidget("SliderBGR").SetVisible(false);
			m_wRoot.FindAnyWidget("ArrowR").SetVisible(false);
		}
		else
		{
			// Collapse VON panel when cursor moves away
			leftVONX += tDelta * 2400.0;
			if (leftVONX > -20)
				leftVONX = -20;
			
			FrameSlot.SetPosX(m_wFrameChannels, leftVONX);
			m_wRoot.FindAnyWidget("SliderBGR").SetVisible(true);
			m_wRoot.FindAnyWidget("ArrowR").SetVisible(true);
		}
	}
	
	/**
	 * Create a new VON channel
	 */
	void CreateChannel()
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		
		// Check if player already has a channel by checking if they're the creator of any channel
		foreach(string channel: m_MenuManager.m_aVONChannels)
		{
			ref array<string> channelSplit = {};
			channel.Split("|", channelSplit, true);
			string channelName = channelSplit.Get(0);
			
			// Check if this player created this channel by looking for their ID in the channel name
			string expectedChannelName = GetGame().GetPlayerManager().GetPlayerName(localPlayerId) + "'s Channel (" + localPlayerId + ")";
			if (channelName == expectedChannelName)
				return;
		}
		
		// Create a new channel
		CRF_RplToAuthorityManager.GetInstance().CreateChannel(localPlayerId);
		
		// Schedule radio frequency update after channel creation
		// Use a longer delay to allow server replication and channel assignment to complete
		if (!CVON_VONGameModeComponent.GetInstance())
			GetGame().GetCallqueue().CallLater(UpdateRadioFrequency, 500, false);
	}
	
	/**
	 * Toggle UI visibility
	 */
	void HideUI()
	{
		SCR_HUDManagerComponent hudManager = SCR_HUDManagerComponent.Cast(
			SCR_PlayerController.Cast(GetGame().GetPlayerController()).GetHUDManagerComponent()
		);
		
		if (m_wRoot.IsVisible())
		{
			// Hide UI
			m_wRoot.SetVisible(false);
			hudManager.GetHUDRootWidget().SetVisible(false);
		}
		else
		{
			// Show UI
			m_wRoot.SetVisible(true);
			hudManager.GetHUDRootWidget().SetVisible(true);
		}
	}
	
	/**
	 * Updates the Voice Over Network (VON) channels display in the UI
	 * Shows available channels and their members
	 */
	void UpdateChannel()
	{
		// Clear existing channels
		m_wVONChannels.Clear();
		
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		
		// Iterate through all available channels
		foreach(string channelData: m_MenuManager.m_aVONChannels)
		{
			// Parse channel data format: "ChannelName|PlayerID1,PlayerID2,..."
			ref array<string> channelParts = {};
			channelData.Split("|", channelParts, true);
			
			if (channelParts.Count() == 0)
				continue;
				
			string channelName = channelParts.Get(0);
			
			// Add channel to the list
			int channelIndex = m_wVONChannels.AddItemChannel(null, channelName);
			CRF_ListBoxElementComponent channelComponent = m_wVONChannels.GetCRFElementComponent(channelIndex);
			
			if (!channelComponent)
				continue;
				
			// Store channel ID and register join button click handler
			channelComponent.m_iChannelId = m_MenuManager.m_aVONChannels.Find(channelData);
			channelComponent.GetChannelButton().m_OnClicked.Insert(JoinChannelDelay);
			
			// If channel has no players, continue to next channel
			if (channelParts.Count() <= 1)
				continue;
				
			// Parse player IDs in the channel
			ref array<string> playerIds = {};
			channelParts.Get(1).Split(",", playerIds, true);
			
			// Add each player in the channel to the display
			foreach(string playerIdStr: playerIds)
			{
				// Skip invalid player IDs
				if (playerIdStr.IsEmpty())
					continue;
					
				int playerId = playerIdStr.ToInt();
				
				// Skip disconnected players
				if (!playerManager.IsPlayerConnected(playerId))
					continue;
					
				// In "Deafen" channel, only show the local player
				if (playerId != SCR_PlayerController.GetLocalPlayerId() && channelName == "Deafen")
					continue;
				
				// Only show dead players in spectator VON channels
				CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
				if (slottingManager)
				{
					CRF_SlotDataContainer playerSlotData = slottingManager.GetPlayerSlotData(playerId);
					if (playerSlotData && !playerSlotData.GetIsDeadSlot())
						continue; // Skip alive players
				}
					
				// Add player to the channel display
				int playerIndex = m_wVONChannels.AddItem(
					playerManager.GetPlayerName(playerId), 
					null, 
					"{68D74FF57296AFFB}UI/Listbox/PlayerListboxElementVON.layout"
				);
				
				CRF_ListBoxElementComponent playerComponent = m_wVONChannels.GetCRFElementComponent(playerIndex);
				if (playerComponent)
				{
					playerComponent.m_iPlayerId = SCR_PlayerController.GetLocalPlayerId();
					playerComponent.m_bIsPlayer = true;
				}
			}
		}
		
		// Update local channel counter to match server state
		m_iLocalChannelUpdates = m_MenuManager.m_iChannelChanges;
		
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			// Toggle radio power based on whether player is in a channel
			int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
			bool isInChannel = m_MenuManager.GetChannel(localPlayerId) != 0;
			SetRadioPower(isInChannel);
	
			// Update radio frequency to match current channel assignment
			// This ensures the radio frequency is correct after channel changes
			if (isInChannel)
			{
				// Schedule frequency update after a small delay to ensure replication is complete
				GetGame().GetCallqueue().CallLater(UpdateRadioFrequency, 100, false);
			}
		}
	}
	
	/**
	 * Updates the radio frequency to match the current channel assignment
	 * Called after channel changes to ensure proper frequency synchronization
	 */
	protected void UpdateRadioFrequency()
	{
		// Get the current transceiver and update its frequency
		RadioTransceiver transceiver = GetVoNTransiver();
		// The GetVoNTransiver() call automatically sets the correct frequency
		// No additional work needed here as the frequency is set in that method
	}
	
	/**
	 * Schedules channel join with a short delay to prevent UI issues
	 */
	void JoinChannelDelay()
	{
		GetGame().GetCallqueue().Call(JoinChannel);
	}
	
	/**
	 * Joins the currently selected VON channel
	 */
	void JoinChannel()
	{
		// Get the selected channel component
		CRF_ListBoxElementComponent selectedComponent = m_wVONChannels.GetCRFElementComponent(m_wVONChannels.GetSelectedItem());
		if (!selectedComponent)
			return;
		
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int channelId = selectedComponent.m_iChannelId;
		
		// Handle channel join through appropriate manager based on channel type
		if (channelId > 1)
		{
			// Request to join non-default channel via RPC
			Print(string.Format("[VON] Client %1 requesting to join channel %2", localPlayerId, channelId), LogLevel.NORMAL);
			CRF_RplToAuthorityManager.GetInstance().RequestToJoinChannel(channelId, localPlayerId);
		}
		else
		{
			// Join default channel directly
			CRF_RplToAuthorityManager.GetInstance().JoinChannel(localPlayerId, channelId);
		}
		
		if (!CVON_VONGameModeComponent.GetInstance())
		{
			// Schedule radio frequency update after channel join
			// Use a delay to allow server replication to complete
			GetGame().GetCallqueue().CallLater(UpdateRadioFrequency, 200, false);
		}
	}
	
	/**
	 * Selects an entity to spectate based on cursor position
	 */
	void SelectSpecCursor()
	{
		// Get widget under cursor
		Widget cursorWidget = WidgetManager.GetWidgetUnderCursor();
		if (!cursorWidget)
			return;
			
		// Get parent widget
		Widget parentWidget = cursorWidget.GetParent();
		if (!parentWidget)
			return;
			
		// Find spectator icon handler
		CRF_SpectatorLabelIconCharacter iconHandler = CRF_SpectatorLabelIconCharacter.Cast(
			parentWidget.FindHandler(CRF_SpectatorLabelIconCharacter)
		);
		
		if (!iconHandler)
			return;
		
		if(iconHandler.m_eEntity)
			m_eSpecEntity = iconHandler.m_eEntity;
	}
	
	/**
	 * Selects BLUFOR faction for display
	 */
	void SelectFactionBlufor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("BLUFOR");
		UpdateSlots();
	}
	
	/**
	 * Selects OPFOR faction for display
	 */
	void SelectFactionOpfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("OPFOR");
		UpdateSlots();
	}
	
	/**
	 * Selects INDFOR faction for display
	 */
	void SelectFactionIndfor()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("INDFOR");
		UpdateSlots();
	}
	
	/**
	 * Selects Civilian faction for display
	 */
	void SelectFactionCiv()
	{
		m_fSelectedFaction = GetGame().GetFactionManager().GetFactionByKey("CIV");
		UpdateSlots();
	}
	
	/**
	 * Initializes slot counters for all factions
	 * Counts total and alive slots for each faction
	 */
	void InitSlots()
	{
		// Get all slots from the slotting manager
		map<int, CRF_SlotDataContainer> slotMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		
		// Process each slot to count by faction
		foreach (int slotId, CRF_SlotDataContainer slotData : slotMap)
		{
			// Skip locked or empty slots
			if(slotData.GetIsLockedSlot() || slotData.GetSlotCurrentPlayerId() == 0)
				continue;
			
			// Update counters based on faction
			string factionKey = slotData.GetSlotFactionKey();
			bool isAlive = !slotData.GetIsDeadSlot();
			
			if (factionKey == "BLUFOR")
			{
				m_iBluforSlots++;
				if(isAlive) 
					m_iAliveBluforSlots++;
			}
			else if (factionKey == "OPFOR")
			{
				m_iOpforSlots++;
				if(isAlive) 
					m_iAliveOpforSlots++;
			}
			else if (factionKey == "INDFOR")
			{
				m_iIndforSlots++;
				if(isAlive) 
					m_iAliveIndforSlots++;
			}
			else if (factionKey == "CIV")
			{
				m_iCivSlots++;
				if(isAlive) 
					m_iAliveCivSlots++;
			}
		}
	}
	
	/**
	 * Updates the UI to display slots for the selected faction
	 * Shows faction flags, player counts, and player lists by group
	 */
	void UpdateSlots()
	{
		// Reset faction counters
		m_iBluforSlots = 0;
		m_iOpforSlots = 0;
		m_iIndforSlots = 0;
		m_iCivSlots = 0;
		m_iAliveBluforSlots = 0;
		m_iAliveOpforSlots = 0;
		m_iAliveIndforSlots = 0;
		m_iAliveCivSlots = 0;
		
		// Clear player slots list
		m_wPlayerSlots.Clear();
		
		// Get required managers
		SCR_GroupsManagerComponent groupManager = SCR_GroupsManagerComponent.GetInstance();
		CRF_GearscriptManager gearscriptManager = CRF_GearscriptManager.GetInstance();
		
		// Initialize slot counters
		InitSlots();
		
		// Update faction UI elements
		UpdateFactionUI("BLUFOR", gearscriptManager, m_wRoot.FindAnyWidget("BLUButton"), 
			m_wRoot.FindAnyWidget("BLUFlag"), m_wRoot.FindAnyWidget("BLURatio"), 
			m_iAliveBluforSlots, m_iBluforSlots);
			
		UpdateFactionUI("OPFOR", gearscriptManager, m_wRoot.FindAnyWidget("OPFButton"), 
			m_wRoot.FindAnyWidget("OPFFlag"), m_wRoot.FindAnyWidget("OPFRatio"), 
			m_iAliveOpforSlots, m_iOpforSlots);
			
		UpdateFactionUI("INDFOR", gearscriptManager, m_wRoot.FindAnyWidget("INDButton"), 
			m_wRoot.FindAnyWidget("INDFlag"), m_wRoot.FindAnyWidget("INDRatio"), 
			m_iAliveIndforSlots, m_iIndforSlots);
			
		UpdateFactionUI("CIV", gearscriptManager, m_wRoot.FindAnyWidget("CIVButton"), 
			m_wRoot.FindAnyWidget("CIVFlag"), m_wRoot.FindAnyWidget("CIVRatio"), 
			m_iAliveCivSlots, m_iCivSlots);
		
		// Get slot and group data
		map<int, CRF_SlotDataContainer> slotMap = CRF_SlottingManager.GetInstance().GetSlotMap();
		array<SCR_AIGroup> factionGroups = CRF_SlottingManager.GetInstance().GetAllGroups(m_fSelectedFaction.GetFactionKey());
		
		if (factionGroups.IsEmpty())
			return;
		
		// Process each group and its players
		foreach(SCR_AIGroup group : factionGroups)
		{	
			// Skip private groups
			if(group.IsPrivate())
				continue;
			
			int playersInGroup = 0;
			int deadPlayersInGroup = 0;
			
			// Add group to the player slots UI
			int groupIndex = m_wPlayerSlots.AddItemSpecGroup(null, group);
			CRF_ListBoxElementComponent groupComponent = m_wPlayerSlots.GetCRFElementComponent(groupIndex);
			
			if (groupComponent)
			{
				// Set group colors and icon
				Color factionColor = group.GetFaction().GetFactionColor();
				groupComponent.GetGroupUnderline().SetColor(factionColor);
				
				if(group.GetFaction().GetFactionKey() == "INDFOR")
					groupComponent.GetGroupIcon().SetColor(factionColor);
				
				groupComponent.GetGroupIcon().LoadImageFromSet(0, SCR_Faction.Cast(group.GetFaction()).GetGroupFlagImageSet(), group.GetGroupFlag());
			}
			
			// Get group ID
			RplId groupId = 0;
			RplComponent groupRplComp = RplComponent.Cast(group.FindComponent(RplComponent));
			if (groupRplComp)
			{
				groupId = groupRplComp.Id();
			}
			
			// Process all slots in this group
			foreach(int slotId, CRF_SlotDataContainer slotData : slotMap)
			{	
				// Skip slots that don't belong to this group/faction
				if (slotData.GetSlotCurrentGroup() != groupId || 
					slotData.GetIsLockedSlot() || 
					slotData.GetSlotCurrentPlayerId() == 0 || 
					GetGame().GetFactionManager().GetFactionByKey(slotData.GetSlotFactionKey()) != m_fSelectedFaction)
					continue;
				
				// Count dead players
				if (slotData.GetIsDeadSlot())
				{
					deadPlayersInGroup++;
					continue;
				}
				
				// Skip locked slots
				if(slotData.GetIsLockedSlot() && slotData.GetSlotCurrentPlayerId() <= 0)
					continue;
				
				// Add slot to the UI
				int slotIndex = m_wPlayerSlots.AddItemSpecSlot(null, slotId);
				CRF_ListBoxElementComponent slotComponent = m_wPlayerSlots.GetCRFElementComponent(slotIndex);
				
				// Count occupied slots
				if (slotData.GetSlotCurrentPlayerId() > 0)
					playersInGroup++;
				
				// Add click handler for spectating
				if (slotComponent && slotComponent.GetSlotButton())
				{
					slotComponent.GetSlotButton().m_OnClicked.Insert(SelectSpecDelay);
				}
				
				// Set player name if slot is occupied
				if (slotData.GetSlotCurrentPlayerId() > 0 && slotComponent)
				{
					PlayerManager playerManager = GetGame().GetPlayerManager();
					if (playerManager)
					{
						slotComponent.SetPlayerText(playerManager.GetPlayerName(slotData.GetSlotCurrentPlayerId()));
						
						// Show disconnected indicator if player is no longer connected
						if (!playerManager.IsPlayerConnected(slotData.GetSlotCurrentPlayerId()))
						{
							slotComponent.GetDisconnectWidget().SetVisible(true);
						}
					}
				}
			}
			
			// Remove empty groups
			if (playersInGroup == 0)
			{
				m_wPlayerSlots.RemoveItem(groupIndex);
			}
		}
	}
	
	/**
	 * Helper method to update a single faction's UI elements
	 * @param factionKey - The faction key (e.g., "BLUFOR", "OPFOR")
	 * @param gearscriptManager - Reference to the gearscript manager
	 * @param buttonWidget - Button widget for this faction
	 * @param flagWidget - Flag image widget for this faction
	 * @param ratioWidget - Text widget showing player count ratio
	 * @param aliveCount - Number of alive players in faction
	 * @param totalCount - Total number of players in faction
	 */
	protected void UpdateFactionUI(string factionKey, CRF_GearscriptManager gearscriptManager, 
		Widget buttonWidget, Widget flagWidget, Widget ratioWidget, int aliveCount, int totalCount)
	{
		// Skip if faction is not valid
		if (!CRF_SlottingManager.GetInstance().IsFactionValid(factionKey))
			return;
			
		// Show faction button
		if (buttonWidget)
			buttonWidget.SetVisible(true);
		
		// Determine faction icon
		ResourceName iconPath;
		
		// Try to get icon from gearscript first
		if (gearscriptManager)
		{	
			ResourceName gearScriptResource = gearscriptManager.GetGearScriptResource(factionKey);
			if (!gearScriptResource.IsEmpty())
			{
				CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(
					BaseContainerTools.CreateInstanceFromContainer(
						BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()
					)
				);
				
				if (gearConfig && !gearConfig.m_FactionIcon.IsEmpty())
				{
					iconPath = gearConfig.m_FactionIcon;
				}
			}
		}
		
		// Fallback to default faction flag
		if (iconPath.IsEmpty())
		{
			SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
			if (faction)
				iconPath = faction.GetFactionFlag();
		}
		
		// Set flag image and player count ratio
		ImageWidget flagImage = ImageWidget.Cast(flagWidget);
		if (flagImage && !iconPath.IsEmpty())
			flagImage.LoadImageTexture(0, iconPath);
			
		TextWidget ratioText = TextWidget.Cast(ratioWidget);
		if (ratioText)
			ratioText.SetText(aliveCount.ToString() + "/" + totalCount.ToString());
	}
	
	/**
	 * Schedules the spectator selection with a small delay
	 * This prevents potential UI issues from immediate execution
	 */
	void SelectSpecDelay()
	{
		// Use a short delay to allow UI to update before selection occurs
		GetGame().GetCallqueue().Call(SelectSpec);
	}
	
	/**
	 * Selects an entity to spectate based on the currently selected slot in the player list
	 * This allows the player to view the game from another player's perspective
	 */
	void SelectSpec()
	{
		// Get the component for the selected item in the player slots list
		CRF_ListBoxElementComponent selectedComponent = CRF_ListBoxElementComponent.Cast(
			m_wPlayerSlots.GetElementComponent(m_wPlayerSlots.GetSelectedItem())
		);
		
		// Return if no valid slot is selected
		if (!selectedComponent || selectedComponent.m_iSlotId == 0)
			return;
		
		// Get slot data from the slotting manager
		CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotData(selectedComponent.m_iSlotId);
		if (!slotData)
			return;
		
		// Find the entity associated with the slot and set it as the spectator target
		RplComponent rplComponent = RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentCharacter()));
		if (rplComponent)
		{
			m_eSpecEntity = rplComponent.GetEntity();
		}
	}
	
	/**
	 * Initializes the menu when it's first created
	 * Sets up root widget and map references
	 */
	override void OnMenuInit()
	{		
		// Call parent class initialization
		super.OnMenuInit();

		// Store reference to the root widget
		m_wRoot = GetRootWidget();
		
		// Initialize map entity if not already done
		if (!m_MapEntity)
		{
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		}
		
		// Hide the map frame initially
		Widget mapFrame = m_wRoot.FindAnyWidget("MapFrame");
		if (mapFrame)
		{
			mapFrame.SetVisible(false);
		}
	}
	
	/**
	 * Performs cleanup when the menu is closed
	 * Unregisters event handlers and resets game state
	 */
	override void OnMenuClose()
	{
		// Call parent class cleanup
		super.OnMenuClose();
		
		GetGame().GetCallqueue().Remove(UpdatePlayerIcons);
		
		// Unregister spectator camera frame event
		UnregisterFrameEvent();
		
		// Unregister from slotting updates
		CRF_SlottingManager slottingManager = CRF_SlottingManager.GetInstance();
		if (slottingManager)
		{
			slottingManager.GetOnSlottingUpdate().Remove(UpdateSlots);
		}
		
		// Remove all action listeners
		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			if (!CVON_VONGameModeComponent.GetInstance())
			{
				inputManager.RemoveActionListener("VONDirect", EActionTrigger.DOWN, Action_VONon);
				inputManager.RemoveActionListener("VONDirect", EActionTrigger.UP, Action_VONOff);
			}
			inputManager.RemoveActionListener("ChatToggle", EActionTrigger.DOWN, Action_OnChatToggleAction);
			inputManager.RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);
			inputManager.RemoveActionListener("GadgetMap", EActionTrigger.DOWN, Action_ToggleMap);
			inputManager.RemoveActionListener("ManualCameraTeleport", EActionTrigger.DOWN, Action_ManualCameraTeleport);
			inputManager.RemoveActionListener("ShowScoreboard", EActionTrigger.DOWN, OnShowPlayerList);
			inputManager.RemoveActionListener("EditorToggleUI", EActionTrigger.DOWN, HideUI);
			inputManager.RemoveActionListener("CRF_SpecNVG", EActionTrigger.DOWN, ToggleNVGs);
		}
		
		ForceNVGsOff();
		
		// Restore workspace opacity if it was set to 0
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace && workspace.GetOpacity() == 0)
		{
			workspace.SetOpacity(1);
		}
		
		// Reset kill feed type to default
		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(
			GetGame().GetGameMode().FindComponent(SCR_NotificationSenderComponent)
		);
		if (sender)
			sender.SetKillFeedTypeNoneLocal();
	}
	
	/**
	 * Opens the player list when the scoreboard key is pressed
	 */
	protected static void OnShowPlayerList()
	{
		ArmaReforgerScripted.OpenPlayerList();
	}
	
	/**
	 * Teleports the camera to the position under the cursor
	 * Triggered by the manual camera teleport action
	 */
	void Action_ManualCameraTeleport()
	{
		vector worldPosition = GetCursorWorldPos();
		if (worldPosition != vector.Zero)
		{
			MoveCamera(worldPosition);
		}
	}
	
	/**
	 * Moves the camera to the specified world position
	 * 
	 * @param worldPosition - The target position in world coordinates
	 */
	void MoveCamera(vector worldPosition)
	{
		// Get the current camera
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (!camera)
			return;
		
		// Find the teleport component and use it to move the camera
		SCR_TeleportToCursorManualCameraComponent teleportComponent = SCR_TeleportToCursorManualCameraComponent.Cast(
			camera.FindCameraComponent(SCR_TeleportToCursorManualCameraComponent)
		);
		
		if (teleportComponent)
		{
			teleportComponent.TeleportCamera(worldPosition, true, false);
		}
	}
	
	/**
	 * Calculates the world position corresponding to the cursor position
	 * Handles both map cursor and 3D world cursor positions
	 * 
	 * @return The world position vector under the cursor
	 */
	vector GetCursorWorldPos()
	{
		ArmaReforgerScripted game = GetGame();
		WorkspaceWidget workspace = game.GetWorkspace();
		BaseWorld world = game.GetWorld();
		
		vector worldPos = vector.Zero;
		
		// If map is open, get position from map cursor
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			float worldX, worldY;
			mapEntity.GetMapCursorWorldPosition(worldX, worldY);
			
			worldPos[0] = worldX;
			worldPos[2] = worldY;
			
			// Get the terrain height at this position
			if (world)
			{
				worldPos[1] = world.GetSurfaceY(worldPos[0], worldPos[2]);
			}
			
			return worldPos;
		}
		
		// If map is not open, perform a raycast from the camera through the cursor
		vector cursorPos = GetCursorPos();
		vector outDir;
		
		// Project from screen to world
		vector startPos = workspace.ProjScreenToWorld(cursorPos[0], cursorPos[1], outDir, world, -1);
		outDir *= 1000.0; // Extend ray to ensure it hits something
	
		// Set up trace parameters
		TraceParam trace = new TraceParam();
		trace.Start = startPos;
		trace.End = startPos + outDir;
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS | TraceFlags.OCEAN; 
		
		// Perform the trace
		float traceDis = 0;
		if (world)
		{
			traceDis = world.TraceMove(trace, null);
		}
		
		worldPos = startPos + outDir * traceDis;
		return worldPos;
	}
	
	/**
	 * Gets the current cursor position in screen coordinates
	 * 
	 * @return The cursor position vector (x, y, 0)
	 */
	vector GetCursorPos()
	{
		InputManager inputManager = GetGame().GetInputManager();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		
		// For mouse and keyboard, return actual cursor position
		if (inputManager && inputManager.IsUsingMouseAndKeyboard())
		{
			int cursorX, cursorY;
			WidgetManager.GetMousePos(cursorX, cursorY);
			
			float scaledX = workspace.DPIUnscale(cursorX);
			float scaledY = workspace.DPIUnscale(cursorY);
			
			return Vector(scaledX, scaledY, 0);
		} 
		else 
		{
			// For gamepad or other input methods, use screen center
			float centerX = workspace.DPIUnscale(workspace.GetWidth() / 2.0);
			float centerY = workspace.DPIUnscale(workspace.GetHeight() / 2.0);
			
			return Vector(centerX, centerY, 0);
		}
	}
	
	//=================================================================================================
	// RADIO AND VOICE COMMUNICATION METHODS
	//=================================================================================================

	/**
	 * Retrieves the player's radio transceiver and configures it for voice communication
	 * @return The configured RadioTransceiver object
	 */
	RadioTransceiver GetVoNTransiver()
	{
		// Get local player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return null;

		// Get all items in player's inventory
		ref array<IEntity> inventoryItems = {};
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(
			playerEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (!inventoryManager)
			return null;

		inventoryManager.GetItems(inventoryItems);

		// Find the radio entity in inventory
		IEntity radioEntity = null;
		foreach (IEntity item : inventoryItems)
		{
			if (item && item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return null;

		// Get radio component and power it on
		BaseRadioComponent radioComponent = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		if (!radioComponent)
			return null;

		radioComponent.SetPower(true);

		// Get transceiver and set frequency based on channel
		RadioTransceiver transceiver = RadioTransceiver.Cast(radioComponent.GetTransceiver(0));
		if (!transceiver)
			return null;

		// Get the current player's channel with improved frequency calculation
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		int playerChannelId = CRF_MenuManager.GetInstance().GetChannel(localPlayerId);

		// Calculate unique frequency for the channel to prevent conflicts
		// Use a base frequency of 10000 + (channelId * 1000) to ensure separation
		// This prevents frequency collisions between different channels
		float frequency = 10000.0 + (playerChannelId * 1000.0);

		// For custom channels (ID > 1), add additional offset based on channel name hash
		// This ensures each custom channel gets a truly unique frequency
		if (playerChannelId > 1 && m_MenuManager.m_aVONChannels.IsIndexValid(playerChannelId))
		{
			string channelData = m_MenuManager.m_aVONChannels[playerChannelId];
			ref array<string> channelParts = {};
			channelData.Split("|", channelParts, true);

			if (channelParts.Count() > 0)
			{
				string channelName = channelParts[0];
				// Use channel name hash to create unique frequency offset
				int nameHash = channelName.Hash();
				// Ensure positive hash and limit range to prevent frequency overlap
				int frequencyOffset = Math.AbsInt(nameHash) % 500;
				frequency += frequencyOffset;
			}
		}

		// Set the radio frequency using RadioHandlerComponent for proper replication
		RadioHandlerComponent radioHandler = RadioHandlerComponent.Cast(
			GetGame().GetPlayerController().FindComponent(RadioHandlerComponent)
		);

		if (radioHandler)
		{
			radioHandler.SetFrequency(transceiver, frequency);
		}

		return transceiver;
	}

	/**
	 * Sets the power state of the player's radio
	 * @param input - true to power on, false to power off
	 */
	void SetRadioPower(bool input)
	{
		// Get local player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return;

		// Get inventory manager
		SCR_InventoryStorageManagerComponent inventoryManager = SCR_InventoryStorageManagerComponent.Cast(
			playerEntity.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		if (!inventoryManager)
			return;

		// Get all inventory items
		ref array<IEntity> inventoryItems = {};
		inventoryManager.GetItems(inventoryItems);

		// Find radio in inventory
		IEntity radioEntity = null;
		foreach (IEntity item : inventoryItems)
		{
			if (item && item.FindComponent(BaseRadioComponent))
			{
				radioEntity = item;
				break;
			}
		}

		if (!radioEntity)
			return;

		// Set radio power state
		BaseRadioComponent radioComponent = BaseRadioComponent.Cast(radioEntity.FindComponent(BaseRadioComponent));
		if (radioComponent)
		{
			radioComponent.SetPower(input);
		}
	}

	/**
	 * Activates voice transmission when PTT key is pressed
	 * Connects to the appropriate radio channel
	 */
	void Action_VONon()
	{
		// Check if player is in a valid channel
		int playerChannel = CRF_MenuManager.GetInstance().GetChannel(SCR_PlayerController.GetLocalPlayerId());
		if (playerChannel == 0)
			return;

		// Cancel any pending VoN disable calls
		GetGame().GetCallqueue().Remove(LobbyVoNDisableDelayed);

		// Get VoN component from player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return;

		SCR_VoNComponent vonComponent = SCR_VoNComponent.Cast(playerEntity.FindComponent(SCR_VoNComponent));
		if (!vonComponent)
			return;

		// Configure and activate voice transmission
		// Get fresh transceiver with updated frequency each time VON is activated
		RadioTransceiver transceiver = GetVoNTransiver();
		if (!transceiver)
			return;

		vonComponent.SetTransmitRadio(transceiver);
		vonComponent.SetCommMethod(ECommMethod.SQUAD_RADIO);
		vonComponent.SetCapture(true);
	}

	/**
	 * Deactivates voice transmission when PTT key is released
	 * Uses a delay to prevent audio cutoff
	 */
	void Action_VONOff()
	{
		// Check if player is in a valid channel
		int playerChannel = CRF_MenuManager.GetInstance().GetChannel(SCR_PlayerController.GetLocalPlayerId());
		if (playerChannel == 0)
			return;

		// Schedule delayed VoN deactivation to prevent audio cutoff
		GetGame().GetCallqueue().Call(LobbyVoNDisableDelayed);
	}

	/**
	 * Delayed method to disable voice transmission
	 * Used to prevent audio cutoff when releasing PTT key
	 */
	void LobbyVoNDisableDelayed()
	{
		// Get VoN component from player entity
		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();
		if (!playerEntity)
			return;

		SCR_VoNComponent vonComponent = SCR_VoNComponent.Cast(playerEntity.FindComponent(SCR_VoNComponent));
		if (!vonComponent)
			return;

		// Reset communication method and stop capturing
		vonComponent.SetCommMethod(ECommMethod.DIRECT);
		vonComponent.SetCapture(false);
	}
	
	//=================================================================================================
	// UI UPDATE METHODS
	//=================================================================================================
	
	/**
	 * Updates all spectator icons on the screen
	 * Refreshes position, visibility, and status of player markers
	 */
	void UpdateIcons()
	{
		if (!m_aSpectatorIcons)
			return;
			
		// Update each spectator icon
		foreach (CRF_SpectatorLabelIconCharacter spectatorIcon : m_aSpectatorIcons)
		{
			if (spectatorIcon)
			{
				spectatorIcon.Update();
			}
		}
	}
	
	/**
	 * Handles the chat toggle action
	 * Opens the chat panel with a small delay to prevent UI issues
	 */
	void Action_OnChatToggleAction()
	{
		if (!m_ChatPanel)
			return;
		
		// Use a small delay to ensure UI is ready
		GetGame().GetCallqueue().Call(OpenChatWrap);
	}
	
	/**
	 * Opens the chat panel if it's currently closed
	 */
	void OpenChatWrap()
	{
		if (!m_ChatPanel)
			return;
			
		if (!m_ChatPanel.IsOpen())
		{
			SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
			if (chatManager)
			{
				chatManager.OpenChatPanel(m_ChatPanel);
			}
		}
	}
	
	/**
	 * Handles the exit action
	 * Opens pause menu instead of directly exiting game to prevent accidental exits
	 */
	void Action_Exit()
	{
		// Note: Opening pause menu instead of directly exiting the game
		// because players often accidentally exit the game
		GetGame().GetCallqueue().Call(OpenPauseMenuWrap);
	}
	
	/**
	 * Opens the pause menu
	 */
	void OpenPauseMenuWrap()
	{
		ArmaReforgerScripted.OpenPauseMenu();
	}
	
	//=================================================================================================
	// MAP CONTROL METHODS
	//=================================================================================================
	
	/**
	 * Toggles the map display on/off
	 */
	void Action_ToggleMap()
	{
		if (!m_MapEntity)
			return;
			
		if (!m_MapEntity.IsOpen())
		{
			OpenMap();
		}
		else
		{
			CloseMap();
		}
	}
	
	/**
	 * Opens the map and configures it for display
	 * Disables camera input while the map is open
	 */
	void OpenMap()
	{
		// Disable camera input while map is open
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			camera.SetInputEnabled(false);
		}
		
		// Show map frame
		if (m_wMapFrame)
		{
			m_wMapFrame.SetVisible(true);
		}
		
		// Clear any tool menu widgets
		Widget toolMenu = GetRootWidget().FindAnyWidget("ToolMenuHoriz");
		if (toolMenu)
		{
			SCR_WidgetHelper.RemoveAllChildren(toolMenu);
		}
		
		// Get map configuration from game mode
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;
		
		SCR_MapConfigComponent configComponent = SCR_MapConfigComponent.Cast(
			gameMode.FindComponent(SCR_MapConfigComponent)
		);
		
		if (!configComponent || !m_MapEntity)
			return;
		
		// Configure and open the map
		MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(
			EMapEntityMode.FULLSCREEN, 
			configComponent.GetEditorMapConfig(), 
			m_wMapFrame
		);
		
		m_MapEntity.OpenMap(mapConfig);
	}
	
	/**
	 * Closes the map and re-enables camera input
	 */
	void CloseMap()
	{
		// Re-enable camera input
		SCR_ManualCamera camera = SCR_ManualCamera.Cast(GetGame().GetCameraManager().CurrentCamera());
		if (camera)
		{
			camera.SetInputEnabled(true);
		}
		
		// Hide map frame
		if (m_wMapFrame)
		{
			m_wMapFrame.SetVisible(false);
		}
		
		// Close the map
		if (m_MapEntity)
		{
			m_MapEntity.CloseMap();
		}
	}
	
	//-------------------------------------------------------------------------
	// Timer Update - Called every second
	//-------------------------------------------------------------------------
	void UpdateTimer()
	{	
		// Get current mission time
		m_sServerWorldTime = CRF_GamemodeManager.GetInstance().GetServerWorldTime();
		
		// Skip update if in safestart, time is empty, or hasn't changed
		if (m_sServerWorldTime == "N/A" ||
			m_SafestartManager.GetSafestartStatus() || 
			m_sServerWorldTime.IsEmpty() || 
			m_sStoredServerWorldTime == m_sServerWorldTime) 
		{
			return;
		}
		
		// Store time for comparison in next update
		m_sStoredServerWorldTime = m_sServerWorldTime;
		
		// Handle time warnings (15min, 5min, end)
		HandleTimeWarnings();
		
		// Format and display time remaining
		UpdateTimeDisplay();
	}
	
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------
	
	/**
	* Handles time warnings at specific thresholds
	*/
	protected void HandleTimeWarnings()
	{
		// Play sound and show notification at specific time thresholds
		if (m_sServerWorldTime == "00:15:00" || 
			m_sServerWorldTime == "00:05:00" || 
			m_sServerWorldTime == "Mission Time Expired!") 
		{
			// Play warning sound
			AudioSystem.PlaySound("{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav");
			
			// Show appropriate message based on time
			if (m_sServerWorldTime == "00:15:00") 
			{
				m_PopUpNotification.PopupMsg("Mission Ends In 15 Minutes!", 10);
			}
			else if (m_sServerWorldTime == "00:05:00") 
			{
				m_PopUpNotification.PopupMsg("Mission Ends In 5 Minutes!", 10);
			}
			else if (m_sServerWorldTime == "Mission Time Expired!") 
			{
				GetGame().GetCallqueue().Remove(UpdateTimer);
				m_PopUpNotification.PopupMsg(m_sServerWorldTime, 10);
				m_wTimer.SetText(m_sServerWorldTime);
				return;
			}
		}
	}
	
	/**
	* Updates the time display including formatting and visibility
	*/
	protected void UpdateTimeDisplay()
	{
		// Split time string into components
		array<string> timeParts = {};
		m_sServerWorldTime.Split(":", timeParts, false);
		
		// Format time display (drop the hour part if it's 00)
		string displayTime = m_sServerWorldTime;
		if (timeParts[0] == "00")
		{
			displayTime = string.Format("%1:%2", timeParts[1], timeParts[2]);
		}
		
		m_wTimer.SetText("Mission End: " + displayTime);
		
		// Set color based on time remaining
		if (timeParts[0] == "00" && timeParts[1].ToInt() < 5)
		{
			// Less than 5 minutes - red
			m_wTimer.SetColorInt(ARGB(255, 200, 65, 65));
		}
		else if (timeParts[0] == "00" && timeParts[1].ToInt() < 15)
		{
			// Less than 15 minutes - yellow
			m_wTimer.SetColorInt(ARGB(255, 230, 230, 0));
		}
		else
		{
			// Normal - light gray
			m_wTimer.SetColorInt(ARGB(255, 215, 215, 215));
		}
	}
} 