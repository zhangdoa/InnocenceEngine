---
name: run-engine-via-script-not-exe
description: "Never launch Main.exe / RenderTest.exe by path — run through Scripts/*.ps1 (StartEngineWin.ps1 -Preset ..., TestPT.ps1, etc.) which set the correct Bin/ working directory, -c preset path, and exit-code propagation"
condition: '(?:Main|RenderTest)\.exe["'']?\s+-'
scope: ["tool:bash"]
---

Stop: do not invoke `Main.exe` / `RenderTest.exe` by path. The working directory, the `-c Engine/...` preset path (which resolves relative to `<cwd>/../Data/` inside `IOService`), the capture/output locations, and exit-code propagation all depend on launching from `Bin/` with the right args. A naked call gets the data dir wrong (or silently falls back to engine defaults — e.g. `totalFrames=0`, which then never auto-terminates) and loses the exit code. The wrapper scripts encode all of this.

This rule fires only on an actual **launch** — the exe immediately followed by a launch flag (`Main.exe -c …`, `RenderTest.exe -test …`, optionally quoted: `"…/Main.exe" -c …`). It deliberately does NOT fire on bare references that merely name the binary: `stat` / `ls` / `find` / `grep` on the path, or reading a wrapper script whose source contains the string — none of those launch the engine. (Accepted gap: a truly argument-less `Main.exe` with no flags slips through; no caller runs the engine that way, and the wrappers always pass `-c` / `-test`.)

Run through `Scripts/` via `powershell.exe -ExecutionPolicy Bypass -File`:
- `StartEngineWin.ps1 -Preset Engine/Configuration/Presets/<preset>.json` — generic launcher (CWD = `Bin/`, `-Wait`, propagates exit code). Default preset `Default.json`.
- `TestPT.ps1` / `TestGIScene.ps1` / `TestPTThreeScene.ps1` — capture-harness wrappers for specific presets.
- `InteractiveTest.ps1 -Scenario <name>` — windowed keystroke scenarios.

Example: `powershell.exe -ExecutionPolicy Bypass -File Scripts/StartEngineWin.ps1 -Preset Engine/Configuration/Presets/Smoke.json`.
