class CRF_ListBoxElementComponent: SCR_ListBoxElementComponent
{
	SCR_AIGroup group;
	RplId entityID;
	bool isGroupLocked;
	bool m_bIsPlayer = false;
	int m_iChannelId;
	CRF_Gamemode m_Gamemode = CRF_Gamemode.GetInstance();	
	bool m_bDeleteRequest = false;
	SCR_ButtonTextComponent GetAccept()
	{
		return SCR_ButtonTextComponent.Cast(m_wRoot.FindAnyWidget("Accept").FindHandler(SCR_ButtonTextComponent));
	}
	
	SCR_ButtonTextComponent GetDeny()
	{
		return SCR_ButtonTextComponent.Cast(m_wRoot.FindAnyWidget("Deny").FindHandler(SCR_ButtonTextComponent));
	}
	
	ProgressBarWidget GetProgress()
	{
		return ProgressBarWidget.Cast(m_wRoot.FindAnyWidget("ProgressBar"));
	}
	
	FrameWidget GetDisconnectWidget()
	{
		FrameWidget w = FrameWidget.Cast(m_wRoot.FindAnyWidget("Disconnected"));
		if(w)
			return w;
		return null;
	}
	
	FrameWidget GetDeathWidget()
	{
		FrameWidget w = FrameWidget.Cast(m_wRoot.FindAnyWidget("Killed"));
		if(w)
			return w;
		return null;
	}
	
	void SetRoleText(string text)
	{
		TextWidget w = TextWidget.Cast(m_wRoot.FindAnyWidget("RoleName"));
		if (w)
			w.SetText(text);
	}
	
	void SetPlayerText(string text)
	{
		TextWidget w = TextWidget.Cast(m_wRoot.FindAnyWidget("PlayerName"));
		if (w)
			w.SetText(text);
	}
	
	SCR_ButtonTextComponent GetChannelButton()
	{
		return SCR_ButtonTextComponent.Cast(m_wRoot.FindAnyWidget("SlotButton").FindHandler(SCR_ButtonTextComponent));
	}
	
	void SetRoleImage(ResourceName imageOrImageset, string iconName)
	{
		ImageWidget w = ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleImage"));
		
		if (imageOrImageset.IsEmpty())
			return;
		
		if (w)
		{
			if (imageOrImageset.EndsWith("imageset"))
			{
				if (!iconName.IsEmpty())
					w.LoadImageFromSet(0, imageOrImageset, iconName);
			}
			else
			{
				w.LoadImageTexture(0, imageOrImageset);
			}
		}
	}
	
	void SetLockImage(ResourceName imageOrImageset, string iconName)
	{
		ImageWidget w = ImageWidget.Cast(m_wRoot.FindAnyWidget("LockImage"));
		
		if (imageOrImageset.IsEmpty())
			return;
		
		if (w)
		{
			if (imageOrImageset.EndsWith("imageset"))
			{
				if (!iconName.IsEmpty())
					w.LoadImageFromSet(0, imageOrImageset, iconName);
			}
			else
			{
				w.LoadImageTexture(0, imageOrImageset);
			}
		}
	}
	
	void SetRoleColor(Color color)
	{
		ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleImage")).SetColor(color);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("Divider1")).SetColor(color);
		ImageWidget.Cast(m_wRoot.FindAnyWidget("Divider2")).SetColor(color);
	}
	
	void SetGroupName(string text)
	{
		TextWidget w = TextWidget.Cast(m_wRoot.FindAnyWidget("RolesGroupName"));
		if (w)
			w.SetText(text);
	}
	
	SCR_MilitarySymbolUIComponent GetGroupIcon()
	{
		SCR_MilitarySymbolUIComponent w = SCR_MilitarySymbolUIComponent.Cast(m_wRoot.FindAnyWidget("SymbolOverlay").FindHandler(SCR_MilitarySymbolUIComponent));
		if(w)
			return w;
		return null;
	}
	
	void SetGroupIconColor(Color color)
	{
		ImageWidget.Cast(m_wRoot.FindAnyWidget("RoleImage")).SetColor(color);
	}
	
	Widget GetGroupWidget()
	{
		Widget w = m_wRoot.FindAnyWidget("SymbolOverlay");
		if(w)
			return w;
		return null;
	}
	
	Widget GetGroupUnderline()
	{
		Widget w = m_wRoot.FindAnyWidget("Underline");
			if(w)
				return w;
			return null;
	}
	
	SCR_ButtonTextComponent GetSlotButton()
	{
		SCR_ButtonTextComponent w = SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("SlotButton")).FindHandler(SCR_ButtonTextComponent));
		if(w)
			return w;
		return null;
	}
	
	SCR_ButtonTextComponent GetLockButton()
	{
		SCR_ButtonTextComponent w = SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("LockButton")).FindHandler(SCR_ButtonTextComponent));
		if(w)
			return w;
		return null;
	}
	
	SCR_ButtonTextComponent GetKickButton()
	{
		SCR_ButtonTextComponent w = SCR_ButtonTextComponent.Cast(ButtonWidget.Cast(m_wRoot.FindAnyWidget("KickButton")).FindHandler(SCR_ButtonTextComponent));
		if(w)
			return w;
		return null;
	}
	
	void DisableKickButton()
	{
		ButtonWidget button = ButtonWidget.Cast(m_wRoot.FindAnyWidget("KickButton"));
		if(button)
			button.SetEnabled(false);
		ImageWidget image = ImageWidget.Cast(m_wRoot.FindAnyWidget("KickImage"));
		if(image)
			image.SetColor(Color.FromRGBA(0,0,0,0));
	}
}