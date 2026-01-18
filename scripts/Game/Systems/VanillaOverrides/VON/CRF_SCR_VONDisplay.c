modded class SCR_VonDisplay
{
//[OBSOLETE]
//	/**
//	 * Event triggered when player activates voice transmission
//	 * @param transmitter The radio device transmitting the voice (can be null for direct speech)
//	 */
//	override event void OnCapture(BaseTransceiver transmitter)
//	{
//		// Skip processing if camera mode is active
//		if (CRF_PlayerControllerManager.GetInstance().m_eCamera)
//			return;
//
//		super.OnCapture(transmitter);
//	}
//	
//	/**
//	 * Event triggered when receiving voice transmission from another player
//	 * @param playerId ID of the player sending the transmission
//	 * @param receiver The radio device receiving the transmission
//	 * @param frequency The frequency on which the transmission is received
//	 * @param quality The signal quality (0.0 to 1.0)
//	 */
//	override event void OnReceive(int playerId, BaseTransceiver receiver, int frequency, float quality)
//	{
//		// Skip processing if camera mode is active or this is direct speach
//		if (CRF_PlayerControllerManager.GetInstance().m_eCamera || !receiver)
//			return;
//
//		super.OnReceive(playerId, receiver, frequency, quality);
//	}
}