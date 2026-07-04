modded class SCR_DamageManagerComponent
{
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		super.OnDamage(damageContext);

		CRF_GunGame gunGame = CRF_GunGame.Cast(GetGame().GetGameMode().FindComponent(CRF_GunGame));
		if (!gunGame)
			return;

		#ifdef WORKBENCH
		#else
		if(RplSession.Mode() != RplMode.Client)
			return;
		#endif
		if (damageContext.instigator.GetInstigatorPlayerID() != SCR_PlayerController.GetLocalPlayerId())
			return;

		if (damageContext.damageType == EDamageType.FRAGMENTATION || damageContext.damageType == EDamageType.PROCESSED_FRAGMENTATION || damageContext.damageType == EDamageType.EXPLOSIVE)
			return;

		AddHitmarker();

		if (damageContext.struckHitZone.GetName() == "Head")
			gunGame.AddMedal("{86192E78B8F02AE1}UI/layouts/HUD/GunGame/Medals/Headshot_Medal_BOII.edds", "HEADSHOT");
	}

	void AddHitmarker()
	{
		CRF_GunGame gunGame = CRF_GunGame.Cast(GetGame().GetGameMode().FindComponent(CRF_GunGame));
		if (!gunGame)
			return;

		if (gunGame.m_wHitmarker)
		{
			delete gunGame.m_wHitmarker;
			gunGame.m_wHitmarker = GetGame().GetWorkspace().CreateWidgets("{B5AEA68D2F3A3C3A}UI/layouts/HUD/GunGame/HitMarker.layout");
			AudioSystem.PlaySound("{477CD4CAF95266BB}Sounds/GunGame/hitmarker.wav");
		}
		else
		{
			gunGame.m_wHitmarker = GetGame().GetWorkspace().CreateWidgets("{B5AEA68D2F3A3C3A}UI/layouts/HUD/GunGame/HitMarker.layout");
			AudioSystem.PlaySound("{477CD4CAF95266BB}Sounds/GunGame/hitmarker.wav");
		}
	}
}