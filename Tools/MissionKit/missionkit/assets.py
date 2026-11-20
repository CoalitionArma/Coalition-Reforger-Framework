"""Asset index: ``GUID -> path(s) + addon`` and ``path -> GUID``.

Sources
  * every ``resourceDatabase.rdb`` under the game ``addons`` folder and the mod
    ``addons`` folder (all of them, so references to non-dependencies can be
    named), and
  * the ``.meta`` files of the local Workbench projects (CRF, COALITION-Lobby,
    ``paths.extra_projects``).  A local project replaces the packed addon with
    the same GUID: the project on disk is the truth while developing.

The *dependency closure* is CRF's ``addon.gproj`` ``Dependencies``, followed
recursively through each dependency's own ``.gproj``; CRF itself is included.

The index is cached in ``<cache_dir>/assets.pickle`` and rebuilt whenever an
rdb, gproj or project ``.meta`` changes (size/mtime signature).
"""
from __future__ import annotations

import glob
import os
import pickle
import re
from dataclasses import dataclass, field
from typing import Optional

from . import rdb
from .config import Config

CACHE_VERSION = 4
EXPORT_VERSION = 1
REF_RE = re.compile(r"^\{([0-9A-Fa-f]{16})\}(.+)$")
_META_NAME = re.compile(r'Name\s+"\{([0-9A-Fa-f]{16})\}([^"]+)"')
_GPROJ_GUID = re.compile(r'^\s*GUID\s+"([0-9A-Fa-f]{16})"', re.M)
_GPROJ_ID = re.compile(r'^\s*ID\s+"([^"]*)"', re.M)
_GPROJ_TITLE = re.compile(r'^\s*TITLE\s+"([^"]*)"', re.M)


@dataclass
class Addon:
    guid: str
    name: str
    kind: str            # game | mod | project
    folder: str
    deps: list = field(default_factory=list)
    source: str = ""     # rdb path or project dir


def read_gproj(path: str) -> tuple:
    """Return ``(guid, id, title, deps)`` of a ``.gproj``."""
    with open(path, encoding="utf-8", errors="replace") as f:
        t = f.read()
    g = _GPROJ_GUID.search(t)
    i = _GPROJ_ID.search(t)
    ti = _GPROJ_TITLE.search(t)
    deps = []
    m = re.search(r"Dependencies\s*\{([^}]*)\}", t)
    if m:
        deps = [d.upper() for d in re.findall(r'"([0-9A-Fa-f]{16})"', m.group(1))]
    return (g.group(1).upper() if g else "", i.group(1) if i else "", ti.group(1) if ti else "", deps)


def _find_gproj(folder: str) -> Optional[str]:
    cands = sorted(glob.glob(os.path.join(folder, "*.gproj")))
    if not cands:
        return None
    for c in cands:
        if os.path.basename(c).lower() == "addon.gproj":
            return c
    return cands[0]


def _iter_metas(root: str):
    stack = [root]
    while stack:
        d = stack.pop()
        try:
            with os.scandir(d) as it:
                for e in it:
                    if e.is_dir(follow_symlinks=False):
                        if e.name.startswith("."):
                            continue
                        stack.append(e.path)
                    elif e.name.endswith(".meta"):
                        yield e
        except OSError:
            continue


@dataclass
class Resolution:
    ref: str
    guid: str
    path: str
    status: str                 # ok | case | path_mismatch | unknown_guid | not_in_deps | script | malformed
    real_paths: list = field(default_factory=list)   # [(path, addon_guid)]
    message: str = ""

    @property
    def ok(self) -> bool:
        return self.status in ("ok", "script")


class AmbiguousAlias(Exception):
    def __init__(self, alias: str, candidates: list):
        self.alias = alias
        self.candidates = candidates
        super().__init__("alias %r is ambiguous (%d matches; use {GUID}path or a longer path): %s" % (
            alias, len(candidates), ", ".join("{%s}%s" % (g, p) for g, p, _ in candidates[:8])))


class UnknownAlias(Exception):
    pass


class AssetIndex:
    def __init__(self):
        self.addons: dict = {}        # guid -> Addon
        self.by_guid: dict = {}       # GUID -> [(path, addon_guid)]
        self.by_path: dict = {}       # lower path -> [(GUID, path, addon_guid)]
        self.closure: set = set()     # addon GUIDs in CRF's dependency closure (incl. CRF)
        self.root_guid: str = ""
        self.missing_deps: list = []  # dependency GUIDs that have no local addon
        self.signature = None
        self.overlay_guids: dict = {}  # in-memory additions (generated files)
        self.stale_meta: dict = {}     # project file path -> differing meta Name path
        self.exported: dict = {}       # set when loaded from an export file: path, created, deps_only

    # ------------------------------------------------------------------ build
    @staticmethod
    def discover(cfg: Config) -> tuple:
        """Find addons; returns (addons dict, signature list)."""
        addons: dict = {}
        sig = []
        # game addons
        for gp in sorted(glob.glob(os.path.join(cfg.game_addons, "*", "*.gproj"))):
            folder = os.path.dirname(gp)
            guid, gid, title, deps = read_gproj(gp)
            r = os.path.join(folder, "resourceDatabase.rdb")
            if guid and os.path.exists(r):
                addons[guid] = Addon(guid, gid or os.path.basename(folder), "game", folder, deps, r)
                sig.append(_stat(gp))
                sig.append(_stat(r))
        # mods: <Name>_<GUID>
        if os.path.isdir(cfg.mod_addons):
            for e in sorted(os.listdir(cfg.mod_addons)):
                m = re.match(r"^(.*)_([0-9A-Fa-f]{16})$", e)
                if not m:
                    continue
                folder = os.path.join(cfg.mod_addons, e)
                r = os.path.join(folder, "resourceDatabase.rdb")
                if not os.path.exists(r):
                    continue
                guid = m.group(2).upper()
                deps = []
                gp = _find_gproj(folder)
                if gp:
                    g2, _, _, deps = read_gproj(gp)
                    sig.append(_stat(gp))
                addons[guid] = Addon(guid, m.group(1), "mod", folder, deps, r)
                sig.append(_stat(r))
        # local projects override packed addons with the same GUID
        projects, psig = AssetIndex.discover_projects(cfg)
        addons.update(projects)
        sig += psig
        return addons, sig

    @staticmethod
    def discover_projects(cfg: Config) -> tuple:
        """The local Workbench projects (CRF, Lobby, ``extra_projects``); returns (addons, signature)."""
        addons: dict = {}
        sig = []
        for proj in cfg.projects:
            gp = _find_gproj(proj) if os.path.isdir(proj) else None
            if not gp:
                continue
            guid, gid, title, deps = read_gproj(gp)
            if not guid:
                continue
            addons[guid] = Addon(guid, (os.path.basename(proj.rstrip("\\/")) + " (project)"), "project", proj, deps, proj)
            sig.append(_stat(gp))
            n = 0
            tot = 0
            for e in _iter_metas(proj):
                st = e.stat()
                n += 1
                tot += st.st_mtime_ns ^ st.st_size
            sig.append((proj, n, tot))
        return addons, sig

    @classmethod
    def build(cls, cfg: Config, addons: dict, sig: list) -> "AssetIndex":
        idx = cls()
        idx.addons = addons
        idx.signature = sig
        for a in addons.values():
            if a.kind == "project":
                recs = idx._project_records(a)
            else:
                try:
                    recs = rdb.read_rdb(a.source)
                except OSError:
                    continue
            for p, g in recs:
                idx._add(p, g, a.guid)
        idx._compute_closure(cfg)
        return idx

    def _project_records(self, a: Addon) -> list:
        """``[(path, GUID)]`` of a local project, from its ``.meta`` files."""
        recs = []
        for e in _iter_metas(a.source):
            try:
                with open(e.path, encoding="utf-8", errors="replace") as f:
                    head = f.read(600)
            except OSError:
                continue
            m = _META_NAME.search(head)
            if m:
                # the file's real location wins: meta Names go stale when files are
                # moved outside Workbench (Workbench resolves by GUID + location)
                real = os.path.relpath(e.path[:-5], a.source).replace("\\", "/")
                recs.append((real, m.group(1).upper()))
                if m.group(2) != real:
                    self.stale_meta[real] = m.group(2)
        return recs

    # ------------------------------------------------------------------ export (CI)
    def export(self, cfg: Config, path: str, deps_only: bool = True) -> dict:
        """Write what a machine without the game and mods needs to validate CRF missions: every addon
        (CRF's dependency closure, or all with ``deps_only=False``) and its resources, except the CRF and
        Lobby projects themselves, which CI checks out and indexes from their ``.meta`` files. Other local
        projects in the closure (Radios, CSI ...) are exported from theirs. Gzipped JSON."""
        import datetime
        import gzip
        import json
        skip = set()
        for proj in (cfg.crf, cfg.lobby):
            gp = _find_gproj(proj) if proj and os.path.isdir(proj) else None
            if gp:
                skip.add(read_gproj(gp)[0])
        keep = {g: a for g, a in self.addons.items() if g not in skip and (not deps_only or g in self.closure)}
        resources: dict = {g: [] for g in keep}
        for guid, lst in self.by_guid.items():
            for p, a in lst:
                if a in resources:
                    resources[a].append([guid, p])
        for recs in resources.values():
            recs.sort(key=lambda r: r[1].lower())
        data = {
            "version": EXPORT_VERSION,
            "created": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "deps_only": deps_only,
            "addons": {g: {"name": a.name, "kind": a.kind, "deps": a.deps} for g, a in sorted(keep.items())},
            "resources": resources,
        }
        tmp = path + ".tmp"
        with gzip.open(tmp, "wt", encoding="utf-8", compresslevel=9) as f:
            json.dump(data, f, separators=(",", ":"))
        os.replace(tmp, path)
        return {"addons": len(keep), "resources": sum(len(r) for r in resources.values()), "bytes": os.path.getsize(path)}

    @classmethod
    def from_export(cls, cfg: Config, path: str) -> "AssetIndex":
        """The index from an ``export`` file plus the local projects (which win over exported addons
        with the same GUID). No game or mod install needed."""
        import gzip
        import json
        with gzip.open(path, "rt", encoding="utf-8") as f:
            data = json.load(f)
        if data.get("version") != EXPORT_VERSION:
            raise ValueError("%s is an asset index export version %r; this MissionKit reads version %d"
                             % (path, data.get("version"), EXPORT_VERSION))
        idx = cls()
        projects, sig = cls.discover_projects(cfg)
        for g, a in data["addons"].items():
            if g not in projects:
                idx.addons[g] = Addon(g, a["name"], a["kind"], "", list(a.get("deps", [])), path)
        idx.addons.update(projects)
        idx.signature = sig
        idx.exported = {"path": path, "created": data.get("created", ""), "deps_only": data.get("deps_only", True)}
        for g, recs in data["resources"].items():
            if g in projects:
                continue
            for guid, p in recs:
                idx._add(p, guid, g)
        for a in projects.values():
            for p, g in idx._project_records(a):
                idx._add(p, g, a.guid)
        idx._compute_closure(cfg)
        return idx

    def _add(self, path: str, guid: str, addon: str) -> None:
        self.by_guid.setdefault(guid, []).append((path, addon))
        self.by_path.setdefault(path.lower(), []).append((guid, path, addon))

    def _compute_closure(self, cfg: Config) -> None:
        gp = _find_gproj(cfg.crf)
        if not gp:
            return
        root, _, _, deps = read_gproj(gp)
        self.root_guid = root
        seen = {root}
        stack = list(deps)
        missing = []
        while stack:
            d = stack.pop()
            if d in seen:
                continue
            seen.add(d)
            a = self.addons.get(d)
            if a is None:
                missing.append(d)
                continue
            stack.extend(a.deps)
        self.closure = seen - set(missing)
        self.missing_deps = sorted(set(missing))

    # ------------------------------------------------------------------ cache
    @classmethod
    def load(cls, cfg: Config, rebuild: bool = False, quiet: bool = True) -> "AssetIndex":
        if cfg.asset_index:
            if not quiet:
                import sys
                print("[missionkit] asset index from %s" % cfg.asset_index, file=sys.stderr)
            return cls.from_export(cfg, cfg.asset_index)
        addons, sig = cls.discover(cfg)
        cache = os.path.join(cfg.cache_dir, "assets.pickle")
        if not rebuild and os.path.exists(cache):
            try:
                with open(cache, "rb") as f:
                    data = pickle.load(f)
                if data.get("version") == CACHE_VERSION and data.get("signature") == sig and data.get("crf") == cfg.crf:
                    idx = cls()
                    idx.__dict__.update(data["index"])
                    return idx
            except Exception:
                pass
        if not quiet:
            import sys
            print("[missionkit] building asset index ...", file=sys.stderr)
        idx = cls.build(cfg, addons, sig)
        os.makedirs(cfg.cache_dir, exist_ok=True)
        tmp = cache + ".tmp"
        with open(tmp, "wb") as f:
            pickle.dump({"version": CACHE_VERSION, "signature": sig, "crf": cfg.crf,
                         "index": {k: v for k, v in idx.__dict__.items() if k != "overlay_guids"}}, f,
                        protocol=pickle.HIGHEST_PROTOCOL)
        os.replace(tmp, cache)
        return idx

    # ------------------------------------------------------------------ overlay
    def add_overlay(self, path: str, guid: str, addon: Optional[str] = None) -> None:
        """Register a resource that only exists in memory / an output dir (generated .meta)."""
        addon = addon or self.root_guid
        guid = guid.upper()
        self.overlay_guids[guid] = path
        lst = self.by_guid.setdefault(guid, [])
        if (path, addon) not in lst:
            lst.append((path, addon))
        pl = self.by_path.setdefault(path.lower(), [])
        if (guid, path, addon) not in pl:
            pl.append((guid, path, addon))

    # ------------------------------------------------------------------ queries
    def addon_name(self, guid: str) -> str:
        a = self.addons.get(guid)
        return a.name if a else guid

    def in_closure(self, addon_guid: str) -> bool:
        return addon_guid in self.closure

    def resolve(self, ref: str) -> Resolution:
        m = REF_RE.match(ref.strip())
        if not m:
            return Resolution(ref, "", ref, "malformed", [], "not a {GUID}path reference")
        guid, path = m.group(1).upper(), m.group(2)
        if path.lower().endswith(".c"):
            return Resolution(ref, guid, path, "script")
        entries = self.by_guid.get(guid, [])
        if not entries:
            alt = self.by_path.get(path.lower(), [])
            msg = "unknown GUID {%s}" % guid
            if alt:
                msg += "; %s is {%s} (%s)" % (path, alt[0][0], self.addon_name(alt[0][2]))
            return Resolution(ref, guid, path, "unknown_guid", [], msg)
        exact = [(p, a) for p, a in entries if p == path]
        ci = [(p, a) for p, a in entries if p.lower() == path.lower()]
        if not ci:
            real = entries[0][0]
            return Resolution(ref, guid, path, "path_mismatch", entries,
                              "GUID {%s} is %s, not %s" % (guid, real, path))
        cands = exact or ci
        in_deps = [(p, a) for p, a in cands if self.in_closure(a)]
        if not in_deps:
            names = sorted({self.addon_name(a) for _, a in cands})
            return Resolution(ref, guid, path, "not_in_deps", cands,
                              "resource is in %s, which is not in CRF's dependency closure" % ", ".join(names))
        if not exact:
            return Resolution(ref, guid, path, "case", cands,
                              "path case differs: real path is %s" % cands[0][0])
        return Resolution(ref, guid, path, "ok", cands)

    def entries(self, deps_only: bool = False):
        for guid, lst in self.by_guid.items():
            seen = set()
            for p, a in lst:
                if deps_only and not self.in_closure(a):
                    continue
                if p in seen:
                    continue
                seen.add(p)
                yield guid, p, a

    def find(self, keywords: list, ext: Optional[str] = None, deps_only: bool = False, limit: int = 0) -> list:
        kws = [k.lower() for k in keywords]
        exts = None
        if ext:
            exts = tuple("." + e.lower().lstrip(".") for e in ext.split(","))
        out = []
        for guid, p, a in self.entries(deps_only):
            pl = p.lower()
            if exts and not pl.endswith(exts):
                continue
            if all(k in pl for k in kws):
                out.append((guid, p, a))
        out.sort(key=lambda r: (not self.in_closure(r[2]), r[1].lower()))
        return out[:limit] if limit else out

    def lookup(self, alias: str, deps_only: bool = True, exts: tuple = ()) -> tuple:
        """Resolve a short alias (``MH6M.et``, ``AH6M/MH6M.et`` or full path) to ``(guid, path, addon)``.

        Raises :class:`UnknownAlias` or :class:`AmbiguousAlias`.
        """
        a = alias.strip().replace("\\", "/")
        if REF_RE.match(a):
            r = self.resolve(a)
            if not r.ok and r.status != "case":
                raise UnknownAlias(r.message)
            return (r.guid, r.real_paths[0][0] if r.real_paths else r.path, r.real_paths[0][1] if r.real_paths else "")
        al = a.lower()

        def pick(cands):
            if deps_only:
                cands = [c for c in cands if self.in_closure(c[2])]
            if exts:
                cands = [c for c in cands if c[1].lower().endswith(exts)]
            uniq = {}
            for g, p, ad in cands:
                uniq.setdefault((g, p.lower()), (g, p, ad))
            return list(uniq.values())

        # 1. full path
        c = pick(self.by_path.get(al, []))
        if len(c) == 1:
            return c[0]
        if len(c) > 1:
            raise AmbiguousAlias(alias, c)
        # 2. suffix (filename or partial path)
        suffix = "/" + al
        c = pick([e for k, lst in self.by_path.items() if k.endswith(suffix) or k == al for e in lst])
        if len(c) == 1:
            return c[0]
        if len(c) > 1:
            raise AmbiguousAlias(alias, sorted(c, key=lambda x: x[1]))
        raise UnknownAlias("no resource matches %r%s" % (alias, " in CRF's dependencies" if deps_only else ""))

    def stats(self) -> dict:
        return {
            "addons": len(self.addons),
            "in_closure": len(self.closure),
            "resources": len(self.by_guid),
            "missing_deps": self.missing_deps,
        }


def _stat(p: str) -> tuple:
    st = os.stat(p)
    return (p, st.st_mtime_ns, st.st_size)


def ref(guid: str, path: str) -> str:
    return "{%s}%s" % (guid, path)
