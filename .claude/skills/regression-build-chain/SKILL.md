---
name: regression-build-chain
description: Use during regression bisects on engine code. Engine-specific build-chain notes for shader compile, deploy_runtime_payload, paranoid bisects across binding refactors, and clangd index purge.
---

# Skill: regression-build-chain

Generic bisect procedure (Confirm → Baseline → Bisect → Identify → Understand → Fix) lives in user-level `regression-debug` skill. This skill covers engine-specific build-chain notes that bisect candidates need.

## Build chain notes

- Mirror-semantic: shader compile drops orphan `.dxil`; `inno_deploy_runtime_payload` wipes-and-recopies.
- Paranoid bisects across binding refactors → `Scripts/HLSL2DXIL.ps1 -NoPause -FullClean`.
- Clangd index purge automated via `Scripts/PurgeStaleClangdIndex.ps1` and the `post-checkout` / `post-merge` hooks.

## Forbid `git checkout` / `git stash` / `git reset` in diagnostic dispatches

Diagnostic dispatches (skip-feature probes, per-pass RT dumps, hypothesis tests) must use **edit-and-revert in place** on the main repo, not git-state mutation. Allowed: `git status`, `git diff` for verification. Forbidden: `git checkout` to alternate SHAs, `git stash push`, `git reset --hard`, branch switches.

Failure mode (observed): a bug-fix dispatch instructed to "use worktree isolation" can operate on the main repo anyway and leave HEAD detached at an old SHA. Main-session's session commits become reflog-only until recovered via `git checkout <branch>`. Reflog recovery is reliable but costs a roundtrip and risks losing uncommitted work.

The brief should be explicit: "DO NOT do any `git checkout` / `git stash` / `git reset` operations. Edit-and-revert in place. Verify tree clean via `git diff` before reporting." The sub-agent's tool surface includes `Bash`; the only line of defense is the brief.

## Cross-references

- User-level `regression-debug` — the bisect procedure this addendum supports.
- `perf-frame-budget` — paired: governs per-bisect-step cost.
- `test-etiquette` — bisect steps require user-verified launches.
