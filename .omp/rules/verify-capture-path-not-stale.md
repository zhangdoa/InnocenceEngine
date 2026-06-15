---
name: verify-capture-path-not-stale
description: "Confirm gpu_output.png is the path the producing run wrote (TestGIScene capture is Bin/gpu_output.png, not Bin/RelWithDebInfo/) before interpreting a render result"
condition: "RelWithDebInfo[\\\\/]gpu_output"
scope: ["tool:read(*gpu_output.png)", "tool:bash"]
---

Stop: you are about to inspect `Bin/RelWithDebInfo/gpu_output.png`. Before drawing ANY conclusion from a capture, confirm it is the file the run you care about actually wrote.

- `TestGIScene.ps1` / `Invoke-EngineMainRun` launch `Main.exe` with **CWD = `Bin/`** (`Split-Path $BinDir -Parent`), and the engine writes captures relative to CWD. So the real TestGIScene capture is **`Bin/gpu_output.png`**.
- `Bin/RelWithDebInfo/gpu_output.png` is only written by a `Main.exe` you launched *directly from that directory*; for a TestGIScene result it is **stale**.

Reading the wrong copy produced an entire false "flat frame / broken auto-exposure" diagnosis. Do not build a theory on a file you have not confirmed the run produced: check the launch CWD and the file's mtime (or diff the MAE the script computed against the file you're reading). View `Bin/gpu_output.png` for TestGIScene output.