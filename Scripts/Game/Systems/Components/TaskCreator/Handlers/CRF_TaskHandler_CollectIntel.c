//=============================================================================
// CRF_TaskHandler_CollectIntel.c
// Handler for CRF_EObjectiveTaskType.COLLECT_INTEL tasks.
//
// Behaviour: player interacts once with an intel object (documents, data
// drive, etc.) which immediately completes the task via a server RPC.
// No extra CanPerform conditions — any eligible player can collect.
// =============================================================================

[BaseContainerProps()]
class CRF_TaskHandler_CollectIntel : CRF_BaseTaskHandler
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab spawned at the task anchor. Must carry CRF_TaskCreatorObjectComponent and CRF_TaskCreatorAction.", params: "et", category: "Collect Intel")]
	ResourceName m_sPrefab;

	override string GetActionName(int taskObjectState)
	{
		return "Collect Intel";
	}

	override ResourceName GetPrefab()
	{
		return m_sPrefab;
	}

	override void OnPerform(int taskIndex, int taskObjectState, IEntity user)
	{
		CRF_PlayerRplToAuthorityManager rplManager = CRF_PlayerRplToAuthorityManager.GetInstance();
		if (!rplManager)
			return;

		rplManager.RequestObjectiveTaskComplete(taskIndex, GetUserSide(user));
	}
}
