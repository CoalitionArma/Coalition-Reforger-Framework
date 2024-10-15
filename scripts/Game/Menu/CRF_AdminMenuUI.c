modded enum ChimeraMenuPreset 
{ 
  CoalAdminMenu
}

class CRF_AdminMenu: ChimeraMenuBase 
{
	protected InputManager m_InputManager;
	protected bool m_bFocused = true;
	protected Widget m_wRoot;
	protected FrameWidget m_adminMenuRoot;
	protected FrameWidget m_gearResetMenuRoot;
	protected PlayerManager m_playerManager;
	protected SCR_GroupsManagerComponent m_groupManagerComponent;
	protected CRF_AdminMenuGameComponent m_adminMenuComponent;
	protected OverlayWidget m_list1Root;
	protected OverlayWidget m_list2Root;
	protected OverlayWidget m_list3Root;
	protected OverlayWidget m_list4Root;
	protected OverlayWidget m_list1Overlay;
	protected OverlayWidget m_list2Overlay;
	protected OverlayWidget m_list3Overlay;
	protected OverlayWidget m_list4Overlay;
	protected SCR_ListBoxComponent m_list2;
	protected SCR_ListBoxComponent m_list1;
	protected SCR_ListBoxComponent m_list3;
	protected SCR_ListBoxComponent m_list4;
	protected SCR_ButtonTextComponent m_respawnMenuButton;
	protected SCR_ButtonTextComponent m_resetGearMenuButton;
	protected SCR_ButtonTextComponent m_ActionButton
	protected TextWidget m_respawnMenuText;
	protected TextWidget m_resetGearMenuText;
	protected ref array<int> m_groupIDList = {};
	protected ref array<int> m_playerIDList = {};
	protected ref array<SCR_AIGroup> m_outGroups = {};
	protected ref array<int> m_allPlayers = {};
	protected ref array<vector> m_spawnPoints = {};
	
	//-------------------------------------------------------------------------------------------
	//-------------------------------General UI Members------------------------------------------
	//-------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		m_InputManager = GetGame().GetInputManager();
		m_playerManager = GetGame().GetPlayerManager();
		m_groupManagerComponent = SCR_GroupsManagerComponent.GetInstance();
		m_adminMenuComponent = CRF_AdminMenuGameComponent.GetInstance();
		
		//Menu Roots
		m_wRoot = GetRootWidget();
		m_adminMenuRoot = FrameWidget.Cast(m_wRoot.FindWidget("AdminMenuTools"));	
		
		//Populate the List Boxes and Buttons
		m_list1Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List1Box"));
		m_list1 = SCR_ListBoxComponent.Cast(m_list1Root.FindHandler(SCR_ListBoxComponent));
		m_list2Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List2Box"));
		m_list2 = SCR_ListBoxComponent.Cast(m_list2Root.FindHandler(SCR_ListBoxComponent));
		m_list3Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List3Box"));
		m_list3 = SCR_ListBoxComponent.Cast(m_list3Root.FindHandler(SCR_ListBoxComponent));
		m_list4Root = OverlayWidget.Cast(m_wRoot.FindAnyWidget("List4Box"));
		m_list4 = SCR_ListBoxComponent.Cast(m_list4Root.FindHandler(SCR_ListBoxComponent));
		m_ActionButton = SCR_ButtonTextComponent.GetButtonText("ActionButton", m_adminMenuRoot);
		
		//Initializes the Respawn Menu
		InitializeRespawnMenu();
	
		m_respawnMenuButton = SCR_ButtonTextComponent.GetButtonText("RespawnButton", m_wRoot);
		m_respawnMenuButton.m_OnClicked.Insert(RespawnButton);
		m_respawnMenuText = TextWidget.Cast(m_respawnMenuButton.GetRootWidget().FindWidget("RespawnText"));
		
		m_resetGearMenuButton = SCR_ButtonTextComponent.GetButtonText("ResetGearButton", m_wRoot);
		m_resetGearMenuButton.m_OnClicked.Insert(ResetGearButton);
		m_resetGearMenuText = TextWidget.Cast(m_resetGearMenuButton.GetRootWidget().FindWidget("ResetGearText"));
	}
	
	override void OnMenuClose()
	{
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_FE_HUD_PAUSE_MENU_CLOSE);
	}
	
	void RespawnButton()
	{
		m_respawnMenuText.SetColor(Color.FromInt(0xffffffff));
		m_resetGearMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeRespawnMenu();
	}
	
	void ResetGearButton()
	{
		m_resetGearMenuText.SetColor(Color.FromInt(0xffffffff));
		m_respawnMenuText.SetColor(Color.FromRGBA(115, 115, 115, 255));
		ClearMenu();
		InitializeGearMenu();
	}
	
	void ClearMenu()
	{
		m_list1Root.SetVisible(false);
		m_list2Root.SetVisible(false);
		m_list3Root.SetVisible(false);
		m_list4Root.SetVisible(false);
		m_ActionButton.SetVisible(false);
		m_list1.Clear();
		m_list2.Clear();
		m_list3.Clear();
		m_list4.Clear();
		
		m_allPlayers.Clear();
		m_outGroups.Clear();
		m_spawnPoints.Clear();
		
		m_ActionButton.m_OnClicked.Clear();
		m_list1.m_OnChanged.Clear();
		TextWidget.Cast(m_ActionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List4Text")).SetText("");
	}
	
	void AddRoles(SCR_ListBoxComponent list)
	{
		list.AddItem("COY");
		list.AddItem("MO");
		list.AddItem("FO");
		list.AddItem("JTAC");
		list.AddItem("PL");
		list.AddItem("Medic");
		list.AddItem("SL");
		list.AddItem("TL");
		list.AddItem("AR");
		list.AddItem("AAR");
		list.AddItem("AT");
		list.AddItem("AAT");
		list.AddItem("Grenadier");
		list.AddItem("Demo");
		list.AddItem("Rifleman");
		list.AddItem("MMG");
		list.AddItem("AMMG");
		list.AddItem("HMG");
		list.AddItem("AHMG");
		list.AddItem("Sniper");
		list.AddItem("Spotter");
		list.AddItem("MAT");
		list.AddItem("AHAT");
		list.AddItem("HAT");
		list.AddItem("AHAT");
		list.AddItem("Anti Air");
		list.AddItem("AAA");
		list.AddItem("Veh. Leader");
		list.AddItem("Veh. Driver");
		list.AddItem("Veh. Gunner");
		list.AddItem("Veh. Loader");
		list.AddItem("IDF Leader");	
		list.AddItem("IDF Gunner");
		list.AddItem("IDF Loader");
		list.AddItem("Pilot");
		list.AddItem("Crew Chief");
		list.AddItem("Logi Leader");
		list.AddItem("Logi Runner");
	}
	
	string GetPrefab(int groupID, int index)
	{
		string factionKey = m_groupManagerComponent.FindGroup(groupID).GetFaction().GetFactionKey();
		string prefab;
		if(factionKey == "BLUFOR")
		{
			switch(index)
			{
				case 0:    {prefab = "{B3182F73DBEACB7B}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_COY_P.et"; 			break;}
				case 1:    {prefab = "{D206D6B07F9B34F7}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_MO_P.et"; 				break;}
				case 2:    {prefab = "{2A15A7803EAF5EE9}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_FO_P.et"; 				break;}
				case 3:    {prefab = "{E3D72085967E1840}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_JTAC_P.et"; 			break;}
				case 4:    {prefab = "{C62B8D48AAA03249}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_PL_P.et"; 				break;}
				case 5:    {prefab = "{4F201B6446013397}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Medic_P.et"; 			break;}
				case 6:    {prefab = "{CC8EDD051CB0C1CE}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_SL_P.et"; 				break;}
				case 7:    {prefab = "{1E08ED0385C765CC}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_TL_P.et"; 				break;}
				case 8:    {prefab = "{690B6588C8F675A4}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AR_P.et"; 				break;}
				case 9:    {prefab = "{F8F683DD0F361239}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AAR_P.et"; 			break;}
				case 10:   {prefab = "{D4DB370FB7ED69BA}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AT_P.et"; 				break;}
				case 11:   {prefab = "{4526D15A702D0E27}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AAT_P.et"; 			break;}
				case 12:   {prefab = "{EC7EF569E938BD63}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Gren_P.et"; 			break;}
				case 13:   {prefab = "{85CCCDA106719506}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Demo_P.et"; 			break;}
				case 14:   {prefab = "{6F99DE8453E6B423}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Rifleman_P.et";		break;}
				case 15:   {prefab = "{232780260F9004E6}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_MMG_P.et"; 			break;}
				case 16:   {prefab = "{0CF6A9358BCE5E57}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AMMG_P.et"; 			break;}
				case 17:   {prefab = "{B6AEA6F0469C66DD}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_HMG_P.et"; 			break;}
				case 18:   {prefab = "{997F8FE3C2C23C6C}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AHMG_P.et"; 			break;}
				case 19:   {prefab = "{DCA2E07271574B14}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Sniper_P.et"; 			break;}
				case 20:   {prefab = "{49CC52FEE609771E}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Spotter_P.et"; 		break;}
				case 21:   {prefab = "{0B9A19205D71AD61}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_MAT_P.et"; 			break;}
				case 22:   {prefab = "{244B3033D92FF7D0}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AMAT_P.et"; 			break;}
				case 23:   {prefab = "{9E133FF6147DCF5A}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_HAT_P.et"; 			break;}
				case 24:   {prefab = "{B1C216E5902395EB}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AHAT_P.et"; 			break;}
				case 25:   {prefab = "{6B23BDB84254123F}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AA_P.et"; 				break;}
				case 26:   {prefab = "{FADE5BED859475A2}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_AAA_P.et"; 			break;}
				case 27:   {prefab = "{478FBA09EDDFC439}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_VehLead_P.et"; 		break;}	
				case 28:   {prefab = "{88611B14BF2B14B1}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_VehDriver_P.et"; 		break;}
				case 29:   {prefab = "{5915E51A1AABB9D2}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_VehGunner_P.et"; 		break;}
				case 30:   {prefab = "{1E94245F486A71E6}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_VehLoader_P.et"; 		break;}
				case 31:   {prefab = "{DDFC9128CBEF1AAB}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_IndirectLead_P.et"; 	break;}
				case 32:   {prefab = "{E6875FC6CEED7B4B}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_IndirectGunner_P.et"; 	break;}
				case 33:   {prefab = "{4ACCBC7F35D46EF7}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_IndirectLoader_P.et"; 	break;}
				case 34:   {prefab = "{810337D2F231A02D}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Pilot_P.et"; 			break;}
				case 35:   {prefab = "{9441BEBB9DF7FC1B}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_CrewChief_P.et"; 		break;}
				case 36:   {prefab = "{48FB5BBC1A942F28}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_LogiLead_P.et"; 		break;}
				case 37:   {prefab = "{EB5C0AB8268A5C43}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_LogiRunner_P.et"; 		break;}
				default:  {prefab = "{6F99DE8453E6B423}Prefabs/Characters/Factions/BLUFOR/CRF_GS_BLUFOR_Rifleman_P.et"; 		break;}
			}
		}else if(factionKey == "OPFOR")
		{
			switch(index)
			{
				case 0:    {prefab = "{37DE97333EEFF4BB}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_COY_P.et"; 				break;}
				case 1:    {prefab = "{823E2872D504BC6C}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_MO_P.et"; 				break;}
				case 2:    {prefab = "{7A2D59429430D672}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_FO_P.et"; 				break;}
				case 3:    {prefab = "{9AA6E3E5E0314052}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_JTAC_P.et"; 				break;}
				case 4:    {prefab = "{9613738A003FBAD2}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_PL_P.et"; 				break;}
				case 5:    {prefab = "{9E37DD66E77861A4}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Medic_P.et"; 			break;}
				case 6:    {prefab = "{9CB623C7B62F4955}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_SL_P.et"; 				break;}
				case 7:    {prefab = "{4E3013C12F58ED57}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_TL_P.et"; 				break;}
				case 8:    {prefab = "{39339B4A6269FD3F}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AR_P.et"; 				break;}
				case 9:    {prefab = "{7C303B9DEA332DF9}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AAR_P.et"; 				break;}
				case 10:   {prefab = "{84E3C9CD1D72E121}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AT_P.et"; 				break;}
				case 11:   {prefab = "{C1E0691A952831E7}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AAT_P.et";				break;}
				case 12:   {prefab = "{950F36099F77E571}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Gren_P.et"; 				break;}
				case 13:   {prefab = "{FCBD0EC1703ECD14}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Demo_P.et"; 				break;}
				case 14:   {prefab = "{FC0904D71EF8DB6A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Rifleman_P.et"; 			break;}
				case 15:   {prefab = "{A7E13866EA953B26}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_MMG_P.et"; 				break;}
				case 16:   {prefab = "{75876A55FD810645}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AMMG_P.et"; 				break;}
				case 17:   {prefab = "{32681EB0A399591D}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_HMG_P.et"; 				break;}
				case 18:   {prefab = "{4100FF2BAF7A9D91}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AHMG_P.et"; 				break;}
				case 19:   {prefab = "{C3569C12A6509E8A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Sniper_P.et"; 			break;}
				case 20:   {prefab = "{C7C9C22599C40225}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Spotter_P.et"; 			break;}
				case 21:   {prefab = "{8F5CA160B87492A1}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_MAT_P.et"; 				break;}
				case 22:   {prefab = "{5D3AF353AF60AFC2}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AMAT_P.et"; 				break;}
				case 23:   {prefab = "{1AD587B6F178F09A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_HAT_P.et"; 				break;}
				case 24:   {prefab = "{C8B3D585E66CCDF9}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AHAT_P.et"; 				break;}
				case 25:   {prefab = "{3B1B437AE8CB9AA4}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AA_P.et"; 				break;}
				case 26:   {prefab = "{7E18E3AD60914A62}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_AAA_P.et"; 				break;}
				case 27:   {prefab = "{963759D86C79E753}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_VehLead_P.et"; 			break;}	
				case 28:   {prefab = "{F513E3FE0CE0E250}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_VehDriver_P.et"; 		break;}
				case 29:   {prefab = "{24671DF0A9604F33}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_VehGunner_P.et"; 		break;}
				case 30:   {prefab = "{63E6DCB5FBA18707}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_VehLoader_P.et"; 		break;}
				case 31:   {prefab = "{1A48A3C1A69E573E}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_IndirectLead_P.et"; 		break;}
				case 32:   {prefab = "{48E86145C5152AD5}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_IndirectGunner_P.et"; 	break;}
				case 33:   {prefab = "{0F69A00097D4E2E1}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_IndirectLoader_P.et"; 	break;}
				case 34:   {prefab = "{5014F1D05348F21E}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Pilot_P.et"; 			break;}
				case 35:   {prefab = "{E93346512E3C0AFA}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_CrewChief_P.et"; 		break;}
				case 36:   {prefab = "{E5250C1BEBC038A2}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_LogiLead_P.et";		 	break;}
				case 37:   {prefab = "{7043203A0D1F11AF}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_LogiRunner_P.et"; 		break;}
				default:  {prefab = "{FC0904D71EF8DB6A}Prefabs/Characters/Factions/OPFOR/CRF_GS_OPFOR_Rifleman_P.et"; 			break;}
			}
		}else if(factionKey == "INDFOR")
		{
			switch(index)
			{
				case 0:    {prefab = "{A53B8AB60784E018}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_COY_P.et"; 			break;}
				case 1:    {prefab = "{97C13390010A9E27}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_MO_P.et"; 				break;}
				case 2:    {prefab = "{6FD242A0403EF439}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_FO_P.et"; 				break;}
				case 3:    {prefab = "{6A9D383435227F56}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_JTAC_P.et"; 			break;}
				case 4:    {prefab = "{83EC6868D4319899}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_PL_P.et"; 				break;}
				case 5:    {prefab = "{23546266D23BBE77}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Medic_P.et"; 			break;}
				case 6:    {prefab = "{8949382562216B1E}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_SL_P.et"; 				break;}
				case 7:    {prefab = "{5BCF0823FB56CF1C}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_TL_P.et"; 				break;}
				case 8:    {prefab = "{2CCC80A8B667DF74}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AR_P.et"; 				break;}
				case 9:    {prefab = "{EED52618D358395A}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AAR_P.et"; 			break;}
				case 10:   {prefab = "{911CD22FC97CC36A}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AT_P.et"; 				break;}
				case 11:   {prefab = "{5305749FAC432544}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AAT_P.et"; 			break;}
				case 12:   {prefab = "{5305749FAC432544}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AAT_P.et"; 			break;}
				case 13:   {prefab = "{0C86D510A52DF210}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Demo_P.et";			break;}
				case 14:   {prefab = "{A303C25424BC7149}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Rifleman_P.et"; 		break;}
				case 15:   {prefab = "{350425E3D3FE2F85}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_MMG_P.et"; 			break;}
				case 16:   {prefab = "{85BCB18428923941}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AMMG_P.et"; 			break;}
				case 17:   {prefab = "{A08D03359AF24DBE}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_HMG_P.et"; 			break;}
				case 18:   {prefab = "{10359752619E5B7A}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AHMG_P.et"; 			break;}
				case 19:   {prefab = "{65F1BBC392B2B484}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Sniper_P.et"; 			break;}
				case 20:   {prefab = "{96BCA2BC7A89F2CF}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Spotter_P.et"; 		break;}
				case 21:   {prefab = "{1DB9BCE5811F8602}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_MAT_P.et"; 			break;}
				case 22:   {prefab = "{AD0128827A7390C6}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AMAT_P.et"; 			break;}
				case 23:   {prefab = "{88309A33C813E439}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_HAT_P.et"; 			break;}
				case 24:   {prefab = "{38880E54337FF2FD}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AHAT_P.et"; 			break;}
				case 25:   {prefab = "{2EE458983CC5B8EF}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AA_P.et"; 				break;}
				case 26:   {prefab = "{ECFDFE2859FA5EC1}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_AAA_P.et";				break;}
				case 27:   {prefab = "{C74239418F3417B9}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_VehLead_P.et"; 		break;}	
				case 28:   {prefab = "{FE9C0A1600114796}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_VehDriver_P.et"; 		break;}
				case 29:   {prefab = "{2FE8F418A591EAF5}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_VehGunner_P.et"; 		break;}
				case 30:   {prefab = "{6869355DF75022C1}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_VehLoader_P.et"; 		break;}
				case 31:   {prefab = "{B28C067E6191CC69}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_IndirectLead_P.et"; 	break;}
				case 32:   {prefab = "{C6A54A2FF38C9845}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_IndirectGunner_P.et"; 	break;}
				case 33:   {prefab = "{81248B6AA14D5071}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_IndirectLoader_P.et"; 	break;}
				case 34:   {prefab = "{ED774ED0660B2DCD}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Pilot_P.et"; 			break;}
				case 35:   {prefab = "{E2BCAFB922CDAF3C}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_CrewChief_P.et"; 		break;}
				case 36:   {prefab = "{6E8D0EF9755740F1}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_LogiLead_P.et"; 		break;}
				case 37:   {prefab = "{AADD26801A0155A4}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_LogiRunner_P.et"; 		break;}
				default:  {prefab = "{A303C25424BC7149}Prefabs/Characters/Factions/INDFOR/CRF_GS_INDFOR_Rifleman_P.et"; 		break;}
			}
		}else if(factionKey == "CIV")
		{
			switch(index)
			{
				case 0:    {prefab = "{66A37C085C2B2873}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_COY_P.et"; 				break;}
				case 1:    {prefab = "{B409C0B57C8ACCBF}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_MO_P.et"; 				break;}
				case 2:    {prefab = "{4C1AB1853DBEA6A1}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_FO_P.et"; 				break;}
				case 3:    {prefab = "{19854386F78A1A01}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_JTAC_P.et"; 				break;}
				case 4:    {prefab = "{A0249B4DA9B1CA01}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_PL_P.et"; 				break;}
				case 5:    {prefab = "{8C7C798DDB2E1DDC}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Medic_P.et"; 			break;}
				case 6:    {prefab = "{AA81CB001FA13986}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_SL_P.et"; 				break;}
				case 7:    {prefab = "{7807FB0686D69D84}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_TL_P.et"; 				break;}
				case 8:    {prefab = "{0F04738DCBE78DEC}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AR_P.et"; 				break;}
				case 9:    {prefab = "{2D4DD0A688F7F131}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AAR_P.et"; 				break;}
				case 10:   {prefab = "{B2D4210AB4FC91F2}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AT_P.et"; 				break;}
				case 11:   {prefab = "{909D8221F7ECED2F}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AAT_P.et"; 				break;}
				case 12:   {prefab = "{162C966A88CCBF22}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Gren_P.et"; 				break;}
				case 13:   {prefab = "{7F9EAEA267859747}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Demo_P.et"; 				break;}
				case 14:   {prefab = "{71EF8F2C5207403C}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Rifleman_P.et"; 			break;}
				case 15:   {prefab = "{F69CD35D8851E7EE}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_MMG_P.et"; 				break;}
				case 16:   {prefab = "{F6A4CA36EA3A5C16}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AMMG_P.et"; 				break;}
				case 17:   {prefab = "{6315F58BC15D85D5}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_HMG_P.et"; 				break;}
				case 18:   {prefab = "{632DECE0A3363E2D}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AHMG_P.et"; 				break;}
				case 19:   {prefab = "{6B2ECC0633190E43}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Sniper_P.et"; 			break;}
				case 20:   {prefab = "{0C3A7A5513878002}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Spotter_P.et"; 			break;}
				case 21:   {prefab = "{DE214A5BDAB04E69}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_MAT_P.et"; 				break;}
				case 22:   {prefab = "{DE195330B8DBF591}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AMAT_P.et"; 				break;}
				case 23:   {prefab = "{4BA86C8D93BC2C52}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_HAT_P.et"; 				break;}
				case 24:   {prefab = "{4B9075E6F1D797AA}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AHAT_P.et"; 				break;}
				case 25:   {prefab = "{0D2CABBD4145EA77}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AA_P.et"; 				break;}
				case 26:   {prefab = "{2F650896025596AA}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_AAA_P.et"; 				break;}
				case 27:   {prefab = "{468195F8659D88FE}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_VehLead_P.et"; 			break;}
				case 28:   {prefab = "{A6410407E82601E2}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_VehDriver_P.et"; 		break;}
				case 29:   {prefab = "{7735FA094DA6AC81}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_VehGunner_P.et"; 		break;}
				case 30:   {prefab = "{30B43B4C1F6764B5}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_VehLoader_P.et"; 		break;}
				case 31:   {prefab = "{988527F813641915}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_IndirectLead_P.et"; 		break;}
				case 32:   {prefab = "{D5FEC649E024636E}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_IndirectGunner_P.et"; 	break;}
				case 33:   {prefab = "{927F070CB2E5AB5A}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_IndirectLoader_P.et"; 	break;}
				case 34:   {prefab = "{425F553B6F1E8E66}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Pilot_P.et"; 			break;}
				case 35:   {prefab = "{BA61A1A8CAFAE948}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_CrewChief_P.et"; 		break;}
				case 36:   {prefab = "{192BB338081045AF}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_LogiLead_P.et"; 			break;}
				case 37:   {prefab = "{598D81084B4F5CDA}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_LogiRunner_P.et"; 		break;}
				default:  {prefab = "{71EF8F2C5207403C}Prefabs/Characters/Factions/CIV/CRF_GS_CIV_Rifleman_P.et"; 			break;}
			}
		} else
		{
			PrintFormat("CRF_ADMINMENU - ERROR | FACTION KEY: %1 DOES NOT MATCH ANY FACTION KEY IN GEARSCRIPT", factionKey);
		}
		
		return prefab;
	}
	
  	override void OnMenuFocusLost()
	{
		m_bFocused = false;
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, Close);
		#endif
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuFocusGained()
	{
		m_bFocused = true;
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN, EActionTrigger.DOWN, Close);
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, Close);
		#ifdef WORKBENCH
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_OPEN_WB, EActionTrigger.DOWN, Close);
			m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK_WB, EActionTrigger.DOWN, Close);
		#endif
	}
	
	//-------------------------------------------------------------------------------------------
	//-------------------------------Gear Menu UI Members----------------------------------------
	//-------------------------------------------------------------------------------------------
	void InitializeGearMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_ActionButton.SetVisible(true);
		m_ActionButton.m_OnClicked.Insert(ResetGear);
		TextWidget.Cast(m_ActionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Reset Gear");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Roles");
		
		
		m_playerManager.GetPlayers(m_allPlayers);
		foreach(int playerID : m_allPlayers)
		{ 
			if(m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list1.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				m_playerIDList.Insert(playerID);
			}
		}
		AddRoles(m_list2);
	}
	
	void ResetGear()
	{
		int playerID = m_playerIDList.Get(m_list1.GetSelectedItem());
		int groupID = m_groupManagerComponent.GetPlayerGroup(playerID).GetGroupID();
		string prefab = GetPrefab(groupID, m_list2.GetSelectedItem());
		CRF_ClientAdminMenuComponent.GetInstance().ResetGear(playerID, prefab);
	}
	
	//-------------------------------------------------------------------------------------------
	//-------------------------------Resspawn Menu UI Members------------------------------------
	//-------------------------------------------------------------------------------------------
	void InitializeRespawnMenu()
	{
		m_list1Root.SetVisible(true);
		m_list2Root.SetVisible(true);
		m_list3Root.SetVisible(true);
		m_list4Root.SetVisible(true);
		m_ActionButton.SetVisible(true);
		m_ActionButton.m_OnClicked.Insert(RespawnPlayer);
		m_list1.m_OnChanged.Insert(UpdateSpawnpoint);
		
		TextWidget.Cast(m_ActionButton.GetRootWidget().FindWidget("ActionButtonText")).SetText("Respawn Player");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List1Text")).SetText("Groups");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List2Text")).SetText("Dead Players");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List3Text")).SetText("Roles");
		TextWidget.Cast(m_wRoot.FindAnyWidget("List4Text")).SetText("Spawnpoints");
		
		m_playerManager.GetPlayers(m_allPlayers);
		m_groupManagerComponent.GetAllPlayableGroups(m_outGroups);
		foreach(int playerID : m_allPlayers)
		{ 
			if(!m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list2.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				m_playerIDList.Insert(playerID);
			}
		}
		int rowNumber = 0;
		foreach(SCR_AIGroup group : m_outGroups)
		{
			string factionTag;
			if(group.GetFaction().GetFactionKey() == "BLUFOR")
				factionTag = "BLU";
			if(group.GetFaction().GetFactionKey() == "OPFOR")
				factionTag = "OPF";
			if(group.GetFaction().GetFactionKey() == "INDFOR")
				factionTag = "IND";
			if(group.GetFaction().GetFactionKey() == "CIV")
				factionTag = "CIV";
			
			m_list1.AddItem(string.Format("%1 | %2", factionTag , group.GetCustomName()));
			m_groupIDList.Insert(group.GetGroupID());
			
		}
		AddRoles(m_list3);
	}
	
	void UpdateSpawnpoint()
	{
		m_list4.Clear();
		m_spawnPoints.Clear();
		int groupID = m_groupIDList.Get(m_list1.GetSelectedItem());
		m_playerManager.GetPlayers(m_allPlayers);
		m_list2.Clear();
		m_playerIDList.Clear();
		foreach(int playerID : m_allPlayers)
		{
			if(!m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				m_list2.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
				m_playerIDList.Insert(playerID);
			}
			if(m_groupManagerComponent.GetPlayerGroup(playerID))
			{
				if(m_groupManagerComponent.GetPlayerGroup(playerID).GetGroupID() == groupID)
				{
					m_list4.AddItem(string.Format("%1", m_playerManager.GetPlayerName(playerID)));
					m_spawnPoints.Insert(GetGame().GetPlayerManager().GetPlayerControlledEntity(playerID).GetOrigin());		
				}
			}
		}
	}
	
	void RespawnPlayer()
	{
		int playerID = m_playerIDList.Get(m_list2.GetSelectedItem());
		int groupID = m_groupIDList.Get(m_list1.GetSelectedItem());
		string prefab = GetPrefab(groupID, m_list3.GetSelectedItem());
		vector spawnpoint = m_spawnPoints.Get(m_list4.GetSelectedItem());
		
		Print("Spawning a player from client");
		CRF_ClientAdminMenuComponent.GetInstance().SpawnGroup(playerID, prefab, spawnpoint , groupID);
	}
}