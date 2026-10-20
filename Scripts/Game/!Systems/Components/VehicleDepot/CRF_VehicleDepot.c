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
	
	[Attribute("10", UIWidgets.SpinBox, "Cost/Amount (TICKETS/SUPPLIES: cost to deduct, USES: total available spawns for this vehicle, -1 = unlimited)", "-1 9999 1", category: "Cost Configuration")]
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
	
	[Attribute("1.0", UIWidgets.Slider, "Spawn clearance scale - grows the vehicle sized box checked for obstructions at each slot. 1.0 fits a truck. Scales every axis except downward, so the box never sinks into the ground.", "0.5 3.0 0.1", category: "Spawn Configuration")]
	protected float m_fClearanceScale;
	
	[Attribute("0", UIWidgets.CheckBox, "Spawn vehicles level with the depot instead of snapping them to the terrain height under each slot. For pads, decks and bridges where the terrain sits well below the depot.", category: "Spawn Configuration")]
	protected bool m_bDisableTerrainSnapping;
	
	[Attribute("0", UIWidgets.CheckBox, "Skip the spawn clearance check and spawn regardless of what is in the slot.", category: "Spawn Configuration")]
	protected bool m_bDisableSpawnCollision;
	
	// Grid Pattern Configuration (only applies when GRID pattern is selected)
	[Attribute("3", UIWidgets.SpinBox, "Grid spawn pattern columns (vehicles per row)", "1 10 1", category: "Spawn Configuration")]
	protected int m_iGridColumns;
	
	[Attribute("8", UIWidgets.Slider, "Grid spawn pattern Row spacing (meters between rows)", "3 20 0.5", category: "Spawn Configuration")]
	protected float m_fGridRowSpacing;

	// Spawn visualization (only visible in Workbench editor)
	[Attribute("0", UIWidgets.CheckBox, "Show debug spawn point visuals", category: "Spawn Configuration")]
	bool m_bEnableSpawnVisuals;
	
	// Resource Configuration
	[Attribute("10.0", UIWidgets.SpinBox, "Supply storage search radius (meters)", "10.0 200.0 5.0", category: "Resource Configuration")]
	protected float m_fSupplySearchRadius;
	
	// Vehicle Configuration
	[Attribute("", UIWidgets.Object, "Available vehicles", category: "Vehicle Configuration")]
	protected ref array<ref CRF_VehicleDepotVehicle> m_aVehicles;
	
	// Per vehicle USES spawn type tracking - only created if USES vehicles exist
	[RplProp()]
	protected ref array<int> m_aVehicleSpawnUses;
	
	// Track aggregated supplies for real-time UI updates (replicated from server)
	[RplProp()]
	protected int m_iAggregatedSupplies;
	
	// Server timer for supply refresh
	protected float m_fLastSupplyRefresh = 0;
	
	// Component references
	protected COA_RespawnManager m_RespawnManager;
	protected COA_SlottingManager m_SlottingManager;
	
	// Debug/Collision Bounding Box Shape Size
	protected static const vector CLEARANCE_MINS = "-1.9 0.15 -3.5";
	protected static const vector CLEARANCE_MAXS = "1.9 2.6 3.5";
	
	//! Slots cache ground-anchored, so lift on spawn and let it settle
	protected static const float SPAWN_HEIGHT_OFFSET = 1.0;
	
	// Performance optimization caches
	protected ref array<vector> m_aCachedSpawnPositions;
	protected bool m_bSpawnPositionsCached = false;
	
	// Role restrictions
	[Attribute("0", UIWidgets.CheckBox, "Restrict to leadership roles only (Squad Leaders, Team Leaders, Platoon Leaders, Platoon Sergeants)")]
	bool m_bRestrictToLeadership;
	
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
			
		// Initialize per-vehicle uses tracking
		InitializeVehicleUses();
		
		// Initialize aggregated supplies to 0 (will be updated by server)
		m_iAggregatedSupplies = 0;
		
		// Initialize supply refresh timer
		m_fLastSupplyRefresh = 0;
		
		// Get CRF managers
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
		{
			m_RespawnManager = COA_RespawnManager.Cast(gameMode.FindComponent(COA_RespawnManager));
			m_SlottingManager = COA_SlottingManager.Cast(gameMode.FindComponent(COA_SlottingManager));
		}
		
		// Set up interaction
		SetEventMask(owner, EntityEvent.INIT);
		
		// Schedule initial supply aggregation after initialization (single call, non-looped)
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(UpdateAggregatedSupplies, 2000, false); // Single update after 2 seconds
		}
		
		// NOTE: Removed automatic proximity checking system (replaced with menu-triggered approach for better performance)
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		// Pre-cache spawn positions during initialization
		GetGame().GetCallqueue().CallLater(InitializeSpawnPositions, 1000, false);
		
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize spawn positions during component startup (delayed to ensure world is ready)
	protected void InitializeSpawnPositions()
	{
		if (!GetOwner())
			return;
			
		CacheSpawnPositions();
		#ifdef WORKBENCH
		DebugPrint("Spawn positions pre-cached during initialization");
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initialize per-vehicle uses tracking (optimized - only creates arrays if USES vehicles exist)
	protected void InitializeVehicleUses()
	{
		// USES vehicle detection pass to prevent unnecessary array creation
		int usesVehicleCount = 0;
		for (int i = 0; i < m_aVehicles.Count(); i++)
		{
			CRF_VehicleDepotVehicle vehicle = m_aVehicles[i];
			if (vehicle && vehicle.m_eCostType == CRF_EVehicleDepotCostType.USES)
				usesVehicleCount++;
		}
		
		// Only create array if we have USES vehicles and clear when empty
		if (usesVehicleCount > 0)
		{
			if (!m_aVehicleSpawnUses)
				m_aVehicleSpawnUses = {};
				
			m_aVehicleSpawnUses.Clear();
			
			// Populate array only with USES vehicles (in order they appear)
			for (int i = 0; i < m_aVehicles.Count(); i++)
			{
				CRF_VehicleDepotVehicle vehicle = m_aVehicles[i];
				if (vehicle && vehicle.m_eCostType == CRF_EVehicleDepotCostType.USES)
				{
					// Store the vehicle's initial uses
					m_aVehicleSpawnUses.Insert(vehicle.m_iCost);
				}
			}
		}
		else
		{
			// No USES vehicles - clear array to save memory
			m_aVehicleSpawnUses = null;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Convert vehicle index to uses array index (returns -1 if vehicle is not USES type or array doesn't exist)
	protected int GetUsesArrayIndex(int vehicleIndex)
	{
		if (!m_aVehicleSpawnUses || vehicleIndex < 0 || vehicleIndex >= m_aVehicles.Count())
			return -1;
			
		// Count USES vehicles up to the target index
		int usesArrayIndex = 0;
		for (int i = 0; i <= vehicleIndex; i++)
		{
			CRF_VehicleDepotVehicle vehicle = m_aVehicles[i];
			if (vehicle && vehicle.m_eCostType == CRF_EVehicleDepotCostType.USES)
			{
				if (i == vehicleIndex)
					return usesArrayIndex; // Found target vehicle
				usesArrayIndex++;
			}
		}
		
		return -1; // Target vehicle is not USES type
	}
	
	//------------------------------------------------------------------------------------------------
	//! Update aggregated supplies on depot
	protected void UpdateAggregatedSupplies()
	{
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		int newAggregatedSupplies = GetLiveAggregatedSupplies();
		
		// Only replicate if value changed to reduce network traffic
		if (newAggregatedSupplies != m_iAggregatedSupplies)
		{
			m_iAggregatedSupplies = newAggregatedSupplies;
			Replication.BumpMe();
			
			#ifdef WORKBENCH
			DebugPrint(string.Format("Server updated aggregated supplies to %1", m_iAggregatedSupplies));
			#endif
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Simple method called by DepotSpawnAction when viewed
	void NotifyPlayerViewing()
	{
		if (!Replication.IsServer())
			return;
			
		#ifdef WORKBENCH
		DebugPrint("Player actively viewing depot menu");
		#endif
		
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001;
		if (currentTime - m_fLastSupplyRefresh >= 10.0) // ! SERVER SIDE SUPPLY UPDATE THROTTLE - Prevents multiple clients overwhelming server
		{
			m_fLastSupplyRefresh = currentTime;
			
			// Refresh supplies
			int liveSupplies = GetLiveAggregatedSupplies();
			if (liveSupplies != m_iAggregatedSupplies)
			{
				#ifdef WORKBENCH
				DebugPrint(string.Format("Supply refresh: %1 to %2", m_iAggregatedSupplies, liveSupplies));
				#endif
					
				m_iAggregatedSupplies = liveSupplies;
				Replication.BumpMe();
			}
		}
	}
	
	//================================================================================================
	// SUPPLY DETECTION SYSTEM
	//================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Check if this depot has any supply-dependent vehicles (internal use)
	protected bool HasSupplyDependentVehicles()
	{
		if (!m_aVehicles)
			return false;
			
		foreach (CRF_VehicleDepotVehicle vehicle : m_aVehicles)
		{
			if (vehicle && vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
				return true;
		}
		
		return false;
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
	bool CanAffordVehicle(int playerId, CRF_VehicleDepotVehicle vehicle, int vehicleIndex, int liveSupplies = -1)
	{
		if (!vehicle)
			return false;
			
		// Check role restrictions first
		if (m_bRestrictToLeadership && !HasRequiredLeadershipRole(playerId))
			return false;
		
		// For supply-cost vehicles, we now rely on the replicated m_iAggregatedSupplies value
		// which gets updated via the menu notification system (NotifyPlayerViewing)
		// No need to do additional supply searches here - reduces cascade calls during spawning
			
		switch (vehicle.m_eCostType)
		{
			case CRF_EVehicleDepotCostType.TICKETS:
				return CanAffordTickets(playerId, vehicle.m_iCost);
				
			case CRF_EVehicleDepotCostType.SUPPLIES:
				return CanAffordSupplies(vehicle.m_iCost, liveSupplies);
				
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
		COA_SlotData slotData = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return false;
			
		// Check if player has leadership slot type
		COA_ESlotType slotType = slotData.GetSlotType();
		if (slotType == COA_ESlotType.SQUAD_LEADER || slotType == COA_ESlotType.TEAM_LEADER)
			return true;
			
		// Also check the player's gear role for additional leadership roles like Platoon Leader/Sergeant
		ResourceName playerResource = slotData.GetSlotResource();
		if (playerResource.IsEmpty())
			return false;
			
		COA_EGearRole gearRole = COA_RoleHelper.ResourceToRole(playerResource);
		
		// Check for additional leadership roles not covered by slot types
		if (gearRole == COA_EGearRole.PLATOON_LEADER || 
			gearRole == COA_EGearRole.PLATOON_SERGEANT ||
			gearRole == COA_EGearRole.COMPANY_COMMANDER ||
			gearRole == COA_EGearRole.FIRST_SERGEANT)
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
		
		// Get tickets from COA_Gamemode based on faction
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return false;
			
		int currentTickets = respawnManager.GetFactionTickets(factionKey);
		return currentTickets >= cost;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CanAffordSupplies(int cost, int liveSupplies = -1)
	{
		// For server-side spawn operations, use passed live supplies or get them if not provided
		// BUT ONLY when explicitly provided (to avoid UI spam in Workbench)
		if (Replication.IsServer() && liveSupplies != -1)
		{
			return liveSupplies >= cost;
		}
		
		// Always use replicated aggregated amount for UI display and Workbench testing
		return m_iAggregatedSupplies >= cost;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Collects every distinct SUPPLIES container reachable from any qualifying consumer within
	//! m_fSupplySearchRadius, de-duplicated by container reference.
	protected void CollectNearbySupplyContainers(notnull set<SCR_ResourceContainer> outContainers)
	{
		if (!GetOwner() || RplSession.Mode() == RplMode.Client)
			return;

		vector depotPos = GetOwner().GetOrigin();

		// Clear previous results and query entities within search radius
		m_aNearbyEntities.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(depotPos, m_fSupplySearchRadius, QueryEntitiesCallback, null, EQueryEntitiesFlags.ALL);

		foreach (IEntity entity : m_aNearbyEntities)
		{
			if (!entity || entity == GetOwner())
				continue;

			// Check if entity has a resource component
			SCR_ResourceComponent resourceComponent = SCR_ResourceComponent.FindResourceComponent(entity, false);
			if (!resourceComponent)
				continue;

			// Try to get consumer for aggregated value
			SCR_ResourceConsumer consumer = resourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			if (!consumer)
				continue;

			// Force resource grid to rebuild this consumer's own container queue (within its own range)
			GetGame().GetResourceGrid().UpdateInteractor(consumer);

			SCR_ResourceContainerQueueBase containerQueue = consumer.GetContainerQueue();
			if (!containerQueue)
				continue;

			int containerCount = containerQueue.GetContainerCount();
			for (int i = 0; i < containerCount; i++)
			{
				SCR_ResourceContainer container = containerQueue.GetContainerAt(i);
				if (container)
					outContainers.Insert(container); // set<> dedupes by reference automatically
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Main Supply Aggregation Function - Detects live aggregated supplies from all sources in radius (SERVER ONLY)
	protected int GetLiveAggregatedSupplies()
	{
		if (!GetOwner() || RplSession.Mode() == RplMode.Client)
			return 0;

		#ifdef WORKBENCH
		if (m_fSupplySearchRadius)
			DebugPrint(string.Format("Server searching for aggregated supplies in %1m radius...", m_fSupplySearchRadius));
		#endif

		set<SCR_ResourceContainer> containers = new set<SCR_ResourceContainer>();
		CollectNearbySupplyContainers(containers);

		float totalSupplies;
		foreach (SCR_ResourceContainer container : containers)
			totalSupplies += container.GetResourceValue();

		#ifdef WORKBENCH
		DebugPrint(string.Format("Found %1 supplies across %2 distinct containers", totalSupplies, containers.Count()));
		#endif

		return (int)totalSupplies;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool CanAffordUses(int playerId, int vehicleIndex)
	{
		if (vehicleIndex < 0 || vehicleIndex >= m_aVehicles.Count())
			return false;
			
		CRF_VehicleDepotVehicle vehicle = m_aVehicles[vehicleIndex];
		if (!vehicle || vehicle.m_eCostType != CRF_EVehicleDepotCostType.USES)
			return false;
			
		// If no uses array exists, no USES vehicles are configured
		if (!m_aVehicleSpawnUses)
			return false;
			
		int usesArrayIndex = GetUsesArrayIndex(vehicleIndex);
		if (usesArrayIndex == -1)
			return false; // Vehicle not found in uses array
			
		int usesRemaining = m_aVehicleSpawnUses[usesArrayIndex];
		if (usesRemaining == -1)
			return true; // Unlimited uses
			
		// Check if this vehicle has uses remaining
		return usesRemaining > 0;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get remaining uses for a specific vehicle
	int GetVehicleUsesRemaining(int vehicleIndex)
	{
		if (!m_aVehicleSpawnUses)
			return 0; // No uses array means no USES vehicles
			
		int usesArrayIndex = GetUsesArrayIndex(vehicleIndex);
		if (usesArrayIndex == -1)
			return 0; // Vehicle not in uses array or not USES type
			
		return m_aVehicleSpawnUses[usesArrayIndex];
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
			
		// Get tickets from respawn manager which now handles ticket replication
		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
			return 0;
			
		return respawnManager.GetFactionTickets(factionKey);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Get aggregated supplies from all nearby sources (replicated value - updated via RPC)
	int GetAggregatedSupplies()
	{
		// Return replicated value that gets updated via RPC from action menu
		return m_iAggregatedSupplies;
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
	protected void DeductCost(int playerId, CRF_VehicleDepotVehicle vehicle, int vehicleIndex, int liveSupplies = -1)
	{
		switch (vehicle.m_eCostType)
		{
			case CRF_EVehicleDepotCostType.TICKETS:
				DeductTickets(playerId, vehicle.m_iCost);
				break;
				
			case CRF_EVehicleDepotCostType.SUPPLIES:
				// Calculate post-consumption supplies to avoid redundant aggregation calls
				int postConsumptionSupplies;
				if (liveSupplies != -1)
					postConsumptionSupplies = liveSupplies - vehicle.m_iCost;
				else
					postConsumptionSupplies = -1;
				DeductSupplies(vehicle.m_iCost, postConsumptionSupplies);
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
	//! Deducts supplies across the same de-duplicated container set GetLiveAggregatedSupplies() reads
	//! from, so what gets consumed matches what was shown as available. Mirrors the per-container
	//! consumption loop in SCR_ResourceConsumer.RequestConsumtion(), but over containers merged from
	//! every qualifying consumer in radius rather than a single consumer's own queue.
	protected void DeductSupplies(int amount, int postConsumptionSupplies = -1)
	{
		if (!GetOwner() || RplSession.Mode() == RplMode.Client)
			return;

		set<SCR_ResourceContainer> containers = new set<SCR_ResourceContainer>();
		CollectNearbySupplyContainers(containers);

		float remainingCost = amount;
		float resourceUsed;
		SCR_ResourceEncapsulator encapsulator;

		foreach (SCR_ResourceContainer container : containers)
		{
			if (remainingCost <= 0)
				break;

			resourceUsed = Math.Min(remainingCost, container.GetResourceValue());
			if (resourceUsed <= 0)
				continue;

			remainingCost -= resourceUsed;

			encapsulator = container.GetResourceEncapsulator();
			if (encapsulator)
				encapsulator.RequestConsumtion(resourceUsed);
			else
				container.DecreaseResourceValue(resourceUsed);
		}

		#ifdef WORKBENCH
		if (remainingCost > 0)
			DebugPrint(string.Format("DeductSupplies: only found %1 of the requested %2 supplies across %3 containers", amount - remainingCost, amount, containers.Count()));
		else
			DebugPrint(string.Format("Successfully consumed %1 supplies across %2 distinct containers", amount, containers.Count()));
		#endif

		// Use pre-calculated post-consumption value if provided, otherwise calculate
		if (postConsumptionSupplies == -1)
			postConsumptionSupplies = GetLiveAggregatedSupplies();

		// Immediately update cached value to prevent exploitation during delay
		m_iAggregatedSupplies = postConsumptionSupplies;
		Replication.BumpMe();

		// Cancel any pending delayed update since we just updated
		GetGame().GetCallqueue().Remove(UpdateAggregatedSupplies);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void DeductUses(int vehicleIndex)
	{
		if (vehicleIndex < 0 || vehicleIndex >= m_aVehicles.Count() || !m_aVehicleSpawnUses)
			return;
			
		CRF_VehicleDepotVehicle vehicle = m_aVehicles[vehicleIndex];
		if (vehicle && vehicle.m_eCostType == CRF_EVehicleDepotCostType.USES)
		{
			int usesArrayIndex = GetUsesArrayIndex(vehicleIndex);
			if (usesArrayIndex == -1)
				return; // Vehicle not in uses array
				
			int usesRemaining = m_aVehicleSpawnUses[usesArrayIndex];
			
			// Check for unlimited uses (uses remaining of -1)
			if (usesRemaining == -1)
			{
				return;
			}
			
			// Deduct 1 use from this specific vehicle's pool
			if (usesRemaining > 0)
			{
				m_aVehicleSpawnUses[usesArrayIndex] = usesRemaining - 1;
				#ifdef WORKBENCH
				DebugPrint(string.Format("Deducted 1 use from vehicle %1 (%2), %3 remaining", vehicleIndex, vehicle.m_sVehicleName, m_aVehicleSpawnUses[usesArrayIndex]));
				#endif
				
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
		
		// For supply-based vehicles on server, get live supplies once to avoid cascading calls
		int liveSupplies = -1;
		if (Replication.IsServer() && vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
		{
			liveSupplies = GetLiveAggregatedSupplies();
		}
		
		// Check if player can afford it (passing live supplies to avoid recalculation)
		if (!CanAffordVehicle(playerId, vehicle, vehicleIndex, liveSupplies))
		{
			// Send notification to player explaining why they can't spawn
			if (m_bRestrictToLeadership && !HasRequiredLeadershipRole(playerId))
			{
				ShowNotificationToPlayer(playerId, "Access Denied", "You lack the required leadership role to spawn vehicles");
			}
			else if (vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
			{
				string message = string.Format("Insufficient supplies! Need %1, but only %2 available", vehicle.m_iCost, liveSupplies);
				ShowNotificationToPlayer(playerId, "Spawn Failed", message);
			}
			else if (vehicle.m_eCostType == CRF_EVehicleDepotCostType.TICKETS)
			{
				ShowNotificationToPlayer(playerId, "Spawn Failed", string.Format("Insufficient tickets! Need %1 tickets", vehicle.m_iCost));
			}
			else if (vehicle.m_eCostType == CRF_EVehicleDepotCostType.USES)
			{
				int usesRemaining = GetVehicleUsesRemaining(vehicleIndex);
				ShowNotificationToPlayer(playerId, "Spawn Failed", string.Format("Insufficient uses! Need 1, but only %1 remaining", usesRemaining));
			}
			else
			{
				ShowNotificationToPlayer(playerId, "Spawn Failed", "Cannot afford this vehicle");
			}
			return false;
		}
		
		// Find spawn position
		vector spawnPos = FindSpawnPosition();
		if (spawnPos == vector.Zero)
		{
			ShowNotificationToPlayer(playerId, "No space available", "Vehicle spawning blocked by obstacles.");
			return false;
		}
		
		// Spawn the vehicle
		#ifdef WORKBENCH
		DebugPrint(string.Format("DEBUG: Vehicle name='%1', prefab='%2', cost=%3, costType=%4", 
			vehicle.m_sVehicleName, vehicle.m_sVehiclePrefab, vehicle.m_iCost, vehicle.m_eCostType));
		#endif
			
		Resource resource = Resource.Load(vehicle.m_sVehiclePrefab);
		if (!resource)
		{
			return false;
		}
		
		// Same transform the test and preview used, lifted clear of the ground
		EntitySpawnParams spawnParams = EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		BuildSlotTransform(spawnPos, spawnParams.Transform);
		spawnParams.Transform[3] = spawnPos + Vector(0, GetSpawnLift(), 0);
		
		IEntity spawnedVehicle = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), spawnParams);
		if (!spawnedVehicle)
		{
			return false;
		}
		
		// Deduct cost (passing live supplies to avoid redundant aggregation calls)
		DeductCost(playerId, vehicle, vehicleIndex, liveSupplies);

		// Notify success
		string costInfo = GetCostInfoText(vehicle);
		string message = string.Format("Spawned %1 for %2", vehicle.m_sVehicleName, costInfo);
		ShowNotificationToPlayer(playerId, "Vehicle Spawned", message);

		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected vector FindSpawnPosition()
	{
		// Spawn positions should already be cached during initialization
		// Simple check without fallback - if not cached, something went wrong during init
		if (!m_bSpawnPositionsCached || m_aCachedSpawnPositions.Count() == 0)
		{
			#ifdef WORKBENCH
			DebugPrint("ERROR: Spawn positions not cached! Initialization failed.");
			#endif
			return vector.Zero;
		}
		
		// Try all cached spawn positions
		for (int i = 0; i < m_aCachedSpawnPositions.Count(); i++)
		{
			vector testPos = m_aCachedSpawnPositions[i];
	
			// Room for a vehicle? Protection off takes the first slot regardless
			if (m_bDisableSpawnCollision || IsSlotClear(testPos))
			{
				#ifdef WORKBENCH
				string patternName;
				if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.LINE)
					patternName = "LINE";
				else if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.ROW)
					patternName = "ROW";
				else
					patternName = "GRID";
				DebugPrint(string.Format("Found spawn position in %1 pattern at slot %2", patternName, i));
				#endif
				return testPos;
			}
			#ifdef WORKBENCH
			else
			{
				DebugPrint(string.Format("Spawn slot %1 is blocked", i));
			}
			#endif
		}
		
		#ifdef WORKBENCH
		string patternName;
		if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.LINE)
			patternName = "behind";
		else if (m_eSpawnPattern == CRF_EVehicleDepotSpawnPattern.ROW)
			patternName = "beside";
		else
			patternName = "in grid around";
		DebugPrint(string.Format("All spawn slots %1 depot are blocked!", patternName));
		#endif
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
			
			// Ground-anchored so test, preview and spawn share this position
			spawnPos[1] = GetSlotHeight(spawnPos, depotPos, world);
			
			m_aCachedSpawnPositions.Insert(spawnPos);
		}
		
		m_bSpawnPositionsCached = true;
		#ifdef WORKBENCH
		DebugPrint(string.Format("Cached %1 spawn positions", m_aCachedSpawnPositions.Count()));
		#endif
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
		#ifdef WORKBENCH
		DebugPrint("Spawn position cache invalidated - will recalculate on next use");
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force immediate recalculation of spawn positions (useful for editor changes or runtime updates)
	void RecalculateSpawnPositions()
	{
		// Directly recalculate without going through invalidate/cache cycle
		CacheSpawnPositions();
		#ifdef WORKBENCH
		DebugPrint("Spawn positions forcibly recalculated");
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	//! Slot height, depot level when terrain snapping is off
	protected float GetSlotHeight(vector slotPos, vector depotPos, BaseWorld world)
	{
		if (m_bDisableTerrainSnapping || !world)
			return depotPos[1];
		
		return world.GetSurfaceY(slotPos[0], slotPos[2]);
	}
	
	//------------------------------------------------------------------------------------------------
	//! No lift when placing level with the depot, the slot is already where it should sit
	protected float GetSpawnLift()
	{
		if (m_bDisableTerrainSnapping)
			return 0;
		
		return SPAWN_HEIGHT_OFFSET;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Ground-anchored slot transform, depot yaw levelled plus the facing offset
	//! Shared by the test, the spawn and the preview so they can't disagree
	protected void BuildSlotTransform(vector groundPos, out vector mat[4])
	{
		vector depotTransform[4];
		GetOwner().GetWorldTransform(depotTransform);
		
		// Strip pitch off the placed depot
		vector forwardDir = depotTransform[2];
		forwardDir[1] = 0;
		forwardDir = forwardDir.Normalized();
		
		// Facing offset around world up
		if (m_fVehicleFacingOffset != 0)
		{
			float offsetRadians = m_fVehicleFacingOffset * Math.DEG2RAD;
			float cosOffset = Math.Cos(offsetRadians);
			float sinOffset = Math.Sin(offsetRadians);
			
			vector rotatedForward;
			rotatedForward[0] = forwardDir[0] * cosOffset + forwardDir[2] * sinOffset;
			rotatedForward[1] = forwardDir[1];
			rotatedForward[2] = -forwardDir[0] * sinOffset + forwardDir[2] * cosOffset;
			
			forwardDir = rotatedForward.Normalized();
		}
		
		vector upDir = "0 1 0";
		vector rightDir = upDir * forwardDir; // Cross product for right direction
		rightDir = rightDir.Normalized();
	
		mat[0] = rightDir;
		mat[1] = upDir;
		mat[2] = forwardDir;
		mat[3] = groundPos;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Uses TraceOBB (perf difference unknown), same as vanilla's vehicle slots
	//! See SCR_EntitySpawnerSlotComponent.IsOccupied and SCR_ItemPlacementComponent for implementation in vanilla 
	//! Hits collision geometry not render bounds, so a trunk blocks but its canopy doesn't
	protected bool IsSlotClear(vector groundPos)
	{
		IEntity owner = GetOwner();
		if (!owner)
			return false;
		
		BaseWorld world = owner.GetWorld();
		if (!world)
			return false;
		
		vector slotTransform[4];
		BuildSlotTransform(groundPos, slotTransform);
		
		vector mins, maxs;
		GetClearanceBounds(mins, maxs);
		
		TraceOBB trace = new TraceOBB();
		trace.Mat[0] = slotTransform[0];
		trace.Mat[1] = slotTransform[1];
		trace.Mat[2] = slotTransform[2];
		trace.Start = groundPos;
		trace.Mins = mins;
		trace.Maxs = maxs;
		trace.Flags = TraceFlags.ENTS; // Entities only, terrain height comes from GetSurfaceY
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		trace.Exclude = owner; // Depot carries its own static collision
		
		world.TracePosition(trace, ClearanceTraceFilter);

		if (m_bEnableDebugLogging && trace.TraceEnt)
			DebugPrint(string.Format("Slot at %1 blocked by %2", groundPos, trace.TraceEnt));

		return trace.TraceEnt == null;
	}
	
	protected void GetClearanceBounds(out vector mins, out vector maxs)
	{
		float scale = m_fClearanceScale;
		if (scale <= 0)
			scale = 1.0;
		
		mins = Vector(CLEARANCE_MINS[0] * scale, CLEARANCE_MINS[1], CLEARANCE_MINS[2] * scale);
		maxs = Vector(CLEARANCE_MAXS[0] * scale, CLEARANCE_MAXS[1] * scale, CLEARANCE_MAXS[2] * scale);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Filter to ignore certain entities (inventory items mainly)
	protected bool ClearanceTraceFilter(IEntity ent)
	{
		// Vehicles and characters always block, whatever else they carry
		// Vehicles hold an InventoryItemComponent, so the check below would drop them
		if (ent.IsInherited(Vehicle) || ent.IsInherited(ChimeraCharacter))
			return true;

		// Inventory item components ignored by collision trace
		return !ent.FindComponent(InventoryItemComponent);
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
	//! Get action text for display (replaces multiple wrapper methods)
	string GetActionText(int vehicleIndex, CRF_VehicleDepotVehicle vehicle)
	{
		if (!vehicle || vehicleIndex < 0)
			return "Invalid Vehicle";
			
		// Generate text using live supply data (server uses live, client uses replicated)
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
			case CRF_EVehicleDepotCostType.USES: return "1 Use";
		}
		
		return "unknown cost";
	}
	
	//------------------------------------------------------------------------------------------------
	//! Generate action text (internal method, use GetActionText instead)
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
				// Use direct aggregated supplies method
				int remaining = GetAggregatedSupplies();
				costText = string.Format("%1 Supplies", vehicle.m_iCost);
				remainingText = string.Format("(%1 left)", remaining);
				break;
			}
			case CRF_EVehicleDepotCostType.USES:
			{
				int remaining = GetVehicleUsesRemaining(vehicleIndex);
				if (remaining == -1)
				{
					costText = "Unlimited Uses";
					remainingText = "";
				}
				else
				{
					costText = "1 Use";
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
	 * Show notification to player (replicated from server to client, with Workbench support)
	 */
	protected void ShowNotificationToPlayer(int playerId, string title, string message)
	{
		// In Workbench or single-player testing, directly show the notification
		#ifdef WORKBENCH
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId == playerId)
		{
			SCR_PopUpNotification popUpNotification = SCR_PopUpNotification.GetInstance();
			if (popUpNotification)
			{
				popUpNotification.PopupMsg(message, 3.0, title);
			}
		}
		return;
		#endif
		
		// Only send RPC notification if running on server in multiplayer
		if (RplSession.Mode() == RplMode.Client)
			return;
			
		// Send RPC with player ID so client can filter
		Rpc(RPC_ShowNotification, playerId, title, message);
		
	}

	//================================================================================================
	// DEBUG FUNCTIONS - Visualization and Logging
	//================================================================================================


	protected void DebugPrint(string message)
	{
		if (m_bEnableDebugLogging)
		{
			Print(string.Format("[CRF_VehicleDepot - %1] %2", m_sDepotName, message));
		}
	}
	
	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//! Draws spawn debug visuals
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		// Only draw if debug visuals are enabled
		if (!m_bEnableSpawnVisuals)
			return;
			
		if (!owner)
			return;
		
		CreateDebugShapes(owner);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Rebuild the shapes for every slot
	protected void CreateDebugShapes(IEntity owner)
	{
		m_aDebugShapes.Clear();
		
		vector depotPos = owner.GetOrigin();
		vector transform[4];
		owner.GetWorldTransform(transform);
		vector forwardDir = transform[2]; // Forward direction (Z-axis)
		vector rightDir = transform[0];   // Right direction (X-axis)
		
		BaseWorld world = owner.GetWorld();
		
		// Shape flags for consistent rendering
		const ShapeFlags slotMarkerFlags = ShapeFlags.VISIBLE | ShapeFlags.NOZBUFFER | ShapeFlags.NOOUTLINE;
		const ShapeFlags clearanceBoxFlags = ShapeFlags.VISIBLE | ShapeFlags.NOZBUFFER | ShapeFlags.WIREFRAME | ShapeFlags.DOUBLESIDE;
		const ShapeFlags arrowFlags = ShapeFlags.VISIBLE | ShapeFlags.NOZBUFFER;
		
		for (int i = 0; i < m_iSpawnPointCount; i++)
		{
			// Same position logic as the cache
			vector slotPos = CalculateSpawnPosition(depotPos, forwardDir, rightDir, i);
			
			slotPos[1] = GetSlotHeight(slotPos, depotPos, world);
			
			// Exact transform and volume the test runs on
			vector slotTransform[4];
			BuildSlotTransform(slotPos, slotTransform);
			
			vector mins, maxs;
			GetClearanceBounds(mins, maxs);
			
			// Visual feedback for spawn slots
			int boxColor;
			if (m_bDisableSpawnCollision)
				boxColor = ARGB(255, 170, 170, 170); // Grey, protection off
			else if (IsSlotClear(slotPos))
				boxColor = ARGB(255, 0, 220, 60); // Green, fits
			else
				boxColor = ARGB(255, 230, 30, 30); // Red, blocked
			
			// Insert first so the array holds the ref, then orient in place
			m_aDebugShapes.Insert(Shape.Create(ShapeType.BBOX, boxColor, clearanceBoxFlags, mins, maxs));
			Shape clearanceBox = m_aDebugShapes[m_aDebugShapes.Count() - 1];
			if (clearanceBox)
				clearanceBox.SetMatrix(slotTransform);
			
			// Facing arrow & spawn position sphere
			vector arrowStart = slotPos + Vector(0, GetSpawnLift(), 0);
			vector arrowEnd = arrowStart + (slotTransform[2] * 4.0); // 4 meter line
			m_aDebugShapes.Insert(Shape.CreateSphere(boxColor, slotMarkerFlags, arrowStart, 0.2));
			m_aDebugShapes.Insert(Shape.CreateArrow(arrowStart, arrowEnd, 0.3, ARGB(255, 0, 255, 0), arrowFlags));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Clear all debug shapes
	protected void ClearDebugShapes()
	{
		if (m_aDebugShapes)
			m_aDebugShapes.Clear();
		
		m_bDebugShapesCreated = false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Force refresh of debug shapes (useful when depot settings change)
	void RefreshDebugShapes()
	{
		if (m_bEnableSpawnVisuals && GetOwner())
			ClearDebugShapes();
	}
	#endif
}
