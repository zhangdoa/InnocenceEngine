---
id: TASK-239
title: >-
  Shader unit tests capability — DXC compile + validate HLSL modules in CI
status: To Do
assignee:
  - code-impl
created_date: '2026-06-15'
labels:
  - rendering
  - shader
  - testing
  - capability
  - ci
dependencies:
  - TASK-227.2
references:
  - CMake/CompileHlslShaders.cmake
  - Source/Engine/RenderGraph/
  - TASK-234
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by TASK-227.2 (commit `df40414a`, full `lightPass.comp` swap, 2026-06-14). User-directed new direction after the swap landed.**

### Why this exists

The engine has a TestSuite that runs C++ unit + integration tests (106–113 tests depending on the increment), but NO equivalent for the HLSL shader layer. The HLSL surface area is comparable in size and complexity (24+ shader files under `Source/Shaders/HLSL/`, 51-pass migration's worth of binding-table assertions duplicated into JSON). Today the only validation is:

- `CMake/CompileHlslShaders.cmake` globs `*.comp/.vert/.frag` and compiles them with DXC, checking only that DXC returns 0. No semantic assertions, no per-binding type checks, no per-uniform-layout validation.
- Per-shader assertions live in `RenderGraphSerializerTests_*.cpp` (round-tripping the JSON's binding table against the imperative reference). These are integration-grade, not shader-grade.
- The GISponza autotest (`TestGIScene.ps1`) does an end-to-end pixel-MAE gate at 0.45 against a CPU reference. This is a USER-VISIBLE integration test, not a shader unit test, and it's the only safety net catching shader-side regressions.

The gap is concrete: `df40414a` landed with two latent shader bugs that the **bypassed** `LightPassFull` scaffold had never compiled live and so had never caught — the light/GI CBs were `ReadWrite` (wrong root-sig range, SRV collision), and 3 dead bindings (Voxelization CBuffer, GI CBuffer, LightPass/Volumetric Fog) were declared but unread. Both were caught only because the graph's strict imported-resource resolver refused to load the JSON; in the imperative path, they'd have been silent GPU-side mismatches.

A shader unit-test capability would have caught the binding-count and root-sig-range issues at compile time, before the JSON refused to load.

### What "shader unit tests" means here

A focused, surgical capability — NOT a full reference harness. The bar is:

1. **Compile-only checks** — DXC emits the .dxil; assertions in test body inspect warnings/errors metadata, root-signature descriptor ranges, register counts per binding slot, expected resource types.
2. **Binding-table parity** — for every migrated JSON node, a C++ test loads the JSON, instantiates the HLSL, and asserts the JSON's `Bindings[]` count + `m_DescriptorSetIndex` + `m_DescriptorIndex` exactly matches the HLSL `register(t#, s#, b#, u#)` declarations. The catch mechanism that surfaced the `df40414a` bugs.
3. **Per-shader dispatch-shape** — for a sample of shaders (3-5 representative ones: `lightPass.comp`, `sunShadowRT.comp`, `taaPass.comp`, `ssaoPass.comp`, `opaquePass.vert/.frag`), a test that calls DXIL reflection to enumerate UAV/SRV/CBV counts and asserts they match a hand-authored "expected shape" table. Catches dead bindings, missing barriers, missing output writes.

NOT in scope:
- GPU execution / execution-time assertions (no compute-shader-test framework; huge surface, separate project).
- RenderDoc / PIX integration.
- Reference-image regression (the GISponza autotest already covers this; this task is about catching the HLSL-shape bugs that the autotest misses).
- Editing HLSL to make it "more testable" — the existing HLSL is the system under test.

### Why medium (not high)

Build green, integration tests green, user-visible lit frame achievable. This is a CAPABILITY addition, not a defect fix. But every shader change today is one CI cycle away from a silent GPU-side mismatch; the cost-of-being-wrong is the kind of misdiagnosis that took 1+ week in 2026-06 (the full LightPass swap reveal). Proactive capability, not reactive.

### Why now (post-TASK-227.2)

The render-graph has fully data-driven bindings; the binding-table shape is a JSON contract. A C++ test can now load the JSON and a HLSL file in the same test body and assert the contract holds — without this refactor, the test would have to instantiate the imperative pass class, which defeats the purpose of the graph.

### Decompose during planning

- **Phase 0 — design** (TASK-239.0): pick the test-framework shape (DXIL reflection API vs hand-parsed `dxc -Qstrip_reflect` output), the test-runner location (extend TestSuite or a new sibling), the bar for "expected shape" tables (hand-authored golden files vs generated from HLSL via libclang), and the ordering of binding assertions.
- **Phase 1 — capability** (TASK-239.1): the compile + reflect infrastructure, the DXIL-reflection API choice, the TestSuite wiring, ONE end-to-end example test (recommend `lightPassFull`-style node) that catches the kind of bug the 2026-06-14 swap revealed.
- **Phase 2 — coverage** (TASK-239.2): roll out to the migrated graph nodes (~14 today) + a representative subset of the imperative/archived shader surfaces.
- **Phase 3 — CI gating** (TASK-239.3): make the shader unit tests a commit-gate class (mirroring the C++ TestSuite gate) so future HLSL edits can't silently regress binding shapes.

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Phase 0 RFC filed at `.backlog/docs/doc-N-task-239-shader-unit-tests-design.md` covering: DXIL-reflection API choice, TestSuite integration shape, expected-shape authoring bar, build sequence
- [ ] #2 Phase 1 capability: a shader unit test runs in `TestSuite` and catches the `df40414a` bug class (binding-count mismatch, root-sig-range mismatch, dead-binding declared-but-unread) for at least one representative shader
- [ ] #3 Phase 2 coverage: shader unit tests exist for all currently-migrated render-graph node shaders (per `Data/ExampleProject/RenderGraph/ExampleRenderGraph.json`); pass on clean HEAD
- [ ] #4 Phase 3 gating: a failure in a shader unit test blocks the build via the existing commit-guard / TestSuite gate (or a new one), preventing silent regression
- [ ] #5 Build green; TestSuite green (existing tests + new shader tests); TestGIScene MAE unchanged
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this is the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
