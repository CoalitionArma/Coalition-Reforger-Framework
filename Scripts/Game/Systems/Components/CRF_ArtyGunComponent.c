class CRF_ArtyGunComponentClass: ScriptComponentClass
{
}

class CRF_ArtyGunComponent: ScriptComponent
{
	[RplProp()] bool m_bIsDestroyed = false;
	
	void SetDestroyed()
	{
		m_bIsDestroyed = true;
		Replication.BumpMe();
		CRF_GamemodeManager.GetInstance().DestroyGun();
	}
}