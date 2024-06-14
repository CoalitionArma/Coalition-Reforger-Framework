class CRF_CacheHuntTriggerEntityClass: SCR_BaseTriggerEntityClass {};

class CRF_CacheHuntTriggerEntity: SCR_BaseTriggerEntity 
{
	IEntity cache1, cache2, cache3;
	IEntity dump1, dump2, dump3;
	int i = 0;
	
	/*Print("CRF Trigger loop");
	IEntity dump1ent = GetGame().GetWorld().FindEntityByName("dump1");
	SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(dump1ent.FindComponent(SCR_DamageManagerComponent));
	if (damageManager.GetState() == EDamageState.DESTROYED)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg("Cache 1 has been destroyed!", 10);
		IEntity cache1ent = GetGame().GetWorld().FindEntityByName("cache1");
		delete cache1ent;
	}*/
	
	override void OnActivate(IEntity ent)
	{
		PrintFormat("CRF OnActivate %1",i);
		i++;
	}
			
}