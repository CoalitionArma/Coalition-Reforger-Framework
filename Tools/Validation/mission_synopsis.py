#!/usr/bin/env python3
"""
Mission pull request automation for Coalition addon projects.

Builds the mission synopsis that the COALITION-Lobby "6 | Generate Config File" Workbench plugin
copies to the clipboard (COA_MissionSynopsisGenerator.c, extended by CRF's
CRF_COA_MissionSynopsisGenerator.c), but straight from the committed files, so a pull request's
description can be kept up to date on every push instead of being pasted by hand.

The output must stay byte-for-byte compatible with the Workbench generator: Coalition_Bot
(functions/githubWebhookHandler.js) and the website's weekly missions page parse this block - the
CRF_SYNOPSIS_START/END markers, the "**Label:**" header line and the "## General" bullets - to build
the Discord AAR thread and the mission listing. If COA_MissionSynopsisGenerator.c changes, change
build_synopsis() to match.

Values resolve the way Workbench resolves them on the placed entity: the mission's own override in
its .layer file, then CRF's COA_Lobby.et override, then the Lobby's COA_Lobby.et, then the script
[Attribute] default - read from the Lobby's scripts at run time, as are the enums, so they can't
drift.

Usage
-----
    # Print the synopsis, title and checks for a world
    python3 Tools/Validation/mission_synopsis.py --project ../COALITION-Lobby --world Worlds/Arland/Tanaka_TVT_ThePatrol.ent

    # CI: find the world a branch adds or changes, and write an updated PR body/title
    python3 Tools/Validation/mission_synopsis.py --project ../COALITION-Lobby --base-ref origin/release \\
        --pr-json pr.json --out-dir out
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
from validate_resources import read  # noqa: E402
from validate_mission import (  # noqa: E402
    FACTIONS,
    GEARSCRIPT_FIELD,
    SLOTS_FIELD,
    TICKETS_FIELD,
    callsign_tier,
    find_mission_entries,
    get_all_instances,
    get_bare_tokens,
    get_block,
    get_field,
    load_default_descriptors,
    load_roles_config,
    parse_container_text,
    split_guid_path,
)

SYNOPSIS_START = "<!-- CRF_SYNOPSIS_START -->"
SYNOPSIS_END = "<!-- CRF_SYNOPSIS_END -->"
CHECKS_START = "<!-- CRF_CHECKS_START -->"
CHECKS_END = "<!-- CRF_CHECKS_END -->"

# Where each managed block goes when a description doesn't have its markers yet
SYNOPSIS_ANCHOR = "**Mission Synopsis:**"
CHECKS_ANCHOR = "**Automated Checks:**"

LOBBY_PREFAB_SUFFIX = "COA_Lobby.et"


# -----------------------------------------------------------------------------------------------
#  SCRIPT DEFAULTS AND ENUMS (read from the Lobby's Enforce sources)
# -----------------------------------------------------------------------------------------------

_CLASS_RE = re.compile(r"^\s*(?:modded\s+)?class\s+(\w+)")
_ENUM_RE = re.compile(r"^\s*enum\s+(\w+)")
_ATTR_RE = re.compile(r"^\s*\[\s*Attribute\s*\((.*)")
_DECL_RE = re.compile(r"^\s*(?:(?:protected|private|static|const|ref)\s+)*[\w<>, ]+?\s+(m_\w+)\s*(?:=[^;]*)?;")
_DEFVALUE_RE = re.compile(r'defvalue\s*:\s*"([^"]*)"')


class ScriptInfo:
    """[Attribute] defaults per class and enum member lists, parsed from .c files."""

    def __init__(self) -> None:
        self.defaults: dict[tuple[str, str], str | None] = {}
        self.enums: dict[str, list[str]] = {}

    def load(self, roots: list[str]) -> None:
        for root in roots:
            scripts = os.path.join(root, "Scripts")
            if not os.path.isdir(scripts):
                continue
            for dirpath, _, filenames in os.walk(scripts):
                for name in filenames:
                    if name.endswith(".c"):
                        self._parse(read(os.path.join(dirpath, name)))

    def _parse(self, text: str) -> None:
        current_class = None
        pending_attr: str | None | bool = False  # False = no pending attribute
        in_attribute = False  # inside an [Attribute(...)] that spans several lines
        enum_name = None
        enum_members: list[str] = []

        for raw in text.splitlines():
            line = raw.split("//", 1)[0]

            if in_attribute:
                if "]" in line:
                    in_attribute = False
                continue

            m = _ENUM_RE.match(line)
            if m and enum_name is None:
                enum_name, enum_members = m.group(1), []
                line = line[m.end():]

            if enum_name is not None:
                for part in line.replace("{", " ").split(","):
                    member = part.strip().split("=")[0].strip()
                    if member == "}" or member.startswith("}"):
                        continue
                    if re.fullmatch(r"\w+", member):
                        enum_members.append(member)
                if "}" in line:
                    self.enums.setdefault(enum_name, enum_members)
                    enum_name = None
                continue

            m = _CLASS_RE.match(line)
            if m:
                current_class = m.group(1)
                pending_attr = False
                continue

            m = _ATTR_RE.match(line)
            if m:
                args = m.group(1)
                d = _DEFVALUE_RE.search(args)
                if d:
                    pending_attr = d.group(1)
                else:
                    first = re.match(r'\s*"([^"]*)"', args)
                    pending_attr = first.group(1) if first else None
                in_attribute = "]" not in args
                continue

            if pending_attr is not False and current_class:
                m = _DECL_RE.match(line)
                if m:
                    self.defaults.setdefault((current_class, m.group(1)), pending_attr)
                    pending_attr = False
                elif line.strip() and not line.strip().startswith("["):
                    pending_attr = False

    def default(self, class_name: str, field: str, fallback: str = "0") -> str:
        value = self.defaults.get((class_name, field))
        if value is None or value == "":
            return fallback
        return {"true": "1", "false": "0"}.get(value.lower(), value)

    def enum_name(self, enum: str, value: str | None) -> str:
        """Enum member name for a stored value, which may be a name or a number."""
        members = self.enums.get(enum, [])
        if value is None or value == "":
            return members[0] if members else "0"
        if re.fullmatch(r"-?\d+", value):
            index = int(value)
            return members[index] if 0 <= index < len(members) else value
        return value


# -----------------------------------------------------------------------------------------------
#  VALUE RESOLUTION (mission -> CRF COA_Lobby.et -> Lobby COA_Lobby.et -> script default)
# -----------------------------------------------------------------------------------------------


def load_lobby_prefabs(roots: list[str]) -> list[list[dict]]:
    """COA_Gamemode blocks of every COA_Lobby.et found, CRF's override first (roots order)."""
    blocks = []
    for root in roots:
        prefabs = os.path.join(root, "Prefabs")
        if not os.path.isdir(prefabs):
            continue
        for dirpath, _, filenames in os.walk(prefabs):
            if LOBBY_PREFAB_SUFFIX in filenames:
                top = parse_container_text(read(os.path.join(dirpath, LOBBY_PREFAB_SUFFIX)))
                for e in top:
                    if e["tokens"] and e["tokens"][0][1] == "COA_Gamemode":
                        blocks.append(e["block"] or [])
                        break
    return blocks


def first_field(levels: list[list[dict]], name: str) -> str | None:
    for level in levels:
        value = get_field(level, name)
        if value is not None:
            return value
    return None


def first_block(levels: list[list[dict]], name: str) -> list[dict] | None:
    for level in levels:
        _, _, block = get_block(level, name)
        if block is not None:
            return block
    return None


def sub_levels(levels: list[list[dict]], name: str) -> list[list[dict]]:
    """The same named object property at every level that sets it."""
    result = []
    for level in levels:
        _, _, block = get_block(level, name)
        if block is not None:
            result.append(block)
    return result


def component_levels(levels: list[list[dict]], class_name: str) -> list[list[dict]]:
    result = []
    for level in levels:
        _, _, components = get_block(level, "components")
        for cname, _, block in get_all_instances(components or [], class_name):
            result.append(block)
    return result


def component_classes(levels: list[list[dict]]) -> set[str]:
    found = set()
    for level in levels:
        _, _, components = get_block(level, "components")
        for cname, _, _ in get_all_instances(components or []):
            found.add(cname)
    return found


# -----------------------------------------------------------------------------------------------
#  MISSION DATA
# -----------------------------------------------------------------------------------------------


def find_mission_config(ent_path: str, repo_root: str) -> tuple[str | None, list[dict] | None, list[str]]:
    """(conf_path, SCR_MissionHeader block, every matching conf) for the .conf whose World is this world."""
    ent_rel = os.path.relpath(ent_path, repo_root).replace("\\", "/")
    parts = ent_rel.split("/")
    if len(parts) < 2:
        return None, None, []
    missions_dir = os.path.join(repo_root, "Missions", parts[-2])
    if not os.path.isdir(missions_dir):
        return None, None, []

    matches = []
    for name in sorted(os.listdir(missions_dir)):
        if not name.endswith(".conf"):
            continue
        path = os.path.join(missions_dir, name)
        for e in parse_container_text(read(path)):
            if e["tokens"] and e["tokens"][0][1] == "SCR_MissionHeader":
                world = get_field(e["block"] or [], "World")
                if world and split_guid_path(world)[1] == ent_rel:
                    matches.append((path, e["block"] or []))
                break
    if not matches:
        return None, None, []
    return matches[0][0], matches[0][1], [p for p, _ in matches]


def count_slots(slots: list[dict] | None) -> int:
    total = 0
    for _, _, group in get_all_instances(slots or [], "COA_SlottingGroup"):
        _, _, roles = get_block(group, "m_aSlots")
        total += len(get_bare_tokens(roles or []))
    return total


def yes_no(value: str | None) -> str:
    return "Yes" if value not in (None, "", "0", "false") else "No"


def tickets_text(value: str | None) -> str:
    return "Unlimited" if value == "-1" else (value or "0")


def float_text(value: str) -> str:
    try:
        return "%g" % float(value)
    except ValueError:
        return value


def strip_display_prefix(display_name: str) -> str:
    """'COA TVT32 The Patrol' -> 'The Patrol'."""
    m = re.match(r"^\s*(?:COA|CRF)\s+\S*\d+\s+(.+)$", display_name)
    return m.group(1).strip() if m else display_name.strip()


class Mission:
    def __init__(self, ent_path: str, repo_root: str, roots: list[str], info: ScriptInfo) -> None:
        self.ent_path = ent_path
        self.repo_root = repo_root
        self.roots = roots
        self.info = info
        self.ent_rel = os.path.relpath(ent_path, repo_root).replace("\\", "/")
        self.terrain = self.ent_rel.split("/")[-2] if "/" in self.ent_rel else ""

        found = find_mission_entries(ent_path)
        self.entity = found[0] if found else None
        self.layers_dir = ent_path[: -len(".ent")] + "_Layers"
        self.levels = ([self.entity] if self.entity is not None else []) + load_lobby_prefabs(roots)

        self.conf_path, self.header, self.all_confs = find_mission_config(ent_path, repo_root)
        self.roles_config = load_roles_config(roots)

    # --- resolved values ---------------------------------------------------------------------------

    def gm(self, field: str, fallback: str = "0") -> str:
        value = first_field(self.levels, field)
        return value if value is not None else self.info.default("COA_Gamemode", field, fallback)

    def slots(self, faction: str) -> list[dict]:
        return first_block(self.levels, SLOTS_FIELD[faction]) or []

    @property
    def slot_based(self) -> bool:
        return self.info.enum_name("COA_ERespawnMode", self.gm("m_eRespawnMode")) == "SLOT"

    @property
    def player_count(self) -> int:
        return sum(count_slots(self.slots(f)) for f in FACTIONS)

    def header_field(self, name: str) -> str:
        return (get_field(self.header or [], name) or "").strip()

    @property
    def mode(self) -> str:
        return self.header_field("m_sGameMode") or "TVT"

    @property
    def author(self) -> str:
        return self.header_field("m_sAuthor")

    @property
    def mission_name(self) -> str:
        return strip_display_prefix(self.header_field("m_sName"))

    @property
    def display_name(self) -> str:
        return f"COA {self.mode}{self.player_count} {self.mission_name}"

    def role_name(self, role: str) -> str:
        if not self.roles_config:
            return role
        _, _, configs = get_block(self.roles_config, "m_RoleConfigs")
        for _, _, sub in get_all_instances(configs or [], "COA_RoleConfig"):
            if self.info.enum_name("COA_EGearRole", get_field(sub, "m_Role")) == role:
                name = get_field(sub, "m_sRoleName")
                return name if name else role
        return role


# -----------------------------------------------------------------------------------------------
#  SYNOPSIS (mirrors COA_MissionSynopsisGenerator.BuildMissionSynopsis)
# -----------------------------------------------------------------------------------------------


def crf_component_names(roots: list[str]) -> list[tuple[str, str]]:
    """(class, display name) pairs from CRF_COA_MissionSynopsisGenerator.c, in its order."""
    pattern = re.compile(r'FindComponentIndex\(\s*entitySource\s*,\s*(\w+)\s*\)\s*>=\s*0\s*\)\s*activeComponents\.Insert\(\s*"([^"]+)"\s*\)')
    for root in roots:
        for dirpath, _, filenames in os.walk(os.path.join(root, "Scripts")):
            if "CRF_COA_MissionSynopsisGenerator.c" in filenames:
                return pattern.findall(read(os.path.join(dirpath, "CRF_COA_MissionSynopsisGenerator.c")))
    return []


def build_synopsis(m: Mission) -> list[str]:
    info = m.info
    lines = [SYNOPSIS_START, f"# {m.display_name}", ""]
    lines.append(f"**Author:** {m.author}  **Terrain:** {m.terrain}  **Game Mode:** {m.mode}  **Player Count:** {m.player_count}")
    lines.append("")

    description = m.header_field("m_sDescription")
    if description and description != "<Description>":
        lines.append(description)
        lines.append("")

    used = [f for f in FACTIONS if get_all_instances(m.slots(f), "COA_SlottingGroup")]

    # General
    lines.append("## General")
    one_key, two_key = m.gm("m_sFactionOneKey", ""), m.gm("m_sFactionTwoKey", "")
    if one_key and two_key:
        lines.append(f"- Slotting Ratio: {one_key} {m.gm('m_iFactionOneRatio')}:{m.gm('m_iFactionTwoRatio')} {two_key}")
    limit = int(m.gm("m_iTimeLimitMinutes") or 0)
    lines.append(f"- Time Limit: {f'{limit} minutes' if limit > 0 else 'No limit'}")
    lines.append(f"- Disable JIP: {yes_no(m.gm('m_bLockUnusedSlots'))}")
    safestart = f"{m.gm('m_iSafestartTimeLimit')} minutes" if yes_no(m.gm("m_bUseSafestartTimeLimit")) == "Yes" else "Disabled"
    lines.append(f"- Safestart Time Limit: {safestart}")
    lines.append(f"- Mission Time Scale: {float_text(m.gm('m_fMissionTimeScale'))}x")
    lines.append(f"- Coalition VON (CVON): {yes_no(m.gm('m_bUseCVON'))}")
    lines.append("")

    # Respawn
    lines.append("## Respawn")
    respawn = yes_no(m.gm("m_bRespawnEnabled"))
    lines.append(f"- Respawn Enabled: {respawn}")
    if respawn == "Yes":
        lines.append(f"- Mode: {'Slot-Based' if m.slot_based else 'Team-Based'}")
        if not m.slot_based:
            for f in used:
                lines.append(f"- {f} Tickets: {tickets_text(m.gm(TICKETS_FIELD[f]))}")
        else:
            lines.append("- Per-squad respawn pools are listed under Slotting below.")
        lines.append(f"- Wave Respawn: {yes_no(m.gm('m_bWaveRespawn'))}")
        lines.append(f"- Time To Respawn: {m.gm('m_iTimeToRespawn')}s")
        cutoff = int(m.gm("m_iRespawnCutoffMinutes") or 0)
        lines.append(f"- Respawn Cutoff: {f'Disables {cutoff} minutes before mission end' if cutoff > 0 else 'Never disables'}")
        lines.append(f"- Rally Points Enabled: {yes_no(m.gm('m_bRallyPointsEnabled'))}")
    lines.append("")

    # Gearscripts
    lines.append("## Gearscripts")
    for f in used:
        gear = first_field(sub_levels(m.levels, GEARSCRIPT_FIELD[f]), "m_rGearScript")
        lines.append(f"- {f}: {gear if gear else 'None'}")
    lines.append("")

    # Game mode components
    lines.append("## Game Mode Components")
    lines.append(f"- Selected Mode: {m.mode}")
    attached = component_classes(m.levels)
    active = [label for cls, label in crf_component_names(m.roots) if cls in attached]
    lines.append(f"- Active Components: {', '.join(active) if active else 'None detected'}")
    lines.append("")

    # Weather
    lines.append("## Weather")
    weather = component_levels(m.levels, "SCR_TimeAndWeatherHandlerComponent")
    states = first_block(weather, "m_aStartingWeatherAndTime") if weather else None
    state_list = get_all_instances(states or [], "SCR_TimeAndWeatherState")
    if not weather or not state_list:
        lines.append("- Not configured")
    else:
        # The first state, with any fields it doesn't set taken from the same state (same ID) further up
        _, state_id, _ = state_list[0]
        state_levels = []
        for level in weather:
            for _, sid, block in get_all_instances(get_block(level, "m_aStartingWeatherAndTime")[2] or [], "SCR_TimeAndWeatherState"):
                if sid == state_id:
                    state_levels.append(block)
        preset = first_field(state_levels, "m_sWeatherPresetName") or ""
        hour = first_field(state_levels, "m_iStartingHour") or "8"
        minutes = first_field(state_levels, "m_iStartingMinutes") or "0"
        lines.append(f"- Starting Weather: {preset}")
        lines.append(f"- Starting Time: {int(hour)}:{int(minutes)}")
        lines.append(f"- Random Starting Weather: {yes_no(first_field(weather, 'm_bRandomStartingWeather'))}")
        lines.append(f"- Random Weather Changes: {yes_no(first_field(weather, 'm_bRandomWeatherChanges'))}")
    lines.append("")

    # Slotting
    for f in used:
        append_faction_slotting(lines, m, f)

    lines.append(SYNOPSIS_END)
    return lines


def append_faction_slotting(lines: list[str], m: Mission, faction: str) -> None:
    info = m.info
    groups = get_all_instances(m.slots(faction), "COA_SlottingGroup")
    lines.append(f"## Slotting - {faction} ({count_slots(m.slots(faction))} slots)")

    active_company = False
    active_platoon = False
    for _, _, group in groups:
        callsign = get_field(group, "m_sCallsign") or ""
        tier = callsign_tier(callsign)
        depth = 0
        if tier == "COMPANY":
            active_company, active_platoon = True, False
        elif tier == "PLATOON":
            depth = 1 if active_company else 0
            active_platoon = True
        elif tier == "SQUAD" and active_platoon:
            depth = 2 if active_company else 1

        indent = "  " * depth
        role_indent = indent + "  "
        flag = info.enum_name("COA_EFlagType", get_field(group, "m_FlagType"))
        lines.append(f"{indent}- **{callsign}** ({flag})")

        pool = info.enum_name("COA_ERespawnPoolType", get_field(group, "m_eRespawnPoolType"))
        if m.slot_based and pool == "PER_GROUP":
            lines.append(f"{role_indent}- Respawn Pool (shared): {tickets_text(get_field(group, 'm_iGroupRespawns') or info.default('COA_SlottingGroup', 'm_iGroupRespawns', '-1'))}")

        _, _, roles_block = get_block(group, "m_aSlots")
        tally: dict[str, int] = {}
        for role in get_bare_tokens(roles_block or []):
            role = info.enum_name("COA_EGearRole", role)
            tally[role] = tally.get(role, 0) + 1

        respawns: dict[str, str] = {}
        _, _, respawn_block = get_block(group, "m_aRoleRespawns")
        for _, _, entry in get_all_instances(respawn_block or [], "COA_RoleRespawnEntry"):
            role = info.enum_name("COA_EGearRole", get_field(entry, "m_Role"))
            respawns.setdefault(role, get_field(entry, "m_iRespawns") or "-1")

        for role, count in tally.items():
            line = f"{role_indent}- {m.role_name(role)} x{count}"
            if m.slot_based and pool == "PER_SLOT":
                line += f" ({tickets_text(respawns.get(role, '-1'))} respawns each)"
            lines.append(line)

    lines.append("")


# -----------------------------------------------------------------------------------------------
#  AUTOMATED CHECKS (the checklist items that can be read from the files)
# -----------------------------------------------------------------------------------------------


def placed_prefabs(layers_dir: str) -> list[str]:
    paths = []
    if not os.path.isdir(layers_dir):
        return paths
    for name in os.listdir(layers_dir):
        if name.endswith(".layer"):
            paths += re.findall(r'"\{[0-9A-Fa-f]{16}\}([^"]+\.et)"', read(os.path.join(layers_dir, name)))
    return paths


def build_checks(m: Mission) -> list[tuple[bool, str]]:
    checks: list[tuple[bool, str]] = []

    checks.append((m.entity is not None, f"Mission file: `{m.ent_rel}`" if m.entity is not None
                   else f"Mission file: `{m.ent_rel}` has no COA_Lobby entity in its layers - run plugin 1"))

    if m.conf_path:
        conf_rel = os.path.relpath(m.conf_path, m.repo_root).replace("\\", "/")
        note = f" ({len(m.all_confs)} configs point at this world - plugin 6 may have run twice; keep one)" if len(m.all_confs) > 1 else ""
        checks.append((len(m.all_confs) == 1, f"Mission .conf file: `{conf_rel}`{note}"))
    else:
        checks.append((False, f"Mission .conf file: none in `Missions/{m.terrain}/` points at this world - run plugin 6 (Generate Config File)"))

    prefabs = placed_prefabs(m.layers_dir)
    border = any(re.search(r"GameBorder\.et$|GameBoundry\.et$", p) for p in prefabs)
    markers = sum(1 for p in prefabs if "/Markers/" in p)
    # Many co-op and special missions use the terrain's edges instead of a game border, so either counts
    marker_text = f"{markers} map marker{'s' if markers != 1 else ''}"
    checks.append((border or markers > 0, f"In-game play area and markers: {'game border placed' if border else 'no game border'}, {marker_text}"
                   + ("" if border or markers else " - place a game border or map markers so players can see the area")))

    descriptors = []
    for level in m.levels[:1]:
        _, _, block = get_block(level, "m_aMissionDescriptors")
        descriptors = get_all_instances(block or [], "COA_MissionDescriptor")
    defaults = {(get_field(d, "m_sTitle"), get_field(d, "m_sTextData"))
                for _, _, d in get_all_instances(load_default_descriptors(m.roots) or [], "COA_MissionDescriptor")}

    def filled(desc: list[dict]) -> bool:
        text = get_field(desc, "m_sTextData")
        return bool(text) and (get_field(desc, "m_sTitle"), text) not in defaults

    show_any_default = m.info.default("COA_MissionDescriptor", "m_bShowForAnyFaction")
    for_all = any(filled(d) and yes_no(get_field(d, "m_bShowForAnyFaction") or show_any_default) == "Yes" for _, _, d in descriptors)
    checks.append((for_all, "In-game briefing for all players" if for_all
                   else "In-game briefing for all players: none filled in (Configure Descriptions, Show For Any Faction)"))

    used = [f for f in FACTIONS if get_all_instances(m.slots(f), "COA_SlottingGroup")]
    missing = []
    for f in used:
        if not any(filled(d) and f in get_bare_tokens(get_block(d, "m_aFactionKeys")[2] or [])
                   for _, _, d in descriptors):
            missing.append(f)
    checks.append((not missing, f"Side-based briefings: {', '.join(used)}" if not missing
                   else f"Side-based briefings: missing or still the template text for {', '.join(missing)}"))
    return checks


def build_checks_block(checks: list[tuple[bool, str]]) -> list[str]:
    lines = [CHECKS_START]
    lines += [f"- [{'x' if ok else ' '}] {text}" for ok, text in checks]
    lines.append("")
    lines.append("_Ticked automatically from the mission files on every push. Run the CRF Mission QA Checklist plugin for the full report._")
    lines.append(CHECKS_END)
    return lines


# -----------------------------------------------------------------------------------------------
#  PULL REQUEST BODY AND TITLE
# -----------------------------------------------------------------------------------------------


def upsert_block(body: str, start: str, end: str, block: str, anchor: str) -> str:
    """Replace the marked block; else put it under the anchor heading; else append it."""
    pattern = re.compile(re.escape(start) + r"[\s\S]*?" + re.escape(end))
    if pattern.search(body):
        return pattern.sub(lambda _: block, body, count=1)

    lines = body.split("\n")
    for i, line in enumerate(lines):
        if line.strip() == anchor:
            # Drop the template's placeholder lines under the heading (up to the next bold heading)
            j = i + 1
            while j < len(lines) and not lines[j].lstrip().startswith("**"):
                if lines[j].strip() and not lines[j].strip().startswith("<"):
                    break
                j += 1
            return "\n".join(lines[: i + 1] + ["", block, ""] + lines[j:]).rstrip() + "\n"

    return body.rstrip() + "\n\n" + (anchor + "\n\n" if anchor else "") + block + "\n"


def drop_unused_faction_placeholders(body: str, used: list[str]) -> str:
    """Remove the template's untouched faction lines for sides the mission has no slots for.

    Left in, Coalition_Bot posts them to the AAR thread, and its "Assets:" pattern reads the next
    line when the assets line is empty. Only the exact template text is removed, never anything a
    mission maker has written.
    """
    lines = body.split("\n")
    result = []
    i = 0
    while i < len(lines):
        m = re.match(r"^(BLUFOR|OPFOR|INDFOR|CIV) - \(EX\. [^)]*\) - Attacking/Defending/NA\s*$", lines[i])
        if m and m.group(1) not in used:
            side = m.group(1)
            i += 1
            if i < len(lines) and re.match(rf"^{side} Assets:\s*$", lines[i]):
                i += 1
            if i < len(lines) and not lines[i].strip() and result and not result[-1].strip():
                i += 1
            continue
        result.append(lines[i])
        i += 1
    return "\n".join(result)


def title_author(m: Mission, fallback: str) -> str:
    author = re.sub(r"\s+", "", m.author)
    return author if author and author != "<Author>" else fallback


def build_title(m: Mission, fallback_author: str) -> str:
    return f"New Mission | {title_author(m, fallback_author)} {m.mode}{m.player_count} {m.mission_name}"


# -----------------------------------------------------------------------------------------------
#  DRIVER
# -----------------------------------------------------------------------------------------------


def changed_worlds(repo_root: str, base_ref: str) -> list[tuple[str, str]]:
    """(status, path) for every world .ent the branch adds (A) or modifies (M) compared to base_ref."""
    try:
        out = subprocess.run(
            ["git", "diff", "--name-status", "--diff-filter=AM", f"{base_ref}...HEAD", "--", "Worlds"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        ).stdout
    except subprocess.CalledProcessError as exc:
        print(f"error: git diff failed: {exc.stderr}", file=sys.stderr)
        return []
    result = []
    for line in out.splitlines():
        parts = line.split("\t")
        if len(parts) == 2 and parts[1].endswith(".ent"):
            result.append((parts[0][0], os.path.join(repo_root, parts[1])))
    # A world whose layers changed but not the .ent itself
    try:
        layers = subprocess.run(
            ["git", "diff", "--name-only", "--diff-filter=AM", f"{base_ref}...HEAD", "--", "Worlds"],
            cwd=repo_root, capture_output=True, text=True, check=True,
        ).stdout
    except subprocess.CalledProcessError:
        layers = ""
    for line in layers.splitlines():
        m = re.match(r"^(Worlds/.+)_Layers/[^/]+\.layer$", line.strip())
        if m:
            ent = os.path.join(repo_root, m.group(1) + ".ent")
            if os.path.isfile(ent) and all(os.path.abspath(p) != os.path.abspath(ent) for _, p in result):
                result.append(("M", ent))
    return result


def main() -> int:
    default_root = os.path.abspath(os.path.join(_HERE, "..", ".."))

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--project", action="append", default=[], metavar="DIR",
                        help="Additional project root (repeatable), e.g. ../COALITION-Lobby - needed for the "
                             "Lobby's COA_Lobby.et, scripts and global roles config.")
    parser.add_argument("--world", metavar="PATH", help="The mission's .ent world file.")
    parser.add_argument("--base-ref", metavar="REF", help="Find the world this branch adds or changes compared to REF.")
    parser.add_argument("--pr-json", metavar="FILE", help="JSON with the pull request's title, body and author.login (gh pr view --json title,body,author).")
    parser.add_argument("--out-dir", metavar="DIR", help="Write body.md / title.txt here when they need changing, plus summary.md.")
    args = parser.parse_args()

    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    roots = [default_root] + [os.path.abspath(p) for p in args.project]
    info = ScriptInfo()
    info.load(roots)
    if "COA_EGearRole" not in info.enums:
        print("error: couldn't find the Lobby's scripts - pass --project ../COALITION-Lobby", file=sys.stderr)
        return 2

    is_new = False
    if args.world:
        ent_path = os.path.abspath(args.world)
    elif args.base_ref:
        worlds = changed_worlds(default_root, args.base_ref)
        if not worlds:
            print("No world added or changed on this branch - nothing to do.")
            return 0
        added = [p for s, p in worlds if s == "A"]
        ent_path = (added or [p for _, p in worlds])[0]
        is_new = bool(added)
        if len(worlds) > 1:
            print(f"note: {len(worlds)} worlds changed; using {os.path.relpath(ent_path, default_root)}")
    else:
        parser.error("pass --world or --base-ref")

    mission = Mission(ent_path, default_root, roots, info)
    if mission.entity is None:
        print(f"{os.path.relpath(ent_path, default_root)} has no COA_Lobby entity - not a Lobby mission, nothing to do.")
        return 0

    checks = build_checks(mission)
    checks_block = "\n".join(build_checks_block(checks))
    synopsis_block = "\n".join(build_synopsis(mission)) if mission.header is not None else None

    pr = {}
    if args.pr_json:
        with open(args.pr_json, encoding="utf-8") as handle:
            pr = json.load(handle)
    pr_author = ((pr.get("author") or {}).get("login")) or "Author"
    title = build_title(mission, pr_author) if mission.header is not None else None

    if not args.out_dir:
        print(f"Title: {title or '(no mission .conf yet - run plugin 6)'}")
        print()
        print(checks_block)
        print()
        print(synopsis_block or "(no synopsis without a mission .conf)")
        return 0

    os.makedirs(args.out_dir, exist_ok=True)
    body = (pr.get("body") or "").replace("\r\n", "\n")
    new_body = upsert_block(body, CHECKS_START, CHECKS_END, checks_block, CHECKS_ANCHOR)
    if synopsis_block:
        new_body = upsert_block(new_body, SYNOPSIS_START, SYNOPSIS_END, synopsis_block, SYNOPSIS_ANCHOR)
    new_body = drop_unused_faction_placeholders(new_body, [f for f in FACTIONS if get_all_instances(mission.slots(f), "COA_SlottingGroup")])
    if new_body.strip() != body.strip():
        with open(os.path.join(args.out_dir, "body.md"), "w", encoding="utf-8") as handle:
            handle.write(new_body)

    # Only new missions get their title set: the AAR bot keys off "New Mission |" titles
    if title and is_new and (pr.get("title") or "").strip() != title:
        with open(os.path.join(args.out_dir, "title.txt"), "w", encoding="utf-8") as handle:
            handle.write(title)

    with open(os.path.join(args.out_dir, "summary.md"), "w", encoding="utf-8") as handle:
        handle.write(f"### {mission.display_name if mission.header is not None else mission.ent_rel}\n\n")
        handle.write(f"World: `{mission.ent_rel}`{' (new)' if is_new else ''}\n\n")
        if title:
            handle.write(f"Title: `{title}`\n\n")
        handle.write("\n".join(f"- {'✅' if ok else '❌'} {text}" for ok, text in checks) + "\n")

    print(open(os.path.join(args.out_dir, "summary.md"), encoding="utf-8").read())
    return 0


if __name__ == "__main__":
    sys.exit(main())
