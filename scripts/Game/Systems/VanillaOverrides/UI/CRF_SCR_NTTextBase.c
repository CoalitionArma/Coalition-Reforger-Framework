//! Base nametag element for text
[BaseContainerProps(), SCR_NameTagElementTitle()]
modded class SCR_NTTextBase : SCR_NTElementBase
{
	//------------------------------------------------------------------------------------------------
	/**
	 * Overrides the base method to customize text widget behavior.
	 * This method is called to set default properties for the nametag element.
	 * 
	 * @param data The nametag data containing element information
	 * @param index The index of the element in the nametag elements array
	 */
	override void SetDefaults(SCR_NameTagData data, int index)
	{
		// Call the parent implementation first
		super.SetDefaults(data, index);

		// Try to cast the element to a TextWidget
		TextWidget tWidget = TextWidget.Cast(data.m_aNametagElements[index]);
		
		// If casting failed, we can't proceed
		if (!tWidget)
			return;

		// Check if the entity exists and is a spectator
		if (data.m_Entity)
		{
			// Hide the text widget for spectator entities
			if (CRF_GamemodeManager.IsSpectator(data.m_Entity))
			{
				tWidget.SetVisible(false);
			}
		}
	}
}
