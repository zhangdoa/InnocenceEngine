---
name: native-debugger
description: "Debug the Windows MSVC build with lldb. scripts/lldb-batch.sh discovers lldb + the python311.dll dir and runs a batch command file."
---

`scripts/lldb-batch.sh <commands-file> <exe> -- <run-args>` — discovers lldb and the python311.dll dir lldb needs (missing → exit 53 / 0xC06D007E).

| Fact | Value |
|---|---|
| Read locals | `frame variable` (reliable on RelWithDebInfo) |
| Avoid | `expression --` (garbage on optimized builds) |
| Config paths | data-relative: `Engine/Configuration/Presets/X.json` |
| Kill strays | `taskkill /F /IM Main.exe` |

`debug` tool path: ensure the python311.dll dir is on PATH before `debug action:launch` (adapter lldb-dap); use a Smoke preset.
