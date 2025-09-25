//------------------------------------------------------------------------------------------------
//! Individual Vehicle Spawn Action
//! Each vehicle gets its own action for direct spawning
class CRF_DepotSpawnAction : ScriptedUserAction
{
	[Attribute("0", UIWidgets.SpinBox, "Vehicle index in depot array", "0 10 1")]
	protected int m_iVehicleIndex;
	
	// Vehicle data
	protected CRF_VehicleDepotVehicle m_Vehicle;
	protected CRF_VehicleDepot m_DepotComponent;
	
	// Text refresh throttling for optimization
	protected string m_sCachedDisplayText = "";
	protected float m_fLastTextUpdate = 0;
	protected float m_fTextUpdateInterval = 1.0; // Update every 1 second for live supply updates
	
	//------------------------------------------------------------------------------------------------
	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		super.Init(pOwnerEntity, pManagerComponent);
		
		// Get depot component from owner entity
		m_DepotComponent = CRF_VehicleDepot.Cast(pOwnerEntity.FindComponent(CRF_VehicleDepot));
		if (!m_DepotComponent)
		{
			// ERROR messages should always show (not debug-only) since they indicate setup problems
			Print("[CRF_DepotSpawnAction] ERROR: No CRF_VehicleDepot component found on owner entity");
			return;
		}
		
		// Get vehicle data from depot
		array<ref CRF_VehicleDepotVehicle> vehicles = m_DepotComponent.GetVehicles();
		
		if (m_iVehicleIndex >= 0 && m_iVehicleIndex < vehicles.Count())
		{
			m_Vehicle = vehicles[m_iVehicleIndex];
			// Only show action registration in debug mode
			if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Initialized action for vehicle %1: %2", m_iVehicleIndex, m_Vehicle.m_sVehicleName));
		}
		else
		{
			// Only show a condensed message for invalid indices (and only once) when debug is enabled
			if (m_iVehicleIndex == vehicles.Count() && m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging)
			{
				// Show brief summary with specific unused action range
				if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] SETUP: %1 vehicles configured.", vehicles.Count(), vehicles.Count()));
			}
		}

	}
	
	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!m_DepotComponent || !m_Vehicle)
		{
			// ERROR messages should always show (not debug-only) since they indicate setup problems
			Print("[CRF_DepotSpawnAction] ERROR: Missing depot component or vehicle data");
			return;
		}
		
		// Get player ID
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		
		if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Attempting to spawn %1 for player %2", m_Vehicle.m_sVehicleName, playerId));
		
		// Check if player can afford the vehicle
		if (!m_DepotComponent.CanAffordVehicle(playerId, m_Vehicle, m_iVehicleIndex))
		{
			if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print("[CRF_DepotSpawnAction] Player cannot afford vehicle");
			
			// Log specific error message based on cost type for debugging
			string errorMsg;
			switch (m_Vehicle.m_eCostType)
			{
				case CRF_EVehicleDepotCostType.TICKETS:
					errorMsg = string.Format("Insufficient tickets! Need %1, have %2", m_Vehicle.m_iCost, m_DepotComponent.GetRemainingTickets(playerId));
					break;
				case CRF_EVehicleDepotCostType.SUPPLIES:
					errorMsg = string.Format("Insufficient supplies! Need %1, have %2", m_Vehicle.m_iCost, m_DepotComponent.GetRemainingSupplies(playerId));
					break;
				case CRF_EVehicleDepotCostType.USES:
					errorMsg = string.Format("Insufficient uses! Need %1, have %2", m_Vehicle.m_iCost, m_DepotComponent.GetUsesRemaining());
					break;
			}
			
			// Show error details only in debug mode
			if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] %1", errorMsg));
			
			return;
		}
		
		// Use CRF RPC pattern for dedicated server support
		CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
		if (rplManager)
		{
			if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print("[CRF_DepotSpawnAction] RplManager found");
			
			// Get the depot entity's RplId for server communication
			RplComponent rplComponent = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
			if (rplComponent)
			{
				RplId depotRplId = rplComponent.Id();
				if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Depot RplComponent found, RplId: %1", depotRplId));
				
				// Send RPC to server to spawn the vehicle - this ensures it works on dedicated servers
				rplManager.RequestVehicleDepotSpawn(playerId, m_iVehicleIndex, depotRplId);
				if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Sent vehicle depot spawn request via RPC for %1", m_Vehicle.m_sVehicleName));
			}
			else
			{
				if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print("[CRF_DepotSpawnAction] Depot entity missing RplComponent - using fallback direct spawn");
				
				// Fallback: direct spawn when no replication available
				bool success = m_DepotComponent.SpawnVehicle(playerId, m_iVehicleIndex);
				if (success && m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Fallback direct spawn successful: %1", m_Vehicle.m_sVehicleName));
				else if (!success && m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Fallback direct spawn failed: %1", m_Vehicle.m_sVehicleName));
			}
		}
		else
		{
			if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print("[CRF_DepotSpawnAction] CRF_RplToAuthorityManager not available - fallback to direct spawn");
			
			// Fallback: direct spawn (for local testing)
			bool success = m_DepotComponent.SpawnVehicle(playerId, m_iVehicleIndex);
			if (success && m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Direct spawn successful: %1", m_Vehicle.m_sVehicleName));
			else if (!success && m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging) Print(string.Format("[CRF_DepotSpawnAction] Direct spawn failed: %1", m_Vehicle.m_sVehicleName));
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (!m_Vehicle || !m_DepotComponent)
		{
			outName = "Invalid Vehicle";
			return true;
		}
		
		// For supply-based vehicles, refresh more frequently to show live aggregated amounts
		float updateInterval = m_fTextUpdateInterval;
		if (m_Vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
		{
			updateInterval = 0.5; // Update every 500ms for live supply detection when menu is active
		}
		
		// Throttle text updates to prevent excessive calls but allow live supply updates
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001; // Convert to seconds
		
		// Only update text if enough time has passed or cache is empty
		if (currentTime - m_fLastTextUpdate >= updateInterval || m_sCachedDisplayText.IsEmpty())
		{
			m_sCachedDisplayText = m_DepotComponent.GetCachedActionText(m_iVehicleIndex, m_Vehicle);
			m_fLastTextUpdate = currentTime;
		}
		
		// Use cached text for smooth performance
		outName = m_sCachedDisplayText;
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_DepotComponent || !m_Vehicle)
			return false;
			
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!CanBeShownScript(user))
			return false;
			
		// Only allow if player can afford it
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		return m_DepotComponent.CanAffordVehicle(playerId, m_Vehicle, m_iVehicleIndex);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	override bool CanBroadcastScript()
	{
		return false;
	}
}
