//------------------------------------------------------------------------------------------------
//! The spectated player's emblem (made on the website, see CRF_Emblem) beside the Lobby's follow-mode
//! card, while spectating someone directly - added by the CRF, so the Lobby's card and layout stay as
//! they are. Same emblem cache as the killed-by card (CRF_EmblemStore).
modded class COA_EntityInfoDisplay
{
	protected static const ResourceName CRF_SPECTATOR_EMBLEM_LAYOUT = "{2224FAB10746C1E0}UI/layouts/Emblems/CRF_SpectatorEmblem.layout";
	protected static const float CRF_SPECTATOR_EMBLEM_SIZE = 80;	// SpectatorEmblem in that layout

	protected Widget m_wCRFEmblem;
	// What is drawn now: the spectator menu updates the card constantly, and drawing rebuilds every layer
	protected int m_iCRFEmblemPlayerId;
	protected string m_sCRFEmblemData;

	//------------------------------------------------------------------------------------------------
	override void UpdateEntityInfoDisplay(IEntity specEntity)
	{
		super.UpdateEntityInfoDisplay(specEntity);
		CRF_UpdateEmblem(specEntity);
	}

	//------------------------------------------------------------------------------------------------
	protected void CRF_UpdateEmblem(IEntity specEntity)
	{
		int playerId;
		if (specEntity)
			playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(specEntity);

		// Not following a player, or they have no emblem (yet: the store fills in as players join)
		string data;
		if (playerId <= 0 || !CRF_EmblemStore.Get().Find(playerId, data) || data.IsEmpty())
		{
			if (m_wCRFEmblem)
				m_wCRFEmblem.SetVisible(false);

			m_iCRFEmblemPlayerId = 0;
			m_sCRFEmblemData = string.Empty;
			return;
		}

		if (playerId == m_iCRFEmblemPlayerId && data == m_sCRFEmblemData)
			return;

		if (!m_wCRFEmblem)
		{
			// Beside the card itself, not the scale root (a ScaleWidget takes a single child)
			Widget card;
			if (m_wEntityInfoDisplay)
				card = m_wEntityInfoDisplay.FindAnyWidget("EntityFrame");

			if (!card)
				return;

			m_wCRFEmblem = GetGame().GetWorkspace().CreateWidgets(CRF_SPECTATOR_EMBLEM_LAYOUT, card);
			if (!m_wCRFEmblem)
				return;
		}

		m_iCRFEmblemPlayerId = playerId;
		m_sCRFEmblemData = data;
		m_wCRFEmblem.SetVisible(CRF_Emblem.Draw(m_wCRFEmblem, data, CRF_SPECTATOR_EMBLEM_SIZE));
	}
}
