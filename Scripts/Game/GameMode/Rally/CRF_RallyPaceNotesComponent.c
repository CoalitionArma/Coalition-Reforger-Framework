//------------------------------------------------------------------------------------
// CRF_RallyPaceNotesComponent: pace notes for a rally stage
//
// The mission lists the calls in course order, each with the point on the road where it is given
// (some way before the corner it describes). On every client the local player's car is checked
// against the list and each call pops up as the car reaches it - for the driver, the co-driver or
// the whole crew (m_eAudience). Works alongside CRF_RallyGamemodeComponent or
// CRF_RallyStageGamemodeComponent and needs neither.
//
// Calls only go forward: reaching a later call skips the ones in between (a cut or a missed point),
// and the list starts again when the car is back at the first call.
//
// Grades in the calls: 1 (tightest) to 6 (fastest), plus square (a 90 degree junction) and hairpin.
// Distances are to the next call in metres; "into" and "and" mean the next one follows straight on.
//
// Setup: add this component to the gamemode entity and fill in the calls. Purely client side: no
// replication, and nothing runs on a dedicated server.
//------------------------------------------------------------------------------------

enum CRF_ERallyPaceNoteAudience
{
	DRIVER,
	CODRIVER,
	CREW
}

//------------------------------------------------------------------------------------------------
//! One call: what is said and where
[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sCall")]
class CRF_RallyPaceNote
{
	[Attribute(desc: "World position the call is given at (on the road, before the corner it describes)")]
	vector m_vPosition;

	[Attribute(desc: "The call, e.g. \"Left 3 tightens, 100\"")]
	string m_sCall;
}


//------------------------------------------------------------------------------------------------
[ComponentEditorProps(category: "Game Mode Component", description: "Rally pace notes: calls shown to the crew as their car reaches each one.")]
class CRF_RallyPaceNotesComponentClass : SCR_BaseGameModeComponentClass {}


class CRF_RallyPaceNotesComponent : SCR_BaseGameModeComponent
{
	[Attribute(desc: "The calls, in course order", category: "Rally - Pace Notes")]
	protected ref array<ref CRF_RallyPaceNote> m_aCalls;

	[Attribute("0", UIWidgets.ComboBox, "Who sees the calls: the driver, the co-driver (any other seat, to read them out) or the whole crew", enums: ParamEnumArray.FromEnum(CRF_ERallyPaceNoteAudience), category: "Rally - Pace Notes")]
	protected CRF_ERallyPaceNoteAudience m_eAudience;

	[Attribute("25", UIWidgets.EditBox, "A call is given when the car comes within this many metres of its point", params: "5 100 1", category: "Rally - Pace Notes")]
	protected float m_fTriggerRadius;

	[Attribute("5", UIWidgets.EditBox, "Seconds a call stays on screen (a new call replaces it straight away)", params: "1 20 0.5", category: "Rally - Pace Notes")]
	protected float m_fDisplaySeconds;

	[Attribute("{E23715DAF7FE2E8A}Sounds/Items/Equipment/Radios/Samples/Items_Radio_Turn_On.wav", UIWidgets.ResourceNamePicker, "Played with each call. Empty = silent.", params: "wav", category: "Rally - Pace Notes")]
	protected ResourceName m_sCallSound;

	protected int m_iNextCall;
	protected float m_fLastCallShownMs = -1e9;

	// Calls further ahead than this are never jumped to, so a road that passes near itself can't skip half the stage
	protected static const int LOOKAHEAD = 4;
	protected static const int CALL_PRIORITY = 100;
	protected static const int UPDATE_MS = 200;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode() || System.IsConsoleApp() || !m_aCalls || m_aCalls.IsEmpty())
			return;

		GetGame().GetCallqueue().CallLater(Update, UPDATE_MS, true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(Update);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected void Update()
	{
		IEntity vehicle;
		bool isDriver;
		if (!GetLocalSeat(vehicle, isDriver))
			return;

		if ((m_eAudience == CRF_ERallyPaceNoteAudience.DRIVER && !isDriver) || (m_eAudience == CRF_ERallyPaceNoteAudience.CODRIVER && isDriver))
			return;

		vector position = vehicle.GetOrigin();
		int count = m_aCalls.Count();

		// Back at the start after calls have been given: a fresh run
		if (m_iNextCall > 1 && vector.DistanceXZ(position, m_aCalls[0].m_vPosition) <= m_fTriggerRadius)
			m_iNextCall = 0;

		int last = Math.Min(count, m_iNextCall + LOOKAHEAD);
		for (int i = m_iNextCall; i < last; i++)
		{
			if (vector.DistanceXZ(position, m_aCalls[i].m_vPosition) > m_fTriggerRadius)
				continue;

			m_iNextCall = i + 1;
			ShowCall(i);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The vehicle the local player is sitting in, and whether they are driving it
	protected bool GetLocalSeat(out IEntity vehicle, out bool isDriver)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!character || !character.IsInVehicle())
			return false;

		CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
		if (!access)
			return false;

		BaseCompartmentSlot compartment = access.GetCompartment();
		if (!compartment)
			return false;

		vehicle = compartment.GetVehicle();
		isDriver = PilotCompartmentSlot.Cast(compartment) != null;
		return vehicle != null;
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowCall(int index)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
			return;

		// Replace the previous call instead of queueing behind it
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fLastCallShownMs < m_fDisplaySeconds * 1000)
			popup.HideCurrentMsg();

		popup.PopupMsg(m_aCalls[index].m_sCall, m_fDisplaySeconds, string.Format("Pace note %1 of %2", index + 1, m_aCalls.Count()), CALL_PRIORITY);
		m_fLastCallShownMs = now;

		if (!m_sCallSound.IsEmpty())
			AudioSystem.PlaySound(m_sCallSound);
	}
}
