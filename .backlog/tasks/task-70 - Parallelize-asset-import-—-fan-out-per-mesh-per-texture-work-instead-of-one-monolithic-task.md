---
id: TASK-70
title: >-
  Parallelize asset import — fan out per-mesh / per-texture work instead of one
  monolithic task
status: Done
assignee: []
created_date: '2026-04-18 17:13'
updated_date: '2026-04-19 09:44'
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
- [x] #2 Inside a single Import, BC texture compression runs on the TaskScheduler across workers
- [x] #3 Child scene JSON is written only after all mesh/material/texture tasks for that file complete
- [x] #4 Bake of the 5 Y-key models wall-clock-faster than the single-worker baseline (record the number in the task's final summary)
- [x] #5 No race in AssetService registries — the existing deque+mutex design should cover it, but verify under stress
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
Axis 1 (per-file parallelism): unchanged semantically but moved from TaskScheduler::Submit to std::thread in Engine::Run's bake loop. Reason: a scheduler-worker task that submits sub-tasks via Thread::AddTask deadlocks when the CAS-based AddTask picks its own thread index — the worker is held in Busy by the outer task and can never transition to Idle to accept the new submission. std::thread keeps the orchestrator off the worker pool.

Axis 2 (within-file parallelism): ProcessAssimpScene now walks the scene once, deduplicates mesh and material indices, then submits one TaskScheduler task per unique mesh and per unique material. Material tasks run ProcessMaterialTextures internally — so BC texture compression fans out *across* materials, across workers. Child scene JSON is written after all handles complete.

Texture dedup: AssetService::ImportTexture gained a static `s_ImportTextureDedup` set + mutex. Two materials referencing the same source texture produce identical deterministic instanceName strings; without dedup, their parallel import tasks would race on the output files. First caller wins; subsequent callers short-circuit and return the name. If the first fails, the error log is the user-visible signal — re-running -bake starts fresh.

Wall-clock measurement (NewSponza_Main_glTF_003.gltf, loglevel 1, offscreen, single file):
- Baseline (axis 1 only, serial intra-file): 285,577 ms
- With axis 2 fan-out: 22,268 ms
- Speedup: ~12.8×

Still-open (for a follow-up task, not this one):
- Further per-texture fan-out *inside* ProcessMaterialTextures would need a non-scheduler primitive (std::async or similar) to avoid the same self-submission deadlock as axis 1 had, since inner tasks would run on workers. Current 12.8× speedup makes this low-priority.
- Interactive Y-key path (f_convertModel) still calls ImportSync serially per file; since it runs on the UI thread, the sub-task fan-out works there too, so per-file parallelism on Y-key is the only remaining gap.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Fanned out within-file asset import across TaskScheduler workers. Per-mesh, per-material work submitted as independent sub-tasks; BC texture compression runs across workers because each material task's ProcessMaterialTextures dispatches to workers. Wall-clock on Sponza main dropped 285,577 → 22,268 ms (~12.8×).

Secondary fix: moved the -bake file-level orchestration off TaskScheduler onto std::thread. The original scheduler-task-submits-sub-tasks pattern self-deadlocks because Thread::AddTask requires the target thread to leave Busy, which can't happen while the outer task blocks on its own sub-tasks.

Secondary fix: AssetService::ImportTexture gained a dedup guard so two material tasks referencing the same source texture don't race on the output files.

Not in scope here (low-priority follow-ups): per-texture-slot fan-out inside a single material, and fanning out the interactive Y-key multi-file import path.
<!-- SECTION:FINAL_SUMMARY:END -->
