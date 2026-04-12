---
id: TASK-11
title: >-
  Decide texture storage strategy: copy to Data/ vs reference OriginalAssets/
  directly
status: To Do
assignee: []
created_date: '2026-04-12 11:50'
labels:
  - assets
  - pipeline
dependencies:
  - TASK-10
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The texture import fix (TASK-9 / texture-path PR) resolved paths at import time by threading `modelBaseDir` through the Assimp wrapper chain. At runtime, textures are still loaded from `../OriginalAssets/` which is outside the engine's `Data/` tree and outside the Bin/ working directory.

Two strategies are possible:

**Option A — Copy source textures into Data/**
- At import time, copy referenced textures from `../OriginalAssets/...` into `Data/ExampleProject/Textures/` (or a per-model subdirectory).
- TextureComponent JSON stores the `Data/`-relative path.
- Runtime load is simple: all assets live under `Data/`.
- Downside: duplicates potentially GBs of source texture data.

**Option B — Reference OriginalAssets/ directly (status quo after fix)**
- MaterialComponent JSON stores the `../OriginalAssets/...` path.
- Runtime load must resolve from Bin/ working directory (currently works after the fix).
- No data duplication; assets remain in one place.
- Downside: `Data/` tree is not self-contained; shipping or CI requires the OriginalAssets tree too.

**Decision point**
The choice here directly affects TASK-10 (BC compression): if we go with Option A, the DDS files live in `Data/`; if Option B, they live alongside the source textures or in a parallel compressed cache.

This task is to evaluate both options against the project's goals (self-contained Data/, CI build, disk budget) and record the decision in a design note or CLAUDE.md before TASK-10 is implemented.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Decision documented (design note or CLAUDE.md update) with rationale
- [ ] #2 If Option A: import pipeline copies textures to Data/ at import time
- [ ] #3 If Option B: document that OriginalAssets/ is a runtime dependency and update onboarding docs
- [ ] #4 Decision is consistent with the BC compression plan in TASK-10
<!-- AC:END -->
