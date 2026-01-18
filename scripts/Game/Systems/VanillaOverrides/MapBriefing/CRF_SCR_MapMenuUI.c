/**
 * Custom Map Menu UI class for the Coalition Reforger Framework
 * Extends the default map menu to provide mission description functionality
 * This class initializes the mission description list when clients open their map
 */
modded class SCR_MapMenuUI
{
	//----------------------------------------
	// UI Components
	//----------------------------------------
	protected SCR_ListBoxComponent m_cMissionDescriptionListBoxComponent; // Component for mission descriptions
	protected ButtonWidget m_wBackButton;                                 // Back button for description navigation
	protected CRF_Gamemode m_Gamemode;                                   // Game mode instance
	protected ref array<ref CRF_MissionDescriptor> m_aActiveDescriptors = {}; // Active mission descriptors
	protected bool m_bMissionDescriptionsInitialized = false;             // Flag to track if descriptions have been initialized

	//----------------------------------------
	// Menu Lifecycle Methods
	//----------------------------------------

	/**
	 * Called when the map menu is opened
	 * Initializes mission description functionality only on first open
	 */
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Don't initialize on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			return;
		}

		// Only initialize once on first map open
		if (m_bMissionDescriptionsInitialized) {
			return;
		}

		// Initialize gamemode reference
		m_Gamemode = CRF_Gamemode.GetInstance();
		if (!m_Gamemode) {
			return;
		}

		// Initialize mission description components
		InitializeMissionDescriptions();
		
		// Mark as initialized to prevent re-initialization
		m_bMissionDescriptionsInitialized = true;
	}

	/**
	 * Called when the map menu is closed
	 * Cleanup mission description components
	 */
	override void OnMenuClose()
	{
		super.OnMenuClose();

		// Clear mission description data
		if (m_cMissionDescriptionListBoxComponent) {
			m_cMissionDescriptionListBoxComponent.Clear();
			m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();
		}
		
		if (m_aActiveDescriptors) {
			m_aActiveDescriptors.Clear();
		}
	}

	//----------------------------------------
	// Mission Description Methods
	//----------------------------------------

	/**
	 * Initialize mission description components and populate the list
	 */
	protected void InitializeMissionDescriptions()
	{
		// Find the MissionDescription widget in the map menu layout
		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		// Find the DescriptionList within the MissionDescription widget
		OverlayWidget descriptionListWidget = OverlayWidget.Cast(missionDescriptionWidget.FindAnyWidget("DescriptionList"));
		if (!descriptionListWidget) {
			return;
		}

		// Get the list box component
		m_cMissionDescriptionListBoxComponent = SCR_ListBoxComponent.Cast(descriptionListWidget.FindHandler(SCR_ListBoxComponent));
		if (!m_cMissionDescriptionListBoxComponent) {
			return;
		}

		// Find the back button
		m_wBackButton = ButtonWidget.Cast(missionDescriptionWidget.FindAnyWidget("BackButton"));
		if (m_wBackButton) {
			m_wBackButton.SetOpacity(0);
		}

		// Initialize the mission description list
		DescriptionInit();
	}

	/**
	 * Initialize the mission description section
	 * Populates the list with mission descriptors relevant to the player's faction
	 */
	void DescriptionInit()
	{
		if (!m_cMissionDescriptionListBoxComponent || !m_Gamemode) {
			return;
		}

		// Find scroll layout and disable it initially
		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(missionDescriptionWidget.FindAnyWidget("ScrollLayout"));
		if (scrollLayout) {
			scrollLayout.SetEnabled(false);
		}

		// Reset back button
		if (m_wBackButton) {
			m_wBackButton.SetOpacity(0);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton) {
				backButton.m_OnClicked.Clear();
			}
		}

		// Clear description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(missionDescriptionWidget.FindAnyWidget("DescriptionInfo"));
		if (missionDescriptionText) {
			missionDescriptionText.SetText("");
		}

		// Clear list components
		m_cMissionDescriptionListBoxComponent.Clear();
		m_aActiveDescriptors.Clear();

		// Get player faction
		SCR_PlayerFactionAffiliationComponent factionComponent = SCR_PlayerFactionAffiliationComponent.Cast(
			GetGame().GetPlayerController().FindComponent(SCR_PlayerFactionAffiliationComponent)
		);

		if (!factionComponent) {
			return;
		}

		string playerFaction = factionComponent.GetAffiliatedFactionKey();

		// Add relevant descriptions to list
		foreach (ref CRF_MissionDescriptor description : m_Gamemode.m_aMissionDescriptors)
		{
			// Add description visible to all factions
			if (description.m_bShowForAnyFaction)
			{
				m_cMissionDescriptionListBoxComponent.AddItem(
					description.m_sTitle, 
					null, 
					"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
				);
				m_aActiveDescriptors.Insert(description);
				continue;
			}
			
			// Add description specific to player's faction
			foreach (string factionKey : description.m_aFactionKeys)
			{
				if (playerFaction == factionKey)
				{
					m_cMissionDescriptionListBoxComponent.AddItem(
						description.m_sTitle, 
						null, 
						"{A564FC959554A1B9}UI/Listbox/DescriptionListboxElementNoIcon.layout"
					);
					m_aActiveDescriptors.Insert(description);
					break;
				}
			}
		}

		// Register selection handler
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Insert(DescriptionSelected);
	}

	/**
	 * Handles selection of a description item
	 * Shows the selected description text and enables navigation
	 */
	void DescriptionSelected()
	{
		if (!m_cMissionDescriptionListBoxComponent || !m_aActiveDescriptors) {
			return;
		}

		Widget missionDescriptionWidget = GetRootWidget().FindAnyWidget("MissionDescription");
		if (!missionDescriptionWidget) {
			return;
		}

		ScrollLayoutWidget scrollLayout = ScrollLayoutWidget.Cast(missionDescriptionWidget.FindAnyWidget("ScrollLayout"));
		if (scrollLayout) {
			scrollLayout.SetEnabled(true);
		}

		// Get selected description
		int index = m_cMissionDescriptionListBoxComponent.GetSelectedItem();
		if (index < 0 || index >= m_aActiveDescriptors.Count()) {
			return;
		}
			
		string description = m_aActiveDescriptors.Get(index).m_sTextData;

		// Show back button
		if (m_wBackButton) {
			m_wBackButton.SetOpacity(1);
			SCR_ButtonTextComponent backButton = SCR_ButtonTextComponent.Cast(m_wBackButton.FindHandler(SCR_ButtonTextComponent));
			if (backButton) {
				backButton.m_OnClicked.Insert(DescriptionInit);
			}
		}

		// Clear description list
		m_cMissionDescriptionListBoxComponent.Clear();
		m_cMissionDescriptionListBoxComponent.m_OnChanged.Clear();

		// Set description text
		RichTextWidget missionDescriptionText = RichTextWidget.Cast(missionDescriptionWidget.FindAnyWidget("DescriptionInfo"));
		if (missionDescriptionText) {
			missionDescriptionText.SetText(description);
		}
	}
}
