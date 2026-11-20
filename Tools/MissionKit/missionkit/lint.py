"""Enforce script linter for mission/mode scripts.

Flags constructs Enforce Script doesn't support or that fail at runtime:

* ``ENF-TERNARY`` - ``a ? b : c`` (no ternary operator in Enforce).
* ``ENF-FORMAT``  - ``string.Format`` with more than 9 arguments after the
  format string (only ``%1``..``%9`` exist).

Ported from the session scratchpad ``scripts/enforce_lint.py``; strings and
``//`` / ``/* */`` comments are ignored.
"""
from __future__ import annotations

import os
import re
from dataclasses import dataclass

BS = "\\"
MAX_FORMAT_ARGS = 9


@dataclass
class LintIssue:
    file: str
    line: int
    code: str
    message: str
    severity: str = "error"

    def format(self) -> str:
        return "%-7s %s:%d: [%s] %s" % (self.severity, self.file, self.line, self.code, self.message)


def _blank_comments(src: str) -> str:
    """Replace comments with spaces (keeping newlines) so offsets/lines stay valid."""
    out = []
    i = 0
    n = len(src)
    q = False
    while i < n:
        c = src[i]
        if q:
            out.append(c)
            if c == BS and i + 1 < n:
                out.append(src[i + 1])
                i += 2
                continue
            if c == '"':
                q = False
            i += 1
            continue
        if c == '"':
            q = True
            out.append(c)
            i += 1
            continue
        if src.startswith("//", i):
            j = src.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", src[i:j]))
            i = j
            continue
        out.append(c)
        i += 1
    return "".join(out)


def _calls(src: str, name: str):
    for m in re.finditer(re.escape(name) + r"\s*\(", src):
        i = m.end()
        depth = 1
        j = i
        inq = False
        while j < len(src) and depth:
            c = src[j]
            if c == '"' and src[j - 1] != BS:
                inq = not inq
            elif not inq:
                if c == "(":
                    depth += 1
                elif c == ")":
                    depth -= 1
            j += 1
        yield m.start(), src[i:j - 1]


def _top_commas(call: str) -> int:
    d = 0
    q = False
    n = 0
    for k, c in enumerate(call):
        if c == '"' and (k == 0 or call[k - 1] != BS):
            q = not q
        elif not q:
            if c in "([{":
                d += 1
            elif c in ")]}":
                d -= 1
            elif c == "," and d == 0:
                n += 1
    return n


def _strip_strings(line: str) -> str:
    out = []
    q = False
    k = 0
    while k < len(line):
        c = line[k]
        if c == '"' and (k == 0 or line[k - 1] != BS):
            q = not q
            k += 1
            continue
        if not q:
            out.append(c)
        k += 1
    return "".join(out)


def lint_source(src: str, file: str = "<string>") -> list:
    issues = []
    code = _blank_comments(src)
    for pos, call in _calls(code, "string.Format"):
        n = _top_commas(call)
        if n > MAX_FORMAT_ARGS:
            issues.append(LintIssue(file, code[:pos].count("\n") + 1, "ENF-FORMAT",
                                    "string.Format with %d arguments (max %d)" % (n, MAX_FORMAT_ARGS)))
    for ln, line in enumerate(code.splitlines(), 1):
        s = _strip_strings(line)
        if "?" in s and re.search(r"\?[^:;]*:", s) and "::" not in s:
            issues.append(LintIssue(file, ln, "ENF-TERNARY", "possible ternary operator: %s" % line.strip()[:100]))
    return issues


def lint_paths(paths: list) -> list:
    issues = []
    for p in paths:
        files = []
        if os.path.isdir(p):
            for d, _, fs in os.walk(p):
                files += [os.path.join(d, f) for f in fs if f.endswith(".c")]
        else:
            files = [p]
        for f in sorted(files):
            with open(f, encoding="utf-8", errors="replace") as fh:
                issues += lint_source(fh.read(), f.replace("\\", "/"))
    return issues
