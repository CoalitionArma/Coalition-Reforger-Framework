"""Reader for Enfusion ``resourceDatabase.rdb`` files.

Record layout (reverse engineered, see README "Asset index")::

    u32 length (LE, includes the NUL) | path bytes | NUL | type u8 | 5 bytes | 8-byte GUID (LE) | ...

``type`` is 5 for directories, 6 for files, 4 for script (``.c``) records, which
carry no usable GUID and are skipped.  The GUID is printed as the 8 bytes
reversed in hex, which is the ``{GUID}`` of a resource name.

Rather than walking records sequentially (several record types have trailing
data of differing length) every NUL-terminated printable run is checked
against the length prefix in front of it, which is robust and fast.
"""
from __future__ import annotations

import re
import struct

_RUN = re.compile(rb"[^\x00-\x1f]{1,1024}\x00")
FILE_RECORD = 6


def read_rdb(path: str) -> list:
    """Return ``[(resource_path, GUID)]`` for every file record (scripts excluded)."""
    with open(path, "rb") as f:
        data = f.read()
    out = []
    n = len(data)
    for m in _RUN.finditer(data):
        s = m.start()
        e = m.end()          # index after NUL
        if s < 4 or e + 14 > n:
            continue
        if struct.unpack_from("<I", data, s - 4)[0] != e - s:
            continue
        if data[e] != FILE_RECORD:
            continue
        raw = data[s:e - 1]
        try:
            p = raw.decode("utf-8")
        except UnicodeDecodeError:
            p = raw.decode("latin-1")
        if p.endswith(".c"):
            continue
        guid = data[e + 6:e + 14][::-1].hex().upper()
        out.append((p, guid))
    return out


if __name__ == "__main__":  # pragma: no cover
    import sys
    for f in sys.argv[1:]:
        recs = read_rdb(f)
        print(f, len(recs))
        for p, g in recs[:5]:
            print("  {%s}%s" % (g, p))
