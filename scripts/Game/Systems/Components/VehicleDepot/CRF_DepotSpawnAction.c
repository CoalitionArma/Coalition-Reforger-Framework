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
	protected float m_fTextUpdateInterval = 3.0; // Update every 3 seconds for live supply updates
	
	// Client-side throttling to prevent spamming
	protected float m_fLastNotification = 0;
	
	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//! Debug print wrapper to include consistent formatting (Workbench only - zero runtime overhead)
	protected void DebugPrint(string message)
	{
		if (m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging)
		{
			string depotName = "";
			if (m_DepotComponent)
			{
				string fullDepotName = m_DepotComponent.GetDepotName();
				if (fullDepotName != "")
					depotName = string.Format(" - %1", fullDepotName);
			}
			Print(string.Format("[CRF_DepotSpawnAction%1] %2", depotName, message));
		}
	}
	#endif
	
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
			#ifdef WORKBENCH
			// Only show action registration in debug mode
			DebugPrint(string.Format("Initialized action for vehicle %1: %2", m_iVehicleIndex, m_Vehicle.m_sVehicleName));
			#endif
		}
		else
		{
			#ifdef WORKBENCH
			// Only show a condensed message for invalid indices (and only once) when debug is enabled
			if (m_iVehicleIndex == vehicles.Count() && m_DepotComponent && m_DepotComponent.m_bEnableDebugLogging)
			{
				// Show brief summary with specific unused action range
				DebugPrint(string.Format("SETUP: %1 vehicles configured.", vehicles.Count()));
			}
			#endif
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
		
		#ifdef WORKBENCH
		DebugPrint(string.Format("Attempting to spawn %1 for player %2", m_Vehicle.m_sVehicleName, playerId));
		#endif
		
		// Check if player can afford the vehicle
		if (!m_DepotComponent.CanAffordVehicle(playerId, m_Vehicle, m_iVehicleIndex))
		{
			#ifdef WORKBENCH
			DebugPrint("Player cannot afford vehicle");
			#endif
			
			// Log specific error message based on cost type for debugging
			string errorMsg;
			switch (m_Vehicle.m_eCostType)
			{
				case CRF_EVehicleDepotCostType.TICKETS:
					errorMsg = string.Format("Insufficient tickets! Need %1, have %2", m_Vehicle.m_iCost, m_DepotComponent.GetRemainingTickets(playerId));
					break;
				case CRF_EVehicleDepotCostType.SUPPLIES:
					errorMsg = string.Format("Insufficient supplies! Need %1, have %2", m_Vehicle.m_iCost, m_DepotComponent.GetAggregatedSupplies());
					break;
				case CRF_EVehicleDepotCostType.USES:
					errorMsg = string.Format("Insufficient uses! Need %1, have %2", m_Vehicle.m_iCost, m_DepotComponent.GetUsesRemaining());
					break;
			}
			
			#ifdef WORKBENCH
			// Show error details only in debug mode
			DebugPrint(errorMsg);
			#endif
			
			return;
		}
		
		// Use CRF RPC pattern for dedicated server support
		CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
		if (rplManager)
		{

			
			// Get the depot entity's RplId for server communication
			RplComponent rplComponent = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
			if (rplComponent)
			{
				RplId depotRplId = rplComponent.Id();
				
				// Send RPC to server to spawn the vehicle - this ensures it works on dedicated servers
				rplManager.RequestVehicleDepotInteraction(playerId, m_iVehicleIndex, depotRplId);
			}
			else
			{
				// Fallback: direct spawn when no replication available
				bool success = m_DepotComponent.SpawnVehicle(playerId, m_iVehicleIndex);
			}
		}
		else
		{
			// Fallback: direct spawn (for local testing)
			bool success = m_DepotComponent.SpawnVehicle(playerId, m_iVehicleIndex);
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
		
		float currentTime = GetGame().GetWorld().GetWorldTime() * 0.001; // Convert to seconds
		
		// CRITICAL: Notify depot of viewing player for supply refresh (only for supply-based vehicles)
		if (m_Vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
		{
			// ! CLIENT SIDE SUPPLY UPDATE THROTTLE - Per-player clientside limit (fires every frame otherwise)
			if (currentTime - m_fLastNotification >= 12.0)
			{
				m_fLastNotification = currentTime;
				
				// Use consolidated spawn RPC which now includes supply refresh functionality  
				CRF_RplToAuthorityManager rplManager = CRF_RplToAuthorityManager.GetInstance();
				if (rplManager)
				{
					RplComponent rplComponent = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
					if (rplComponent)
					{
						RplId depotRplId = rplComponent.Id();
						// Send viewing notification using interaction RPC with special index (-1 = refresh only)
						rplManager.RequestVehicleDepotInteraction(-1, -1, depotRplId);
					}
				}
			}
		}
		
		// For supply-based vehicles, refresh text more frequently to show live aggregated amounts
		float textUpdateInterval = m_fTextUpdateInterval;
		if (m_Vehicle.m_eCostType == CRF_EVehicleDepotCostType.SUPPLIES)
		{
			textUpdateInterval = 1.0; // Update every 1 seconds for smoother supply amounts when menu is active
		}
		
		// Only update text if enough time has passed or cache is empty
		if (currentTime - m_fLastTextUpdate >= textUpdateInterval || m_sCachedDisplayText.IsEmpty())
		{
			// Use direct depot method for real-time display (server uses live supplies, client uses replicated)
			m_sCachedDisplayText = m_DepotComponent.GetActionText(m_iVehicleIndex, m_Vehicle);
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
