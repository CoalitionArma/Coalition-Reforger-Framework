modded class SCR_GroupsManagerComponent
{
	//------------------------------------------------------------------------------------------------
	void AssignGroupIDUnprotected(SCR_AIGroup group)
	{
		group.SetGroupID(m_iLatestGroupID);
		m_iLatestGroupID++;
	}
	
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] playerID
	//! \param[in] previousGroupID
	//! \param[in] newGroupID
	//! \return
	override int MovePlayerToGroup(int playerID, int previousGroupID, int newGroupID)
	{
		m_iMovingPlayerToGroupID = newGroupID;
		SCR_AIGroup previousGroup = FindGroup(previousGroupID);
		if (previousGroup)
			previousGroup.RemovePlayer(playerID);
		
		SCR_AIGroup newGroup = FindGroup(newGroupID);
		if (newGroup)
		{
			if (newGroup.IsFull())
			{
				m_iMovingPlayerToGroupID = -1;
				return -1;
			}	
			
			if (CRF_SlottingManager.GetInstance() && CRF_RplToAuthorityManager.GetInstance())
			{
				int slotID = CRF_SlottingManager.GetInstance().GetPlayerSlotID(playerID);
				
				if (slotID != 0)
					CRF_SlottingManager.GetInstance().UpdateSlotGroup(slotID, RplComponent.Cast(newGroup.FindComponent(RplComponent)).Id());
			};
			
			newGroup.AddPlayer(playerID);
			m_iMovingPlayerToGroupID = -1;
			return newGroupID;
		}
		else
		{
			m_iMovingPlayerToGroupID = -1;
			return -1;
		}
	}
}