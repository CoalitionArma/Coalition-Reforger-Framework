[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class CRF_EnableLeadersArsenal : SCR_BaseEditorAttribute
{	
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);
		if (!gamemode)
			return null;
		
		return SCR_BaseEditorAttributeVar.CreateBool(gamemode.IsLeadersArsenalEnabled());
	}
	
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) 
			return;
		
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);
		if (!gamemode)
			return;
		
		gamemode.SetLeadersArsenalEnabled(var.GetBool());
	}
}

modded class SCR_BaseGameMode 
{
	[RplProp()]
	protected bool m_bIsLeadersArsenalEnabled = true;
	
	void SetLeadersArsenalEnabled(bool input) 
	{
		m_bIsLeadersArsenalEnabled = input;
		Replication.BumpMe();
	};
	
	bool IsLeadersArsenalEnabled() 
	{
		return m_bIsLeadersArsenalEnabled;
	};
}