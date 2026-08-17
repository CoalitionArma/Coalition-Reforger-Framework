//------------------------------------------------------------------------------------------------
//! Hides the plain "Move Here" waypoint order from the map-based commanding menu for everyone -
//! this is the entry that gives any squad leader a persistent, faction-wide-visible 3D marker/
//! waypoint they can drop anywhere on the map.
//!
//! SCR_WaypointGroupCommand is also the base class for SCR_PatrolGroupCommand,
//! SCR_FollowGroupCommand, SCR_ArtilleryWaypointGroupCommand and
//! SCR_CharacterEntityWaypointGroupCommand, so this only touches the plain, non-subclassed "Move
//! Here" entry (checked via ClassName(), since Commands.conf is a binary asset with no readable
//! command-name string to match against instead) - the other order types are unaffected.
//!
//! Only CanShowOnMap() is hidden, not CanBeShown()/CanRoleShow() - the normal squad-leader radial
//! order (hold the commanding key) stays intact; only the map-click placement path is removed.
modded class SCR_WaypointGroupCommand
{
	//------------------------------------------------------------------------------------------------
	override bool CanShowOnMap()
	{
		if (ClassName() == "SCR_WaypointGroupCommand")
			return false;

		return super.CanShowOnMap();
	}
}
