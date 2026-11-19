"""Semantic comparison of two missions (used by the Sideswipe acceptance test).

Two missions are equal when they have the same entities (matched by name, or
for unnamed entities by class + prefab + position), with the same classes,
prefabs, positions (within ``tol`` metres), attributes, ORBAT and component
attributes, and the same header/meta values.  Ignored on purpose:

* text layout ($grp grouping, line breaks inside arrays, key order is *not*
  ignored inside a block's array values but is ignored between properties),
* the layer an entity lives in (reported separately as a note),
* object IDs of objects the mission itself creates (slotting groups, briefing
  descriptors, shape points, weather states, new components); differences in
  those are counted as "ID-only" notes.
"""
from __future__ import annotations

import re
from typing import Optional

from . import enf
from .mission import Mission

ID_TOKEN = re.compile(r'^\{?[0-9A-Fa-f]{16}\}?$')


def _is_array(b: enf.Block) -> bool:
    if b.blocks or not b.lines:
        return False
    for ln in b.lines:
        if len(ln.tokens) > 1 and ln.tokens[0].kind == enf.WORD and not all(t.kind == enf.WORD and re.match(r"^[A-Z0-9_]+$", t.value) for t in ln.tokens):
            return False
    return True


def _label(b: enf.Block, keep_ids: bool) -> str:
    parts = []
    for t in b.words:
        if t.kind == enf.STRING and ID_TOKEN.match(t.value) and not keep_ids:
            parts.append("<id>")
        else:
            parts.append(t.value)
    if b.parent is not None:
        parts += [":", b.parent.value]
    if b.is_grp:
        parts.insert(0, "$grp")
    return " ".join(parts)


def flatten(b: enf.Block, path: str = "", keep_ids: bool = False, skip_children: bool = True) -> dict:
    out: dict = {}
    if _is_array(b):
        out[path + "[]"] = tuple(t.value for t in b.array_tokens())
        return out
    for ln in b.lines:
        k = path + "/" + ln.key
        out.setdefault(k, ())
        out[k] = out[k] + tuple(t.value for t in ln.values) if k in out and out[k] else tuple(t.value for t in ln.values)
    counts: dict = {}
    for c in b.blocks:
        if skip_children and c.key is None and not c.is_grp:
            continue   # hierarchy children are compared as entities
        lab = _label(c, keep_ids)
        i = counts.get(lab, 0)
        counts[lab] = i + 1
        out.update(flatten(c, "%s/%s[%d]" % (path, lab, i), keep_ids, False))
    return out


def _num(s: str) -> Optional[float]:
    try:
        return float(s)
    except ValueError:
        return None


def values_equal(a: tuple, b: tuple, tol: float) -> bool:
    if len(a) != len(b):
        return False
    for x, y in zip(a, b):
        if x == y:
            continue
        nx, ny = _num(x), _num(y)
        if nx is None or ny is None or abs(nx - ny) > tol:
            return False
    return True


def diff_flat(fa: dict, fb: dict, tol: float, where: str) -> list:
    out = []
    for k in sorted(set(fa) | set(fb)):
        if k not in fb:
            out.append("%s: %s only in A = %s" % (where, k, " ".join(fa[k])[:120]))
        elif k not in fa:
            out.append("%s: %s only in B = %s" % (where, k, " ".join(fb[k])[:120]))
        elif not values_equal(fa[k], fb[k], tol):
            out.append("%s: %s differs: A=%s | B=%s" % (where, k, " ".join(fa[k])[:100], " ".join(fb[k])[:100]))
    return out


def _entity_key(e) -> tuple:
    return (e.cls, e.prefab or "", e.depth)


def compare_missions(a: Mission, b: Mission, tol: float = 0.01) -> dict:
    """Return ``{"diffs": [...], "notes": [...], "id_only": n, "matched": n}``."""
    diffs, notes = [], []
    id_only = 0

    # ---- entities
    ea = [e for e in a.entities if e.depth == 0]
    eb = [e for e in b.entities if e.depth == 0]
    na = {e.name: e for e in ea if e.name}
    nb = {e.name: e for e in eb if e.name}
    pairs = []
    for n in sorted(set(na) | set(nb)):
        if n not in nb:
            diffs.append("entity %r only in A (%s)" % (n, na[n].label()))
        elif n not in na:
            diffs.append("entity %r only in B (%s)" % (n, nb[n].label()))
        else:
            pairs.append((na[n], nb[n]))
    ua = [e for e in ea if not e.name]
    ub = [e for e in eb if not e.name]
    used = set()
    for e in ua:
        best = None
        for j, f in enumerate(ub):
            if j in used or _entity_key(f) != _entity_key(e):
                continue
            ca, cb = e.coords, f.coords
            if ca and cb and max(abs(x - y) for x, y in zip(ca, cb)) <= tol:
                best = j
                break
        if best is None:
            diffs.append("unnamed entity only in A: %s at %s" % (e.label(), e.coords))
        else:
            used.add(best)
            pairs.append((e, ub[best]))
    for j, f in enumerate(ub):
        if j not in used:
            diffs.append("unnamed entity only in B: %s at %s" % (f.label(), f.coords))

    def layer_name(e):
        return e.file.rel.rsplit("/", 1)[-1]

    for x, y in pairs:
        label = x.name or "%s@%s" % (x.cls, x.coords)
        if x.cls != y.cls:
            diffs.append("%s: class A=%s B=%s" % (label, x.cls, y.cls))
        if (x.prefab or "") != (y.prefab or ""):
            diffs.append("%s: prefab A=%s B=%s" % (label, x.prefab, y.prefab))
        if layer_name(x) != layer_name(y):
            notes.append("%s: layer A=%s B=%s" % (label, layer_name(x), layer_name(y)))
        fa, fb = flatten(x.block), flatten(y.block)
        d = diff_flat(fa, fb, tol, label)
        diffs += d
        if not d and diff_flat(flatten(x.block, keep_ids=True), flatten(y.block, keep_ids=True), tol, label):
            id_only += 1
        # hierarchy children
        ca = [c for c in a.entities if c.parent_entity is x]
        cb = [c for c in b.entities if c.parent_entity is y]
        if len(ca) != len(cb):
            diffs.append("%s: %d hierarchy children in A, %d in B" % (label, len(ca), len(cb)))
        else:
            for i, (p, q) in enumerate(zip(ca, cb)):
                diffs += diff_flat(flatten(p.block), flatten(q.block), tol, "%s/child[%d]" % (label, i))
                if (p.cls, p.prefab) != (q.cls, q.prefab):
                    diffs.append("%s/child[%d]: %s vs %s" % (label, i, p.label(), q.label()))

    # ---- header + metas + world
    for fa_, fb_, what in ((a.header, b.header, "header"), (a.header_meta, b.header_meta, "header meta"),
                           (a.world, b.world, "world"), (a.world_meta, b.world_meta, "world meta")):
        if fa_ is None or fb_ is None:
            if fa_ is not fb_:
                diffs.append("%s missing in %s" % (what, "A" if fa_ is None else "B"))
            continue
        if fa_.rel != fb_.rel:
            diffs.append("%s path A=%s B=%s" % (what, fa_.rel, fb_.rel))
        if fa_.doc and fb_.doc:
            for i, (p, q) in enumerate(zip(fa_.doc.blocks, fb_.doc.blocks)):
                if _label(p, True) != _label(q, True):
                    diffs.append("%s: block %s vs %s" % (what, _label(p, True), _label(q, True)))
                diffs += diff_flat(flatten(p, keep_ids=True), flatten(q, keep_ids=True), tol, what)
    return {"diffs": diffs, "notes": notes, "id_only": id_only, "matched": len(pairs)}
