class PS_ManuaMarkerRenderClass : PS_ManualMarkerClass
{
	
}

class PS_ManuaMarkerRender : PS_ManualMarker
{
	PS_ManualMarkerRenderComponent m_hManualMarkerRenderComponent;
	
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_sMarkerPrefab = "{4EE4EBD5EC3C0999}UI/Map/ManualMapMarkerRender.layout";
	}
	
	override void CreateMapWidget(MapConfiguration mapConfig)
	{
		super.CreateMapWidget(mapConfig);
		if (!m_hManualMarkerComponent)
			return;
		m_hManualMarkerRenderComponent = PS_ManualMarkerRenderComponent.Cast(m_hManualMarkerComponent);
		m_hManualMarkerRenderComponent.SetCameraEntity(GetChildren(), 9);
	}
}