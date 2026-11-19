"""Semantic view of a mission: its files, entities, game mode, ORBAT and header.

A mission is identified by its world (``Worlds/<Map>/<stem>.ent``) inside a
project root.  Its files are the world ``.ent`` + ``.ent.meta``, every
``<stem>_Layers/*.layer`` and the header ``Missions/**/*.conf`` (+ ``.meta``)
whose ``World`` points at it.  Files can also be supplied in memory
(``overlay``) so a spec can be validated before anything is written.
"""
from __future__ import annotations

import glob
import os
import re
from dataclasses import dataclass, field
from typing import Optional

from . import enf

FACTION_SLOT_ATTRS = {
    "BLUFOR": "m_BLUFORSlots",
    "OPFOR": "m_OPFORSlots",
    "INDFOR": "m_INDFORSlots",
    "CIV": "m_CIVSlots",
}
FACTION_GEAR_ATTRS = {
    "BLUFOR": "m_BLUFORGearScriptSettings",
    "OPFOR": "m_OPFORGearScriptSettings",
    "INDFOR": "m_INDFORGearScriptSettings",
    "CIV": "m_CIVILIANGearScriptSettings",
}
FACTION_TICKET_ATTRS = {
    "BLUFOR": "m_iBLUFORTickets",
    "OPFOR": "m_iOPFORTickets",
    "INDFOR": "m_iINDFORTickets",
    "CIV": "m_iCIVTickets",
}
FACTIONS = ("BLUFOR", "OPFOR", "INDFOR", "CIV")


def norm(p: str) -> str:
    return p.replace("\\", "/")


@dataclass
class MFile:
    rel: str                 # path relative to the project root, forward slashes
    kind: str                # world | world_meta | header | header_meta | layer
    text: str
    abspath: str = ""
    doc: Optional[enf.Document] = None
    error: Optional[enf.ParseError] = None

    @property
    def display(self) -> str:
        return self.abspath or self.rel


@dataclass
class Entity:
    cls: str
    name: Optional[str]
    prefab: Optional[str]
    block: enf.Block
    file: MFile
    depth: int = 0           # 0 = top level of a layer, >0 = hierarchy child
    grp: bool = False        # came from a $grp block
    parent_entity: Optional["Entity"] = None

    @property
    def line(self) -> int:
        return self.block.line

    @property
    def where(self) -> str:
        return "%s:%d" % (self.file.display, self.line)

    @property
    def coords(self) -> Optional[tuple]:
        v = self.block.get("coords")
        if v is None:
            return None
        try:
            return tuple(float(x) for x in v.split()[:3])
        except ValueError:
            return None

    @property
    def angles(self) -> tuple:
        v = self.block.get("angles")
        if v:
            try:
                return tuple(float(x) for x in v.split()[:3])
            except ValueError:
                pass
        y = self.block.get("angleY")
        return (0.0, float(y) if y else 0.0, 0.0)

    def label(self) -> str:
        n = self.name or "<unnamed>"
        pf = (" : " + self.prefab.split("}")[-1]) if self.prefab else ""
        return "%s %s%s" % (self.cls, n, pf)


@dataclass
class SlotGroup:
    faction: str
    callsign: str
    flag: Optional[str]
    roles: list              # [(role, Token)]
    block: enf.Block
    file: MFile


class Mission:
    def __init__(self, root: str):
        self.root = norm(os.path.abspath(root)) if root else ""
        self.files: list = []
        self.world: Optional[MFile] = None
        self.world_meta: Optional[MFile] = None
        self.header: Optional[MFile] = None
        self.header_meta: Optional[MFile] = None
        self.layers: list = []
        self.entities: list = []
        self.notes: list = []        # loader problems: (severity, where, message)

    # ------------------------------------------------------------------ loading
    @classmethod
    def from_world(cls, world_path: str, root: Optional[str] = None, overlay: Optional[dict] = None,
                   header_path: Optional[str] = None) -> "Mission":
        world_path = norm(os.path.abspath(world_path))
        root = norm(os.path.abspath(root)) if root else find_project_root(world_path)
        m = cls(root)
        m._load_world(world_path, overlay or {}, header_path)
        return m

    @classmethod
    def from_header(cls, conf_path: str, root: Optional[str] = None, index=None) -> "Mission":
        conf_path = norm(os.path.abspath(conf_path))
        root = norm(os.path.abspath(root)) if root else find_project_root(conf_path)
        doc = enf.parse_file(conf_path)
        world_ref = None
        for b in doc.blocks:
            world_ref = b.get("World")
            if world_ref:
                break
        if not world_ref:
            raise ValueError("%s: no World in mission header" % conf_path)
        wp = world_ref.split("}", 1)[-1]
        cand = os.path.join(root, wp)
        if not os.path.exists(cand) and index is not None:
            r = index.resolve(world_ref)
            if r.real_paths:
                cand = os.path.join(root, r.real_paths[0][0])
        return cls.from_world(cand, root, header_path=conf_path)

    @classmethod
    def from_files(cls, root: str, files: dict) -> "Mission":
        """In-memory mission from ``{rel_path: text}`` (as produced by the generator)."""
        worlds = [r for r in files if r.endswith(".ent")]
        if not worlds:
            raise ValueError("no .ent in files")
        return cls.from_world(os.path.join(root, worlds[0]), root, overlay=files)

    def _read(self, rel: str, kind: str, overlay: dict) -> Optional[MFile]:
        rel = norm(rel)
        if rel in overlay:
            f = MFile(rel, kind, overlay[rel], "")
        else:
            ap = os.path.join(self.root, rel)
            if not os.path.exists(ap):
                return None
            f = MFile(rel, kind, enf.read_text(ap), norm(ap))
        try:
            f.doc = enf.parse(f.text, f.display)
        except enf.ParseError as e:
            f.error = e
        self.files.append(f)
        return f

    def rel(self, p: str) -> str:
        p = norm(os.path.abspath(p))
        if self.root and p.lower().startswith(self.root.lower() + "/"):
            return p[len(self.root) + 1:]
        return p

    def _load_world(self, world_path: str, overlay: dict, header_path: Optional[str]) -> None:
        wrel = self.rel(world_path)
        self.world = self._read(wrel, "world", overlay)
        if self.world is None:
            raise FileNotFoundError(world_path)
        self.world_meta = self._read(wrel + ".meta", "world_meta", overlay)
        if self.world_meta is None:
            self.notes.append(("error", self.world.display, "world has no .meta file (%s.meta)" % wrel))
        stem = wrel[:-4]
        layer_dir = stem + "_Layers"
        layer_rels = set()
        for r in overlay:
            if norm(r).startswith(layer_dir + "/") and r.endswith(".layer"):
                layer_rels.add(norm(r))
        ad = os.path.join(self.root, layer_dir)
        if os.path.isdir(ad):
            for f in os.listdir(ad):
                if f.endswith(".layer"):
                    layer_rels.add(layer_dir + "/" + f)
        for r in sorted(layer_rels, key=lambda x: (not x.endswith("/_INIT.layer"), x.lower())):
            lf = self._read(r, "layer", overlay)
            if lf:
                self.layers.append(lf)
        # header
        if header_path:
            hrel = self.rel(header_path)
        else:
            hrel = self._find_header(overlay)
        if hrel:
            self.header = self._read(hrel, "header", overlay)
            self.header_meta = self._read(hrel + ".meta", "header_meta", overlay)
        else:
            self.notes.append(("warning", self.world.display, "no mission header (Missions/**/*.conf) references this world"))
        self._collect_entities()

    def world_guid(self) -> Optional[str]:
        if self.world_meta and self.world_meta.doc:
            for b in self.world_meta.doc.blocks:
                n = b.get("Name")
                if n:
                    m = enf.RESOURCE_RE.match(n)
                    if m:
                        return m.group(1).upper()
        return None

    def _find_header(self, overlay: dict) -> Optional[str]:
        guid = self.world_guid()
        wrel = self.world.rel
        pat = re.compile(r'^\s*World\s+"\{([0-9A-Fa-f]{16})\}([^"]*)"', re.M)
        cands = []
        for r, t in overlay.items():
            if r.endswith(".conf") and norm(r).startswith("Missions/"):
                m = pat.search(t)
                if m and (m.group(1).upper() == guid or m.group(2).lower() == wrel.lower()):
                    cands.append(norm(r))
        if not cands:
            for p in glob.glob(os.path.join(self.root, "Missions", "**", "*.conf"), recursive=True):
                try:
                    with open(p, encoding="utf-8", errors="replace") as f:
                        t = f.read(4000)
                except OSError:
                    continue
                m = pat.search(t)
                if m and (m.group(1).upper() == guid or m.group(2).lower() == wrel.lower()):
                    cands.append(self.rel(p))
        if len(cands) > 1:
            self.notes.append(("warning", self.world.display, "several headers reference this world: %s" % ", ".join(cands)))
        return sorted(cands)[0] if cands else None

    # ------------------------------------------------------------------ entities
    def _collect_entities(self) -> None:
        for f in [self.world] + self.layers:
            if f is None or f.doc is None:
                continue
            for b in f.doc.blocks:
                if b.key == "SubScene" and f.kind == "world":
                    continue
                self._entity_block(b, f, 0, None)

    def _entity_block(self, b: enf.Block, f: MFile, depth: int, parent: Optional[Entity]) -> None:
        if b.is_grp:
            words = b.words
            cls = words[0].value if words else "?"
            prefab = b.parent.value if b.parent is not None else None
            for mem in b.blocks:
                name = mem.words[0].value if mem.words else None
                e = Entity(cls, name, prefab, mem, f, depth, True, parent)
                self.entities.append(e)
                self._children(e)
            return
        words = b.words
        if not words:
            # anonymous block at top level: children container of nothing (unusual)
            for c in b.blocks:
                self._entity_block(c, f, depth, parent)
            return
        cls = words[0].value
        name = None
        if len(words) > 1 and words[1].kind == enf.WORD:
            name = words[1].value
        e = Entity(cls, name, b.parent_ref, b, f, depth, False, parent)
        self.entities.append(e)
        self._children(e)

    def _children(self, e: Entity) -> None:
        for c in e.block.blocks:
            if c.key is None and not c.is_grp:       # '{' ... '}' = hierarchy children
                for cb in c.blocks:
                    self._entity_block(cb, e.file, e.depth + 1, e)

    # ------------------------------------------------------------------ views
    def named(self) -> dict:
        out: dict = {}
        for e in self.entities:
            if e.name:
                out.setdefault(e.name, []).append(e)
        return out

    def gamemode(self) -> Optional[Entity]:
        for e in self.entities:
            if e.cls == "COA_Gamemode" or (e.name == "COA_Lobby" and e.depth == 0):
                return e
        return None

    def slot_groups(self) -> list:
        gm = self.gamemode()
        out = []
        if gm is None:
            return out
        for fac, attr in FACTION_SLOT_ATTRS.items():
            arr = gm.block.child(attr)
            if arr is None:
                continue
            for g in arr.blocks:
                cs = g.get("m_sCallsign") or ""
                flag = g.get("m_FlagType")
                slots = g.child("m_aSlots")
                roles = [(t.value, t) for t in slots.array_tokens()] if slots is not None else []
                out.append(SlotGroup(fac, cs, flag, roles, g, gm.file))
        return out

    def slot_counts(self) -> dict:
        c = {f: 0 for f in FACTIONS}
        for g in self.slot_groups():
            c[g.faction] += len(g.roles)
        return c

    def header_block(self) -> Optional[enf.Block]:
        if self.header and self.header.doc and self.header.doc.blocks:
            return self.header.doc.blocks[0]
        return None

    def components(self, entity: Entity) -> list:
        comps = entity.block.child("components")
        return comps.blocks if comps is not None else []


def find_project_root(path: str) -> str:
    """Walk up from ``path`` to the directory holding a ``*.gproj``."""
    d = os.path.dirname(os.path.abspath(path))
    while True:
        if glob.glob(os.path.join(d, "*.gproj")):
            return norm(d)
        nd = os.path.dirname(d)
        if nd == d:
            return norm(os.path.dirname(os.path.abspath(path)))
        d = nd
