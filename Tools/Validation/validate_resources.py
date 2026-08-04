#!/usr/bin/env python3
"""
Static integrity checks for Coalition addon projects.

Runs without the game, Workbench or any third-party package - just a Python 3
interpreter and the checked-out repository. Intended to gate every pull request.

Checks
------
1. dangling-reference   A {GUID} reference to a Coalition-owned resource that no
                        .meta in any scanned project declares. This is what turns
                        into "Can't open config file ..." plus a null dereference
                        at runtime, because the engine resolves resources by GUID
                        and hands script a ResourceName that loads to null.

2. unattached-component A class deriving from SCR_BaseGameModeComponent that no
                        prefab attaches. The class compiles, its singleton is
                        never constructed, GetInstance() returns null forever and
                        every call site silently no-ops.

3. duplicate-component  The same component class declared twice in one prefab's
                        components block.

Ownership
---------
Check 1 only validates references to resources this organisation owns, decided by
filename prefix (default: CRF_, COA_). Base-game and third-party resources are
skipped, since their .meta files live outside the repository. Add prefixes with
--prefix when a new addon family appears.

Usage
-----
    python3 Tools/Validation/validate_resources.py
    python3 Tools/Validation/validate_resources.py --project ../COALITION-Lobby
    python3 Tools/Validation/validate_resources.py --check dangling-reference

Exits non-zero when any check reports a failure.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from collections import defaultdict

# Files the engine resolves resources from.
RESOURCE_SUFFIXES = (".conf", ".et", ".ent", ".layer", ".layout", ".ct")

GUID_REF = re.compile(r"\{([0-9A-F]{16})\}([^\"\s]+)")
META_NAME = re.compile(r'Name\s+"\{([0-9A-F]{16})\}([^"]*)"')
GAMEMODE_COMPONENT = re.compile(
    r"^\s*(?:modded\s+)?class\s+([A-Za-z0-9_]+)\s*:\s*SCR_BaseGameModeComponent\b", re.M
)
# "  ClassName "{0123456789ABCDEF}" {" - a component or entity declaration.
DECLARATION = re.compile(r'^\s*([A-Za-z_][A-Za-z0-9_]*)\s+"\{[0-9A-F]{16}\}"\s*(?::[^{]*)?\{\s*$')
BLOCK_OPEN = re.compile(r"^\s*components\s*\{\s*$")

ALLOWLIST_FILE = "allowlist.json"


def read(path: str) -> str:
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def walk(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in (".git", ".github", "node_modules")]
        for name in filenames:
            yield os.path.join(dirpath, name)


def rel(path: str, roots: list[str]) -> str:
    for root in roots:
        try:
            common = os.path.commonpath([os.path.abspath(path), os.path.abspath(root)])
        except ValueError:
            continue
        if common == os.path.abspath(root):
            parent = os.path.dirname(os.path.abspath(root))
            return os.path.relpath(os.path.abspath(path), parent).replace("\\", "/")
    return path.replace("\\", "/")


def collect_declared_guids(roots: list[str]) -> dict[str, str]:
    """GUID -> declaring .meta path, for every resource in every scanned project."""
    declared: dict[str, str] = {}
    for root in roots:
        for path in walk(root):
            if not path.endswith(".meta"):
                continue
            match = META_NAME.search(read(path)[:400])
            if match:
                declared[match.group(1)] = path
    return declared


def parse_components(text: str) -> list[tuple[str, int]]:
    """Component class names declared directly inside a `components { }` block.

    Brace-tracked rather than regexed over the whole file, because attribute
    arrays (m_Filters, m_aOverlays, ...) use the exact same
    `ClassName "{GUID}" {` syntax and would otherwise be counted as components.
    """
    found: list[tuple[str, int]] = []
    stack: list[bool] = []  # one entry per open brace: True when it is a components block
    pending_components = False

    for number, line in enumerate(text.splitlines(), start=1):
        if BLOCK_OPEN.match(line):
            stack.append(True)
            pending_components = False
            continue

        declaration = DECLARATION.match(line)
        if declaration:
            if stack and stack[-1]:
                found.append((declaration.group(1), number))
            stack.append(False)
            continue

        opens = line.count("{")
        closes = line.count("}")
        for _ in range(opens):
            stack.append(False)
        for _ in range(closes):
            if stack:
                stack.pop()

    return found


def check_dangling_references(roots: list[str], prefixes: tuple[str, ...]) -> list[str]:
    declared = collect_declared_guids(roots)
    failures: list[str] = []
    seen: set[tuple[str, str]] = set()

    for root in roots:
        for path in walk(root):
            if not path.endswith(RESOURCE_SUFFIXES):
                continue
            for guid, refpath in GUID_REF.findall(read(path)):
                basename = refpath.rsplit("/", 1)[-1]
                if not basename.startswith(prefixes):
                    continue  # base game or third-party addon, .meta is not in this repo
                if guid in declared:
                    continue
                key = (guid, rel(path, roots))
                if key in seen:
                    continue
                seen.add(key)
                failures.append(
                    f"{rel(path, roots)}: {{{guid}}}{refpath} - no .meta declares this GUID"
                )
    return failures


def collect_gamemode_components(roots: list[str]) -> dict[str, str]:
    """Class name -> declaring script, for every SCR_BaseGameModeComponent."""
    declarations: dict[str, str] = {}
    for root in roots:
        for path in walk(root):
            if path.endswith(".c"):
                for name in GAMEMODE_COMPONENT.findall(read(path)):
                    declarations.setdefault(name, rel(path, roots))
    return declarations


def check_unattached_components(roots: list[str], allowlist: set[str]) -> list[str]:
    declarations = collect_gamemode_components(roots)
    attached: set[str] = set()

    for root in roots:
        for path in walk(root):
            if path.endswith((".et", ".ent", ".layer")):
                for name, _ in parse_components(read(path)):
                    attached.add(name)

    return [
        f"{source}: class {name} is a SCR_BaseGameModeComponent but no prefab attaches it"
        for name, source in sorted(declarations.items())
        if name not in attached and name not in allowlist
    ]


def collect_singleton_components(roots: list[str]) -> set[str]:
    """Component classes that guard a static instance.

    Detected by the class holding a `static <ClassName> ...` member - which
    covers both `protected static CRF_SlotLottery m_sInstance;` and
    `static CRF_ServerStatsManager GetInstance()`.
    """
    singletons: set[str] = set()
    for root in roots:
        for path in walk(root):
            if not path.endswith(".c"):
                continue
            text = read(path)
            for name in re.findall(r"^\s*(?:modded\s+)?class\s+([A-Za-z0-9_]+)\s*:\s*\w*Component\b", text, re.M):
                if re.search(rf"\bstatic\s+{re.escape(name)}\s+[A-Za-z_]", text):
                    singletons.add(name)
    return singletons


def check_duplicate_components(roots: list[str]) -> list[str]:
    """Only singleton components.

    Enfusion happily allows many instances of the same component class on one
    entity - WeaponSlotComponent, RplComponent and friends legitimately repeat -
    so a blanket duplicate check is pure noise. A class guarding a static
    instance is different: a second copy means one of them loses the race and
    whichever wins is down to prefab ordering.
    """
    singletons = collect_singleton_components(roots)
    failures: list[str] = []

    for root in roots:
        for path in walk(root):
            if not path.endswith((".et", ".ent")):
                continue
            lines_by_name: dict[str, list[int]] = defaultdict(list)
            for name, number in parse_components(read(path)):
                if name in singletons:
                    lines_by_name[name].append(number)
            for name, numbers in sorted(lines_by_name.items()):
                if len(numbers) > 1:
                    where = ", ".join(f"line {n}" for n in numbers)
                    failures.append(
                        f"{rel(path, roots)}: singleton component {name} declared "
                        f"{len(numbers)}x ({where})"
                    )
    return failures


def check_unattached_handlers(roots: list[str], allowlist: set[str]) -> list[str]:
    """Widget handler classes script looks up but no layout attaches.

    `widget.FindHandler(X)` returning null is a silent failure - most call sites
    just bail out, so a handler dropped from a layout produces no log line at all.
    That is how COA_Hint was lost: the CRF->COA rename left hint.layout naming a
    CRF_Hint class that no longer existed, and re-saving the layout through
    Workbench silently discarded the unresolvable handler.

    FindHandler matches subclasses, so a base class is satisfied by any descendant
    being attached.
    """
    parents: dict[str, str] = {}
    wanted: dict[str, str] = {}
    attached: set[str] = set()

    handler_decl = re.compile(r"^\s*(?:modded\s+)?class\s+((?:CRF|COA)_[A-Za-z0-9_]+)\s*:\s*([A-Za-z0-9_]+)", re.M)
    find_handler = re.compile(r"FindHandler\(\s*((?:CRF|COA)_[A-Za-z0-9_]+)\s*\)")
    layout_attach = re.compile(r'^\s*((?:CRF|COA)_[A-Za-z0-9_]+)\s+"\{[0-9A-F]{16}\}"', re.M)

    for root in roots:
        for path in walk(root):
            if path.endswith(".c"):
                text = read(path)
                for name, parent in handler_decl.findall(text):
                    parents[name] = parent
                for name in find_handler.findall(text):
                    wanted.setdefault(name, rel(path, roots))
            elif path.endswith(".layout"):
                attached.update(layout_attach.findall(read(path)))

    def satisfied(name: str) -> bool:
        if name in attached:
            return True
        # Any attached class inheriting from `name` also satisfies FindHandler.
        for candidate in attached:
            seen = 0
            walker = candidate
            while walker in parents and seen < 20:
                walker = parents[walker]
                seen += 1
                if walker == name:
                    return True
        return False

    return [
        f"{source}: FindHandler({name}) but no layout attaches {name} or a subclass"
        for name, source in sorted(wanted.items())
        if not satisfied(name) and name not in allowlist
    ]


CHECKS = {
    "dangling-reference": "References to Coalition resources that no .meta declares",
    "unattached-component": "Gamemode components no prefab attaches",
    "duplicate-component": "Component classes declared twice in one prefab",
    "unattached-handler": "Widget handlers script looks up but no layout attaches",
}


def main() -> int:
    here = os.path.dirname(os.path.abspath(__file__))
    default_root = os.path.abspath(os.path.join(here, "..", ".."))

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--project", action="append", default=[], metavar="DIR",
                        help="Additional project root to scan (repeatable). Sibling addons "
                             "must be scanned together or their resources look dangling.")
    parser.add_argument("--prefix", action="append", default=[], metavar="PREFIX",
                        help="Additional owned-resource filename prefix (repeatable).")
    parser.add_argument("--check", action="append", default=[], choices=sorted(CHECKS),
                        help="Run only the named check (repeatable). Default: all.")
    args = parser.parse_args()

    roots = [default_root] + [os.path.abspath(p) for p in args.project]
    missing = [r for r in roots if not os.path.isdir(r)]
    if missing:
        print(f"error: project root does not exist: {', '.join(missing)}", file=sys.stderr)
        return 2

    prefixes = tuple(["CRF_", "COA_"] + args.prefix)

    allowlist: set[str] = set()
    allowlist_path = os.path.join(here, ALLOWLIST_FILE)
    if os.path.exists(allowlist_path):
        allowlist = set(json.loads(read(allowlist_path)).get("unattached-component", []))

    selected = args.check or sorted(CHECKS)
    results: dict[str, list[str]] = {}

    if "dangling-reference" in selected:
        results["dangling-reference"] = check_dangling_references(roots, prefixes)
    if "unattached-component" in selected:
        results["unattached-component"] = check_unattached_components(roots, allowlist)
    if "duplicate-component" in selected:
        results["duplicate-component"] = check_duplicate_components(roots)
    if "unattached-handler" in selected:
        results["unattached-handler"] = check_unattached_handlers(
            roots, set(json.loads(read(allowlist_path)).get("unattached-handler", []))
            if os.path.exists(allowlist_path) else set())

    print("Scanned: " + ", ".join(os.path.basename(r) for r in roots))
    print("Owned prefixes: " + ", ".join(prefixes))
    print()

    total = 0
    for name in selected:
        failures = results[name]
        total += len(failures)
        status = "FAIL" if failures else "ok"
        print(f"[{status:>4}] {name} - {CHECKS[name]}")
        for failure in failures:
            print(f"         {failure}")
        print()

    if total:
        print(f"{total} problem(s) found.")
        return 1

    print("All checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
