"""Command line: ``python -m missionkit <command> ...`` (or ``missionkit.cmd``)."""
from __future__ import annotations

import argparse
import os
import sys

from . import enf
from .assets import AssetIndex
from .config import load_config


def _index(args, cfg):
    return AssetIndex.load(cfg, rebuild=getattr(args, "rebuild", False), quiet=False)


def _short(p: str, cfg) -> str:
    p = p.replace("\\", "/")
    for root in (cfg.crf, cfg.lobby):
        r = root.replace("\\", "/") + "/"
        if p.lower().startswith(r.lower()):
            return p[len(r):]
    return p


# --------------------------------------------------------------------------- assets
def cmd_assets(args) -> int:
    cfg = load_config(args.config)
    idx = _index(args, cfg)
    if args.action == "find":
        rows = idx.find(args.terms, args.ext, args.in_deps_only, args.limit)
        for g, p, a in rows:
            dep = "" if idx.in_closure(a) else "  (NOT a CRF dependency)"
            print("{%s}%s\t[%s]%s" % (g, p, idx.addon_name(a), dep))
        print("%d match(es)" % len(rows), file=sys.stderr)
        return 0
    if args.action == "check":
        rc = 0
        for r in args.terms:
            res = idx.resolve(r)
            if res.ok:
                print("ok      %s\t[%s]" % (r, ", ".join(sorted({idx.addon_name(a) for _, a in res.real_paths}))))
            else:
                rc = 1
                print("%-7s %s: %s" % ("warning" if res.status in ("case", "path_mismatch") else "error", r, res.message))
        return rc
    if args.action == "export":
        if len(args.terms) != 1:
            print("usage: missionkit assets export <out.json.gz> [--all-addons]", file=sys.stderr)
            return 2
        if idx.exported:
            print("error   this index was itself loaded from %s; export from a machine with the game and mods"
                  % idx.exported["path"], file=sys.stderr)
            return 1
        if idx.missing_deps:
            print("warning CRF dependencies with no local addon (not in the export): %s" % ", ".join(idx.missing_deps),
                  file=sys.stderr)
        st = idx.export(cfg, os.path.abspath(args.terms[0]), deps_only=not args.all_addons)
        print("exported %d addons, %d resources to %s (%.1f MB)" % (st["addons"], st["resources"], args.terms[0], st["bytes"] / 1e6))
        return 0
    if args.action == "status":
        st = idx.stats()
        print("addons indexed: %d, in CRF dependency closure: %d, resources: %d" % (st["addons"], st["in_closure"], st["resources"]))
        for g in st["missing_deps"]:
            print("missing dependency (no local addon): %s" % g)
        print("stale project .meta Names (file moved outside Workbench): %d" % len(idx.stale_meta))
        return 0
    return 2


# --------------------------------------------------------------------------- validate
def _load_target(target: str, cfg, idx, root=None, extra: list = None):
    """Load a mission. For a spec, ``extra`` receives diagnostics the generator itself raises
    (entities skipped for an uninstalled addon, find aliases that had to pick a candidate)."""
    from .mission import Mission
    t = target.lower()
    if t.endswith(".toml"):
        from .generator import Generator
        from .spec import Spec
        spec = Spec.load(target, idx)
        gen = Generator(spec)
        files = gen.generate()
        if extra is not None:
            extra += _spec_diags(target, gen.pending, spec.picks)
        out = root or cfg.crf
        _overlay_metas(idx, files)
        return Mission.from_files(out, files)
    if t.endswith(".conf"):
        return Mission.from_header(target, root, idx)
    return Mission.from_world(target, root)


def _spec_diags(where: str, pending: list, picks: dict) -> list:
    from .validate import ERROR, INFO, Diagnostic
    out = []
    for what, alias, msg in pending:
        out.append(Diagnostic(ERROR, "PENDING-ASSET", os.path.basename(where),
                              "%s NOT generated - %s" % (what, msg.split(": ", 1)[-1] if msg.startswith(what) else msg)))
    for alias, (ref, cands) in sorted(picks.items()):
        out.append(Diagnostic(INFO, "ALIAS-PICK", os.path.basename(where),
                              "find alias %r picked %s from %d candidates: %s" % (alias, ref, len(cands), ", ".join(cands))))
    return out


def _overlay_metas(idx, files: dict) -> None:
    import re
    for rel, text in files.items():
        if rel.endswith(".meta"):
            m = re.search(r'Name "\{([0-9A-F]{16})\}([^"]+)"', text)
            if m:
                idx.add_overlay(m.group(2), m.group(1))


def cmd_validate(args) -> int:
    from .validate import Context, ERROR, validate_mission, summarize
    cfg = load_config(args.config)
    idx = _index(args, cfg)
    ctx = Context(cfg, idx)
    worst = 0
    for target in args.targets:
        extra: list = []
        try:
            m = _load_target(target, cfg, idx, args.root, extra)
        except Exception as e:  # noqa: BLE001 - report and continue
            print("error   %s: cannot load: %s" % (target, e))
            worst = 1
            continue
        diags = extra + validate_mission(ctx, m)
        print("== %s%s" % (_short(m.world.display, cfg), (" + " + _short(m.header.display, cfg)) if m.header else ""))
        for d in diags:
            if d.severity == "info" and not args.verbose:
                continue
            print(d.format() if args.full_paths else d.format().replace(cfg.crf.replace("\\", "/") + "/", ""))
        print(summarize(diags))
        if any(d.severity == ERROR for d in diags):
            worst = 1
    return worst


# --------------------------------------------------------------------------- lint
def cmd_lint(args) -> int:
    from .lint import lint_paths
    issues = lint_paths(args.paths)
    for i in issues:
        print(i.format())
    print("%d issue(s)" % len(issues), file=sys.stderr)
    return 1 if issues else 0


# --------------------------------------------------------------------------- build
def cmd_build(args) -> int:
    from .build import apply_build, plan_build, unified
    from .mission import Mission
    from .spec import Spec, SpecError
    from .validate import Context, ERROR, validate_mission, summarize
    cfg = load_config(args.config)
    idx = _index(args, cfg)
    out = os.path.abspath(args.out or cfg.crf)
    try:
        spec = Spec.load(args.spec, idx)
        res = plan_build(spec, out, adopt=args.adopt, force=args.force)
    except SpecError as e:
        print("error   %s" % e)
        return 1
    for p in res.plans:
        print("%-18s %s" % (p.action, p.rel))
        if args.diff and p.action != "unchanged" and p.old:
            sys.stdout.write(unified(p))
        elif args.diff and p.action == "create" and args.verbose:
            print(p.text)
    for k in res.kept:
        print("%-18s %s (not owned by the spec)" % ("kept", k))
    for s in res.stale:
        print("%-18s %s (owned before, no longer generated - delete it if unwanted)" % ("stale", s))
    if spec.resolved and args.verbose:
        for a, r in sorted(spec.resolved.items()):
            print("alias   %s -> %s" % (a, r))
    _overlay_metas(idx, res.files)
    ctx = Context(cfg, idx)
    m = Mission.from_files(out, res.files)
    diags = _spec_diags(args.spec, res.pending, res.picks) + validate_mission(ctx, m)
    for d in diags:
        if d.severity != "info" or args.verbose:
            print(d.format().replace(out.replace("\\", "/") + "/", "").replace(cfg.crf.replace("\\", "/") + "/", ""))
    print("validate: " + summarize(diags))
    if res.conflicts:
        print("refusing to overwrite %d file(s) the spec doesn't own or that changed since the last build "
              "(--adopt to take over unowned files, --force to overwrite modified ones)" % len(res.conflicts))
        if not args.dry_run:
            return 1
    if args.dry_run:
        print("dry run: nothing written")
        return 0
    if args.strict and any(d.severity == ERROR for d in diags):
        print("--strict: validation errors, nothing written")
        return 1
    apply_build(res, out)
    print("wrote %d file(s) to %s; ownership manifest %s" % (sum(1 for p in res.plans if p.writes), out, res.manifest))
    return 0


# --------------------------------------------------------------------------- compare / parse
def cmd_compare(args) -> int:
    from .compare import compare_missions
    cfg = load_config(args.config)
    idx = _index(args, cfg)
    a = _load_target(args.a, cfg, idx, args.root_a)
    b = _load_target(args.b, cfg, idx, args.root_b)
    r = compare_missions(a, b, args.tol)
    for d in r["diffs"]:
        print("diff  " + d)
    for n in r["notes"]:
        print("note  " + n)
    print("%d entities matched, %d difference(s), %d note(s), %d ID-only difference(s)" %
          (r["matched"], len(r["diffs"]), len(r["notes"]), r["id_only"]))
    return 1 if r["diffs"] else 0


def cmd_parse(args) -> int:
    import glob
    bad = 0
    n = 0
    for pat in args.paths:
        files = glob.glob(pat, recursive=True) if any(c in pat for c in "*?[") else [pat]
        for f in files:
            n += 1
            try:
                enf.parse_file(f)
            except enf.ParseError as e:
                bad += 1
                print("error   %s" % e)
    print("%d file(s) parsed, %d failure(s)" % (n, bad))
    return 1 if bad else 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="missionkit", description="CRF mission-authoring kit")
    ap.add_argument("--config", help="missionkit.toml (default: repo root)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    a = sub.add_parser("assets", help="query the asset index")
    a.add_argument("action", choices=["find", "check", "status", "export"])
    a.add_argument("terms", nargs="*", help="keywords (find), {GUID}path refs (check) or the output file (export)")
    a.add_argument("--all-addons", action="store_true", help="export: every addon, not only CRF's dependency closure")
    a.add_argument("--ext", help="extension filter, e.g. et or et,conf")
    a.add_argument("--in-deps-only", action="store_true", help="only CRF's dependency closure")
    a.add_argument("--limit", type=int, default=50)
    a.add_argument("--rebuild", action="store_true", help="rebuild the cached index")
    a.set_defaults(func=cmd_assets)

    v = sub.add_parser("validate", help="validate missions (world .ent, header .conf or spec .toml)")
    v.add_argument("targets", nargs="+")
    v.add_argument("--root", help="project root (default: found from the file / CRF for specs)")
    v.add_argument("-v", "--verbose", action="store_true", help="show info diagnostics")
    v.add_argument("--full-paths", action="store_true")
    v.add_argument("--rebuild", action="store_true")
    v.set_defaults(func=cmd_validate)

    li = sub.add_parser("lint", help="lint Enforce scripts (.c files or folders)")
    li.add_argument("paths", nargs="+")
    li.set_defaults(func=cmd_lint)

    b = sub.add_parser("build", help="generate a mission from a spec")
    b.add_argument("spec")
    b.add_argument("--out", help="project root to write into (default: CRF)")
    b.add_argument("--dry-run", action="store_true")
    b.add_argument("--diff", action="store_true", help="show unified diffs against existing files")
    b.add_argument("--adopt", action="store_true", help="take ownership of existing files the spec generates")
    b.add_argument("--force", action="store_true", help="overwrite owned files modified since the last build")
    b.add_argument("--strict", action="store_true", help="don't write when validation reports errors")
    b.add_argument("-v", "--verbose", action="store_true")
    b.add_argument("--rebuild", action="store_true")
    b.set_defaults(func=cmd_build)

    c = sub.add_parser("compare", help="semantic diff of two missions (A, B: .ent/.conf/.toml)")
    c.add_argument("a")
    c.add_argument("b")
    c.add_argument("--root-a")
    c.add_argument("--root-b")
    c.add_argument("--tol", type=float, default=0.01)
    c.set_defaults(func=cmd_compare)

    p = sub.add_parser("parse", help="check that Enfusion text files parse (globs allowed)")
    p.add_argument("paths", nargs="+")
    p.set_defaults(func=cmd_parse)

    args = ap.parse_args(argv)
    try:
        return args.func(args)
    except KeyboardInterrupt:
        return 130
    except BrokenPipeError:
        return 0
