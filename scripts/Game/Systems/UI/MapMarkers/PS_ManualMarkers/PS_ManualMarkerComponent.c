// Custom marker widget component
class PS_ManualMarkerComponent : SCR_ScriptedWidgetComponent
{
	protected SCR_MapEntity m_MapEntity;
	
	// Cache widgets
	protected ImageWidget m_wMarkerIcon;
	protected ImageWidget m_wMarkerIconGlow;
	protected FrameWidget m_wMarkerFrame;
	protected RichTextWidget m_wDescriptionText;
	protected PanelWidget m_wDescriptionPanel;
	protected OverlayWidget m_wMarkerScrollLayout; // ...
	
	// Internal variables
	protected string m_sDescription;
	protected bool m_bHasGlow;
	protected int m_iZOrder;
	
	// Cache every used widget after attaching to widget tree
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_MapEntity = SCR_MapEntity.GetMapInstance();
		m_wMarkerIcon = ImageWidget.Cast(w.FindAnyWidget("MarkerIcon"));
		m_wMarkerIconGlow = ImageWidget.Cast(w.FindAnyWidget("MarkerGlowIcon"));
		m_wMarkerFrame = FrameWidget.Cast(w.FindAnyWidget("MarkerFrame"));
		m_wDescriptionText = RichTextWidget.Cast(w.FindAnyWidget("DescriptionText"));
		m_wMarkerScrollLayout = OverlayWidget.Cast(w.FindAnyWidget("MarkerScrollLayout"));
		m_wDescriptionPanel = PanelWidget.Cast(w.FindAnyWidget("DescriptionPanel"));
	}
	
	float GetYScale()
	{
		int x, y;
		m_wMarkerIcon.GetImageSize(0, x, y);
		if (y == 0) y = 1;
		if (x == 0) x = 1;
		float scale = (float) y / (float) x;
		return scale;
	}
	
	// Every info contains in PS_ManualMarker, soo ther is onle setters
	void SetImage(ResourceName m_sImageSet, string quadName)
	{
		if (m_sImageSet.EndsWith(".edds"))
			m_wMarkerIcon.LoadImageTexture(0, m_sImageSet);
		else
			m_wMarkerIcon.LoadImageFromSet(0, m_sImageSet, quadName);
	}
	void SetImageGlow(ResourceName m_sImageSet, string quadName)
	{
		if (m_sImageSet != "") m_bHasGlow = true;
		if (m_sImageSet.EndsWith(".edds"))
			m_wMarkerIconGlow.LoadImageTexture(0, m_sImageSet);
		else
			m_wMarkerIconGlow.LoadImageFromSet(0, m_sImageSet, quadName);
	}
	void SetDescription(string description)
	{
		m_sDescription = description;
		
		m_wDescriptionText.SetText(description);
	}
	void SetColor(Color color)
	{
		m_wMarkerIcon.SetColor(color);	
		m_wMarkerIconGlow.SetColor(color);	
	}
	
	void SetOpacity(float opacity)
	{
		m_wRoot.SetOpacity(opacity);
	}
	
	void SetSlotWorld(vector worldPosition, vector rotation, float worldSize, bool useWorldScale, float minSize = 0.0)
	{
		// Get screen position
		float wX, wY, screenX, screenY, screenXEnd, screenYEnd;
		wX = worldPosition[0];
		wY = worldPosition[2];
		m_MapEntity.WorldToScreen(wX, wY, screenX, screenY, true);
		m_MapEntity.WorldToScreen(wX + worldSize, wY + worldSize, screenXEnd, screenYEnd, true);
		
		// Scale to workspace
		float screenXD = GetGame().GetWorkspace().DPIUnscale(screenX);
		float screenYD = GetGame().GetWorkspace().DPIUnscale(screenY);
		float sizeXD = worldSize;
		float sizeYD = worldSize;
		if (useWorldScale) // Calculate world size if need
		{
			sizeXD = GetGame().GetWorkspace().DPIUnscale(screenXEnd - screenX);
			sizeYD = GetGame().GetWorkspace().DPIUnscale(screenY - screenYEnd); // Y flip
		}
		if (minSize > 0)
		{
			if (sizeXD < minSize) sizeXD = minSize;
			if (sizeYD < minSize) sizeYD = minSize;
		}
		sizeYD *= GetYScale();
		
		SetSlot(screenXD, screenYD, sizeXD, sizeYD, rotation[0] - 90);
	}
	
	// Update marker "Transform", called every frame
	void SetSlot(float posX, float posY, float sizeX, float sizeY, float rotation)
	{
		FrameSlot.SetPos(m_wRoot, posX, posY);
		
		FrameSlot.SetPos(m_wMarkerIcon, -sizeX/2, -sizeY/2);
		FrameSlot.SetSize(m_wMarkerIcon, sizeX, sizeY);
		FrameSlot.SetPos(m_wMarkerIconGlow, -sizeX/2, -sizeY/2);
		FrameSlot.SetSize(m_wMarkerIconGlow, sizeX, sizeY);
		FrameSlot.SetPos(m_wMarkerScrollLayout, -sizeX/2, -sizeY/2);
		FrameSlot.SetSize(m_wMarkerScrollLayout, sizeX, sizeY);
		
		float panelX, panelY;
		m_wDescriptionPanel.GetScreenSize(panelX, panelY);
		float panelXD = GetGame().GetWorkspace().DPIUnscale(panelX);
		float panelYD = GetGame().GetWorkspace().DPIUnscale(panelY);
		if (panelX == 0)
			m_wDescriptionPanel.SetOpacity(0);
		else
			m_wDescriptionPanel.SetOpacity(1);
		FrameSlot.SetPos(m_wDescriptionPanel, -panelXD/2, -panelYD/2);
		m_wMarkerIcon.SetRotation(rotation);
		m_wMarkerIconGlow.SetRotation(rotation);
	}
	
	// Show/Hide description box
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		m_iZOrder = m_wRoot.GetZOrder();
		m_wRoot.SetZOrder(10000);
		if (m_bHasGlow) m_wMarkerIconGlow.SetVisible(true);
		if (m_sDescription != "") m_wDescriptionPanel.SetVisible(true);
		
		return true;
	}
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		m_wRoot.SetZOrder(m_iZOrder);
		m_wMarkerIconGlow.SetVisible(false);	
		m_wDescriptionPanel.SetVisible(false);
		
		return true;
	}
};