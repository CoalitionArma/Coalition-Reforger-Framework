// CRF_CVON_GamemodeOverride.c
// Disables the Coalition VON (CVON) system when the mission designer has unchecked "Use CVON"
// on the CRF_Lobby entity. When CVON is disabled, the vanilla Arma Reforger in-game VON is
// restored automatically because CVON_SCR_VONController.ActivateVON() and Init() both guard
// against a null CVON_VONGameModeComponent instance.

modded class CVON_VONGameModeComponent
{
	//------------------------------------------------------------------------------------------------
	//! After base init runs (and sets m_Instance = this in the constructor), check whether the
	//! CRF_Gamemode that owns this component has elected to use CVON. If not, clear the static
	//! instance pointer so every downstream CVON system treats CVON as absent and falls through
	//! to vanilla VON behaviour.
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		CRF_Gamemode gamemode = CRF_Gamemode.Cast(owner);
		if (!gamemode)
			return;

		if (!gamemode.m_bUseCVON)
			m_Instance = null;
	}
}
