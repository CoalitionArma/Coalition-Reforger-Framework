"""Parser and writer for the Enfusion text serialisation format.

Used by ``.layer``, ``.ent``, ``.et``, ``.conf``, ``.meta`` and ``.gproj`` files::

    Class [name] [: "{GUID}parent"] {
     key value value ...
     key "string"\\
     "continued string"
     child [Class] ["{ID}"] {
      ...
     }
     { ...anonymous block (children / $grp member) ... }
    }
    $grp Class : "parent" {
     name { ... }
     { ... }
    }
    m_aItems +{ ... }        (merge syntax, also written ``+ {``)

The format has no schema at the syntax level, so the tree is generic: a
:class:`Block` has a *header* (the tokens before ``{``) and an ordered list of
items, each either a :class:`Line` (the tokens of one logical line) or a
nested :class:`Block`.  Interpretation (is ``X Y {`` "class X named Y" or
"property X of class Y"?) is left to the callers, which know the context.

Every token keeps its 1-based source line so diagnostics can point at
``file:line``.
"""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import Iterator, Optional, Union

__all__ = [
    "Token", "Line", "Block", "Document", "ParseError",
    "parse", "parse_file", "quote", "format_number",
    "GUID_RE", "RESOURCE_RE", "ID_RE",
]

# {GUID}path resource reference (path part may be empty for plain IDs)
RESOURCE_RE = re.compile(r"^\{([0-9A-Fa-f]{16})\}(.+)$")
# object ID in a header: "{16 hex}" or "16 hex"
ID_RE = re.compile(r"^\{?([0-9A-Fa-f]{16})\}?$")
GUID_RE = re.compile(r"\{([0-9A-Fa-f]{16})\}")


class ParseError(Exception):
    def __init__(self, msg: str, file: str = "<string>", line: int = 0):
        super().__init__(f"{file}:{line}: {msg}")
        self.file = file
        self.line = line
        self.msg = msg


# --------------------------------------------------------------------------- tokens
WORD, STRING, LBRACE, RBRACE, COLON, PLUS, NL = "WORD", "STRING", "{", "}", ":", "+", "NL"


@dataclass
class Token:
    kind: str
    value: str          # decoded value (strings unescaped, continuation parts joined by \n)
    line: int
    raw: Optional[list] = None   # for strings: list of raw (still escaped) parts

    @property
    def is_string(self) -> bool:
        return self.kind == STRING

    def text(self) -> str:
        """Source-like text of the token (strings re-quoted)."""
        if self.kind == STRING:
            return quote(self.value)
        return self.value

    def __repr__(self) -> str:  # pragma: no cover - debugging aid
        return f"Token({self.kind},{self.value!r},L{self.line})"


def _unescape(s: str) -> str:
    out = []
    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            n = s[i + 1]
            out.append({"n": "\n", "t": "\t", "r": "\r"}.get(n, n))
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def quote(s: str) -> str:
    """Quote a string value (single line; embedded newlines use ``"\\`` continuation)."""
    parts = s.split("\n")
    esc = ['"' + p.replace("\\", "\\\\").replace('"', '\\"') + '"' for p in parts]
    return "\\\n".join(esc)


def tokenize(text: str, file: str = "<string>") -> Iterator[Token]:
    i = 0
    n = len(text)
    line = 1
    if text.startswith("﻿"):
        i = 1
    while i < n:
        c = text[i]
        if c == "\n":
            yield Token(NL, "\n", line)
            line += 1
            i += 1
            continue
        if c in " \t\r":
            i += 1
            continue
        if c == "{":
            yield Token(LBRACE, "{", line)
            i += 1
            continue
        if c == "}":
            yield Token(RBRACE, "}", line)
            i += 1
            continue
        if c == '"':
            start_line = line
            raws = []
            while True:
                # read one quoted part starting at text[i] == '"'
                j = i + 1
                buf = []
                while j < n and text[j] != '"':
                    if text[j] == "\\" and j + 1 < n:
                        buf.append(text[j:j + 2])
                        if text[j + 1] == "\n":
                            line += 1
                        j += 2
                        continue
                    if text[j] == "\n":
                        line += 1
                    buf.append(text[j])
                    j += 1
                if j >= n:
                    raise ParseError("unterminated string", file, start_line)
                raws.append("".join(buf))
                i = j + 1
                # continuation: optional spaces, backslash, optional spaces, newline, spaces, quote
                m = re.compile(r"[ \t]*\\[ \t]*\r?\n[ \t]*\"").match(text, i)
                if m:
                    line += 1
                    i = m.end() - 1
                    continue
                break
            value = "\n".join(_unescape(r) for r in raws)
            yield Token(STRING, value, start_line, raws)
            continue
        # bare word
        j = i
        while j < n and text[j] not in ' \t\r\n{}"':
            j += 1
        word = text[i:j]
        i = j
        if word == ":":
            yield Token(COLON, ":", line)
        elif word == "+":
            yield Token(PLUS, "+", line)
        else:
            yield Token(WORD, word, line)


# --------------------------------------------------------------------------- tree
@dataclass
class Line:
    tokens: list
    line: int

    @property
    def key(self) -> Optional[str]:
        return self.tokens[0].value if self.tokens else None

    @property
    def values(self) -> list:
        return self.tokens[1:]

    def value_str(self) -> str:
        """Values joined with spaces (strings unquoted)."""
        return " ".join(t.value for t in self.tokens[1:])

    def __repr__(self) -> str:  # pragma: no cover
        return "Line(%s)" % " ".join(t.text() for t in self.tokens)


Item = Union[Line, "Block"]


@dataclass
class Block:
    header: list                    # tokens before ':'/'{' (may include '$grp')
    parent: Optional[Token] = None  # token after ':'
    merge: bool = False             # '+{' / '+ {'
    items: list = field(default_factory=list)
    line: int = 0
    end_line: int = 0
    file: str = "<string>"

    # ---- header helpers
    @property
    def is_grp(self) -> bool:
        return bool(self.header) and self.header[0].value == "$grp" and self.header[0].kind == WORD

    @property
    def words(self) -> list:
        """Header tokens without a leading ``$grp``."""
        return self.header[1:] if self.is_grp else self.header

    @property
    def key(self) -> Optional[str]:
        w = self.words
        return w[0].value if w else None

    def header_text(self) -> str:
        parts = [t.text() for t in self.header]
        if self.parent is not None:
            parts += [":", self.parent.text()]
        return " ".join(parts)

    @property
    def object_id(self) -> Optional[str]:
        """A quoted ``"{16hex}"``/``"16hex"`` header token (object/component ID)."""
        for t in self.words[1:]:
            if t.kind == STRING:
                m = ID_RE.match(t.value)
                if m:
                    return m.group(1).upper()
        return None

    @property
    def parent_ref(self) -> Optional[str]:
        """Prefab/config parent: after ':' or a quoted ``{GUID}path`` header token."""
        if self.parent is not None:
            return self.parent.value
        for t in self.words[1:]:
            if t.kind == STRING and RESOURCE_RE.match(t.value):
                return t.value
        return None

    # ---- body helpers
    @property
    def lines(self) -> list:
        return [i for i in self.items if isinstance(i, Line)]

    @property
    def blocks(self) -> list:
        return [i for i in self.items if isinstance(i, Block)]

    def get_line(self, key: str) -> Optional[Line]:
        for i in self.items:
            if isinstance(i, Line) and i.key == key:
                return i
        return None

    def get(self, key: str, default=None):
        """Value of property ``key`` as a string (multiple tokens joined by a space)."""
        ln = self.get_line(key)
        if ln is None:
            return default
        return ln.value_str()

    def get_tokens(self, key: str) -> Optional[list]:
        ln = self.get_line(key)
        return None if ln is None else ln.values

    def child(self, key: str) -> Optional["Block"]:
        for b in self.blocks:
            if b.key == key:
                return b
        return None

    def children(self, key: Optional[str] = None) -> list:
        return [b for b in self.blocks if key is None or b.key == key]

    def array_tokens(self) -> list:
        """All tokens of all lines (for array-like blocks such as ``m_aSlots { A B }``)."""
        out = []
        for ln in self.lines:
            out.extend(ln.tokens)
        return out

    def array_values(self) -> list:
        return [t.value for t in self.array_tokens()]

    def walk(self) -> Iterator["Block"]:
        yield self
        for b in self.blocks:
            yield from b.walk()

    def iter_tokens(self) -> Iterator[Token]:
        yield from self.header
        if self.parent is not None:
            yield self.parent
        for i in self.items:
            if isinstance(i, Line):
                yield from i.tokens
            else:
                yield from i.iter_tokens()

    def __repr__(self) -> str:  # pragma: no cover
        return f"Block({self.header_text()!r}, L{self.line}, {len(self.items)} items)"


@dataclass
class Document:
    items: list
    file: str = "<string>"

    @property
    def blocks(self) -> list:
        return [i for i in self.items if isinstance(i, Block)]

    @property
    def lines(self) -> list:
        return [i for i in self.items if isinstance(i, Line)]

    def walk(self) -> Iterator[Block]:
        for b in self.blocks:
            yield from b.walk()

    def iter_tokens(self) -> Iterator[Token]:
        for i in self.items:
            if isinstance(i, Line):
                yield from i.tokens
            else:
                yield from i.iter_tokens()


# --------------------------------------------------------------------------- parser
class _Parser:
    def __init__(self, text: str, file: str):
        self.toks = list(tokenize(text, file))
        self.pos = 0
        self.file = file

    def parse_body(self, closing: bool, open_line: int) -> tuple:
        items: list = []
        cur: list = []
        while self.pos < len(self.toks):
            t = self.toks[self.pos]
            self.pos += 1
            if t.kind == NL:
                if cur:
                    items.append(self._make_line(cur))
                    cur = []
                continue
            if t.kind == LBRACE:
                items.append(self._make_block(cur, t))
                cur = []
                continue
            if t.kind == RBRACE:
                if cur:
                    items.append(self._make_line(cur))
                if not closing:
                    raise ParseError("unexpected '}'", self.file, t.line)
                return items, t.line
            cur.append(t)
        if cur:
            items.append(self._make_line(cur))
        if closing:
            raise ParseError("unclosed '{' (opened here)", self.file, open_line)
        return items, (self.toks[-1].line if self.toks else 0)

    def _make_line(self, toks: list) -> Line:
        for t in toks:
            if t.kind in (COLON, PLUS):
                # stray ':' / '+' in a value line: keep as a word (rare, e.g. odd strings)
                t.kind = WORD
        return Line(list(toks), toks[0].line)

    def _make_block(self, header: list, brace: Token) -> Block:
        merge = False
        parent = None
        h = list(header)
        if h and h[-1].kind == PLUS:
            merge = True
            h.pop()
        elif h and h[-1].kind == WORD and h[-1].value.endswith("+") and h[-1].value != "+":
            # 'key+{' (unlikely) – treat as merge
            merge = True
            h[-1] = Token(WORD, h[-1].value[:-1], h[-1].line)
        if len(h) >= 2 and h[-2].kind == COLON:
            parent = h[-1]
            h = h[:-2]
        for t in h:
            if t.kind in (COLON, PLUS):
                raise ParseError("unexpected %r in block header" % t.value, self.file, t.line)
        line = h[0].line if h else brace.line
        items, end = self.parse_body(True, brace.line)
        return Block(h, parent, merge, items, line, end, self.file)


def parse(text: str, file: str = "<string>") -> Document:
    """Parse Enfusion text. Raises :class:`ParseError` with file:line on failure."""
    # '+{' glued: split into '+' '{'
    p = _Parser(text, file)
    # post-process: WORD tokens ending with '+' directly followed by '{' are handled
    # in _make_block; words like 'ActionRefs' '+' handled via PLUS.
    fixed = []
    for t in p.toks:
        if t.kind == WORD and t.value == "+":
            t.kind = PLUS
        fixed.append(t)
    p.toks = fixed
    items, _ = p.parse_body(False, 0)
    return Document(items, file)


def read_text(path: str) -> str:
    with open(path, "rb") as f:
        data = f.read()
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return data.decode("latin-1")


def parse_file(path: str) -> Document:
    return parse(read_text(path), str(path))


# --------------------------------------------------------------------------- writer helpers
def format_number(v: float, places: int = 3) -> str:
    """Format a number the way Workbench does: integers bare, otherwise up to ``places`` decimals."""
    if isinstance(v, bool):
        return "1" if v else "0"
    if isinstance(v, int):
        return str(v)
    r = round(float(v), places)
    if abs(r - round(r)) < 10 ** (-places - 1):
        return "%d" % round(r)
    s = ("%." + str(places) + "f") % r
    s = s.rstrip("0").rstrip(".")
    if s == "-0":
        s = "0"
    return s


def write_items(items: list, indent: int = 0) -> str:
    """Serialise parsed items back to text (round-trip helper; one space per level)."""
    out = []
    pad = " " * indent
    for it in items:
        if isinstance(it, Line):
            text = " ".join(_raw_string(t) if t.kind == STRING else t.value for t in it.tokens)
            out.append(pad + text.replace('\\\n"', '\\\n' + pad + '"'))
        else:
            hdr = it.header_text()
            if it.merge:
                hdr = (hdr + " +") if hdr else "+"
            out.append(pad + (hdr + " {" if hdr else "{"))
            body = write_items(it.items, indent + 1)
            if body:
                out.append(body)
            out.append(pad + "}")
    return "\n".join(out)


def _raw_string(t: Token) -> str:
    if t.raw is None:
        return quote(t.value)
    return "\\\n".join('"' + r + '"' for r in t.raw)
