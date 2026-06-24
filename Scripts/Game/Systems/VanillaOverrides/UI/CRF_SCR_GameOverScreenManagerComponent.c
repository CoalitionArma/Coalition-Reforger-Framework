// Suppress the vanilla game-over fade overlay when CRF is already handling the
// end-of-mission experience through its own AAR/outro system.
// Without this, SetGameState(POSTGAME) triggers StartEndGameFade() which creates
// a black fade-in overlay at ALWAYS_TOP and eventually opens the vanilla end screen.
modded class SCR_GameOverScreenManagerComponent
{
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		CRF_Gamemode crfGamemode = CRF_Gamemode.GetInstance();
		if (crfGamemode && crfGamemode.m_GamemodeState == CRF_EGamemodeState.AAR)
			return;

		super.OnGameModeEnd(data);
	}
}
