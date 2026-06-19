---
name: no-claim-without-artifact-verify
description: "Never claim a subprocess result without reading the log artifact; exit code 0 with 0-byte log is a wedge, not success"
condition: "terminated cleanly|engine ran \\d+ frames|smoke green|smoke pass|reached steady state|engine rendered correctly|ran \\d+ frames"
scope: "text"
---

Exit code 0 is not evidence a process succeeded. The engine has known wedge paths (TASK-241 CreateFenceEvents AV; TASK-247 TextureResourceService init loop) that exit 0 with 0-byte logs because the logger never reaches `~LogService()` to flush.

Before claiming ANY subprocess result, you must:
1. Read the artifact (log file, output capture). Get its size.
2. If size == 0, the process wedged or the logger never flushed. Debug, don't claim success.
3. If size > 0, grep for the expected success markers (scene loaded, frame N reached, no D3D12 ERROR / CORRUPTION / Validation Error). Each marker must be present.
4. Only then say 'smoke green' / 'reached steady state' / 'engine rendered'.

'Engine terminated cleanly' requires ALL of: nonzero log size, expected scene loaded marker, expected frame count reached, no Error lines. Zero trust on the rest.

Also: a long wall time is not evidence of progress — the user can shoot down any subprocess. If a run exceeds 60s without artifact evidence, stop it and read the prior log before starting another.