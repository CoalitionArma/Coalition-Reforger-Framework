#!/usr/bin/env python3
"""Export the game + mod asset index the Mission QA GitHub Action checks references against, and publish it.

Run on a machine with Arma Reforger and the mods (the ones CRF depends on) installed, from anywhere:

    python Tools/MissionKit/publish_asset_index.py --game-addons "<Arma Reforger>/addons" --mod-addons "<mods>/addons"

The paths can also come from Tools/MissionKit/missionkit.toml ([paths] game_addons / mod_addons; not
committed). Re-run it when CRF's dependencies change or the mods update.

It writes asset-index.json.gz (about 3 MB) and force-pushes it as the only commit of the `ci/asset-index`
branch of this repository, so refreshing it never grows the history. Nothing in your working tree or
current branch changes. --no-push only writes the file.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
import tempfile

KIT = os.path.dirname(os.path.abspath(__file__))
CRF = os.path.dirname(os.path.dirname(KIT))
BRANCH = "ci/asset-index"
FILE = "asset-index.json.gz"
sys.path.insert(0, KIT)

from missionkit.assets import AssetIndex  # noqa: E402
from missionkit.config import load_config  # noqa: E402


def git(*args, stdin: str | None = None) -> str:
    return subprocess.run(["git", *args], cwd=CRF, check=True, capture_output=True, text=True, input=stdin).stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--game-addons", help="Arma Reforger's addons folder")
    ap.add_argument("--mod-addons", help="the folder holding the mods (<Name>_<GUID> folders)")
    ap.add_argument("--lobby", help="COALITION-Lobby project (default: next to this CRF)")
    ap.add_argument("--out", help="also keep the file here")
    ap.add_argument("--remote", default="origin")
    ap.add_argument("--no-push", action="store_true", help="write the file only")
    args = ap.parse_args()

    cfg = load_config()
    paths = cfg.data["paths"]
    paths["crf_project"] = CRF
    paths["lobby_project"] = os.path.abspath(args.lobby) if args.lobby else (paths.get("lobby_project") or
                                                                              os.path.join(os.path.dirname(CRF), "COALITION-Lobby"))
    if args.game_addons:
        paths["game_addons"] = args.game_addons
    if args.mod_addons:
        paths["mod_addons"] = args.mod_addons
    paths["asset_index"] = ""
    os.environ.pop("MISSIONKIT_ASSET_INDEX", None)
    paths["cache_dir"] = os.path.join(tempfile.gettempdir(), "missionkit-publish-cache")
    for key in ("game_addons", "mod_addons"):
        if not paths.get(key) or not os.path.isdir(paths[key]):
            print("error: %s isn't set or doesn't exist (%r): pass --%s or set it in %s" %
                  (key, paths.get(key), key.replace("_", "-"), os.path.join(KIT, "missionkit.toml")))
            return 1
    if not os.path.isdir(paths["lobby_project"]):
        print("error: no COALITION-Lobby at %s (--lobby)" % paths["lobby_project"])
        return 1

    idx = AssetIndex.load(cfg, rebuild=True, quiet=False)
    if idx.missing_deps:
        print("warning: CRF dependencies not installed here, so not in the index (their references will fail in CI): %s"
              % ", ".join(idx.missing_deps))
    out = os.path.abspath(args.out) if args.out else os.path.join(tempfile.mkdtemp(prefix="asset-index-"), FILE)
    st = idx.export(cfg, out)
    print("exported %d addons, %d resources (%.1f MB) to %s" % (st["addons"], st["resources"], st["bytes"] / 1e6, out))
    if args.no_push:
        return 0

    # A parentless commit holding just the file, pushed over the branch: no history to grow
    blob = git("hash-object", "-w", out)
    tree = git("mktree", stdin="100644 blob %s\t%s\n" % (blob, FILE))
    commit = git("commit-tree", tree, "-m", "Asset index for the Mission QA check (%d addons, %d resources)"
                 % (st["addons"], st["resources"]))
    subprocess.run(["git", "push", "--force", args.remote, "%s:refs/heads/%s" % (commit, BRANCH)], cwd=CRF, check=True)
    print("published to %s/%s" % (args.remote, BRANCH))
    return 0


if __name__ == "__main__":
    sys.exit(main())
