"""Kit configuration: ``missionkit.toml`` at the repo root (all keys optional)."""
from __future__ import annotations

import os
import tomllib
from dataclasses import dataclass, field

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DEFAULTS = {
    "paths": {
        # machine-specific: set them in missionkit.toml (the kit's copy in CRF is published, so no
        # personal paths here)
        "game_addons": "",
        "mod_addons": "",
        "crf_project": "",
        "lobby_project": "",
        "extra_projects": [],
        "cache_dir": ".cache",
        # an `assets export` file to use instead of scanning game_addons/mod_addons (CI has no game install);
        # the MISSIONKIT_ASSET_INDEX environment variable overrides it
        "asset_index": "",
    },
    "conventions": {
        # m_sGameMode values used by missions besides the COA_EGamemode enum (parsed from source)
        "extra_modes": ["ESCORT", "REPLAY", "VIP", "RADIOPHASE", "PROPHUNT"],
        # base mission header every COA mission inherits
        "base_header": "{3D094352621EA88C}Missions/COA_BaseMissionConfig.conf",
    },
    "validate": {
        # extra attribute names whose value names an entity (FindEntityByName targets)
        "entity_ref_attributes": [],
    },
}


def _merge(a: dict, b: dict) -> dict:
    out = dict(a)
    for k, v in b.items():
        if isinstance(v, dict) and isinstance(out.get(k), dict):
            out[k] = _merge(out[k], v)
        else:
            out[k] = v
    return out


@dataclass
class Config:
    data: dict = field(default_factory=lambda: dict(DEFAULTS))
    path: str = ""

    # ---- paths
    def p(self, key: str) -> str:
        return self.data["paths"][key]

    @property
    def game_addons(self) -> str:
        return self.p("game_addons")

    @property
    def mod_addons(self) -> str:
        return self.p("mod_addons")

    @property
    def crf(self) -> str:
        return self.p("crf_project")

    @property
    def lobby(self) -> str:
        return self.p("lobby_project")

    @property
    def projects(self) -> list:
        out = [self.crf, self.lobby]
        out += list(self.data["paths"].get("extra_projects", []))
        return [p for p in out if p]

    @property
    def asset_index(self) -> str:
        return os.environ.get("MISSIONKIT_ASSET_INDEX") or self.data["paths"].get("asset_index", "")

    @property
    def cache_dir(self) -> str:
        c = self.p("cache_dir")
        return c if os.path.isabs(c) else os.path.join(REPO_ROOT, c)

    def section(self, name: str) -> dict:
        return self.data.get(name, {})


def load_config(path: str | None = None) -> Config:
    path = path or os.environ.get("MISSIONKIT_CONFIG") or os.path.join(REPO_ROOT, "missionkit.toml")
    data = DEFAULTS
    if os.path.exists(path):
        with open(path, "rb") as f:
            data = _merge(DEFAULTS, tomllib.load(f))
    return Config(data, path if os.path.exists(path) else "")
