class CRF_VAAR_BaseProjectileEffect: BaseProjectileEffect
{
	override void OnEffect(IEntity pHitEntity, inout vector outMat[3], IEntity damageSource, notnull Instigator instigator, string colliderName, float speed)
	{
		
		IEntity shooter = instigator.GetInstigatorEntity();
		if (!shooter)
			return;
		
		if (!damageSource)
			return;
		
		CRF_VAAR_GamemodeComponent aarGamemodeComponent = CRF_VAAR_GamemodeComponent.GetInstance();
		if (!aarGamemodeComponent)
			return;
		
		if (!aarGamemodeComponent.IsRecording())
			return;
		
		aarGamemodeComponent.RegisterShot(shooter, damageSource, outMat[0][0], outMat[0][2]);
	}
}