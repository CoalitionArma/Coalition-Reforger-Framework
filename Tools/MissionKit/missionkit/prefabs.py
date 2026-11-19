"""Read prefab/config sources that live in a local project (CRF, Lobby).

Packed addons (``data.pak``) can't be read, so inheritance can only be
followed while the chain stays inside a local project; callers treat a
``None`` as "unknown".
"""
from __future__ import annotations

import os
from typing import Optional

from . import enf
from .assets import AssetIndex


class PrefabSource:
    def __init__(self, index: AssetIndex):
        self.index = index
        self._cache: dict = {}

    def file_for(self, ref: str) -> Optional[str]:
        r = self.index.resolve(ref)
        for p, a in (r.real_paths or []):
            addon = self.index.addons.get(a)
            if addon and addon.kind == "project":
                fp = os.path.join(addon.source, p)
                if os.path.exists(fp):
                    return fp
        return None

    def top_block(self, ref: str) -> Optional[enf.Block]:
        if ref in self._cache:
            return self._cache[ref]
        fp = self.file_for(ref)
        blk = None
        if fp:
            try:
                doc = enf.parse_file(fp)
                blk = doc.blocks[0] if doc.blocks else None
            except (enf.ParseError, OSError):
                blk = None
        self._cache[ref] = blk
        return blk

    def chain(self, ref: Optional[str], limit: int = 12) -> list:
        """``[block, parent_block, ...]`` for the readable part of a prefab's inheritance chain."""
        out = []
        seen = set()
        while ref and len(out) < limit and ref not in seen:
            seen.add(ref)
            b = self.top_block(ref)
            if b is None:
                break
            out.append(b)
            ref = b.parent_ref
        return out

    def root_class(self, ref: str) -> Optional[str]:
        ch = self.chain(ref)
        return ch[0].key if ch else None
