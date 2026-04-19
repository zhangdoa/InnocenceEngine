---
id: TASK-111
title: >-
  Add serialization determinism test tier — scene load → save must be byte-equal
  (idempotent round-trip)
status: To Do
assignee: []
created_date: '2026-04-19 19:49'
labels:
  - test
  - scene-save
  - serialization
  - harness
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What

We don't currently test that `Load(scene) → Save(scene)` is a no-op on disk. TASK-110 surfaced four separate drift sources (missing ComponentType, entity-name-based filenames, wrong project dir, float precision expansion) that a determinism test would have caught immediately on the first CI run.

## Proposed tier

A new offscreen integration test that:

1. Boots Main.exe in a dedicated mode: load a specific scene, immediately call `SceneService::Save` on it, exit.
2. Compares the pre-load tracked `.InnoScene` + all referenced component JSONs against their post-save state.
3. Exits non-zero on any diff (file list + first 50 lines of each).

Invocation shape (proposal — exact flag name TBD):

```
Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -serialize_test ExampleProject/Scenes/UnitTest.InnoScene
```

## Why

- Scene save is the hot spot for silent data corruption (TASK-110). Every commit touching the serializer has been untested for this.
- Float precision, field ordering, and filename generation all drift under refactors; a determinism test pins them down.
- Current CI tiers (RenderTest / tier-2 frame / tier-3 reload) don't exercise Save at all.

## Dependencies

- TASK-110 must land first (or alongside) — right now the test would immediately fail against every scene because of the known bugs, and the test wouldn't have a clean green baseline to regress against.

## Acceptance

- A new tier invocation exists and is documented in `CLAUDE.md` test-tier list.
- On a freshly-loaded tracked scene, the serialize-test exits 0.
- On a deliberately-introduced serializer bug (e.g. revert the ComponentType fix), the test exits non-zero and lists the diverging file(s).
- The hook (`.claude/hooks/commit-gate.js`) is extended so any commit touching `Source/Engine/ThirdParty/JSONWrapper/` or `Source/Engine/Services/AssetService.*` or `SceneService.*` requires the serialize-test to have run in the current turn.

## Not in scope

- Testing binary-blob components (mesh / texture binary files). Scope here is just the JSON surface.
- Format migration between versions — determinism is on the current format only.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
