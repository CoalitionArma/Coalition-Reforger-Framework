//------------------------------------------------------------------------------------------------
// Static store for per-player kill/death name lists and numeric session stats received from
// the server at AAR time.
// Populated by COA_PlayerRplToOwnerManager RPCs on the owning client.
// Cleared/replaced each time a new AAR begins.
//------------------------------------------------------------------------------------------------
class CRF_AARSessionStats
{
	// Kill / death name lists (filled by RpcDo_SendAARKillStats)
	static ref array<string> s_aPlayerKills = {};
	static string s_sKilledBy = "";

	// Numeric session stats (filled by RpcDo_SendAARStats)
	static int s_nKills;
	static int s_nDeaths;
	static int s_nShots;
	static int s_nGrenades;
	static int s_nBandages;
	static int s_nDistKm;
	static int s_nFriendlyKills;
	static int s_nXP;

	// Set to true once RpcDo_SendAARStats fires; used by the HUD to detect late arrival.
	static bool s_bStatsReceived = false;
	static ref ScriptInvoker s_OnStatsReceived = new ScriptInvoker();

	//------------------------------------------------------------------------------------------------
	static void SetData(array<string> kills, string killedBy)
	{
		s_aPlayerKills.Clear();
		if (kills)
			s_aPlayerKills.InsertAll(kills);
		s_sKilledBy = killedBy;
	}

	//------------------------------------------------------------------------------------------------
	static void SetStats(int kills, int deaths, int shots, int grenades, int bandages, int distKm, int friendlyKills, int xp)
	{
		s_nKills        = kills;
		s_nDeaths       = deaths;
		s_nShots        = shots;
		s_nGrenades     = grenades;
		s_nBandages     = bandages;
		s_nDistKm       = distKm;
		s_nFriendlyKills = friendlyKills;
		s_nXP           = xp;
		s_bStatsReceived = true;
		s_OnStatsReceived.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! Reset between missions so stale data never bleeds into the next AAR.
	static void Reset()
	{
		s_aPlayerKills.Clear();
		s_sKilledBy      = "";
		s_nKills         = 0;
		s_nDeaths        = 0;
		s_nShots         = 0;
		s_nGrenades      = 0;
		s_nBandages      = 0;
		s_nDistKm        = 0;
		s_nFriendlyKills = 0;
		s_nXP            = 0;
		s_bStatsReceived = false;
	}
}

//------------------------------------------------------------------------------------------------
// Handles widget population for the CRF_AARStats overlay panel.
// Instantiated by COA_Outro after CreateLayout() succeeds.
// Reads session stats from CRF_AARSessionStats static fields populated by
// COA_PlayerRplToOwnerManager RPC calls (SendAARStats / SendAARKillStats).
//
// If stats have not yet arrived when TryPopulate() is called the handler
// subscribes to s_OnStatsReceived and populates once the data arrives.
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
	void Cleanup()
	{
		CRF_AARSessionStats.s_OnStatsReceived.Remove(OnStatsReceived);
	}

	//------------------------------------------------------------------------------------------------
	protected void TryPopulate()
	{
		if (!CRF_AARSessionStats.s_bStatsReceived)
		{
			// Stats haven't arrived yet — subscribe and wait.
			CRF_AARSessionStats.s_OnStatsReceived.Insert(OnStatsReceived);
			return;
		}

		PopulateStats();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStatsReceived()
	{
		CRF_AARSessionStats.s_OnStatsReceived.Remove(OnStatsReceived);
		PopulateStats();
	}

	//------------------------------------------------------------------------------------------------
	protected void PopulateStats()
	{
		if (m_wKillsValue)         m_wKillsValue.SetText(CRF_AARSessionStats.s_nKills.ToString());
		if (m_wDeathsValue)        m_wDeathsValue.SetText(CRF_AARSessionStats.s_nDeaths.ToString());
		if (m_wShotsValue)         m_wShotsValue.SetText(CRF_AARSessionStats.s_nShots.ToString());
		if (m_wGrenadesValue)      m_wGrenadesValue.SetText(CRF_AARSessionStats.s_nGrenades.ToString());
		if (m_wBandagesValue)      m_wBandagesValue.SetText(CRF_AARSessionStats.s_nBandages.ToString());
		if (m_wDistanceValue)      m_wDistanceValue.SetText(string.Format("%1 km", CRF_AARSessionStats.s_nDistKm.ToString()));
		if (m_wFriendlyKillsValue) m_wFriendlyKillsValue.SetText(CRF_AARSessionStats.s_nFriendlyKills.ToString());
		if (m_wXPValue)            m_wXPValue.SetText(CRF_AARSessionStats.s_nXP.ToString());

		// Players this player killed (named list)
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

