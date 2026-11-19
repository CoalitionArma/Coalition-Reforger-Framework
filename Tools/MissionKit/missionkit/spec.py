"""TOML mission spec: loading, schema checks, prefab alias resolution.

See README.md "Spec reference" for the format.  Everything that names a
resource accepts either a verified ``{GUID}path`` or a short alias (file name
or path suffix, e.g. ``MH6M.et`` / ``AH6M/MH6M.et``) resolved through the
asset index within CRF's dependency closure; an ambiguous alias is an error.
Spec-local aliases (``[aliases]``) take precedence.
"""
from __future__ import annotations

import hashlib
import math
import os
import re
import tomllib
from typing import Any, Optional

from .assets import AmbiguousAlias, AssetIndex, UnknownAlias


class SpecError(Exception):
    pass


class PendingAsset(SpecError):
    """A ``find`` alias whose addon isn't installed yet. The generator skips the entities that
    use it and reports them (``PENDING-ASSET`` error), so the rest of the mission still builds."""

    def __init__(self, alias: str, message: str):
        super().__init__(message)
        self.alias = alias


# keys of a find alias: [aliases] X = { addon = "GUID", find = [...], ... } (README "Pending assets")
FIND_ALIAS_KEYS = {"addon", "find", "ext", "exclude", "path", "pending"}
FIND_DEFAULT_EXCLUDE = ["base", "wreck", "dst", "ruin", "preview", "proxy", "destroyed"]


# allowed keys per section (unknown keys are errors: catches typos)
COMMON = {"name", "pos", "yaw", "angles", "prefab", "class", "layer", "attrs", "snippet"}
SCHEMA = {
    "mission": {"map", "world", "author", "author_guid", "date", "mode", "name", "description", "start_hour",
                "id_prefix", "world_guid", "header_guid", "icon", "loading_screen", "preview_image",
                "marker_limit", "player_count", "terrain_name", "header_attrs"},
    "world": {"parent", "ai_world"},
    "game": {"respawn", "wave_respawn", "rally_points", "respawn_time", "respawn_cutoff_minutes", "respawn_mode",
             "time_limit_minutes", "lock_unused_slots", "safestart_time_limit", "use_cvon", "faction_ratio",
             "faction_keys", "time_scale", "attrs", "position"},
    "weather": {"random_start", "random_changes", "states"},
    "weather.states": {"preset", "hour", "minute", "id"},
    "faction": {"gearscript", "gearscript_id", "tickets", "bft", "leadership_radios", "gi_radios", "rto_radios",
                "friendly", "groups"},
    "group": {"callsign", "flag", "roles", "respawn_pool", "respawns", "repeat", "start", "id", "attrs"},
    "component": None,  # free-form
    "briefing": {"title", "text", "factions", "show_for_any", "id"},
    "spawns": COMMON | {"faction"},
    "group_spawns": COMMON | {"faction", "callsign"},
    "vehicle_spawners": COMMON | {"faction", "vehicle", "tickets_per_respawn"},
    "vehicles": COMMON,
    "waypoints": COMMON | {"type", "radius", "waypoints"},
    "routes": {"name", "type", "radius", "points", "cycle", "prefab", "class", "layer", "attrs", "cycle_name", "digits"},
    "ai_groups": COMMON | {"faction", "members", "waypoints", "vehicles"},
    "markers": COMMON | {"faction", "color", "description", "factions", "show_for_any"},
    "lines": COMMON | {"points", "points_file", "factions", "color", "width", "line_mode"},
    "border": COMMON | {"points"},
    "safestart": COMMON | {"faction", "points", "radius", "sides"},
    "props": COMMON,
    "anchors": {"name", "pos", "class", "layer", "attrs", "yaw", "angles"},
}
TOP = {"mission", "world", "game", "weather", "factions", "gamemode", "briefing", "spawns", "group_spawns",
       "vehicle_spawners", "vehicles", "waypoints", "routes", "ai_groups", "markers", "lines", "border",
       "safestart", "props", "anchors", "snippets", "aliases", "layers"}


def check_keys(section: str, d: dict, where: str) -> None:
    allowed = SCHEMA.get(section)
    if allowed is None:
        return
    bad = [k for k in d if k not in allowed]
    if bad:
        raise SpecError("%s: unknown key(s) %s (allowed: %s)" % (where, ", ".join(bad), ", ".join(sorted(allowed))))


class Spec:
    def __init__(self, data: dict, path: str, index: Optional[AssetIndex]):
        self.data = data
        self.path = path
        self.dir = os.path.dirname(os.path.abspath(path)) if path else os.getcwd()
        self.index = index
        self.aliases = dict(data.get("aliases", {}))
        self.snippets = dict(data.get("snippets", {}))
        self.resolved: dict = {}      # alias -> {GUID}path (for reporting)
        self.picks: dict = {}         # find alias -> (picked ref, all candidates) when it had to choose
        self._validate()

    @classmethod
    def load(cls, path: str, index: Optional[AssetIndex]) -> "Spec":
        with open(path, "rb") as f:
            try:
                data = tomllib.load(f)
            except tomllib.TOMLDecodeError as e:
                raise SpecError("%s: %s" % (path, e))
        return cls(data, path, index)

    # ------------------------------------------------------------------ schema
    def _validate(self) -> None:
        d = self.data
        bad = [k for k in d if k not in TOP]
        if bad:
            raise SpecError("unknown top-level section(s): %s" % ", ".join(bad))
        m = d.get("mission")
        if not m:
            raise SpecError("[mission] is required")
        check_keys("mission", m, "[mission]")
        for k in ("map", "world", "author", "date", "mode", "name"):
            if k not in m:
                raise SpecError("[mission] needs %r" % k)
        if not re.match(r"^\d{4}$", str(m["date"])):
            raise SpecError("[mission] date must be MMDD, e.g. \"1108\"")
        pfx = str(m.get("id_prefix", ""))
        if pfx and not re.match(r"^[0-9A-Fa-f]{1,12}$", pfx):
            raise SpecError("[mission] id_prefix must be 1-12 hex digits")
        for sec in ("world", "game", "weather"):
            if sec in d:
                check_keys(sec, d[sec], "[%s]" % sec)
        for i, s in enumerate(d.get("weather", {}).get("states", [])):
            check_keys("weather.states", s, "[[weather.states]] #%d" % (i + 1))
        for fk, f in d.get("factions", {}).items():
            if fk not in ("BLUFOR", "OPFOR", "INDFOR", "CIV"):
                raise SpecError("[factions.%s]: faction must be BLUFOR, OPFOR, INDFOR or CIV" % fk)
            check_keys("faction", f, "[factions.%s]" % fk)
            for i, g in enumerate(f.get("groups", [])):
                check_keys("group", g, "[[factions.%s.groups]] #%d" % (fk, i + 1))
                if "callsign" not in g or "roles" not in g:
                    raise SpecError("[[factions.%s.groups]] #%d needs callsign and roles" % (fk, i + 1))
        for sec in ("briefing", "spawns", "group_spawns", "vehicle_spawners", "vehicles", "waypoints", "routes",
                    "ai_groups", "markers", "lines", "safestart", "props", "anchors"):
            items = d.get(sec, [])
            if not isinstance(items, list):
                raise SpecError("%s must be an array of tables ([[%s]])" % (sec, sec))
            for i, it in enumerate(items):
                check_keys(sec, it, "[[%s]] #%d%s" % (sec, i + 1, (" (%s)" % it["name"]) if "name" in it else ""))
        if "border" in d:
            check_keys("border", d["border"], "[border]")
        for i, c in enumerate(d.get("gamemode", {}).get("components", [])):
            if "class" not in c:
                raise SpecError("[[gamemode.components]] #%d needs class" % (i + 1))
        for name, a in self.aliases.items():
            if isinstance(a, dict):
                bad = [k for k in a if k not in FIND_ALIAS_KEYS]
                if bad:
                    raise SpecError("[aliases] %s: unknown key(s) %s (allowed: %s)" % (name, ", ".join(bad), ", ".join(sorted(FIND_ALIAS_KEYS))))
                if not re.match(r"^[0-9A-Fa-f]{16}$", str(a.get("addon", ""))) or not (a.get("find") or a.get("path")):
                    raise SpecError("[aliases] %s: a find alias needs addon = \"<16-hex addon GUID>\" and find = [...] (or path)" % name)
            elif not isinstance(a, str):
                raise SpecError("[aliases] %s: must be a string or a find table" % name)

    # ------------------------------------------------------------------ accessors
    @property
    def mission(self) -> dict:
        return self.data["mission"]

    def get(self, sec: str, default=None):
        return self.data.get(sec, default if default is not None else {})

    def items(self, sec: str) -> list:
        return self.data.get(sec, [])

    # ------------------------------------------------------------------ resources
    def resource(self, value: str, what: str, exts: tuple = ()) -> str:
        """Resolve a prefab/resource alias to ``{GUID}path``."""
        if value in self.aliases and isinstance(self.aliases[value], dict):
            return self.find_alias(value, what)
        if value in self.aliases:
            value = self.aliases[value]
        if re.match(r"^\{[0-9A-Fa-f]{16}\}.+", value):
            if self.index is not None:
                r = self.index.resolve(value)
                if r.status in ("unknown_guid", "malformed", "not_in_deps"):
                    raise SpecError("%s: %s: %s" % (what, value, r.message))
                if r.status in ("path_mismatch", "case") and r.real_paths:
                    raise SpecError("%s: %s: %s" % (what, value, r.message))
            return value
        if self.index is None:
            raise SpecError("%s: alias %r needs the asset index" % (what, value))
        try:
            g, p, _ = self.index.lookup(value, deps_only=True, exts=exts)
        except AmbiguousAlias as e:
            raise SpecError("%s: %s" % (what, e))
        except UnknownAlias as e:
            raise SpecError("%s: %s" % (what, e))
        ref = "{%s}%s" % (g, p)
        self.resolved[value] = ref
        return ref

    def find_alias(self, name: str, what: str) -> str:
        """Resolve ``[aliases] name = { addon, find, ... }`` against one addon's resources.

        The addon missing from the index raises PendingAsset (the entity is skipped and reported).
        Otherwise: ``path`` (exact path or path suffix) if given, else every resource of the addon
        whose path contains all ``find`` keywords, has extension ``ext`` (default ``et``) and whose
        file name contains none of ``exclude`` (default: base/wreck/dst/ruin/preview/proxy/
        destroyed; editable ``E_*``/``PrefabsEditable`` copies are always skipped). Several left:
        prefer ``Prefabs/Vehicles/``, then a file named exactly after a keyword, then the shortest
        name; the pick is listed in ``self.picks`` so build/validate print it."""
        if name in self.resolved:
            return self.resolved[name]
        a = self.aliases[name]
        addon = str(a["addon"]).upper()
        if self.index is None:
            raise SpecError("%s: alias %r needs the asset index" % (what, name))
        if addon not in self.index.addons:
            raise PendingAsset(name, "%s: alias %r: %s" % (
                what, name, a.get("pending") or "addon %s is not installed (download it, then rebuild)" % addon))
        ext = "." + str(a.get("ext", "et")).lstrip(".").lower()
        rows = sorted({(g, p) for g, p, ad in self.index.entries() if ad == addon and p.lower().endswith(ext)},
                      key=lambda r: r[1].lower())
        if a.get("path"):
            want = str(a["path"]).lower()
            cands = [r for r in rows if r[1].lower() == want or r[1].lower().endswith("/" + want)]
        else:
            kws = [str(k).lower() for k in a.get("find", [])]
            excl = [str(x).lower() for x in a.get("exclude", FIND_DEFAULT_EXCLUDE)]
            cands = [r for r in rows if all(k in r[1].lower() for k in kws)
                     and not any(x in r[1].rsplit("/", 1)[-1].lower() for x in excl)
                     and not r[1].rsplit("/", 1)[-1].startswith("E_")          # editable-entity copies
                     and not r[1].lower().startswith("prefabseditable/")]
        if not cands:
            raise SpecError("%s: alias %r: no %s resource in addon %s matches %s (%d %s file(s) in the addon)" % (
                what, name, ext, addon, a.get("path") or a.get("find"), len(rows), ext))
        picked = cands
        if len(picked) > 1:
            veh = [r for r in picked if r[1].lower().startswith("prefabs/vehicles/")]
            picked = veh or picked
        if len(picked) > 1:
            stems = {str(k).lower() for k in a.get("find", [])}
            exact = [r for r in picked if r[1].rsplit("/", 1)[-1].rsplit(".", 1)[0].lower() in stems]
            picked = exact or sorted(picked, key=lambda r: (len(r[1].rsplit("/", 1)[-1]), r[1].lower()))
        g, p = picked[0]
        ref = "{%s}%s" % (g, p)
        self.resolved[name] = ref
        if len(cands) > 1:
            self.picks[name] = (ref, ["{%s}%s" % r for r in cands])
        return ref

    def snippet(self, name: str, what: str) -> str:
        if name not in self.snippets:
            raise SpecError("%s: unknown snippet %r" % (what, name))
        return self.snippets[name]


# --------------------------------------------------------------------------- helpers
def gen_id(prefix: str, *parts: Any) -> str:
    """Deterministic 16-hex ID: ``prefix`` + hash of the parts (stable across reorders)."""
    prefix = (prefix or "").upper()
    h = hashlib.sha1("/".join(str(p) for p in parts).encode("utf-8")).hexdigest().upper()
    return (prefix + h)[:16]


def vec3(v, what: str) -> tuple:
    if not isinstance(v, (list, tuple)) or len(v) not in (2, 3) or not all(isinstance(x, (int, float)) for x in v):
        raise SpecError("%s: position must be [x, y, z] (or [x, z] for shapes)" % what)
    if len(v) == 2:
        return (float(v[0]), 0.0, float(v[1]))
    return tuple(float(x) for x in v)


def angles_of(item: dict, what: str) -> tuple:
    if "angles" in item:
        a = item["angles"]
        if not isinstance(a, list) or len(a) != 3:
            raise SpecError("%s: angles must be [pitch, yaw, roll]" % what)
        return tuple(float(x) for x in a)
    if "yaw" in item:
        return (0.0, float(item["yaw"]), 0.0)
    return (0.0, 0.0, 0.0)


def yaw_towards(dx: float, dz: float) -> float:
    return math.degrees(math.atan2(dx, dz))


def expand_names(names: list) -> list:
    """Expand ``"route_{02..15}"`` (zero padding kept) in name lists."""
    out = []
    for n in names:
        m = re.search(r"\{(\d+)\.\.(\d+)\}", n)
        if not m:
            out.append(n)
            continue
        a, b = m.group(1), m.group(2)
        width = len(a) if a.startswith("0") else 0
        step = 1 if int(b) >= int(a) else -1
        for i in range(int(a), int(b) + step, step):
            out.append(n[:m.start()] + (str(i).zfill(width) if width else str(i)) + n[m.end():])
    return out


def expand_roles(roles: list, what: str) -> list:
    out = []
    for r in roles:
        m = re.match(r"^([A-Z_]+)\s*(?:\*\s*(\d+))?$", r.strip())
        if not m:
            raise SpecError("%s: bad role %r (use ROLE or ROLE*N)" % (what, r))
        out += [m.group(1)] * int(m.group(2) or 1)
    return out
