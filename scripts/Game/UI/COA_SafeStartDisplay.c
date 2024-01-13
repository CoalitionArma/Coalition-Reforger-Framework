class COA_SafeStartDisplay : SCR_InfoDisplay
{
	protected ImageWidget timerImage;
	protected TextWidget timerDescription;
	protected TextWidget timerText;
	protected TextWidget missionStart;
	protected TextWidget missionStart2;
	
	protected ImageWidget hintBackground;
	protected RichTextWidget hint;
	
	protected float currentOpacity = 0;
	
	override protected void UpdateValues(IEntity owner, float timeSlice)
	{
		timerImage       = ImageWidget.Cast(m_wRoot.FindWidget("TimerImage"));
		timerDescription = TextWidget.Cast(m_wRoot.FindWidget("TimerDescription"));
		timerText        = TextWidget.Cast(m_wRoot.FindWidget("TimerText"));
		missionStart     = TextWidget.Cast(m_wRoot.FindWidget("MissionStart"));
		missionStart2    = TextWidget.Cast(m_wRoot.FindWidget("MissionStart2"));
		hintBackground   = ImageWidget.Cast(m_wRoot.FindWidget("HintBackground"));
		hint             = RichTextWidget.Cast(m_wRoot.FindWidget("Hint"));
	}
	
	void Hint(string hintText)
	{
		hintBackground.SetOpacity(0.785);
		hint.SetOpacity(1);
		
		hint.SetText(hintText);
		
		float hintOpacity = 1;
		
		while(hintOpacity > 0)
		{
		 	hintOpacity = hintOpacity - 0.005;
			
			hintBackground.SetOpacity(hintOpacity);
			hint.SetOpacity(hintOpacity);
		}
		
		hint.SetText("");
	}
	
	void UpdateTimer(int time)
	{	
		timerText.SetText(time.ToString());
	}

	void StopMission()
	{
		timerDescription.SetOpacity(1);
		timerText.SetOpacity(1);
		timerImage.SetOpacity(1);
		
		missionStart.SetOpacity(0);
		missionStart2.SetOpacity(0);
		
		currentOpacity = 0;
	}
	
	void StartMission()
	{
		timerDescription.SetOpacity(0);
		timerText.SetOpacity(0);
		timerImage.SetOpacity(0);
			
		missionStart.SetOpacity(1);
		missionStart2.SetOpacity(1);
		
		currentOpacity = 1;
			
		while(currentOpacity > 0)
		{
		 	currentOpacity = currentOpacity - 0.005;
			
			missionStart.SetOpacity(currentOpacity);
			missionStart2.SetOpacity(currentOpacity);
		}
	}
}