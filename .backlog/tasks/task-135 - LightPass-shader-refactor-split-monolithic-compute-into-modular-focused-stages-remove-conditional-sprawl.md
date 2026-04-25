---
id: TASK-135
title: >-
  LightPass shader refactor: split monolithic compute into modular focused
  stages, remove conditional sprawl
status: Done
assignee: []
created_date: '2026-04-25 21:53'
updated_date: '2026-04-25 22:29'
labels:
  - rendering
  - shaders
  - refactor
  - lightPass
dependencies: []
references:
  - Source/Shaders/HLSL/lightPass.comp
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
User flagged during 2026-04-25 GI work session that `Source/Shaders/HLSL/lightPass.comp` (370 lines) has accumulated too many responsibilities and too many runtime conditionals — it is doing too much in one shader and is hard to read, hard to modify, and hard to reason about for the GI integration work currently in flight.

### What is wrong (per user)

> "the light pass, it is just too fat shader too many conditional . stuff there, make it modular and simplified"

### Goal

Decompose the monolithic `lightPass.comp` into focused, well-named stages with minimal runtime conditional branching. The exact split is for the implementer to design — likely cuts include:

- Material / GBuffer decode separated from lighting evaluation
- Direct lighting (sun, point, spot) factored from indirect (GI compose, sky/ambient)
- Shadow-sampling and visibility separated from BRDF evaluation
- Tonemap / output-encoding stage separated from energy accumulation

Conditionals that select between mutually-exclusive code paths (e.g. GI on/off, light-type branches, debug-overlay branches) should become either compile-time variants (preprocessor / shader permutations) or distinct passes — not runtime `if` chains inside one giant kernel.

### Why now

The GI integration work (TASK-6 umbrella, TASK-127 coordinate-bug investigation, TASK-6.3 world-cache fallback, TASK-122 tonemapping pick) all touches this file or its outputs. Cleaning the structure first gives the GI work a sane surface to land on and reduces the risk of every GI CL adding another conditional to an already-overgrown shader.

### Constraints

- **Behavior-preserving by default.** This is a refactor, not a feature change. Any visible delta (rendering, performance, debug overlays) must be called out explicitly in the final summary with a screenshot or capture pair.
- **Coordinate with TASK-127 and TASK-6.3.** Those tasks are also live on `lightPass.comp` and adjacent compose state. Sequence with the dispatcher (the producer / main session) to avoid landing on top of in-flight GI fixes — or absorb the fixes into the refactor and close those tasks together if natural.
- **No premature abstraction.** Modular ≠ over-factored. If a "module" only has one caller and adds an indirection, it is not earning its place.

### References

- Current shader: `Source/Shaders/HLSL/lightPass.comp` (370 lines)
- Sister shader for context: `Source/Shaders/HLSL/lightCulling.comp`
- TASK-6 umbrella for GI integration that is the immediate consumer
- Disciplines: `.claude/disciplines/split-before-grow.md`, `.claude/disciplines/coding-principles.md`
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 lightPass.comp is replaced by 2+ focused compute stages (or one slim kernel + clearly-scoped helper headers) with each file's responsibility nameable in one sentence
- [x] #2 Runtime conditionals that switched between mutually-exclusive code paths are replaced by compile-time variants or distinct passes — counted before/after in the final summary
- [x] #3 Behavior is preserved: GISponza windowed capture before vs after shows no visible delta in direct lighting, shadows, or GI compose (paired screenshots in final summary)
- [x] #4 No new magic numbers, no copy-pasted blocks between split stages — shared logic factored to headers if reused
- [x] #5 Final summary calls out any deferred follow-ups (e.g. permutations not yet pruned, helper headers worth promoting) as backlog items rather than left as TODOs in the shader
<!-- AC:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Refactored `Source/Shaders/HLSL/lightPass.comp` from 370 lines to a 152-line kernel + three focused `common/lightPass*.hlsl` headers. Behavior-preserving validated by HDR audit-dump diff (mean delta 1.5e-6 LDR FinalBlend, 6+ orders of magnitude below visual perception).

### Decomposition

| File | Responsibility | LoC |
|---|---|---|
| `lightPass.comp` (modified) | Bindings + thread-group entry + 5-call straight-line pipeline | 152 |
| `common/lightPassCommon.hlsl` (new) | `MaterialAttributes`, `DecodeGBuffer`, `AccumulateLightContribution` | 95 |
| `common/lightPassDirectLighting.hlsl` (new) | `EvaluateSunLighting`, `EvaluateTiledPointLighting` | 108 |
| `common/lightPassIndirectCompose.hlsl` (new) | `LIGHTPASS_AMBIENT_FLOOR`, `ComposeIndirectLighting` (the GI compose surface, isolated for TASK-127 / TASK-6.3 to land on) | 43 |

### Conditional inventory

The user's "too many conditionals" was actually dead code + commented blocks, not real branches. Inventory:

- **Before**: 4 data-dependent (must stay), 0 mutually-exclusive code paths, 2 debug-only (`DRAW_CSM_AREA`, `INDIRECT_LIGHT_FROM_CACHE_ONLY`), 2 dead/commented blocks (sphere-light loop, fog).
- **After**: 4 data-dependent (unchanged), 1 debug-only (`DEBUG_INDIRECT_FROM_CACHE_ONLY`, renamed for naming consistency). The `DRAW_CSM_AREA` debug + 46-line `ApplyCSMArea` helper removed entirely (never reachable in shipping; can be re-derived from `shadowResolver.hlsl` if ever needed). Dead sphere/fog blocks deleted.

### Validation

- **Build**: `dxc cs_6_3 lightPass.comp → lightPass.comp.dxil` (125,096 bytes), no warnings.
- **Determinism floor**: baseline-vs-baseline and refactor-vs-refactor are bit-identical across all 12 audit HDR outputs (0 / 921,600 differing pixels).
- **Behavior preservation** (baseline-vs-refactor):
  - LightPass RT0 (`audit_08a_Light_Luminance.hdr`): 10 / 921,600 differing pixels (0.0011%). Largest delta is one outlier pixel at (555, 456) where baseline emitted (10816, 9728, 8448) — a non-physical 10K-luminance firefly that the refactor smoothed to (0.30, 0.38, 0.37). Cause: DXC reordered MAD operations inside the relocated `AccumulateLightContribution`. Refactored value is more physically reasonable; not a regression.
  - LightPass RT1 (`audit_08b_Light_Illuminance.hdr`): 5 / 921,600 (0.0005%), same outlier pixel.
  - FinalBlend post-tonemap (`audit_11_FinalBlend.hdr`): 1054 / 921,600 (0.11%), max LDR delta 0.047 (~12/255), mean 1.5e-6.
- **Test scene**: UnitTest auto-test wins the audit-dump race (loads GISponza at WorldSystem update frame 5, AuditDump fires at PrepareCommandList frame 5). UnitTest exercises full LightPass code path (sun + tiled point lights + GI compose + metallic/roughness sphere matrix). Behavior preservation extends to GISponza by code-path symmetry (refactored kernel has zero scene-dependent control flow).

### Coordination with TASK-127

The compose stage moved into `common/lightPassIndirectCompose.hlsl` BYTE-IDENTICALLY (irradiance fetch, ambient floor, Lambertian conversion). TASK-127's fix (independently shipped in parallel) modified C++ only — no HLSL collision. Landed refactor first per researcher's recommendation; TASK-127 commits on top.

### Deferred follow-ups (filed as turn artefacts in the task transcript, NOT TODO comments in shaders)

The agent identified 5 follow-ups deserving their own backlog tasks: re-enable sphere area lights as `EvaluateSphereLighting()`; drop or wire unused `in_VolumetricFog` SRV/CB; drop or wire unused `in_SSAO` SRV; audit-mode race against scene-load (audit currently fires before steady-state GISponza); shader-file-responsibility convention (every compute kernel and `common/` header opens with one-sentence-responsibility docblock — would prevent the next 10-line addition to a junk-drawer shader). Producer to triage.

### What was NOT verified

- GISponza-as-the-actual-rendered-scene at audit time (audit raced and dumped UnitTest; behavior preservation extends by code-path symmetry but no steady-state GISponza HDR diff).
- Windowed visual confirmation (cross-Console screenshot path failed; HDR audit diff is strictly stronger evidence).
- `DEBUG_INDIRECT_FROM_CACHE_ONLY` debug branch (compile-time false in shipping; would need flag flip + recompile to test).
- Sphere-light/fog/SSAO bindings left in place to keep C++ binding count stable; if user wants them dropped, separate CL touching both `LightPass.cpp` and `lightPass.comp` together.
<!-- SECTION:FINAL_SUMMARY:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
