//------------------------------------------------------------------------------------------------
//! Safestart voice range: while safestart is on, everyone's voice range is set to Close Contact, so the
//! briefing at the spawn isn't drowned out; when it ends (the round goes live) everyone is set back to
//! Normal. Players can still change their range during safestart (the voice keys), and anyone joining
//! during safestart starts on Close Contact too.
//!
//! Server only: CVON keeps every player's range in its replicated m_aPlayerVolumes, so the clients (and
//! their HUD) follow. Safestart comes from COA_SafestartManager.m_OnSafeStartChange, which the server
//! raises itself on both edges (on: safestart begins; off: the round goes live, or an admin ends a
//! safestart they turned back on).
modded class CVON_VONGameModeComponent
{
	protected static const int CRF_SAFESTART_VOICE_TIER = 1;	//!< SCR_PlayerController.m_aVolumeValues: Close Contact
	protected static const int CRF_LIVE_VOICE_TIER = 2;			//!< ... Normal
	protected static const int CRF_SAFESTART_HOOK_RETRY_MS = 1000;

	protected COA_SafestartManager m_CRF_Safestart;
	protected bool m_bCRF_SafestartVoice;	//!< everyone was put on Close Contact for this safestart

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (GetGame().InPlayMode() && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(CRF_HookSafestart, CRF_SAFESTART_HOOK_RETRY_MS, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(CRF_HookSafestart);

		if (m_CRF_Safestart && m_CRF_Safestart.m_OnSafeStartChange)
			m_CRF_Safestart.m_OnSafeStartChange.Remove(CRF_OnSafeStartChange);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the safestart manager may initialise after this component, so wait for it
	protected void CRF_HookSafestart()
	{
		m_CRF_Safestart = COA_SafestartManager.GetInstance();
		if (!m_CRF_Safestart)
		{
			GetGame().GetCallqueue().CallLater(CRF_HookSafestart, CRF_SAFESTART_HOOK_RETRY_MS, false);
			return;
		}

		m_CRF_Safestart.m_OnSafeStartChange.Insert(CRF_OnSafeStartChange);

		// Already in safestart (hooked late)
		if (m_CRF_Safestart.GetSafestartStatus())
			CRF_OnSafeStartChange(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: safestart on -> everyone to Close Contact; off -> everyone back to Normal (only after a
	//! safestart that set them: the Lobby turns safestart off for a moment as the mission enters the game
	//! state, and that mustn't reset ranges players chose during the briefing)
	protected void CRF_OnSafeStartChange(bool enabled)
	{
		if (!Replication.IsServer())
			return;

		if (enabled)
		{
			m_bCRF_SafestartVoice = true;
			CRF_SetEveryoneVoiceTier(CRF_SAFESTART_VOICE_TIER);
			Print("[CRF] Safestart: everyone's voice range set to Close Contact", LogLevel.NORMAL);
			return;
		}

		if (!m_bCRF_SafestartVoice)
			return;

		m_bCRF_SafestartVoice = false;
		CRF_SetEveryoneVoiceTier(CRF_LIVE_VOICE_TIER);
		Print("[CRF] Safestart over: everyone's voice range set to Normal", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Server: a player joining during safestart starts on Close Contact like everyone else
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);

		if (!Replication.IsServer() || !m_bCRF_SafestartVoice)
			return;

		int index = m_aPlayerIds.Find(playerId);
		if (index < 0 || !m_aPlayerVolumes.IsIndexValid(index))
			return;

		m_aPlayerVolumes.Set(index, SCR_PlayerController.m_aVolumeValues[CRF_SAFESTART_VOICE_TIER]);
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void CRF_SetEveryoneVoiceTier(int tier)
	{
		if (!SCR_PlayerController.m_aVolumeValues.IsIndexValid(tier))
			return;

		int range = SCR_PlayerController.m_aVolumeValues[tier];
		bool changed;
		foreach (int i, int volume : m_aPlayerVolumes)
		{
			if (volume == range)
				continue;

			m_aPlayerVolumes.Set(i, range);
			changed = true;
		}

		if (changed)
			Replication.BumpMe();
	}
}
