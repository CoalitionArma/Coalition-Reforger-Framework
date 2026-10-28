//------------------------------------------------------------------------------------------------
class CRF_ReplayGameModeClass : SCR_BaseGameModeClass
{
}

//------------------------------------------------------------------------------------------------
//! The game mode of the replay worlds (Worlds/Replay/CRF_Replay_<terrain>.ent - a terrain and this, nothing
//! else). Nobody spawns: the player watches through the replay's spectator camera. Starts the replay the
//! launcher asked for, or in Workbench the newest downloaded one recorded on this terrain.
class CRF_ReplayGameMode : SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	protected override void OnGameStart()
	{
		super.OnGameStart();
		CRF_ReplayLauncher.OnReplayWorldStarted();
	}
}

//------------------------------------------------------------------------------------------------
//! Spawns no one - not even Workbench's play-from-camera character
[BaseContainerProps(category: "Respawn")]
class CRF_ReplaySpawnLogic : SCR_SpawnLogic
{
	//------------------------------------------------------------------------------------------------
	protected override void DoInitialSpawn_S(int playerId)
	{
	}

	//------------------------------------------------------------------------------------------------
	protected override void DoSpawn_S(int playerId)
	{
	}
}
