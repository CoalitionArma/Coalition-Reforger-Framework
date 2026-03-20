modded class SCR_MapMenuUI
{
	//----------------------------------------
	// UI Components
	//----------------------------------------
	protected CRF_LooterGamemodeComponent m_LooterGamemodeComponent;                                   // Game mode instance
	protected bool m_bPlayerIconsInitialized = false;             // Flag to track if descriptions have been initialized
	ref array<ref CRF_PlayerIcon> m_aPlayerIcons = {};
	bool m_bDrawPlayerIcon = false;
	
	static ResourceName PLAYER_ICON = "{A9CABBA67E57C10C}UI/layouts/HUD/PlayerMapIcon.layout";

	//----------------------------------------
	// Menu Lifecycle Methods
	//----------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Called when the map menu is opened
	//! Initializes mission description functionality only on first open
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

		// Don't initialize on dedicated servers
		if (RplSession.Mode() == RplMode.Dedicated) {
			return;
		}

		// Only initialize once on first map open
		if (m_bPlayerIconsInitialized) {
			return;
		}

		// Initialize gamemode reference
		m_LooterGamemodeComponent = CRF_LooterGamemodeComponent.GetInstance();
		if (!m_LooterGamemodeComponent) {
			return;
		}
		
		SCR_FactionManager factionMan = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionMan)
			return;
		
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		if (playerId <= 0)
			return;

		Faction playerFaction = factionMan.GetPlayerFaction(playerId);
		if (!playerFaction)
			return;
		
		if (m_LooterGamemodeComponent.m_bEnableIndividualBFT)
			m_bDrawPlayerIcon = true;
		
		if (!m_bDrawPlayerIcon)
			return;
		
		SCR_GroupsManagerComponent groupsManagerComp = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManagerComp)
			return;
		
		SCR_AIGroup playerGroup = groupsManagerComp.GetPlayerGroup(playerId);
		array<int> playerIds = playerGroup.GetPlayerIDs();
		
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		foreach (int otherPlayerId: playerIds)
		{	
			CRF_PlayerIcon icon = new CRF_PlayerIcon;
			IEntity playerEntity = pm.GetPlayerControlledEntity(otherPlayerId);
			if (!playerEntity)
				continue;
			
			icon.m_Player = playerEntity;
			icon.m_PlayerIcon = GetGame().GetWorkspace().CreateWidgets("{A9CABBA67E57C10C}UI/layouts/HUD/PlayerMapIcon.layout", GetRootWidget());
			icon.m_PlayerIcon.SetZOrder(99);
			m_aPlayerIcons.Insert(icon);
		}
		
		CRF_PlayerIcon icon = new CRF_PlayerIcon;
		IEntity playerEntity = pm.GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			return;
		
		icon.m_Player = playerEntity;
		icon.m_PlayerIcon = GetGame().GetWorkspace().CreateWidgets("{A9CABBA67E57C10C}UI/layouts/HUD/PlayerMapIcon.layout", GetRootWidget());
		icon.m_PlayerIcon.SetZOrder(99);
		m_aPlayerIcons.Insert(icon);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		if (!m_bDrawPlayerIcon)
			return;
		
        UpdatePlayerIcons();
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdatePlayerIcons()
	{
		foreach (CRF_PlayerIcon icon: m_aPlayerIcons)
		{
			// Get player world position
	        vector playerPos = icon.m_Player.GetOrigin();
	        
	      	float x, y;
			m_MapEntity.WorldToScreen(playerPos[0], playerPos[2], x, y, true);
			FrameSlot.SetPos(icon.m_PlayerIcon, GetGame().GetWorkspace().DPIUnscale(x), GetGame().GetWorkspace().DPIUnscale(y));
			FrameSlot.SetAlignment(icon.m_PlayerIcon, 0.5, 0.2);
	        
	        // Get player rotation (yaw angle)
	        vector playerAngles = icon.m_Player.GetAngles();
	        float yaw = playerAngles[1];
	        
	        ImageWidget.Cast(icon.m_PlayerIcon).SetRotation(yaw + 180);
		}
	}
}

class CRF_PlayerIcon
{
	Widget m_PlayerIcon;
	IEntity m_Player;
}