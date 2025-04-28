modded class SCR_VONController
{
	//------------------------------------------------------------------------------------------------
	//! Public wrapper for the protected ResetVON method to allow external systems to reset VON
	//------------------------------------------------------------------------------------------------
	void PublicResetVON()
	{
		ResetVON();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Set transmission method depending on entry type when starting VON transmit
	//! @param entry The VON entry to set as active transmitter
	//------------------------------------------------------------------------------------------------
	override protected void SetActiveTransmit(notnull SCR_VONEntry entry)
	{        
		// Early return if VON component is not available
		if (!m_VONComp)
			return;
		
		// Handle squad radio transmission
		if (entry.GetVONMethod() == ECommMethod.SQUAD_RADIO)
		{
			// Configure VON component for squad radio communication
			m_VONComp.SetCommMethod(ECommMethod.SQUAD_RADIO);
			
			// Set the radio transceiver from the entry after casting to radio type
			SCR_VONEntryRadio radioEntry = SCR_VONEntryRadio.Cast(entry);
			m_VONComp.SetTransmitRadio(radioEntry.GetTransceiver());
			
			// Set this entry as the active one
			SetEntryActive(entry);
		} else {
			// Configure VON component for direct (non-radio) communication
			m_VONComp.SetCommMethod(ECommMethod.DIRECT);
			m_VONComp.SetTransmitRadio(null);
		}
		
		// Clear saved entry if it's the current one being processed
		if (entry == m_SavedEntry)
		{
			m_SavedEntry = null;
		}
	}
}