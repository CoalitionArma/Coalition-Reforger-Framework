#!/usr/bin/env python3
"""
Static mission QA checks for Coalition addon projects.

A Python static re-implementation of most of the checks in
`Scripts/WorkbenchGame/MissionPlugins/CRF_MissionQAChecklistPlugin.c` (the
in-Workbench "CRF Mission QA Checklist" plugin), for `mission/<name>` branches
where nobody may have run that plugin before opening the PR.

This is NOT the plugin re-run headlessly - there is no headless Workbench.
It is an independent parser over the same underlying text files the plugin
reads through the Workbench API: a mission's placed `COA_Lobby`-derived
entity and its property overrides live inline, in plain text, in the world's
`<WorldName>_Layers/*.layer` files - the exact same brace-delimited container
format `.conf` files use. Referenced gearscripts, the global roles config,
and the generated mission `.conf` are the same format again.

Usage
-----
    python3 Tools/Validation/validate_mission.py --project ../COALITION-Lobby --world Worlds/AlHadra/FanServ_TVT_DroneZone.ent
    python3 Tools/Validation/validate_mission.py --project ../COALITION-Lobby --base-ref origin/release

Exits non-zero when any check reports a [X] failure. [!] warnings print but
do not fail the run, matching the Workbench plugin's own severity split.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
from validate_resources import read, rel  # noqa: E402

GUID_PATH_RE = re.compile(r"^\{([0-9A-Fa-f]{16})\}(.+)$")

# -----------------------------------------------------------------------------------------------
#  CONTAINER TEXT PARSER
#
#  Line-oriented, not a whitespace-agnostic token stream - the format has no punctuation
#  separating entries, only newlines (one entry per physical line, except for the
#  backslash-continued multi-line string case), so entry boundaries must be tracked per line.
# -----------------------------------------------------------------------------------------------

_LINE_TOKEN_RE = re.compile(r'"([^"]*)"|([^\s{}"]+)')


def _split_line_tokens(line: str) -> list[tuple[str, str]]:
    tokens = []
    for m in _LINE_TOKEN_RE.finditer(line):
        if m.group(1) is not None:
            tokens.append(("str", m.group(1)))
        else:
            tokens.append(("word", m.group(2)))
    return tokens


def _line_shape(raw_line: str) -> tuple[str, bool, bool]:
    """Returns (line_without_trailing_brace_or_backslash, ends_with_open_brace, ends_with_backslash)."""
    line = raw_line.strip()
    ends_open = False
    if line.endswith("{"):
        ends_open = True
        line = line[:-1].rstrip()
    ends_backslash = False
    if line.endswith("\\"):
        ends_backslash = True
        line = line[:-1].rstrip()
    return line, ends_open, ends_backslash


def parse_block(lines: list[str], i: int, n: int) -> tuple[list[dict], int]:
    """Parses entries starting at lines[i] until a line that is exactly '}' or EOF.

    Returns (entries, next_index). Each entry is {'tokens': [(kind, val), ...], 'block': [...]|None}.
    """
    entries: list[dict] = []
    while i < n:
        stripped = lines[i].strip()
        if stripped == "}":
            return entries, i + 1
        if not stripped:
            i += 1
            continue

        body, ends_open, ends_backslash = _line_shape(lines[i])
        tokens = _split_line_tokens(body)
        i += 1

        while ends_backslash and i < n:
            cont_body, cont_open, cont_backslash = _line_shape(lines[i])
            cont_tokens = _split_line_tokens(cont_body)
            i += 1
            addition = cont_tokens[0][1] if cont_tokens else ""
            if tokens and tokens[-1][0] == "str":
                tokens[-1] = ("str", tokens[-1][1] + "\n" + addition)
            else:
                tokens.append(("str", addition))
            ends_backslash = cont_backslash
            ends_open = cont_open

        block = None
        if ends_open:
            block, i = parse_block(lines, i, n)

        if tokens or block is not None:
            entries.append({"tokens": tokens, "block": block})

    return entries, i


def parse_container_text(text: str) -> list[dict]:
    lines = text.splitlines()
    entries, _ = parse_block(lines, 0, len(lines))
    return entries


def get_field(entries: list[dict], name: str) -> str | None:
    """A scalar `key value` or `key "string"` entry - exactly one value token, no block."""
    for e in entries:
        if e["tokens"] and e["tokens"][0] == ("word", name):
            if len(e["tokens"]) == 2 and e["block"] is None:
                return e["tokens"][1][1]
            return None
    return None


def get_block(entries: list[dict], name: str) -> tuple[str | None, str | None, list[dict] | None]:
    """A `key [ClassName] ["{GUID}"] { ... }` entry. Returns (classname, guid, sub_entries)."""
    for e in entries:
        if e["tokens"] and e["tokens"][0] == ("word", name):
            classname = e["tokens"][1][1] if len(e["tokens"]) >= 2 else None
            guid = e["tokens"][2][1] if len(e["tokens"]) >= 3 else None
            return classname, guid, e["block"]
    return None, None, None


def get_all_instances(entries: list[dict], class_name: str | None = None) -> list[tuple[str, str | None, list[dict]]]:
    """Positional child instances of a block, e.g. every COA_SlottingGroup under m_BLUFORSlots."""
    result = []
    for e in entries:
        if not e["tokens"]:
            continue
        cname = e["tokens"][0][1]
        if class_name and cname != class_name:
            continue
        guid = e["tokens"][1][1] if len(e["tokens"]) >= 2 else None
        result.append((cname, guid, e["block"] or []))
    return result


def get_bare_tokens(entries: list[dict]) -> list[str]:
    """Plain enum-array contents, e.g. m_aSlots { COMPANY_COMMANDER MEDIC ... }."""
    return [e["tokens"][0][1] for e in entries if len(e["tokens"]) == 1 and e["block"] is None]


def find_all_recursive(entries: list[dict], class_name: str) -> list[dict]:
    """Every entry anywhere in the tree (any depth) whose leading token is class_name."""
    result = []
    for e in entries:
        if e["tokens"] and e["tokens"][0][1] == class_name:
            result.append(e)
        if e["block"]:
            result.extend(find_all_recursive(e["block"], class_name))
    return result


def find_entity_by_ancestor(top_level_entries: list[dict], ancestor_suffix: str) -> list[dict] | None:
    """The block of the first top-level entity whose prefab ancestor path ends with ancestor_suffix."""
    for e in top_level_entries:
        for kind, val in e["tokens"]:
            if kind == "str" and val.endswith(ancestor_suffix):
                return e["block"] or []
    return None


def split_guid_path(resource: str) -> tuple[str | None, str]:
    m = GUID_PATH_RE.match(resource)
    if m:
        return m.group(1), m.group(2)
    return None, resource


def resolve_resource_path(resource: str, roots: list[str]) -> str | None:
    """Absolute path to `resource` if its path component exists under any scanned root."""
    if not resource:
        return None
    _, path = split_guid_path(resource)
    for root in roots:
        candidate = os.path.join(root, path)
        if os.path.isfile(candidate):
            return candidate
    return None


# -----------------------------------------------------------------------------------------------
#  MISSION LOCATION
# -----------------------------------------------------------------------------------------------

FACTIONS = ("BLUFOR", "OPFOR", "INDFOR", "CIV")
SLOTS_FIELD = {"BLUFOR": "m_BLUFORSlots", "OPFOR": "m_OPFORSlots", "INDFOR": "m_INDFORSlots", "CIV": "m_CIVSlots"}
TICKETS_FIELD = {"BLUFOR": "m_iBLUFORTickets", "OPFOR": "m_iOPFORTickets", "INDFOR": "m_iINDFORTickets", "CIV": "m_iCIVTickets"}
GEARSCRIPT_FIELD = {
    "BLUFOR": "m_BLUFORGearScriptSettings",
    "OPFOR": "m_OPFORGearScriptSettings",
    "INDFOR": "m_INDFORGearScriptSettings",
    "CIV": "m_CIVILIANGearScriptSettings",
}

# COA_Gamemode's own [Attribute("default", ...)] declarations, for fields a mission's .layer
# (and COA_Lobby.et itself) may never override. Keep in sync with COA_Gamemode.c if it changes.
CLASS_DEFAULTS = {
    "m_bRespawnEnabled": "0",
    "m_bHideOtherSpectatorFactions": "0",
    "m_iFactionOneRatio": "0",
    "m_iFactionTwoRatio": "0",
    "m_sFactionOneKey": "",
    "m_sFactionTwoKey": "",
    "m_eRespawnMode": "0",  # TEAM
    "m_iBLUFORTickets": "0",
    "m_iOPFORTickets": "0",
    "m_iINDFORTickets": "0",
    "m_iCIVTickets": "0",
}


def find_mission_entries(ent_path: str) -> tuple[list[dict], str] | None:
    """Locate the COA_Lobby-derived entity for a world. Returns (entries, layer_file_path) or None."""
    layers_dir = ent_path[: -len(".ent")] + "_Layers" if ent_path.endswith(".ent") else ent_path + "_Layers"
    if not os.path.isdir(layers_dir):
        return None

    for name in sorted(os.listdir(layers_dir)):
        if not name.endswith(".layer"):
            continue
        path = os.path.join(layers_dir, name)
        top = parse_container_text(read(path))
        entries = find_entity_by_ancestor(top, "COA_Lobby.et")
        if entries is not None:
            return entries, path
    return None


def resolve_field(mission: list[dict], lobby_default: list[dict] | None, name: str) -> str | None:
    """Mission override -> COA_Lobby.et prefab default -> hardcoded class default."""
    value = get_field(mission, name)
    if value is not None:
        return value
    if lobby_default is not None:
        value = get_field(lobby_default, name)
        if value is not None:
            return value
    return CLASS_DEFAULTS.get(name)


# -----------------------------------------------------------------------------------------------
#  REPORT
# -----------------------------------------------------------------------------------------------


class Report:
    def __init__(self) -> None:
        self.lines: list[str] = []
        self.fail_count = 0
        self.warn_count = 0

    def section(self, title: str) -> None:
        self.lines.append("")
        self.lines.append(f"## {title}")

    def ok(self, msg: str) -> None:
        self.lines.append(f"[OK] {msg}")

    def fail(self, msg: str) -> None:
        self.lines.append(f"[X] {msg}")
        self.fail_count += 1

    def warn(self, msg: str) -> None:
        self.lines.append(f"[!] {msg}")
        self.warn_count += 1

    def info(self, msg: str) -> None:
        self.lines.append(f"[-] {msg}")


# -----------------------------------------------------------------------------------------------
#  ORBAT + TICKETS
# -----------------------------------------------------------------------------------------------

LEADERSHIP_ROLES = {
    "COMPANY_COMMANDER", "FIRST_SERGEANT", "PLATOON_LEADER", "PLATOON_SERGEANT",
    "SQUAD_LEAD", "TEAM_LEAD", "VEHICLE_LEAD", "INDIRECT_LEAD", "LOGI_LEAD",
}


def callsign_tier(callsign: str) -> str:
    upper = callsign.upper()
    if upper == "COY":
        return "COMPANY"
    if "PLT" in upper:
        return "PLATOON"
    if "-" in upper:
        return "SQUAD"
    return "OTHER"


def check_orbat(report: Report, faction: str, slots: list[dict], respawn_mode: str) -> set[str]:
    report.section(f"ORBAT - {faction}")
    slot_based = respawn_mode == "1" or respawn_mode.upper() == "SLOT"
    seen_callsigns: set[str] = set()
    all_roles: set[str] = set()
    found_issue = False

    for cname, guid, group in get_all_instances(slots, "COA_SlottingGroup"):
        callsign = get_field(group, "m_sCallsign") or ""
        display = f'"{callsign}"' if callsign else "(unnamed group)"

        if not callsign:
            report.fail(f"A group in {faction} has no callsign/group name set")
            found_issue = True
        elif callsign in seen_callsigns:
            report.fail(f'Duplicate callsign "{callsign}" in {faction} - two groups share this name')
            found_issue = True
        else:
            seen_callsigns.add(callsign)

        _, _, roles_block = get_block(group, "m_aSlots")
        roles = get_bare_tokens(roles_block) if roles_block is not None else []
        all_roles.update(roles)

        if not roles:
            report.fail(f"{display} has no roles assigned - empty group")
            found_issue = True
            continue

        if callsign_tier(callsign) != "OTHER" and not (set(roles) & LEADERSHIP_ROLES):
            report.warn(f"{display} has no leadership role (Squad Lead, Team Lead, etc)")
            found_issue = True

        if slot_based:
            pool_type = get_field(group, "m_eRespawnPoolType")
            group_respawns = get_field(group, "m_iGroupRespawns")
            if pool_type == "PER_GROUP" and group_respawns is not None:
                try:
                    if int(group_respawns) == 0:
                        report.warn(f"{display} has a shared respawn pool with 0 respawns granted")
                        found_issue = True
                except ValueError:
                    pass

    if not found_issue:
        report.ok(f"{faction} ORBAT has no issues")

    return all_roles


# -----------------------------------------------------------------------------------------------
#  MAIN
# -----------------------------------------------------------------------------------------------


def check_tickets(report: Report, faction: str, mission: list[dict], lobby_default: list[dict] | None, respawn_enabled: bool, respawn_mode: str) -> None:
    slot_based = respawn_mode == "1" or respawn_mode.upper() == "SLOT"
    if not respawn_enabled or slot_based:
        return
    tickets = resolve_field(mission, lobby_default, TICKETS_FIELD[faction])
    try:
        if tickets is not None and int(tickets) == 0:
            report.fail(f"{faction} has team-based respawn enabled but 0 tickets assigned (0 = disabled)")
    except ValueError:
        pass


def get_specialty_field(role: str) -> str | None:
    return {
        "AUTOMATIC_RIFLEMAN": "m_AR", "ASSISTANT_AUTOMATIC_RIFLEMAN": "m_AR",
        "MEDIUM_MACHINEGUN": "m_MMG", "ASSISTANT_MEDIUM_MACHINEGUN": "m_MMG",
        "HEAVY_MACHINEGUN": "m_HMG", "ASSISTANT_HEAVY_MACHINEGUN": "m_HMG",
        "RIFLEMAN_ANTITANK": "m_AT", "ASSISTANT_RIFLEMAN_ANTITANK": "m_AT",
        "MEDIUM_ANTITANK": "m_MAT", "ASSISTANT_MEDIUM_ANTITANK": "m_MAT",
        "HEAVY_ANTITANK": "m_HAT", "ASSISTANT_HEAVY_ANTITANK": "m_HAT",
        "ANTI_AIR": "m_AA", "ASSISTANT_ANTI_AIR": "m_AA",
    }.get(role)


def load_gearscript_config(resource: str, roots: list[str]) -> list[dict] | None:
    path = resolve_resource_path(resource, roots)
    if not path:
        return None
    top = parse_container_text(read(path))
    for e in top:
        if e["tokens"] and e["tokens"][0][1] == "COA_GearScriptConfig":
            return e["block"]
    return None


def check_gear(report: Report, faction: str, mission: list[dict], roles: set[str], roots: list[str], crf_root: str) -> tuple[list[dict] | None, str | None]:
    report.section(f"Gear - {faction}")
    _, _, gs_container = get_block(mission, GEARSCRIPT_FIELD[faction])
    gearscript_resource = get_field(gs_container, "m_rGearScript") if gs_container else None

    if not gearscript_resource:
        report.fail(f"No gearscript assigned to {faction}, which has active slots")
        return None, None

    path = resolve_resource_path(gearscript_resource, roots)
    if not path:
        report.info(f"{faction} gearscript {gearscript_resource} not found in scanned project roots - not verifiable in CI")
        return None, gearscript_resource

    config = load_gearscript_config(gearscript_resource, roots)
    if config is None:
        report.fail(f"Gearscript {gearscript_resource} did not parse as a COA_GearScriptConfig")
        return None, gearscript_resource

    found_issue = False

    _, _, rifles = get_block(config, "m_Rifles")
    if not rifles:
        report.warn(f"{faction} gearscript has no rifles configured")
        found_issue = True

    _, _, clothing = get_block(config, "m_DefaultClothing")
    if not clothing:
        report.warn(f"{faction} gearscript has no default clothing configured")
        found_issue = True

    for role in sorted(roles):
        if role == "SNIPER":
            _, _, sniper = get_block(config, "m_SNIPER")
            weapon = get_field(sniper, "m_Weapon") if sniper else None
            if not weapon:
                report.fail(f"Role SNIPER is slotted but {faction} gearscript has no weapon configured for it")
                found_issue = True
            continue

        field = get_specialty_field(role)
        if not field:
            continue
        _, _, spec = get_block(config, field)
        weapon = get_field(spec, "m_Weapon") if spec else None
        if not weapon:
            report.fail(f"Role {role} is slotted but {faction} gearscript has no weapon configured for it")
            found_issue = True

    if not found_issue:
        report.ok(f"{faction} gearscript resolves and covers every slotted role")

    check_medic_gear(report, faction, roles, config)
    check_ammo_compatibility(report, faction, config, roots)
    check_resource_naming(report, faction, config, crf_root)

    return config, gearscript_resource


def check_medic_gear(report: Report, faction: str, roles: set[str], config: list[dict]) -> None:
    if "MEDIC" not in roles:
        return
    _, _, items = get_block(config, "m_MedicMedicalItems")
    if not items:
        report.fail(f"Medic role is slotted in {faction} but no medic medical items are configured")
        return
    has_kit = False
    for cname, guid, sub in get_all_instances(items):
        prefab = get_field(sub, "m_sItemPrefab") or ""
        if "MedicalKit" in prefab:
            has_kit = True
            break
    if not has_kit:
        report.fail(f"Medic role is slotted in {faction} but its medic items have no Medical Kit - medics won't be able to fully heal other players")
        return
    report.ok(f"{faction} medic role has medical items including a healing kit")


# --- resource naming ---------------------------------------------------------------------------

_SAFE_FILENAME_RE = re.compile(r"^[A-Za-z0-9_.-]+$")


def check_resource_naming(report: Report, faction: str, config: list[dict], crf_root: str) -> None:
    """Checks each resource's FILENAME only, not the directories it sits in - directory names
    (e.g. the "North America" folder under Configs/Identities) are fine in-engine and not
    something a mission maker can fix anyway. Only the file's own name is worth flagging.

    Also only checks resources that actually resolve inside the CRF project root specifically -
    not COALITION-Lobby, not any other --project root, not anything absent from both (a
    third-party mod dependency). A mission maker can't rename a file that isn't theirs to rename,
    so flagging one is just noise nobody can act on."""
    found_issue = False
    seen: set[str] = set()

    def visit(entries: list[dict]) -> None:
        nonlocal found_issue
        for e in entries:
            for kind, val in e["tokens"]:
                if kind == "str" and GUID_PATH_RE.match(val):
                    _, path = split_guid_path(val)
                    if path in seen:
                        continue
                    seen.add(path)
                    if not resolve_resource_path(val, [crf_root]):
                        continue  # not a CRF-owned file - nothing we can fix here
                    filename = path.rsplit("/", 1)[-1]
                    if not _SAFE_FILENAME_RE.match(filename):
                        report.fail(f'{faction}: "{path}" - filename contains a space or special character')
                        found_issue = True
            if e["block"]:
                visit(e["block"])

    visit(config)
    if not found_issue:
        report.ok(f"{faction} gearscript resource filenames are clean (no spaces/special characters, for files this project owns)")


# --- ammo compatibility -------------------------------------------------------------------------

WEAPON_ARRAY_FIELDS = [("m_Rifles", "Rifle"), ("m_RifleUGLs", "Rifle UGL"), ("m_Carbines", "Carbine"), ("m_Pistols", "Pistol")]
SPEC_WEAPON_FIELDS = [("m_AR", "AR"), ("m_MMG", "MMG"), ("m_HMG", "HMG"), ("m_AT", "AT"), ("m_MAT", "MAT"), ("m_HAT", "HAT"), ("m_AA", "AA")]


def get_attachment_prefabs(entries: list[dict]) -> list[str]:
    """Every `AttachmentSlotComponent { AttachmentSlot ... { Prefab "{GUID}..." } }` reference
    anywhere in the tree - e.g. a rifle's UGL/underbarrel slot, which is a separate prefab
    entirely rather than a component embedded in the rifle's own .et."""
    result = []
    for comp in find_all_recursive(entries, "AttachmentSlotComponent"):
        _, _, slot = get_block(comp["block"] or [], "AttachmentSlot")
        if slot is not None:
            prefab = get_field(slot, "Prefab")
            if prefab:
                result.append(prefab)
    return result


def _entity_block_and_ancestor(path: str) -> tuple[list[dict], str | None]:
    top = parse_container_text(read(path))
    if not top:
        return [], None
    ancestor = None
    for kind, val in top[0]["tokens"]:
        if kind == "str" and val.endswith(".et"):
            ancestor = val
    return top[0]["block"] or [], ancestor


def collect_magazine_wells(resource: str, roots: list[str], visited: set[str] | None = None) -> tuple[list[str], bool]:
    """Every magazine well a weapon accepts - from its own MuzzleComponent(s), its prefab
    ancestor chain (a variant .et often doesn't redeclare a well its base prefab already sets),
    and every attached prefab reachable through an AttachmentSlotComponent (a UGL is a separate
    prefab, not a second MuzzleComponent embedded in the rifle's own file).

    Returns (wells, fully_resolved). fully_resolved is False when any ancestor/attachment
    reference along the way couldn't be found in a scanned root - meaning the real well set may
    be larger than what's returned, so an unmatched magazine should be treated as unverifiable
    rather than a confirmed mismatch.
    """
    if visited is None:
        visited = set()

    path = resolve_resource_path(resource, roots)
    if path is None:
        return [], False
    if path in visited:
        return [], True
    visited.add(path)

    entity_block, ancestor = _entity_block_and_ancestor(path)

    wells: list[str] = []
    for muzzle in find_all_recursive(entity_block, "MuzzleComponent"):
        for w in find_all_recursive(muzzle["block"] or [], "MagazineWell"):
            if len(w["tokens"]) >= 2:
                wells.append(w["tokens"][1][1])

    fully_resolved = True

    if ancestor:
        ancestor_wells, ancestor_resolved = collect_magazine_wells(ancestor, roots, visited)
        wells.extend(ancestor_wells)
        fully_resolved = fully_resolved and ancestor_resolved

    for prefab in get_attachment_prefabs(entity_block):
        prefab_wells, prefab_resolved = collect_magazine_wells(prefab, roots, visited)
        wells.extend(prefab_wells)
        fully_resolved = fully_resolved and prefab_resolved

    return wells, fully_resolved


def collect_magazine_component_well(resource: str, roots: list[str], visited: set[str] | None = None) -> tuple[str | None, bool]:
    """A magazine's own well, following its prefab ancestor chain the same way (a tracer/variant
    magazine .et often just extends a base magazine .et without redeclaring MagazineWell)."""
    if visited is None:
        visited = set()

    path = resolve_resource_path(resource, roots)
    if path is None:
        return None, False
    if path in visited:
        return None, True
    visited.add(path)

    entity_block, ancestor = _entity_block_and_ancestor(path)

    for mc in find_all_recursive(entity_block, "MagazineComponent"):
        for w in find_all_recursive(mc["block"] or [], "MagazineWell"):
            if len(w["tokens"]) >= 2:
                return w["tokens"][1][1], True

    if ancestor:
        return collect_magazine_component_well(ancestor, roots, visited)

    return None, True


LAUNCHER_AMMO_KEYWORDS = ("GRENADE", "FLARE", "SMOKE", "40MM", "VOG", "VG40", "HEDP", "ILLUM", "BUCKSHOT", "FLECHETTE")


def is_likely_launcher_ammo(magazine_resource: str) -> bool:
    """A "Rifle UGL" weapon inherently has two magazine wells - the rifle's own and the
    underbarrel launcher's - and the launcher is very often a vanilla/third-party-mod attachment
    prefab whose MagazineWell can't be resolved from these two repos, producing a false "does not
    fit" for perfectly legitimate 40mm grenade/flare/smoke rounds. A magazine that doesn't match a
    KNOWN well is allowed through when its own name looks like launcher ammunition rather than a
    rifle-caliber round - a genuinely wrong-caliber rifle magazine won't match any of these.
    """
    _, path = split_guid_path(magazine_resource)
    upper = path.upper()
    return any(keyword in upper for keyword in LAUNCHER_AMMO_KEYWORDS)


def check_weapon_ammo(report: Report, faction: str, label: str, weapon_entries: list[dict], roots: list[str], not_verifiable: list[str]) -> bool:
    weapon_resource = get_field(weapon_entries, "m_Weapon")
    if not weapon_resource:
        return False
    _, _, mags = get_block(weapon_entries, "m_MagazineArray")
    if not mags:
        return False

    wells, fully_resolved = collect_magazine_wells(weapon_resource, roots)
    if not wells:
        if not fully_resolved:
            not_verifiable.append(f"{faction} {label} weapon {weapon_resource}")
        return False

    found_issue = False
    for cname, guid, mag in get_all_instances(mags):
        mag_resource = get_field(mag, "m_Magazine")
        if not mag_resource:
            continue
        mag_well, mag_resolved = collect_magazine_component_well(mag_resource, roots)
        if mag_well is None:
            if not mag_resolved:
                not_verifiable.append(f"{faction} {label} magazine {mag_resource}")
            continue
        if mag_well in wells:
            continue
        if label == "Rifle UGL" and is_likely_launcher_ammo(mag_resource):
            continue
        if not fully_resolved:
            # The weapon has an attachment/ancestor we couldn't resolve (e.g. a third-party-mod
            # UGL) - it may declare exactly the well this magazine needs, so this isn't a
            # confirmed mismatch, just something CI can't verify.
            not_verifiable.append(f"{faction} {label} magazine {mag_resource} (weapon has an unresolved attachment/ancestor)")
            continue
        report.fail(
            f"{faction} {label}: magazine {mag_resource} ({mag_well}) does not fit weapon "
            f"{weapon_resource} ({', '.join(wells)})"
        )
        found_issue = True
    return found_issue


def check_ammo_compatibility(report: Report, faction: str, config: list[dict], roots: list[str]) -> None:
    found_issue = False
    not_verifiable: list[str] = []

    for field, label in WEAPON_ARRAY_FIELDS:
        _, _, weapons = get_block(config, field)
        if weapons:
            for cname, guid, weapon in get_all_instances(weapons):
                if check_weapon_ammo(report, faction, label, weapon, roots, not_verifiable):
                    found_issue = True

    for field, label in SPEC_WEAPON_FIELDS:
        _, _, weapon = get_block(config, field)
        if weapon and check_weapon_ammo(report, faction, label, weapon, roots, not_verifiable):
            found_issue = True

    _, _, sniper = get_block(config, "m_SNIPER")
    if sniper and check_weapon_ammo(report, faction, "Sniper", sniper, roots, not_verifiable):
        found_issue = True

    if not_verifiable:
        unique = sorted(set(not_verifiable))
        report.info(f"{faction} ammo compatibility not verifiable in CI for: {'; '.join(unique[:5])}" + (f" and {len(unique) - 5} more" if len(unique) > 5 else ""))

    if not found_issue:
        report.ok(f"{faction} magazines match their weapons' magazine wells (for weapons found in scanned roots)")


# --- radios --------------------------------------------------------------------------------------

LEADERSHIP_SLOT_TYPES = {"TEAM_LEADER", "SQUAD_LEADER", "SPECIALTY", "SPECIALTY_ASSISTANT"}


def load_roles_config(roots: list[str]) -> list[dict] | None:
    for root in roots:
        candidate = os.path.join(root, "Configs", "Gearscripts", "COA_Global_Roles_Config.conf")
        if os.path.isfile(candidate):
            top = parse_container_text(read(candidate))
            for e in top:
                if e["tokens"] and e["tokens"][0][1] == "COA_RolesConfig":
                    return e["block"]
    return None


def find_role_config(roles_config: list[dict], role: str) -> list[dict] | None:
    _, _, role_configs = get_block(roles_config, "m_RoleConfigs")
    if not role_configs:
        return None
    for cname, guid, sub in get_all_instances(role_configs, "COA_RoleConfig"):
        if get_field(sub, "m_Role") == role:
            return sub
    return None


def check_radios(report: Report, faction: str, mission: list[dict], roles: set[str], roles_config: list[dict] | None) -> None:
    report.section(f"Radios - {faction}")
    _, _, gear_container = get_block(mission, GEARSCRIPT_FIELD[faction])
    if not gear_container:
        return

    if roles_config is None:
        report.warn(f"{faction} - could not load the global roles config, radio coverage not checked")
        return

    gi = get_field(gear_container, "m_bEnableGIRadios") == "1"
    leadership = get_field(gear_container, "m_bEnableLeadershipRadios") == "1"
    rto = get_field(gear_container, "m_bEnableRTORadios") == "1"
    short_range_prefab = get_field(gear_container, "m_rShortRangeRadioPrefab") or ""
    long_range_prefab = get_field(gear_container, "m_rLongRangeRadioPrefab") or ""
    rto_prefab = get_field(gear_container, "m_rRTORadiosPrefab") or ""

    found_issue = False
    for role in sorted(roles):
        role_config = find_role_config(roles_config, role)
        if role_config is None:
            continue
        _, _, items_block = get_block(role_config, "m_aItems")
        items = set(get_bare_tokens(items_block)) if items_block is not None else set()
        slotting_type = get_field(role_config, "m_SlottingType")
        is_leadership_slot = slotting_type in LEADERSHIP_SLOT_TYPES

        if "SHORTRANGE_RADIO" in items:
            expects = gi or (leadership and is_leadership_slot)
            if expects and not short_range_prefab:
                report.fail(f"{faction} role {role} should get a short-range radio but none is configured")
                found_issue = True

        if "LONGRANGE_RADIO" in items and leadership and not long_range_prefab:
            report.fail(f"{faction} role {role} should get a long-range radio but none is configured")
            found_issue = True

        if "RTO_RADIO" in items:
            if rto and not rto_prefab:
                report.fail(f"{faction} role {role} should get an RTO radio but none is configured")
                found_issue = True
            elif not rto:
                report.warn(f"{faction} role {role} is slotted but RTO Radios are disabled for this faction - will spawn without a radio")
                found_issue = True

    if not found_issue:
        report.ok(f"{faction} radio settings cover every slotted role")


# --- mission settings ------------------------------------------------------------------------


def check_mission_config(report: Report, ent_path: str, repo_root: str) -> None:
    ent_repo_rel = os.path.relpath(ent_path, repo_root).replace("\\", "/")
    parts = ent_repo_rel.split("/")
    if len(parts) < 2:
        return
    terrain = parts[-2]
    missions_dir = os.path.join(repo_root, "Missions", terrain)

    if not os.path.isdir(missions_dir):
        report.fail('No mission config has been generated for this world - run "Generate Config File" first')
        return

    conf_files = [f for f in os.listdir(missions_dir) if f.endswith(".conf")]
    if not conf_files:
        report.fail('No mission config has been generated for this world - run "Generate Config File" first')
        return

    world_assigned = False
    world_matches = False
    for name in conf_files:
        top = parse_container_text(read(os.path.join(missions_dir, name)))
        header = None
        for e in top:
            if e["tokens"] and e["tokens"][0][1] == "SCR_MissionHeader":
                header = e["block"]
                break
        if header is None:
            continue
        world_field = get_field(header, "World")
        if not world_field:
            continue
        world_assigned = True
        _, world_path = split_guid_path(world_field)
        if world_path == ent_repo_rel:
            world_matches = True
            break

    if world_matches:
        report.ok("Mission config found with the world file assigned")
    elif world_assigned:
        report.fail(f"Mission config(s) in Missions/{terrain}/ have a World field set, but none matches this world - re-run \"Generate Config File\"")
    else:
        report.fail(f"Mission config(s) found in Missions/{terrain}/ but none has a world file assigned")


def check_faction_ratios(report: Report, mission: list[dict], lobby_default: list[dict] | None) -> None:
    def ratio(name: str) -> int:
        val = resolve_field(mission, lobby_default, name)
        try:
            return int(val)
        except (TypeError, ValueError):
            return 0

    one_ratio = ratio("m_iFactionOneRatio")
    two_ratio = ratio("m_iFactionTwoRatio")
    one_key = resolve_field(mission, lobby_default, "m_sFactionOneKey") or ""
    two_key = resolve_field(mission, lobby_default, "m_sFactionTwoKey") or ""

    found_issue = False
    if one_ratio <= 0 and two_ratio <= 0:
        report.fail("Neither Faction Ratio is set - at least one must be greater than 0 for ratio-based (TVT) slotting to work")
        found_issue = True
    else:
        if one_ratio <= 0:
            report.warn("Faction One Ratio is 0 - set a ratio if this mission uses ratio-based slotting")
            found_issue = True
        elif not one_key:
            report.fail("Faction One Ratio is set but no Faction One Key is assigned")
            found_issue = True

        if two_ratio <= 0:
            report.warn("Faction Two Ratio is 0 - set a ratio if this mission uses ratio-based slotting")
            found_issue = True
        elif not two_key:
            report.fail("Faction Two Ratio is set but no Faction Two Key is assigned")
            found_issue = True

    if not found_issue:
        report.ok(f"Faction ratios set to {one_key} {one_ratio}:{two_ratio} {two_key}")


EVENT_BASED_RESPAWN_COMPONENTS = ("CRF_CacheHuntGamemodeManager", "CRF_RushGamemodeManager")


def check_spectator_visibility(report: Report, mission: list[dict], lobby_default: list[dict] | None) -> None:
    _, _, components = get_block(mission, "components")
    attached = {cname for cname, guid, sub in get_all_instances(components or [])}
    event_based_respawn = any(c in attached for c in EVENT_BASED_RESPAWN_COMPONENTS)

    respawn_enabled = resolve_field(mission, lobby_default, "m_bRespawnEnabled") == "1"

    if not respawn_enabled and not event_based_respawn:
        report.ok("One-life mission (no respawn, no event-based respawn gamemode) - spectator visibility is not a concern")
        return

    hide_others = resolve_field(mission, lobby_default, "m_bHideOtherSpectatorFactions") == "1"
    if hide_others:
        report.ok("Respawn/event-based respawn is active and other factions are hidden in spectator")
        return

    report.fail('Respawn is enabled (or an event-based respawn gamemode like Cache Hunt/Rush is active) but "Hide Other Spectator Factions" is off - dead/spectating players can see and spectate the enemy faction')


def load_default_descriptors(roots: list[str]) -> list[dict] | None:
    for root in roots:
        candidate = os.path.join(root, "Prefabs", "!Systems", "!Lobby", "COA_Lobby.et")
        if os.path.isfile(candidate):
            top = parse_container_text(read(candidate))
            for e in top:
                if e["tokens"] and e["tokens"][0][1] == "COA_Gamemode":
                    _, _, defaults = get_block(e["block"] or [], "m_aDefaultMissionDescriptors")
                    return defaults
    return None


def check_briefing(report: Report, mission: list[dict], roots: list[str]) -> None:
    _, _, descriptors = get_block(mission, "m_aMissionDescriptors")
    if not descriptors:
        report.fail("No mission briefing/descriptors are configured")
        return

    default_descriptors = load_default_descriptors(roots)

    found_issue = False
    typo_issue = False
    for cname, guid, desc in get_all_instances(descriptors, "COA_MissionDescriptor"):
        title = get_field(desc, "m_sTitle") or "(untitled)"
        text = get_field(desc, "m_sTextData")

        if not text:
            report.fail(f'Briefing "{title}" has no text')
            found_issue = True
            continue

        if default_descriptors is not None:
            for dcname, dguid, ddesc in get_all_instances(default_descriptors, "COA_MissionDescriptor"):
                if get_field(ddesc, "m_sTitle") == get_field(desc, "m_sTitle") and get_field(ddesc, "m_sTextData") == text:
                    report.fail(f'Briefing "{title}" still has the default template text - fill it in')
                    found_issue = True
                    break

        for issue in find_text_issues(text):
            report.warn(f'Briefing "{title}": {issue}')
            typo_issue = True

    if not found_issue:
        report.ok("Mission briefing is filled out")
    if not typo_issue:
        report.ok("No obvious typos found in the briefing (doubled words, double spaces, leftover placeholders)")


def find_text_issues(text: str) -> list[str]:
    issues = []
    words = text.split(" ")
    previous = None
    for word in words:
        lower = word.strip().lower()
        if lower and lower == previous:
            issues.append(f'doubled word "{word}"')
        previous = lower
    if "  " in text:
        issues.append("contains a double space")
    upper = text.upper()
    if any(marker in upper for marker in ("TODO", "FIXME", "TBD", "???", "XXX")):
        issues.append("contains a leftover placeholder marker (TODO/FIXME/TBD/???/XXX)")
    return issues


# -----------------------------------------------------------------------------------------------
#  DRIVER
# -----------------------------------------------------------------------------------------------


def check_world(ent_path: str, repo_root: str, roots: list[str]) -> Report:
    report = Report()
    found = find_mission_entries(ent_path)
    if found is None:
        report.warn(f"{rel(ent_path, roots)}: no COA_Lobby-derived entity found in its _Layers folder - skipping")
        return report

    mission, layer_path = found
    lobby_default = None
    for root in roots:
        if os.path.basename(root).lower() in ("coalition-lobby",) or os.path.isdir(os.path.join(root, "Prefabs", "!Systems", "!Lobby")):
            candidate = os.path.join(root, "Prefabs", "!Systems", "!Lobby", "COA_Lobby.et")
            if os.path.isfile(candidate):
                top = parse_container_text(read(candidate))
                for e in top:
                    if e["tokens"] and e["tokens"][0][1] == "COA_Gamemode":
                        lobby_default = e["block"]
                        break
                break

    report.lines.append(f"# {rel(ent_path, roots)}  (entity in {rel(layer_path, roots)})")

    respawn_enabled = resolve_field(mission, lobby_default, "m_bRespawnEnabled") == "1"
    respawn_mode = resolve_field(mission, lobby_default, "m_eRespawnMode") or "0"
    roles_config = load_roles_config(roots)

    any_faction_used = False
    for faction in FACTIONS:
        _, _, slots = get_block(mission, SLOTS_FIELD[faction])
        if not slots:
            continue
        any_faction_used = True

        roles = check_orbat(report, faction, slots, respawn_mode)
        check_tickets(report, faction, mission, lobby_default, respawn_enabled, respawn_mode)
        check_gear(report, faction, mission, roles, roots, repo_root)
        check_radios(report, faction, mission, roles, roles_config)

    if not any_faction_used:
        report.warn("No faction has any slots configured - nothing to check")
        return report

    report.section("Mission Settings")
    check_mission_config(report, ent_path, repo_root)
    check_faction_ratios(report, mission, lobby_default)
    check_spectator_visibility(report, mission, lobby_default)
    check_briefing(report, mission, roots)

    return report


def find_changed_worlds(repo_root: str, base_ref: str) -> list[str]:
    try:
        out = subprocess.run(
            ["git", "diff", "--name-only", "--diff-filter=ACM", f"{base_ref}...HEAD"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        ).stdout
    except subprocess.CalledProcessError as exc:
        print(f"error: git diff failed: {exc.stderr}", file=sys.stderr)
        return []

    worlds = []
    for line in out.splitlines():
        line = line.strip()
        if line.endswith(".ent") and line.startswith("Worlds/"):
            worlds.append(os.path.join(repo_root, line))
    return worlds


def main() -> int:
    here = os.path.dirname(os.path.abspath(__file__))
    default_root = os.path.abspath(os.path.join(here, "..", ".."))

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--project", action="append", default=[], metavar="DIR",
                        help="Additional project root to scan (repeatable) - e.g. ../COALITION-Lobby, "
                             "needed for the global roles config and COA_Lobby.et prefab defaults.")
    parser.add_argument("--world", action="append", default=[], metavar="PATH",
                        help="Path to a .ent world file to check (repeatable).")
    parser.add_argument("--base-ref", metavar="REF",
                        help="Diff against this git ref to auto-detect changed/added worlds "
                             "instead of passing --world explicitly.")
    args = parser.parse_args()

    roots = [default_root] + [os.path.abspath(p) for p in args.project]
    missing = [r for r in roots if not os.path.isdir(r)]
    if missing:
        print(f"error: project root does not exist: {', '.join(missing)}", file=sys.stderr)
        return 2

    worlds = [os.path.abspath(w) for w in args.world]
    if args.base_ref:
        worlds += find_changed_worlds(default_root, args.base_ref)

    if not worlds:
        print("No .ent world specified or found - nothing to check.")
        return 0

    print("Scanned: " + ", ".join(os.path.basename(r) for r in roots))
    print()

    total_fail = 0
    total_warn = 0
    for ent_path in worlds:
        if not os.path.isfile(ent_path):
            print(f"error: world file does not exist: {ent_path}", file=sys.stderr)
            total_fail += 1
            continue

        report = check_world(ent_path, default_root, roots)
        total_fail += report.fail_count
        total_warn += report.warn_count
        for line in report.lines:
            print(line)
        print()

    print(f"{total_fail} failure(s), {total_warn} warning(s) across {len(worlds)} world(s).")
    return 1 if total_fail else 0


if __name__ == "__main__":
    sys.exit(main())
