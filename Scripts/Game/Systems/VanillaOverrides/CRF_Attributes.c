class CRF_BaseContainerCustomTitleResourceFields : BaseContainerCustomTitle
{
	protected ref array<string> m_aPropertyNames;
	protected string m_sFormat;

	//------------------------------------------------------------------------------------------------
	void CRF_BaseContainerCustomTitleResourceFields(array<string> propertyNames, string format = "%1")
	{
		m_aPropertyNames = propertyNames;
		m_sFormat = format;
	}

	//------------------------------------------------------------------------------------------------
	override bool _WB_GetCustomTitle(BaseContainer source, out string title)
	{
		if (!m_aPropertyNames)
		{
			title = m_sFormat;
			return false;
		}

		int count = m_aPropertyNames.Count();
		if (count > 5)
			count = 5;

		array<string> arguments = {};
		arguments.Resize(count); // needed here

		for (int i = 0; i < count; i++)
		{
			ResourceName tempResourceName;
			if (source.Get(m_aPropertyNames[i], tempResourceName))
			{
				string temptitle = FilePath.StripPath(tempResourceName);
				title = FilePath.StripExtension(temptitle);
				title.ToUpper()
			};

			arguments[i] = title;
		}

		switch (count)
		{
			case 0: title = WidgetManager.Translate(m_sFormat); break;
			case 1: title = WidgetManager.Translate(m_sFormat, arguments[0]); break;
			case 2: title = WidgetManager.Translate(m_sFormat, arguments[0], arguments[1]); break;
			case 3: title = WidgetManager.Translate(m_sFormat, arguments[0], arguments[1], arguments[2]); break;
			case 4: title = WidgetManager.Translate(m_sFormat, arguments[0], arguments[1], arguments[2], arguments[3]); break;
			default: title = WidgetManager.Translate(m_sFormat, arguments[0], arguments[1], arguments[2], arguments[3], arguments[4]); break;
		}

		return true;
	}
};