---
name: shader-standards
description: "HLSL/GPU correctness invariants for InnocenceEngine — non-obvious failures: deadlock, silent corruption, collapsed geometry."
---

| Invariant | Rule |
|---|---|
| Matrix multiply | row-vector × matrix: `mul(v, g_Frame.p_inv)`. The engine uploads row-major; reversed order transposes the op |
| Missing transform | fall back to identity, never `Mat4{}` — all-zeros collapses geometry |
| Cross-queue UAV write | `DeviceMemoryBarrier()` after writes a different queue consumes; a fence retires the command list, not write visibility |
| `*WithGroupSync` barrier | every thread reaches it — no early `return` before one (deadlock / silent hang). Compute an `earlyExit` flag, keep participating in barriers, branch only at the write phase |

Canonical patterns: `common/skyResolver.hlsl` (mul order), `RadianceCacheReprojection.comp` (earlyExit + uniform barriers).
