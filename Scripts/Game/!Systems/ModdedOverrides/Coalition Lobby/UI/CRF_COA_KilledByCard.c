//------------------------------------------------------------------------------------------------
//! The killer's emblem (made on the website, see CRF_Emblem) above the Lobby's killed-by card - added by the
//! CRF, so the Lobby's own card and layout stay as they are
modded class COA_KilledByCard
{
	protected static const ResourceName CRF_EMBLEM_LAYOUT = "{6B3E1A0C5D7F2E95}UI/layouts/Emblems/CRF_KillerEmblem.layout";
	protected static const float CRF_EMBLEM_SIZE = 96;	// KillerEmblem in that layout

	protected static Widget s_wCRFEmblem;
	protected static int s_iCRFEmblemPlayerId;		// whose emblem the card is waiting for

	//------------------------------------------------------------------------------------------------
	override protected static void Show(COA_SpectatorDamageReportEntry entry)
	{
		super.Show(entry);
		if (!s_wRoot)
			return;

		if (!s_wCRFEmblem)
			s_wCRFEmblem = GetGame().GetWorkspace().CreateWidgets(CRF_EMBLEM_LAYOUT, s_wRoot);

		// Hidden until the server says what it is
		if (s_wCRFEmblem)
			s_wCRFEmblem.SetVisible(false);

		s_iCRFEmblemPlayerId = 0;
		if (entry.m_iAttackerPlayerId > 0 && entry.m_iAttackerPlayerId != entry.m_iVictimPlayerId)
		{
			s_iCRFEmblemPlayerId = entry.m_iAttackerPlayerId;
			CRF_EmblemStore.Get().Request(entry.m_iAttackerPlayerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! A player's emblem arrived (CRF_EmblemStore) - shown if it's the killer on the card now
	static void CRF_OnEmblem(int playerId, string data)
	{
		if (!s_wCRFEmblem || playerId != s_iCRFEmblemPlayerId)
			return;

		s_wCRFEmblem.SetVisible(CRF_Emblem.Draw(s_wCRFEmblem, data, CRF_EMBLEM_SIZE));
	}
}
