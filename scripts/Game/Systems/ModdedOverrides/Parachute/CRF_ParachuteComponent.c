modded class ParachuteComponent
{
	override protected void EnableComponentControls()
	{
		if (!m_InputManager)
			return;

		if (!m_PlayerController)
			return;

		// Only local owner should listen to input
		int localId = SCR_PlayerController.GetLocalPlayerId();
		if (m_PlayerController.GetPlayerId() != localId)
		    return;


		m_InputManager.AddActionListener("ParachuteDeploy", EActionTrigger.DOWN, OnJumpPressed);
	}
	
	override void OnJumpPressed()
	{
		IEntity pilot = GetPilotEntity();
		if (!pilot)
			return;

		// Client-side pre-check for responsiveness
		if (!m_ParachuteItem)
			return;

		if (!MayDeployParachute_Internal(pilot, m_ParachuteItem))
			return;
		
		if (IsAuthority())
			RpcAskDeployParachute();
		else
			Rpc(RpcAskDeployParachute);
	}
	
	override protected bool MayDeployParachute_Internal(IEntity pilot, ParachuteItemComponent item)
	{
		Print(pilot);
		Print(item);
		if (!pilot || !item)
			return false;

		Print(m_bParachuteDeployed);
		if (m_bParachuteDeployed)
			return false;

		Print(item.GetParachuteUsed());
		if (item.GetParachuteUsed())
			return false;

		Print(SCR_ChimeraCharacter.Cast(pilot));
		SCR_ChimeraCharacter pawn = SCR_ChimeraCharacter.Cast(pilot);
		if (!pawn)
			return false;

		if (pawn.IsInVehicle())
			return false;

		// Safe radius query
		m_QueryPilot = pilot;

		float terrainY = SCR_TerrainHelper.GetTerrainY(pawn.GetOrigin(), null, true);
		float heightAGL = pawn.GetOrigin()[1] - terrainY;
		Print(heightAGL);
		Print(terrainY);
		if (heightAGL < m_fMinimumAltitude)
			return false;

		return true;
	}
}