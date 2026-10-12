//------------------------------------------------------------------------------------------------
//! Mission loading screen: builds the CRF layout instead of vanilla's ScenarioLoadingScreen.layout.
//! The CRF layout is its own resource rather than an override of the vanilla file: layout overrides
//! merge into the base like prefab overrides, so a full replacement layout ended up stacked on top of
//! vanilla's widgets (and those of any other mod overriding it, e.g. Coalition Loading Screens).
//! Widget paths match vanilla's, so SCR_ScenarioLoadingScreenWidgets binds to it unchanged.
//! Vanilla keeps its layout component private, so this keeps its own and routes every call to it.
modded class ArmaReforgerLoadingAnim
{
	protected static const ResourceName CRF_LOADING_SCREEN_LAYOUT = "{61628EB200B908A1}UI/layouts/Menus/LoadingScreen/CRF_ScenarioLoadingScreen.layout";

	protected ref SCR_LoadingScreenComponent m_CRF_LayoutComponent;

	//------------------------------------------------------------------------------------------------
	override void Load()
	{
		m_wRoot = m_WorkspaceWidget.CreateWidgets(CRF_LOADING_SCREEN_LAYOUT, m_WorkspaceWidget);
		if (!m_wRoot)
		{
			// Layout missing: fall back to vanilla
			super.Load();
			return;
		}

		m_CRF_LayoutComponent = new SCR_LoadingScreenComponent();
		m_wRoot.AddHandler(m_CRF_LayoutComponent);

		SplashScreen.s_iSplashShown++;
	}

	//------------------------------------------------------------------------------------------------
	override void Update(float timeSlice, float progress, float minDurationRatio)
	{
		if (!m_CRF_LayoutComponent)
		{
			super.Update(timeSlice, progress, minDurationRatio);
			return;
		}

		if (!m_wRoot)
			return;

		m_CRF_LayoutComponent.Update(timeSlice, progress, minDurationRatio);
	}

	//------------------------------------------------------------------------------------------------
	override void Hide()
	{
		if (m_CRF_LayoutComponent)
			m_CRF_LayoutComponent.OnHideInternal();

		super.Hide();
	}

	//------------------------------------------------------------------------------------------------
	override void OnMissionSet(SCR_MissionHeader header)
	{
		super.OnMissionSet(header);

		if (m_CRF_LayoutComponent)
			m_CRF_LayoutComponent.OnMissionDataRetrieved(header);
	}

	//------------------------------------------------------------------------------------------------
	override void SetJoiningCrossPlay(bool isCrossPlay)
	{
		super.SetJoiningCrossPlay(isCrossPlay);

		if (m_CRF_LayoutComponent)
			m_CRF_LayoutComponent.SetJoiningCrossPlay(isCrossPlay);
	}

	//------------------------------------------------------------------------------------------------
	override void SetLoadingModded(bool isModded)
	{
		super.SetLoadingModded(isModded);

		if (m_CRF_LayoutComponent)
			m_CRF_LayoutComponent.SetLoadingModded(isModded);
	}
}
