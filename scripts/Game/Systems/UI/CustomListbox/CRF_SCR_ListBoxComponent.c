modded class SCR_ListBoxComponent
{
	int AddItemAndIconPlayer(string item, ResourceName imageOrImageset, string iconName, Managed data = null, ResourceName itemLayout = string.Empty, int playerId = 0)
	{
		SCR_ListBoxElementComponent comp;
		
		int id = _AddItem(item, data, comp, itemLayout);
		
		comp.SetImage(imageOrImageset, iconName);
		comp.m_iplayerId = playerId;
		
		return id;
	}
}