//------------------------------------------------------------------------------------
// CRF_PatrolCheckpointComponent: one checkpoint on a Patrol & Ambush route.
//
// Add it to a GenericEntity placed in the mission at the checkpoint (any small entity will do; it isn't
// drawn). Number the checkpoints in route order with m_iOrder; the highest number is the end of the route
// (the extraction point). See CRF_PatrolGamemodeComponent.
//------------------------------------------------------------------------------------

[ComponentEditorProps(category: "CRF | Modular", description: "A checkpoint on a Patrol & Ambush route. Place one per checkpoint and number them in route order.")]
class CRF_PatrolCheckpointComponentClass : ScriptComponentClass {}

class CRF_PatrolCheckpointComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Auto, "Position on the route: 1 = first checkpoint. The highest number is the end of the route.", category: "Patrol Checkpoint")]
	int m_iOrder;

	[Attribute("Checkpoint", UIWidgets.EditBox, "Name the players see, e.g. \"Bravo\" or \"Bridge\".", category: "Patrol Checkpoint")]
	string m_sCheckpointName;

	[Attribute("30", UIWidgets.Auto, "Metres from the checkpoint the patrol has to come within.", category: "Patrol Checkpoint")]
	float m_fRadius;

	// Every checkpoint in the mission, for the gamemode to collect when the round starts
	protected static ref array<CRF_PatrolCheckpointComponent> s_aCheckpoints = {};

	static array<CRF_PatrolCheckpointComponent> GetCheckpoints()
	{
		return s_aCheckpoints;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aCheckpoints.Contains(this))
			s_aCheckpoints.Insert(this);
	}

	void ~CRF_PatrolCheckpointComponent()
	{
		if (s_aCheckpoints)
			s_aCheckpoints.RemoveItem(this);
	}

	vector GetPosition()
	{
		return GetOwner().GetOrigin();
	}
}
