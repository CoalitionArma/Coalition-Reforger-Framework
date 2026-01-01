modded class ParachuteComponent
{
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override void RpcAskDeployParachute() // ← new parameter
	{
		if (m_bParachuteDeployed || !m_ParachuteItem || !pilotEntity || m_ParachuteItem.GetParachuteUsed())
			return;

		/* spawn with setup TRUE so components self‑insert */
		EntitySpawnParams p = new EntitySpawnParams;
		p.TransformMode = ETransformMode.WORLD;
		pilotEntity.GetWorldTransform(p.Transform);

		IEntity parachuteEntity = GetGame().SpawnEntityPrefabEx(
			m_ParachuteItem.GetParachutePrefab(),
			/*setup*/ false,
			GetGame().GetWorld(),
			p);
		m_DeployedParachute = ParachuteDeployedEntity.Cast(parachuteEntity);

		giveOwnershipToClient();

		vector v0 = pilotEntity.GetPhysics().GetVelocity();

		GetGame().GetCallqueue().CallLater(RpcAskSyncReplication, 100, false, m_DeployedParachute.GetRplId());
		GetGame().GetCallqueue().CallLater(RpcAskSyncVelocity, 150, false, v0);

		m_bParachuteDeployed = true;
	}
	
	/* 1)  NEW SERVER EXIT / DAMAGE RPC  */
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	override void Rpc_ServerExitParachute(float velocityAtExit)
	{
		if (!m_DeployedParachute || !pilotEntity || !m_bParachuteDeployed)
			return;

		/* damage logic – ONLY on server */
		if (velocityAtExit >= m_fHardLandingVelocity && velocityAtExit < m_fDeathLandingVelocity)
			Rpc_ServerBreakLegs();
		else if (velocityAtExit >= m_fDeathLandingVelocity)
			Rpc_ServerKillPlayer();

		m_CompartmentAccess.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.LEAVE_OPEN, true, true);

		GetGame().GetCallqueue().CallLater(DeleteParachuteEntity, 200, false, m_DeployedParachute);
		m_DeployedParachute = null;
		m_bParachuteDeployed = false;
		Replication.BumpMe();
		
		GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 1000, false, m_ParachuteItem.GetOwner());
	}
}