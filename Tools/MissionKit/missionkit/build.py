"""``missionkit build``: write a spec's files with layer ownership tracking.

Enfusion text files have no comment syntax (none of the ~3,400 CRF/Lobby
text resources contain one), so ownership lives in a sidecar manifest:

    <out root>/.missionkit/<world stem>.ownership     (JSON, one per mission)

It records the SHA-256 of every file the kit wrote.  On the next build a file
is overwritten only if it is (a) new, (b) unchanged, or (c) still exactly what
the kit last wrote.  A file that exists but isn't in the manifest (a layer you
made in Workbench) or that changed since the kit wrote it (re-saved in
Workbench) is never overwritten - the build reports a conflict instead
(``--adopt`` takes over files not in the manifest, ``--force`` overwrites
modified ones).  Layers in the world's ``_Layers`` folder that the spec
doesn't produce are left alone.  The ``.ownership`` extension is not an
Enfusion resource type, so Workbench doesn't register or pack it.
"""
from __future__ import annotations

import difflib
import hashlib
import json
import os
from dataclasses import dataclass, field

from .generator import Generator
from .spec import Spec

MANIFEST_DIR = ".missionkit"
LF, CRLF = "\n", "\r\n"


def sha(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def manifest_path(out_root: str, world_rel: str) -> str:
    stem = os.path.splitext(os.path.basename(world_rel))[0]
    return os.path.join(out_root, MANIFEST_DIR, stem + ".ownership")


def load_manifest(path: str) -> dict:
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    return {"files": {}}


@dataclass
class FilePlan:
    rel: str
    action: str          # create | update | unchanged | conflict-unowned | conflict-modified
    text: str
    old: str = ""
    eol: str = LF      # keep an existing file's line ending (git may check CRF out as CRLF)

    @property
    def writes(self) -> bool:
        return self.action in ("create", "update", "adopt", "force")


@dataclass
class BuildResult:
    files: dict
    plans: list = field(default_factory=list)
    kept: list = field(default_factory=list)       # unowned layers left untouched
    stale: list = field(default_factory=list)      # owned before, no longer generated
    world_rel: str = ""
    header_rel: str = ""
    manifest: str = ""
    pending: list = field(default_factory=list)    # (what, alias, message): skipped, addon not installed
    picks: dict = field(default_factory=dict)      # find alias -> (picked ref, candidates)

    @property
    def conflicts(self) -> list:
        return [p for p in self.plans if p.action.startswith("conflict")]


def read(path: str):
    if not os.path.exists(path):
        return None
    with open(path, "rb") as f:
        data = f.read()
    return data.decode("utf-8", errors="replace")


def plan_build(spec: Spec, out_root: str, adopt: bool = False, force: bool = False,
               default_eol: str = LF) -> BuildResult:
    """Plan a build. Hashes and comparisons use LF-normalised text, so a git CRLF checkout
    doesn't count as a modification."""
    gen = Generator(spec)
    files = gen.generate()
    pth = gen.paths()
    mpath = manifest_path(out_root, pth["world"])
    man = load_manifest(mpath)
    owned = man.get("files", {})
    res = BuildResult(files, world_rel=pth["world"], header_rel=pth["header"], manifest=mpath,
                      pending=list(gen.pending), picks=dict(spec.picks))
    for rel, text in files.items():
        raw = read(os.path.join(out_root, rel))
        eol = CRLF if raw is not None and CRLF in raw else default_eol
        old = raw.replace(CRLF, LF) if raw is not None else None
        if old is None:
            act = "create"
        elif old == text:
            act = "unchanged"
        elif rel in owned and owned[rel] == sha(old):
            act = "update"
        elif rel in owned:
            act = "force" if force else "conflict-modified"
        else:
            act = "adopt" if adopt else "conflict-unowned"
        res.plans.append(FilePlan(rel, act, text, old or "", eol))
    ldir = os.path.join(out_root, pth["layers"])
    if os.path.isdir(ldir):
        for f in sorted(os.listdir(ldir)):
            rel = pth["layers"] + "/" + f
            if f.endswith(".layer") and rel not in files:
                (res.stale if rel in owned else res.kept).append(rel)
    for rel in owned:
        if rel not in files and not rel.endswith(".layer") and os.path.exists(os.path.join(out_root, rel)):
            res.stale.append(rel)
    return res


def apply_build(res: BuildResult, out_root: str) -> None:
    man = load_manifest(res.manifest)
    owned = man.setdefault("files", {})
    for p in res.plans:
        if p.action.startswith("conflict"):
            continue
        if p.writes:
            ap = os.path.join(out_root, p.rel)
            os.makedirs(os.path.dirname(ap), exist_ok=True)
            with open(ap, "w", encoding="utf-8", newline="") as f:
                f.write(p.text.replace(LF, p.eol))
        owned[p.rel] = sha(p.text)
    man["world"] = res.world_rel
    man["header"] = res.header_rel
    man["note"] = "Files listed here are owned by a missionkit spec; see CRF-MissionKit README (Layer ownership)."
    os.makedirs(os.path.dirname(res.manifest), exist_ok=True)
    with open(res.manifest, "w", encoding="utf-8", newline="\n") as f:
        json.dump(man, f, indent=1, sort_keys=True)


def unified(p: FilePlan) -> str:
    return "".join(difflib.unified_diff(p.old.splitlines(True), p.text.splitlines(True),
                                        "a/" + p.rel, "b/" + p.rel, n=2))
