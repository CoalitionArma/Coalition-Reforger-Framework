modded class SCR_VONController
{
	void PublicResetVON()
	{
		ResetVON();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set transmission method depending on entry type when starting VON transmit
	override protected void SetActiveTransmit(notnull SCR_VONEntry entry)
	{		
		if (!m_VONComp)
			return;
		
		if (entry.GetVONMethod() == ECommMethod.SQUAD_RADIO)
		{
			m_VONComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
			m_VONComp.SetTransmitRadio(SCR_VONEntryRadio.Cast(entry).GetTransceiver());
			SetEntryActive(entry);
		}
		else 
		{
			m_VONComp.SetCommMethod(ECommMethod.DIRECT);
			m_VONComp.SetTransmitRadio(null);
		}
		
		if (entry == m_SavedEntry)
			m_SavedEntry = null;
	}
}