//=============================================================================
// CRF_TaskHandler_PlantDefuseBomb.c
// Task handler for plant/defuse bomb mechanics.
//
// Bomb lifecycle:
//   UNPLANTED  ->  player holds "Plant Bomb"  ->  OnPerform: SetState(PLANTED)
//   PLANTED    ->  countdown ticking          ->  server runs countdown
//   PLANTED    ->  player holds "Defuse Bomb" ->  OnPerform: SetState(DEFUSED) + TaskComplete
//   PLANTED    ->  countdown expires          ->  OnObjectCountdownExpired: explosion + TaskComplete
//
// Sounds:
//   Planting/defusing sounds:  broadcast 3D to all clients via
//     RequestTaskObjectPlaySound / RequestTaskObjectStopSound RPCs.
//   Tick sound: server starts on state->PLANTED in OnObjectStateChangedServer;
//     stops on state->DEFUSED or EXPLODED.
//   Explosion: particle prefabs spawned server-side on countdown expiry
//     (replicated to clients via the engine entity system).
//
// To use:
//   1. Set this handler on CRF_TaskCreatorComponent.m_PlantDefuseBomb.
//   2. Create a task prefab with CRF_TaskCreatorObjectComponent +
//      CRF_TaskCreatorAction, set m_eTaskType = PLANT_DEFUSE_BOMB.
//   3. Add a PLANT_DEFUSE_BOMB entry to CRF_TaskCreatorComponent.m_aTaskEntries.
// =============================================================================

// Bomb lifecycle states. Stored as int in CRF_TaskCreatorObjectComponent.
// Cast m_iTaskObjectState to this enum to read and write the current phase.
enum CRF_EBombTaskState
{
UNPLANTED = 0,   // Bomb ready to plant (initial state after object spawns)
PLANTED   = 1,   // Bomb planted; countdown ticking towards detonation
DEFUSED   = 2,   // Defender successfully defused — task ends
EXPLODED  = 3    // Countdown expired; bomb detonated
}

// Controls what happens to the bomb object after a successful defuse.
enum CRF_EDefuseBehaviour
{
DELETE    = 0,   // Delete the task object and mark the task complete (default)
REPLANT   = 1    // Reset the bomb to UNPLANTED so it can be planted again
}

[BaseContainerProps()]
class CRF_TaskHandler_PlantDefuseBomb : CRF_BaseTaskHandler
{
//=========================================================================
// ATTRIBUTES
//=========================================================================

// ---- Prefab -------------------------------------------------------------

[Attribute("", UIWidgets.ResourceNamePicker, "Prefab spawned at the task anchor. Must carry CRF_TaskCreatorObjectComponent + CRF_TaskCreatorAction, with Task Type set to PLANT_DEFUSE_BOMB.", params: "et", category: "Plant/Defuse")]
ResourceName m_sPrefab;

// ---- Timing -------------------------------------------------------------

[Attribute("45", UIWidgets.SpinBox, "Seconds from plant until the bomb explodes if not defused.", "5 300 5", category: "Plant/Defuse")]
float m_fExplosionCountdown;

[Attribute("0", UIWidgets.ComboBox, "What happens when the bomb is successfully defused.\nDELETE — remove the object and mark the task complete.\nREPLANT — reset the bomb to UNPLANTED so it can be planted again.", "", ParamEnumArray.FromEnum(CRF_EDefuseBehaviour), category: "Plant/Defuse")]
CRF_EDefuseBehaviour m_eDefuseBehaviour;

	[Attribute("0", UIWidgets.ComboBox, "Which faction defends (can defuse but not plant). NONE/ALL = open to all. Specific faction = only that faction can defuse, others can only plant.", "", ParamEnumArray.FromEnum(CRF_EObjectiveNotifySide), category: "Plant/Defuse")]
	CRF_EObjectiveNotifySide m_eDefendingSide;

[Attribute("{1D6C7E5479081CAF}Sounds/Rush/planting_3D.acp", UIWidgets.ResourceNamePicker, "Sound broadcast to all clients while a player is planting the bomb.", params: "acp", category: "Plant/Defuse - Sounds")]
string m_sPlantSoundResource;

[Attribute("RUSH_PLANTING", UIWidgets.EditBox, "AudioSystem event name for the planting sound. Must match event defined in sound config.", category: "Plant/Defuse - Sounds")]
string m_sPlantSoundEvent;

[Attribute("{1D6C7E5479081CAF}Sounds/Rush/planting_3D.acp", UIWidgets.ResourceNamePicker, "Sound broadcast to all clients while a player is defusing the bomb.", params: "acp", category: "Plant/Defuse - Sounds")]
string m_sDefuseSoundResource;

[Attribute("RUSH_PLANTING", UIWidgets.EditBox, "AudioSystem event name for the defusing sound. Must match event defined in sound config.", category: "Plant/Defuse - Sounds")]
string m_sDefuseSoundEvent;

[Attribute("{A6BBE7DBD7C64EE6}Sounds/Rush/beep_3D.acp", UIWidgets.ResourceNamePicker, "Looping bomb ticking sound started by server when PLANTED, heard by all clients.", params: "acp", category: "Plant/Defuse - Sounds")]
string m_sTickSoundResource;

[Attribute("RUSH_BEEP", UIWidgets.EditBox, "AudioSystem event name for the bomb ticking sound. Must match event defined in sound config.", category: "Plant/Defuse - Sounds")]
string m_sTickSoundEvent;

// ---- Explosion prefabs --------------------------------------------------

[Attribute("{DDDDBEC77B49A995}Prefabs/Systems/Explosions/Wrapper_Bomb_Huge.et", UIWidgets.ResourceNamePicker, "Primary explosion prefab spawned immediately on detonation.", params: "et", category: "Plant/Defuse - Explosion")]
ResourceName m_sExplosionPrefab;

[Attribute("{BCE4E0823FCFBCB7}Prefabs/Weapons/Warheads/Explosions/Explosion_AmmoRack_Large.et", UIWidgets.ResourceNamePicker, "Secondary explosion prefab spawned 385 ms after detonation.", params: "et", category: "Plant/Defuse - Explosion")]
ResourceName m_sExplosionPrefabSecondary;

[Attribute("{4BE47BA2B7E3877E}Prefabs/Systems/Fire/Wrapper_Fire_Large_Damage.et", UIWidgets.ResourceNamePicker, "Fire/smoke prefab spawned 385 ms after detonation.", params: "et", category: "Plant/Defuse - Explosion")]
ResourceName m_sFirePrefab;

//=========================================================================
// RUNTIME (not serialised)
//=========================================================================

// Cached world position of the bomb, set just before any entity deletion
// so deferred effects (e.g. SpawnDelayedExplosionEffects) have a valid pos.
protected vector m_vExplosionPos;

//=========================================================================
// HANDLER INTERFACE
//=========================================================================

// Client -- returns the HUD label based on the current bomb state.
// PLANTED = bomb is live; show "Defuse Bomb". Any other state = show "Plant Bomb".
override string GetActionName(int taskObjectState)
{
	if (taskObjectState == CRF_EBombTaskState.PLANTED)
		return "Defuse Bomb";
	return "Plant Bomb";
}

override ResourceName GetPrefab()
{
return m_sPrefab;
}

// Client -- called when the hold action completes. Branches on bomb state.
override void OnPerform(int taskIndex, int taskObjectState, IEntity user)
{
	CRF_PlayerRplToAuthorityManager rpl = CRF_PlayerRplToAuthorityManager.GetInstance();
	if (!rpl)
		return;

	if (taskObjectState == CRF_EBombTaskState.PLANTED)  // Defuse
	{
		rpl.RequestStopPositionalSound(m_sDefuseSoundEvent);
		rpl.RequestTaskObjectSetState(taskIndex, CRF_EBombTaskState.DEFUSED);
		if (m_eDefuseBehaviour == CRF_EDefuseBehaviour.DELETE)
			rpl.RequestObjectiveTaskComplete(taskIndex, GetUserSide(user));
	}
	else  // Plant
	{
		rpl.RequestStopPositionalSound(m_sPlantSoundEvent);
		rpl.RequestTaskObjectSetState(taskIndex, CRF_EBombTaskState.PLANTED, GetUserSide(user));
	}
}

// Client -- hides the action only when the task is in a terminal state (already over).
// Wrong-side and wrong-state restrictions are handled by CanPerformExtra (gray out).
override bool CanBeShownExtra(int taskIndex, int taskObjectState, IEntity user)
{
	// Example: to fully hide instead of gray for the wrong side, you could do:
	// bool defendingConfigured = (m_eDefendingSide != CRF_EObjectiveNotifySide.NONE && m_eDefendingSide != CRF_EObjectiveNotifySide.ALL);
	// if (defendingConfigured)
	// {
	// 	bool isDefender = (GetUserSide(user) == m_eDefendingSide);
	// 	if (taskObjectState == CRF_EBombTaskState.PLANTED)
	// 		return isDefender;   // Only defenders see "Defuse Bomb"
	// 	return !isDefender;      // Only attackers see "Plant Bomb"
	// }

	return (taskObjectState != CRF_EBombTaskState.DEFUSED && taskObjectState != CRF_EBombTaskState.EXPLODED);
}

// Client -- gates performability based on bomb state and user faction.
// Plant: bomb must be UNPLANTED and user must not be the defending side.
// Defuse: bomb must be PLANTED and user must be the defending side (if configured).
override bool CanPerformExtra(int taskIndex, int taskObjectState, IEntity user)
{
	bool defendingConfigured = (m_eDefendingSide != CRF_EObjectiveNotifySide.NONE && m_eDefendingSide != CRF_EObjectiveNotifySide.ALL);
	int userSide = GetUserSide(user);

	if (taskObjectState == CRF_EBombTaskState.PLANTED)  // Defuse
	{
		if (defendingConfigured)
			return (userSide == m_eDefendingSide);
		return true;  // No defending side configured — anyone can defuse
	}
	else  // Plant
	{
		if (taskObjectState != CRF_EBombTaskState.UNPLANTED)
			return false;
		if (defendingConfigured && userSide == m_eDefendingSide)
			return false;
		return super.CanPerformExtra(taskIndex, taskObjectState, user);
	}
}

// Client -- player begins holding the action. Plays plant or defuse sound based on state.
override void OnActionStart(int taskIndex, int taskObjectState, IEntity taskObject, IEntity user)
{
	CRF_PlayerRplToAuthorityManager rpl = CRF_PlayerRplToAuthorityManager.GetInstance();
	if (!rpl)
		return;

	if (taskObjectState == CRF_EBombTaskState.PLANTED)  // Defuse
		rpl.RequestPlayPositionalSound(m_sDefuseSoundResource, m_sDefuseSoundEvent, taskObject.GetOrigin());
	else  // Plant
		rpl.RequestPlayPositionalSound(m_sPlantSoundResource, m_sPlantSoundEvent, taskObject.GetOrigin());
}

// Client -- player released button before completing. Stops the active sound.
override void OnActionCanceled(int taskIndex, int taskObjectState, IEntity taskObject, IEntity user)
{
	CRF_PlayerRplToAuthorityManager rpl = CRF_PlayerRplToAuthorityManager.GetInstance();
	if (!rpl)
		return;

	if (taskObjectState == CRF_EBombTaskState.PLANTED)  // Defuse
		rpl.RequestStopPositionalSound(m_sDefuseSoundEvent);
	else  // Plant
		rpl.RequestStopPositionalSound(m_sPlantSoundEvent);
}

// Server -- react to state transitions.
// PLANTED:           start explosion countdown + broadcast tick sound.
// DEFUSED/EXPLODED:  cancel countdown + stop tick sound.
override void OnObjectStateChangedServer(int newState, int taskIndex, CRF_TaskCreatorObjectComponent objectComp)
{
CRF_RplBroadcastManager broadcast = CRF_RplBroadcastManager.GetInstance();
IEntity bombEntity = objectComp.GetOwner();

if (newState == CRF_EBombTaskState.PLANTED)
{
objectComp.StartCountdown(m_fExplosionCountdown);
if (broadcast && bombEntity)
broadcast.PlayPositionalSound(m_sTickSoundResource, m_sTickSoundEvent, bombEntity.GetOrigin());
}
else if (newState == CRF_EBombTaskState.DEFUSED || newState == CRF_EBombTaskState.EXPLODED)
{
objectComp.CancelCountdown();
if (broadcast && bombEntity)
broadcast.StopPositionalSound(m_sTickSoundEvent);

// REPLANT: reset the bomb to UNPLANTED so attackers can plant again.
// Only applies on DEFUSED — an explosion always ends the task.
if (newState == CRF_EBombTaskState.DEFUSED && m_eDefuseBehaviour == CRF_EDefuseBehaviour.REPLANT)
objectComp.SetTaskObjectState(CRF_EBombTaskState.UNPLANTED);
}
}

// Server -- countdown expired while PLANTED: detonate.
override void OnObjectCountdownExpired(int taskIndex, CRF_TaskCreatorObjectComponent objectComp)
{
IEntity bombEntity = objectComp.GetOwner();
if (!bombEntity)
return;

// Cache position before any entity deletion (MarkTaskComplete may delete the object).
m_vExplosionPos = bombEntity.GetOrigin();

// Transition to EXPLODED -- also cancels countdown + stops tick via OnObjectStateChangedServer.
objectComp.SetTaskObjectState(CRF_EBombTaskState.EXPLODED);

// Spawn primary explosion immediately.
EntitySpawnParams spawnParams = new EntitySpawnParams();
spawnParams.TransformMode = ETransformMode.WORLD;
spawnParams.Transform[3] = m_vExplosionPos;
if (!m_sExplosionPrefab.IsEmpty())
GetGame().SpawnEntityPrefab(Resource.Load(m_sExplosionPrefab), GetGame().GetWorld(), spawnParams);

// Schedule secondary explosion + fire slightly after the primary.
GetGame().GetCallqueue().CallLater(SpawnDelayedExplosionEffects, 385, false);

// Mark task complete -- attackers succeeded (bomb detonated).
	// Credit the faction that planted the bomb.
	CRF_TaskCreatorComponent taskComp = CRF_TaskCreatorComponent.GetInstance();
	if (taskComp)
	{
		int planterSide = objectComp.GetPlanterSide();
		taskComp.MarkTaskComplete(taskIndex, planterSide);
	}
}

//=========================================================================
// HELPERS
//=========================================================================

// Spawns secondary explosion and fire prefabs at the cached detonation position.
protected void SpawnDelayedExplosionEffects()
{
EntitySpawnParams spawnParams = new EntitySpawnParams();
spawnParams.TransformMode = ETransformMode.WORLD;
spawnParams.Transform[3] = m_vExplosionPos;

if (!m_sExplosionPrefabSecondary.IsEmpty())
GetGame().SpawnEntityPrefab(Resource.Load(m_sExplosionPrefabSecondary), GetGame().GetWorld(), spawnParams);

if (!m_sFirePrefab.IsEmpty())
GetGame().SpawnEntityPrefab(Resource.Load(m_sFirePrefab), GetGame().GetWorld(), spawnParams);
}

}