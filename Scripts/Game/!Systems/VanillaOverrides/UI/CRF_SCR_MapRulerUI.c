//------------------------------------------------------------------------------------------------
//! The protractor remembers it was out and reopens with the next map, any map. Vanilla's SetVisible shows its
//! frame first and only then looks for the map the player is holding - with none (dead, spectating, the AAR
//! map) it returns before sizing the protractor or hooking map zoom, so it sits on screen at its old size and
//! never scales. It only belongs on a map you're holding: without one it stays hidden. Whether it was out is
//! still remembered, so it comes back the next time you open your own map.
modded class SCR_MapRulerUI
{
	//------------------------------------------------------------------------------------------------
	override protected void SetVisible(bool visible, bool saveState = false)
	{
		if (visible && !CRF_IsHoldingMap())
			visible = false;

		super.SetVisible(visible, saveState);
	}

	//------------------------------------------------------------------------------------------------
	//! A living local character with a map gadget, found the way vanilla's SetVisible finds it
	protected bool CRF_IsHoldingMap()
	{
		ChimeraCharacter player = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!player)
			return false;

		CharacterControllerComponent controller = player.GetCharacterController();
		if (!controller || controller.IsDead())
			return false;

		IEntity mapItem = controller.GetAttachedGadgetAtLeftHandSlot();
		if (!mapItem)
		{
			SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(player);
			if (gadgetManager)
				mapItem = gadgetManager.GetGadgetByType(EGadgetType.MAP);
		}

		return mapItem != null && mapItem.FindComponent(SCR_MapGadgetComponent) != null;
	}
}
