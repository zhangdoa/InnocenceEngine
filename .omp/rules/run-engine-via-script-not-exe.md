---
name: run-engine-via-script-not-exe
description: "Never launch Main.exe / RenderTest.exe by path — run through Scripts/*.ps1 (StartEngineWin.ps1 -Preset ..., TestPT.ps1, etc.) which set the correct Bin/ working directory, -c preset path, and exit-code propagation"
condition: "(?:Main|RenderTest)\\.exe"
scope: ["tool:bash"]
---

Stop: do not invoke `Main.exe` / `RenderTest.exe` by path. The working directory, the `-c Engine/...` preset path (which resolves relative to `<cwd>/../Data/` inside `IOService`), the capture/output locations, and exit-code propagation all depend on launching from `Bin/` with the right args. A naked call gets the data dir wrong (or silently falls back to engine defaults — e.g. `totalFrames=0`, which then never auto-terminates) and loses the exit code. The wrapper scripts encode all of this.

Run through `Scripts/` via `powershell.exe -ExecutionPolicy Bypass -File`:
- `StartEngineWin.ps1 -Preset Engine/Configuration/Presets/<preset>.json` — generic launcher (CWD = `Bin/`, `-Wait`, propagates exit code). Default preset `Default.json`.
- `TestPT.ps1` / `TestGIScene.ps1` / `TestPTThreeScene.ps1` — capture-harness wrappers for specific presets.
- `InteractiveTest.ps1 -Scenario <name>` — windowed keystroke scenarios.

Example: `powershell.exe -ExecutionPolicy Bypass -File Scripts/StartEngineWin.ps1 -Preset Engine/Configuration/Presets/Smoke.json`.
