//------------------------------------------------------------------------------------------------
//! Fixes for nametags going missing on players outside of the local players group.
//! Layers on top of CSI's SCR_NameTagDisplay override - CRF depends on CSI, so this modded class
//! wraps CSI's, which in turn wraps vanilla.
modded class SCR_NameTagDisplay
{
	protected SCR_CharacterFactionAffiliationComponent m_CRFFactionAffiliation;	// hooked component, kept only so it can be unhooked symmetrically

	//------------------------------------------------------------------------------------------------
	//! Re-point the faction-changed hook at the newly controlled character and resolve the initial
	//! faction for its tag.
	//!
	//! InitializeTag() has just latched m_CurrentFaction off the new character (inside the super call
	//! above), which is null whenever the slot faction has not reached the client yet -
	//! COA_Player.et clears the prefab faction ("faction affiliation" ""), and
	//! COA_GamemodeManager.SpawnPlayableCharacter() only applies the slot faction after
	//! SpawnEntityPrefab() has already published the entity, so the character can reach the client
	//! with no affiliation at all rather than merely the wrong one. CheckFilters() then bails out on a
	//! null m_CurrentFaction before it reaches the faction relation filters, so anyone who is not in
	//! the local players own group is silently denied a tag - the group branch above it still passes,
	//! which is why the own squad keeps its nametags while every other group loses theirs.
	//!
	//! Only the controlled entitys own tag exists at this point - RefreshTags() (called by super, via
	//! ProcessFiltered) wiped every other tag and rebuilt just that one - so nothing has been filtered
	//! against the faction yet and filling it in here needs no further rebuild.
	override void DisplayControlledEntityChanged(IEntity from, IEntity to)
	{
		super.DisplayControlledEntityChanged(from, to);

		UnhookCRFFactionChanged();

		if (!to)
			return;

		m_CRFFactionAffiliation = SCR_CharacterFactionAffiliationComponent.Cast(to.FindComponent(SCR_CharacterFactionAffiliationComponent));
		if (m_CRFFactionAffiliation)
			m_CRFFactionAffiliation.GetOnFactionChanged().Insert(OnCRFFactionChanged);

		if (m_CurrentFaction)
			return;

		Faction faction;
		if (m_CRFFactionAffiliation)
			faction = m_CRFFactionAffiliation.GetAffiliatedFaction();

		// The player controller is given the same slot faction before the handover
		// (InitilizePlayer calls AssignFactionToPlayer before AssignCharacterToPlayer) and replicates
		// through SCR_FactionManager on its own path, so it is available even when the characters own
		// affiliation has not landed yet. OnCRFFactionChanged below takes over as the authoritative
		// source the moment the characters own affiliation actually fires.
		if (!faction)
			faction = SCR_FactionManager.SGetLocalPlayerFaction();

		if (faction)
			m_CurrentFaction = faction;
	}

	//------------------------------------------------------------------------------------------------
	//! SCR_CharacterFactionAffiliationComponent event - fires the moment the local characters own
	//! affiliation actually lands or changes (spawn catch-up, side swap, GM possession, ...).
	protected void OnCRFFactionChanged(FactionAffiliationComponent owner, Faction previousFaction, Faction newFaction)
	{
		if (!newFaction || newFaction == m_CurrentFaction)
			return;

		// Every existing tag was filtered against the old (possibly null) faction, so all of them have
		// to be re-evaluated now that the real one is known.
		m_CurrentFaction = newFaction;

		IEntity localEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (localEntity)
			RefreshTags(localEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected void UnhookCRFFactionChanged()
	{
		if (!m_CRFFactionAffiliation)
			return;

		m_CRFFactionAffiliation.GetOnFactionChanged().Remove(OnCRFFactionChanged);
		m_CRFFactionAffiliation = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Safety net for teardown paths that stop the display without routing back through
	//! DisplayControlledEntityChanged(to: null) first, so the hook cannot outlive the display.
	override void DisplayStopDraw(IEntity owner)
	{
		super.DisplayStopDraw(owner);

		UnhookCRFFactionChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Drop m_CurrentPlayerTag when the tag it points at is wiped.
	//!
	//! SCR_NameTagData.SetGroup() calls CleanupAllTags() whenever the local player joins or leaves a
	//! group so that group membership can be compared again, but unlike RefreshTags() it leaves
	//! m_CurrentPlayerTag pointing at the tag that was just recycled into the uninitialized pool.
	//! ProcessFiltered() only rebuilds the controlled entitys tag when m_CurrentPlayerTag is null, so
	//! the stale object stays in place and can be handed out to another character - the ruleset then
	//! measures every distance from that characters position instead of ours and tags drop out.
	//!
	//! The Find() guard keeps the init path intact: InitializeTag() assigns m_CurrentPlayerTag before
	//! calling InitTag(), which reaches CleanupAllTags() through SetGroup() while the tag is not yet in
	//! m_aNameTags. Nulling it there would make ProcessFiltered() rebuild the tag every single frame.
	override void CleanupAllTags()
	{
		bool currentTagWiped = m_CurrentPlayerTag && m_aNameTags.Find(m_CurrentPlayerTag) != -1;

		super.CleanupAllTags();

		if (currentTagWiped)
			m_CurrentPlayerTag = null;
	}
}
