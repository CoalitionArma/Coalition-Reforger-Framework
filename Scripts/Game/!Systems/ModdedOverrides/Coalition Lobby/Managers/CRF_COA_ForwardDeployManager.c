//------------------------------------------------------------------------------------------------
//! Hostage Rescue (CRF_HostageRescueGamemodeComponent): the hostages move with the captors' keeper group
//! when its leader forward deploys, and never forward deploy on their own. Without that mode in the
//! mission nothing changes.
modded class COA_ForwardDeployManager
{
	protected bool m_bCRF_MovingHostages;	//!< set while this override queues the hostages' own moves

	//------------------------------------------------------------------------------------------------
	override void CreateForwardDeployRequest(int playerId, vector transform)
	{
		CRF_HostageRescueGamemodeComponent hostageRescue = CRF_HostageRescueGamemodeComponent.GetInstance();
		if (!hostageRescue)
		{
			super.CreateForwardDeployRequest(playerId, transform);
			return;
		}

		// A hostage's group asked for itself (the zone is visible to them so they can be moved into it)
		if (!m_bCRF_MovingHostages && hostageRescue.IsHostage(playerId))
			return;

		super.CreateForwardDeployRequest(playerId, transform);
		if (m_bCRF_MovingHostages)
			return;

		array<int> hostages = {};
		if (!hostageRescue.GetHostagesToMove(playerId, hostages))
			return;

		m_bCRF_MovingHostages = true;
		foreach (int hostageId : hostages)
			CreateForwardDeployRequest(hostageId, transform);
		m_bCRF_MovingHostages = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Hides the map's forward deploy option from the hostages
	override bool IsForwardDeployActive(string factionKey)
	{
		CRF_HostageRescueGamemodeComponent hostageRescue = CRF_HostageRescueGamemodeComponent.GetInstance();
		if (hostageRescue && factionKey == hostageRescue.GetHostageFaction())
			return false;

		return super.IsForwardDeployActive(factionKey);
	}
}
