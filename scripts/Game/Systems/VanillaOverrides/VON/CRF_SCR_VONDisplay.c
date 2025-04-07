modded class SCR_VonDisplay
{
	override event void OnCapture(BaseTransceiver transmitter)
	{
		if (!m_wRoot)
			return;
		
		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_eCamera)
			return;

		int frequency;

		if (transmitter)	// can be null when using direct speech
			frequency = transmitter.GetFrequency();

		// update only when first activation / device changed / frequency changed
		if (m_OutTransmission.m_bForceUpdate == true
			|| m_OutTransmission.m_bIsActive == false
			|| m_OutTransmission.m_RadioTransceiver != transmitter
			|| (transmitter && m_OutTransmission.m_fFrequency != transmitter.GetFrequency())
		)
		{
			UpdateTransmission(m_OutTransmission, transmitter, frequency, false);
		}

		m_OutTransmission.m_bIsActive = true;
		m_OutTransmission.m_fActiveTimeout = 0;
	}
	
	override event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		if (!m_wRoot || m_bIsVONUIDisabled) // ignore receiving transmissions if VON UI is off
			return;

		// Check if there is an active transmission from given player
		TransmissionData pTransmission = m_aTransmissionMap.Get(playerId);

		if (SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_eCamera)
			return;

		// Active transmission from the player not found, create it
		if (!pTransmission)
		{
			if (!receiver && m_bIsVONDirectDisabled)	// direct UI off
				return;
			

			// No free widget
			if (!GetWidget())
			{
				//Insert new widget to display other transmissions as number
				pTransmission = new TransmissionData(m_wAdditionalSpeakersWidget, playerId);
				m_aAdditionalSpeakers.Insert(pTransmission);
				m_wAdditionalSpeakersText.SetText("+" + m_aAdditionalSpeakers.Count().ToString());
				m_wAdditionalSpeakersWidget.SetVisible(true);
				m_wAdditionalSpeakersWidget.SetOpacity(1);
				pTransmission.m_bIsAdditional = true;
				pTransmission.m_bVisible = true;
			}
			else
			{
				pTransmission = new TransmissionData(GetWidget(), playerId);
				pTransmission.m_bIsAdditional = false;
			}

			pTransmission.m_fQuality = quality;

			m_aTransmissions.Insert(pTransmission);
			m_aTransmissionMap.Insert(playerId, pTransmission);
		}

		// update only when first activation / device changed / frequency changed
		if (pTransmission.m_bIsActive == false
			|| pTransmission.m_RadioTransceiver != receiver
			|| (receiver && pTransmission.m_fFrequency != frequency)
		)
		{
			bool filtered = UpdateTransmission(pTransmission, receiver, frequency, true);
			if (!filtered)
			{
				pTransmission.HideTransmission();
				return;
			}
		}

		pTransmission.m_bIsActive = true;
		pTransmission.m_fActiveTimeout = 0;
	}
}