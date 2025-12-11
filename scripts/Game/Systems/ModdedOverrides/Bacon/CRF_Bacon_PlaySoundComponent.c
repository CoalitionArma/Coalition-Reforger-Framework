modded class Bacon_PlaySoundComponent
{
	SoundComponent m_SoundComp;
	override void OnPostInit(IEntity owner) {
		owner.SetFlags(EntityFlags.ACTIVE, false);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
	};
	
	override void EOnInit(IEntity owner) {
		if (SCR_Global.IsEditMode(owner))
			return;
		
		m_SoundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
        if (m_SoundComp) {
            // soundComponent.SetEventMask(owner, EntityEvent.FRAME);
            m_SoundComp.SoundEvent( m_soundEventName );
		};
	};
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_SoundComp)
		{
			if (!m_SoundComp.IsPlaying())
				m_SoundComp.SoundEvent( m_soundEventName );
		}
	}
}