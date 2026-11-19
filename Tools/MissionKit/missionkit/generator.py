"""Spec -> mission files (world .ent + .meta, header .conf + .meta, layers).

Output conventions (checked against CRF): LF line endings, one space of
indentation per level, no trailing newline (as Workbench writes them),
numbers with up to 3 decimals, IDs derived from ``mission.id_prefix`` and
entity names (so a rebuild of an unchanged spec is byte-identical and an
edit gives a clean diff).
"""
from __future__ import annotations

import json
import math
import os
import re
from contextlib import contextmanager
from dataclasses import dataclass, field
from typing import Optional

from . import enf
from .enf import format_number as fmt, quote
from .spec import (PendingAsset, Spec, SpecError, angles_of, expand_names, expand_roles, gen_id, vec3)

# ----------------------------------------------------------------------------- templates (from Zelena/Sideswipe)
BASE_HEADER = "{3D094352621EA88C}Missions/COA_BaseMissionConfig.conf"
CRF_LOGO = "{B7385DA4504BD4D5}UI/Images/Coalition/crflogo.edds"
COA_LOBBY_PREFAB = "{6A996BBFCEB37E78}Prefabs/Modded Overrides/Coalition Lobby/COA_Lobby.et"
WEATHER_COMPONENT_ID = "64427EEC304DD9A0"      # SCR_TimeAndWeatherHandlerComponent in COA_Lobby.et
WEATHER_STATE0_ID = "66EE8AEC3605B974"         # first predefined state in COA_Lobby.et
GEAR_IDS = {"BLUFOR": "64570ACB9B419A7B", "OPFOR": "64570ACB9B11EAAD", "INDFOR": "64570ACB9AE1C9ED", "CIV": "64570ACB9A9C5C52"}
GEAR_ATTR = {"BLUFOR": "m_BLUFORGearScriptSettings", "OPFOR": "m_OPFORGearScriptSettings",
             "INDFOR": "m_INDFORGearScriptSettings", "CIV": "m_CIVILIANGearScriptSettings"}
GEAR_DEFAULTS = {
    "BLUFOR": ("", True, True, False, False),
    "OPFOR": ("", True, True, False, False),
    "INDFOR": ("{9F4A0C3FCDF3F6DE}Configs/Gearscripts/Standard/COA_GS_FIA.conf", True, True, False, True),
    "CIV": ("{6FFD426FE0C1079B}Configs/Gearscripts/Standard/COA_GS_CIV.conf", True, False, False, False),
}
SLOT_ATTR = {"BLUFOR": "m_BLUFORSlots", "OPFOR": "m_OPFORSlots", "INDFOR": "m_INDFORSlots", "CIV": "m_CIVSlots"}
TICKET_ATTR = {"BLUFOR": "m_iBLUFORTickets", "OPFOR": "m_iOPFORTickets", "INDFOR": "m_iINDFORTickets", "CIV": "m_iCIVTickets"}
FACTION_MANAGER_ID = "6A283830C79B67D2"
FACTION_MANAGER_ENTITY_ID = "632A77473B3CC3A8"
# SCR_Faction IDs in COA_Lobby.et's faction manager, in the order they are overridden
FACTION_OBJ_IDS = [("CIV", "607AA5C7A94496DA"), ("BLUFOR", "628B22E9B4056C88"),
                   ("OPFOR", "628C2D2BFC8C6447"), ("INDFOR", "628CDE9209CA3FF9")]
DEFAULT_FRIENDLY = {"CIV": ["BLUFOR", "OPFOR", "INDFOR"], "BLUFOR": [], "OPFOR": [], "INDFOR": []}
FACTION_PREFIX = {"BLUFOR": "BLUFOR", "OPFOR": "OPFOR", "INDFOR": "INDFOR", "CIV": "CIVILIAN"}

P = {
    "spawn": "{%s}PrefabsMissionMaking/SpawnPoints/SpawnPoints/%s_Spawn.et",
    "group_spawn": "{%s}PrefabsMissionMaking/SpawnPoints/SpawnPoints/Groups/%s_InitalGroupSpawn.et",
    "vehicle_spawner": "{%s}PrefabsMissionMaking/SpawnPoints/VehicleSpawner/%s_VehicleSpawner.et",
    "safestart": "{%s}PrefabsMissionMaking/GameBorders/Safestart/%s_SafestartZone.et",
}
PREFAB_GUIDS = {
    ("spawn", "BLUFOR"): "62865C82AB534D91", ("spawn", "OPFOR"): "0B3312C6940005B9",
    ("group_spawn", "BLUFOR"): "29BC39B29CEBFDC6", ("group_spawn", "OPFOR"): "8918864B69446F6E",
    ("vehicle_spawner", "BLUFOR"): "EBAE2FB05F6A0E06", ("vehicle_spawner", "OPFOR"): "8A85A4193BD323FD",
    ("safestart", "BLUFOR"): "8AE1D02BC25C93C5", ("safestart", "OPFOR"): "D8E7812AC12E29EB",
    ("safestart", "INDFOR"): "36DDFCD0D6C43000", ("safestart", "CIV"): "2150339CD6D25BF2",
}
GAME_BORDER = "{318DDE6633C34CF1}PrefabsMissionMaking/GameBorders/GameBorder.et"
DRAW_LINE = "{FE33610F66DBCB07}PrefabsMissionMaking/PolyZone/Special/DrawLinePrefab.et"
OBJECTIVE_MARKER = "{C3939B6C408F27CA}PrefabsMissionMaking/Markers/ObjectiveMarkers/BLUFOR_ObjectiveMarker.et"
WAYPOINTS = {   # type -> (class, prefab)
    "Move": ("SCR_AIWaypoint", "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et"),
    "Defend": ("SCR_DefendWaypoint", "{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et"),
    "GetIn": ("SCR_BoardingTimedWaypoint", "{B049D4C74FBC0C4D}Prefabs/AI/Waypoints/AIWaypoint_GetInNearest.et"),
    "Cycle": ("AIWaypointCycle", "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et"),
}
MAPS = {
    "Chernarus": {
        "parent": "{F513898A573B9C3F}Worlds/ChernarusMinusBeta.ent",
        "ai_world": "chernarus",
    },
    # vanilla Kolguyev (Cain); folder name and m_sTerrainName follow the Descent/Battleship missions
    "Kolguyev": {
        "parent": "{1EA95DAE3230BEB0}worlds/Cain/Cain.ent",
        "ai_world": "kolguyev",
    },
}
AI_WORLD_PRESETS = {
    # Chernarus Minus 2.1.6 navmesh (see docs/CONVENTIONS.md "AI world")
    "chernarus": ("SCR_AIWorld", "{E0A05C76552E7F58}Prefabs/AI/SCR_AIWorld.et", (11105.187, 204.696, 12307.863), """components {
 NavmeshWorldComponent "{5584F30E67F617AD}" {
  NavmeshSettings NavmeshWorld "{50FC63BEBE3973C5}" {
   NavmeshFilesConfig BaseNavmeshFilesConfig "{6A28383B60276145}" {
    NavmeshFile "{A73C56FB3B82C174}Worlds/Navmesh/soliders.nmn"
   }
  }
 }
 NavmeshWorldComponent "{5584F30EEFEE1223}" {
  NavmeshSettings ChimeraNavmeshWorld "{50FC63BEBE3973C5}" {
   NavmeshFilesConfig BaseNavmeshFilesConfig "{60CA9BE5536BF701}" {
    NavmeshFile "{496DB79F7F4F33BE}Worlds/Navmesh/BTRlike.nmn"
   }
  }
 }
 NavmeshWorldComponent "{5C8C9B750D124A63}" {
  NavmeshSettings NavmeshWorld "{5C8C9B750B60C6E2}" {
   NavmeshFilesConfig BaseNavmeshFilesConfig "{5C90BD0EC793647D}" {
    NavmeshFile "{7C34271D8EC6FA18}Worlds/Navmesh/lowres.nmn"
   }
  }
 }
}"""),
    # Kolguyev: SCR_AIWorld_Cain.et only carries the LowRes navmesh, so inherit the Game Master
    # soldier/vehicle navmesh configs (as Descent does; see docs/CONVENTIONS.md "AI world")
    "kolguyev": ("SCR_AIWorld", "{672531FAC0834C06}Prefabs/AI/SCR_AIWorld_Cain.et", (2022.696, 350.795, 7441.247), """components {
 NavmeshWorldComponent "{5584F30E67F617AD}" {
  NavmeshSettings NavmeshWorld "{50FC63BEBE3973C5}" {
   NavmeshFilesConfig BaseNavmeshFilesConfig "{6B1CA0E0AC5E0001}" : "{FA1B76B175A8AE55}Configs/Navmesh/Navmesh_GM_Cain_Soldier.conf" {
   }
  }
 }
 NavmeshWorldComponent "{5584F30EEFEE1223}" {
  NavmeshSettings ChimeraNavmeshWorld "{50FC63BEBE3973C5}" {
   NavmeshFilesConfig BaseNavmeshFilesConfig "{6B1CA0E0AC5E0002}" : "{0F41A96826AAE63B}Configs/Navmesh/Navmesh_GM_Cain_Vehicle.conf" {
   }
  }
 }
}"""),
}
DEFAULT_LAYERS = {
    "ai_world": "_INIT", "gamemode": "_INIT", "spawns": "SPAWNPOINTS", "group_spawns": "SPAWNPOINTSGROUPS",
    "vehicle_spawners": "VEHICLES", "vehicles": "VEHICLES", "routes": "AI", "waypoints": "AI", "ai_groups": "AI",
    "lines": "MARKERS", "markers": "MARKERS", "border": "GameBorderS", "safestart": "GameBorderS",
    "props": "PROPS", "anchors": "OBJECTIVES",
}
PLATFORMS = [("PC", None), ("XBOX_ONE", "PC"), ("XBOX_SERIES", "PC"), ("PS4", "PC"), ("PS5", "PC"),
             ("HEADLESS", "PC"), ("Xbox", "PC")]


def alnum(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", s or "")


def co(v: tuple) -> str:
    return " ".join(fmt(x) for x in v)


# ----------------------------------------------------------------------------- body builder
class Body:
    """Lines of an entity body with relative indentation (one space per level)."""

    def __init__(self, spec: Spec, prefix: str):
        self.l: list = []
        self.spec = spec
        self.prefix = prefix

    def line(self, d: int, text: str) -> None:
        pad = " " * d
        # one list element per physical line, so callers can re-indent bodies uniformly
        self.l.extend((pad + text.replace('\\\n"', '\\\n' + pad + '"')).split("\n"))

    def kv(self, d: int, key: str, raw: str) -> None:
        self.line(d, "%s %s" % (key, raw))

    def s(self, d: int, key: str, value: str) -> None:
        self.line(d, "%s %s" % (key, quote(value)))

    def b(self, d: int, key: str, value: bool) -> None:
        self.line(d, "%s %d" % (key, 1 if value else 0))

    def n(self, d: int, key: str, value) -> None:
        self.line(d, "%s %s" % (key, fmt(value)))

    def strings(self, d: int, key: str, values: list) -> None:
        self.line(d, key + " {")
        for v in values:
            self.line(d + 1, quote(v))
        self.line(d, "}")

    def raw(self, d: int, text: str, what: str) -> None:
        """Insert Enfusion text (snippet); parsed first so errors surface here."""
        try:
            doc = enf.parse(text, what)
        except enf.ParseError as e:
            raise SpecError("%s: snippet does not parse: %s" % (what, e))
        out = enf.write_items(doc.items, d)
        if out:
            self.l.extend(out.split("\n"))

    # generic attribute encoding (README "Attribute values")
    def attr(self, d: int, key: str, v, path: str) -> None:
        if isinstance(v, bool):
            self.b(d, key, v)
        elif isinstance(v, (int, float)):
            self.n(d, key, v)
        elif isinstance(v, str):
            self.s(d, key, v)
        elif isinstance(v, list):
            if v and all(isinstance(x, (int, float)) and not isinstance(x, bool) for x in v):
                self.kv(d, key, " ".join(fmt(x) for x in v))
            else:
                self.line(d, key + " {")
                for i, x in enumerate(v):
                    self.item(d + 1, x, "%s/%s[%d]" % (path, key, i))
                self.line(d, "}")
        elif isinstance(v, dict):
            if "merge" in v:          # 'key +{ ... }': append to the inherited array
                self.line(d, key + " +{")
                for i, x in enumerate(v["merge"]):
                    self.item(d + 1, x, "%s/%s[%d]" % (path, key, i))
                self.line(d, "}")
            elif "enum" in v:
                self.kv(d, key, str(v["enum"]))
            elif "raw" in v:
                self.kv(d, key, str(v["raw"]))
            elif "ref" in v:
                self.s(d, key, self.spec.resource(v["ref"], path + "/" + key))
            elif "class" in v:
                oid = v.get("id") or gen_id(self.prefix, path, key)
                self.line(d, '%s %s "{%s}" {' % (key, v["class"], oid))
                for k2, v2 in v.items():
                    if k2 not in ("class", "id"):
                        self.attr(d + 1, k2, v2, path + "/" + key)
                self.line(d, "}")
            else:
                self.line(d, key + " {")
                for k2, v2 in v.items():
                    self.attr(d + 1, k2, v2, path + "/" + key)
                self.line(d, "}")
        else:
            raise SpecError("%s: unsupported value for %s: %r" % (path, key, v))

    def item(self, d: int, x, path: str) -> None:
        if isinstance(x, str):
            self.line(d, quote(x))
        elif isinstance(x, bool):
            self.line(d, "1" if x else "0")
        elif isinstance(x, (int, float)):
            self.line(d, fmt(x))
        elif isinstance(x, dict) and "enum" in x:
            self.line(d, str(x["enum"]))
        elif isinstance(x, dict) and "ref" in x:
            self.line(d, quote(self.spec.resource(x["ref"], path)))
        elif isinstance(x, dict) and "class" in x:
            oid = x.get("id") or gen_id(self.prefix, path)
            self.line(d, '%s "{%s}" {' % (x["class"], oid))
            for k2, v2 in x.items():
                if k2 not in ("class", "id"):
                    self.attr(d + 1, k2, v2, path)
            self.line(d, "}")
        else:
            raise SpecError("%s: unsupported array element %r" % (path, x))

    def attrs(self, d: int, attrs: dict, path: str) -> None:
        for k, v in (attrs or {}).items():
            self.attr(d, k, v, path)


@dataclass
class GenEntity:
    cls: str
    name: Optional[str]
    prefab: Optional[str]
    body: list = field(default_factory=list)   # relative-indented lines
    layer: str = ""


# ----------------------------------------------------------------------------- generator
class Generator:
    def __init__(self, spec: Spec):
        self.spec = spec
        m = spec.mission
        self.m = m
        self.prefix = str(m.get("id_prefix") or gen_id("", m["world"])[:8]).upper()
        self.map = m["map"]
        self.layers: dict = {}      # name -> [GenEntity]
        self.layer_order: list = []
        self.names: set = set()
        self.pending: list = []     # (what, alias, message): entities skipped for a missing addon

    # -------------------------------------------------------------- helpers
    @contextmanager
    def guard(self, what: str):
        """Skip one placed entity whose prefab is a find alias of an addon that isn't installed."""
        try:
            yield
        except PendingAsset as e:
            self.pending.append((what, e.alias, str(e)))

    def body(self) -> Body:
        return Body(self.spec, self.prefix)

    def layer_for(self, kind: str, item: dict) -> str:
        return item.get("layer") or self.spec.get("layers").get(kind) or DEFAULT_LAYERS[kind]

    def add(self, e: GenEntity) -> None:
        if e.name:
            if e.name in self.names:
                raise SpecError("entity name %r defined twice" % e.name)
            self.names.add(e.name)
        if e.layer not in self.layers:
            self.layers[e.layer] = []
            self.layer_order.append(e.layer)
        self.layers[e.layer].append(e)

    def placed(self, b: Body, item: dict, what: str, pos=None) -> None:
        p = vec3(pos if pos is not None else item.get("pos"), what) if (pos is not None or "pos" in item) else None
        if p is None:
            raise SpecError("%s: pos is required" % what)
        b.kv(0, "coords", co(p))
        a = angles_of(item, what)
        if any(abs(x) > 1e-9 for x in a):
            b.kv(0, "angles", co(a))

    def finish(self, b: Body, item: dict, what: str) -> None:
        b.attrs(0, item.get("attrs"), what)
        if item.get("snippet"):
            b.raw(0, self.spec.snippet(item["snippet"], what), what)

    def res(self, v: str, what: str, exts=(".et",)) -> str:
        return self.spec.resource(v, what, exts)

    def faction_prefab(self, kind: str, faction: str) -> str:
        g = PREFAB_GUIDS.get((kind, faction))
        if g:
            return P[kind] % (g, FACTION_PREFIX[faction])
        # unknown GUID: resolve by alias through the index
        return self.res(P[kind].split("}", 1)[1] % FACTION_PREFIX[faction], kind)

    # -------------------------------------------------------------- ORBAT
    def orbat(self) -> dict:
        out = {}
        for fk, f in self.spec.get("factions").items():
            groups = []
            for i, g in enumerate(f.get("groups", [])):
                what = "[[factions.%s.groups]] #%d" % (fk, i + 1)
                rep = int(g.get("repeat", 1))
                start = int(g.get("start", 1))
                roles = expand_roles(g["roles"], what)
                for k in range(rep):
                    n = start + k
                    cs = g["callsign"].replace("{n}", str(n)) if rep > 1 or "{n}" in g["callsign"] else g["callsign"]
                    if rep > 1 and "{n}" not in g["callsign"]:
                        raise SpecError("%s: repeat needs {n} in the callsign" % what)
                    groups.append({"callsign": cs, "flag": g.get("flag", "INFANTRY"), "roles": roles,
                                   "pool": g.get("respawn_pool", "PER_GROUP"), "respawns": int(g.get("respawns", -1)),
                                   "id": g.get("id") if rep == 1 else None, "attrs": g.get("attrs", {})})
            out[fk] = groups
        return out

    def slot_total(self) -> int:
        return sum(len(g["roles"]) for gs in self.orbat().values() for g in gs)

    # -------------------------------------------------------------- paths
    def paths(self) -> dict:
        m = self.m
        total = self.slot_total()
        world_rel = "Worlds/%s/%s.ent" % (self.map, m["world"])
        hdr = "Missions/%s/%s_%s_%s%d_%s.conf" % (self.map, alnum(m["author"]), m["date"], m["mode"], total, alnum(m["name"]))
        return {"world": world_rel, "layers": world_rel[:-4] + "_Layers", "header": hdr, "total": total}

    # -------------------------------------------------------------- top-level files
    def meta(self, guid: str, rel: str, cls: str) -> str:
        out = ["MetaFileClass {", ' Name "{%s}%s"' % (guid, rel), " Configurations {"]
        for plat, parent in PLATFORMS:
            out.append("  %s %s%s {" % (cls, plat, (" : " + parent) if parent else ""))
            out.append("  }")
        out += [" }", "}"]
        return "\n".join(out)

    def world_guid(self) -> str:
        return str(self.m.get("world_guid") or gen_id(self.prefix, "world")).upper()

    def header_guid(self) -> str:
        return str(self.m.get("header_guid") or gen_id(self.prefix, "header")).upper()

    def world_text(self) -> str:
        parent = self.spec.get("world").get("parent") or MAPS.get(self.map, {}).get("parent")
        if not parent:
            raise SpecError("[world] parent is required for map %r (no preset)" % self.map)
        parent = self.spec.resource(parent, "[world] parent", (".ent",))
        return "SubScene {\n Parent %s\n}" % quote(parent)

    def header_text(self, world_rel: str, total: int) -> str:
        m = self.m
        b = self.body()
        b.kv(0, "World", quote("{%s}%s" % (self.world_guid(), world_rel)))
        b.s(0, "m_sName", "COA %s%d %s" % (m["mode"], total, m["name"]))
        b.s(0, "m_sAuthor", m["author"])
        b.s(0, "m_sDescription", m.get("description", ""))
        for k, key in (("icon", "m_sIcon"), ("loading_screen", "m_sLoadingScreen"), ("preview_image", "m_sPreviewImage")):
            b.s(0, key, self.spec.resource(m[k], "[mission] " + k, (".edds",)) if k in m else CRF_LOGO)
        b.s(0, "m_sGameMode", m["mode"])
        pc = m.get("player_count", "auto")
        b.n(0, "m_iPlayerCount", total if pc == "auto" else int(pc))
        if "start_hour" in m:
            b.n(0, "m_iStartingHours", int(m["start_hour"]))
        b.n(0, "m_iMapMarkerLimitPerPlayer", int(m.get("marker_limit", 256)))
        b.s(0, "m_sTerrainName", m.get("terrain_name", self.map))
        if m.get("author_guid"):
            b.s(0, "m_sAuthorGUID", m["author_guid"])
        b.attrs(0, m.get("header_attrs"), "[mission] header_attrs")
        return 'SCR_MissionHeader "%s" {\n%s\n}' % (BASE_HEADER, "\n".join(" " + x for x in b.l))

    # -------------------------------------------------------------- _INIT
    def ai_world(self) -> None:
        w = self.spec.get("world")
        aw = w.get("ai_world", MAPS.get(self.map, {}).get("ai_world"))
        if aw is None or aw == "none":
            return
        if isinstance(aw, str):
            aw = {"preset": aw}
        b = self.body()
        if "preset" in aw:
            if aw["preset"] not in AI_WORLD_PRESETS:
                raise SpecError("[world] ai_world preset %r unknown (have: %s)" % (aw["preset"], ", ".join(AI_WORLD_PRESETS)))
            cls, prefab, pos, comps = AI_WORLD_PRESETS[aw["preset"]]
            b.raw(0, comps, "ai_world preset")
            pos = tuple(aw.get("pos", pos))
        else:
            cls = aw.get("class", "SCR_AIWorld")
            prefab = self.res(aw["prefab"], "[world] ai_world")
            pos = tuple(aw.get("pos", (0, 0, 0)))
            if aw.get("snippet"):
                b.raw(0, self.spec.snippet(aw["snippet"], "ai_world"), "ai_world")
        b.kv(0, "coords", co(vec3(list(pos), "ai_world pos")))
        self.add(GenEntity(cls, aw.get("name"), prefab, b.l, self.layer_for("ai_world", aw)))

    def gamemode(self) -> None:
        sp = self.spec
        g = sp.get("game")
        b = self.body()
        b.line(0, "components {")
        for c in sp.get("gamemode").get("components", []):
            cid = c.get("id") or gen_id(self.prefix, "component", c["class"])
            b.line(1, '%s "{%s}" {' % (c["class"], cid))
            for k, v in c.items():
                if k not in ("class", "id"):
                    b.attr(2, k, v, "[[gamemode.components]] " + c["class"])
            b.line(1, "}")
        wt = sp.get("weather")
        if wt or self.m.get("start_hour") is not None:
            states = wt.get("states") or [{"preset": "Clear", "hour": int(self.m.get("start_hour", 12)), "minute": 0}]
            b.line(1, 'SCR_TimeAndWeatherHandlerComponent "{%s}" {' % WEATHER_COMPONENT_ID)
            b.b(2, "m_bRandomStartingWeather", wt.get("random_start", False))
            b.line(2, "m_aStartingWeatherAndTime {")
            for i, s in enumerate(states):
                sid = s.get("id") or (WEATHER_STATE0_ID if i == 0 else gen_id(self.prefix, "weather", i))
                b.line(3, 'SCR_TimeAndWeatherState "{%s}" {' % sid)
                b.s(4, "m_sWeatherPresetName", s.get("preset", "Clear"))
                b.n(4, "m_iStartingHour", int(s.get("hour", 12)))
                b.n(4, "m_iStartingMinutes", int(s.get("minute", 0)))
                b.line(3, "}")
            b.line(2, "}")
            b.b(2, "m_bRandomWeatherChanges", wt.get("random_changes", False))
            b.line(1, "}")
        b.line(0, "}")
        b.kv(0, "coords", co(tuple(g.get("position", (0, 0, 0)))))
        b.b(0, "m_bRespawnEnabled", g.get("respawn", True))
        b.b(0, "m_bWaveRespawn", g.get("wave_respawn", True))
        b.b(0, "m_bRallyPointsEnabled", g.get("rally_points", False))
        b.n(0, "m_iTimeToRespawn", int(g.get("respawn_time", 60)))
        b.n(0, "m_iRespawnCutoffMinutes", int(g.get("respawn_cutoff_minutes", 0)))
        b.n(0, "m_eRespawnMode", int(g.get("respawn_mode", 0)))
        b.n(0, "m_iTimeLimitMinutes", int(g.get("time_limit_minutes", 60)))
        b.b(0, "m_bLockUnusedSlots", g.get("lock_unused_slots", True))
        ss = int(g.get("safestart_time_limit", 0))
        b.b(0, "m_bUseSafestartTimeLimit", ss > 0)
        b.n(0, "m_iSafestartTimeLimit", ss)
        b.b(0, "m_bUseCVON", g.get("use_cvon", True))
        brief = sp.items("briefing")
        if brief:
            b.line(0, "m_aMissionDescriptors {")
            for i, d in enumerate(brief):
                did = d.get("id") or gen_id(self.prefix, "briefing", d.get("title", i))
                b.line(1, 'COA_MissionDescriptor "{%s}" {' % did)
                b.s(2, "m_sTitle", d.get("title", ""))
                b.s(2, "m_sTextData", d.get("text", "").strip("\n"))
                b.strings(2, "m_aFactionKeys", d.get("factions", []))
                b.b(2, "m_bShowForAnyFaction", d.get("show_for_any", not d.get("factions")))
                b.line(1, "}")
            b.line(0, "}")
        ratio = g.get("faction_ratio", [1, 1])
        keys = g.get("faction_keys", ["BLU", "OPF"])
        b.n(0, "m_iFactionOneRatio", int(ratio[0]))
        b.n(0, "m_iFactionTwoRatio", int(ratio[1]))
        b.s(0, "m_sFactionOneKey", keys[0])
        b.s(0, "m_sFactionTwoKey", keys[1])
        orbat = self.orbat()
        for fk in ("BLUFOR", "OPFOR", "INDFOR", "CIV"):
            groups = orbat.get(fk)
            if not groups:
                continue
            b.line(0, SLOT_ATTR[fk] + " {")
            for gr in groups:
                gid = gr["id"] or gen_id(self.prefix, "slots", fk, gr["callsign"])
                b.line(1, 'COA_SlottingGroup "{%s}" {' % gid)
                b.s(2, "m_sCallsign", gr["callsign"])
                b.kv(2, "m_FlagType", gr["flag"])
                b.line(2, "m_aSlots {")
                for r in gr["roles"]:
                    b.line(3, r)
                b.line(2, "}")
                b.kv(2, "m_eRespawnPoolType", gr["pool"])
                b.n(2, "m_iGroupRespawns", gr["respawns"])
                b.attrs(2, gr["attrs"], "group " + gr["callsign"])
                b.line(1, "}")
            b.line(0, "}")
        facs = sp.get("factions")
        for fk in ("BLUFOR", "OPFOR", "INDFOR", "CIV"):
            b.n(0, TICKET_ATTR[fk], int(facs.get(fk, {}).get("tickets", 0)))
        b.n(0, "m_fMissionTimeScale", g.get("time_scale", 1))
        for fk in ("BLUFOR", "OPFOR", "INDFOR", "CIV"):
            f = facs.get(fk, {})
            dgs, dbft, dlead, dgi, drto = GEAR_DEFAULTS[fk]
            gs = f.get("gearscript", dgs)
            if not gs:
                if orbat.get(fk):
                    raise SpecError("[factions.%s] has groups but no gearscript" % fk)
                continue
            gs = self.spec.resource(gs, "[factions.%s] gearscript" % fk, (".conf",))
            b.line(0, '%s COA_GearScriptContainer "{%s}" {' % (GEAR_ATTR[fk], f.get("gearscript_id", GEAR_IDS[fk])))
            b.s(1, "m_rGearScript", gs)
            b.b(1, "m_bEnableBFT", f.get("bft", dbft))
            b.b(1, "m_bEnableLeadershipRadios", f.get("leadership_radios", dlead))
            b.b(1, "m_bEnableGIRadios", f.get("gi_radios", dgi))
            b.b(1, "m_bEnableRTORadios", f.get("rto_radios", drto))
            b.line(0, "}")
        b.attrs(0, g.get("attrs"), "[game] attrs")
        # faction manager child override (friendly factions)
        b.line(0, "{")
        b.line(1, 'SCR_FactionManager "%s" {' % FACTION_MANAGER_ID)
        b.kv(2, "ID", quote(FACTION_MANAGER_ENTITY_ID))
        b.line(2, "Factions {")
        for fk, oid in FACTION_OBJ_IDS:
            fr = facs.get(fk, {}).get("friendly", DEFAULT_FRIENDLY[fk])
            b.line(3, 'SCR_Faction "{%s}" {' % oid)
            b.strings(4, "m_aFriendlyFactionsIds", fr)
            b.line(3, "}")
        b.line(2, "}")
        b.line(1, "}")
        b.line(0, "}")
        self.add(GenEntity("COA_Gamemode", "COA_Lobby", COA_LOBBY_PREFAB, b.l, self.layer_for("gamemode", {})))

    # -------------------------------------------------------------- placed entities
    def spawns(self) -> None:
        for i, it in enumerate(self.spec.items("spawns")):
            what = "[[spawns]] #%d" % (i + 1)
            with self.guard(what):
                fac = it.get("faction")
                prefab = self.res(it["prefab"], what) if "prefab" in it else self.faction_prefab("spawn", fac)
                b = self.body()
                self.placed(b, it, what)
                self.finish(b, it, what)
                self.add(GenEntity(it.get("class", "COA_StaticSpawnPoint"), it.get("name"), prefab, b.l, self.layer_for("spawns", it)))

    def group_spawns(self) -> None:
        for i, it in enumerate(self.spec.items("group_spawns")):
            what = "[[group_spawns]] #%d" % (i + 1)
            with self.guard(what):
                prefab = self.res(it["prefab"], what) if "prefab" in it else self.faction_prefab("group_spawn", it.get("faction"))
                b = self.body()
                self.placed(b, it, what)
                b.s(0, "m_sCallsignOfGroupToSpawn", it["callsign"])
                self.finish(b, it, what)
                self.add(GenEntity(it.get("class", "COA_GroupSpawnPoint"), it.get("name"), prefab, b.l, self.layer_for("group_spawns", it)))

    def vehicle_spawners(self) -> None:
        for i, it in enumerate(self.spec.items("vehicle_spawners")):
            what = "[[vehicle_spawners]] #%d" % (i + 1)
            with self.guard(what):
                fac = it["faction"]
                prefab = self.res(it["prefab"], what) if "prefab" in it else self.faction_prefab("vehicle_spawner", fac)
                b = self.body()
                self.placed(b, it, what)
                b.s(0, "m_rVehicle", self.res(it["vehicle"], what + " vehicle"))
                b.s(0, "m_sFactionKey", fac)
                if "tickets_per_respawn" in it:
                    b.n(0, "m_iTicketsPerRespawn", int(it["tickets_per_respawn"]))
                self.finish(b, it, what)
                self.add(GenEntity(it.get("class", "COA_VehicleSpawner"), it.get("name"), prefab, b.l, self.layer_for("vehicle_spawners", it)))

    def vehicles(self) -> None:
        for i, it in enumerate(self.spec.items("vehicles")):
            what = "[[vehicles]] #%d (%s)" % (i + 1, it.get("name"))
            with self.guard(what):
                b = self.body()
                self.placed(b, it, what)
                self.finish(b, it, what)
                self.add(GenEntity(it.get("class", "Vehicle"), it.get("name"), self.res(it["prefab"], what), b.l, self.layer_for("vehicles", it)))

    def _waypoint(self, it: dict, what: str, kind: str) -> GenEntity:
        wtype = it.get("type", "Move")
        if wtype not in WAYPOINTS and "prefab" not in it:
            raise SpecError("%s: waypoint type %r unknown (%s) - or give prefab + class" % (what, wtype, ", ".join(WAYPOINTS)))
        cls, prefab = WAYPOINTS.get(wtype, (it.get("class", "SCR_AIWaypoint"), None))
        cls = it.get("class", cls)
        prefab = self.res(it["prefab"], what) if "prefab" in it else prefab
        b = self.body()
        self.placed(b, it, what)
        if wtype == "Cycle" or "waypoints" in it:
            names = expand_names(it.get("waypoints", []))
            b.strings(0, "CycleWaypoints", names)
        if "radius" in it:
            b.n(0, "CompletionRadius", it["radius"])
        self.finish(b, it, what)
        return GenEntity(cls, it.get("name"), prefab, b.l, self.layer_for(kind, it))

    def routes(self) -> None:
        for i, r in enumerate(self.spec.items("routes")):
            what = "[[routes]] #%d (%s)" % (i + 1, r.get("name"))
            pts = r.get("points", [])
            if len(pts) < 2:
                raise SpecError("%s: needs at least 2 points" % what)
            digits = int(r.get("digits", 2))
            names = []
            first = None
            for k, p in enumerate(pts):
                nm = "%s_%0*d" % (r["name"], digits, k + 1)
                names.append(nm)
                it = {"name": nm, "type": r.get("type", "Move"), "layer": r.get("layer")}
                for key in ("radius", "prefab", "class", "attrs"):
                    if key in r:
                        it[key] = r[key]
                if isinstance(p, dict):     # { pos = [...], angles = [...], radius = .. }
                    bad = [x for x in p if x not in ("pos", "angles", "yaw", "radius", "attrs")]
                    if bad:
                        raise SpecError("%s point %d: unknown key(s) %s" % (what, k + 1, ", ".join(bad)))
                    it.update(p)
                else:
                    it["pos"] = p
                first = first or it["pos"]
                self.add(self._waypoint({k2: v for k2, v in it.items() if v is not None}, what, "routes"))
            if r.get("cycle", True):
                cn = r.get("cycle_name", r["name"] + "_cycle")
                self.add(self._waypoint({"name": cn, "pos": first, "type": "Cycle", "waypoints": names,
                                         **({"layer": r["layer"]} if r.get("layer") else {})}, what, "routes"))

    def waypoints(self) -> None:
        for i, it in enumerate(self.spec.items("waypoints")):
            self.add(self._waypoint(it, "[[waypoints]] #%d (%s)" % (i + 1, it.get("name")), "waypoints"))

    def ai_groups(self) -> None:
        for i, it in enumerate(self.spec.items("ai_groups")):
            what = "[[ai_groups]] #%d (%s)" % (i + 1, it.get("name"))
            with self.guard(what):
                b = self.body()
                self.placed(b, it, what)
                if "faction" in it:
                    b.s(0, "m_faction", it["faction"])
                if it.get("members"):
                    b.strings(0, "m_aUnitPrefabSlots", [self.res(x, what + " member") for x in it["members"]])
                if it.get("waypoints"):
                    b.strings(0, "m_aStaticWaypoints", expand_names(it["waypoints"]))
                if it.get("vehicles"):
                    b.strings(0, "m_aStaticVehicles", expand_names(it["vehicles"]))
                self.finish(b, it, what)
                self.add(GenEntity(it.get("class", "SCR_AIGroup"), it.get("name"), self.res(it["prefab"], what), b.l, self.layer_for("ai_groups", it)))

    def _points(self, b: Body, origin: tuple, pts: list, name: str, what: str) -> None:
        b.line(0, "Points {")
        for k, p in enumerate(pts):
            v = vec3(p, what)
            rel = (round(v[0] - origin[0], 3), 0.0, round(v[2] - origin[2], 3))
            b.line(1, 'ShapePoint "{%s}" {' % gen_id(self.prefix, "shape", name, k))
            b.kv(2, "Position", co(rel))
            b.line(1, "}")
        b.line(0, "}")

    def lines(self) -> None:
        for i, it in enumerate(self.spec.items("lines")):
            what = "[[lines]] #%d (%s)" % (i + 1, it.get("name"))
            pts = it.get("points")
            if "points_file" in it:
                with open(os.path.join(self.spec.dir, it["points_file"]), encoding="utf-8") as f:
                    pts = json.load(f)
            if not pts or len(pts) < 2:
                raise SpecError("%s: needs points (>= 2)" % what)
            origin = vec3(it.get("pos", pts[0]), what)
            b = self.body()
            b.kv(0, "coords", co(origin))
            self._points(b, origin, pts, it.get("name") or "line%d" % i, what)
            if "factions" in it:
                b.strings(0, "m_aVisibleForFactions", it["factions"])
            if "color" in it:
                b.kv(0, "m_cPolygonBorderColor", " ".join(fmt(x) for x in it["color"]))
            if "width" in it:
                b.n(0, "m_fPolygonBorderWidth", it["width"])
            b.b(0, "m_bLineMode", it.get("line_mode", True))
            self.finish(b, it, what)
            prefab = self.res(it["prefab"], what) if "prefab" in it else DRAW_LINE
            self.add(GenEntity(it.get("class", "COA_BorderBase"), it.get("name"), prefab, b.l, self.layer_for("lines", it)))

    def markers(self) -> None:
        for i, it in enumerate(self.spec.items("markers")):
            what = "[[markers]] #%d (%s)" % (i + 1, it.get("name"))
            b = self.body()
            self.placed(b, it, what)
            if "color" in it:
                b.kv(0, "m_MarkerColor", " ".join(fmt(x) for x in it["color"]))
            if "description" in it:
                b.s(0, "m_sDescription", it["description"])
            if "factions" in it:
                b.strings(0, "m_aVisibleForFactions", it["factions"])
            if "show_for_any" in it:
                b.b(0, "m_bShowForAnyFaction", it["show_for_any"])
            self.finish(b, it, what)
            prefab = self.res(it["prefab"], what) if "prefab" in it else OBJECTIVE_MARKER
            self.add(GenEntity(it.get("class", "COA_ManualMarker"), it.get("name"), prefab, b.l, self.layer_for("markers", it)))

    def _shape(self, it: dict, what: str, cls: str, prefab: str, kind: str) -> None:
        c = vec3(it["pos"], what)
        if "points" in it:
            pts = it["points"]
        elif "radius" in it:
            r = float(it["radius"])
            n = int(it.get("sides", 8))
            pts = [[c[0] + r * math.sin(2 * math.pi * k / n), c[2] + r * math.cos(2 * math.pi * k / n)] for k in range(n)]
        else:
            raise SpecError("%s: needs points or radius" % what)
        b = self.body()
        b.kv(0, "coords", co(c))
        a = angles_of(it, what)
        if any(abs(x) > 1e-9 for x in a):
            b.kv(0, "angles", co(a))
        self._points(b, c, pts, it.get("name") or kind, what)
        self.finish(b, it, what)
        self.add(GenEntity(it.get("class", cls), it.get("name"), prefab, b.l, self.layer_for(kind, it)))

    def borders(self) -> None:
        bd = self.spec.data.get("border")
        if bd:
            self._shape(bd, "[border]", "COA_GameBorder", self.res(bd["prefab"], "[border]") if "prefab" in bd else GAME_BORDER, "border")
        for i, it in enumerate(self.spec.items("safestart")):
            what = "[[safestart]] #%d (%s)" % (i + 1, it.get("name"))
            prefab = self.res(it["prefab"], what) if "prefab" in it else self.faction_prefab("safestart", it["faction"])
            self._shape(it, what, "COA_SafestartBorder", prefab, "safestart")

    def props(self) -> None:
        for i, it in enumerate(self.spec.items("props")):
            what = "[[props]] #%d%s" % (i + 1, (" (%s)" % it["name"]) if "name" in it else "")
            with self.guard(what):
                b = self.body()
                self.placed(b, it, what)
                self.finish(b, it, what)
                self.add(GenEntity(it.get("class", "GenericEntity"), it.get("name"), self.res(it["prefab"], what), b.l, self.layer_for("props", it)))

    def anchors(self) -> None:
        for i, it in enumerate(self.spec.items("anchors")):
            what = "[[anchors]] #%d (%s)" % (i + 1, it.get("name"))
            if not it.get("name"):
                raise SpecError("%s: anchors need a name" % what)
            b = self.body()
            self.placed(b, it, what)
            b.attrs(0, it.get("attrs"), what)
            self.add(GenEntity(it.get("class", "GameEntity"), it["name"], None, b.l, self.layer_for("anchors", it)))

    # -------------------------------------------------------------- layer text
    @staticmethod
    def layer_text(ents: list) -> str:
        out = []
        i = 0
        while i < len(ents):
            e = ents[i]
            j = i + 1
            while j < len(ents) and ents[j].cls == e.cls and ents[j].prefab == e.prefab:
                j += 1
            if j - i >= 2:
                out.append("$grp %s%s {" % (e.cls, (" : " + quote(e.prefab)) if e.prefab else ""))
                for g in ents[i:j]:
                    out.append(" %s{" % (g.name + " " if g.name else ""))
                    out += ["  " + x for x in g.body]
                    out.append(" }")
                out.append("}")
            else:
                hdr = e.cls + ((" " + e.name) if e.name else "") + ((" : " + quote(e.prefab)) if e.prefab else "")
                out.append(hdr + " {")
                out += [" " + x for x in e.body]
                out.append("}")
            i = j
        return "\n".join(out)

    # -------------------------------------------------------------- main
    def generate(self) -> dict:
        """Return ``{rel_path: text}`` for every file the spec owns."""
        pth = self.paths()
        self.ai_world()
        self.gamemode()
        self.spawns()
        self.group_spawns()
        self.vehicle_spawners()
        self.vehicles()
        self.routes()
        self.waypoints()
        self.ai_groups()
        self.lines()
        self.markers()
        self.borders()
        self.props()
        self.anchors()
        files = {
            pth["world"]: self.world_text(),
            pth["world"] + ".meta": self.meta(self.world_guid(), pth["world"], "ENTResourceClass"),
            pth["header"]: self.header_text(pth["world"], pth["total"]),
            pth["header"] + ".meta": self.meta(self.header_guid(), pth["header"], "CONFResourceClass"),
        }
        for ln in self.layer_order:
            files["%s/%s.layer" % (pth["layers"], ln)] = self.layer_text(self.layers[ln])
        return files


def generate(spec: Spec) -> dict:
    return Generator(spec).generate()
