modded class SCR_ListBoxElementComponent
{
	int m_iplayerId;
	int m_iDescriptionIndex;
	void SetColor(Color color)
	{
		TextWidget w = TextWidget.Cast(m_wRoot.FindAnyWidget(m_sWidgetTextName));
		if (w)
			w.SetColor(color);
	}
	
	void SetDescriptionIndex(int input)
	{
		m_iDescriptionIndex = input;
	}
	
	SCR_ButtonTextComponent GetSelectButton()
	{
		SCR_ButtonTextComponent w = SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("PlayerButton")).FindHandler(SCR_ButtonTextComponent));
		if(w)
			return w;
		return null;
	}
}