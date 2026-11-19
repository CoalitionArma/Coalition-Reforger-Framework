"""Mission validator. See docs/CONVENTIONS.md for the rules and their sources."""
from __future__ import annotations

import datetime
import os
import re
from dataclasses import dataclass
from typing import Optional

from . import enf
from .assets import AssetIndex
from .config import Config
from .mission import (FACTIONS, FACTION_GEAR_ATTRS, FACTION_SLOT_ATTRS, Entity, Mission)
from .prefabs import PrefabSource
from . import scripts

ERROR, WARNING, INFO = "error", "warning", "info"

# (owning class or "*", attribute): value(s) name an entity in the world.
# "*" = any block declaring the attribute.  Sources: CRF Scripts, FindEntityByName call sites.
ENTITY_REF_ATTRS = [
    ("CRF_InterdictionGamemodeComponent", "m_sConvoyGroupName"),
    ("CRF_InterdictionGamemodeComponent", "m_sHQAnchorName"),
    ("CRF_InterdictionGamemodeComponent", "m_sBaseAnchorName"),
    ("CRF_EvacuationGamemodeComponent", "m_sSiteName"),
    ("CRF_EvacuationGamemodeComponent", "m_sLoadingBayName"),
    ("CRF_EvacuationGamemodeComponent", "m_sExtractionName"),
    ("CRF_PayloadGamemodeComponent", "m_sRouteName"),
    ("CRF_PayloadGamemodeComponent", "m_sPayloadName"),
    ("CRF_SlasherGamemodeComponent", "m_sRadioName"),
    ("CRF_SlasherGamemodeComponent", "m_sExtractionName"),
    ("CRF_TheTurnGamemodeComponent", "m_sLandingZoneName"),
    ("CRF_TheTurnGamemodeComponent", "m_sRefugeePointName"),
    ("CRF_TankBattlePoint", "m_sEntityName"),
    ("CRF_TankBattleSide", "m_aSpawnEntities"),
    ("*", "m_sTeamABaseName"),
    ("*", "m_sTeamBBaseName"),
    ("*", "m_sRandomCenterEntityName"),
    ("*", "m_sDefenderHomeFlagName"),
    ("*", "m_sHunterReturnSpawnName"),
    ("*", "m_sObjectNameToTeleportTo"),
    ("*", "m_sSpawnEntityName"),
    ("*", "m_sTransponderEntityName"),
    ("*", "m_sMarkerEntityName"),
    ("*", "m_sStageBoundaryName"),
    ("*", "m_sTargetEntityName"),
    # entity-level arrays (SCR_AIGroup, AIWaypointCycle)
    ("*", "m_aStaticWaypoints"),
    ("*", "m_aStaticVehicles"),
    ("*", "CycleWaypoints"),
]
ARRAY_REF_ATTRS = {"m_aStaticWaypoints", "m_aStaticVehicles", "CycleWaypoints", "m_aSpawnEntities"}

HEADER_RE = re.compile(r"^(?P<author>[^_]+)_(?P<mmdd>\d{4})_(?P<mode>[A-Za-z]+)(?P<count>\d+)_(?P<name>[^_]+)\.conf$")
UUID_RE = re.compile(r"^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$")
FACTION_PREFIX = re.compile(r"(?:^|/)(BLUFOR|OPFOR|INDFOR|CIVILIAN|CIV)_[^/]*$")
SOLDIER_NAVMESH_COMPONENT = "5584F30E67F617AD"


def alnum(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "", s or "")


@dataclass
class Diagnostic:
    severity: str
    code: str
    where: str
    message: str

    def format(self) -> str:
        return "%-7s %s: [%s] %s" % (self.severity, self.where, self.code, self.message)


class Context:
    """Shared, cached lookups for validation (index, enums, script attribute defaults)."""

    def __init__(self, cfg: Config, index: AssetIndex):
        self.cfg = cfg
        self.index = index
        self.prefabs = PrefabSource(index)
        roots = [cfg.lobby, cfg.crf] + list(cfg.data["paths"].get("extra_projects", []))
        self.enums = scripts.scan_enums(roots)
        self.attr_defaults = scripts.scan_attributes(roots)
        extra = [x.upper() for x in cfg.section("conventions").get("extra_modes", [])]
        self.modes = set(self.enums.get("COA_EGamemode", [])) | set(extra)

    def enum(self, name: str) -> set:
        return set(self.enums.get(name, []))


class Validator:
    def __init__(self, ctx: Context, mission: Mission):
        self.ctx = ctx
        self.m = mission
        self.diags: list = []

    # ------------------------------------------------------------------ helpers
    def add(self, sev: str, code: str, where: str, msg: str) -> None:
        self.diags.append(Diagnostic(sev, code, where, msg))

    def at(self, f, line: int) -> str:
        return "%s:%d" % (f.display, line)

    def run(self) -> list:
        for sev, where, msg in self.m.notes:
            self.add(sev, "LOAD", where, msg)
        for f in self.m.files:
            if f.error:
                self.add(ERROR, "PARSE", "%s:%d" % (f.display, f.error.line), f.error.msg)
        self.check_refs()
        self.check_metas()
        self.check_header()
        self.check_slots()
        self.check_spawns()
        self.check_ai_world()
        self.check_names()
        self.check_gearscripts()
        self.check_ids()
        self.check_weather()
        self.check_borders()
        order = {ERROR: 0, WARNING: 1, INFO: 2}
        self.diags.sort(key=lambda d: (order[d.severity], d.where))
        return self.diags

    # ------------------------------------------------------------------ resource refs
    def check_refs(self) -> None:
        seen = set()
        for f in self.m.files:
            if f.doc is None:
                continue
            for t in f.doc.iter_tokens():
                if t.kind != enf.STRING:
                    continue
                if not enf.RESOURCE_RE.match(t.value):
                    continue
                key = (f.rel, t.line, t.value)
                if key in seen:
                    continue
                seen.add(key)
                if f.kind.endswith("meta") and t.line <= 3:
                    continue   # the meta's own Name: checked in check_metas
                r = self.ctx.index.resolve(t.value)
                if r.ok:
                    continue
                # a known GUID with a stale path still loads (resolution is by GUID)
                sev = WARNING if r.status in ("case", "path_mismatch") else ERROR
                code = {"unknown_guid": "REF-UNKNOWN", "path_mismatch": "REF-PATH", "not_in_deps": "REF-DEPS",
                        "case": "REF-CASE", "malformed": "REF-MALFORMED"}.get(r.status, "REF")
                self.add(sev, code, self.at(f, t.line), "%s: %s" % (t.value, r.message))

    # ------------------------------------------------------------------ metas / header <-> world
    def _meta_name(self, mf) -> Optional[tuple]:
        if mf is None or mf.doc is None or not mf.doc.blocks:
            return None
        b = mf.doc.blocks[0]
        ln = b.get_line("Name")
        if ln is None or not ln.values:
            return None
        m = enf.RESOURCE_RE.match(ln.values[0].value)
        if not m:
            return None
        return m.group(1).upper(), m.group(2), ln.line

    def check_metas(self) -> None:
        for mf, target in ((self.m.world_meta, self.m.world), (self.m.header_meta, self.m.header)):
            if target is None:
                continue
            if mf is None:
                if target is self.m.header:
                    self.add(ERROR, "META-MISSING", target.display, "header has no .meta file")
                continue
            nm = self._meta_name(mf)
            if nm is None:
                self.add(ERROR, "META-NAME", mf.display, "meta has no Name \"{GUID}path\"")
                continue
            guid, path, line = nm
            if path != target.rel:
                sev = WARNING
                self.add(sev, "META-PATH", self.at(mf, line), "meta Name path %s does not match the file %s" % (path, target.rel))
            others = [(p, a) for p, a in self.ctx.index.by_guid.get(guid, []) if p.lower() != target.rel.lower()]
            if others:
                hint = ""
                if target.kind == "header" and others[0][0].startswith("Missions/"):
                    hint = " - if the header was renamed (e.g. the slot count changed), delete or git-mv the old file"
                self.add(ERROR, "META-GUID-DUP", self.at(mf, line), "GUID {%s} is also used by %s (%s)%s" %
                         (guid, others[0][0], self.ctx.index.addon_name(others[0][1]), hint))
            # resource class matches extension
            exp = "ENTResourceClass" if target.kind == "world" else "CONFResourceClass"
            confs = mf.doc.blocks[0].child("Configurations")
            if confs is not None and confs.blocks and confs.blocks[0].key != exp:
                self.add(WARNING, "META-CLASS", self.at(mf, confs.blocks[0].line),
                         "expected %s configurations, found %s" % (exp, confs.blocks[0].key))

    def check_header(self) -> None:
        hb = self.m.header_block()
        if hb is None:
            return
        hf = self.m.header
        base = self.ctx.cfg.section("conventions").get("base_header")
        if hb.key != "SCR_MissionHeader":
            self.add(ERROR, "HDR-CLASS", self.at(hf, hb.line), "header class is %s, expected SCR_MissionHeader" % hb.key)
        if base and hb.parent_ref != base:
            self.add(WARNING, "HDR-BASE", self.at(hf, hb.line), "header does not inherit %s" % base)
        # World <-> world meta
        wl = hb.get_line("World")
        wguid = self.m.world_guid()
        if wl is None or not wl.values:
            self.add(ERROR, "HDR-WORLD", self.at(hf, hb.line), "header has no World")
        else:
            m = enf.RESOURCE_RE.match(wl.values[0].value)
            if m:
                if wguid and m.group(1).upper() != wguid:
                    self.add(ERROR, "HDR-WORLD", self.at(hf, wl.line), "World GUID {%s} != world .ent.meta GUID {%s}" % (m.group(1).upper(), wguid))
                if m.group(2) != self.m.world.rel:
                    self.add(ERROR if m.group(2).lower() != self.m.world.rel.lower() else WARNING, "HDR-WORLD",
                             self.at(hf, wl.line), "World path %s != %s" % (m.group(2), self.m.world.rel))
        # file name / m_sName conventions
        rel = hf.rel
        parts = rel.split("/")
        fname = parts[-1]
        hmap = parts[1] if len(parts) == 3 and parts[0] == "Missions" else None
        if hmap is None:
            self.add(ERROR, "NAME-PATH", hf.display, "header must be Missions/<Map>/<file>.conf, is %s" % rel)
        wparts = self.m.world.rel.split("/")
        wmap = wparts[1] if len(wparts) >= 3 and wparts[0] == "Worlds" else None
        if hmap and wmap and hmap != wmap:
            self.add(WARNING, "NAME-MAP", hf.display, "header folder Missions/%s differs from world folder Worlds/%s" % (hmap, wmap))
        terrain = hb.get("m_sTerrainName")
        if terrain is not None and wmap and terrain != wmap:
            self.add(WARNING, "NAME-TERRAIN", self.at(hf, hb.get_line("m_sTerrainName").line),
                     "m_sTerrainName %r differs from the world folder %r (the Lobby plugin uses the folder)" % (terrain, wmap))
        total = sum(self.m.slot_counts().values())
        mode = hb.get("m_sGameMode")
        sname = hb.get("m_sName")
        fm = HEADER_RE.match(fname)
        if not fm:
            self.add(ERROR, "NAME-FILE", hf.display,
                     "header file name %s does not match <Author>_<MMDD>_<MODE><count>_<Name>.conf (alphanumeric parts)" % fname)
        else:
            for part in ("author", "name"):
                if not fm.group(part).isalnum():
                    self.add(ERROR, "NAME-FILE", hf.display, "%s part %r of the file name must be alphanumeric" % (part, fm.group(part)))
            mmdd = fm.group("mmdd")
            try:
                datetime.date(2000, int(mmdd[:2]), int(mmdd[2:]))
            except ValueError:
                self.add(WARNING, "NAME-DATE", hf.display, "MMDD %s is not a valid date" % mmdd)
            fmode = fm.group("mode")
            if fmode != fmode.upper():
                self.add(ERROR, "NAME-FILE", hf.display, "mode %r in the file name must be upper case" % fmode)
            if mode is not None and fmode != mode:
                self.add(ERROR, "NAME-MODE", hf.display, "file name mode %s != m_sGameMode %s" % (fmode, mode))
            fcount = int(fm.group("count"))
            if total and fcount != total:
                self.add(ERROR, "NAME-COUNT", hf.display, "file name says %d slots, the ORBAT has %d" % (fcount, total))
            author = hb.get("m_sAuthor")
            if author is not None and alnum(author) != fm.group("author"):
                self.add(WARNING, "NAME-AUTHOR", hf.display, "file name author %r != alphanumeric m_sAuthor %r" % (fm.group("author"), alnum(author)))
            if sname is not None:
                sm = re.match(r"^COA ([A-Za-z]+)(\d+) (.+)$", sname)
                if not sm:
                    self.add(ERROR, "NAME-TITLE", self.at(hf, hb.get_line("m_sName").line),
                             "m_sName %r must be \"COA <MODE><count> <Name>\"" % sname)
                else:
                    if alnum(sm.group(3)) != fm.group("name"):
                        self.add(ERROR, "NAME-TITLE", self.at(hf, hb.get_line("m_sName").line),
                                 "m_sName name %r does not match file name part %r" % (sm.group(3), fm.group("name")))
                    if sm.group(1) != fmode or int(sm.group(2)) != fcount:
                        self.add(ERROR, "NAME-TITLE", self.at(hf, hb.get_line("m_sName").line),
                                 "m_sName %r disagrees with the file name (%s%d)" % (sname, fmode, fcount))
        if sname is not None and total:
            sm = re.match(r"^COA ([A-Za-z]+)(\d+) ", sname)
            if sm and int(sm.group(2)) != total:
                self.add(ERROR, "NAME-COUNT", self.at(hf, hb.get_line("m_sName").line),
                         "m_sName says %s slots, the ORBAT has %d" % (sm.group(2), total))
        if mode is None:
            self.add(ERROR, "HDR-MODE", self.at(hf, hb.line), "m_sGameMode is not set")
        elif mode not in self.ctx.modes:
            sev = ERROR if not mode.isalnum() or mode != mode.upper() else WARNING
            self.add(sev, "HDR-MODE", self.at(hf, hb.get_line("m_sGameMode").line),
                     "m_sGameMode %r is not a COA_EGamemode value or known mode string" % mode)
        pc = hb.get("m_iPlayerCount")
        if total and (pc is None or pc != str(total)):
            self.add(WARNING, "HDR-PLAYERCOUNT", self.at(hf, hb.get_line("m_iPlayerCount").line if pc is not None else hb.line),
                     "m_iPlayerCount is %s, the ORBAT has %d slots" % (pc, total))
        ag = hb.get("m_sAuthorGUID")
        if ag is None:
            self.add(WARNING, "HDR-AUTHORGUID", self.at(hf, hb.line), "m_sAuthorGUID not set (mission maker won't get admin)")
        elif not UUID_RE.match(ag):
            self.add(WARNING, "HDR-AUTHORGUID", self.at(hf, hb.get_line("m_sAuthorGUID").line), "m_sAuthorGUID %r is not a UUID" % ag)
        for key in ("m_iStartingHours",):
            v = hb.get(key)
            if v is not None:
                try:
                    if not 0 <= int(float(v)) <= 23:
                        raise ValueError
                except ValueError:
                    self.add(WARNING, "HDR-HOUR", self.at(hf, hb.get_line(key).line), "%s %s out of range 0-23" % (key, v))

    # ------------------------------------------------------------------ ORBAT
    def check_slots(self) -> None:
        gm = self.m.gamemode()
        if gm is None:
            self.add(ERROR, "GM-MISSING", self.m.world.display, "no COA_Gamemode (COA_Lobby) entity in the world")
            return
        roles = self.ctx.enum("COA_EGearRole")
        flags = self.ctx.enum("COA_EFlagType")
        pools = self.ctx.enum("COA_ERespawnPoolType")
        seen = {}
        groups = self.m.slot_groups()
        if not groups:
            self.add(ERROR, "SLOT-NONE", gm.where, "the game mode has no slotting groups (m_BLUFORSlots/...)")
        for g in groups:
            w = self.at(g.file, g.block.line)
            if not g.callsign:
                self.add(WARNING, "SLOT-CALLSIGN", w, "%s slotting group has no m_sCallsign" % g.faction)
            key = (g.faction, g.callsign)
            if g.callsign and key in seen:
                self.add(WARNING, "SLOT-DUP", w, "%s callsign %r used twice (first at line %d)" % (g.faction, g.callsign, seen[key]))
            seen.setdefault(key, g.block.line)
            if not g.roles:
                self.add(WARNING, "SLOT-EMPTY", w, "group %r has no slots" % g.callsign)
            for role, tok in g.roles:
                if roles and role not in roles:
                    self.add(ERROR, "SLOT-ROLE", self.at(g.file, tok.line), "role %s (group %r) is not a COA_EGearRole" % (role, g.callsign))
            if g.flag is not None and flags and g.flag not in flags:
                self.add(ERROR, "SLOT-FLAG", self.at(g.file, g.block.get_line("m_FlagType").line),
                         "flag %s (group %r) is not a COA_EFlagType" % (g.flag, g.callsign))
            pool = g.block.get("m_eRespawnPoolType")
            if pool is not None and pools and pool not in pools:
                self.add(ERROR, "SLOT-POOL", self.at(g.file, g.block.get_line("m_eRespawnPoolType").line),
                         "m_eRespawnPoolType %s is not a COA_ERespawnPoolType" % pool)
            for rr in (g.block.child("m_aRoleRespawns").blocks if g.block.child("m_aRoleRespawns") else []):
                r = rr.get("m_Role")
                if r and roles and r not in roles:
                    self.add(ERROR, "SLOT-ROLE", self.at(g.file, rr.line), "m_aRoleRespawns role %s is not a COA_EGearRole" % r)

    # ------------------------------------------------------------------ spawn points
    def entity_faction(self, e: Entity) -> Optional[str]:
        blocks = [e.block] + self.ctx.prefabs.chain(e.prefab)
        for b in blocks:
            sps = b.child("m_SpawnPointSettings")
            if sps is not None:
                v = sps.get("m_iSpawnPointFaction")
                if v:
                    return v
            v = b.get("m_sFactionKey")
            if v:
                return v
        for b in blocks:
            comps = b.child("components")
            if comps is None:
                continue
            for c in comps.blocks:
                if c.key == "SCR_FactionAffiliationComponent":
                    v = c.get('faction affiliation')
                    if v:
                        return v
        for ref in [e.prefab] + [b.parent_ref for b in blocks if b.parent_ref]:
            if ref:
                m = FACTION_PREFIX.search(ref.split("}", 1)[-1].replace(".et", ""))
                if m:
                    return "CIV" if m.group(1) in ("CIVILIAN", "CIV") else m.group(1)
        return None

    def check_spawns(self) -> None:
        counts = self.m.slot_counts()
        statics = {f: [] for f in FACTIONS}
        for e in self.m.entities:
            if e.cls == "COA_StaticSpawnPoint":
                fac = self.entity_faction(e)
                if fac is None:
                    self.add(WARNING, "SPAWN-FACTION", e.where, "can't tell the faction of spawn point %s" % e.label())
                elif fac in statics:
                    statics[fac].append(e)
        for fac in FACTIONS:
            if counts[fac] and not statics[fac]:
                self.add(ERROR, "SPAWN-MISSING", self.m.world.display,
                         "%s has %d slots but no COA_StaticSpawnPoint (respawn will fail)" % (fac, counts[fac]))

    # ------------------------------------------------------------------ AI world / navmesh
    def check_ai_world(self) -> None:
        aiw = [e for e in self.m.entities if e.cls.startswith("SCR_AIWorld") or (e.prefab and "AIWorld" in e.prefab)]
        groups = [e for e in self.m.entities if e.cls == "SCR_AIGroup"]
        if not aiw:
            self.add(ERROR if groups else WARNING, "AI-WORLD", self.m.world.display,
                     "no SCR_AIWorld entity%s" % (" but the mission places %d AI groups" % len(groups) if groups else ""))
            return
        if len(aiw) > 1:
            self.add(WARNING, "AI-WORLD", aiw[1].where, "%d SCR_AIWorld entities" % len(aiw))
        e = aiw[0]
        files, configs = [], []
        for b in [e.block] + self.ctx.prefabs.chain(e.prefab):
            for blk in b.walk():
                nf = blk.get("NavmeshFile")
                if nf:
                    files.append(nf)
                if blk.key == "NavmeshFilesConfig" and blk.parent_ref:
                    configs.append(blk.parent_ref)
        world_parent = ""
        if self.m.world.doc:
            for b in self.m.world.doc.blocks:
                if b.key == "SubScene":
                    world_parent = b.get("Parent") or ""
        is_cherno = "chernarus" in world_parent.lower() or "/Chernarus/" in self.m.world.rel
        old = [f for f in files if "/ChernoT/" in f or "ChernoT/" in f]
        for f in old:
            self.add(ERROR, "AI-NAVMESH", e.where, "obsolete Chernarus navmesh %s (loads an empty navmesh; use Worlds/Navmesh/*.nmn or SCR_AIWorld_Cherno.et)" % f)
        if is_cherno:
            ok = any(f.split("}", 1)[-1].lower().startswith("worlds/navmesh/") for f in files) or \
                (e.prefab and "SCR_AIWorld_Cherno.et" in e.prefab)
            if not ok:
                self.add(ERROR, "AI-NAVMESH", e.where,
                         "Chernarus mission without the Worlds/Navmesh/*.nmn navmesh (instance SCR_AIWorld_Cherno.et or list the files)")
        elif not files and not configs:
            self.add(ERROR if groups else WARNING, "AI-NAVMESH", e.where, "AI world has no navmesh files or Navmesh_GM_*.conf configs")
        if groups and files and not configs:
            low = [f for f in files if "lowres" in f.lower()]
            if len(low) == len(files):
                self.add(WARNING, "AI-NAVMESH", e.where, "only a LowRes navmesh: soldiers/vehicles can't path")

    # ------------------------------------------------------------------ names and references
    def check_names(self) -> None:
        named = self.m.named()
        for name, lst in named.items():
            tops = [e for e in lst]
            if len(tops) > 1:
                self.add(ERROR, "NAME-DUP", tops[1].where, "entity name %r used %d times (first %s)" % (name, len(tops), tops[0].where))
        cls_attrs = self.ctx.attr_defaults
        refs = list(ENTITY_REF_ATTRS) + [("*", a) for a in self.ctx.cfg.section("validate").get("entity_ref_attributes", [])]
        by_attr: dict = {}
        for c, a in refs:
            by_attr.setdefault(a, set()).add(c)

        def check_value(name: str, where: str, attr: str, owner: str) -> None:
            if not name:
                return
            if name not in named:
                # an unset attribute's script default may be optional (e.g. CacheHunt home flag)
                sev = WARNING if attr.endswith("(default)") else ERROR
                self.add(sev, "NREF-MISSING", where, "%s.%s refers to entity %r, which does not exist" % (owner, attr, name))

        for e in self.m.entities:
            # entity-level attributes
            for blk, owner in self._ref_blocks(e):
                present = set()
                for ln in blk.lines:
                    if ln.key in by_attr and (owner in by_attr[ln.key] or "*" in by_attr[ln.key]):
                        present.add(ln.key)
                        for t in ln.values:
                            check_value(t.value, self.at(e.file, ln.line), ln.key, owner)
                for ch in blk.blocks:
                    if ch.key in by_attr and ch.key in ARRAY_REF_ATTRS:
                        present.add(ch.key)
                        for t in ch.array_tokens():
                            check_value(t.value, self.at(e.file, t.line), ch.key, owner)
                # unset attributes fall back to the script default
                for attr, owners in by_attr.items():
                    if attr in present or attr in ARRAY_REF_ATTRS:
                        continue
                    if owner in owners or ("*" in owners and attr in cls_attrs.get(owner, {})):
                        dv = cls_attrs.get(owner, {}).get(attr)
                        if dv:
                            check_value(dv, self.at(e.file, blk.line), attr + " (default)", owner)
        # waypoint lists: static waypoints should be waypoints, vehicles vehicles
        for e in self.m.entities:
            sw = e.block.child("m_aStaticWaypoints")
            if sw is not None:
                for t in sw.array_tokens():
                    tgt = named.get(t.value)
                    if tgt and "Waypoint" not in tgt[0].cls and "waypoint" not in (tgt[0].prefab or "").lower():
                        self.add(WARNING, "NREF-KIND", self.at(e.file, t.line), "%r in m_aStaticWaypoints is a %s, not a waypoint" % (t.value, tgt[0].cls))
        # group spawn callsigns
        groups = self.m.slot_groups()
        callsigns = {}
        for g in groups:
            callsigns.setdefault(g.callsign, set()).add(g.faction)
        spawned = set()
        for e in self.m.entities:
            if e.cls != "COA_GroupSpawnPoint":
                continue
            ln = e.block.get_line("m_sCallsignOfGroupToSpawn")
            cs = ln.value_str() if ln else self.ctx.attr_defaults.get("COA_GroupSpawnPoint", {}).get("m_sCallsignOfGroupToSpawn", "")
            where = self.at(e.file, ln.line) if ln else e.where
            if cs not in callsigns:
                self.add(ERROR, "NREF-CALLSIGN", where, "group spawn point for callsign %r: no ORBAT group has that callsign" % cs)
                continue
            fac = self.entity_faction(e)
            if fac and fac not in callsigns[cs]:
                self.add(ERROR, "NREF-CALLSIGN", where, "group spawn point is %s but callsign %r is %s" % (fac, cs, "/".join(sorted(callsigns[cs]))))
            spawned.add(cs)
        if any(e.cls == "COA_GroupSpawnPoint" for e in self.m.entities):
            for g in groups:
                if g.callsign and g.callsign not in spawned:
                    self.add(INFO, "NREF-CALLSIGN", self.at(g.file, g.block.line),
                             "%s group %r has no group spawn point (spawns at the faction spawn)" % (g.faction, g.callsign))

    def _ref_blocks(self, e: Entity):
        """(block, owner class) pairs to scan for entity-name attributes."""
        yield e.block, e.cls
        for c in self.m.components(e):
            if c.key:
                yield c, c.key
                for sub in c.walk():
                    if sub is not c and sub.key and sub.words and len(sub.words) >= 1:
                        # nested config objects (entries) - owner = their class (2nd word for 'attr Class')
                        cls = sub.words[1].value if len(sub.words) > 1 and sub.words[1].kind == enf.WORD else sub.key
                        yield sub, cls

    # ------------------------------------------------------------------ gearscripts
    def check_gearscripts(self) -> None:
        gm = self.m.gamemode()
        if gm is None:
            return
        counts = self.m.slot_counts()
        for fac, attr in FACTION_GEAR_ATTRS.items():
            blk = gm.block.child(attr)
            if blk is None:
                if counts[fac]:
                    self.add(INFO, "GEAR-DEFAULT", gm.where, "%s has slots but no %s in the world (prefab default used)" % (fac, attr))
                continue
            ln = blk.get_line("m_rGearScript")
            if ln is None or not ln.values or not ln.values[0].value:
                if counts[fac]:
                    self.add(ERROR, "GEAR-MISSING", self.at(gm.file, blk.line), "%s has slots but no m_rGearScript" % fac)
                continue
            ref = ln.values[0].value
            if not counts[fac]:
                continue
            r = self.ctx.index.resolve(ref)
            if not r.ok:
                continue  # already reported by check_refs
            fp = self.ctx.prefabs.file_for(ref)
            if not fp:
                continue
            try:
                doc = enf.parse_file(fp)
            except (enf.ParseError, OSError) as ex:
                self.add(ERROR, "GEAR-PARSE", fp, str(ex))
                continue
            bad = []
            for t in doc.iter_tokens():
                if t.kind == enf.STRING and enf.RESOURCE_RE.match(t.value):
                    rr = self.ctx.index.resolve(t.value)
                    if not rr.ok and rr.status != "case":
                        bad.append((t.line, t.value, rr.message))
            if bad:
                shown = "; ".join("line %d %s (%s)" % (l, v.split("}", 1)[-1], msg.split(":")[0] if "GUID" in msg else msg) for l, v, msg in bad[:5])
                self.add(WARNING, "GEAR-ITEMS", "%s" % fp.replace("\\", "/"),
                         "%s gearscript: %d item reference(s) don't resolve: %s%s" % (fac, len(bad), shown, " ..." if len(bad) > 5 else ""))

    # ------------------------------------------------------------------ IDs
    def check_ids(self) -> None:
        seen = {}
        for f in self.m.layers:
            if f.doc is None:
                continue
            for b in f.doc.walk():
                if b.key == "ShapePoint":
                    oid = b.object_id
                    if oid:
                        if oid in seen:
                            self.add(WARNING, "ID-DUP", self.at(f, b.line), "ShapePoint ID {%s} also at %s" % (oid, seen[oid]))
                        else:
                            seen[oid] = self.at(f, b.line)
        gm = self.m.gamemode()
        if gm is not None:
            ids = {}
            for b in gm.block.walk():
                if b.key in ("COA_SlottingGroup", "COA_MissionDescriptor") and b.object_id:
                    if b.object_id in ids:
                        self.add(WARNING, "ID-DUP", self.at(gm.file, b.line), "%s ID {%s} also used at line %d" % (b.key, b.object_id, ids[b.object_id]))
                    ids.setdefault(b.object_id, b.line)


    # ------------------------------------------------------------------ weather / borders
    def check_weather(self) -> None:
        gm = self.m.gamemode()
        if gm is None:
            return
        for c in self.m.components(gm):
            if c.key != "SCR_TimeAndWeatherHandlerComponent":
                continue
            arr = c.child("m_aStartingWeatherAndTime")
            states = arr.blocks if arr is not None else []
            desc = ["%s %s:%02d" % (s.get("m_sWeatherPresetName", "?"), s.get("m_iStartingHour", "?"),
                                    int(s.get("m_iStartingMinutes", "0") or 0)) for s in states]
            if len(states) > 1:
                self.add(WARNING, "WEATHER-RANDOM", self.at(gm.file, arr.line),
                         "%d predefined weather/time states (%s): COA_Lobby.et enables predefined states, so one is "
                         "picked at random each mission" % (len(states), ", ".join(desc)))
            hb = self.m.header_block()
            hh = hb.get("m_iStartingHours") if hb is not None else None
            if hh is not None and states and all(s.get("m_iStartingHour") != hh for s in states):
                self.add(WARNING, "WEATHER-HOUR", self.at(gm.file, arr.line),
                         "header m_iStartingHours %s matches none of the world's weather states (%s); the world wins"
                         % (hh, ", ".join(desc)))

    def check_borders(self) -> None:
        counts = self.m.slot_counts()
        def pf(e):
            return (e.prefab or "").split("}", 1)[-1]
        # COA_GameBorder (Lobby game border system) or the legacy PolyZone prefabs (PolylineShapeEntity)
        legacy = [e for e in self.m.entities if pf(e).startswith("PrefabsMissionMaking/PolyZone/")
                  and ("GameBoundry" in pf(e) or "/Safestart/" in pf(e))]
        if legacy:
            self.add(INFO, "BORDER-LEGACY", legacy[0].where,
                     "uses the legacy PolyZone border prefabs (%d); the Lobby now has COA_GameBorder/COA_SafestartBorder" % len(legacy))
        if not any(e.cls == "COA_GameBorder" or pf(e).endswith("/GameBoundry.et") for e in self.m.entities):
            self.add(WARNING, "BORDER-MISSING", self.m.world.display, "no COA_GameBorder: players can leave the AO")
        zones = {}
        for e in self.m.entities:
            if e.cls == "COA_SafestartBorder" or "/Safestart/" in pf(e):
                zones.setdefault(self.entity_faction(e), []).append(e)
        for fac in FACTIONS:
            if counts[fac] and not zones.get(fac):
                self.add(WARNING, "SAFESTART-MISSING", self.m.world.display, "%s has slots but no safestart zone" % fac)


def validate_mission(ctx: Context, mission: Mission) -> list:
    return Validator(ctx, mission).run()


def summarize(diags: list) -> str:
    e = sum(1 for d in diags if d.severity == ERROR)
    w = sum(1 for d in diags if d.severity == WARNING)
    i = sum(1 for d in diags if d.severity == INFO)
    return "%d error(s), %d warning(s), %d info" % (e, w, i)
