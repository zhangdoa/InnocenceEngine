---
name: renderdoc-capture-is-in-engine
description: "RenderDoc capture is built into the engine via ConfigurationService::captureFrame (in-application RENDERDOC_GetAPI); set it in a preset and run — never drive capture through the system renderdoccmd/qrenderdoc install"
condition: "renderdoccmd\\s+capture|qrenderdoc.{0,20}capture|renderdoc.{0,16}capture"
scope: ["tool:bash"]
---

Stop: the engine captures RenderDoc frames itself — do NOT reach for the system `renderdoccmd capture` or inject through `qrenderdoc`. Both AI sessions (MiniMax, then me) instinctively went to `C:\Program Files\RenderDoc\`; that is the wrong path and burned multiple rounds each time.

To capture: set `session.captureFrame` (the frame index to capture) in a preset JSON, then run `Main.exe -c Engine/Configuration/Presets/<preset>.json`. When `captureFrame >= 0` the engine loads `renderdoc.dll` via `RENDERDOC_GetAPI`, brackets the frame with `BeginCapture`/`EndCapture`, and writes `<cwd>/../Build/captures/frame_*.rdc`. The run self-terminates at `session.totalFrames`. The integration lives in `Source/Engine/Services/DX12/DX12GraphicsHardwareService_Debug.cpp` (`TryLoadRenderDocAPI` / `BeginCapture` / `EndCapture`), wired in `Engine_RenderingCallbacks.cpp`. The gitsubmodule at `Source/External/GitSubmodules/renderdoc/` supplies only the `renderdoc_app.h` header for that API.

The system install is for VIEWING only: open the produced `.rdc` in `qrenderdoc.exe`. Capture via the engine, inspect via qrenderdoc — never the reverse.
