[ComponentEditorProps(category: "Game Mode Component", description: "BTI faction-flip gamemode. A player changes teams at the slot level after death so the next spawn uses the flipped faction.")]
class CRF_BTIGamemodeManagerClass : SCR_BaseGameModeComponentClass
{
}

class CRF_BTIGamemodeManager : SCR_BaseGameModeComponent
{
	[Attribute("", UIWidgets.Auto, desc: "Any listed faction is switched to the target faction after death.", category: "BTI - Factions")]
	protected ref array<FactionKey> m_aFlipFromFactionKeys;

	[Attribute("BLUFOR", UIWidgets.ComboBox, desc: "Faction players are switched to after death.", enums: {ParamEnum("BLUFOR", "BLUFOR"), ParamEnum("OPFOR", "OPFOR"), ParamEnum("INDFOR", "INDFOR"), ParamEnum("CIV", "CIV")}, category: "BTI - Factions")]
	protected FactionKey m_sFlipToFactionKey;

	protected static CRF_BTIGamemodeManager m_sInstance;

	//------------------------------------------------------------------------------------------------
	void CRF_BTIGamemodeManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_BTIGamemodeManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_BTIGamemodeManager GetInstance()
	{
		return m_sInstance;
	}

	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnPlayerKilled(instigatorContextData);

		if (!Replication.IsServer())
			return;

		int victimPlayerId = instigatorContextData.GetVictimPlayerID();
		if (victimPlayerId <= 0)
			return;

		FlipSlotFaction(victimPlayerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void FlipSlotFaction(int playerId)
	{
		if (!m_aFlipFromFactionKeys || m_aFlipFromFactionKeys.IsEmpty() || m_sFlipToFactionKey.IsEmpty())
			return;

		COA_SlottingManager slottingManager = COA_SlottingManager.GetInstance();
		if (!slottingManager)
			return;

		COA_SlotData slotData = slottingManager.GetPlayerSlotData(playerId);
		if (!slotData)
			return;

		FactionKey currentFactionKey = slotData.GetSlotFactionKey();
		if (currentFactionKey == m_sFlipToFactionKey)
			return;

		bool shouldFlip = false;
		foreach (FactionKey factionKey : m_aFlipFromFactionKeys)
		{
			if (factionKey == currentFactionKey)
			{
				shouldFlip = true;
				break;
			}
		}

		if (!shouldFlip)
			return;

		slotData.SetSlotFactionKey(m_sFlipToFactionKey);

		GetGame().GetCallqueue().CallLater(RespawnFlippedPlayer, 250, false, playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected void RespawnFlippedPlayer(int playerId)
	{
		if (!Replication.IsServer())
			return;

		COA_RespawnManager respawnManager = COA_RespawnManager.GetInstance();
		if (!respawnManager)
		{
			Print("[CRF_BTIGamemodeManager] Cannot respawn flipped player: COA_RespawnManager is unavailable.", LogLevel.ERROR);
			return;
		}

		respawnManager.RespawnPlayer(playerId);
	}
}
