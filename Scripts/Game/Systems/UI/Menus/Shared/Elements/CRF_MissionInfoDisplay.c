/**
 * CRF_MissionInfoDisplay
 *
 * Modular widget component that displays the mission name, author, and current
 * weather state. Intended to be placed on a root widget that contains child
 * widgets named "MissionText" and "WeatherText". Used by both the Slotting
 * Menu and the After Action Report Menu.
 *
 * Usage:
 *   Widget missionInfoRoot = m_wRoot.FindAnyWidget("MissionInfo");
 *   m_MissionInfoDisplay = CRF_MissionInfoDisplay.Cast(missionInfoRoot.FindHandler(CRF_MissionInfoDisplay));
 *   m_MissionInfoDisplay.Populate();
 */
class CRF_MissionInfoDisplay : SCR_ScriptedWidgetComponent
{
	protected TextWidget m_wMissionText;
	protected TextWidget m_wWeatherText;

	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);

		m_wMissionText = TextWidget.Cast(w.FindAnyWidget("MissionText"));
		m_wWeatherText = TextWidget.Cast(w.FindAnyWidget("WeatherText"));
	}

	/**
	 * Populates mission name + author and current weather state.
	 * Call once after the menu opens — these values are static for the
	 * lifetime of the mission.
	 */
	void Populate()
	{
		if (!m_wMissionText || !m_wWeatherText)
			return;

		// --- Mission name & author ---
		string missionName = GetGame().GetMissionName();
	if (missionName.IsEmpty())
		missionName = "Unknown Mission";

	SCR_MissionHeader header = SCR_MissionHeader.Cast(GetGame().GetMissionHeader());
	string author = "Unknown";
	if (header)
		author = header.m_sAuthor;

	m_wMissionText.SetText(missionName + " | By " + author);		// --- Weather ---
		string weatherName = ChimeraWorld.CastFrom(GetGame().GetWorld())
			.GetTimeAndWeatherManager()
			.GetCurrentWeatherState()
			.GetStateName();

		m_wWeatherText.SetText("Weather: " + weatherName);
	}
}
