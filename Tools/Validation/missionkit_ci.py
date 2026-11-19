#!/usr/bin/env python3
"""MissionKit checks for the missions a pull request adds or changes (the Mission QA GitHub Action).

Runs the copy of CRF-MissionKit in Tools/MissionKit against the checked-out CRF and COALITION-Lobby. Every
reference into the game and the mods is checked against an asset index exported on a machine that has them
(`python -m missionkit assets export`, published to the `ci/asset-index` branch by
Tools/MissionKit/publish_asset_index.py). Without that file the reference checks are left out and the rest
still run.

    python Tools/Validation/missionkit_ci.py --lobby ../COALITION-Lobby --base-ref origin/release \
        [--index asset-index.json.gz] [--summary $GITHUB_STEP_SUMMARY]

Prints GitHub annotations (::error / ::warning) on the files, writes a Markdown summary, and exits 1 when
a mission has errors. Warnings don't fail the check.
"""
from __future__ import annotations

import argparse
import datetime
import gzip
import json
import os
import re
import subprocess
import sys
import tempfile

CRF = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(CRF, "Tools", "MissionKit"))

# Diagnostics that need the game and mod resources: dropped when there's no asset index
INDEX_CODES = {"REF-UNKNOWN", "REF-DEPS", "GEAR-ITEMS"}
WORLD_IN_LAYERS = re.compile(r"^(Worlds/.+)_Layers/[^/]+$")
HEADER_WORLD = re.compile(r'^\s*World\s+"\{[0-9A-Fa-f]{16}\}([^"]+)"', re.M)


def git(*args) -> str:
    return subprocess.run(["git", *args], cwd=CRF, check=True, capture_output=True, text=True).stdout


def changed_files(base_ref: str) -> list:
    """Files the branch added or changed since it left the base (deleted ones left out)."""
    base = git("merge-base", base_ref, "HEAD").strip()
    out = git("diff", "--name-only", "--diff-filter=d", base, "HEAD")
    return [l.strip().replace("\\", "/") for l in out.splitlines() if l.strip()]


def targets(files: list) -> list:
    """Mission headers (.conf) to validate, each with its world; a changed world no header points at is
    validated on its own."""
    worlds = set()
    headers = set()
    for f in files:
        if f.startswith("Missions/") and f.endswith(".conf"):
            headers.add(f)
        elif f.startswith("Worlds/") and f.endswith(".ent"):
            worlds.add(f)
        else:
            m = WORLD_IN_LAYERS.match(f)
            if m and f.endswith(".layer"):
                worlds.add(m.group(1) + ".ent")
    # which headers point at the changed worlds (a layer edit alone still checks the whole mission)
    world_header = {}
    for root, _, names in os.walk(os.path.join(CRF, "Missions")):
        for n in names:
            if not n.endswith(".conf"):
                continue
            p = os.path.join(root, n)
            try:
                with open(p, encoding="utf-8", errors="replace") as fh:
                    m = HEADER_WORLD.search(fh.read(4000))
            except OSError:
                continue
            if m:
                world_header.setdefault(m.group(1), os.path.relpath(p, CRF).replace("\\", "/"))
    out = sorted(h for h in headers if os.path.exists(os.path.join(CRF, h)))
    covered = set()
    for h in out:
        try:
            with open(os.path.join(CRF, h), encoding="utf-8", errors="replace") as fh:
                m = HEADER_WORLD.search(fh.read(4000))
            if m:
                covered.add(m.group(1))
        except OSError:
            pass
    for w in sorted(worlds):
        if w in covered or not os.path.exists(os.path.join(CRF, w)):
            continue
        h = world_header.get(w)
        out.append(h if h else w)
    return list(dict.fromkeys(out))


def empty_index(path: str) -> None:
    from missionkit.assets import EXPORT_VERSION
    with gzip.open(path, "wt", encoding="utf-8") as f:
        json.dump({"version": EXPORT_VERSION, "created": "", "addons": {}, "resources": {}}, f)


def rel(where: str, lobby: str) -> tuple:
    """(repo-relative file or "", line or 0, display text) of a diagnostic's location."""
    m = re.match(r"^(.*?):(\d+)$", where)
    path, line = (m.group(1), int(m.group(2))) if m else (where, 0)
    p = path.replace("\\", "/")
    for root, label in ((CRF, ""), (lobby, "COALITION-Lobby/")):
        r = os.path.abspath(root).replace("\\", "/").rstrip("/") + "/"
        if p.lower().startswith(r.lower()):
            short = p[len(r):]
            return (short if not label else "", line, label + short + (":%d" % line if line else ""))
    return ("", line, where)


def escape(s: str) -> str:
    return s.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--lobby", required=True, help="COALITION-Lobby checkout")
    ap.add_argument("--base-ref", default="origin/release")
    ap.add_argument("--index", default="", help="asset index export (gzipped JSON); omit to skip reference checks")
    ap.add_argument("--summary", default=os.environ.get("GITHUB_STEP_SUMMARY", ""))
    ap.add_argument("targets", nargs="*", help="missions to check instead of the branch's changes")
    args = ap.parse_args()
    lobby = os.path.abspath(args.lobby)

    try:
        todo = args.targets or targets(changed_files(args.base_ref))
    except subprocess.CalledProcessError as e:
        print("::error::couldn't diff against %s: %s" % (args.base_ref, (e.stderr or "").strip()))
        return 1

    lines = ["## MissionKit checks", ""]
    if not todo:
        print("No mission changes to check.")
        lines.append("No mission headers or worlds changed on this branch.")
        _write_summary(args.summary, lines)
        return 0

    index = args.index if args.index and os.path.exists(args.index) else ""
    tmp = tempfile.mkdtemp(prefix="missionkit-ci-")
    if not index:
        index = os.path.join(tmp, "empty-index.json.gz")
        empty_index(index)
    cfg_path = os.path.join(tmp, "missionkit.toml")
    with open(cfg_path, "w", encoding="utf-8") as f:
        f.write("[paths]\n")
        f.write("crf_project = %s\n" % json.dumps(CRF))
        f.write("lobby_project = %s\n" % json.dumps(lobby))
        f.write("asset_index = %s\n" % json.dumps(index))
        f.write("cache_dir = %s\n" % json.dumps(os.path.join(tmp, "cache")))

    from missionkit.assets import AssetIndex
    from missionkit.cli import _load_target
    from missionkit.config import load_config
    from missionkit.validate import ERROR, INFO, Context, validate_mission

    cfg = load_config(cfg_path)
    try:
        idx = AssetIndex.load(cfg)
    except Exception as e:  # noqa: BLE001 - a broken or truncated index: carry on without it
        print("::warning::The asset index %s couldn't be read (%s): references into the game and mods aren't checked"
              % (index, escape(str(e))))
        index = os.path.join(tmp, "empty-index.json.gz")
        empty_index(index)
        os.environ["MISSIONKIT_ASSET_INDEX"] = index   # Config.asset_index reads this before the file
        idx = AssetIndex.load(cfg)
    have_index = bool(idx.exported.get("created"))
    ctx = Context(cfg, idx)

    if have_index:
        created = idx.exported["created"]
        try:
            age = (datetime.datetime.now(datetime.timezone.utc) -
                   datetime.datetime.strptime(created, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=datetime.timezone.utc)).days
        except ValueError:
            age = -1
        lines.append("References checked against the game and mod asset index exported %s%s." %
                     (created[:10], " (%d days old: mods updated since may show as unknown)" % age if age > 30 else ""))
    else:
        print("::notice::No asset index (ci/asset-index branch): references into the game and mods aren't checked")
        lines.append("> **No asset index**, so references into the game and mods (prefabs, gearscript items) "
                     "weren't checked. Publish one with `Tools/MissionKit/publish_asset_index.py`.")
    lines.append("")

    errors = warnings = 0
    for target in todo:
        try:
            mission = _load_target(os.path.join(CRF, target), cfg, idx, CRF)
            diags = validate_mission(ctx, mission)
        except Exception as e:  # noqa: BLE001 - report and carry on with the next mission
            errors += 1
            print("::error file=%s,title=MissionKit::couldn't load the mission: %s" % (target, escape(str(e))))
            lines += ["### %s" % target, "", ":x: couldn't load the mission: `%s`" % e, ""]
            continue
        diags = [d for d in diags if d.severity != INFO and (have_index or d.code not in INDEX_CODES)]
        n_err = sum(1 for d in diags if d.severity == ERROR)
        errors += n_err
        warnings += len(diags) - n_err
        lines.append("### %s %s" % (":x:" if n_err else (":warning:" if diags else ":white_check_mark:"), target))
        lines.append("")
        if not diags:
            lines += ["No problems found.", ""]
            continue
        lines += ["| | Check | Where | Problem |", "|---|---|---|---|"]
        for d in sorted(diags, key=lambda d: (d.severity != ERROR, d.where)):
            f, line, shown = rel(d.where, lobby)
            kind = "error" if d.severity == ERROR else "warning"
            loc = (",line=%d" % line) if (f and line) else ""
            print("::%s %stitle=MissionKit %s::%s%s" % (kind, ("file=%s%s," % (f, loc)) if f else "", d.code,
                                                       "" if f else shown + ": ", escape(d.message)))
            lines.append("| %s | `%s` | `%s` | %s |" % (":x:" if kind == "error" else ":warning:", d.code, shown,
                                                       d.message.replace("|", "\\|")))
        lines.append("")

    lines.append("**%d error(s), %d warning(s)** in %d mission(s). Errors fail this check; warnings are for the "
                 "reviewer." % (errors, warnings, len(todo)))
    _write_summary(args.summary, lines)
    print("%d error(s), %d warning(s) in %d mission(s)" % (errors, warnings, len(todo)))
    return 1 if errors else 0


def _write_summary(path: str, lines: list) -> None:
    if not path:
        print("\n".join(lines))
        return
    with open(path, "a", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    sys.exit(main())
