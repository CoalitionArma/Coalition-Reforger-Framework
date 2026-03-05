modded class SCR_MapMarkerSquadLeaderComponent : SCR_MapMarkerDynamicWComponent
{
	//------------------------------------------------------------------------------------------------
	//Removes hovering over the group icon showing you whos alive
	override bool OnMouseEnter(Widget w, int x, int y)
	{	
		return false;
	}
	
	//Removes hovering over the group icon showing you whos alive
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{	
		return false;
	}
}

