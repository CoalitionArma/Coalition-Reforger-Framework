class CRF_SpectatorCameraClass : SCR_ManualCameraClass
{
}

class CRF_SpectatorCamera : SCR_ManualCamera
{
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		IEntity playerEntity = SCR_PlayerController.GetLocalMainEntity();

		// Skip initialization if conditions aren't met
		if (!GetGame().InPlayMode() || !CRF_Gamemode.GetInstance() || !playerEntity || !CRF_GamemodeManager.IsSpectator(playerEntity))
			return;

		// Get the slot component for camera positioning
		SlotManagerComponent slotComp = SlotManagerComponent.Cast(owner.FindComponent(SlotManagerComponent));
		if (!slotComp)
			return;

		EntitySlotInfo cameraPoint = slotComp.GetSlotByName("SpectatorEntity");
		if (!cameraPoint)
			return;

		cameraPoint.AttachEntity(playerEntity);
	}
}