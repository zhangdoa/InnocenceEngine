---
id: TASK-227
title: >-
  Declarative render-pass resource graph (replace imperative ::Setup with
  serialized JSON/data declarations)
status: In Progress
assignee:
  - code-impl
created_date: '2026-05-15 20:32'
updated_date: '2026-05-31 17:20'
labels:
  - rendering
  - engine-architecture
  - render-graph
  - refactor
  - umbrella
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/
  - >-
    Source/ExampleProject/RenderingClient/ExampleRenderingClient_ExecuteCommands_GI.cpp
  - >-
    Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass_Setup.cpp
  - Source/Foundation/Serialize/
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Why this exists

The engine currently expresses render-pass resource ownership and pass-to-pass dependencies imperatively in C++ ::Setup methods (51 `*Pass.cpp` files under `Source/ExampleProject/RenderingClient/` plus the dispatch-graph in `ExampleRenderingClient_ExecuteCommands_*.cpp`). Examples:

- `ComputeCullingPass::Setup` (and every sibling) imperatively creates buffers, declares texture sizes, binds resources, in code that must be touched whenever a binding changes.
- Pass-to-pass dependencies live as `WaitIfActive(UpstreamPass::Get(), ...)` chains in the ExecuteCommands_*.cpp files, with no single declarative source of truth — a misordered `WaitIfActive` is invisible until runtime.
- Resource lifetime, format, descriptor-heap slot, and queue affinity are scattered across the ::Setup body and the binding layout `.cpp` next door.

## Direction

Move render-pass description to **data**:
- Resources (textures / buffers / RTVs / DSVs / UAVs) declared in JSON or another serialized form. Format, size-expression, lifetime tier, and queue affinity are columns.
- Pass-to-pass dependencies declared as `reads:` / `writes:` lists per pass. The engine builds the dependency DAG from the reads/writes intersection — no manual `WaitIfActive` chains.
- C++ side: a single render-graph loader/compiler reads the data, allocates resources, schedules passes in topo order, emits the cross-queue fences. The per-pass `.cpp` shrinks to the kernel-body command-recording.

## Scope (umbrella — decompose in Phase 0)

The end state is engine-wide. The work is multi-phase:

**Phase 0 — design / RFC**:
- Pick the serialization shape (JSON vs YAML vs in-house format vs reuse `Source/Foundation/Serialize/`).
- Pick the resource-graph compiler shape (build at engine startup vs hot-reload vs offline-compiled).
- Inventory the 51 existing Pass.cpp files: which already have clean Setup-as-data shapes (PTHashGridCache* siblings look uniform), which carry exotic logic that can't move to data (eg. dynamic dispatch sizes from CPU-side state).
- Decide the migration order (one-at-a-time vs all-at-once).

**Phase 1+** — incremental Pass.cpp migrations decomposed during Phase 0.

## Examples of pain this fixes

- `RadianceCacheReprojectionPass_Setup.cpp` is its own 261-line file because the Pass.cpp grew past the 300-line ratchet — a data-driven Setup eliminates the split.
- `ExampleRenderingClient_ExecuteCommands_GI.cpp` is 158 lines of stamped-out `WaitIfActive → Execute → SignalOnGPU → WaitOnGPU` cycles — every pass identical except for the upstream pass reference. A render-graph engine emits all of this from data.
- TASK-182 (bypass-clear semantics), TASK-171 (m_Bypassed honour) — these live in the dispatch site because there's no central pipeline description; with a graph, bypass becomes a node attribute.

## What this supersedes / orthogonal to

- Orthogonal to TASK-226 (GI port). Both touch Pass.cpp files but at different layers — TASK-226 changes the HLSL bodies, this changes the pass-description scaffolding around them.
- Orthogonal to TASK-128 (ping-pong helper). This umbrella may make TASK-128 obsolete if ping-pong becomes a node attribute.
- Touches the same surface as TASK-198 (unused-includes sweep) — both reduce per-Pass.cpp surface area but for different reasons.

## Why high priority

Pass.cpp boilerplate is the dominant friction tax on new rendering work. Every new pass costs ~150–300 LOC of stamped-out Setup + binding-layout + ExecuteCommands wiring before any kernel logic. Going data-driven cuts that to a JSON entry + a kernel body. Over the next dozen rendering tasks (TASK-226.{3..8}, TASK-77.x, future paper ports) the saved per-pass scaffolding cost amortizes the refactor.

## Out of scope (initial scope; revisit in Phase 0)

- Replacing the engine's `RenderPassComponent` / `CommandListComp` types themselves. Graph data targets these existing types; type replacements are a separate concern.
- Hot-reload of the graph data at runtime. Phase 0 picks startup-load OR hot-reload — likely startup first.
- Editor-side authoring UI for the graph. The data is text-editable; an editor comes later.

## Phase 0 deliverables

1. RFC document at `.alignments/TASK-227-render-graph-design.md` (or equivalent next-id) covering: data format choice, compiler shape, migration order, anti-patterns inventory from current 51 Pass.cpp files.
2. Working POC migrating ONE simple pass (probably `BRDFLUTPass` — no inputs, single output, no dependencies) to the data-driven form, demonstrating the shape.
3. Phase 1+ decomposed sub-tasks filed against the inventory.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 #1 Phase 0 RFC at .alignments/TASK-<N>-render-graph-design.md: data-format pick, compiler-shape pick, migration order, inventory of 51 Pass.cpp files binned by migration difficulty
- [x] #2 #2 POC: one pass (recommend BRDFLUTPass) migrated to the data-driven form end-to-end (loaded from data at startup, dispatched in correct topo order, build + runtime smoke green)
- [x] #3 #3 Phase 1+ sub-tasks filed against the inventory — one per migration batch, dependency-ordered
- [ ] #4 #4 All 51 Pass.cpp files migrated to the data-driven form OR explicitly exempted in the RFC with reason cited
- [ ] #5 #5 ExampleRenderingClient_ExecuteCommands_*.cpp imperative WaitIfActive/Execute/Signal chains replaced by render-graph-emitted dispatch — the orchestrator file shrinks to a loader-and-run entry point
- [ ] #6 #6 60-FPS bar preserved on the Sponza autotest — graph compile cost amortized across frames (or measured + acknowledged as one-time-at-startup)
- [ ] #7 #7 Build green; existing integration tests green; visual parity vs pre-refactor screenshots on all autotest scenes
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-05-31 — Phase 0 design landed. RFC = backlog doc-1 (format=JSON-on-existing-nlohmann, compiler=startup-load+in-memory, kernel-registration with Default/override/opaque tiers mapping the 3 difficulty bins). AC#1 (RFC+inventory) and AC#3 (Phase 1+ sub-tasks .1-.4) done. AC#2 (BRDFLUTPass POC) in progress. AC#4-7 are whole-umbrella, deferred to TASK-227.{1..4}.

2026-05-31 — AC#2 POC implemented (PENDING PEER REVIEW, not committed). New module Source/Engine/RenderGraph/: RenderGraphDesc.h (POD structs), RenderGraphEnumStrings.{h,cpp} (string<->enum for the plain enum-class graphics enums, which lack INNO_ENUM ToString), RenderGraphSerializer.{h,cpp} (hand-written to_json/from_json), IRenderGraphKernel.h + DefaultKernel.{h,cpp} (binds Reads/Writes per binding table, static Dispatch), RenderGraphService.{h,cpp} (load->deserialize->create resources via *ResourceService->create RenderPassComponent+binding layout+CLs+attach kernel; ScheduledNodes() is the topo-sort extension point). Data: Data/ExampleProject/RenderGraph/ExampleRenderGraph.json (BRDF LUT 512x512 Float16 RGBA + BRDFLUTPass node). Coexistence seam (RFC §10): BRDFLUTPass::Setup adopts the graph node's pointers under a constexpr g_UseRenderGraph flag; imperative body preserved verbatim under the false branch (reversible). PrepareCommandList delegates to RenderGraphService::RecordNode. All consumers (LightPass, ExecuteCommands, BRDFLUTMSPass, AuditDump) unchanged via the singleton accessors.

2026-05-31 — AC#2 verification: Build green (BuildWin.ps1 MSVC RelWithDebInfo, Main+RenderTest exit 0). Unit test RenderGraphSerializerTests (enum + full-graph round-trip) added to TestSuite, 105/105 pass. Runtime smoke: Main.exe -total_frames 120 GISponza graph-driven — exit 0, scene loaded, auto-terminated, 0 D3D12 errors, logged 'RenderGraphService loaded graph [ExampleRenderGraph] with 1 resources and 1 passes'. Visual parity: audit-dumped BRDF LUT graph-vs-imperative MAE=0 (BIT-IDENTICAL via magick compare). Captures: Build/captures/brdflut_{graph,imperative}.hdr.

2026-05-31 — DISCOVERY (surface, not chase): (1) TestGIScene.ps1 whole-scene MAE threshold 0.45 is ALREADY exceeded on clean ecs-overhaul HEAD (baseline 0.556-0.559 across 3 runs vs graph build 0.499) — pre-existing stale-CPU-reference issue on this branch, unrelated to TASK-227. (2) -audit mode crashes at process teardown (exit -1073740791) identically on imperative AND graph builds AFTER all dumps complete — pre-existing audit-shutdown bug, not introduced here. Both candidates for separate tasks if not already tracked.

2026-05-31 — Phase 0 COMPLETE (AC#1-3 checked). POC committed 9e6d5acb (RenderGraph module + data-driven BRDFLUTPass, build green, TestSuite 105/105, GISponza smoke exit 0, BRDF LUT MAE=0 vs imperative). Paperwork committed a08b564c. RFC = backlog doc-1 (untracked in backlog store per venue decision). Umbrella stays In Progress: AC#4-7 are whole-engine migration, tracked in TASK-227.{1..4}.
<!-- SECTION:NOTES:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
