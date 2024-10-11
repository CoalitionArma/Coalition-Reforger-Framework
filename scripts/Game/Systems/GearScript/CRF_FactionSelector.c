modded class PS_FactionSelector : SCR_ButtonBaseComponent
{
	override void SetFaction(SCR_Faction faction)
	{	
		m_faction = faction;
		UIInfo uiInfo = faction.GetUIInfo();
		
		string name = m_faction.GetFactionName();
		string icon = uiInfo.GetIconPath();
		
		CRF_GearScriptGamemodeComponent gsComp = CRF_GearScriptGamemodeComponent.GetInstance();
		
		if(gsComp)
		{	
			ResourceName gearScriptResource = gsComp.GetGearScriptResource(m_faction.GetFactionKey());
			
			if(!gearScriptResource.IsEmpty())
			{
				CRF_GearScriptConfig gearConfig = CRF_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
			
				if(gearConfig)
				{
					if(!gearConfig.m_FactionName.IsEmpty())
					{
						name = gearConfig.m_FactionName;
						uiInfo.SetName(name);
					};
				
					if(!gearConfig.m_FactionIcon.IsEmpty())
						icon = gearConfig.m_FactionIcon;
				}
			};
		};
		
		m_wFactionName.SetText(name);
		m_wFactionColor.SetColor(m_faction.GetOutlineFactionColor());
		m_wFactionFlag.LoadImageTexture(0, icon);
	}
};