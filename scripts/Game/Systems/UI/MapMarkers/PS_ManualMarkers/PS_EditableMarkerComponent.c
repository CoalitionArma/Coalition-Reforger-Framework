[ComponentEditorProps(category: "GameScripted/Editor (Editables)", description: "", icon: "WBData/ComponentEditorProps/componentEditor.png")]
class PS_EditableMarkerComponentClass: SCR_EditableDescriptorComponentClass
{
	
};

class PS_EditableMarkerComponent: SCR_EditableSystemComponent
{
	PS_ManualMarker m_eManualMarker;
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_eManualMarker = PS_ManualMarker.Cast(owner);
	}
	
	string GetMarkerDescription()
	{
		return m_eManualMarker.GetDescription();
	}
	void SetMarkerDescription(string description)
	{
		m_eManualMarker.SetDescription(description);
	}
	
	string GetMarkerColor()
	{
		Color color = m_eManualMarker.GetColor();
		return color.A().ToString() + " " + color.R().ToString() + " " + color.G().ToString() + " " + color.B().ToString();
	}
	void SetMarkerColor(string colorStr)
	{
		array<string> outTokens = new array<string>();
		colorStr.Split(" ", outTokens, false);
		if (outTokens.Count() == 4)
		{
			float a = outTokens[0].ToFloat();
			float r = outTokens[1].ToFloat();
			float g = outTokens[2].ToFloat();
			float b = outTokens[3].ToFloat();
			Color color = new Color(r, g, b, a);
			m_eManualMarker.SetColor(color);
		}
	}
	
	string GetMarkerImage()
	{
		return m_eManualMarker.GetImageSet() + "|" + m_eManualMarker.GetImageSetGlow() + "|" + m_eManualMarker.GetQuadName();
	}
	void SetMarkerImage(string str)
	{
		array<string> outTokens = new array<string>();
		str.Split("|", outTokens, false);
		m_eManualMarker.SetImageSet(outTokens[0]);
		m_eManualMarker.SetImageSetGlow(outTokens[1]);
		m_eManualMarker.SetQuadName(outTokens[2]);
	}
	
	float GetMarkerSize()
	{
		return m_eManualMarker.GetSize();
	}
	void SetMarkerSize(float size)
	{
		m_eManualMarker.SetSize(size);
	}
	
	bool GetMarkerUseWorldScale()
	{
		return m_eManualMarker.GetUseWorldScale();
	}
	void SetMarkerUseWorldScale(bool worldScale)
	{
		m_eManualMarker.SetUseWorldScale(worldScale);
	}
	
	bool GetMarkerVisibleForFaction(Faction faction)
	{
		return m_eManualMarker.GetVisibleForFaction(faction.GetFactionKey());
	}
	void SetMarkerVisibleForFaction(Faction faction, bool visible)
	{
		m_eManualMarker.SetVisibleForFaction(faction, visible);
	}
	
	bool GetVisibleForEmptyFaction()
	{
		return m_eManualMarker.GetVisibleForEmptyFaction();
	}
	void SetVisibleForEmptyFaction(bool visibleForEmptyFaction)
	{
		m_eManualMarker.SetVisibleForEmptyFaction(visibleForEmptyFaction);
	}
};




