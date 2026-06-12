/*
	Cross-Faction Respawn
	
	Optional gamemode component. When enabled, players who die on a configured source faction
	are moved into a pre-defined mirror slot on a target faction before the normal respawn
	flow runs — target faction tickets, spawn points, wave timer, and cutoff all apply as usual.
	
	Each mapping must name an explicit target group (e.g. BLUFOR 1-1 -> INDFOR 1-1A,
	OPFOR 1-1 -> INDFOR 1-1B). The target group and matching slot must already exist in
	slotting; the player is placed at the same slot index with the same role.
*/

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_eSourceFaction")]
class CRF_CrossFactionRespawnMapping
{
	[Attribute("0", UIWidgets.ComboBox, "Source faction whose deaths redirect to the target faction.", enums: ParamEnumArray.FromEnum(CRF_HVTFaction))]
	CRF_HVTFaction m_eSourceFaction;
	
	[Attribute("0", UIWidgets.ComboBox, "Faction the player respawns into after death.", enums: ParamEnumArray.FromEnum(CRF_HVTFaction))]
	CRF_HVTFaction m_eTargetFaction;
	
	[Attribute("", "auto", "Only redirect players in source groups whose name contains this substring (case-insensitive). Empty = all groups on the source faction.")]
	string m_sSourceCallsignFilter;
	
	[Attribute("", "auto", "Target group callsign substring (case-insensitive). Required — must match a group already defined in target faction slotting (e.g. 1-1A, BLUFOR 1-1).")]
	string m_sTargetCallsignFilter;
	
	string GetSourceFactionKey()
	{
		return CRF_CrossFactionRespawnManager.FactionEnumToKey(m_eSourceFaction);
	}
	
	string GetTargetFactionKey()
	{
		return CRF_CrossFactionRespawnManager.FactionEnumToKey(m_eTargetFaction);
	}
}

[ComponentEditorProps(category: "Game Mode Component", description: "Redirect dead players from configured source factions into target faction slots on respawn")]
class CRF_CrossFactionRespawnManagerClass : SCR_BaseGameModeComponentClass {}

class CRF_CrossFactionRespawnManager : SCR_BaseGameModeComponent
{
	[Attribute("false", "auto", "Enable cross-faction respawn for configured source factions.", category: "Cross-Faction Respawn")]
	bool m_bEnabled;
	
	[Attribute("", "auto", "Per-squad redirect rules. Each entry must specify a target group callsign.", category: "Cross-Faction Respawn")]
	ref array<ref CRF_CrossFactionRespawnMapping> m_aMappings;
	
	protected CRF_SlottingManager m_SlottingManager;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (RplSession.Mode() == RplMode.Client)
			return;
		
		m_SlottingManager = CRF_SlottingManager.GetInstance();
	}
	
	//------------------------------------------------------------------------------------------------
	static string FactionEnumToKey(CRF_HVTFaction faction)
	{
		switch (faction)
		{
			case CRF_HVTFaction.BLUFOR:  return "BLUFOR";
			case CRF_HVTFaction.OPFOR:   return "OPFOR";
			case CRF_HVTFaction.INDFOR:  return "INDFOR";
			case CRF_HVTFaction.CIV:     return "CIV";
		}
		
		return "";
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsEnabled()
	{
		return m_bEnabled && m_aMappings && !m_aMappings.IsEmpty();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Vacates the source slot and assigns the player to the mirror target slot.
	//! \param[out] respawnFactionKey Faction used for respawn eligibility after transfer (unchanged when no redirect applies)
	//! \return True when the player was moved to a target faction slot
	bool TryTransferPlayerOnDeath(int playerId, out FactionKey respawnFactionKey)
	{
		if (!IsEnabled() || playerId <= 0 || !m_SlottingManager)
			return false;
		
		CRF_SlotData sourceSlot = m_SlottingManager.GetPlayerSlotData(playerId);
		if (!sourceSlot)
			return false;
		
		FactionKey sourceFactionKey = sourceSlot.GetSlotFactionKey();
		if (sourceFactionKey.IsEmpty())
			return false;
		
		SCR_AIGroup sourceGroup = CRF_EntityHelper.GetGroupFromRplId(sourceSlot.GetSlotCurrentGroup());
		if (!sourceGroup)
			return false;
		
		CRF_CrossFactionRespawnMapping mapping = FindMappingForPlayer(sourceFactionKey, sourceGroup);
		if (!mapping)
			return false;
		
		FactionKey targetFactionKey = mapping.GetTargetFactionKey();
		if (targetFactionKey.IsEmpty() || targetFactionKey == sourceFactionKey)
			return false;
		
		string targetCallsignFilter = mapping.m_sTargetCallsignFilter;
		if (targetCallsignFilter.IsEmpty())
			return false;
		
		SCR_AIGroup targetGroup = FindGroupByFactionAndCallsign(targetFactionKey, targetCallsignFilter);
		if (!targetGroup)
		{
			Print(string.Format("[CrossFactionRespawn] No target group '%1' on %2 for player %3. Define it in target faction slotting.", targetCallsignFilter, targetFactionKey, playerId), LogLevel.WARNING);
			return false;
		}
		
		int sourceSlotId = m_SlottingManager.GetPlayerSlotID(playerId);
		int sourceIndex = m_SlottingManager.GetSlotIndexInGroup(sourceSlotId);
		CRF_EGearRole role = sourceSlot.GetSlotRole();
		
		int targetSlotId = FindMirrorTargetSlot(targetGroup, role, sourceIndex);
		if (targetSlotId <= 0)
		{
			Print(string.Format("[CrossFactionRespawn] No available mirror slot (index %1, role) in '%2' for player %3.", sourceIndex, targetCallsignFilter, playerId), LogLevel.WARNING);
			return false;
		}
		
		VacateSourceSlot(sourceSlotId);
		m_SlottingManager.ForceUpdateSlotPlayerID(targetSlotId, playerId);
		m_SlottingManager.UpdateSlotDeathState(targetSlotId, true);
		
		respawnFactionKey = targetFactionKey;
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected CRF_CrossFactionRespawnMapping FindMappingForPlayer(FactionKey sourceFactionKey, SCR_AIGroup sourceGroup)
	{
		string groupName = sourceGroup.GetCustomNameWithOriginal();
		groupName.ToLower();
		
		foreach (CRF_CrossFactionRespawnMapping mapping : m_aMappings)
		{
			if (!mapping)
				continue;
			
			if (mapping.GetSourceFactionKey() != sourceFactionKey)
				continue;
			
			if (mapping.GetTargetFactionKey().IsEmpty() || mapping.GetSourceFactionKey() == mapping.GetTargetFactionKey())
				continue;
			
			if (mapping.m_sTargetCallsignFilter.IsEmpty())
				continue;
			
			if (!mapping.m_sSourceCallsignFilter.IsEmpty())
			{
				string filterLower = mapping.m_sSourceCallsignFilter;
				filterLower.ToLower();
				if (groupName.IndexOf(filterLower) == -1)
					continue;
			}
			
			return mapping;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	protected SCR_AIGroup FindGroupByFactionAndCallsign(string factionKey, string callsignFilter)
	{
		if (factionKey.IsEmpty() || callsignFilter.IsEmpty() || !m_SlottingManager)
			return null;
		
		string filterLower = callsignFilter;
		filterLower.ToLower();
		
		array<SCR_AIGroup> groups = m_SlottingManager.GetAllGroups(factionKey);
		foreach (SCR_AIGroup group : groups)
		{
			if (!group)
				continue;
			
			string groupName = group.GetCustomNameWithOriginal();
			groupName.ToLower();
			if (groupName.IndexOf(filterLower) != -1)
				return group;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns the target slot at the same group index with the same role, or -1 when unavailable.
	protected int FindMirrorTargetSlot(SCR_AIGroup targetGroup, CRF_EGearRole role, int sourceIndex)
	{
		RplComponent groupRplComp = RplComponent.Cast(targetGroup.FindComponent(RplComponent));
		if (!groupRplComp)
			return -1;
		
		array<int> slotIds = m_SlottingManager.GetAllSlotIDsForGroup(groupRplComp.Id());
		if (sourceIndex < 0 || sourceIndex >= slotIds.Count())
			return -1;
		
		int targetSlotId = slotIds[sourceIndex];
		CRF_SlotData targetSlot = m_SlottingManager.GetSlotData(targetSlotId);
		if (!targetSlot || targetSlot.GetSlotRole() != role)
			return -1;
		
		if (!IsSlotAvailableForTransfer(targetSlot))
			return -1;
		
		return targetSlotId;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool IsSlotAvailableForTransfer(CRF_SlotData slotData)
	{
		if (!slotData)
			return false;
		
		if (slotData.GetIsLockedSlot())
			return false;
		
		return slotData.GetSlotCurrentPlayerId() <= 0;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void VacateSourceSlot(int sourceSlotId)
	{
		m_SlottingManager.UpdateSlotDeathState(sourceSlotId, false);
		m_SlottingManager.UpdateSlotCharacter(sourceSlotId, RplId.Invalid());
		m_SlottingManager.UpdateSlotPlayerID(sourceSlotId, 0);
	}
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_CrossFactionRespawnManager s_Instance;
	
	void CRF_CrossFactionRespawnManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		s_Instance = this;
	}
	
	//------------------------------------------------------------------------------------------------
	static CRF_CrossFactionRespawnManager GetInstance()
	{
		return s_Instance;
	}
}
