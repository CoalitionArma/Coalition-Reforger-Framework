//------------------------------------------------------------------------------------------------
//! Live, in-game counterpart to Scripts/WorkbenchGame/MissionPlugins/CRF_MissionQAChecklistPlugin.c
//! (the Workbench-editor QA checklist) - shows every role slotted in the mission, its configured
//! loadout/ammo/radio, and any issues, read off the LIVE COA_Gamemode/COA_GearscriptManager
//! instances instead of parsed .layer/.conf text. Opened via the "/qa" chat command
//! (Scripts/Game/!Systems/ModdedOverrides/Coalition Lobby/Managers/CRF_COA_PLayerChatCommandManager.c),
//! which is itself Workbench-only and admin-only - this menu additionally refuses to open outside
//! Workbench as a second line of defense in case anything else ever opens this preset directly.
//!
//! The ammo/magazine-well helpers here intentionally duplicate (rather than share) the logic in
//! CRF_MissionQAChecklistPlugin.c - both files are Enfusion Script that can't be compiled/tested
//! from outside Workbench, so keeping them independent keeps each one's blast radius contained.
class CRF_MissionQAMenu: ChimeraMenuBase
{
	protected Widget m_wRoot;
	protected OverlayWidget m_wRoleListRoot;
	protected SCR_ListBoxComponent m_wRoleListBox;
	protected MultilineEditBoxWidget m_wDetailBox;
	protected SCR_ButtonTextComponent m_bRefreshButton;
	protected Widget m_wWeaponPreviewBackground;
	//! Holds one dynamically-created weapon-preview cell per weapon the selected role carries (e.g.
	//! RIFLEMAN_ANTITANK gets a rifle cell AND an AT launcher cell), stacked vertically - same "clear
	//! children, CreateWidgets() one per entry" pattern CRF_MiniArsenal.c already uses for its item
	//! list.
	protected VerticalLayoutWidget m_wWeaponPreviewList;
	protected const string WEAPON_PREVIEW_CELL_WIDE = "{346E0876850453E2}UI/MissionQA/CRF_MissionQAWeaponPreviewCell.layout";
	protected const string WEAPON_PREVIEW_CELL_SMALL = "{43A7FADEECD07917}UI/MissionQA/CRF_MissionQAWeaponPreviewCellSmall.layout";

	//! Index 0 is always the "Mission Summary" sentinel (null entry); every following index is a
	//! slotted (faction, role) pair, in the same order as m_wRoleListBox's items.
	protected ref array<ref CRF_MissionQARoleEntry> m_aEntries = {};

	protected static const ref array<string> FACTIONS = {"BLUFOR", "OPFOR", "INDFOR", "CIV"};

	protected static const ref array<COA_EGearRole> LEADERSHIP_ROLES = {
		COA_EGearRole.COMPANY_COMMANDER,
		COA_EGearRole.FIRST_SERGEANT,
		COA_EGearRole.PLATOON_LEADER,
		COA_EGearRole.PLATOON_SERGEANT,
		COA_EGearRole.SQUAD_LEAD,
		COA_EGearRole.TEAM_LEAD,
		COA_EGearRole.VEHICLE_LEAD,
		COA_EGearRole.INDIRECT_LEAD,
		COA_EGearRole.LOGI_LEAD
	};

	protected static const ref array<COA_ESlotType> LEADERSHIP_SLOT_TYPES = {
		COA_ESlotType.TEAM_LEADER,
		COA_ESlotType.SQUAD_LEADER,
		COA_ESlotType.SPECIALTY,
		COA_ESlotType.SPECIALTY_ASSISTANT
	};

//=============================================================================================================================================================================================================================================================================================================================================================
//	 LIFECYCLE
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();

#ifdef WORKBENCH
		m_wRoot = GetRootWidget();

		m_wRoleListRoot = OverlayWidget.Cast(m_wRoot.FindAnyWidget("RoleListBox0"));
		if (!m_wRoleListRoot)
		{
			Close();
			return;
		}

		m_wRoleListBox = SCR_ListBoxComponent.Cast(m_wRoleListRoot.FindHandler(SCR_ListBoxComponent));
		m_wDetailBox = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("QADetailBox0"));
		m_bRefreshButton = SCR_ButtonTextComponent.GetButtonText("MenuButton0", m_wRoot);
		m_wWeaponPreviewBackground = m_wRoot.FindAnyWidget("WeaponPreviewBackground0");
		m_wWeaponPreviewList = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("WeaponPreviewList0"));

		if (!m_wRoleListBox || !m_wDetailBox || !m_bRefreshButton || !m_wWeaponPreviewBackground || !m_wWeaponPreviewList)
		{
			Close();
			return;
		}

		m_wRoleListBox.m_OnChanged.Insert(OnRoleSelectionChanged);
		m_bRefreshButton.m_OnClicked.Insert(Refresh);

		GetGame().GetInputManager().AddActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);

		Refresh();
#else
		Close();
#endif
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		super.OnMenuClose();

		if (GetGame())
			GetGame().GetInputManager().RemoveActionListener("MenuBack", EActionTrigger.DOWN, Action_Exit);

		if (m_wRoleListBox)
			m_wRoleListBox.m_OnChanged.Remove(OnRoleSelectionChanged);

		if (m_bRefreshButton)
			m_bRefreshButton.m_OnClicked.Remove(Refresh);
	}

	//------------------------------------------------------------------------------------------------
	void Action_Exit()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.CRF_MissionQAMenu);
	}

	//------------------------------------------------------------------------------------------------
	//! Rebuilds the role list from the live mission state and shows the Mission Summary again.
	void Refresh()
	{
		PopulateRoleList();

		if (m_wRoleListBox.GetItemCount() > 0)
			m_wRoleListBox.SetItemSelected(0, true, true);
		else
			ShowMissionSummary();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRoleSelectionChanged()
	{
		int index = m_wRoleListBox.GetSelectedItem();
		if (index < 0 || index >= m_aEntries.Count())
			return;

		CRF_MissionQARoleEntry entry = m_aEntries[index];
		if (!entry)
			ShowMissionSummary();
		else
			ShowRoleDetail(entry);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ROLE LIST
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void PopulateRoleList()
	{
		m_wRoleListBox.Clear();
		m_aEntries.Clear();

		m_wRoleListBox.AddItem("Mission Summary");
		m_aEntries.Insert(null);

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
			return;

		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();

		foreach (string factionKey : FACTIONS)
		{
			array<ref COA_SlottingGroup> slots = gamemode.GetSlots(factionKey);
			if (!slots || slots.IsEmpty())
				continue;

			map<COA_EGearRole, int> roleCounts = new map<COA_EGearRole, int>();
			array<COA_EGearRole> roleOrder = {};

			foreach (ref COA_SlottingGroup group : slots)
			{
				if (!group || !group.m_aSlots)
					continue;

				foreach (COA_EGearRole role : group.m_aSlots)
				{
					if (!roleCounts.Contains(role))
					{
						roleCounts.Set(role, 0);
						roleOrder.Insert(role);
					}

					roleCounts.Set(role, roleCounts.Get(role) + 1);
				}
			}

			foreach (COA_EGearRole role : roleOrder)
			{
				int count = roleCounts.Get(role);

				CRF_MissionQARoleEntry entry = new CRF_MissionQARoleEntry();
				entry.m_sFactionKey = factionKey;
				entry.m_eRole = role;
				entry.m_iSlotCount = count;
				m_aEntries.Insert(entry);

				m_wRoleListBox.AddItem(string.Format("%1 - %2 (x%3)", factionKey, GetRoleName(role, rolesConfig), count));
			}
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISSION SUMMARY
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void ShowMissionSummary()
	{
		ShowWeaponPreviews({}, {}, {});

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
		{
			m_wDetailBox.SetText("COA_Gamemode is not available yet.");
			return;
		}

		array<string> lines = {};
		lines.Insert("MISSION SUMMARY");
		lines.Insert("");

		AppendFactionRatios(lines, gamemode);
		lines.Insert("");
		AppendSpectatorVisibility(lines, gamemode);
		lines.Insert("");
		AppendBriefingStatus(lines, gamemode);
		lines.Insert("");
		AppendPerFactionRollup(lines, gamemode);

		m_wDetailBox.SetText(SCR_StringHelper.Join("\n", lines));
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendFactionRatios(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("Faction Ratios");

		if (gamemode.m_iFactionOneRatio <= 0 && gamemode.m_iFactionTwoRatio <= 0)
		{
			lines.Insert("  [X] Neither Faction Ratio is set - at least one must be greater than 0 for ratio-based (TVT) slotting");
			return;
		}

		lines.Insert(string.Format("  %1 %2 : %3 %4", gamemode.m_sFactionOneKey, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, gamemode.m_sFactionTwoKey));

		if (gamemode.m_iFactionOneRatio <= 0)
			lines.Insert("  [!] Faction One Ratio is 0");
		else if (gamemode.m_sFactionOneKey.IsEmpty())
			lines.Insert("  [X] Faction One Ratio is set but no Faction One Key is assigned");

		if (gamemode.m_iFactionTwoRatio <= 0)
			lines.Insert("  [!] Faction Two Ratio is 0");
		else if (gamemode.m_sFactionTwoKey.IsEmpty())
			lines.Insert("  [X] Faction Two Ratio is set but no Faction Two Key is assigned");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendSpectatorVisibility(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("Spectator Visibility");

		bool eventBasedRespawn = gamemode.FindComponent(CRF_CacheHuntGamemodeManager) != null
			|| gamemode.FindComponent(CRF_RushGamemodeManager) != null;

		if (!gamemode.m_bRespawnEnabled && !eventBasedRespawn)
		{
			lines.Insert("  [OK] One-life mission - not a concern");
			return;
		}

		if (gamemode.m_bHideOtherSpectatorFactions)
		{
			lines.Insert("  [OK] Other factions are hidden in spectator");
			return;
		}

		lines.Insert("  [X] Respawn/event-based respawn is active but \"Hide Other Spectator Factions\" is off");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendBriefingStatus(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("Briefing");

		if (!gamemode.m_aMissionDescriptors || gamemode.m_aMissionDescriptors.IsEmpty())
		{
			lines.Insert("  [X] No mission briefing/descriptors are configured");
			return;
		}

		int emptyCount, defaultCount;
		foreach (ref COA_MissionDescriptor descriptor : gamemode.m_aMissionDescriptors)
		{
			if (!descriptor)
				continue;

			if (descriptor.m_sTextData.IsEmpty())
				emptyCount++;
			else if (IsDefaultDescriptorText(gamemode, descriptor))
				defaultCount++;
		}

		if (emptyCount == 0 && defaultCount == 0)
		{
			lines.Insert("  [OK] Filled out");
			return;
		}

		if (emptyCount > 0)
			lines.Insert(string.Format("  [X] %1 briefing entr%2 with no text", emptyCount, PluralySuffix(emptyCount)));

		if (defaultCount > 0)
			lines.Insert(string.Format("  [X] %1 briefing entr%2 still has the default template text", defaultCount, PluralySuffix(defaultCount)));
	}

	//------------------------------------------------------------------------------------------------
	protected string PluralySuffix(int count)
	{
		if (count == 1)
			return "y";

		return "ies";
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsDefaultDescriptorText(COA_Gamemode gamemode, COA_MissionDescriptor descriptor)
	{
		if (!gamemode.m_aDefaultMissionDescriptors)
			return false;

		foreach (ref COA_MissionDescriptor defaultDescriptor : gamemode.m_aDefaultMissionDescriptors)
		{
			if (!defaultDescriptor)
				continue;

			if (defaultDescriptor.m_sTitle == descriptor.m_sTitle && defaultDescriptor.m_sTextData == descriptor.m_sTextData)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! One line per faction: gearscript status + ammo-compatibility issue count across its whole
	//! weapon pool (not role-scoped - see ShowRoleDetail for a specific role's own weapon).
	protected void AppendPerFactionRollup(array<string> lines, COA_Gamemode gamemode)
	{
		lines.Insert("Gear (per faction)");

		foreach (string factionKey : FACTIONS)
		{
			array<ref COA_SlottingGroup> slots = gamemode.GetSlots(factionKey);
			if (!slots || slots.IsEmpty())
				continue;

			ResourceName gearScriptResource = gamemode.GetGearScriptResource(factionKey);
			if (gearScriptResource.IsEmpty())
			{
				lines.Insert(string.Format("  [X] %1: no gearscript assigned", factionKey));
				continue;
			}

			COA_GearScriptConfig gearConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(gearScriptResource);
			if (!gearConfig)
			{
				lines.Insert(string.Format("  [X] %1: gearscript %2 did not load", factionKey, gearScriptResource));
				continue;
			}

			int ammoIssues = CountAmmoCompatibilityIssues(gearConfig);
			if (ammoIssues > 0)
				lines.Insert(string.Format("  [!] %1: %2 magazine/weapon well mismatch(es) - see individual roles", factionKey, ammoIssues));
			else
				lines.Insert(string.Format("  [OK] %1: gearscript loaded, no ammo mismatches found", factionKey));
		}
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ROLE DETAIL
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void ShowRoleDetail(CRF_MissionQARoleEntry entry)
	{
		ShowWeaponPreviews({}, {}, {});

		COA_Gamemode gamemode = COA_Gamemode.GetInstance();
		if (!gamemode)
		{
			m_wDetailBox.SetText("COA_Gamemode is not available yet.");
			return;
		}

		array<string> lines = {};
		COA_RolesConfig rolesConfig = COA_GearscriptManager.GetRolesConfig();
		lines.Insert(string.Format("%1 - %2 (x%3)", entry.m_sFactionKey, GetRoleName(entry.m_eRole, rolesConfig), entry.m_iSlotCount));
		lines.Insert("");

		ResourceName gearScriptResource = gamemode.GetGearScriptResource(entry.m_sFactionKey);
		if (gearScriptResource.IsEmpty())
		{
			lines.Insert("[X] No gearscript assigned to this faction");
			m_wDetailBox.SetText(SCR_StringHelper.Join("\n", lines));
			return;
		}

		COA_GearScriptConfig gearConfig = COA_GearscriptManager.GetInstance().LoadGearScriptConfig(gearScriptResource);
		if (!gearConfig)
		{
			lines.Insert(string.Format("[X] Gearscript %1 did not load", gearScriptResource));
			m_wDetailBox.SetText(SCR_StringHelper.Join("\n", lines));
			return;
		}

		COA_RoleConfig roleConfig;
		if (rolesConfig)
			roleConfig = rolesConfig.FindRoleConfig(entry.m_eRole);

		array<ResourceName> previewWeapons = {};
		array<string> previewLabels = {};
		array<bool> previewIsPistol = {};
		AppendWeaponAndAmmo(lines, entry.m_eRole, gearConfig, roleConfig, previewWeapons, previewLabels, previewIsPistol);
		lines.Insert("");
		AppendRadioStatus(lines, entry.m_eRole, gamemode.GetGearScriptSettings(entry.m_sFactionKey), roleConfig);

		if (entry.m_eRole == COA_EGearRole.MEDIC)
		{
			lines.Insert("");
			AppendMedicStatus(lines, gearConfig);
		}

		m_wDetailBox.SetText(SCR_StringHelper.Join("\n", lines));
		ShowWeaponPreviews(previewWeapons, previewLabels, previewIsPistol);
	}

	//------------------------------------------------------------------------------------------------
	//! Shows one live 3D preview cell per weapon the role carries, stacked vertically - a role can
	//! have more than one (e.g. RIFLEMAN_ANTITANK gets a rifle AND an AT launcher), so this
	//! dynamically creates one cell instance per entry into m_wWeaponPreviewList, the same "clear
	//! then CreateWidgets() per item" pattern CRF_MiniArsenal.c uses for its item list. Long guns get
	//! the wide cell (full container width, landscape) since a rifle/launcher needs the horizontal
	//! room to render recognizably; a pistol is small enough that the compact square cell is plenty,
	//! so it keeps its own fixed size instead of stretching. Each cell's own preview comes from the
	//! same ItemPreviewManagerEntity.SetPreviewItemFromPrefab mechanism CRF's Arsenal UIs already use
	//! for item art from a bare ResourceName.
	protected void ShowWeaponPreviews(array<ResourceName> weapons, array<string> labels, array<bool> isPistol)
	{
		while (m_wWeaponPreviewList.GetChildren())
			m_wWeaponPreviewList.GetChildren().RemoveFromHierarchy();

		if (!weapons || weapons.IsEmpty())
		{
			m_wWeaponPreviewBackground.SetVisible(false);
			return;
		}

		ItemPreviewManagerEntity manager = ChimeraWorld.CastFrom(GetGame().GetWorld()).GetItemPreviewManager();
		if (!manager)
		{
			m_wWeaponPreviewBackground.SetVisible(false);
			return;
		}

		for (int i = 0, count = weapons.Count(); i < count; i++)
		{
			if (weapons[i].IsEmpty())
				continue;

			string cellLayout = WEAPON_PREVIEW_CELL_WIDE;
			if (i < isPistol.Count() && isPistol[i])
				cellLayout = WEAPON_PREVIEW_CELL_SMALL;

			Widget cell = GetGame().GetWorkspace().CreateWidgets(cellLayout, m_wWeaponPreviewList);
			if (!cell)
				continue;

			ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(cell.FindAnyWidget("PreviewWidget"));
			if (previewWidget)
				manager.SetPreviewItemFromPrefab(previewWidget, weapons[i]);

			TextWidget labelWidget = TextWidget.Cast(cell.FindAnyWidget("PreviewLabel"));
			if (labelWidget && i < labels.Count())
				labelWidget.SetText(labels[i]);
		}

		m_wWeaponPreviewBackground.SetVisible(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Weapon assignment(s) + total magazine count. A role can carry more than one weapon at once -
	//! e.g. RIFLEMAN_ANTITANK's COA_RoleConfig.m_aWeapons lists both RIFLE and AT, so it gets a rifle
	//! AND the launcher - so every entry in m_aWeapons is shown, not just one. Driven directly off the
	//! role's actual m_aWeapons data (matching COA_GearscriptManager.ApplyDefaultWeapons's own
	//! per-entry loop) rather than a hardcoded per-role guess, so it stays correct if the mission's
	//! role config changes. Every resolved weapon prefab (and its display label, for the preview
	//! cell's caption, and whether it's a pistol, for the preview cell's size) is appended to
	//! previewWeapons/previewLabels/previewIsPistol for the live item preview.
	protected void AppendWeaponAndAmmo(array<string> lines, COA_EGearRole role, COA_GearScriptConfig gearConfig, COA_RoleConfig roleConfig, inout array<ResourceName> previewWeapons, inout array<string> previewLabels, inout array<bool> previewIsPistol)
	{
		lines.Insert("Weapon");

		if (!roleConfig || !roleConfig.m_aWeapons || roleConfig.m_aWeapons.IsEmpty())
		{
			lines.Insert("  [X] No weapons configured for this role");
		}
		else
		{
			foreach (COA_EGearscriptWeapons weaponType : roleConfig.m_aWeapons)
			{
				ResourceName weaponResource = AppendWeaponTypeLine(lines, gearConfig, weaponType);
				if (!weaponResource.IsEmpty())
				{
					previewWeapons.Insert(weaponResource);
					previewLabels.Insert(GetWeaponTypeLabel(weaponType));
					previewIsPistol.Insert(weaponType == COA_EGearscriptWeapons.PISTOL);
				}
			}
		}

		lines.Insert("");
		lines.Insert("Ammunition");

		if (!roleConfig || !roleConfig.m_aMagazines)
		{
			lines.Insert("  Unable to determine - role not found in the global roles config");
			return;
		}

		bool isAssistant = (roleConfig.m_SlottingType == COA_ESlotType.ASSISTANT || roleConfig.m_SlottingType == COA_ESlotType.SPECIALTY_ASSISTANT);
		int totalRounds;
		int totalMagazineCount;
		bool anyMagazineType;

		foreach (COA_EGearscriptMagazines magType : roleConfig.m_aMagazines)
		{
			array<ref COA_Magazine_Class> magArray = ResolveMagazineArray(gearConfig, magType, isAssistant);
			if (!magArray || magArray.IsEmpty())
				continue;

			anyMagazineType = true;

			//! RIFLEUGL_MAG is a combo category - its magazine array mixes rifle-caliber magazines
			//! with underslung-launcher rounds (of possibly several distinct types - HE/smoke/flare/
			//! etc.) in one list (the same "one weapon file, two wells" structure that caused the
			//! earlier UGL ammo-compatibility false positives). Split it by magazine well so "main
			//! weapon" and "UGL" ammo are reported as separate groups, each broken out by type, rather
			//! than one lumped total.
			if (magType == COA_EGearscriptMagazines.RIFLEUGL_MAG && gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty() && gearConfig.m_RifleUGLs[0])
			{
				array<typename> ownWells = {};
				array<typename> attachmentWells = {};
				GetWeaponWellsSplit(gearConfig.m_RifleUGLs[0].m_Weapon, ownWells, attachmentWells);

				array<ref COA_Magazine_Class> rifleMags = {};
				array<ref COA_Magazine_Class> uglMags = {};
				array<ref COA_Magazine_Class> unclassifiedMags = {};
				SplitMagazinesByWell(magArray, ownWells, attachmentWells, rifleMags, uglMags, unclassifiedMags);

				lines.Insert("  Rifle magazines:");
				AppendMagazineBreakdown(lines, rifleMags, totalRounds, totalMagazineCount);

				lines.Insert("  UGL rounds:");
				AppendMagazineBreakdown(lines, uglMags, totalRounds, totalMagazineCount);

				if (!unclassifiedMags.IsEmpty())
				{
					lines.Insert("  [!] Unclassified (well did not match rifle or UGL - see Mission Summary for ammo compatibility issues):");
					AppendMagazineBreakdown(lines, unclassifiedMags, totalRounds, totalMagazineCount);
				}

				continue;
			}

			lines.Insert(string.Format("  %1:", SCR_Enum.GetEnumName(COA_EGearscriptMagazines, magType)));
			AppendMagazineBreakdown(lines, magArray, totalRounds, totalMagazineCount);
		}

		if (!anyMagazineType)
			lines.Insert("  No magazines configured for this role's magazine type(s)");
		else
			lines.Insert(string.Format("  Total: %1 magazine(s)/round(s), %2 rounds", totalMagazineCount, totalRounds));
	}

	//------------------------------------------------------------------------------------------------
	//! Appends one line per distinct magazine/round entry, inferring each entry's round total from
	//! its own magazine capacity (MaxAmmo) rather than just counting magazines - so "how much
	//! ammunition" reflects actual rounds carried, not just how many separate mags that is.
	protected void AppendMagazineBreakdown(array<string> lines, array<ref COA_Magazine_Class> magazines, inout int totalRounds, inout int totalMagazineCount)
	{
		if (!magazines)
			return;

		foreach (COA_Magazine_Class magazine : magazines)
		{
			if (!magazine || magazine.m_Magazine.IsEmpty() || magazine.m_MagazineCount <= 0)
				continue;

			int capacity = GetMagazineCapacity(magazine.m_Magazine);
			totalMagazineCount += magazine.m_MagazineCount;
			string fileName = GetFileName(magazine.m_Magazine.GetPath());

			if (capacity > 0)
			{
				int rounds = capacity * magazine.m_MagazineCount;
				totalRounds += rounds;
				lines.Insert(string.Format("    %1: %2x %3 rounds = %4 rounds", fileName, magazine.m_MagazineCount, capacity, rounds));
			}
			else
			{
				lines.Insert(string.Format("    %1: %2x (capacity unknown)", fileName, magazine.m_MagazineCount));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Buckets a combo weapon's mixed magazine array by which well each entry actually fits - the
	//! weapon's own barrel (ownWells) vs an attachment like an UGL (attachmentWells) vs neither (a
	//! real mismatch, already surfaced separately by the mission-summary ammo-compatibility rollup;
	//! kept here rather than silently dropped so totals still add up).
	protected void SplitMagazinesByWell(array<ref COA_Magazine_Class> magazines, array<typename> ownWells, array<typename> attachmentWells, array<ref COA_Magazine_Class> ownMagazines, array<ref COA_Magazine_Class> attachmentMagazines, array<ref COA_Magazine_Class> unclassifiedMagazines)
	{
		foreach (COA_Magazine_Class magazine : magazines)
		{
			if (!magazine)
				continue;

			typename well = GetMagazineWell(magazine.m_Magazine);

			if (well && ownWells.Contains(well))
				ownMagazines.Insert(magazine);
			else if ((well && attachmentWells.Contains(well)) || IsLikelyLauncherAmmo(magazine.m_Magazine))
				attachmentMagazines.Insert(magazine);
			else
				unclassifiedMagazines.Insert(magazine);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Same label text AppendWeaponTypeLine's branches already print in the Weapon section - reused
	//! as-is for the preview cell's caption so the picture and the text line clearly correspond.
	protected string GetWeaponTypeLabel(COA_EGearscriptWeapons weaponType)
	{
		switch (weaponType)
		{
			case COA_EGearscriptWeapons.RIFLE: return "Rifle";
			case COA_EGearscriptWeapons.RIFLEUGL: return "Rifle (UGL)";
			case COA_EGearscriptWeapons.CARBINE: return "Carbine";
			case COA_EGearscriptWeapons.PISTOL: return "Pistol";
			case COA_EGearscriptWeapons.SNIPER: return "Sniper";
			case COA_EGearscriptWeapons.AR: return "AR";
			case COA_EGearscriptWeapons.MMG: return "MMG";
			case COA_EGearscriptWeapons.HMG: return "HMG";
			case COA_EGearscriptWeapons.AT: return "AT";
			case COA_EGearscriptWeapons.MAT: return "MAT";
			case COA_EGearscriptWeapons.HAT: return "HAT";
			case COA_EGearscriptWeapons.AA: return "AA";
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! Dispatches one COA_EGearscriptWeapons entry to the matching gearConfig field, mirroring
	//! COA_GearscriptManager.ApplyDefaultWeapons's own switch exactly (same field-per-case mapping),
	//! so this stays correct for whatever weapon types a role's data actually lists. Returns the
	//! resolved weapon's ResourceName (for the item preview), or empty if nothing was configured.
	protected ResourceName AppendWeaponTypeLine(array<string> lines, COA_GearScriptConfig gearConfig, COA_EGearscriptWeapons weaponType)
	{
		switch (weaponType)
		{
			case COA_EGearscriptWeapons.RIFLE:
				return AppendPoolWeaponLine(lines, gearConfig.m_Rifles, "Rifle");

			case COA_EGearscriptWeapons.RIFLEUGL:
				return AppendPoolWeaponLine(lines, gearConfig.m_RifleUGLs, "Rifle (UGL)");

			case COA_EGearscriptWeapons.CARBINE:
				return AppendPoolWeaponLine(lines, gearConfig.m_Carbines, "Carbine");

			case COA_EGearscriptWeapons.PISTOL:
				return AppendPoolWeaponLine(lines, gearConfig.m_Pistols, "Pistol");

			case COA_EGearscriptWeapons.SNIPER:
				return AppendSniperWeaponLine(lines, gearConfig);

			case COA_EGearscriptWeapons.AR:
				return AppendSpecWeaponLine(lines, gearConfig.m_AR, "AR");

			case COA_EGearscriptWeapons.MMG:
				return AppendSpecWeaponLine(lines, gearConfig.m_MMG, "MMG");

			case COA_EGearscriptWeapons.HMG:
				return AppendSpecWeaponLine(lines, gearConfig.m_HMG, "HMG");

			case COA_EGearscriptWeapons.AT:
				return AppendSpecWeaponLine(lines, gearConfig.m_AT, "AT");

			case COA_EGearscriptWeapons.MAT:
				return AppendSpecWeaponLine(lines, gearConfig.m_MAT, "MAT");

			case COA_EGearscriptWeapons.HAT:
				return AppendSpecWeaponLine(lines, gearConfig.m_HAT, "HAT");

			case COA_EGearscriptWeapons.AA:
				return AppendSpecWeaponLine(lines, gearConfig.m_AA, "AA");
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! RIFLE/RIFLEUGL/CARBINE/PISTOL are all drawn randomly from a pool at actual gear-application
	//! time (COA_WeaponHelper.SelectRandomWeapon), not one fixed weapon - index [0] is shown as a
	//! representative example, same convention already used for the ammo totals below.
	protected ResourceName AppendPoolWeaponLine(array<string> lines, array<ref COA_Weapon_Class> pool, string label)
	{
		if (!pool || pool.IsEmpty() || !pool[0] || pool[0].m_Weapon.IsEmpty())
		{
			lines.Insert(string.Format("  [X] No %1 configured for this role", label));
			return string.Empty;
		}

		lines.Insert(string.Format("  %1: %2", label, pool[0].m_Weapon));
		lines.Insert(string.Format("    (representative - general infantry pool, exact %1 issued may vary)", label));
		return pool[0].m_Weapon;
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName AppendSniperWeaponLine(array<string> lines, COA_GearScriptConfig gearConfig)
	{
		if (!gearConfig.m_SNIPER || gearConfig.m_SNIPER.m_Weapon.IsEmpty())
		{
			lines.Insert("  [X] No Sniper weapon configured for this role");
			return string.Empty;
		}

		lines.Insert(string.Format("  Sniper: %1", gearConfig.m_SNIPER.m_Weapon));
		if (gearConfig.m_SNIPER.m_MagazineArray)
			AppendWeaponAmmoIssues(lines, gearConfig.m_SNIPER.m_Weapon, gearConfig.m_SNIPER.m_MagazineArray);

		return gearConfig.m_SNIPER.m_Weapon;
	}

	//------------------------------------------------------------------------------------------------
	protected ResourceName AppendSpecWeaponLine(array<string> lines, COA_Spec_Weapon_Class weapon, string label)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty())
		{
			lines.Insert(string.Format("  [X] No %1 configured for this role", label));
			return string.Empty;
		}

		lines.Insert(string.Format("  %1: %2", label, weapon.m_Weapon));
		AppendSpecWeaponAmmoIssues(lines, weapon.m_Weapon, weapon.m_MagazineArray);
		return weapon.m_Weapon;
	}

	//------------------------------------------------------------------------------------------------
	protected array<ref COA_Magazine_Class> ResolveMagazineArray(COA_GearScriptConfig gearConfig, COA_EGearscriptMagazines magType, bool isAssistant)
	{
		switch (magType)
		{
			case COA_EGearscriptMagazines.RIFLE_MAG:
				if (gearConfig.m_Rifles && !gearConfig.m_Rifles.IsEmpty())
					return gearConfig.m_Rifles[0].m_MagazineArray;
				break;

			case COA_EGearscriptMagazines.RIFLEUGL_MAG:
				if (gearConfig.m_RifleUGLs && !gearConfig.m_RifleUGLs.IsEmpty())
					return gearConfig.m_RifleUGLs[0].m_MagazineArray;
				break;

			case COA_EGearscriptMagazines.CARBINE_MAG:
				if (gearConfig.m_Carbines && !gearConfig.m_Carbines.IsEmpty())
					return gearConfig.m_Carbines[0].m_MagazineArray;
				break;

			case COA_EGearscriptMagazines.PISTOL_MAG:
				if (gearConfig.m_Pistols && !gearConfig.m_Pistols.IsEmpty())
					return gearConfig.m_Pistols[0].m_MagazineArray;
				break;

			case COA_EGearscriptMagazines.SNIPER_MAG:
				if (gearConfig.m_SNIPER)
					return gearConfig.m_SNIPER.m_MagazineArray;
				break;

			case COA_EGearscriptMagazines.AR_MAG:
				if (gearConfig.m_AR)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AR.m_MagazineArray, isAssistant);
				break;

			case COA_EGearscriptMagazines.MMG_MAG:
				if (gearConfig.m_MMG)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MMG.m_MagazineArray, isAssistant);
				break;

			case COA_EGearscriptMagazines.AT_MAG:
				if (gearConfig.m_AT)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AT.m_MagazineArray, isAssistant);
				break;

			case COA_EGearscriptMagazines.MAT_MAG:
				if (gearConfig.m_MAT)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_MAT.m_MagazineArray, isAssistant);
				break;

			case COA_EGearscriptMagazines.HAT_MAG:
				if (gearConfig.m_HAT)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HAT.m_MagazineArray, isAssistant);
				break;

			case COA_EGearscriptMagazines.AA_MAG:
				if (gearConfig.m_AA)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_AA.m_MagazineArray, isAssistant);
				break;

			case COA_EGearscriptMagazines.HMG_MAG:
				if (gearConfig.m_HMG)
					return COA_WeaponHelper.ConvertSpecMagArrayIntoMagArray(gearConfig.m_HMG.m_MagazineArray, isAssistant);
				break;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendRadioStatus(array<string> lines, COA_EGearRole role, COA_GearScriptContainer gearContainer, COA_RoleConfig roleConfig)
	{
		lines.Insert("Radio");

		if (!gearContainer || !roleConfig || !roleConfig.m_aItems)
		{
			lines.Insert("  Unable to determine");
			return;
		}

		bool isLeadershipSlot = LEADERSHIP_SLOT_TYPES.Contains(roleConfig.m_SlottingType);
		bool found;

		if (roleConfig.m_aItems.Contains(COA_EGearscriptItems.SHORTRANGE_RADIO))
		{
			found = true;
			bool expects = gearContainer.m_bEnableGIRadios || (gearContainer.m_bEnableLeadershipRadios && isLeadershipSlot);
			if (expects && gearContainer.m_rShortRangeRadioPrefab.IsEmpty())
				lines.Insert("  [X] Should get a short-range radio but none is configured");
			else if (expects)
				lines.Insert("  [OK] Short-range radio configured");
			else
				lines.Insert("  No short-range radio for this role under current settings");
		}

		if (roleConfig.m_aItems.Contains(COA_EGearscriptItems.LONGRANGE_RADIO))
		{
			found = true;
			if (gearContainer.m_bEnableLeadershipRadios && gearContainer.m_rLongRangeRadioPrefab.IsEmpty())
				lines.Insert("  [X] Should get a long-range radio but none is configured");
			else if (gearContainer.m_bEnableLeadershipRadios)
				lines.Insert("  [OK] Long-range radio configured");
			else
				lines.Insert("  Long-range radios disabled for this faction");
		}

		if (roleConfig.m_aItems.Contains(COA_EGearscriptItems.RTO_RADIO))
		{
			found = true;
			if (gearContainer.m_bEnableRTORadios && gearContainer.m_rRTORadiosPrefab.IsEmpty())
				lines.Insert("  [X] Should get an RTO radio but none is configured");
			else if (gearContainer.m_bEnableRTORadios)
				lines.Insert("  [OK] RTO radio configured");
			else
				lines.Insert("  [!] RTO radios are disabled for this faction - will spawn without a radio");
		}

		if (!found)
			lines.Insert("  No radio item configured for this role");
	}

	//------------------------------------------------------------------------------------------------
	protected void AppendMedicStatus(array<string> lines, COA_GearScriptConfig gearConfig)
	{
		lines.Insert("Medic Kit");

		if (!gearConfig.m_MedicMedicalItems || gearConfig.m_MedicMedicalItems.IsEmpty())
		{
			lines.Insert("  [X] No medic medical items are configured");
			return;
		}

		foreach (COA_Inventory_Item item : gearConfig.m_MedicMedicalItems)
		{
			if (item && item.m_sItemPrefab.GetPath().Contains("MedicalKit"))
			{
				lines.Insert("  [OK] Medical Kit present");
				return;
			}
		}

		lines.Insert("  [X] No Medical Kit among the medic items - medics won't be able to fully heal other players");
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SHARED HELPERS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected string GetRoleName(COA_EGearRole role, COA_RolesConfig rolesConfig)
	{
		if (rolesConfig)
		{
			COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
			if (roleConfig && !roleConfig.m_sRoleName.IsEmpty())
				return roleConfig.m_sRoleName;
		}

		return SCR_Enum.GetEnumName(COA_EGearRole, role);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 AMMO / MAGAZINE WELL COMPATIBILITY
//
//	 Same approach as CRF_MissionQAChecklistPlugin.c's ammo-compatibility check: read every
//	 magazine well a weapon accepts off its MuzzleComponent(s), plus recursively through any
//	 AttachmentSlotComponent (a UGL is a separate prefab, not a second MuzzleComponent on the
//	 rifle itself), and skip a mismatch when the magazine's own name looks like underslung-launcher
//	 ammunition rather than a rifle-caliber round (grenades/flares/smoke commonly live in an
//	 attachment prefab this can't resolve, e.g. a vanilla or third-party-mod UGL).
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	protected void GetWeaponMagazineWells(ResourceName weaponPrefab, array<typename> wells, array<ResourceName> visited)
	{
		if (weaponPrefab.IsEmpty() || visited.Contains(weaponPrefab))
			return;

		visited.Insert(weaponPrefab);

		Resource resource = Resource.Load(weaponPrefab);
		if (!resource.IsValid())
			return;

		BaseContainer weaponContainer = resource.GetResource().ToBaseContainer();
		if (!weaponContainer)
			return;

		array<IEntityComponentSource> muzzleSources = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(weaponContainer, MuzzleComponent, true, muzzleSources);

		foreach (IEntityComponentSource muzzleSource : muzzleSources)
		{
			BaseContainer magazineWellContainer = muzzleSource.GetObject("MagazineWell");
			if (!magazineWellContainer)
				continue;

			typename well = magazineWellContainer.GetClassName().ToType();
			if (well)
				wells.Insert(well);
		}

		array<IEntityComponentSource> attachmentSlotSources = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(weaponContainer, AttachmentSlotComponent, true, attachmentSlotSources);

		foreach (IEntityComponentSource attachmentSlotSource : attachmentSlotSources)
		{
			BaseContainer attachmentSlot = attachmentSlotSource.GetObject("AttachmentSlot");
			if (!attachmentSlot)
				continue;

			ResourceName attachmentPrefab;
			if (!attachmentSlot.Get("Prefab", attachmentPrefab) || attachmentPrefab.IsEmpty())
				continue;

			GetWeaponMagazineWells(attachmentPrefab, wells, visited);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Like GetWeaponMagazineWells, but keeps the weapon's OWN MuzzleComponent well(s) (ownWells)
	//! separate from wells reachable only through an attachment (attachmentWells, e.g. an UGL's own
	//! well) - needed to split a combo weapon's mixed magazine array by which barrel each magazine
	//! actually belongs to, rather than just checking "does it fit somewhere on this weapon".
	protected void GetWeaponWellsSplit(ResourceName weaponPrefab, array<typename> ownWells, array<typename> attachmentWells)
	{
		if (weaponPrefab.IsEmpty())
			return;

		Resource resource = Resource.Load(weaponPrefab);
		if (!resource.IsValid())
			return;

		BaseContainer weaponContainer = resource.GetResource().ToBaseContainer();
		if (!weaponContainer)
			return;

		array<IEntityComponentSource> muzzleSources = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(weaponContainer, MuzzleComponent, true, muzzleSources);

		foreach (IEntityComponentSource muzzleSource : muzzleSources)
		{
			BaseContainer magazineWellContainer = muzzleSource.GetObject("MagazineWell");
			if (!magazineWellContainer)
				continue;

			typename well = magazineWellContainer.GetClassName().ToType();
			if (well)
				ownWells.Insert(well);
		}

		array<IEntityComponentSource> attachmentSlotSources = {};
		SCR_BaseContainerTools.FindComponentSourcesOfClass(weaponContainer, AttachmentSlotComponent, true, attachmentSlotSources);

		foreach (IEntityComponentSource attachmentSlotSource : attachmentSlotSources)
		{
			BaseContainer attachmentSlot = attachmentSlotSource.GetObject("AttachmentSlot");
			if (!attachmentSlot)
				continue;

			ResourceName attachmentPrefab;
			if (!attachmentSlot.Get("Prefab", attachmentPrefab) || attachmentPrefab.IsEmpty())
				continue;

			array<ResourceName> visited = {weaponPrefab};
			GetWeaponMagazineWells(attachmentPrefab, attachmentWells, visited);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected typename GetMagazineWell(ResourceName magazinePrefab)
	{
		Resource resource = Resource.Load(magazinePrefab);
		if (!resource.IsValid())
			return typename.Empty;

		IEntityComponentSource magazineComponentSource = SCR_BaseContainerTools.FindComponentSource(resource, MagazineComponent);
		if (!magazineComponentSource)
			return typename.Empty;

		BaseContainer magazineWellContainer = magazineComponentSource.GetObject("MagazineWell");
		if (!magazineWellContainer)
			return typename.Empty;

		return magazineWellContainer.GetClassName().ToType();
	}

	//------------------------------------------------------------------------------------------------
	//! Reads a magazine prefab's round capacity off its MagazineComponent's "MaxAmmo" attribute -
	//! same container key CRF_VehicleGearscriptManager.c's ComputeMagazineCount() already reads for
	//! vehicle magazines, confirmed against real magazine prefabs (e.g.
	//! Prefabs/Weapons/Magazines/BWAR_Box_762x51_MG5_60rnd_DM41_AP_low_weight.et has MaxAmmo 60).
	//! Returns 0 if the prefab has no MagazineComponent or didn't load, so callers can tell "no
	//! capacity data" apart from "genuinely empty".
	protected int GetMagazineCapacity(ResourceName magazinePrefab)
	{
		Resource resource = Resource.Load(magazinePrefab);
		if (!resource.IsValid())
			return 0;

		IEntityComponentSource magazineComponentSource = SCR_BaseContainerTools.FindComponentSource(resource, MagazineComponent);
		if (!magazineComponentSource)
			return 0;

		int maxAmmo;
		magazineComponentSource.Get("MaxAmmo", maxAmmo);
		return maxAmmo;
	}

	//------------------------------------------------------------------------------------------------
	//! Same trim-to-filename logic as CRF_MissionQAChecklistPlugin.c's GetFileName - used here purely
	//! as a short display label for a magazine/round entry, not for any naming validation.
	protected string GetFileName(string path)
	{
		int lastSlash = path.LastIndexOf("/");
		if (lastSlash == -1)
			return path;

		return path.Substring(lastSlash + 1, path.Length() - lastSlash - 1);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsLikelyLauncherAmmo(ResourceName magazine)
	{
		string upper = magazine.GetPath();
		upper.ToUpper();

		return upper.Contains("GRENADE")
			|| upper.Contains("FLARE")
			|| upper.Contains("SMOKE")
			|| upper.Contains("40MM")
			|| upper.Contains("VOG")
			|| upper.Contains("VG40")
			|| upper.Contains("HEDP")
			|| upper.Contains("ILLUM")
			|| upper.Contains("BUCKSHOT")
			|| upper.Contains("FLECHETTE");
	}

	//------------------------------------------------------------------------------------------------
	//! allowLauncherBypass must only be true for the "Rifle UGL" category - matching
	//! CRF_MissionQAChecklistPlugin.c's CheckWeaponAmmo, which gates the same bypass to
	//! `label == "Rifle UGL"` specifically. Applying it to any other category would let a
	//! genuinely wrong-caliber magazine (e.g. a grenade round on a plain Carbine slot) go
	//! unflagged just because its name contains a launcher-ammo keyword.
	protected void AppendWeaponAmmoIssues(array<string> lines, ResourceName weaponResource, array<ref COA_Magazine_Class> magazines, bool allowLauncherBypass = false)
	{
		if (weaponResource.IsEmpty() || !magazines)
			return;

		array<typename> weaponWells = {};
		array<ResourceName> visited = {};
		GetWeaponMagazineWells(weaponResource, weaponWells, visited);
		if (weaponWells.IsEmpty())
			return;

		foreach (COA_Magazine_Class magazine : magazines)
		{
			if (!magazine || magazine.m_Magazine.IsEmpty())
				continue;

			typename magazineWell = GetMagazineWell(magazine.m_Magazine);
			if (!magazineWell || weaponWells.Contains(magazineWell))
				continue;

			if (allowLauncherBypass && IsLikelyLauncherAmmo(magazine.m_Magazine))
				continue;

			lines.Insert(string.Format("  [X] Magazine %1 (%2) does not fit this weapon", magazine.m_Magazine, magazineWell.ToString()));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! See AppendWeaponAmmoIssues - allowLauncherBypass should stay false here, since no specialty
	//! role weapon (AR/MMG/HMG/AT/MAT/HAT/AA/Sniper) is ever the "Rifle UGL" category.
	protected void AppendSpecWeaponAmmoIssues(array<string> lines, ResourceName weaponResource, array<ref COA_Spec_Magazine_Class> magazines, bool allowLauncherBypass = false)
	{
		if (weaponResource.IsEmpty() || !magazines)
			return;

		array<typename> weaponWells = {};
		array<ResourceName> visited = {};
		GetWeaponMagazineWells(weaponResource, weaponWells, visited);
		if (weaponWells.IsEmpty())
			return;

		foreach (COA_Spec_Magazine_Class magazine : magazines)
		{
			if (!magazine || magazine.m_Magazine.IsEmpty())
				continue;

			typename magazineWell = GetMagazineWell(magazine.m_Magazine);
			if (!magazineWell || weaponWells.Contains(magazineWell))
				continue;

			if (allowLauncherBypass && IsLikelyLauncherAmmo(magazine.m_Magazine))
				continue;

			lines.Insert(string.Format("  [X] Magazine %1 (%2) does not fit this weapon", magazine.m_Magazine, magazineWell.ToString()));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Mission-summary rollup: total mismatch count across every weapon array in the gearscript,
	//! not scoped to any one role (used by AppendPerFactionRollup).
	protected int CountAmmoCompatibilityIssues(COA_GearScriptConfig gearConfig)
	{
		int count;
		count += CountWeaponArrayIssues(gearConfig.m_Rifles, false);
		count += CountWeaponArrayIssues(gearConfig.m_RifleUGLs, true);
		count += CountWeaponArrayIssues(gearConfig.m_Carbines, false);
		count += CountWeaponArrayIssues(gearConfig.m_Pistols, false);
		count += CountSpecWeaponIssues(gearConfig.m_AR);
		count += CountSpecWeaponIssues(gearConfig.m_MMG);
		count += CountSpecWeaponIssues(gearConfig.m_HMG);
		count += CountSpecWeaponIssues(gearConfig.m_AT);
		count += CountSpecWeaponIssues(gearConfig.m_MAT);
		count += CountSpecWeaponIssues(gearConfig.m_HAT);
		count += CountSpecWeaponIssues(gearConfig.m_AA);

		if (gearConfig.m_SNIPER)
		{
			array<string> lines = {};
			AppendWeaponAmmoIssues(lines, gearConfig.m_SNIPER.m_Weapon, gearConfig.m_SNIPER.m_MagazineArray);
			count += lines.Count();
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected int CountWeaponArrayIssues(array<ref COA_Weapon_Class> weapons, bool allowLauncherBypass)
	{
		if (!weapons)
			return 0;

		int count;
		foreach (COA_Weapon_Class weapon : weapons)
		{
			if (!weapon)
				continue;

			array<string> lines = {};
			AppendWeaponAmmoIssues(lines, weapon.m_Weapon, weapon.m_MagazineArray, allowLauncherBypass);
			count += lines.Count();
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected int CountSpecWeaponIssues(COA_Spec_Weapon_Class weapon)
	{
		if (!weapon)
			return 0;

		array<string> lines = {};
		AppendSpecWeaponAmmoIssues(lines, weapon.m_Weapon, weapon.m_MagazineArray);
		return lines.Count();
	}
}

//------------------------------------------------------------------------------------------------
//! One (faction, role) entry in the role list; index 0 in CRF_MissionQAMenu.m_aEntries is always
//! null, representing the "Mission Summary" sentinel row instead of a real role.
class CRF_MissionQARoleEntry
{
	string m_sFactionKey;
	COA_EGearRole m_eRole;
	int m_iSlotCount;
}
