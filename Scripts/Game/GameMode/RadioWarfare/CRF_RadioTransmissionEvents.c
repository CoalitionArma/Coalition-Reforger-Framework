//------------------------------------------------------------------------------------------------
// CRF_RadioTransmissionEvents: server-side radio traffic events for game modes built on CVON
// (Radio Direction Finding, Patrol & Ambush). CVON sends every radio transmission through the server
// when push-to-talk starts (AddLocalVONBroadcasts) and when it ends (RemoveLocalVONBroadcasts); this
// hooks both and raises:
//   OnTransmissionStarted(int playerId, string frequency, RplId radioId)
//   OnTransmissionEnded(int playerId)
// Direct (proximity) speech never reaches the server, so only radio counts.
//------------------------------------------------------------------------------------------------
class CRF_RadioTransmissionEvents
{
	protected static ref ScriptInvoker s_OnTransmissionStarted = new ScriptInvoker();
	protected static ref ScriptInvoker s_OnTransmissionEnded = new ScriptInvoker();

	static ScriptInvoker GetOnTransmissionStarted()
	{
		return s_OnTransmissionStarted;
	}

	static ScriptInvoker GetOnTransmissionEnded()
	{
		return s_OnTransmissionEnded;
	}

	//------------------------------------------------------------------------------------------------
	//! Shared helpers for the radio game modes
	//------------------------------------------------------------------------------------------------

	//! The faction a player is slotted on (the Lobby's slot, so spectators and the respawn screen don't
	//! count as their side), falling back to the faction manager
	static string GetPlayerFactionKey(int playerId)
	{
		COA_SlottingManager slotMgr = COA_SlottingManager.GetInstance();
		if (slotMgr)
		{
			COA_SlotData slotData = slotMgr.GetPlayerSlotData(playerId);
			if (slotData)
				return slotData.GetSlotFactionKey();
		}

		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMan)
			return "";

		Faction faction = factionMan.GetPlayerFaction(playerId);
		if (!faction)
			return "";
		return faction.GetFactionKey();
	}

	//! The player's character if they're alive, otherwise null
	static IEntity GetAlivePlayerEntity(int playerId)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return null;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!controller || controller.IsDead())
			return null;
		return entity;
	}

	//! Popup for every player slotted on a faction ("" = everyone)
	static void PopupToFaction(string factionKey, string text, string text2 = "", float duration = 6)
	{
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (!factionKey.IsEmpty() && GetPlayerFactionKey(playerId) != factionKey)
				continue;
			PopupToPlayer(playerId, text, text2, duration);
		}
	}

	static void PopupToPlayer(int playerId, string text, string text2 = "", float duration = 6)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (pc)
			pc.CRF_ShowRadioWarfarePopup(text, text2, duration);
	}

	//! 6-figure grid (100 m squares), e.g. "045 062"
	static string GetGrid100(vector position)
	{
		return Pad((int)(position[0] / 100), 3) + " " + Pad((int)(position[2] / 100), 3);
	}

	//! 4-figure grid (1 km squares), e.g. "04 06"
	static string GetGrid1000(vector position)
	{
		return Pad((int)(position[0] / 1000), 2) + " " + Pad((int)(position[2] / 1000), 2);
	}

	protected static string Pad(int value, int digits)
	{
		if (value < 0)
			value = 0;

		string text = value.ToString();
		while (text.Length() < digits)
			text = "0" + text;
		return text;
	}
}

//------------------------------------------------------------------------------------------------
modded class CVON_VONGameModeComponent
{
	override void AddLocalVONBroadcasts(CVON_VONContainer VONContainer, int playerIdToAdd, RplId radioId)
	{
		super.AddLocalVONBroadcasts(VONContainer, playerIdToAdd, radioId);

		if (VONContainer)
			CRF_RadioTransmissionEvents.GetOnTransmissionStarted().Invoke(playerIdToAdd, VONContainer.m_sFrequency, radioId);
	}

	override void RemoveLocalVONBroadcasts(int playerIdToRemove)
	{
		super.RemoveLocalVONBroadcasts(playerIdToRemove);
		CRF_RadioTransmissionEvents.GetOnTransmissionEnded().Invoke(playerIdToRemove);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	//! Server -> this player: an on-screen popup from a radio game mode
	void CRF_ShowRadioWarfarePopup(string text, string text2, float duration)
	{
		Rpc(RpcDo_CRF_ShowRadioWarfarePopup, text, text2, duration);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CRF_ShowRadioWarfarePopup(string text, string text2, float duration)
	{
		SCR_PopUpNotification notification = SCR_PopUpNotification.GetInstance();
		if (notification)
			notification.PopupMsg(text, duration, text2);
	}
}
