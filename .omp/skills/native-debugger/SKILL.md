---
name: native-debugger
description: Use when debugging the Windows native build (Main.exe / RenderTest.exe) with a real debugger instead of code-reading or run-and-grep. Owns the lldb / debug-tool setup on this machine.
---

# Native debugger (Windows / MSVC build)

The engine builds with MSVC (RelWithDebInfo → PDB). lldb reads the PDB fine — symbols,
breakpoints, `frame variable` all resolve. Reach for the debugger when a runtime
question is "what is this value at this point" — do NOT keep re-reading code or
run-and-grepping the log.

## The one prerequisite that makes it work

lldb / lldb-dap on this box link against **`python311.dll`**, which is NOT on the
default PATH. Without it the `debug` tool's adapter exits `code 53` and bare lldb
crashes with `0xC06D007E` ("unable to find 'python311.dll'"). The DLL lives at
`C:\Python311`.

- **`debug` tool**: ensure `C:\Python311` is on the Windows PATH of the session
  before `debug action:launch` (adapter `lldb-dap`). Missing → silent `code 53`.
- **Manual lldb**: prepend it per-invocation (see below).

## Why not stdout / the .Log

- `Main.exe` is a **GUI-subsystem** app (`WinMain`) — it has no console; `> file`
  captures nothing.
- It writes a timestamped `[...].Log` in the **cwd** (`Bin/`, since `m_dataDir =
  cwd/../Data/`). That file is held under a write-lock while running, and
  `taskkill /F` skips the flush — so a killed run leaves a **0-line log**. Only a
  clean self-exit (e.g. `session.totalFrames > 0` auto-terminate) produces a
  readable log. The debugger sidesteps all of this.

## Manual lldb batch recipe

Quote-hell through `cmd /c` is real; drive lldb from a `.bat` + a command script.

`Bin/_dbg.txt` (commands):
```
breakpoint set --name ConfigurationService::LoadFromFile
run
frame variable          # reliable on optimized builds
thread backtrace
continue
kill
quit
```

`Bin/_dbgrun.bat`:
```
@echo off
set PATH=C:\Python311;%PATH%
"C:\Program Files\LLVM\bin\lldb.exe" --batch -s _dbg.txt "RelWithDebInfo\Main.exe" -- -c Engine/Configuration/Presets/Smoke.json
```

Run from `Bin/`: `cmd.exe /c _dbgrun.bat`. Args after `--` become `run-args`
(the engine's `pScmdline`). Use a `Smoke`-style preset (offscreen, small
`totalFrames`) so the target is cheap.

## Gotchas

- `frame variable` shows real locals/`this->member` values. `expression --` eval
  is **unreliable** on optimized RelWithDebInfo (garbage strings, template-call
  ambiguity) — prefer `frame variable`, or break deeper and read the member.
- Config paths are **data-relative** (`Engine/Configuration/Presets/X.json`), not
  `Data/...` — `JSONWrapper::Load` prepends `GetDataDirectory()`.
- Kill strays first: `cmd /c "taskkill /F /IM Main.exe"`.
- Clean up `_dbg.txt` / `_dbgrun.bat` when done; they are not committed.
