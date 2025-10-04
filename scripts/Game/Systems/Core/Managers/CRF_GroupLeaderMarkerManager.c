//------------------------------------------------------------------------------------
// CRF_GroupLeaderMarkerManager: Manager for group leader 3D markers during safestart
// Creates floating text above group leaders showing their group name/callsign
// Only visible during safestart period
//------------------------------------------------------------------------------------

//===================================================================================
// DATA STRUCTURES
//===================================================================================

/**
 * Data structure to hold marker information for a group leader
 */
class CRF_GroupLeaderMarkerData
{
	Widget m_wMarkerRoot;
	TextWidget m_wMarkerText;
	PanelWidget m_wMarkerBackground;
	string m_sGroupName;
	vector m_vWorldPosition;
	bool m_bIsVisible;
	
	void CRF_GroupLeaderMarkerData()
	{
		m_sGroupName = "";
		m_vWorldPosition = vector.Zero;
		m_bIsVisible = false;
	}
	
	void Cleanup()
	{
		if (m_wMarkerRoot)
		{
			m_wMarkerRoot.RemoveFromHierarchy();
			m_wMarkerRoot = null;
		}
		m_wMarkerText = null;
		m_wMarkerBackground = null;
	}
}

[ComponentEditorProps(category: "CRF Component", description: "Manager for group leader 3D markers during safestart")]
class CRF_GroupLeaderMarkerManagerClass: SCR_BaseGameModeComponentClass
{
	
}

class CRF_GroupLeaderMarkerManager: SCR_BaseGameModeComponent
{
	//===================================================================================
	// ATTRIBUTES AND PROPERTIES
	//===================================================================================
	
	[Attribute("2.0", desc: "Height offset above the character (meters)")]
	float m_fHeightOffset;
	
	[Attribute("20.0", desc: "Maximum font size of the text")]
	float m_fMaxTextSize;
	
	[Attribute("12.0", desc: "Minimum font size of the text")]
	float m_fMinTextSize;
	
	[Attribute("300.0", desc: "Maximum distance at which text is visible")]
	float m_fMaxDistance;
	
	[Attribute("5.0", desc: "Minimum distance for maximum size")]
	float m_fMinDistance;
	
	//===================================================================================
	// RUNTIME VARIABLES
	//===================================================================================
	
	// Manager references
	protected CRF_SafestartManager m_SafestartManager;
	protected SCR_GroupsManagerComponent m_GroupsManager;
	protected CRF_RplBroadcastManager m_BroadcastManager;
	
	// Server-side tracking (authority data)
	protected ref map<int, string> m_mServerGroupLeaders = new map<int, string>(); // playerId -> groupName
	protected bool m_bSafestartActive = false;
	
	// Client-side tracking (UI data)
	protected ref map<int, ref CRF_GroupLeaderMarkerData> m_mPlayerMarkers = new map<int, ref CRF_GroupLeaderMarkerData>();
	protected bool m_bIsInitialized = false;
	
	// Update timer
	protected float m_fUpdateTimer = 0.0;
	protected float m_fUpdateInterval = 1.0; // Check every second
	
	protected static CRF_GroupLeaderMarkerManager m_sInstance;
	
	//===================================================================================
	// INITIALIZATION
	//===================================================================================
	
	void CRF_GroupLeaderMarkerManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	/**
	 * Initialize the manager
	 */
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Get manager references
		m_SafestartManager = CRF_SafestartManager.GetInstance();
		m_GroupsManager = SCR_GroupsManagerComponent.GetInstance();
		m_BroadcastManager = CRF_RplBroadcastManager.GetInstance();
		
		if (!m_SafestartManager || !m_GroupsManager)
			return;
		
		// Server-side initialization
		if (Replication.IsServer())
		{
			// Subscribe to safestart changes on server
			if (m_SafestartManager.m_OnSafeStartChange)
				m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChangedServer);
			
			// Initial check for safestart state
			m_bSafestartActive = m_SafestartManager.GetSafestartStatus();
			
			SetEventMask(owner, EntityEvent.FRAME);
		}
		// Client-side initialization
		else
		{
			// Subscribe to safestart changes on client
			if (m_SafestartManager.m_OnSafeStartChange)
				m_SafestartManager.m_OnSafeStartChange.Insert(OnSafestartChangedClient);
			
			SetEventMask(owner, EntityEvent.FRAME);
			
			// Delayed start to ensure everything is initialized
			GetGame().GetCallqueue().CallLater(InitializeClientUI, 3000, false);
		}
	}
	
	/**
	 * Initialize client-side UI elements
	 */
	protected void InitializeClientUI()
	{
		m_bIsInitialized = true;
		
		// Check initial safestart state
		m_bSafestartActive = m_SafestartManager.GetSafestartStatus();
		
		Print(string.Format("[CRF_GroupLeaderMarkerManager] Client initialized - Safestart active: %1", m_bSafestartActive));
		
		if (m_bSafestartActive)
		{
			UpdateGroupLeaderMarkers();
		}
	}
	
	//===================================================================================
	// UPDATE LOOP
	//===================================================================================
	
	/**
	 * Frame update for periodic checks and marker positioning
	 */
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Server-side update logic
		if (Replication.IsServer())
		{
			m_fUpdateTimer += timeSlice;
			if (m_fUpdateTimer >= m_fUpdateInterval)
			{
				m_fUpdateTimer = 0.0;
				UpdateServerGroupLeaderTracking();
			}
			return;
		}
		
		// Client-side update logic
		if (!m_bIsInitialized)
			return;
		
		// Update existing markers every frame for smooth positioning
		UpdateMarkerPositions();
		
		// Periodic update for group leader status
		m_fUpdateTimer += timeSlice;
		if (m_fUpdateTimer >= m_fUpdateInterval)
		{
			m_fUpdateTimer = 0.0;
			
			if (m_bSafestartActive)
				UpdateGroupLeaderMarkers();
		}
	}
	
	//===================================================================================
	// SERVER-SIDE LOGIC
	//===================================================================================
	
	/**
	 * Handle safestart state changes on server
	 * @param safestartActive New safestart state
	 */
	protected void OnSafestartChangedServer(bool safestartActive)
	{
		m_bSafestartActive = safestartActive;
		
		if (safestartActive)
		{
			// Safestart started - update tracking
			UpdateServerGroupLeaderTracking();
		}
		else
		{
			// Safestart ended - clear all markers
			BroadcastClearAllMarkers();
			m_mServerGroupLeaders.Clear();
		}
	}
	
	/**
	 * Update server-side group leader tracking and broadcast changes
	 */
	protected void UpdateServerGroupLeaderTracking()
	{
		if (!m_bSafestartActive || !m_GroupsManager)
			return;
		
		// Track current group leaders
		map<int, string> currentLeaders = new map<int, string>();
		
		// Get all players
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach (int playerId : playerIds)
		{
			if (IsPlayerGroupLeader(playerId))
			{
				string groupName = GetPlayerGroupName(playerId);
				if (!groupName.IsEmpty())
				{
					currentLeaders.Set(playerId, groupName);
				}
			}
		}
		
		// Compare with previous state and broadcast changes
		// Check for new leaders
		foreach (int playerId, string groupName : currentLeaders)
		{
			if (!m_mServerGroupLeaders.Contains(playerId) || m_mServerGroupLeaders.Get(playerId) != groupName)
			{
				// New leader or changed group name
				BroadcastCreateMarker(playerId, groupName);
			}
		}
		
		// Check for removed leaders
		array<int> removedLeaders = {};
		foreach (int playerId, string groupName : m_mServerGroupLeaders)
		{
			if (!currentLeaders.Contains(playerId))
			{
				removedLeaders.Insert(playerId);
			}
		}
		
		foreach (int playerId : removedLeaders)
		{
			BroadcastRemoveMarker(playerId);
		}
		
		// Update server tracking
		m_mServerGroupLeaders = currentLeaders;
	}
	
	/**
	 * Check if a player is a group leader
	 * @param playerId Player ID to check
	 * @return True if player is a group leader
	 */
	protected bool IsPlayerGroupLeader(int playerId)
	{
		// Get player's controlled entity
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return false;
		
		// Check if player is a group leader
		SCR_AIGroup group = m_GroupsManager.GetPlayerGroup(playerId);
		if (!group)
			return false;
		
		return group.GetLeaderID() == playerId;
	}
	
	/**
	 * Get the group name for a player
	 * @param playerId Player ID
	 * @return Group name or empty string
	 */
	protected string GetPlayerGroupName(int playerId)
	{
		SCR_AIGroup group = m_GroupsManager.GetPlayerGroup(playerId);
		if (!group)
			return "";
		
		// Get group name
		string groupName = group.GetCustomName();
		if (groupName.IsEmpty())
			groupName = group.GetCustomNameWithOriginal();
		
		return groupName;
	}
	
	//===================================================================================
	// BROADCAST METHODS (Server -> Client Communication)
	//===================================================================================
	
	/**
	 * Broadcast creation of a new marker to all clients
	 * @param playerId Player ID for the marker
	 * @param groupName Group name to display
	 */
	protected void BroadcastCreateMarker(int playerId, string groupName)
	{
		if (!m_BroadcastManager)
			return;
		
		m_BroadcastManager.CreateGroupLeaderMarker(playerId, groupName);
	}
	
	/**
	 * Broadcast removal of a marker to all clients
	 * @param playerId Player ID to remove marker for
	 */
	protected void BroadcastRemoveMarker(int playerId)
	{
		if (!m_BroadcastManager)
			return;
		
		m_BroadcastManager.RemoveGroupLeaderMarker(playerId);
	}
	
	/**
	 * Broadcast clearing of all markers to all clients
	 */
	protected void BroadcastClearAllMarkers()
	{
		if (!m_BroadcastManager)
			return;
		
		m_BroadcastManager.ClearAllGroupLeaderMarkers();
	}
	
	/**
	 * Send current marker state to a specific client (for late joiners)
	 * @param playerId Player ID requesting the state
	 */
	void SendCurrentStateToClient(int playerId)
	{
		if (!Replication.IsServer() || !m_BroadcastManager)
			return;
		
		// Send current safestart state and markers to the requesting client
		if (m_bSafestartActive)
		{
			foreach (int leaderPlayerId, string groupName : m_mServerGroupLeaders)
			{
				m_BroadcastManager.CreateGroupLeaderMarkerForPlayer(playerId, leaderPlayerId, groupName);
			}
		}
	}
	
	//===================================================================================
	// CLIENT-SIDE SAFESTART HANDLING
	//===================================================================================
	
	/**
	 * Handle safestart state changes on client
	 * @param safestartActive New safestart state
	 */
	protected void OnSafestartChangedClient(bool safestartActive)
	{
		m_bSafestartActive = safestartActive;
		
		if (safestartActive)
		{
			// Safestart started - add markers to group leaders
			if (m_bIsInitialized)
				UpdateGroupLeaderMarkers();
		}
		else
		{
			// Safestart ended - remove all markers
			RemoveAllMarkers();
		}
	}
	
	/**
	 * Create a marker for a player (client-side RPC handler)
	 * @param playerId Player ID
	 * @param groupName Group name to display
	 */
	void CreateMarkerForPlayerRPC(int playerId, string groupName)
	{
		if (Replication.IsServer())
			return; // Only execute on clients
		
		// Find or create the main HUD root
		Widget hudRoot = GetGame().GetWorkspace().FindWidget("HudRoot");
		if (!hudRoot)
			hudRoot = GetGame().GetWorkspace();
		
		if (!hudRoot)
			return;
		
		// Remove existing marker if present
		if (m_mPlayerMarkers.Contains(playerId))
		{
			RemoveMarkerForPlayer(playerId);
		}
		
		// Create marker data
		CRF_GroupLeaderMarkerData markerData = new CRF_GroupLeaderMarkerData();
		markerData.m_sGroupName = groupName;
		
		// Create marker widget
		markerData.m_wMarkerRoot = CreateMarkerWidget(hudRoot, markerData);
		if (!markerData.m_wMarkerRoot)
		{
			delete markerData;
			return;
		}
		
		// Set marker text and color
		if (markerData.m_wMarkerText)
		{
			markerData.m_wMarkerText.SetText(groupName);
			Color textColor = GetGroupColor(groupName);
			markerData.m_wMarkerText.SetColor(textColor);
		}
		
		// Store marker data
		m_mPlayerMarkers.Set(playerId, markerData);
	}
	
	/**
	 * Remove marker for a player (client-side RPC handler)
	 * @param playerId Player ID
	 */
	void RemoveMarkerForPlayerRPC(int playerId)
	{
		if (Replication.IsServer())
			return; // Only execute on clients
		
		RemoveMarkerForPlayer(playerId);
	}
	
	/**
	 * Clear all markers (client-side RPC handler)
	 */
	void ClearAllMarkersRPC()
	{
		if (Replication.IsServer())
			return; // Only execute on clients
		
		RemoveAllMarkers();
	}
	
	//===================================================================================
	// CLIENT-SIDE MARKER MANAGEMENT
	//===================================================================================
	
	/**
	 * Update positions for all visible markers
	 */
	protected void UpdateMarkerPositions()
	{
		foreach (int playerId, CRF_GroupLeaderMarkerData markerData : m_mPlayerMarkers)
		{
			if (markerData.m_bIsVisible)
				UpdateMarkerDisplay(playerId, markerData);
		}
	}
	
	/**
	 * Update group leader markers based on current state (client-side)
	 */
	protected void UpdateGroupLeaderMarkers()
	{
		if (!m_GroupsManager || !m_bSafestartActive)
		{
			return;
		}
		
		// Get all players
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		// Track which players should have markers
		array<int> currentGroupLeaders = {};
		
		foreach (int playerId : playerIds)
		{
			// Debug player faction info
			string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			string playerFactionName = "None";
			bool isFriendly = false;
			
			if (playerEntity)
			{
				SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playerEntity);
				if (character)
				{
					SCR_Faction playerFaction = SCR_Faction.Cast(character.GetFaction());
					if (playerFaction)
					{
						playerFactionName = playerFaction.GetFactionName();
						isFriendly = IsPlayerFriendly(playerId);
					}
				}
			}
			
			if (ShouldPlayerHaveMarker(playerId))
			{
				currentGroupLeaders.Insert(playerId);
				
				SCR_AIGroup group = m_GroupsManager.GetPlayerGroup(playerId);
				string groupName = "";
				if (group)
				{
					groupName = group.GetCustomName();
					if (groupName.IsEmpty())
						groupName = group.GetCustomNameWithOriginal();
				}
				
				Print(string.Format("[CRF_GroupLeaderMarkerManager] Creating marker for %1 (Group: %2, Faction: %3, Friendly: %4)", 
					playerName, groupName, playerFactionName, isFriendly));
				
				// Add marker if not already present
				if (!m_mPlayerMarkers.Contains(playerId))
				{
					CreateMarkerForPlayer(playerId);
				}
			}
		}
		
		// Remove markers from players who shouldn't have them anymore
		array<int> playersToRemove = {};
		foreach (int playerId, CRF_GroupLeaderMarkerData markerData : m_mPlayerMarkers)
		{
			if (currentGroupLeaders.Find(playerId) == -1)
			{
				playersToRemove.Insert(playerId);
			}
		}
		
		foreach (int playerId : playersToRemove)
		{
			RemoveMarkerForPlayer(playerId);
		}
	}
	
	/**
	 * Check if a player should have a group leader marker (client-side)
	 * @param playerId Player ID to check
	 * @return True if player should have a marker
	 */
	protected bool ShouldPlayerHaveMarker(int playerId)
	{
		if (!m_bSafestartActive)
			return false;
		
		// Don't show marker for local player
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (playerId == localPlayerId)
			return false;
		
		// Get player's controlled entity
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return false;
		
		// Check if player is a group leader
		SCR_AIGroup group = m_GroupsManager.GetPlayerGroup(playerId);
		if (!group)
			return false;
		
		if (group.GetLeaderID() != playerId)
			return false;
		
		// Only show markers for friendly factions
		if (!IsPlayerFriendly(playerId))
			return false;
		
		return true;
	}
	
	/**
	 * Check if a player belongs to a friendly faction
	 * @param playerId Player ID to check
	 * @return True if player is friendly to local player
	 */
	protected bool IsPlayerFriendly(int playerId)
	{
		// Get local player's faction
		SCR_Faction localFaction = SCR_Faction.Cast(SCR_FactionManager.SGetLocalPlayerFaction());
		if (!localFaction)
		{
			// If we can't determine local faction, default to showing no markers for safety
			return false;
		}
		
		// Get target player's faction
		IEntity targetPlayerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!targetPlayerEntity)
			return false;
		
		SCR_ChimeraCharacter targetCharacter = SCR_ChimeraCharacter.Cast(targetPlayerEntity);
		if (!targetCharacter)
			return false;
		
		SCR_Faction targetFaction = SCR_Faction.Cast(targetCharacter.GetFaction());
		if (!targetFaction)
		{
			// If target has no faction, don't show markers for safety
			return false;
		}
		
		// Check if factions are the same
		if (localFaction == targetFaction)
			return true;
		
		// For this framework, we'll only show markers for the exact same faction
		// This ensures operational security by not revealing friendly command structure
		return false;
	}
	
	/**
	 * Create a new marker for a player (client-side)
	 * @param playerId Player ID
	 */
	protected void CreateMarkerForPlayer(int playerId)
	{
		// Check if marker already exists
		if (m_mPlayerMarkers.Contains(playerId))
		{
			UpdateMarkerForPlayer(playerId);
			return;
		}
		
		// Find or create the main HUD root
		Widget hudRoot = GetGame().GetWorkspace().FindWidget("HudRoot");
		if (!hudRoot)
			hudRoot = GetGame().GetWorkspace();
		
		if (!hudRoot)
			return;
		
		// Create marker data
		CRF_GroupLeaderMarkerData markerData = new CRF_GroupLeaderMarkerData();
		
		// Create marker widget
		markerData.m_wMarkerRoot = CreateMarkerWidget(hudRoot, markerData);
		if (!markerData.m_wMarkerRoot)
		{
			delete markerData;
			return;
		}
		
		// Store marker data
		m_mPlayerMarkers.Set(playerId, markerData);
		
		// Set marker as visible
		markerData.m_bIsVisible = true;
		
		// Update marker content
		UpdateMarkerForPlayer(playerId);
	}
	
	/**
	 * Update marker text and color for a specific player (client-side)
	 * @param playerId Player ID
	 */
	protected void UpdateMarkerForPlayer(int playerId)
	{
		if (!m_mPlayerMarkers.Contains(playerId))
			return;
		
		CRF_GroupLeaderMarkerData markerData = m_mPlayerMarkers.Get(playerId);
		if (!markerData || !markerData.m_wMarkerText)
			return;
		
		// Get player's group
		SCR_AIGroup group = m_GroupsManager.GetPlayerGroup(playerId);
		if (!group)
			return;
		
		// Get group name
		string groupName = group.GetCustomName();
		if (groupName.IsEmpty())
			groupName = group.GetCustomNameWithOriginal();
		
		// Set marker text
		markerData.m_wMarkerText.SetText(groupName);
		
		// Set color based on group name
		Color textColor = GetGroupColor(groupName);
		markerData.m_wMarkerText.SetColor(textColor);
		
		// Store group name for reference
		markerData.m_sGroupName = groupName;
	}
	
	/**
	 * Get color for a group based on its name
	 * @param groupName Group name to check
	 * @return Color for the group
	 */
	protected Color GetGroupColor(string groupName)
	{
		// Convert to uppercase for consistent comparison
		string upperGroupName = groupName;
		upperGroupName.ToUpper();
		
		// Squad-specific colors
		if (upperGroupName == "1-1" || upperGroupName == "2-1")
			return Color.FromRGBA(255, 0, 0, 255);    // Red
		else if (upperGroupName == "1-2" || upperGroupName == "2-2")
			return Color.FromRGBA(0, 0, 255, 255);    // Blue
		else if (upperGroupName == "1-3" || upperGroupName == "2-3")
			return Color.FromRGBA(0, 255, 0, 255);    // Green
		else if (upperGroupName == "1-4" || upperGroupName == "2-4")
			return Color.FromRGBA(128, 0, 128, 255);  // Purple
		else if (upperGroupName == "1-5" || upperGroupName == "2-5")
			return Color.FromRGBA(255, 165, 0, 255);  // Orange
		
		// Check for command/leadership groups (COY, PLT variants)
		if (upperGroupName.Contains("COY") || upperGroupName.Contains("PLT"))
			return Color.FromRGBA(255, 255, 0, 255);  // Yellow
		
		// Default to white for unknown groups
		return Color.FromRGBA(255, 255, 255, 255);    // White
	}
	
	/**
	 * Create the marker widget structure
	 * @param parent Parent widget to attach to
	 * @param markerData Marker data to populate
	 * @return Created marker root widget
	 */
	protected Widget CreateMarkerWidget(Widget parent, CRF_GroupLeaderMarkerData markerData)
	{
		// Create root frame widget
		Widget root = GetGame().GetWorkspace().CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, parent);
		if (!root)
			return null;
		
		// Create background panel for better text readability
		Widget background = GetGame().GetWorkspace().CreateWidget(WidgetType.PanelWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, root);
		if (background)
		{
			markerData.m_wMarkerBackground = PanelWidget.Cast(background);
			if (markerData.m_wMarkerBackground)
			{
				// Set background color with transparency
				markerData.m_wMarkerBackground.SetColor(Color.FromRGBA(0, 0, 0, 120)); // Semi-transparent black
				markerData.m_wMarkerBackground.SetOpacity(0.8);
			}
			
			// Position background to fill the root widget
			FrameSlot.SetAnchor(background, 0.0, 0.0);
			FrameSlot.SetAlignment(background, 0.0, 0.0);
		}
		
		// Create text widget
		Widget text = GetGame().GetWorkspace().CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE, NULL, 0, root);
		if (text)
		{
			markerData.m_wMarkerText = TextWidget.Cast(text);
			if (markerData.m_wMarkerText)
			{
				markerData.m_wMarkerText.SetText("");
				markerData.m_wMarkerText.SetColor(Color.White); // Default color, will be overridden
				markerData.m_wMarkerText.SetExactFontSize(16); // Start with a reasonable font size
			}
			
			// Center the text within the root widget
			FrameSlot.SetAnchor(text, 0.5, 0.5);
			FrameSlot.SetAlignment(text, 0.5, 0.5);
		}
		
		// Set initial position off-screen
		FrameSlot.SetPos(root, -10000, -10000);
		root.SetOpacity(0.0); // Start invisible
		root.SetZOrder(50); // Lower z-order to avoid blocking other UI
		
		return root;
	}
	
	/**
	 * Update marker background size to fit text
	 * @param markerData Marker data
	 */
	protected void UpdateMarkerBackgroundSize(CRF_GroupLeaderMarkerData markerData)
	{
		if (!markerData.m_wMarkerBackground || !markerData.m_wMarkerText)
			return;
		
		float textWidth, textHeight;
		markerData.m_wMarkerText.GetTextSize(textWidth, textHeight);
		
		// Add some padding
		float padding = 8.0;
		FrameSlot.SetSize(markerData.m_wMarkerBackground, textWidth + padding, textHeight + padding);
	}
	
	/**
	 * Update marker display position and visibility
	 * @param playerId Player ID
	 * @param markerData Marker data
	 */
	protected void UpdateMarkerDisplay(int playerId, CRF_GroupLeaderMarkerData markerData)
	{
		// Get player entity
		IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!playerEntity)
		{
			markerData.m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		// Hide marker if map is open
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (mapEntity && mapEntity.IsOpen())
		{
			markerData.m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		// Update world position
		UpdateWorldPosition(playerEntity, markerData);
		
		// Project 3D world position to 2D screen coordinates
		vector screenPosition = GetGame().GetWorkspace().ProjWorldToScreen(markerData.m_vWorldPosition, GetGame().GetWorld());
		
		// Get camera position and calculate distance
		CameraBase camera = GetGame().GetCameraManager().CurrentCamera();
		if (!camera)
		{
			markerData.m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		vector cameraPosition = camera.GetOrigin();
		float distance = vector.Distance(cameraPosition, markerData.m_vWorldPosition);
		
		// Hide if behind camera or too far away
		if (screenPosition[2] < 0 || distance > m_fMaxDistance)
		{
			markerData.m_wMarkerRoot.SetOpacity(0.0);
			return;
		}
		
		// Calculate text size based on distance
		float sizeFactor = Math.InverseLerp(m_fMaxDistance, m_fMinDistance, distance);
		float textSize = Math.Lerp(m_fMinTextSize, m_fMaxTextSize, sizeFactor);
		
		// Update text font size
		if (markerData.m_wMarkerText)
			markerData.m_wMarkerText.SetExactFontSize(textSize);
		
		// Get text dimensions for positioning
		float textWidth, textHeight;
		if (markerData.m_wMarkerText)
		{
			markerData.m_wMarkerText.GetTextSize(textWidth, textHeight);
		}
		else
		{
			textWidth = textSize * 8; // Rough estimate
			textHeight = textSize;
		}
		
		// Update widget position (centered on screen position)
		FrameSlot.SetPos(markerData.m_wMarkerRoot, screenPosition[0] - textWidth * 0.5, screenPosition[1] - textHeight * 0.5);
		FrameSlot.SetSize(markerData.m_wMarkerRoot, textWidth, textHeight);
		
		// Update background size
		UpdateMarkerBackgroundSize(markerData);
		
		// Set opacity based on distance (closer = more opaque)
		float opacity = Math.Clamp(sizeFactor, 0.4, 1.0);
		markerData.m_wMarkerRoot.SetOpacity(opacity);
	}
	
	/**
	 * Update world position for marker
	 * @param playerEntity Player entity
	 * @param markerData Marker data
	 */
	protected void UpdateWorldPosition(IEntity playerEntity, CRF_GroupLeaderMarkerData markerData)
	{
		if (!playerEntity)
			return;
		
		// Get entity position
		vector entityPos = playerEntity.GetOrigin();
		
		// Try to get a better centered position using entity bounds
		vector mins, maxs;
		playerEntity.GetBounds(mins, maxs);
		if (mins != vector.Zero || maxs != vector.Zero)
		{
			// Use character center position (X and Z centered, Y at feet)
			float centerX = (mins[0] + maxs[0]) * 0.5;
			float centerZ = (mins[2] + maxs[2]) * 0.5;
			markerData.m_vWorldPosition = Vector(entityPos[0] + centerX, entityPos[1], entityPos[2] + centerZ);
		}
		else
		{
			markerData.m_vWorldPosition = entityPos;
		}
		
		// Add height offset to position above the character
		markerData.m_vWorldPosition[1] = markerData.m_vWorldPosition[1] + m_fHeightOffset;
	}
	
	/**
	 * Remove marker for a specific player
	 * @param playerId Player ID
	 */
	protected void RemoveMarkerForPlayer(int playerId)
	{
		if (!m_mPlayerMarkers.Contains(playerId))
			return;
		
		CRF_GroupLeaderMarkerData markerData = m_mPlayerMarkers.Get(playerId);
		
		// Cleanup the marker widget first
		if (markerData)
			markerData.Cleanup();
		
		// Remove from map (this will handle object deletion automatically)
		m_mPlayerMarkers.Remove(playerId);
	}
	
	/**
	 * Remove all markers
	 */
	protected void RemoveAllMarkers()
	{
		// Cleanup all marker widgets first (without deleting the objects yet)
		foreach (int playerId, CRF_GroupLeaderMarkerData markerData : m_mPlayerMarkers)
		{
			if (markerData)
				markerData.Cleanup();
		}
		
		// Now clear the map, which will automatically handle the object deletion
		m_mPlayerMarkers.Clear();
	}
	
	//===================================================================================
	// STATIC ACCESS
	//===================================================================================
	
	/**
	 * Get the instance of the group leader marker manager
	 * @return Manager instance or null
	 */
	static CRF_GroupLeaderMarkerManager GetInstance()
	{
		return m_sInstance;
	}
	
	//===================================================================================
	// CLEANUP
	//===================================================================================
	
	/**
	 * Clean up when component is deleted
	 */
	override void OnDelete(IEntity owner)
	{
		// Unsubscribe from safestart changes
		if (m_SafestartManager && m_SafestartManager.m_OnSafeStartChange)
		{
			if (Replication.IsServer())
				m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChangedServer);
			else
				m_SafestartManager.m_OnSafeStartChange.Remove(OnSafestartChangedClient);
		}
		
		// Remove all markers (client-side only)
		if (!Replication.IsServer())
			RemoveAllMarkers();
		
		// Clear server tracking
		if (Replication.IsServer())
			m_mServerGroupLeaders.Clear();
	}
}
