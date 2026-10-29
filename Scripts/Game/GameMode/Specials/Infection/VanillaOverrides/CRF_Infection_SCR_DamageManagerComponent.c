//! Infection special: hits from zombies may infect survivors (CRF_InfectionGamemodeComponent.OnCharacterDamaged)
modded class SCR_DamageManagerComponent
{
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		if (!Replication.IsServer())
			return;

		CRF_InfectionGamemodeComponent infection = CRF_InfectionGamemodeComponent.GetInstance();
		if (infection)
			infection.OnCharacterDamaged(GetOwner(), damageContext);
	}
}
