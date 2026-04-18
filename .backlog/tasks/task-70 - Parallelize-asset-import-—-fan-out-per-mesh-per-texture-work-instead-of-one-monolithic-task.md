---
id: TASK-70
title: >-
  Parallelize asset import — fan out per-mesh / per-texture work instead of one
  monolithic task
status: To Do
assignee: []
created_date: '2026-04-18 17:13'
labels:
  - performance
  - asset-pipeline
  - tooling
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`AssetService::Import` currently submits exactly one worker task per file (one glTF/FBX → one TaskScheduler submission). The Assimp parse + BC compression of N textures + mesh serialization all run serialized on that single worker, while the other 15 workers sit idle. On the full 5-model Y-key bake, total wall-clock is dominated by Sponza main; texture BC compression is the hot path and embarrassingly parallel.

Fan out. Two axes:

1. **Per-file parallelism**: `f_convertModel` calls `Import` 5 times in sequence inside one OneShot callback — submit them from the caller so they queue simultaneously.
2. **Within-file parallelism**: After `Assimp::Importer::ReadFile` parses the scene (single-threaded), the per-mesh serialization + per-texture BC encode + per-material JSON write are all independent. Submit them as fine-grained sub-tasks and wait on completion before saving the child scene JSON (which needs the mesh-to-material table).

Complements TASK-68 (headless bake mode); a headless baker gains even more from parallelism. Ideally share the same "import one file → fan-out tasks" primitive between the interactive Y-key path and the `-bake` CLI.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 f_convertModel launches the 5 model imports in parallel (not serial)
- [ ] #2 Inside a single Import, BC texture compression runs on the TaskScheduler across workers
- [ ] #3 Child scene JSON is written only after all mesh/material/texture tasks for that file complete
- [ ] #4 Bake of the 5 Y-key models wall-clock-faster than the single-worker baseline (record the number in the task's final summary)
- [ ] #5 No race in AssetService registries — the existing deque+mutex design should cover it, but verify under stress
<!-- AC:END -->
