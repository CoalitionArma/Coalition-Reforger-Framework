//------------------------------------------------------------------------------------------------
// Static store for per-player kill/death name lists received from the server at AAR time.
// Populated by CRF_PlayerRplToOwnerManager.RpcDo_SendAARKillStats() on the owning client.
// Cleared/replaced each time a new AAR begins.
//------------------------------------------------------------------------------------------------
class CRF_AARSessionStats
{
	static ref array<string> s_aPlayerKills = {};
	static string s_sKilledBy = "";

	//------------------------------------------------------------------------------------------------
	static void SetData(array<string> kills, string killedBy)
	{
		s_aPlayerKills.Clear();
		if (kills)
			s_aPlayerKills.InsertAll(kills);
		s_sKilledBy = killedBy;
	}
}

//------------------------------------------------------------------------------------------------
// Handles widget population for the CRF_AARStats overlay panel.
// Instantiated by CRF_Outro after CreateLayout() succeeds.
// Reads session stat deltas from SCR_PlayerData (sent server->client via SendData RPC).
// Reads player kill/death name lists from CRF_AARSessionStats (sent via RpcDo_SendAARKillStats).
//
// If SCR_PlayerData is not yet available when TryPopulate() is called (unlikely given the
// 5-second delay, but handled gracefully), the handler subscribes to m_OnDataReceived and
// populates once data arrives.
//------------------------------------------------------------------------------------------------
class CRF_AARStatsHUD
{
	protected TextWidget     m_wKillsValue;
	protected TextWidget     m_wDeathsValue;
	protected TextWidget     m_wShotsValue;
	protected TextWidget     m_wGrenadesValue;
	protected TextWidget     m_wBandagesValue;
	protected TextWidget     m_wDistanceValue;
	protected TextWidget     m_wKilledValue;
	protected TextWidget     m_wKilledByValue;
	protected TextWidget     m_wFriendlyKillsValue;
	protected TextWidget     m_wXPValue;

	protected SCR_DataCollectorCommunicationComponent m_pCommComp;

	//------------------------------------------------------------------------------------------------
	void CRF_AARStatsHUD(notnull Widget root)
	{
		m_wKillsValue    = TextWidget.Cast(root.FindAnyWidget("KillsValue"));
		m_wDeathsValue   = TextWidget.Cast(root.FindAnyWidget("DeathsValue"));
		m_wShotsValue    = TextWidget.Cast(root.FindAnyWidget("ShotsValue"));
		m_wGrenadesValue = TextWidget.Cast(root.FindAnyWidget("GrenadesValue"));
		m_wBandagesValue = TextWidget.Cast(root.FindAnyWidget("BandagesValue"));
		m_wDistanceValue = TextWidget.Cast(root.FindAnyWidget("DistanceValue"));
		m_wKilledValue        = TextWidget.Cast(root.FindAnyWidget("KilledValue"));
		m_wKilledByValue      = TextWidget.Cast(root.FindAnyWidget("KilledByValue"));
		m_wFriendlyKillsValue = TextWidget.Cast(root.FindAnyWidget("FriendlyKillsValue"));
		m_wXPValue            = TextWidget.Cast(root.FindAnyWidget("XPValue"));

		TryPopulate();
	}

	//------------------------------------------------------------------------------------------------
	// Call this when the owning widget is being destroyed to unsubscribe from any pending callbacks.
	void Cleanup()
	{
		if (m_pCommComp)
		{
			m_pCommComp.GetOnDataReceived().Remove(OnDataReceived);
			m_pCommComp = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void TryPopulate()
	{
		SCR_DataCollectorComponent dataCollector = GetGame().GetDataCollector();
		if (!dataCollector)
			return;

		// id=0 = local player on client; createNew=false returns null until Rpc_DoSendData fires
		SCR_PlayerData playerData = dataCollector.GetPlayerData(0, false, false);
		if (!playerData)
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (!pc)
				return;

			m_pCommComp = SCR_DataCollectorCommunicationComponent.Cast(pc.FindComponent(SCR_DataCollectorCommunicationComponent));
			if (m_pCommComp)
				m_pCommComp.GetOnDataReceived().Insert(OnDataReceived);
			return;
		}

		PopulateStats(playerData);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDataReceived(SCR_PlayerData playerData)
	{
		if (m_pCommComp)
		{
			m_pCommComp.GetOnDataReceived().Remove(OnDataReceived);
			m_pCommComp = null;
		}

		PopulateStats(playerData);
	}

	//------------------------------------------------------------------------------------------------
	protected void PopulateStats(notnull SCR_PlayerData playerData)
	{
		// Session deltas: current total minus value at session start
		int nKills    = (int)(playerData.GetStat(SCR_EDataStats.KILLS)   - playerData.GetStat(SCR_EDataStats.KILLS, false));
		int nDeaths   = (int)(playerData.GetStat(SCR_EDataStats.DEATHS)  - playerData.GetStat(SCR_EDataStats.DEATHS, false));
		int nShots    = (int)(playerData.GetStat(SCR_EDataStats.SHOTS)   - playerData.GetStat(SCR_EDataStats.SHOTS, false));
		int nGrenades = (int)(playerData.GetStat(SCR_EDataStats.GRENADES_THROWN) - playerData.GetStat(SCR_EDataStats.GRENADES_THROWN, false));

		float fBandageSelf   = playerData.GetStat(SCR_EDataStats.BANDAGE_SELF)        - playerData.GetStat(SCR_EDataStats.BANDAGE_SELF, false);
		float fBandageFriend = playerData.GetStat(SCR_EDataStats.BANDAGE_FRIENDLIES) - playerData.GetStat(SCR_EDataStats.BANDAGE_FRIENDLIES, false);
		int nBandages = (int)(fBandageSelf + fBandageFriend);

		float fDistWalked = playerData.GetStat(SCR_EDataStats.DISTANCE_WALKED) - playerData.GetStat(SCR_EDataStats.DISTANCE_WALKED, false);
		float fDistDriven = playerData.GetStat(SCR_EDataStats.DISTANCE_DRIVEN) - playerData.GetStat(SCR_EDataStats.DISTANCE_DRIVEN, false);
		int nDistKm = (int)((fDistWalked + fDistDriven) / 1000);

		int nFriendlyKills = (int)(playerData.GetStat(SCR_EDataStats.FRIENDLY_KILLS) - playerData.GetStat(SCR_EDataStats.FRIENDLY_KILLS, false));
		int nXP            = (int)(playerData.GetStat(SCR_EDataStats.LEVEL_EXPERIENCE) - playerData.GetStat(SCR_EDataStats.LEVEL_EXPERIENCE, false));

		if (m_wKillsValue)         m_wKillsValue.SetText(nKills.ToString());
		if (m_wDeathsValue)        m_wDeathsValue.SetText(nDeaths.ToString());
		if (m_wShotsValue)         m_wShotsValue.SetText(nShots.ToString());
		if (m_wGrenadesValue)      m_wGrenadesValue.SetText(nGrenades.ToString());
		if (m_wBandagesValue)      m_wBandagesValue.SetText(nBandages.ToString());
		if (m_wDistanceValue)      m_wDistanceValue.SetText(string.Format("%1 km", nDistKm.ToString()));
		if (m_wFriendlyKillsValue) m_wFriendlyKillsValue.SetText(nFriendlyKills.ToString());
		if (m_wXPValue)            m_wXPValue.SetText(nXP.ToString());

		// Players this player killed (named list from custom server tracking)
		if (m_wKilledValue)
		{
			array<string> killNames = CRF_AARSessionStats.s_aPlayerKills;
			if (killNames && killNames.Count() > 0)
			{
				string listText = "";
				foreach (int i, string name : killNames)
				{
					if (i > 0)
						listText += "\n";
					listText += name;
				}
				m_wKilledValue.SetText(listText);
			}
			else
			{
				m_wKilledValue.SetText("—");
			}
		}

		// Who killed this player last
		if (m_wKilledByValue)
		{
			string killedBy = CRF_AARSessionStats.s_sKilledBy;
			if (killedBy == "")
				m_wKilledByValue.SetText("—");
			else
				m_wKilledByValue.SetText(killedBy);
		}
	}
}
