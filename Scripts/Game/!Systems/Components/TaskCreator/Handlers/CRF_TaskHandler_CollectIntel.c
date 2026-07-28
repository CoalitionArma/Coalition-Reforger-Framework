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

	[Attribute("Collect Intel", UIWidgets.EditBox, "Text shown on the interaction action.", category: "Collect Intel")]
	string m_sActionName;

	override string GetActionName(int taskObjectState)
	{
		return m_sActionName;
	}

	override ResourceName GetPrefab()
	{
		return m_sPrefab;
	}

	override void OnPerform(int taskIndex, int taskObjectState, IEntity user)
	{
		COA_PlayerRplToAuthorityManager rplManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!rplManager)
			return;

		rplManager.RequestObjectiveTaskComplete(taskIndex, GetUserSide(user));
	}
}
