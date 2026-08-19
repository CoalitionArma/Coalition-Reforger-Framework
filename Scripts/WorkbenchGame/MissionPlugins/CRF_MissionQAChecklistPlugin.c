#ifdef WORKBENCH
//------------------------------------------------------------------------------------------------
//! Runs a checklist of ORBAT, gear, radio and mission-settings checks against the mission as
//! currently edited/saved and reports what's missing, one section per faction. Unlike
//! CRF_MissionValidatorManager (which only runs after pressing Play, against the live game world)
//! this reads the World Editor's entity sources directly, so it works without ever entering Play
//! mode.
[WorkbenchPluginAttribute(
	name: "CRF Mission QA Checklist",
	description: "Runs ORBAT, gear, radio, resource-naming, briefing, mission config and respawn/ratio checks against the mission and reports what's missing",
	wbModules: { "WorldEditor" },
	category: "CRF Mission Plugins",
	awesomeFontCode: 0xF0AE)] // list-alt
class CRF_MissionQAChecklistPlugin : WorkbenchPlugin
{
	//! Leadership roles that satisfy the "squad has a leader" check.
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

	//! Slotting tiers COA_GearscriptManager.ApplyInventoryItems treats as eligible for a
	//! leadership-radio-driven short-range radio (mirrors its SHORTRANGE_RADIO branch exactly).
	protected static const ref array<COA_ESlotType> LEADERSHIP_SLOT_TYPES = {
		COA_ESlotType.TEAM_LEADER,
		COA_ESlotType.SQUAD_LEADER,
		COA_ESlotType.SPECIALTY,
		COA_ESlotType.SPECIALTY_ASSISTANT
	};

	//! Global roles config, used to read a role's item list/slotting tier (for the radio check) and
	//! display name. Loaded the same way COA_MissionSynopsisGenerator does - directly as a resource,
	//! not via a live manager singleton, since Workbench context can't guarantee one exists.
	protected static const ResourceName ROLES_CONFIG = "{4388548E9F600148}Configs/Gearscripts/COA_Global_Roles_Config.conf";

	//! Absolute path to this project's own root folder, computed once in Run(). Used by the
	//! resource-naming check to skip files that belong to COALITION-Lobby, the base game, or a
	//! third-party mod dependency - a mission maker can't rename those, so flagging them is just
	//! noise. Workbench resolves resource references across every loaded addon transparently, so
	//! there's no direct "which addon owns this file" API - this is derived by taking the
	//! currently open world's own absolute path (definitely a CRF file) and stripping its known
	//! resource-relative suffix off the end.
	protected string m_sProjectRoot;

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		if (!worldEditor)
			return;

		WorldEditorAPI api = worldEditor.GetApi();
		if (!api)
			return;

		string worldPath;
		api.GetWorldPath(worldPath);

		string absWorldPath;
		m_sProjectRoot = string.Empty;
		if (!worldPath.IsEmpty() && Workbench.GetAbsolutePath(worldPath, absWorldPath, false) && absWorldPath.Length() > worldPath.Length())
			m_sProjectRoot = absWorldPath.Substring(0, absWorldPath.Length() - worldPath.Length());

		IEntitySource entitySource = api.FindEntityByName("COA_Lobby");
		if (!entitySource)
		{
			SCR_WorkbenchHelper.PrintDialog("Could not find a COA_Lobby entity in this world - be sure it's loaded and saved before running the QA checklist.", "CRF Mission QA Checklist", LogLevel.WARNING);
			return;
		}

		COA_Gamemode gamemode = COA_Gamemode.Cast(api.SourceToEntity(entitySource));
		if (!gamemode)
		{
			SCR_WorkbenchHelper.PrintDialog("COA_Lobby entity did not resolve to a COA_Gamemode instance.", "CRF Mission QA Checklist", LogLevel.WARNING);
			return;
		}

		COA_RolesConfig rolesConfig = COA_RolesConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(ROLES_CONFIG).GetResource().ToBaseContainer()));

		array<string> factions = {};
		CRF_TestHelper.GetFactionKeys(factions);

		array<string> lines = {};
		int failCount, warnCount;
		bool anyFactionUsed;

		foreach (string factionKey : factions)
		{
			array<ref COA_SlottingGroup> slots = gamemode.GetSlots(factionKey);
			if (!slots || slots.IsEmpty())
				continue; // faction not used in this mission - not a fault

			anyFactionUsed = true;
			set<COA_EGearRole> slottedRoles = GetSlottedRoles(slots);

			lines.Insert(string.Format("## ORBAT - %1", factionKey));
			CheckORBAT(lines, factionKey, slots, gamemode, failCount, warnCount);
			lines.Insert("");

			lines.Insert(string.Format("## Gear - %1", factionKey));
			CheckGear(lines, factionKey, slottedRoles, gamemode, rolesConfig, failCount, warnCount);
			lines.Insert("");

			lines.Insert(string.Format("## Radios - %1", factionKey));
			CheckRadios(lines, factionKey, slottedRoles, gamemode, rolesConfig, failCount, warnCount);
			lines.Insert("");
		}

		if (!anyFactionUsed)
		{
			SCR_WorkbenchHelper.PrintDialog("No faction has any slots configured - nothing to check.", "CRF Mission QA Checklist", LogLevel.WARNING);
			return;
		}

		lines.Insert("## Mission Settings");
		CheckMissionConfig(lines, api, failCount);
		CheckFactionRatios(lines, gamemode, failCount, warnCount);
		CheckSpectatorVisibility(lines, gamemode, entitySource, failCount, warnCount);
		CheckBriefing(lines, gamemode, failCount, warnCount);
		CheckBriefingTypos(lines, gamemode, warnCount);
		lines.Insert("");

		array<string> summaryLines = {string.Format("%1 fail(s), %2 warning(s)", failCount, warnCount), ""};
		summaryLines.InsertAll(lines);

		CRF_MissionQAChecklistDialog dialog = new CRF_MissionQAChecklistDialog();
		dialog.m_sReport = SCR_StringHelper.Join("\n", summaryLines);
		Workbench.ScriptDialog("CRF Mission QA Checklist", "", dialog);
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 ORBAT
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks the placed ORBAT (squads/groups) for a faction: missing/duplicate callsigns, empty
	//! groups, squads with no leadership role, and zero-respawn pools.
	protected void CheckORBAT(array<string> lines, string factionKey, array<ref COA_SlottingGroup> slots, COA_Gamemode gamemode, inout int failCount, inout int warnCount)
	{
		bool slotBased = (gamemode.m_eRespawnMode == COA_ERespawnMode.SLOT);

		set<string> seenCallsigns = new set<string>();
		bool foundIssue;

		foreach (ref COA_SlottingGroup slotGroup : slots)
		{
			string callsign = slotGroup.m_sCallsign;

			if (callsign.IsEmpty())
			{
				lines.Insert(string.Format("[X] A group in %1 has no callsign/group name set", factionKey));
				failCount++;
				foundIssue = true;
			}
			else if (seenCallsigns.Contains(callsign))
			{
				lines.Insert(string.Format("[X] Duplicate callsign \"%1\" in %2 - two groups share this name", callsign, factionKey));
				failCount++;
				foundIssue = true;
			}
			else
			{
				seenCallsigns.Insert(callsign);
			}

			if (slotGroup.m_aSlots.IsEmpty())
			{
				lines.Insert(string.Format("[X] %1 has no roles assigned - empty group", DisplayCallsign(callsign)));
				failCount++;
				foundIssue = true;
				continue;
			}

			if (GetCallsignTier(callsign) != CRF_QAChecklistCallsignTier.OTHER && !HasLeadershipRole(slotGroup.m_aSlots))
			{
				lines.Insert(string.Format("[!] %1 has no leadership role (Squad Lead, Team Lead, etc)", DisplayCallsign(callsign)));
				warnCount++;
				foundIssue = true;
			}

			if (slotBased && slotGroup.m_eRespawnPoolType == COA_ERespawnPoolType.PER_GROUP && slotGroup.m_iGroupRespawns <= 0)
			{
				lines.Insert(string.Format("[!] %1 has a shared respawn pool with 0 respawns granted", DisplayCallsign(callsign)));
				warnCount++;
				foundIssue = true;
			}
		}

		if (!foundIssue)
			lines.Insert(string.Format("[OK] %1 ORBAT has no issues", factionKey));

		CheckTickets(lines, factionKey, gamemode, slotBased, failCount);
	}

	//------------------------------------------------------------------------------------------------
	//! Team-based respawn draws from the faction's ticket pool (m_iXXXTickets) instead of the
	//! per-squad pools slot-based respawn uses (already checked above) - a faction with active slots
	//! left at 0 tickets (COA_MissionFactionsPlugin's own field description: "0 = disabled") can't
	//! respawn at all under team-based respawn.
	protected void CheckTickets(array<string> lines, string factionKey, COA_Gamemode gamemode, bool slotBased, inout int failCount)
	{
		if (!gamemode.m_bRespawnEnabled || slotBased)
			return;

		if (GetFactionTickets(gamemode, factionKey) == 0)
		{
			lines.Insert(string.Format("[X] %1 has team-based respawn enabled but 0 tickets assigned (0 = disabled)", factionKey));
			failCount++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected int GetFactionTickets(COA_Gamemode gamemode, string factionKey)
	{
		switch (factionKey)
		{
			case "BLUFOR": return gamemode.m_iBLUFORTickets;
			case "OPFOR": return gamemode.m_iOPFORTickets;
			case "INDFOR": return gamemode.m_iINDFORTickets;
			case "CIV": return gamemode.m_iCIVTickets;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected string DisplayCallsign(string callsign)
	{
		if (callsign.IsEmpty())
			return "(unnamed group)";

		return string.Format("\"%1\"", callsign);
	}

	//------------------------------------------------------------------------------------------------
	//! True if any role in the group is a leadership role (COMPANY_COMMANDER, SQUAD_LEAD, etc).
	protected bool HasLeadershipRole(array<ref COA_EGearRole> roles)
	{
		foreach (COA_EGearRole role : roles)
		{
			if (LEADERSHIP_ROLES.Contains(role))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Same callsign-tier inference COA_MissionSynopsisGenerator.GetCallsignTier uses, kept local
	//! since it's a private display/heuristic concept, not shared API.
	protected CRF_QAChecklistCallsignTier GetCallsignTier(string callsign)
	{
		string upper = callsign;
		upper.ToUpper();

		if (upper == "COY")
			return CRF_QAChecklistCallsignTier.COMPANY;

		if (upper.IndexOf("PLT") != -1)
			return CRF_QAChecklistCallsignTier.PLATOON;

		if (upper.IndexOf("-") != -1)
			return CRF_QAChecklistCallsignTier.SQUAD;

		return CRF_QAChecklistCallsignTier.OTHER;
	}

	//------------------------------------------------------------------------------------------------
	protected set<COA_EGearRole> GetSlottedRoles(array<ref COA_SlottingGroup> slots)
	{
		set<COA_EGearRole> result = new set<COA_EGearRole>();

		foreach (ref COA_SlottingGroup slotGroup : slots)
			foreach (COA_EGearRole role : slotGroup.m_aSlots)
				result.Insert(role);

		return result;
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 GEAR
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks the faction's gearscript: assigned, resolvable, covers every role actually slotted,
	//! and (via CheckAmmoCompatibility) that every configured magazine fits its weapon.
	protected void CheckGear(array<string> lines, string factionKey, set<COA_EGearRole> slottedRoles, COA_Gamemode gamemode, COA_RolesConfig rolesConfig, inout int failCount, inout int warnCount)
	{
		ResourceName gearScriptResource = gamemode.GetGearScriptResource(factionKey);

		if (gearScriptResource.IsEmpty())
		{
			lines.Insert(string.Format("[X] No gearscript assigned to %1, which has active slots", factionKey));
			failCount++;
			return;
		}

		if (!CRF_TestHelper.CanLoadContainer(gearScriptResource))
		{
			lines.Insert(string.Format("[X] Gearscript %1 does not resolve (renamed/deleted config)", gearScriptResource));
			failCount++;
			return;
		}

		COA_GearScriptConfig gearScriptConfig = COA_GearScriptConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(BaseContainerTools.LoadContainer(gearScriptResource).GetResource().ToBaseContainer()));
		if (!gearScriptConfig)
		{
			lines.Insert(string.Format("[X] Gearscript %1 did not load as a COA_GearScriptConfig", gearScriptResource));
			failCount++;
			return;
		}

		bool foundIssue;

		if (gearScriptConfig.m_Rifles.IsEmpty())
		{
			lines.Insert(string.Format("[!] %1 gearscript has no rifles configured", factionKey));
			warnCount++;
			foundIssue = true;
		}

		if (gearScriptConfig.m_DefaultClothing.IsEmpty())
		{
			lines.Insert(string.Format("[!] %1 gearscript has no default clothing configured", factionKey));
			warnCount++;
			foundIssue = true;
		}

		foreach (COA_EGearRole role : slottedRoles)
		{
			bool weaponConfigured;

			if (role == COA_EGearRole.SNIPER) // m_SNIPER is a plain COA_Weapon_Class, not COA_Spec_Weapon_Class like the other specialty fields
			{
				weaponConfigured = gearScriptConfig.m_SNIPER && !gearScriptConfig.m_SNIPER.m_Weapon.IsEmpty();
			}
			else
			{
				COA_Spec_Weapon_Class specWeapon = GetSpecialtyWeapon(gearScriptConfig, role);
				if (!specWeapon)
					continue; // role has no dedicated weapon field to check (e.g. rifleman uses the general pool)

				weaponConfigured = !specWeapon.m_Weapon.IsEmpty();
			}

			if (!weaponConfigured)
			{
				lines.Insert(string.Format("[X] Role %1 is slotted but %2 gearscript has no weapon configured for it", GetRoleName(role, rolesConfig), factionKey));
				failCount++;
				foundIssue = true;
			}
		}

		if (!foundIssue)
			lines.Insert(string.Format("[OK] %1 gearscript resolves and covers every slotted role", factionKey));

		CheckMedicGear(lines, factionKey, slottedRoles, gearScriptConfig, failCount, warnCount);
		CheckAmmoCompatibility(lines, factionKey, gearScriptConfig, failCount, warnCount);
		CheckResourceNaming(lines, factionKey, gearScriptConfig, failCount);
	}

	//------------------------------------------------------------------------------------------------
	//! A slotted MEDIC needs medic-specific medical items configured, and specifically a Medical Kit
	//! among them - COA_GearscriptManager.ApplyInventoryItems only grants m_MedicMedicalItems to the
	//! MEDIC role (everyone else only gets m_InfantryMedicalItems, which is bandages/tourniquets - not
	//! enough to fully heal a teammate). Every gearscript in the repo identifies the healing-capable
	//! kit by its "MedicalKit" prefab path (e.g. Prefabs/Items/Equipment/Kits/MedicalKit_01/...),
	//! distinct from FieldDressing/Tourniquet/Morphine/SalineBag which only stop bleeding or self-aid.
	protected void CheckMedicGear(array<string> lines, string factionKey, set<COA_EGearRole> slottedRoles, COA_GearScriptConfig gearScriptConfig, inout int failCount, inout int warnCount)
	{
		if (!slottedRoles.Contains(COA_EGearRole.MEDIC))
			return; // no medic slotted in this faction - nothing to check

		if (gearScriptConfig.m_MedicMedicalItems.IsEmpty())
		{
			lines.Insert(string.Format("[X] Medic role is slotted in %1 but no medic medical items are configured", factionKey));
			failCount++;
			return;
		}

		bool hasHealingKit;
		foreach (COA_Inventory_Item item : gearScriptConfig.m_MedicMedicalItems)
		{
			if (!item || item.m_sItemPrefab.IsEmpty())
				continue;

			if (item.m_sItemPrefab.GetPath().Contains("MedicalKit"))
			{
				hasHealingKit = true;
				break;
			}
		}

		if (!hasHealingKit)
		{
			lines.Insert(string.Format("[X] Medic role is slotted in %1 but its medic items have no Medical Kit - medics won't be able to fully heal other players", factionKey));
			failCount++;
			return;
		}

		lines.Insert(string.Format("[OK] %1 medic role has medical items including a healing kit", factionKey));
	}

	//------------------------------------------------------------------------------------------------
	//! Maps a specialty COA_EGearRole to its dedicated weapon field on COA_GearScriptConfig.
	//! Returns null for roles that share the general weapon pool instead of having a dedicated
	//! field (e.g. RIFLEMAN, MEDIC), and for SNIPER, which the caller handles separately since
	//! m_SNIPER is a plain COA_Weapon_Class rather than COA_Spec_Weapon_Class.
	protected COA_Spec_Weapon_Class GetSpecialtyWeapon(COA_GearScriptConfig config, COA_EGearRole role)
	{
		switch (role)
		{
			case COA_EGearRole.AUTOMATIC_RIFLEMAN:
			case COA_EGearRole.ASSISTANT_AUTOMATIC_RIFLEMAN:
				return config.m_AR;

			case COA_EGearRole.MEDIUM_MACHINEGUN:
			case COA_EGearRole.ASSISTANT_MEDIUM_MACHINEGUN:
				return config.m_MMG;

			case COA_EGearRole.HEAVY_MACHINEGUN:
			case COA_EGearRole.ASSISTANT_HEAVY_MACHINEGUN:
				return config.m_HMG;

			case COA_EGearRole.RIFLEMAN_ANTITANK:
			case COA_EGearRole.ASSISTANT_RIFLEMAN_ANTITANK:
				return config.m_AT;

			case COA_EGearRole.MEDIUM_ANTITANK:
			case COA_EGearRole.ASSISTANT_MEDIUM_ANTITANK:
				return config.m_MAT;

			case COA_EGearRole.HEAVY_ANTITANK:
			case COA_EGearRole.ASSISTANT_HEAVY_ANTITANK:
				return config.m_HAT;

			case COA_EGearRole.ANTI_AIR:
			case COA_EGearRole.ASSISTANT_ANTI_AIR:
				return config.m_AA;
		}

		return null;
	}

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
//	 AMMO / MAGAZINE COMPATIBILITY
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks every weapon entry's magazine array against that weapon's actual magazine well(s), read
	//! off the same MuzzleComponent/MagazineComponent "MagazineWell" attribute the Resource Browser
	//! export tools (SCR_BIKIWeaponHelper/SCR_BIKIMagazineHelper) use - a magazine whose well doesn't
	//! match any of the weapon's wells is a round that will never load in-game.
	protected void CheckAmmoCompatibility(array<string> lines, string factionKey, COA_GearScriptConfig gearScriptConfig, inout int failCount, inout int warnCount)
	{
		bool foundIssue;

		if (CheckWeaponArrayAmmo(lines, factionKey, gearScriptConfig.m_Rifles, "Rifle", failCount))
			foundIssue = true;
		if (CheckWeaponArrayAmmo(lines, factionKey, gearScriptConfig.m_RifleUGLs, "Rifle UGL", failCount))
			foundIssue = true;
		if (CheckWeaponArrayAmmo(lines, factionKey, gearScriptConfig.m_Carbines, "Carbine", failCount))
			foundIssue = true;
		if (CheckWeaponArrayAmmo(lines, factionKey, gearScriptConfig.m_Pistols, "Pistol", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_AR, "AR", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_MMG, "MMG", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_HMG, "HMG", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_AT, "AT", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_MAT, "MAT", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_HAT, "HAT", failCount))
			foundIssue = true;
		if (CheckSpecWeaponAmmo(lines, factionKey, gearScriptConfig.m_AA, "AA", failCount))
			foundIssue = true;
		if (CheckWeaponAmmo(lines, factionKey, gearScriptConfig.m_SNIPER, "Sniper", failCount))
			foundIssue = true;

		if (!foundIssue)
			lines.Insert(string.Format("[OK] %1 magazines match their weapons' magazine wells", factionKey));
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckWeaponArrayAmmo(array<string> lines, string factionKey, array<ref COA_Weapon_Class> weapons, string label, inout int failCount)
	{
		if (!weapons)
			return false;

		bool foundIssue;
		foreach (COA_Weapon_Class weapon : weapons)
		{
			if (CheckWeaponAmmo(lines, factionKey, weapon, label, failCount))
				foundIssue = true;
		}

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckWeaponAmmo(array<string> lines, string factionKey, COA_Weapon_Class weapon, string label, inout int failCount)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty() || !weapon.m_MagazineArray)
			return false;

		array<typename> weaponWells = {};
		array<ResourceName> visitedWeapons = {};
		GetWeaponMagazineWells(weapon.m_Weapon, weaponWells, visitedWeapons);
		if (weaponWells.IsEmpty())
			return false; // weapon resource itself unresolved/has no magazine well - already caught elsewhere

		bool foundIssue;
		foreach (COA_Magazine_Class magazine : weapon.m_MagazineArray)
		{
			if (!magazine || magazine.m_Magazine.IsEmpty())
				continue;

			typename magazineWell = GetMagazineWell(magazine.m_Magazine);
			if (!magazineWell || weaponWells.Contains(magazineWell))
				continue;

			if (label == "Rifle UGL" && IsLikelyLauncherAmmo(magazine.m_Magazine))
				continue;

			lines.Insert(string.Format("[X] %1 %2: magazine %3 (%4) does not fit weapon %5 (%6)", factionKey, label, magazine.m_Magazine, magazineWell.ToString(), weapon.m_Weapon, JoinWellNames(weaponWells)));
			failCount++;
			foundIssue = true;
		}

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	//! A "Rifle UGL" weapon inherently has two magazine wells - the rifle's own and the underbarrel
	//! launcher's - and the launcher is very often a vanilla or third-party-mod attachment prefab
	//! (see GetWeaponMagazineWells) whose MagazineWell this plugin has repeatedly turned out unable
	//! to resolve, producing false "does not fit" reports for perfectly legitimate 40mm grenade/
	//! flare/smoke rounds. Rather than keep chasing exact vanilla prefab structure, a magazine that
	//! doesn't match a KNOWN well is allowed through here when its own name looks like launcher
	//! ammunition rather than a rifle-caliber round - a genuinely wrong-caliber rifle magazine won't
	//! match any of these and still gets flagged.
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
	protected bool CheckSpecWeaponAmmo(array<string> lines, string factionKey, COA_Spec_Weapon_Class weapon, string label, inout int failCount)
	{
		if (!weapon || weapon.m_Weapon.IsEmpty() || !weapon.m_MagazineArray)
			return false;

		array<typename> weaponWells = {};
		array<ResourceName> visitedWeapons = {};
		GetWeaponMagazineWells(weapon.m_Weapon, weaponWells, visitedWeapons);
		if (weaponWells.IsEmpty())
			return false;

		bool foundIssue;
		foreach (COA_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
		{
			if (!magazine || magazine.m_Magazine.IsEmpty())
				continue;

			typename magazineWell = GetMagazineWell(magazine.m_Magazine);
			if (!magazineWell || weaponWells.Contains(magazineWell))
				continue;

			lines.Insert(string.Format("[X] %1 %2: magazine %3 (%4) does not fit weapon %5 (%6)", factionKey, label, magazine.m_Magazine, magazineWell.ToString(), weapon.m_Weapon, JoinWellNames(weaponWells)));
			failCount++;
			foundIssue = true;
		}

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	//! Reads every magazine well reachable from a weapon prefab: its own MuzzleComponent(s), plus -
	//! recursively - every attachment reachable through an AttachmentSlotComponent. A combo weapon
	//! like a rifle with an UGL attached has two wells, but the UGL is a SEPARATE prefab plugged
	//! into an attachment slot, not a second MuzzleComponent embedded in the rifle's own file - so
	//! FindComponentSourcesOfClass(..., recursive: true) alone never sees it, since that only walks
	//! components already present on the ONE loaded container, not other resources it references by
	//! GUID. visited guards against a (theoretical) attachment cycle.
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
	protected string JoinWellNames(array<typename> wells)
	{
		string result;
		foreach (int i, typename well : wells)
		{
			if (i > 0)
				result = result + ", ";

			result = result + well.ToString();
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Reads a magazine prefab's magazine well type off its MagazineComponent, the same way
	//! SCR_BIKIMagazineHelper.GetMagazineWells does for the Resource Browser export tools.
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

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RESOURCE NAMING
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Walks every ResourceName the faction's gearscript references - the same set
	//! COA_ValidateGearScriptPlugin.ValidateGearScript walks to check loadability - and flags any
	//! whose FILENAME (not its containing directories, which are fine in-engine and not something a
	//! mission maker can fix anyway) has a space or a character outside [A-Za-z0-9_./-]. A space or
	//! odd character in a filename is a recurring source of breakage across engine tooling,
	//! packaging and command-line workflows even when the resource loads fine in Workbench.
	protected void CheckResourceNaming(array<string> lines, string factionKey, COA_GearScriptConfig gearScriptConfig, inout int failCount)
	{
		bool foundIssue;

		if (CheckResourceName(lines, factionKey, gearScriptConfig.m_FactionIcon, "Faction Icon", failCount))
			foundIssue = true;
		if (CheckResourceName(lines, factionKey, gearScriptConfig.m_sLeadershipBinocularsPrefab, "Leadership Binoculars", failCount))
			foundIssue = true;
		if (CheckResourceName(lines, factionKey, gearScriptConfig.m_sAssistantBinocularsPrefab, "Assistant Binoculars", failCount))
			foundIssue = true;
		if (CheckResourceName(lines, factionKey, gearScriptConfig.m_FactionIdentity, "Faction Identity", failCount))
			foundIssue = true;

		if (CheckWeaponArrayNaming(lines, factionKey, gearScriptConfig.m_Rifles, "Rifle", failCount))
			foundIssue = true;
		if (CheckWeaponArrayNaming(lines, factionKey, gearScriptConfig.m_RifleUGLs, "Rifle UGL", failCount))
			foundIssue = true;
		if (CheckWeaponArrayNaming(lines, factionKey, gearScriptConfig.m_Carbines, "Carbine", failCount))
			foundIssue = true;
		if (CheckWeaponArrayNaming(lines, factionKey, gearScriptConfig.m_Pistols, "Pistol", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_AR, "AR", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_MMG, "MMG", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_HMG, "HMG", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_AT, "AT", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_MAT, "MAT", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_HAT, "HAT", failCount))
			foundIssue = true;
		if (CheckSpecWeaponNaming(lines, factionKey, gearScriptConfig.m_AA, "AA", failCount))
			foundIssue = true;
		if (CheckWeaponNaming(lines, factionKey, gearScriptConfig.m_SNIPER, "Sniper", failCount))
			foundIssue = true;

		if (CheckClothingArrayNaming(lines, factionKey, gearScriptConfig.m_DefaultClothing, "Default Clothing", failCount))
			foundIssue = true;

		if (CheckInventoryArrayNaming(lines, factionKey, gearScriptConfig.m_DefaultInventoryItems, "Default Inventory Item", failCount))
			foundIssue = true;
		if (CheckInventoryArrayNaming(lines, factionKey, gearScriptConfig.m_InfantryMedicalItems, "Infantry Medical Item", failCount))
			foundIssue = true;
		if (CheckInventoryArrayNaming(lines, factionKey, gearScriptConfig.m_MedicMedicalItems, "Medic Medical Item", failCount))
			foundIssue = true;

		if (gearScriptConfig.m_RolesToSetCustomSettings)
		{
			foreach (COA_Role_Custom_Gear roleGear : gearScriptConfig.m_RolesToSetCustomSettings)
			{
				if (!roleGear)
					continue;

				string roleLabel = string.Format("Role '%1'", roleGear.m_sRoleName);
				if (CheckWeaponArrayNaming(lines, factionKey, roleGear.m_PrimaryWeapon, roleLabel + " Primary Weapon", failCount))
					foundIssue = true;
				if (CheckWeaponArrayNaming(lines, factionKey, roleGear.m_SecondaryWeapon, roleLabel + " Secondary Weapon", failCount))
					foundIssue = true;
				if (CheckWeaponArrayNaming(lines, factionKey, roleGear.m_Pistols, roleLabel + " Pistol", failCount))
					foundIssue = true;
				if (CheckClothingArrayNaming(lines, factionKey, roleGear.m_Clothing, roleLabel + " Clothing", failCount))
					foundIssue = true;
				if (CheckInventoryArrayNaming(lines, factionKey, roleGear.m_AdditionalInventoryItems, roleLabel + " Additional Inventory Item", failCount))
					foundIssue = true;
			}
		}

		if (!foundIssue)
			lines.Insert(string.Format("[OK] %1 gearscript resource paths are clean (no spaces/special characters)", factionKey));
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckWeaponArrayNaming(array<string> lines, string factionKey, array<ref COA_Weapon_Class> weapons, string label, inout int failCount)
	{
		if (!weapons)
			return false;

		bool foundIssue;
		foreach (COA_Weapon_Class weapon : weapons)
			if (CheckWeaponNaming(lines, factionKey, weapon, label, failCount))
				foundIssue = true;

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckWeaponNaming(array<string> lines, string factionKey, COA_Weapon_Class weapon, string label, inout int failCount)
	{
		if (!weapon)
			return false;

		bool foundIssue;
		if (CheckResourceName(lines, factionKey, weapon.m_Weapon, label + " Weapon", failCount))
			foundIssue = true;

		if (weapon.m_Attachments)
		{
			foreach (ResourceName attachment : weapon.m_Attachments)
				if (CheckResourceName(lines, factionKey, attachment, label + " Attachment", failCount))
					foundIssue = true;
		}

		if (weapon.m_MagazineArray)
		{
			foreach (COA_Magazine_Class magazine : weapon.m_MagazineArray)
				if (magazine && CheckResourceName(lines, factionKey, magazine.m_Magazine, label + " Magazine", failCount))
					foundIssue = true;
		}

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckSpecWeaponNaming(array<string> lines, string factionKey, COA_Spec_Weapon_Class weapon, string label, inout int failCount)
	{
		if (!weapon)
			return false;

		bool foundIssue;
		if (CheckResourceName(lines, factionKey, weapon.m_Weapon, label + " Weapon", failCount))
			foundIssue = true;

		if (weapon.m_Attachments)
		{
			foreach (ResourceName attachment : weapon.m_Attachments)
				if (CheckResourceName(lines, factionKey, attachment, label + " Attachment", failCount))
					foundIssue = true;
		}

		if (weapon.m_MagazineArray)
		{
			foreach (COA_Spec_Magazine_Class magazine : weapon.m_MagazineArray)
				if (magazine && CheckResourceName(lines, factionKey, magazine.m_Magazine, label + " Magazine", failCount))
					foundIssue = true;
		}

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckClothingArrayNaming(array<string> lines, string factionKey, array<ref COA_Clothing> clothingArray, string label, inout int failCount)
	{
		if (!clothingArray)
			return false;

		bool foundIssue;
		foreach (COA_Clothing clothing : clothingArray)
		{
			if (!clothing || !clothing.m_ClothingPrefabs)
				continue;

			foreach (ResourceName prefab : clothing.m_ClothingPrefabs)
				if (CheckResourceName(lines, factionKey, prefab, label, failCount))
					foundIssue = true;
		}

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	protected bool CheckInventoryArrayNaming(array<string> lines, string factionKey, array<ref COA_Inventory_Item> items, string label, inout int failCount)
	{
		if (!items)
			return false;

		bool foundIssue;
		foreach (COA_Inventory_Item item : items)
			if (item && CheckResourceName(lines, factionKey, item.m_sItemPrefab, label, failCount))
				foundIssue = true;

		return foundIssue;
	}

	//------------------------------------------------------------------------------------------------
	//! Checks the FILENAME only, not the directories it sits in - directory names (e.g. the
	//! "North America" folder under Configs/Identities) are fine in-engine and out of a mission
	//! maker's control anyway. Only the file's own name is what actually needs to be renamed. Also
	//! only checks files this project actually owns - see IsOwnedByThisProject.
	//! \return true if resourceName is non-empty, owned by this project, and its filename was
	//!         flagged (and a line inserted)
	protected bool CheckResourceName(array<string> lines, string factionKey, ResourceName resourceName, string label, inout int failCount)
	{
		if (resourceName.IsEmpty())
			return false;

		if (!IsOwnedByThisProject(resourceName))
			return false; // COALITION-Lobby / base-game / third-party mod asset - not ours to rename

		string path = resourceName.GetPath();
		string fileName = GetFileName(path);
		if (!HasUnsafeCharacters(fileName))
			return false;

		lines.Insert(string.Format("[X] %1 %2: \"%3\" - filename contains a space or special character", factionKey, label, path));
		failCount++;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! True if resourceName resolves to a file physically inside this project's own root folder
	//! (m_sProjectRoot), as opposed to COALITION-Lobby, the base game, or a third-party mod
	//! dependency - all of which resolve fine through Workbench's merged resource namespace but
	//! aren't files a CRF mission maker has any ability to rename.
	protected bool IsOwnedByThisProject(ResourceName resourceName)
	{
		if (resourceName.IsEmpty() || m_sProjectRoot.IsEmpty())
			return false;

		string absResourcePath;
		if (!Workbench.GetAbsolutePath(resourceName.GetPath(), absResourcePath, false))
			return false;

		return absResourcePath.StartsWith(m_sProjectRoot);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetFileName(string path)
	{
		int lastSlash = path.LastIndexOf("/");
		if (lastSlash == -1)
			return path;

		return path.Substring(lastSlash + 1, path.Length() - lastSlash - 1);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HasUnsafeCharacters(string fileName)
	{
		int length = fileName.Length();
		for (int i = 0; i < length; i++)
		{
			if (!IsPathCharSafe(fileName.ToAscii(i)))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Allows [A-Za-z0-9_./-] only - the safe subset for asset paths across engine tooling,
	//! packaging and command-line workflows.
	protected bool IsPathCharSafe(int asciiCode)
	{
		if (asciiCode >= 97 && asciiCode <= 122) // a-z
			return true;
		if (asciiCode >= 65 && asciiCode <= 90) // A-Z
			return true;
		if (asciiCode >= 48 && asciiCode <= 57) // 0-9
			return true;

		return asciiCode == 95  // _
			|| asciiCode == 45  // -
			|| asciiCode == 46  // .
			|| asciiCode == 47; // /
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 RADIOS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! Checks that every slotted role's radio, per the global roles config's item list, is actually
	//! backed by a configured prefab under this faction's current radio game mode settings. Mirrors
	//! COA_GearscriptManager.ApplyInventoryItems's SHORTRANGE_RADIO/LONGRANGE_RADIO/RTO_RADIO branches
	//! exactly, so a role that WOULD get a radio at runtime but has no prefab configured is caught
	//! here instead of a player spawning without one. Also flags an RTO role slotted while RTO radios
	//! are disabled outright - the role has no point without one.
	protected void CheckRadios(array<string> lines, string factionKey, set<COA_EGearRole> slottedRoles, COA_Gamemode gamemode, COA_RolesConfig rolesConfig, inout int failCount, inout int warnCount)
	{
		COA_GearScriptContainer gearContainer = gamemode.GetGearScriptSettings(factionKey);
		if (!gearContainer)
			return; // already flagged as "no gearscript assigned" by CheckGear

		if (!rolesConfig)
		{
			lines.Insert(string.Format("[!] %1 - could not load the global roles config, radio coverage not checked", factionKey));
			warnCount++;
			return;
		}

		bool foundIssue;

		foreach (COA_EGearRole role : slottedRoles)
		{
			COA_RoleConfig roleConfig = rolesConfig.FindRoleConfig(role);
			if (!roleConfig || !roleConfig.m_aItems)
				continue;

			bool isLeadershipSlot = LEADERSHIP_SLOT_TYPES.Contains(roleConfig.m_SlottingType);
			string roleName = GetRoleName(role, rolesConfig);

			if (roleConfig.m_aItems.Contains(COA_EGearscriptItems.SHORTRANGE_RADIO))
			{
				bool expectsShortRange = gearContainer.m_bEnableGIRadios || (gearContainer.m_bEnableLeadershipRadios && isLeadershipSlot);
				if (expectsShortRange && gearContainer.m_rShortRangeRadioPrefab.IsEmpty())
				{
					lines.Insert(string.Format("[X] %1 role %2 should get a short-range radio but none is configured", factionKey, roleName));
					failCount++;
					foundIssue = true;
				}
			}

			if (roleConfig.m_aItems.Contains(COA_EGearscriptItems.LONGRANGE_RADIO) && gearContainer.m_bEnableLeadershipRadios && gearContainer.m_rLongRangeRadioPrefab.IsEmpty())
			{
				lines.Insert(string.Format("[X] %1 role %2 should get a long-range radio but none is configured", factionKey, roleName));
				failCount++;
				foundIssue = true;
			}

			if (roleConfig.m_aItems.Contains(COA_EGearscriptItems.RTO_RADIO))
			{
				if (gearContainer.m_bEnableRTORadios && gearContainer.m_rRTORadiosPrefab.IsEmpty())
				{
					lines.Insert(string.Format("[X] %1 role %2 should get an RTO radio but none is configured", factionKey, roleName));
					failCount++;
					foundIssue = true;
				}
				else if (!gearContainer.m_bEnableRTORadios)
				{
					lines.Insert(string.Format("[!] %1 role %2 is slotted but RTO Radios are disabled for this faction - will spawn without a radio", factionKey, roleName));
					warnCount++;
					foundIssue = true;
				}
			}
		}

		if (!foundIssue)
			lines.Insert(string.Format("[OK] %1 radio settings cover every slotted role", factionKey));
	}

//=============================================================================================================================================================================================================================================================================================================================================================
//	 MISSION SETTINGS
//=============================================================================================================================================================================================================================================================================================================================================================

	//------------------------------------------------------------------------------------------------
	//! A mission with respawn (or an event-based respawn gamemode like Cache Hunt/Rush, which cycles
	//! players through spectator between lives/rounds even without the generic respawn toggle) should
	//! hide other factions in spectator - otherwise a dead/waiting player can freely scout the enemy.
	protected void CheckSpectatorVisibility(array<string> lines, COA_Gamemode gamemode, IEntitySource lobbyEntitySource, inout int failCount, inout int warnCount)
	{
		bool eventBasedRespawn = SCR_BaseContainerTools.FindComponentIndex(lobbyEntitySource, CRF_CacheHuntGamemodeManager) >= 0
			|| SCR_BaseContainerTools.FindComponentIndex(lobbyEntitySource, CRF_RushGamemodeManager) >= 0;

		if (!gamemode.m_bRespawnEnabled && !eventBasedRespawn)
		{
			lines.Insert("[OK] One-life mission (no respawn, no event-based respawn gamemode) - spectator visibility is not a concern");
			return;
		}

		if (gamemode.m_bHideOtherSpectatorFactions)
		{
			lines.Insert("[OK] Respawn/event-based respawn is active and other factions are hidden in spectator");
			return;
		}

		lines.Insert("[X] Respawn is enabled (or an event-based respawn gamemode like Cache Hunt/Rush is active) but \"Hide Other Spectator Factions\" is off - dead/spectating players can see and spectate the enemy faction");
		failCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! Every briefing entry (COA_Gamemode.m_aMissionDescriptors, shown in the in-game "Mission
	//! Description" panel) needs real text, and shouldn't still be the unedited template text the
	//! "Configure Descriptions" plugin pre-populates from m_aDefaultMissionDescriptors (COA_Lobby.et's
	//! "Insert your intent as the mission maker for this side" placeholders) - both mean players open
	//! the briefing to nothing useful.
	protected void CheckBriefing(array<string> lines, COA_Gamemode gamemode, inout int failCount, inout int warnCount)
	{
		if (!gamemode.m_aMissionDescriptors || gamemode.m_aMissionDescriptors.IsEmpty())
		{
			lines.Insert("[X] No mission briefing/descriptors are configured");
			failCount++;
			return;
		}

		bool foundIssue;
		foreach (ref COA_MissionDescriptor descriptor : gamemode.m_aMissionDescriptors)
		{
			if (!descriptor)
				continue;

			string title = descriptor.m_sTitle;
			if (title.IsEmpty())
				title = "(untitled)";

			if (descriptor.m_sTextData.IsEmpty())
			{
				lines.Insert(string.Format("[X] Briefing \"%1\" has no text", title));
				failCount++;
				foundIssue = true;
				continue;
			}

			if (IsDefaultDescriptorText(gamemode, descriptor))
			{
				lines.Insert(string.Format("[X] Briefing \"%1\" still has the default template text - fill it in", title));
				failCount++;
				foundIssue = true;
			}
		}

		if (!foundIssue)
			lines.Insert("[OK] Mission briefing is filled out");
	}

	//------------------------------------------------------------------------------------------------
	//! True if a live briefing entry's title+text is an exact, unedited copy of one of the entity's
	//! own default/template descriptors - reading the template off the entity itself (rather than a
	//! hardcoded copy of its text) means this keeps working if the COA_Lobby template ever changes.
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
	//! A generated mission config (.conf) is a separate resource from COA_Lobby itself - built by the
	//! "Generate Config File" plugin (COA_MissionConfigurationPlugin) into the same
	//! Missions/<TerrainName>/ folder that plugin computes the path to, with the currently open world's
	//! GUID written into its "World" property. Re-derives that same folder path and looks for any
	//! .conf in it whose World property actually resolves, rather than guessing the generated
	//! filename (which bakes in the author/date/mode/name typed into that plugin at generation time,
	//! none of which is recoverable here).
	protected void CheckMissionConfig(array<string> lines, WorldEditorAPI api, inout int failCount)
	{
		string worldPath;
		api.GetWorldPath(worldPath);

		array<string> pathParts = {};
		worldPath.Split("/", pathParts, false);
		if (pathParts.Count() < 2)
			return;

		string missionTerrain = pathParts.Get(pathParts.Count() - 2);

		string fileSystem = FilePath.FileSystemNameFromFileName(worldPath);
		fileSystem = SCR_AddonTool.ToFileSystem(fileSystem);
		string relativeDirPath = fileSystem + "Missions/" + missionTerrain;

		string absoluteDirPath;
		if (!Workbench.GetAbsolutePath(relativeDirPath, absoluteDirPath, true)) // true = must already exist
		{
			lines.Insert("[X] No mission config has been generated for this world - run the \"Generate Config File\" plugin first");
			failCount++;
			return;
		}

		array<string> configFiles = {};
		FileIO.FindFiles(configFiles.Insert, absoluteDirPath, ".conf");

		if (configFiles.IsEmpty())
		{
			lines.Insert("[X] No mission config has been generated for this world - run the \"Generate Config File\" plugin first");
			failCount++;
			return;
		}

		bool foundWorldAssigned;
		bool foundWorldResolves;
		foreach (string configFile : configFiles)
		{
			Resource resource = BaseContainerTools.LoadContainer(configFile);
			if (!resource || !resource.IsValid())
				continue;

			BaseContainer configContainer = resource.GetResource().ToBaseContainer();
			if (!configContainer)
				continue;

			ResourceName assignedWorld;
			if (!configContainer.Get("World", assignedWorld) || assignedWorld.IsEmpty())
				continue;

			foundWorldAssigned = true;

			Resource worldResource = Resource.Load(assignedWorld);
			if (worldResource && worldResource.IsValid())
			{
				foundWorldResolves = true;
				break;
			}
		}

		if (foundWorldResolves)
		{
			lines.Insert("[OK] Mission config found with the world file assigned");
			return;
		}

		if (foundWorldAssigned)
		{
			lines.Insert(string.Format("[X] Mission config(s) in %1 have a World field set, but it doesn't resolve to a loadable world - re-run \"Generate Config File\"", relativeDirPath));
			failCount++;
			return;
		}

		lines.Insert(string.Format("[X] Mission config(s) found in %1 but none has a world file assigned", relativeDirPath));
		failCount++;
	}

	//------------------------------------------------------------------------------------------------
	//! Mirrors CRF_MissionValidatorManager.ValidateGamemodeEntity's faction-ratio check, since that
	//! validator only runs after pressing Play - this lets a mission maker catch it before then.
	//! A ratio needs both a positive value AND an assigned faction key to actually do anything.
	protected void CheckFactionRatios(array<string> lines, COA_Gamemode gamemode, inout int failCount, inout int warnCount)
	{
		bool foundIssue;

		if (gamemode.m_iFactionOneRatio <= 0 && gamemode.m_iFactionTwoRatio <= 0)
		{
			lines.Insert("[X] Neither Faction Ratio is set - at least one must be greater than 0 for ratio-based (TVT) slotting to work");
			failCount++;
			foundIssue = true;
		}
		else
		{
			if (gamemode.m_iFactionOneRatio <= 0)
			{
				lines.Insert("[!] Faction One Ratio is 0 - set a ratio if this mission uses ratio-based slotting");
				warnCount++;
				foundIssue = true;
			}
			else if (gamemode.m_sFactionOneKey.IsEmpty())
			{
				lines.Insert("[X] Faction One Ratio is set but no Faction One Key is assigned");
				failCount++;
				foundIssue = true;
			}

			if (gamemode.m_iFactionTwoRatio <= 0)
			{
				lines.Insert("[!] Faction Two Ratio is 0 - set a ratio if this mission uses ratio-based slotting");
				warnCount++;
				foundIssue = true;
			}
			else if (gamemode.m_sFactionTwoKey.IsEmpty())
			{
				lines.Insert("[X] Faction Two Ratio is set but no Faction Two Key is assigned");
				failCount++;
				foundIssue = true;
			}
		}

		if (!foundIssue)
			lines.Insert(string.Format("[OK] Faction ratios set to %1 %2:%3 %4", gamemode.m_sFactionOneKey, gamemode.m_iFactionOneRatio, gamemode.m_iFactionTwoRatio, gamemode.m_sFactionTwoKey));
	}

	//------------------------------------------------------------------------------------------------
	//! NOT a real spellchecker - Enfusion Script has no dictionary/spellcheck API to call into. This
	//! is a heuristic "typo smell" pass: doubled words, double spaces, and leftover dev placeholder
	//! markers, which together catch the most common copy-paste and unfinished-edit mistakes without
	//! needing a bundled word list. Flags are a prompt to re-read the text, not a verdict.
	protected void CheckBriefingTypos(array<string> lines, COA_Gamemode gamemode, inout int warnCount)
	{
		if (!gamemode.m_aMissionDescriptors)
			return;

		bool foundIssue;
		foreach (ref COA_MissionDescriptor descriptor : gamemode.m_aMissionDescriptors)
		{
			if (!descriptor || descriptor.m_sTextData.IsEmpty())
				continue;

			string title = descriptor.m_sTitle;
			if (title.IsEmpty())
				title = "(untitled)";

			array<string> issues = {};
			FindTextIssues(descriptor.m_sTextData, issues);

			foreach (string issue : issues)
			{
				lines.Insert(string.Format("[!] Briefing \"%1\": %2", title, issue));
				warnCount++;
				foundIssue = true;
			}
		}

		if (!foundIssue)
			lines.Insert("[OK] No obvious typos found in the briefing (doubled words, double spaces, leftover placeholders)");
	}

	//------------------------------------------------------------------------------------------------
	protected void FindTextIssues(string text, array<string> issues)
	{
		array<string> words = {};
		text.Split(" ", words, true);

		string previousWordLower;
		foreach (string word : words)
		{
			string wordLower = word;
			wordLower.ToLower();

			if (!wordLower.IsEmpty() && wordLower == previousWordLower)
				issues.Insert(string.Format("doubled word \"%1\"", word));

			previousWordLower = wordLower;
		}

		if (text.Contains("  "))
			issues.Insert("contains a double space");

		string upperText = text;
		upperText.ToUpper();
		if (upperText.Contains("TODO") || upperText.Contains("FIXME") || upperText.Contains("TBD") || text.Contains("???") || text.Contains("XXX"))
			issues.Insert("contains a leftover placeholder marker (TODO/FIXME/TBD/???/XXX)");
	}
}

//------------------------------------------------------------------------------------------------
//! Callsign-inferred ORBAT tier, used only to decide whether a group is "squad-like enough" to
//! expect a leadership role. Kept local to the plugin - not a shared/runtime concept.
enum CRF_QAChecklistCallsignTier
{
	COMPANY,
	PLATOON,
	SQUAD,
	OTHER
}

//------------------------------------------------------------------------------------------------
//! Read-only result window for the QA checklist.
class CRF_MissionQAChecklistDialog
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline, desc: "QA checklist results")]
	string m_sReport;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Close", true)]
	protected bool ButtonClose()
	{
		return true;
	}
}
#endif
