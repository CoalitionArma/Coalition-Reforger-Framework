modded class SCR_CharacterDamageManagerComponent
{
	//Logs death of a playable character on server
	override void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		super.OnLifeStateChanged(previousLifeState, newLifeState);
		
		CRF_PlayableCharacter playableComp = CRF_PlayableCharacter.Cast(GetOwner().FindComponent(CRF_PlayableCharacter));
		
		if(playableComp && newLifeState == 10 && playableComp.IsPlayable())
			CRF_Gamemode.GetInstance().SetDeathState(GetOwner(), true);
	}
}