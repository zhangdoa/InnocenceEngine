---
name: regression-build-chain
description: Use during regression bisects on engine code. Engine-specific build-chain notes for shader compile, deploy_runtime_payload, paranoid bisects across binding refactors, and clangd index purge.
---

# Skill: regression-build-chain

Generic bisect procedure (Confirm → Baseline → Bisect → Identify → Understand → Fix) lives in user-level `regression-debug` skill. This skill covers engine-specific build-chain notes that bisect candidates need.

## Build chain notes

- Mirror-semantic: shader compile drops orphan `.dxil`; `inno_deploy_runtime_payload` wipes-and-recopies.
- Paranoid bisects across binding refactors → `Scripts/HLSL2DXIL_NoPause.ps1 -FullClean`.
- Clangd index purge automated via `Scripts/PurgeStaleClangdIndex.ps1` and the `post-checkout` / `post-merge` hooks.

## Cross-references

- User-level `regression-debug` — the bisect procedure this addendum supports.
- `perf-frame-budget` — paired: governs per-bisect-step cost.
- `test-etiquette` — bisect steps require user-verified launches.
