"""Light-weight scanning of Enforce script sources (no full parser).

* ``scan_enums`` - ``enum X { A, B = 3, ... }`` and ``modded enum X`` additions.
* ``scan_attributes`` - ``[Attribute("default", ...)] type m_name;`` per class,
  so the validator knows the default of an attribute a layer leaves unset.
"""
from __future__ import annotations

import os
import re

_COMMENT_LINE = re.compile(r"//[^\n]*")
_COMMENT_BLOCK = re.compile(r"/\*.*?\*/", re.S)
_ENUM = re.compile(r"\b(modded\s+)?enum\s+(\w+)\s*(?::\s*\w+\s*)?\{([^}]*)\}", re.S)
_CLASS = re.compile(r"^\s*(?:modded\s+)?class\s+(\w+)", re.M)
_ATTR = re.compile(
    r"\[\s*Attribute\s*\(\s*(?:defvalue\s*:\s*)?(\"(?:[^\"\\]|\\.)*\"|[^,)\]]*)?[^\]]*\]\s*(?:,\s*\w+\([^)]*\)\s*)*\]?"
    r"\s*(?:(?:protected|private|static|ref|const|autoptr)\s+)*([\w<>, ]+?)\s+(m_\w+|\w+)\s*(?:=[^;]*)?;",
    re.S)


def strip_comments(src: str) -> str:
    src = _COMMENT_BLOCK.sub("", src)
    return _COMMENT_LINE.sub("", src)


def iter_sources(roots: list):
    for r in roots:
        sd = os.path.join(r, "Scripts")
        if not os.path.isdir(sd):
            continue
        for d, _, files in os.walk(sd):
            for f in files:
                if f.endswith(".c"):
                    yield os.path.join(d, f)


def _read(p: str) -> str:
    with open(p, encoding="utf-8", errors="replace") as f:
        return f.read()


def scan_enums(roots: list) -> dict:
    """Return ``{enum_name: [member, ...]}`` merged over all roots (modded enums append)."""
    enums: dict = {}
    for p in iter_sources(roots):
        src = _read(p)
        if "enum" not in src:
            continue
        for m in _ENUM.finditer(strip_comments(src)):
            name = m.group(2)
            members = []
            for part in m.group(3).split(","):
                part = part.strip()
                if not part:
                    continue
                members.append(part.split("=")[0].strip())
            lst = enums.setdefault(name, [])
            for x in members:
                if x and x not in lst:
                    lst.append(x)
    return enums


def scan_attributes(roots: list) -> dict:
    """Return ``{class_name: {attr_name: default_string}}``.

    Only the first positional / ``defvalue`` argument is captured; strings are
    unquoted.  Good enough for name-reference defaults such as
    ``[Attribute("interdiction_convoy", ...)] protected string m_sConvoyGroupName;``.
    """
    out: dict = {}
    for p in iter_sources(roots):
        src = _read(p)
        if "Attribute" not in src:
            continue
        src = strip_comments(src)
        classes = [(m.start(), m.group(1)) for m in _CLASS.finditer(src)]
        if not classes:
            continue
        for m in re.finditer(r"\[\s*Attribute\s*\(", src):
            # find the end of the attribute list: matching ']' at depth 0
            i = m.end()
            depth = 1
            q = False
            while i < len(src) and depth:
                c = src[i]
                if c == '"' and src[i - 1] != "\\":
                    q = not q
                elif not q:
                    if c == "(":
                        depth += 1
                    elif c == ")":
                        depth -= 1
                i += 1
            args = src[m.end():i - 1]
            j = src.find("]", i)
            if j < 0:
                continue
            decl = re.match(r"\s*(?:\[[^\]]*\]\s*)*(?:(?:protected|private|static|ref|const|autoptr)\s+)*"
                            r"([\w<>, ]+?)\s+(\w+)\s*(?:=[^;]*)?;", src[j + 1:j + 400])
            if not decl:
                continue
            name = decl.group(2)
            first = _first_arg(args)
            cls = None
            for pos, cn in classes:
                if pos < m.start():
                    cls = cn
                else:
                    break
            if cls:
                out.setdefault(cls, {})[name] = first
    return out


def _first_arg(args: str) -> str:
    args = args.strip()
    dm = re.search(r"defvalue\s*:\s*(\"(?:[^\"\\]|\\.)*\"|[^,]*)", args)
    if dm:
        v = dm.group(1).strip()
    else:
        if not args or re.match(r"^\w+\s*:", args):
            return ""
        if args.startswith('"'):
            mm = re.match(r"\"((?:[^\"\\]|\\.)*)\"", args)
            return mm.group(1) if mm else ""
        v = args.split(",")[0].strip()
    if v.startswith('"') and v.endswith('"'):
        v = v[1:-1]
    return v
