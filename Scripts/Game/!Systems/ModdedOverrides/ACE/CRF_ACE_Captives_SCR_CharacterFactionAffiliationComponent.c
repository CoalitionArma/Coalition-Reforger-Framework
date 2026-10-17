//------------------------------------------------------------------------------------------------
//! ACE Captives makes captured and surrendered characters CIV, and on release puts them back on their
//! prefab's default faction. COALITION gives characters their real side after spawning (the slot's
//! faction), so that default is wrong and released players stayed CIV or the wrong side.
//! Here the server remembers the faction a character had while free and gives it back once they are
//! neither captive nor surrendered, whichever way ACE got them there or let them go (user actions,
//! Zeus, death, surrender turning into captivity).
modded class SCR_CharacterFactionAffiliationComponent
{
	protected Faction m_CRF_LastFreeFaction;			// faction while neither captive nor surrendered
	protected Faction m_CRF_FactionBeforeCaptivity;		// set while captive/surrendered, given back on release
	protected Faction m_CRF_PreviousFaction;			// faction before the first change since the last check
	protected bool m_bCRF_CaptivityCheckQueued;
	protected bool m_bCRF_CaptivityPolling;

	//! While captive, how often to check for a release that didn't change the faction (e.g. the
	//! prefab's default is CIV, so ACE's "restore" left it as it was)
	protected static const int CRF_CAPTIVITY_POLL_MS = 1000;

	//------------------------------------------------------------------------------------------------
	override protected void OnFactionChanged(Faction previous, Faction current)
	{
		super.OnFactionChanged(previous, current);

		if (!Replication.IsServer())
			return;

		// ACE sets CIV before flagging a captive but after flagging a surrender, so look once both are done
		if (!m_bCRF_CaptivityCheckQueued)
		{
			m_bCRF_CaptivityCheckQueued = true;
			m_CRF_PreviousFaction = previous;
			GetGame().GetCallqueue().CallLater(CRF_CheckCaptivity);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void CRF_CheckCaptivity()
	{
		m_bCRF_CaptivityCheckQueued = false;

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(GetOwner());
		if (!character)
			return;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!controller)
			return;

		if (controller.ACE_Captives_IsCaptive() || controller.ACE_Captives_HasSurrendered())
		{
			// Keep the first one - surrendering and then being cuffed must not store CIV
			if (!m_CRF_FactionBeforeCaptivity)
			{
				m_CRF_FactionBeforeCaptivity = m_CRF_LastFreeFaction;
				if (!m_CRF_FactionBeforeCaptivity)
					m_CRF_FactionBeforeCaptivity = m_CRF_PreviousFaction;
			}

			if (!m_bCRF_CaptivityPolling)
			{
				m_bCRF_CaptivityPolling = true;
				GetGame().GetCallqueue().CallLater(CRF_CheckCaptivity, CRF_CAPTIVITY_POLL_MS, true);
			}
			return;
		}

		// Free
		if (m_bCRF_CaptivityPolling)
		{
			m_bCRF_CaptivityPolling = false;
			GetGame().GetCallqueue().Remove(CRF_CheckCaptivity);
		}

		if (m_CRF_FactionBeforeCaptivity)
		{
			Faction faction = m_CRF_FactionBeforeCaptivity;
			m_CRF_FactionBeforeCaptivity = null;
			m_CRF_LastFreeFaction = faction;
			if (GetAffiliatedFaction() != faction)
				SetAffiliatedFaction(faction);
			return;
		}

		m_CRF_LastFreeFaction = GetAffiliatedFaction();
	}
}
