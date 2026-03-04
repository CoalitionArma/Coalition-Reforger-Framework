modded class SCR_GroupsManagerComponent
{
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
			
			if (CRF_SlottingManager.GetInstance())
			{
				int slotID = CRF_SlottingManager.GetInstance().GetPlayerSlotID(playerID);
				
				if (slotID != -1)
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