modded class SCR_GroupsManagerComponent
{
	void ConvertIntoPlayableGroup(SCR_AIGroup group, Faction faction)
	{
		if (!group)
			return;
		
		group.DeactivateAI();
		group.SetCanDeleteIfNoPlayer(false);
		group.SetMaxMembers(12);
		group.SetFaction(faction);
		
		RegisterGroup(group);
		AssignGroupFrequency(group);
		AssignGroupID(group);
		
		m_OnPlayableGroupCreated.Invoke(group);
		
		//if there is commanding present, we create the slave group for AIs at the creation of the group
		SCR_CommandingManagerComponent commandingManager = SCR_CommandingManagerComponent.GetInstance();
		if (commandingManager)
		{
			IEntity groupEntity = GetGame().SpawnEntityPrefab(Resource.Load(commandingManager.GetGroupPrefab()));
			if (!groupEntity)
				return;
			
			SCR_AIGroup slaveGroup = SCR_AIGroup.Cast(groupEntity);
			if (!slaveGroup)
				return;
			
			slaveGroup.DeactivateAI();
			
			RplComponent RplComp = RplComponent.Cast(slaveGroup.FindComponent(RplComponent));
			if (!RplComp)
				return;
			
			RplId slaveGroupRplID = RplComp.Id();
			
			RplComp = RplComponent.Cast(group.FindComponent(RplComponent));
			if (!RplComp)
				return;
			
			RequestSetGroupSlave(RplComp.Id(), slaveGroupRplID);
		}
	}
}