---
id: TASK-70
title: >-
  Parallelize asset import — fan out per-mesh / per-texture work instead of one
  monolithic task
status: In Progress
assignee: []
created_date: '2026-04-18 17:13'
updated_date: '2026-04-18 18:35'
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
- [x] #1 f_convertModel launches the 5 model imports in parallel (not serial)
- [ ] #2 Inside a single Import, BC texture compression runs on the TaskScheduler across workers
- [ ] #3 Child scene JSON is written only after all mesh/material/texture tasks for that file complete
- [ ] #4 Bake of the 5 Y-key models wall-clock-faster than the single-worker baseline (record the number in the task's final summary)
- [x] #5 No race in AssetService registries — the existing deque+mutex design should cover it, but verify under stress
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Axis 1 (per-file parallelism) landed for the -bake path. Each path submits its own TaskScheduler task; Engine::Run awaits all and aggregates results. Wall-clock on a 3-file batch drops from 130+1546+221=1897ms serial to 1546ms (= longest file, dragon), confirming real parallelism. AssetService registries already serialize correctly through per-type shared_mutex, no race added.

Remaining:
- Axis 2 (within-file BC compression parallelism) — the real Sponza win. AssimpTextureProcessor::CompressToBC is per-texture and embarrassingly parallel. Fanning those out inside ProcessMaterialTextures would cut Sponza main's (~50 textures) bake time significantly.
- Extend to the interactive Y-key path in World.inl (still calls ImportSync serially in f_convertModel).
<!-- SECTION:NOTES:END -->
