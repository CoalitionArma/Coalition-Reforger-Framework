/**
 * Enhanced Vehicle Depot System - Trist
    * - Allows ticket/custom use count/supply management
    * - Collision detection to prevent spawning into blocked areas
    * - Leadership role restriction (optional)
 */
[ComponentEditorProps(category: "CRF/Vehicle Depot", description: "Enhanced vehicle depot with collision detection and real resource integration")]
class CRF_VehicleDepotClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
enum CRF_EVehicleDepotCostType
{
	TICKETS,
	SUPPLIES,
	USES
}

//------------------------------------------------------------------------------------------------
enum CRF_EVehicleDepotSpawnPattern
{
	LINE,		// Spawn vehicles in a line behind depot
	ROW,		// Spawn vehicles in a row beside depot (alternating left/right)
	GRID		// Spawn vehicles in a grid pattern (rows and columns)
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sVehicleName")]
class CRF_VehicleDepotVehicle
{
	[Attribute("", UIWidgets.EditBox, "Display name for this vehicle", category: "Vehicle Identity")]
	string m_sVehicleName;
	
	[Attribute("", UIWidgets.ResourceNamePicker, "Vehicle prefab to spawn", "et", category: "Vehicle Identity")]
	ResourceName m_sVehiclePrefab;
	
	[Attribute("0", UIWidgets.ComboBox, "Cost type", "", ParamEnumArray.FromEnum(CRF_EVehicleDepotCostType), category: "Cost Configuration")]
	CRF_EVehicleDepotCostType m_eCostType;
	
	[Attribute("10", UIWidgets.SpinBox, "Cost amount (TICKETS/SUPPLIES: direct cost, USES: cost deducted from global pool, -1 = unlimited)", "-1 1000 1", category: "Cost Configuration")]
	int m_iCost;
}

//------------------------------------------------------------------------------------------------
class CRF_VehicleDepot : ScriptComponent
{
	// Spawn Configuration
	[Attribute("Vehicle Depot", UIWidgets.EditBox, "Name of this depot", category: "Spawn Configuration")]
	protected string m_sDepotName;
	
	[Attribute("3", UIWidgets.SpinBox, "Number of spawn points to create", "1 20 1", category: "Spawn Configuration")]
	protected int m_iSpawnPointCount;

	[Attribute("6", UIWidgets.Slider, "Spawn distance from depot (meters)", "3 50 1", category: "Spawn Configuration")]
	protected float m_fSpawnDistance;
	
	[Attribute("0", UIWidgets.ComboBox, "Spawn pattern", "", ParamEnumArray.FromEnum(CRF_EVehicleDepotSpawnPattern), category: "Spawn Configuration")]
	protected CRF_EVehicleDepotSpawnPattern m_eSpawnPattern;
	
	[Attribute("6", UIWidgets.Slider, "Spacing between spawn positions (meters)", "3 20 0.5", category: "Spawn Configuration")]
	protected float m_fSpawnSpacing;
	
	// Vehicle Orientation Configuration
	[Attribute("0", UIWidgets.Slider, "Vehicle facing direction offset (degrees)", "-180 180 5", category: "Spawn Configuration")]
	protected float m_fVehicleFacingOffset;
	
	[Attribute("1.0", UIWidgets.Slider, "Spawn Collision detection radius (meters)", "1.0 10.0 0.1", category: "Spawn Configuration")]
	protected float m_fCollisionRadius;
	
	// Grid Pattern Configuration (only applies when GRID pattern is selected)
	[Attribute("3", UIWidgets.SpinBox, "Grid spawn pattern columns (vehicles per row)", "1 10 1", category: "Spawn Configuration")]
	protected int m_iGridColumns;
	
	[Attribute("8", UIWidgets.Slider, "Grid spawn pattern Row spacing (meters between rows)", "3 20 0.5", category: "Spawn Configuration")]
	protected float m_fGridRowSpacing;
	
	// Resource Configuration
	[Attribute("10.0", UIWidgets.SpinBox, "Supply storage search radius (meters)", "10.0 200.0 5.0", category: "Resource Configuration")]
	protected float m_fSupplySearchRadius;
	
	[Attribute("20", UIWidgets.SpinBox, "Total uses pool for USES-type vehicles", "0 1000 1", category: "Resource Configuration")]
	protected int m_iGlobalUsesPool;
	
	// Vehicle Configuration
	[Attribute("", UIWidgets.Object, "Available vehicles", category: "Vehicle Configuration")]
	protected ref array<ref CRF_VehicleDepotVehicle> m_aVehicles;
	
	// Track current uses remaining in the global pool (replicated for real-time UI updates)
	[RplProp()]
	protected int m_iUsesRemaining;
	
	// Track current supplies remaining for real-time UI updates (cached from external sources)
	[RplProp()]
	protected int m_iCachedSuppliesRemaining;
	
	// Component references
	protected CRF_RespawnManager m_RespawnManager;
	protected CRF_SlottingManager m_SlottingManager;
	
	// Collision detection helper
	protected bool m_bCollisionDetected = false;
	
	// Performance optimization caches
	protected ref array<vector> m_aCachedSpawnPositions;
	protected bool m_bSpawnPositionsCached = false;
	
	// Resource checking cache (prevents frame-rate spam)
	protected SCR_ResourceComponent m_CachedResourceComponent;
	// Resource search cache
	protected float m_fLastResourceSearchTime = 0;
	protected static const float RESOURCE_CACHE_DURATION = 30.0; // Cache for 30 seconds to reduce expensive queries
	protected static const int MAX_ENTITIES_TO_PROCESS = 15; // Limit entity processing to prevent performance spikes
	
	// Role restrictions
	[Attribute("0", UIWidgets.CheckBox, "Restrict to leadership roles only (Squad Leaders, Team Leaders, Platoon Leaders, Platoon Sergeants)")]
	bool m_bRestrictToLeadership;
	
	// Debug visualization (only visible in Workbench editor)
	[Attribute("0", UIWidgets.CheckBox, "Show debug spawn point visuals")]
	bool m_bEnableSpawnVisuals;
	
	// Debug logging
	[Attribute("0", UIWidgets.CheckBox, "Enable debug logging")]
	bool m_bEnableDebugLogging;
	
	#ifdef WORKBENCH
	// Debug shape creation tracking (Workbench only - zero runtime overhead)
	protected bool m_bDebugShapesCreated = false;
	protected ref array<ref Shape> m_aDebugShapes = {};
	#endif
	
	// Store found entities for processing
	protected ref array<IEntity> m_aNearbyEntities = {};

	//================================================================================================
	// CORE SYSTEM FUNCTIONS - Initialization and Setup
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!m_aVehicles)
			m_aVehicles = {};
			
		// Initialize performance caches
		m_aCachedSpawnPositions = {};
		#ifdef WORKBENCH
		m_aDebugShapes = {};
		#endif
			
		// Initialize global uses pool
		m_iUsesRemaining = m_iGlobalUsesPool;
		
		// Initialize cached supplies count (only if world is available)
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			m_iCachedSuppliesRemaining = GetRemainingSuppliesFromSource();
		}
		else
		{
			// World not ready yet (editor mode) - initialize to 0, will update later
			m_iCachedSuppliesRemaining = 0;
			DebugPrint("World not available during initialization - supplies cache will be updated later");
		}
			
		// Get CRF managers
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
		{
			m_RespawnManager = CRF_RespawnManager.Cast(gameMode.FindComponent(CRF_RespawnManager));
			m_SlottingManager = CRF_SlottingManager.Cast(gameMode.FindComponent(CRF_SlottingManager));
		}
		
		// Set up interaction
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
	//-	DebugPrint(string.Format("Vehicle depot initialized with %1 vehicles and %2 spawn points.", m_aVehicles.Count(), m_iSpawnPointCount));
		
		// Pre-cache spawn positions during initialization for better performance
		// This follows the pattern used in other CRF components (like CRF_MapStagingComponent)
		GetGame().GetCallqueue().CallLater(InitializeSpawnPositions, 1000, false);
		
		// Initialize supplies cache on server
		if (RplSession.Mode() != RplMode.Client)
		{
			GetGame().GetCallqueue().CallLater(InitializeSuppliesCache, 1500, false);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize spawn positions during component startup (delayed to ensure world is ready)
	protected void InitializeSpawnPositions()
	{
		if (!GetOwner())
			return;
			
		CacheSpawnPositions();
		DebugPrint("Spawn positions pre-cached during initialization");
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize supplies cache during component startup (delayed to ensure resource system is ready)
	protected void InitializeSuppliesCache()
	{
		if (!GetOwner())
			return;
			
		// Cache current supplies value for replication
		m_iCachedSuppliesRemaining = GetRemainingSuppliesFromSource();
		Replication.BumpMe();
		DebugPrint(string.Format("Supplies cache initialized to %1", m_iCachedSuppliesRemaining));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get list of vehicles for UI/interaction
	array<ref CRF_VehicleDepotVehicle> GetVehicles()
	{
		return m_aVehicles;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get depot info
	string GetDepotName() { return m_sDepotName; }

	//================================================================================================
	// RESOURCE MANAGEMENT FUNCTIONS - Tickets, Supplies, Uses
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Check if player can afford a vehicle
	bool CanAffordVehicle(int playerId, CRF_VehicleDepotVehicle vehicle, int vehicleIndex)
	{
		if (!vehicle)
			return false;
			
		// Check role restrictions first
		if (m_bRestrictToLeadership && !HasRequiredLeadershipRole(playerId))
			return false;
			
		switch (vehicle.m_eCostType)
		{
			case CRF_EVehicleDepotCostType.TICKETS:
				return CanAffordTickets(playerId, vehicle.m_iCost);
				
			case CRF_EVehicleDepotCostType.SUPPLIES:
				return CanAffordSupplies(vehicle.m_iCost);
				
			case CRF_EVehicleDepotCostType.USES:
				return CanAffordUses(playerId, vehicleIndex);
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check if player has required leadership role
	bool HasRequiredLeadershipRole(int playerId)
	{
		if (!m_SlottingManager)
			return false;
			
		// Get player's slot data to check their role
		CRF_SlotDataContainer slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return false;
			
		// Check if player has leadership slot type
		CRF_ESlotType slotType = slotData.GetSlotType();
		if (slotType == CRF_ESlotType.SQUAD_LEADER || slotType == CRF_ESlotType.TEAM_LEADER)
			return true;
			
		// Also check the player's gear role for additional leadership roles like Platoon Leader/Sergeant
		ResourceName playerResource = slotData.GetSlotResource();
		if (playerResource.IsEmpty())
			return false;
			
		CRF_EGearRole gearRole = CRF_RoleHelper.ResourceToRole(playerResource);
		
		// Check for additional leadership roles not covered by slot types
		if (gearRole == CRF_EGearRole.PLATOON_LEADER || 
			gearRole == CRF_EGearRole.PLATOON_SERGEANT ||
			gearRole == CRF_EGearRole.COMPANY_COMMANDER ||
			gearRole == CRF_EGearRole.FIRST_SERGEANT)
			return true;
			
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CanAffordTickets(int playerId, int cost)
	{
		// Check for unlimited tickets (cost of -1)
		if (cost == -1)
			return true;
			
		if (!m_SlottingManager)
			return false;
			
		Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		if (!playerFaction)
			return false;
			
		FactionKey factionKey = playerFaction.GetFactionKey();
		
		// Get tickets from CRF_Gamemode based on faction
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return false;
			
		int currentTickets = 0;
		if (factionKey == "BLUFOR")
			currentTickets = gamemode.m_iBLUFORTickets;
		else if (factionKey == "OPFOR")
			currentTickets = gamemode.m_iOPFORTickets;
		else if (factionKey == "INDFOR")
			currentTickets = gamemode.m_iINDFORTickets;
		else if (factionKey == "CIV")
			currentTickets = gamemode.m_iCIVTickets;
		
		return currentTickets >= cost;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CanAffordSupplies(int cost)
	{
		// Use cached replicated value for consistent client-server synchronization
		return m_iCachedSuppliesRemaining >= cost;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Check supplies for spawning operations (includes debug logging)
	protected bool CanAffordSuppliesForSpawn(int cost)
	{
		bool canAfford = CanAffordSupplies(cost);
		
		// Only log during actual spawn attempts, not UI updates
		if (!canAfford)
		{
			if (m_bEnableDebugLogging) Print(string.Format("[CRF_VehicleDepot] Spawn failed - need %1 supplies, only %2 available (cached)", cost, m_iCachedSuppliesRemaining));
		}
		
		return canAfford;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CanAffordUses(int playerId, int vehicleIndex)
	{
		CRF_VehicleDepotVehicle vehicle = m_aVehicles[vehicleIndex];
		if (!vehicle || vehicle.m_eCostType != CRF_EVehicleDepotCostType.USES)
			return false;
			
		// Check for unlimited uses (cost of -1)
		if (vehicle.m_iCost == -1)
			return true;
			
		// Check if global pool has enough uses for this vehicle's cost
		return m_iUsesRemaining >= vehicle.m_iCost;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get remaining uses in the global pool
	int GetUsesRemaining()
	{
		return m_iUsesRemaining;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get remaining resources for display
	int GetRemainingTickets(int playerId)
	{
		if (!m_RespawnManager || !m_SlottingManager)
			return 0;
			
		// Get player's faction
		Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		if (!playerFaction)
			return 0;
			
		FactionKey factionKey = playerFaction.GetFactionKey();
		if (factionKey.IsEmpty())
			return 0;
			
		// Use the private method through reflection or find public access
		// For now, assume we can access faction tickets directly from gamemode
		CRF_Gamemode gamemode = CRF_Gamemode.GetInstance();
		if (!gamemode)
			return 0;
			
		switch (factionKey)
		{
			case "BLUFOR": return gamemode.m_iBLUFORTickets;
			case "OPFOR": return gamemode.m_iOPFORTickets;
			case "INDFOR": return gamemode.m_iINDFORTickets;
			case "CIV": return gamemode.m_iCIVTickets;
		}
		
		return 0;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetRemainingSupplies(int playerId)
	{
		// Use cached replicated value for UI updates (fast and reliable)
		return m_iCachedSuppliesRemaining;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get supplies directly from source (for server-side operations)
	protected int GetRemainingSuppliesFromSource()
	{
		// Use cached resource component for UI updates
		SCR_ResourceComponent resourceComponent = GetCachedResourceComponent();
		if (!resourceComponent)
			return 0;
		
		// Use real Arma resource system to get supplies
		SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
		if (consumer)
		{
			float currentSupplies = consumer.GetAggregatedResourceValue();
			return (int)currentSupplies;
		}
		else
		{
			// Fallback to direct storage reading
			float currentSupplies = 0;
			SCR_ResourceSystemHelper.GetStoredResources(resourceComponent, currentSupplies);
			return (int)currentSupplies;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get cached resource component to avoid expensive repeated searches
	protected SCR_ResourceComponent GetCachedResourceComponent()
	{
		// Check if world is available (prevents crashes during editor initialization)
		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			DebugPrint("World not available yet - skipping resource component search");
			return null;
		}
		
		float currentTime = world.GetWorldTime();
		
		// Check if cache is still valid
		if (m_CachedResourceComponent && (currentTime - m_fLastResourceSearchTime) < RESOURCE_CACHE_DURATION)
		{
			return m_CachedResourceComponent;
		}
		
		// Cache expired or doesn't exist, search for new one
		m_CachedResourceComponent = FindNearbyResourceStorage();
		
		// Only log on first cache miss, not every refresh (prevent spam)
		bool isFirstSearch = (m_fLastResourceSearchTime == 0);
		m_fLastResourceSearchTime = currentTime;
		
		if (!m_CachedResourceComponent && isFirstSearch)
		{
			DebugPrint("No supply storage found nearby - depot will only work with TICKETS/USES vehicles");
		}
		
		return m_CachedResourceComponent;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Find nearby resource storage for supply management
	protected SCR_ResourceComponent FindNearbyResourceStorage()
	{
		if (!GetOwner())
			return null;
			
		// Check if world is available (prevents crashes during editor initialization)
		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			DebugPrint("World not available - cannot search for resource storage");
			return null;
		}
			
		vector depotPos = GetOwner().GetOrigin();
		
		// Clear previous results and query entities within search radius  
		m_aNearbyEntities.Clear();
		world.QueryEntitiesBySphere(depotPos, m_fSupplySearchRadius, QueryEntitiesCallback, null, EQueryEntitiesFlags.STATIC | EQueryEntitiesFlags.WITH_OBJECT);
		
		// Find the closest entity with a resource component that has supplies
		SCR_ResourceComponent bestResourceComponent = null;
		float closestDistance = float.MAX;
		float bestSupplies = 0;
		int processedCount = 0;
		
		foreach (IEntity entity : m_aNearbyEntities)
		{
			// Limit processing to prevent performance spikes with many entities
			if (processedCount >= MAX_ENTITIES_TO_PROCESS)
			{
				DebugPrint(string.Format("Resource search limited to %1 entities for performance", MAX_ENTITIES_TO_PROCESS));
				break;
			}
			
			if (!entity || entity == GetOwner())
				continue;
				
			processedCount++;
				
			// Check if entity has a resource component
			SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.FindResourceComponent(entity, false);
			if (!resourceComponent)
				continue;
				
			float availableSupplies = 0;
			
			// Try consumer first
			SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			if (consumer)
			{
				availableSupplies = consumer.GetAggregatedResourceValue();
			}
			else
			{
				// Fallback to direct storage check
				SCR_ResourceSystemHelper.GetStoredResources(resourceComponent, availableSupplies);
			}
			
			// Check distance and supply availability
			float distance = vector.Distance(depotPos, entity.GetOrigin());
			if (availableSupplies > 0 && distance < closestDistance)
			{
				closestDistance = distance;
				bestResourceComponent = resourceComponent;
				bestSupplies = availableSupplies;
				
				// Early exit optimization: if we find a very close resource with supplies, use it immediately
				if (distance < 5.0 && availableSupplies > 0)
				{
					DebugPrint(string.Format("Found close resource at %1m with %2 supplies - using immediately", distance, availableSupplies));
					break;
				}
			}
		}
		
		return bestResourceComponent;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Callback for entity queries - collect entities with resource components
	protected bool QueryEntitiesCallback(IEntity entity)
	{
		if (!entity)
			return true;
			
		// Check if entity has a resource component
		SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.FindResourceComponent(entity, false);
		if (resourceComponent)
		{
			m_aNearbyEntities.Insert(entity);
		}
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DeductCost(int playerId, CRF_VehicleDepotVehicle vehicle, int vehicleIndex)
	{
		switch (vehicle.m_eCostType)
		{
			case CRF_EVehicleDepotCostType.TICKETS:
				DeductTickets(playerId, vehicle.m_iCost);
				break;
				
			case CRF_EVehicleDepotCostType.SUPPLIES:
				DeductSupplies(vehicle.m_iCost);
				break;
				
			case CRF_EVehicleDepotCostType.USES:
				DeductUses(vehicleIndex);
				break;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DeductTickets(int playerId, int amount)
	{
		// Check for unlimited tickets (cost of -1)
		if (amount == -1)
		{
			if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] Spawned unlimited ticket vehicle - no tickets deducted");
			return;
		}
		
		if (!m_SlottingManager || !m_RespawnManager)
			return;
			
		Faction playerFaction = m_SlottingManager.GetPlayerSlotFaction(playerId);
		if (!playerFaction)
			return;
			
		FactionKey factionKey = playerFaction.GetFactionKey();
		m_RespawnManager.SubtractTicket(factionKey, amount, true);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DeductSupplies(int amount)
	{
		// Find nearby supply depot or resource storage
		SCR_ResourceComponent resourceComponent = FindNearbyResourceStorage();
		if (!resourceComponent)
		{
			if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] No supply storage found nearby for deduction!");
			return;
		}
		
		// Get the entity that owns this resource component
		IEntity resourceEntity = resourceComponent.GetOwner();
		if (!resourceEntity)
		{
			if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] Could not find resource entity owner!");
			return;
		}
		
		// Use real Arma resource system to consume supplies
		SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
		if (consumer)
		{
			// Actually consume the supplies using real Arma system
			bool consumptionSuccess = consumer.RequestConsumtion(amount);
			if (consumptionSuccess)
			{
				if (m_bEnableDebugLogging) Print(string.Format("[CRF_VehicleDepot] Successfully consumed %1 supplies from real Arma resource system", amount));
				
				// Update cached supply count for real-time UI updates
				m_iCachedSuppliesRemaining = GetRemainingSuppliesFromSource();
				
				// Invalidate resource cache since supplies changed
				m_fLastResourceSearchTime = 0;
				
				// Trigger replication to update all clients immediately
				Replication.BumpMe();
			}
			else
			{
				if (m_bEnableDebugLogging) Print(string.Format("[CRF_VehicleDepot] Failed to consume %1 supplies - insufficient supplies available", amount));
			}
		}
		else
		{
			// Warning: Cannot actually consume from direct storage
			if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] WARNING: Supply storage found but no consumer available - cannot actually deduct supplies!");
			if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] You need to configure the supply storage with proper consumers/generators for real resource consumption.");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DeductUses(int vehicleIndex)
	{
		CRF_VehicleDepotVehicle vehicle = m_aVehicles[vehicleIndex];
		if (vehicle && vehicle.m_eCostType == CRF_EVehicleDepotCostType.USES)
		{
			// Check for unlimited uses (cost of -1)
			if (vehicle.m_iCost == -1)
			{
				if (m_bEnableDebugLogging) Print(string.Format("[CRF_VehicleDepot] Spawned unlimited vehicle %1 - no cost deducted", vehicle.m_sVehicleName));
				return;
			}
			
			// Deduct the vehicle's cost from the global pool
			int cost = vehicle.m_iCost;
			if (m_iUsesRemaining >= cost)
			{
				m_iUsesRemaining -= cost;
				if (m_bEnableDebugLogging) Print(string.Format("[CRF_VehicleDepot] Deducted %1 uses from global pool, %2 remaining", cost, m_iUsesRemaining));
				
				// Trigger replication to update all clients immediately
				Replication.BumpMe();
			}
		}
	}

	//================================================================================================
	// SPAWN SYSTEM FUNCTIONS - Positioning, Collision Detection, Vehicle Spawning
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Spawn a vehicle for the player
	bool SpawnVehicle(int playerId, int vehicleIndex)
	{
		if (vehicleIndex < 0 || vehicleIndex >= m_aVehicles.Count())
			return false;
			
		CRF_VehicleDepotVehicle vehicle = m_aVehicles[vehicleIndex];
		if (!vehicle)
			return false;
			
		// Check if player can afford it
		if (!CanAffordVehicle(playerId, vehicle, vehicleIndex))
		{
			// Log error but don't show notification to player
			if (m_bRestrictToLeadership && !HasRequiredLeadershipRole(playerId))
			{
				if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] Player lacks required leadership role!");
			}
			else if (vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
			{
				// Use the debug version for spawn attempts
				if (!CanAffordSuppliesForSpawn(vehicle.m_iCost))
				{
					if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] Player cannot afford supplies!");
				}
			}
			else
			{
				if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] Player cannot afford this vehicle!");
			}
			return false;
		}
		
		// Find spawn position
		vector spawnPos = FindSpawnPosition();
		if (spawnPos == vector.Zero)
		{
			DebugPrint("No valid spawn position found!");
			ShowNotificationToPlayer(playerId, "No space available", "Vehicle spawning blocked by obstacles.");
			return false;
		}
		
		// Spawn the vehicle
		DebugPrint(string.Format("DEBUG: Vehicle name='%1', prefab='%2', cost=%3, costType=%4", 
			vehicle.m_sVehicleName, vehicle.m_sVehiclePrefab, vehicle.m_iCost, vehicle.m_eCostType));
			
		Resource resource = Resource.Load(vehicle.m_sVehiclePrefab);
		if (!resource)
		{
			DebugPrint("Failed to load vehicle prefab!");
			return false;
		}
		
		EntitySpawnParams spawnParams = EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = spawnPos;
		
		// Set the vehicle orientation to be level but facing the depot's direction with optional offset
		// Get depot's transform matrix for proper directional vectors
		vector depotTransform[4];
		GetOwner().GetWorldTransform(depotTransform);
		
		// Create a level orientation using only the depot's yaw direction
		vector forwardDir = depotTransform[2]; // Forward direction
		vector rightDir = depotTransform[0];   // Right direction
		
		// Force the forward direction to be level (remove any pitch)
		forwardDir[1] = 0; // Remove Y component to make it level
		forwardDir = forwardDir.Normalized(); // Normalize after modification
		
		// Apply vehicle facing direction offset
		if (m_fVehicleFacingOffset != 0)
		{
			// Convert offset to radians
			float offsetRadians = m_fVehicleFacingOffset * Math.DEG2RAD;
			
			// Rotate the forward direction by the offset angle around the Y axis
			float cosOffset = Math.Cos(offsetRadians);
			float sinOffset = Math.Sin(offsetRadians);
			
			// Apply rotation matrix for Y-axis rotation
			vector newForwardDir;
			newForwardDir[0] = forwardDir[0] * cosOffset + forwardDir[2] * sinOffset;
			newForwardDir[1] = forwardDir[1]; // Y stays the same
			newForwardDir[2] = -forwardDir[0] * sinOffset + forwardDir[2] * cosOffset;
			
			forwardDir = newForwardDir.Normalized();
		}
		
		// Calculate right direction perpendicular to level forward
		vector upDir = "0 1 0"; // World up
		rightDir = upDir * forwardDir; // Cross product for right direction
		rightDir = rightDir.Normalized();
		
		// Rebuild the transform matrix with level orientation
		spawnParams.Transform[0] = rightDir;   // Right vector
		spawnParams.Transform[1] = upDir;      // Up vector (always world up)
		spawnParams.Transform[2] = forwardDir; // Forward vector (level)
		spawnParams.Transform[3] = spawnPos;   // Position
		
		IEntity spawnedVehicle = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!spawnedVehicle)
		{
			if (m_bEnableDebugLogging) Print("[CRF_VehicleDepot] Failed to spawn vehicle!");
			return false;
		}
		
		// Deduct cost
		DeductCost(playerId, vehicle, vehicleIndex);

		// Notify success
		string costInfo = GetCostInfoText(vehicle);
		string message = string.Format("Spawned %1 for %2", vehicle.m_sVehicleName, costInfo);
		ShowNotificationToPlayer(playerId, "Vehicle Spawned", message);
		DebugPrint(message);

		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected vector FindSpawnPosition()
	{
		// Spawn positions should already be cached during initialization
		// Simple check without fallback - if not cached, something went wrong during init
		if (!m_bSpawnPositionsCached || m_aCachedSpawnPositions.Count() == 0)
		{
			DebugPrint("ERROR: Spawn positions not cached! Initialization failed.");
			return vector.Zero;
		}
		
		// Try all cached spawn positions
		for (int i = 0; i < m_aCachedSpawnPositions.Count(); i++)
		{
			vector testPos = m_aCachedSpawnPositions[i];
			
			// Check if position is clear
			if (IsPositionClear(testPos, m_fCollisionRadius))
			{
				string patternName;
				if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.LINE)
					patternName = "LINE";
				else if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.ROW)
					patternName = "ROW";
				else
					patternName = "GRID";
				DebugPrint(string.Format("Found spawn position in %1 pattern at slot %2", patternName, i));
				return testPos;
			}
			else
			{
				DebugPrint(string.Format("Spawn slot %1 is blocked", i));
			}
		}
		
		string patternName;
		if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.LINE)
			patternName = "behind";
		else if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.ROW)
			patternName = "beside";
		else
			patternName = "in grid around";
		DebugPrint(string.Format("All spawn slots %1 depot are blocked!", patternName));
		return vector.Zero;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Cache spawn positions to avoid repeated calculations
	protected void CacheSpawnPositions()
	{
		// Array should already be initialized in OnPostInit, just clear it
		m_aCachedSpawnPositions.Clear();
		
		vector depotPos = GetOwner().GetOrigin();
		vector depotAngles = GetOwner().GetAngles();
		
		// Calculate directional vectors based on depot orientation
		vector transform[4];
		GetOwner().GetWorldTransform(transform);
		vector forwardDir = transform[2]; // Forward direction (Z-axis)
		vector rightDir = transform[0];   // Right direction (X-axis)
		
		BaseWorld world = GetGame().GetWorld();
		
		// Pre-calculate all spawn positions with ground height
		for (int i = 0; i < m_iSpawnPointCount; i++)
		{
			vector spawnPos = CalculateSpawnPosition(depotPos, forwardDir, rightDir, i);
			
			// Properly calculate ground height and adjust for vehicle
			if (world)
			{
				float surfaceY = world.GetSurfaceY(spawnPos[0], spawnPos[2]);
				spawnPos[1] = surfaceY + 1.5; // 1.5m elevation should prevent most clipping
			}
			else
			{
				spawnPos[1] = depotPos[1] + 1.5; // Fallback to depot height + 1.5m
			}
			
			m_aCachedSpawnPositions.Insert(spawnPos);
		}
		
		m_bSpawnPositionsCached = true;
		DebugPrint(string.Format("Cached %1 spawn positions", m_aCachedSpawnPositions.Count()));
	}
	
	//------------------------------------------------------------------------------------------------
	//! Calculate spawn position based on pattern and index - optimized math for reuse
	vector CalculateSpawnPosition(vector depotPos, vector forwardDir, vector rightDir, int index)
	{
		if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.LINE)
		{
			// LINE: Spawn behind depot with increasing distance
			float distance = m_fSpawnDistance + (m_fSpawnSpacing * index);
			return depotPos + (-forwardDir * distance);
		}
		else if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.ROW)
		{
			// ROW: Start behind depot, then alternate left and right
			vector baseOffset = -forwardDir * m_fSpawnDistance;
			
			if (index == 0)
			{
				// First spawn: directly behind depot (center)
				return depotPos + baseOffset;
			}
			
			// Calculate lateral position - optimized for alternating pattern
			int lateralIndex = (index + 1) / 2; // Integer division for consistent spacing
			float lateralDistance = m_fSpawnSpacing * lateralIndex;
			
			// Determine side: odd indices go right, even go left
			vector lateralOffset = rightDir * lateralDistance;
			if (index % 2 == 0) // Even numbers = Left side  
				lateralOffset = -lateralOffset;
			
			return depotPos + baseOffset + lateralOffset;
		}
		else // GRID pattern
		{
			// GRID: Arrange vehicles in rows and columns
			// Row 0: vehicles 0, 1, 2 (if 3 columns)
			// Row 1: vehicles 3, 4, 5 (if 3 columns)
			// etc.
			int row = index / m_iGridColumns;
			int col = index % m_iGridColumns;
			
			// Calculate base position behind depot
			vector baseOffset = -forwardDir * m_fSpawnDistance;
			
			// Add row offset (further back for each row)
			vector rowOffset = -forwardDir * (m_fGridRowSpacing * row);
			
			// Add column offset (lateral positioning)
			// Center the columns around the depot
			float totalWidth = (m_iGridColumns - 1) * m_fSpawnSpacing;
			float columnStart = -totalWidth * 0.5; // Start from left side
			float columnPosition = columnStart + (col * m_fSpawnSpacing);
			vector columnOffset = rightDir * columnPosition;
			
			return depotPos + baseOffset + rowOffset + columnOffset;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Invalidate spawn position cache (call when depot moves or settings change)
	//! This will force recalculation on next spawn attempt
	void InvalidateSpawnCache()
	{
		m_bSpawnPositionsCached = false;
		if (m_aCachedSpawnPositions)
			m_aCachedSpawnPositions.Clear();
		DebugPrint("Spawn position cache invalidated - will recalculate on next use");
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force immediate recalculation of spawn positions (useful for editor changes or runtime updates)
	void RecalculateSpawnPositions()
	{
		// Directly recalculate without going through invalidate/cache cycle
		CacheSpawnPositions();
		DebugPrint("Spawn positions forcibly recalculated");
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsPositionClear(vector pos, float radius)
	{
		// Reset collision detection flag
		m_bCollisionDetected = false;
		
		// Use physics to check for overlapping objects
		World world = GetGame().GetWorld();
		
		// Query entities in the area using our callback
		world.QueryEntitiesBySphere(pos, radius, CheckEntityCallback, null, EQueryEntitiesFlags.DYNAMIC | EQueryEntitiesFlags.WITH_OBJECT);
		
		// Return the result based on whether collision was detected
		return !m_bCollisionDetected;
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Callback function for sphere query to check entities
	 */
	protected bool CheckEntityCallback(IEntity entity)
	{
		// Skip the depot itself
		if (entity == GetOwner())
			return true; // Continue searching
			
		// Check for vehicles
		VehicleControllerComponent vehicleController = VehicleControllerComponent.Cast(entity.FindComponent(VehicleControllerComponent));
		if (vehicleController)
		{
			DebugPrint("Position blocked by vehicle");
			m_bCollisionDetected = true;
			return false; // Stop searching - position is blocked
		}
		
		// Check for characters
		CharacterControllerComponent characterController = CharacterControllerComponent.Cast(entity.FindComponent(CharacterControllerComponent));
		if (characterController)
		{
			DebugPrint("Position blocked by character");
			m_bCollisionDetected = true;
			return false; // Stop searching - position is blocked
		}
		
		// Check for large static objects that might block spawning
		Physics physics = entity.GetPhysics();
		if (physics && physics.IsDynamic() == false)
		{
			// Check if it's a significant object (has collision and reasonable size)
			vector min, max;
			entity.GetBounds(min, max);
			vector size = max - min;
			if (size.Length() > 2.0) // Objects larger than 2m might block spawning
			{
				DebugPrint(string.Format("Position blocked by static object: %1", entity.GetPrefabData().GetPrefabName()));
				m_bCollisionDetected = true;
				return false; // Stop searching - position is blocked
			}
		}
		
		return true; // Continue searching
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool QueryCallback(IEntity entity)
	{
		// Return true to include this entity in the results
		return true;
	}

	//================================================================================================
	// UI AND INTERACTION FUNCTIONS - Action Text, Notifications
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! RPC to show notification on client
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ShowNotification(int targetPlayerId, string title, string message)
	{
		// This runs on all clients, but only show for the target player
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId != targetPlayerId)
			return;
			
		SCR_PopUpNotification popUpNotification = SCR_PopUpNotification.GetInstance();
		if (popUpNotification)
		{
			popUpNotification.PopupMsg(message, 3.0, title);
		}
		else
		{
			// Fallback to print if notification system isn't available
			Print(string.Format("[CRF_VehicleDepot] %1: %2", title, message));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get action text without caching - real-time generation
	string GetCachedActionText(int vehicleIndex, CRF_VehicleDepotVehicle vehicle)
	{
		if (!vehicle || vehicleIndex < 0)
			return "Invalid Vehicle";
			
		// Generate text every time - no caching to ensure real-time updates
		return GenerateActionText(vehicleIndex, vehicle);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Generate cost info text for notifications
	 */
	protected string GetCostInfoText(CRF_VehicleDepotVehicle vehicle)
	{
		if (vehicle.m_iCost == -1)
		{
			switch (vehicle.m_eCostType)
			{
				case CRF_EVehicleDepotCostType.TICKETS: return "unlimited tickets";
				case CRF_EVehicleDepotCostType.SUPPLIES: return "unlimited supplies";
				case CRF_EVehicleDepotCostType.USES: return "unlimited uses";
			}
		}
		
		switch (vehicle.m_eCostType)
		{
			case CRF_EVehicleDepotCostType.TICKETS: return string.Format("%1 tickets", vehicle.m_iCost);
			case CRF_EVehicleDepotCostType.SUPPLIES: return string.Format("%1 supplies", vehicle.m_iCost);
			case CRF_EVehicleDepotCostType.USES: return string.Format("%1 uses (Pool: %2 remaining)", vehicle.m_iCost, m_iUsesRemaining);
		}
		
		return "unknown cost";
	}
	
	//------------------------------------------------------------------------------------------------
	//! Generate action text (internal method, use GetCachedActionText instead)
	protected string GenerateActionText(int vehicleIndex, CRF_VehicleDepotVehicle vehicle)
	{
		// Get player ID for resource checking
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		
		// Check if there's a role restriction first
		if (m_bRestrictToLeadership)
		{
			bool hasRoleAccess = HasRequiredLeadershipRole(playerId);
			if (!hasRoleAccess)
			{
				return string.Format("[LEADERSHIP ONLY] Spawn %1", vehicle.m_sVehicleName);
			}
		}
		
		// Get current remaining resources and vehicle cost (this is cached, so no spam)
		string costText;
		string remainingText;
		bool canAfford = CanAffordVehicle(playerId, vehicle, vehicleIndex);
		
		switch (vehicle.m_eCostType)
		{
			case CRF_EVehicleDepotCostType.TICKETS:
			{
				if (vehicle.m_iCost == -1)
				{
					costText = "Unlimited Tickets";
					remainingText = "";
				}
				else
				{
					int remaining = GetRemainingTickets(playerId);
					costText = string.Format("%1 Tickets", vehicle.m_iCost);
					remainingText = string.Format("(%1 left)", remaining);
				}
				break;
			}
			case CRF_EVehicleDepotCostType.SUPPLIES:
			{
				int remaining = GetRemainingSupplies(playerId);
				costText = string.Format("%1 Supplies", vehicle.m_iCost);
				remainingText = string.Format("(%1 left)", remaining);
				break;
			}
			case CRF_EVehicleDepotCostType.USES:
			{
				int remaining = GetUsesRemaining();
				if (vehicle.m_iCost == -1)
				{
					costText = "Unlimited Uses";
					remainingText = "";
				}
				else
				{
					costText = string.Format("%1 Uses", vehicle.m_iCost);
					remainingText = string.Format("(%1 left)", remaining);
				}
				break;
			}
		}
		
		// Format: "Spawn Humvee - 5 Tickets (15 left)" or "[NO FUNDS] Spawn Humvee - 5 Tickets (0 left)"
		if (canAfford)
			return string.Format("Spawn %1 - %2 %3", vehicle.m_sVehicleName, costText, remainingText);
		else
			return string.Format("[NO FUNDS] Spawn %1 - %2 %3", vehicle.m_sVehicleName, costText, remainingText);
	}
	
	//------------------------------------------------------------------------------------------------
	/**
	 * Show notification to player (replicated from server to client)
	 */
	protected void ShowNotificationToPlayer(int playerId, string title, string message)
	{
		// Only send notification if running on server
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		// Send RPC with player ID so client can filter
		Rpc(RPC_ShowNotification, playerId, title, message);
		
		// Also log for server debugging
		DebugPrint(string.Format("Notification sent to player %1: %2 - %3", playerId, title, message));
	}

	//================================================================================================
	// DEBUG FUNCTIONS - Visualization and Logging
	//================================================================================================


	protected void DebugPrint(string message)
	{
		if (m_bEnableDebugLogging)
		{
			Print(string.Format("[CRF_VehicleDepot] %1", message));
		}
	}
	
	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//! Workbench debug visualization - shows spawn points as red spheres only in editor
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		// Only draw if debug visuals are enabled
		if (!m_bEnableSpawnVisuals)
		{
			// Clear shapes if visuals are disabled
			if (m_bDebugShapesCreated)
			{
				ClearDebugShapes();
			}
			return;
		}
			
		if (!owner)
			return;
		
		// Only create shapes once, then leave them persistent
		if (!m_bDebugShapesCreated)
		{
			CreateDebugShapes(owner);
			m_bDebugShapesCreated = true;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Create persistent debug shapes (called once)
	protected void CreateDebugShapes(IEntity owner)
	{
		vector depotPos = owner.GetOrigin();
		vector transform[4];
		owner.GetWorldTransform(transform);
		vector forwardDir = transform[2]; // Forward direction (Z-axis)
		vector rightDir = transform[0];   // Right direction (X-axis)
		
		// Calculate vehicle facing direction with offset for orientation line
		vector vehicleForwardDir = forwardDir;
		vehicleForwardDir[1] = 0; // Make level
		vehicleForwardDir = vehicleForwardDir.Normalized();
		
		// Apply vehicle facing direction offset
		if (m_fVehicleFacingOffset != 0)
		{
			// Convert offset to radians
			float offsetRadians = m_fVehicleFacingOffset * Math.DEG2RAD;
			
			// Rotate the forward direction by the offset angle around the Y axis
			float cosOffset = Math.Cos(offsetRadians);
			float sinOffset = Math.Sin(offsetRadians);
			
			// Apply rotation matrix for Y-axis rotation
			vector newForwardDir;
			newForwardDir[0] = vehicleForwardDir[0] * cosOffset + vehicleForwardDir[2] * sinOffset;
			newForwardDir[1] = vehicleForwardDir[1]; // Y stays the same
			newForwardDir[2] = -vehicleForwardDir[0] * sinOffset + vehicleForwardDir[2] * cosOffset;
			
			vehicleForwardDir = newForwardDir.Normalized();
		}
		
		// Use the proper world reference and coordinate handling like CRF_PolyZoneMeshComponent
		BaseWorld world = owner.GetWorld(); // Use owner.GetWorld() instead of GetGame().GetWorld()
		
		// Shape flags for consistent rendering
		const ShapeFlags solidSphereFlags = ShapeFlags.VISIBLE | ShapeFlags.NOZBUFFER | ShapeFlags.NOOUTLINE;
		const ShapeFlags wireframeSphereFlags = ShapeFlags.TRANSP | ShapeFlags.VISIBLE | ShapeFlags.NOOUTLINE | ShapeFlags.NOZBUFFER;
		const ShapeFlags arrowFlags = ShapeFlags.VISIBLE | ShapeFlags.NOZBUFFER;
		
		for (int i = 0; i < m_iSpawnPointCount; i++)
		{
			// Calculate spawn position using same logic as cache
			vector spawnPos = CalculateSpawnPosition(depotPos, forwardDir, rightDir, i);
			
			// Apply proper ground snapping using owner's world reference
			if (world)
			{
				float surfaceY = world.GetSurfaceY(spawnPos[0], spawnPos[2]);
				spawnPos[1] = surfaceY + 0.1; // Small offset to keep spheres just above ground
			}
			else
			{
				spawnPos[1] = depotPos[1] + 0.1; // Small fallback offset
			}
			
			// Create and store shape references for proper management
			// Red sphere for spawn position
			ref Shape spawnSphere = Shape.CreateSphere(ARGB(255, 255, 0, 0), solidSphereFlags, spawnPos, 0.2);
			if (spawnSphere)
				m_aDebugShapes.Insert(spawnSphere);
			
			// Blue wireframe sphere for collision radius
			ref Shape collisionSphere = Shape.CreateSphere(ARGB(100, 0, 150, 255), wireframeSphereFlags, spawnPos, m_fCollisionRadius);
			if (collisionSphere)
				m_aDebugShapes.Insert(collisionSphere);

			// Green arrow showing vehicle orientation
			vector lineEnd = spawnPos + (vehicleForwardDir * 4.0); // 4 meter line
			ref Shape orientationArrow = Shape.CreateArrow(spawnPos, lineEnd, 0.3, ARGB(255, 0, 255, 0), arrowFlags);
			if (orientationArrow)
				m_aDebugShapes.Insert(orientationArrow);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear all debug shapes
	protected void ClearDebugShapes()
	{
		// In Arma Reforger, shapes are automatically cleaned up by the engine
		// We just need to clear our references and reset the flag
		if (m_aDebugShapes)
		{
			m_aDebugShapes.Clear();
		}
		
		// Reset creation flag
		m_bDebugShapesCreated = false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force refresh of debug shapes (useful when depot settings change)
	void RefreshDebugShapes()
	{
		if (m_bEnableSpawnVisuals && GetOwner())
		{
			// Clear existing shapes first
			ClearDebugShapes();
			// Recreate them on next update
			m_bDebugShapesCreated = false;
		}
	}
	#endif
}
