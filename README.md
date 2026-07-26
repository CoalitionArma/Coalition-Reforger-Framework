# CRF - Coalition Reforger Framework
This is the Arma Reforger Framework developed and used by COALITION to host our events on. It's a comprehensive, modular foundation for tactical multiplayer missions, covering player/slot management, role-based equipment, respawn logic, mission phases and more. It's designed to be extended by dependent addons, such as [COALITION Lobby](https://github.com/CoalitionArma/COALITION-Lobby), which supplies the lobby UI and slotting flow built on top of it.

# Features
- **Core managers**: `COA_Gamemode` and `COA_GamemodeManager` coordinate mission-wide state through the Briefing → Slotting → Game → AAR phases, with dedicated managers for slotting, respawns, safestart, gearscripts, replication, permissions, logging and garbage collection.
- **Role-based equipment**: `COA_GearscriptManager` applies data-driven gearscripts per role, configured under `Configs/Gearscripts`.
- **Mission library**: ready-made missions across a wide range of official and community terrains under `Missions/`.
- **Extensive vanilla overrides**: `Scripts/Game/Systems/VanillaOverrides` and `ModdedOverrides` adapt base-game systems (stamina, characters, etc.) for framework use.

# Structure
| Folder | Contents |
| --- | --- |
| `Scripts/Game/Systems` | Core framework systems: Core managers, UI, VAAR, Persistence, vanilla/modded overrides |
| `Scripts/Game/GameMode` | Game mode implementations |
| `Configs/Gearscripts` | Role and equipment configurations |
| `Configs/Systems` | Core system configurations |
| `Missions` | Per-terrain mission configs |
| `Prefabs` / `PrefabsEditable` / `PrefabsMissionMaking` | Entity prefabs |
| `UI` | Framework UI layouts |

For a deeper dive into the architecture, core systems and API, see [TECHNICAL_README.md](TECHNICAL_README.md).

# Taking Part in Events
COALITION hosts events and weekly sessions open to the public on this framework. If you would like to take part in our Arma Reforger events, feel free to join our [Discord](https://discord.gg/the-coalition)!

# Contributing and Reporting Issues/Feature Requests
This Framework is open to be used by and contributed to by other Reforger Groups, it is licensed under the Arma Public License.
If you would like to report Bugs or Feedback feel free to create an Issue on the Github page as well as letting us know in our [Discord](https://discord.gg/the-coalition) if that is easier for you.
If you would like to contribute to this repo, feel free to fork and PR as you see fit.

# Further Info
Most of the discussion relating to this Framework takes place on our [Discord](https://discord.gg/the-coalition), feel free to take part or just read up.

# Support Us
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/I2I1VTOXS)

<img src="http://coalitiongroup.net/coalition.png">
![Discord Banner 1](https://discordapp.com/api/guilds/237991125523103747/widget.png?style=banner1)
