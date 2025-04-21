modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		if(!CRF_Gamemode.GetInstance())
			return;
		
		if (CRF_PlayerControllerComponent.GetInstance().m_bActivated)
			SCR_ScreenEffectsManager.GetScreenEffectsDisplay().RHS_SetHDR("{765A5E642D09A4B8}Common/Postprocess/HDR_Vanila.emat", false);

		CRF_PlayerControllerComponent.GetInstance().m_bActivated = false;

		super.OnControlledEntityChanged(from, to);
	}

	//------------------------------------------------------------------------------------------------
	override void DisconnectFromGame()
	{
		if(!CRF_Gamemode.GetInstance())
			return;
		
		super.DisconnectFromGame();

		CRF_PlayerControllerComponent.GetInstance().ResetSettingsToStoredValues();
	}
}