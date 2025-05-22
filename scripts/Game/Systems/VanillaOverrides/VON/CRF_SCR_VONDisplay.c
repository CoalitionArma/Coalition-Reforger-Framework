modded class SCR_VonDisplay
{
	/**
	 * Event triggered when player activates voice transmission
	 * @param transmitter The radio device transmitting the voice (can be null for direct speech)
	 */
	override event void OnCapture(BaseTransceiver transmitter)
	{
		// Exit if UI root is not available
		if (!m_wRoot)
			return;
		
		// Skip processing if camera mode is active
		if (CRF_PlayerControllerManager.GetInstance().m_eCamera)
			return;

		// Initialize frequency variable
		int frequency = 0;

		// Get frequency from transmitter if available (transmitter is null for direct speech)
		if (transmitter)
			frequency = transmitter.GetFrequency();

		// Determine if transmission data needs to be updated
		// Updates when: forced, first activation, device changed, or frequency changed
		bool needsUpdate = m_OutTransmission.m_bForceUpdate == true
			|| m_OutTransmission.m_bIsActive == false
			|| m_OutTransmission.m_RadioTransceiver != transmitter
			|| (transmitter && m_OutTransmission.m_fFrequency != transmitter.GetFrequency());
		
		// Update transmission data if needed
		if (needsUpdate)
		{
			UpdateTransmission(m_OutTransmission, transmitter, frequency, false);
		}

		// Mark transmission as active and reset timeout
		m_OutTransmission.m_bIsActive = true;
		m_OutTransmission.m_fActiveTimeout = 0;
	}
	
	/**
	 * Event triggered when receiving voice transmission from another player
	 * @param playerId ID of the player sending the transmission
	 * @param receiver The radio device receiving the transmission
	 * @param frequency The frequency on which the transmission is received
	 * @param quality The signal quality (0.0 to 1.0)
	 */
	override event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
	{
		// Exit if UI root is not available or VON UI is disabled
		if (!m_wRoot || m_bIsVONUIDisabled)
			return;

		// Skip processing if camera mode is active
		if (CRF_PlayerControllerManager.GetInstance().m_eCamera)
			return;

		// Try to find existing transmission data for this player
		TransmissionData pTransmission = m_aTransmissionMap.Get(playerId);

		// If no existing transmission from this player, create a new one
		if (!pTransmission)
		{
			// Skip if this is direct speech and direct speech UI is disabled
			if (!receiver && m_bIsVONDirectDisabled)
				return;
			
			// If no widget is available for this transmission
			if (!GetWidget())
			{
				// Add to additional speakers count instead of creating dedicated widget
				pTransmission = new TransmissionData(m_wAdditionalSpeakersWidget, playerId);
				m_aAdditionalSpeakers.Insert(pTransmission);
				
				// Update the +X text to show number of additional speakers
				m_wAdditionalSpeakersText.SetText("+" + m_aAdditionalSpeakers.Count().ToString());
				
				// Show the additional speakers widget
				m_wAdditionalSpeakersWidget.SetVisible(true);
				m_wAdditionalSpeakersWidget.SetOpacity(1);
				
				pTransmission.m_bIsAdditional = true;
				pTransmission.m_bVisible = true;
			}
			else
			{
				// Create new transmission with available widget
				pTransmission = new TransmissionData(GetWidget(), playerId);
				pTransmission.m_bIsAdditional = false;
			}

			// Set transmission quality
			pTransmission.m_fQuality = quality;

			// Add to collections for tracking
			m_aTransmissions.Insert(pTransmission);
			m_aTransmissionMap.Insert(playerId, pTransmission);
		}

		// Determine if transmission data needs to be updated
		// Updates when: first activation, device changed, or frequency changed
		bool needsUpdate = pTransmission.m_bIsActive == false
			|| pTransmission.m_RadioTransceiver != receiver
			|| (receiver && pTransmission.m_fFrequency != frequency);
		
		// Update transmission if needed
		if (needsUpdate)
		{
			bool isTransmissionVisible = UpdateTransmission(pTransmission, receiver, frequency, true);
			if (!isTransmissionVisible)
			{
				pTransmission.HideTransmission();
				return;
			}
		}

		// Mark transmission as active and reset timeout
		pTransmission.m_bIsActive = true;
		pTransmission.m_fActiveTimeout = 0;
	}
}