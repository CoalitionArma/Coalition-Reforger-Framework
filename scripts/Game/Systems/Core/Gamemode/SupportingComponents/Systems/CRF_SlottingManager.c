class CRF_SlottingManagerClass : ScriptComponentClass {}

class CRF_SlottingManager : ScriptComponent
{
	// Cannot replicate maps, so we use this array to replicate all map data (in correllation with the map key array above).
	[RplProp()]
	protected ref array<ref CRF_SlotDataContainer> m_aSlotsData = {}; 
	
	// Script Invoker for all your invoker needs
	protected ref ScriptInvoker m_OnSlottingUpdate;
	
	// How we propagate slotting updates
	[RplProp(onRplName: "UpdateClient")]
	protected int m_SlottingUpdate;
	
	protected CRF_Gamemode m_Gamemode;
	
	//------------------------------------------------------------------------------------------------
	static CRF_SlottingManager GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (gameMode)
			return CRF_SlottingManager.Cast(gameMode.FindComponent(CRF_SlottingManager));
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	// Init method
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Gamemode = CRF_Gamemode.GetInstance();
	};
	
	//------------------------------------------------------------------------------------------------
	void SendUpdateClient()
	{
		m_SlottingUpdate++;
		Replication.BumpMe();
		UpdateClient();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateClient()
	{
		if (m_OnSlottingUpdate)
			m_OnSlottingUpdate.Invoke();
	}
	
	//------------------------------------------------------------------------------------------------
	ScriptInvoker GetOnSlottingUpdate()
	{
		if (!m_OnSlottingUpdate)
			m_OnSlottingUpdate = new ScriptInvoker();

		return m_OnSlottingUpdate;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetSlotData(int slotId)
	{
		return m_aSlotsData.Get(slotId);
	}
	
	//------------------------------------------------------------------------------------------------
	array<ref CRF_SlotDataContainer> GetSlotArray()
	{
		return m_aSlotsData;
	}
	
	//------------------------------------------------------------------------------------------------
	array<SCR_AIGroup> GetAllGroups(FactionKey factionKey = "")
	{
		map<int, SCR_AIGroup> tempHoldermap = new map<int, SCR_AIGroup>;
		array<SCR_AIGroup> outputArray = {};
		array<int> sortingArray = {};
		
		foreach (CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(!slotData || !slotData.GetSlotCurrentGroup() || slotData.GetSlotCurrentGroup() == RplId.Invalid() || !Replication.FindItem(slotData.GetSlotCurrentGroup()))
				continue;
			
			if(factionKey.IsEmpty() || (!factionKey.IsEmpty() && slotData.GetSlotFactionKey() == factionKey))
			{
				SCR_AIGroup tempGroup = SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(slotData.GetSlotCurrentGroup())).GetEntity());
				
				if (tempGroup && !tempHoldermap.Get(tempGroup.GetGroupID()))
					tempHoldermap.Set(tempGroup.GetGroupID(), tempGroup);
			}
		}
		
		foreach (int Id, SCR_AIGroup groupToSort : tempHoldermap)
			sortingArray.Insert(Id);
		
		sortingArray.Sort(false);
		
		foreach (int Id : sortingArray)
		{
			SCR_AIGroup group = tempHoldermap.Get(Id);
			outputArray.Insert(group);
		}
		
		return outputArray;
	}
	
	//------------------------------------------------------------------------------------------------
	array<int> GetAllSlotIDsForGroup(RplId rplId)
	{
		array<int> outputArray = {};
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentGroup() == rplId)
				outputArray.Insert(slotID);
		}
		
		return outputArray;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetSlotDataFromCharacter(RplId rplId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentCharacter() == rplId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	int GetPlayerSlotID(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentPlayerId() == playerId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	CRF_SlotDataContainer GetPlayerSlotData(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentPlayerId() == playerId)
				return slotData;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroup GetPlayerSlotGroup(int playerId)
	{
		CRF_SlotDataContainer data = GetPlayerSlotData(playerId);
		
		if(!data || !data.GetSlotCurrentGroup() || data.GetSlotCurrentGroup() == RplId.Invalid() || !Replication.FindItem(data.GetSlotCurrentGroup()))
			return null;
		
		return SCR_AIGroup.Cast(RplComponent.Cast(Replication.FindItem(data.GetSlotCurrentGroup())).GetEntity());
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_ChimeraCharacter GetPlayerSlotCharacter(int playerId)
	{
		CRF_SlotDataContainer data = GetPlayerSlotData(playerId);
		
		if(!data || !data.GetSlotCurrentCharacter() || data.GetSlotCurrentCharacter() == RplId.Invalid() || !Replication.FindItem(data.GetSlotCurrentCharacter()))
			return null;
		
		// Get ChimeraCharacter so we can pull the controller
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(RplComponent.Cast(Replication.FindItem(data.GetSlotCurrentCharacter())).GetEntity());
		
		if (!character)
			return null;
	
		// Get the controller from the character
		CharacterControllerComponent controller = character.GetCharacterController();
	
		// If the character is a valid character and is not dead then return that this guys character
		if (controller && !controller.IsDead())
			return character;
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	Faction GetPlayerSlotFaction(int playerId)
	{
		if(GetPlayerSlotData(playerId) && (!(GetPlayerSlotData(playerId).GetSlotFactionKey()).IsEmpty()))
			return GetGame().GetFactionManager().GetFactionByKey(GetPlayerSlotData(playerId).GetSlotFactionKey());
		else
			return null;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetPlayerSlotResource(int playerId)
	{
		return GetPlayerSlotData(playerId).GetSlotResource();
	}
	
	//------------------------------------------------------------------------------------------------
	void GetPlayerSlotVector(int playerId, out vector vec[4])
	{
		GetPlayerSlotData(playerId).GetSlotVector(vec);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetCharacterSlotID(IEntity entity)
	{
		RplId rplId = RplComponent.Cast(entity.FindComponent(RplComponent)).Id();
		
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentCharacter() == rplId)
				return slotID;
		}
		
		return -1;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsFactionValid(FactionKey factionKey)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotFactionKey() == factionKey)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerInASlot(int playerId)
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentPlayerId() == playerId)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsPlayerConsideredDead(int playerId)
	{
		CRF_SlotDataContainer slotData = GetPlayerSlotData(playerId);
		if(slotData)
		{
			return slotData.GetIsDeadSlot();
		} else
			return null;
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotLockedState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = m_aSlotsData.Get(slotId);
		slotData.SetIsLockedSlot(input);
		
		SendUpdateClient();
	}

	//------------------------------------------------------------------------------------------------
	void UpdateSlotDeathState(int slotId, bool input)
	{
		CRF_SlotDataContainer slotData = m_aSlotsData.Get(slotId);
		slotData.SetIsDeadSlot(input);
		
		SendUpdateClient();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotPlayerID(int slotId, int playerId)
	{
		if(playerId <= 0)
		{
			CRF_SlotDataContainer data = GetSlotData(slotId);
			
			if(data && data.GetSlotCurrentCharacter() && data.GetSlotCurrentCharacter() != RplId.Invalid() && Replication.FindItem(data.GetSlotCurrentCharacter()))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(SCR_ChimeraCharacter.Cast(RplComponent.Cast(Replication.FindItem(data.GetSlotCurrentCharacter())).GetEntity()));
				UpdateSlotCharacter(slotId, RplId.Invalid());
			};
		}
	
		CRF_SlotDataContainer slotData = m_aSlotsData.Get(slotId);
		slotData.SetSlotCurrentPlayerId(playerId);
		
		SendUpdateClient();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotGroup(int slotId, RplId groupId)
	{
		CRF_SlotDataContainer slotData = m_aSlotsData.Get(slotId);
		slotData.SetSlotCurrentGroup(groupId);
		
		SendUpdateClient();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotResource(int slotId, ResourceName resource)
	{
		CRF_SlotDataContainer slotData = m_aSlotsData.Get(slotId);
		slotData.SetSlotResource(resource);
		
		SendUpdateClient();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdateSlotCharacter(int slotId, RplId charId)
	{
		CRF_SlotDataContainer slotData = m_aSlotsData.Get(slotId);
		slotData.SetSlotCurrentCharacter(charId);
		
		SendUpdateClient();
	}

	//------------------------------------------------------------------------------------------------
	void LockAllOpenSlots()
	{
		foreach (int slotID, CRF_SlotDataContainer slotData : m_aSlotsData)
		{
			if(slotData.GetSlotCurrentPlayerId() <= 0)
				UpdateSlotLockedState(slotID, true);
		}
		
		array<SCR_AIGroup> allGroups = GetAllGroups();
		
		// Process each group and its players
		foreach(SCR_AIGroup group : allGroups)
		{	
			// Skip already private groups
			if(group.IsPrivate())
				continue;
			
			int playersInGroup = 0;
			
			// Get group ID
			RplId groupId = RplComponent.Cast(group.FindComponent(RplComponent)).Id();
			
			// Process all slots in this group
			foreach(int slotId, CRF_SlotDataContainer slotData : m_aSlotsData)
			{
				if(slotData.GetSlotCurrentGroup() != groupId)
					continue;
				
				// Count slots
				if (slotData.GetSlotCurrentPlayerId() > 0)
					playersInGroup++;
			};
							
			// lock empty groups
			if (playersInGroup == 0)
				group.SetPrivate(true);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void AddPlayableEntityToManager(IEntity entity)
	{
		if(RplSession.Mode() == RplMode.Client)
			return;
		
		CRF_PlayableCharacter playableCharComp = CRF_PlayableCharacter.Cast(entity.FindComponent(CRF_PlayableCharacter));
		
		if(!playableCharComp || !playableCharComp.m_bIsPlayable)
			return;
		
		SCR_EditableCharacterComponent editableCharComp = SCR_EditableCharacterComponent.Cast(entity.FindComponent(SCR_EditableCharacterComponent));
		SCR_AIGroup group = SCR_AIGroup.Cast(ChimeraAIControlComponent.Cast(entity.FindComponent(ChimeraAIControlComponent)).GetControlAIAgent().GetParentGroup());
		
		CRF_SlotDataContainer slotData = new CRF_SlotDataContainer;
		
		if(group)
		{
			slotData.SetSlotCurrentGroup(RplComponent.Cast(group.FindComponent(RplComponent)).Id());
			slotData.SetSlotFactionKey(group.GetFaction().GetFactionKey());
		} else 
			slotData.SetSlotFactionKey(SCR_FactionAffiliationComponent.Cast(entity.FindComponent(SCR_FactionAffiliationComponent)).GetAffiliatedFactionKey());
		
		vector tempVec[4];
		entity.GetWorldTransform(tempVec);
		slotData.SetSlotVector(tempVec);
		
		slotData.SetSlotResource(entity.GetPrefabData().GetPrefabName());
		slotData.SetSlotCurrentCharacter(RplComponent.Cast(entity.FindComponent(RplComponent)).Id());
		
		if (!playableCharComp.m_sName.IsEmpty())
			slotData.SetSlotName(playableCharComp.m_sName);
		else
			slotData.SetSlotName(editableCharComp.GetDisplayName());	
		
		slotData.SetSlotIcon(editableCharComp.GetInfo().GetIconPath());
		slotData.SetSlotType(playableCharComp.m_SlottingRole);
				
		m_aSlotsData.Insert(slotData);
		SendUpdateClient();
		
		if(m_Gamemode.m_GamemodeState != CRF_EGamemodeState.GAME)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
}