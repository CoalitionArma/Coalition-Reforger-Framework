# Automated tests

Two layers, deliberately separate because they catch different things and cost
different amounts to run.

## 1. Static checks — `validate_resources.py`

Pure Python, standard library only. No game install, no Workbench. Runs on a
hosted GitHub runner in seconds via `.github/workflows/validate.yml`, on every
pull request.

```bash
# from the repository root
python3 Tools/Validation/validate_resources.py --project ../COALITION-Lobby
```

**COALITION-Lobby must be scanned alongside CRF.** The two addons reference each
other's resources by GUID, so scanning CRF alone reports every `COA_*` resource
that lives in the Lobby as dangling — about 70 false positives. The workflow
checks it out from `CoalitionArma/COALITION-Lobby@release`; it is public, so no
token is involved.

| Check | Catches |
| --- | --- |
| `dangling-reference` | A `{GUID}` reference to a Coalition resource that no `.meta` declares. The engine resolves by GUID, so this becomes `Can't open config file ...` and a null `ResourceName` handed to script. |
| `unattached-component` | A `SCR_BaseGameModeComponent` subclass no prefab attaches. Compiles fine, singleton never constructed, `GetInstance()` null forever, every call site silently no-ops. |
| `duplicate-component` | The same singleton component declared twice in one prefab, where which instance wins comes down to prefab ordering. |
| `unattached-handler` | A widget handler class script resolves with `FindHandler()` that no layout attaches. `FindHandler` returning null is a silent failure — call sites just bail — so this produces no log line at all. Subclasses count: a base class is satisfied by any descendant being attached. |

Ownership for `dangling-reference` is decided by filename prefix — default
`CRF_` and `COA_`. Base-game and third-party resources are skipped because their
`.meta` files are not in the repository. Add a family with `--prefix`, and add
the project that owns it with `--project`, or its resources start looking
dangling too.

Suppress an intentional finding in `allowlist.json`, with a reason.

## 2. In-engine tests — `Scripts/Game/Tests/`

Built on the autotest framework that ships with Arma Reforger
(`SCR_AutotestSuiteBase` / `SCR_AutotestCaseBase`). These need a world, so they
need the game.

```
Workbench   put the cursor in a test case class and press F4
Headless    ArmaReforgerSteam.exe -autotest CRF_FrameworkTestSuite
```

Headless results go to `autotest.log`. `-autotest` also accepts a single case
class name, or the GUID of an `SCR_AutotestGroup` config.

All test code is inside `#ifdef WORKBENCH` and never reaches a live server.

| Case | Asserts |
| --- | --- |
| `CRF_Test_FactionGearscripts_Resolve` | Every assigned faction gearscript loads. Covers gearscripts assigned at runtime, which the static check cannot see. |
| `CRF_Test_GamemodeManagers_Attached` | The six framework managers resolve via `GetInstance()`. This is the regression that silently removed stat tracking and AAR replay saving. |

Writing more: a case is a class deriving from `SCR_AutotestCaseBase` tagged
`[Test(suite: CRF_FrameworkTestSuite)]`, with a `[Step(EStage.Main)] bool Execute()`.
Return `false` to be re-run next frame — that is how you wait for async state —
and `true` when done. Use `AssertTrue`, `SetResultSuccess`, `SetResultFailure`.
`[Step(EStage.Setup)]` and `[Step(EStage.TearDown)]` run either side. Note the
framework does **not** recreate the case object between runs, so initialise
fields in `Setup`, not at declaration.

The suite loads `CRF_FrameworkTestSuite.TEST_WORLD` before any case runs. Point
it at a different world to validate that one instead.

## CI wiring

`.github/workflows/validate.yml` runs layer 1 only. Layer 2 needs a runner with
Arma Reforger installed; if you stand up a self-hosted runner, add a job that
invokes `-autotest CRF_FrameworkTestSuite` and fails on a non-zero exit.
