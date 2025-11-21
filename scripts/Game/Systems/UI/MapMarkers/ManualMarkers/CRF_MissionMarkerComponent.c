class CRF_MissionMarkerComponent: CRF_ManualMarkerComponent
{
	PanelWidget m_wMarkerBriefing;
	bool m_bIsDisabled = false;
	CRF_MissionMarker m_MissionMarker;
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		m_wMarkerBriefing = PanelWidget.Cast(w.FindAnyWidget("MarkerBriefing"));
		GetGame().GetCallqueue().CallLater(Init, 1000, false, w);
	}
	
	void Init(Widget w)
	{
		m_MissionMarker = CRF_MissionMarker.Cast(m_ManualMarker);
		if (m_MissionMarker.m_sMissionMarkerDescription.Length() == 0)
		{
			if (!m_MissionMarker.m_aMissionMarkerImages)
			{
				m_bIsDisabled = true;
				m_wMarkerBriefing.SetVisible(false);
				return;
			}
			
			if (m_MissionMarker.m_aMissionMarkerImages.Count() == 0)
			{
				m_bIsDisabled = true;
				m_wMarkerBriefing.SetVisible(false);
				return;
			}
		}
		SetMissionMarkerDescription(m_MissionMarker.m_sMissionMarkerDescription, w);
		if (m_MissionMarker.m_aMissionMarkerImages)
			if (m_MissionMarker.m_aMissionMarkerImages.Count() > 0)
				SetMissionMarkerImages(m_MissionMarker.m_aMissionMarkerImages, w);
	}
	
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!m_bIsDisabled)
			m_wMarkerBriefing.SetVisible(true);
		return super.OnMouseEnter(w, x, y);
	}
	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_bIsDisabled)
			m_wMarkerBriefing.SetVisible(false);
		return super.OnMouseLeave(w, enterW, x, y);
	}
	
	override void SetSlot(float posX, float posY, float sizeX, float sizeY, float rotation)
	{
		super.SetSlot(posX, posY, sizeX, sizeY, rotation);
		WorkspaceWidget ws = GetGame().GetWorkspace();

		float panelX, panelY;
		m_wDescriptionPanel.GetScreenSize(panelX, panelY);
	
		float panelXD = ws.DPIUnscale(panelX);
		float panelYD = ws.DPIUnscale(panelY);
	
		float pX = panelXD / 2;
		float pY = -panelYD / 2;
	
		float parentX, parentY;
		m_wMarkerBriefing.GetParent().GetScreenPos(parentX, parentY);
	
		float screenX = parentX + ws.DPIScale(pX);
		float screenY = parentY + ws.DPIScale(pY);
	
		float sX, sY;
		m_wMarkerBriefing.GetScreenSize(sX, sY);
	
		float topY = screenY;
		float bottomY = screenY + sY;
	
		float screenH, screenW;
		ws.GetScreenSize(screenW, screenH);
		float correction = 0;
	
		if (topY < 0)
		{
			correction = -topY;
		}
		else if (bottomY > screenH)
		{
			correction = screenH - bottomY;
		}
	
		float correctedY = pY + ws.DPIUnscale(correction);
		FrameSlot.SetPos(m_wMarkerBriefing, pX, correctedY);
	}
	
	void SetMissionMarkerImages(array<ResourceName> images, Widget root)
	{
		Widget imagesWidget = root.FindAnyWidget("Images");
		foreach (ResourceName image: images)
		{
			Widget newWidget = GetGame().GetWorkspace().CreateWidgets("{35083585E37FE529}UI/Map/MissionMarkerImage.layout", imagesWidget);
			ImageWidget imageWidget = ImageWidget.Cast(newWidget.FindAnyWidget("BriefImage"));
			imageWidget.LoadImageTexture(0, image);
			imageWidget.SetImage(0);
		}
	}
	
	void SetMissionMarkerDescription(string input, Widget root)
	{
		string result;
		int start = 0;
		int len = input.Length();
		int maxLength = 45;
		
		while (start < len)
		{
			int remaining = len - start;
	
			if (remaining <= maxLength)
			{
				result += input.Substring(start, remaining);
				break;
			}
	
			string block = input.Substring(start, maxLength);
			int lastSpace = block.LastIndexOf(" ");
			
			if (lastSpace == -1)
				lastSpace = maxLength;
	
			result += input.Substring(start, lastSpace);
			result += "\n";
	
			start += lastSpace;
	
			if (start < len && input[start] == " ")
				start++;
		}
		RichTextWidget.Cast(root.FindAnyWidget("MissionDescriptionText")).SetText(result);
	}
}