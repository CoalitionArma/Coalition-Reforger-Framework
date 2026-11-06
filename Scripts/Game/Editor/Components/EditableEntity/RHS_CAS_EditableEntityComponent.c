// TEMPORARY, LOCAL ONLY: copy of this script from RHS - Status Quo 0.16.5237 by Red Hammer Studios, at the same
// path so it replaces the original, which doesn't compile on Reforger 1.9: SetAuthor gained a parameter. RHS has
// a custom license, so don't publish this without checking it. Delete it as soon as RHS has a 1.9 update.
[ComponentEditorProps(category: "GameScripted/Editor (Editables)", description: "", icon: "WBData/ComponentEditorProps/componentEditor.png")]
class RHS_CAS_EditableEntityComponentClass : SCR_EditableEntityComponentClass
{
	[Attribute(desc: "Configuration of each element spawned via slots.", category: "Timings")]
	protected ref array<ref RHS_CASElementConfig> m_aRHS_CASElementConfig;

	//------------------------------------------------------------------------------------------------
	array<ref RHS_CASElementConfig> GetCASElementsConfig()
	{
		return m_aRHS_CASElementConfig;
	}
}

class RHS_CAS_EditableEntityComponent : SCR_EditableEntityComponent
{
	[Attribute("0", category: "Timings", desc: "Seconds after which to delete the entire component")]
	protected float m_fSelfDeleteTime;

	//------------------------------------------------------------------------------------------------
	/*!
	Event called on server when the entity is placed in the editor.
	\param[out] parent Editable entity in which the new one is being created (rewrite the variable the change the parent)
	\param recipient Entity that receives this editable entity (e.g., a group receiving a waypoint)
	\param isQueue True if the entity was placed in a queue (i.e., placing remains active)
	\param flags Placing flags enabled by user
	\return Editable entity which is added to editor hieraechy (can be overloaded, e.g., to provide group after spawning a character)
	*/
	override SCR_EditableEntityComponent EOnEditorPlace(out SCR_EditableEntityComponent parent, SCR_EditableEntityComponent recipient, EEditorPlacingFlags flags, bool isQueue, int playerID = 0)
	{
		RHS_CAS_EditableEntityComponentClass data = RHS_CAS_EditableEntityComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return super.EOnEditorPlace(parent, recipient, flags, isQueue);

		Instigator instigator;
		if (playerID > -1)
		{
			super.SetAuthor(playerID); // call super version to avoid setting instigator twice for the call in
			IEntity authorEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID);
			instigator = Instigator.CreateInstigator(authorEntity);
			if (instigator)
				instigator.SetInstigatorByPlayerID(playerID); // idk if this is necessary but i dont trunk using entity via replication... what if the author DC for a minute, and he is back when the bomb dropps

		}

		array<ref RHS_CASElementConfig> configs = data.GetCASElementsConfig();

		foreach (RHS_CASElementConfig element : configs)
		{
			if (element.IsWarhead())
			{
				ScheduleTriggerUse(element.GetSlotName(), element.GetDeletionTime());
				if (!instigator)
					continue;

				SetWarheadInstigator(element.GetSlotName(), instigator);
				continue;
			}

			ScheduleEntityDeletion(element.GetSlotName(), element.GetDeletionTime());
		}

		GetGame().GetCallqueue().CallLater(Delete, m_fSelfDeleteTime * 1000, param1: false, param2: false);

		return super.EOnEditorPlace(parent, recipient, flags, isQueue);
	}

	//------------------------------------------------------------------------------------------------
	override void SetAuthor(int playerID, bool isGM = false)
	{
		super.SetAuthor(playerID, isGM);

		RHS_CAS_EditableEntityComponentClass data = RHS_CAS_EditableEntityComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return;

		Instigator instigator = Instigator.CreateInstigatorGM(playerID);
		array<ref RHS_CASElementConfig> configs = data.GetCASElementsConfig();
		foreach (RHS_CASElementConfig element : configs)
		{
			if (!element.IsWarhead())
				continue;

			SetWarheadInstigator(element.GetSlotName(), instigator);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void Trigger(notnull IEntity ent)
	{
		RplComponent rplComp = RplComponent.Cast(ent.FindComponent(RplComponent));
		if (!rplComp)
			return;

		GetGame().GetCallqueue().CallLater(RPC_DoTrigger, param1: rplComp.Id());
		Rpc(RPC_DoTrigger, rplComp.Id());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_DoTrigger(RplId rplCompId)
	{
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(rplCompId));
		if (!rplComp)
			return;

		IEntity warhead = rplComp.GetEntity();
		if (!warhead)
			return;

		BaseTriggerComponent triggerComponent = BaseTriggerComponent.Cast(warhead.FindComponent(BaseTriggerComponent));
		if (!triggerComponent)
			return;

		triggerComponent.OnUserTrigger(warhead);
	}

	//------------------------------------------------------------------------------------------------
	protected void ScheduleEntityDeletion(string slotName, int delay)
	{
		IEntity owner = this.GetOwner();

		if (!owner)
			return;

		SlotManagerComponent slotManager = SlotManagerComponent.Cast(owner.FindComponent(SlotManagerComponent));

		if (!slotManager)
			return;

		EntitySlotInfo slot = slotManager.GetSlotByName(slotName);

		if (!slot || !slot.GetAttachedEntity())
			return;

		GetGame().GetCallqueue().CallLater(DeleteEntity, delay, param1: slot.GetAttachedEntity());
	}

	//------------------------------------------------------------------------------------------------
	protected void ScheduleTriggerUse(string slotName, int delay)
	{
		IEntity owner = this.GetOwner();
		if (!owner)
			return;

		SlotManagerComponent slotManager = SlotManagerComponent.Cast(owner.FindComponent(SlotManagerComponent));
		if (!slotManager)
			return;

		EntitySlotInfo slot = slotManager.GetSlotByName(slotName);
		if (!slot || !slot.GetAttachedEntity())
			return;

		GetGame().GetCallqueue().CallLater(Trigger, delay, param1: slot.GetAttachedEntity());
	}

	//------------------------------------------------------------------------------------------------
	protected void SetWarheadInstigator(string slotName, notnull Instigator instigator)
	{
		IEntity owner = this.GetOwner();
		if (!owner)
			return;

		SlotManagerComponent slotManager = SlotManagerComponent.Cast(owner.FindComponent(SlotManagerComponent));
		if (!slotManager)
			return;

		EntitySlotInfo slot = slotManager.GetSlotByName(slotName);
		if (!slot || !slot.GetAttachedEntity())
			return;

		IEntity warhead = slot.GetAttachedEntity();
		if (!warhead)
			return;

		BaseTriggerComponent trigger = BaseTriggerComponent.Cast(warhead.FindComponent(BaseTriggerComponent));
		if (trigger)
			trigger.SetInstigator(instigator);
	}

	//------------------------------------------------------------------------------------------------
	protected void DeleteEntity(notnull IEntity ent)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(ent);
	}
}
