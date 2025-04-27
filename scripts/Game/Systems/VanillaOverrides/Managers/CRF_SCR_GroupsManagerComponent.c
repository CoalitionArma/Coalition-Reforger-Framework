modded class SCR_GroupsManagerComponent
{
	void ConvertIntoPlayableGroup(SCR_AIGroup group)
	{
		if (!group)
			return;
		
		group.DeactivateAI();
		group.SetCanDeleteIfNoPlayer(false);
		group.SetMaxMembers(12);

		RegisterGroup(group);
		AssignGroupFrequency(group);
		AssignGroupID(group);
	}
}