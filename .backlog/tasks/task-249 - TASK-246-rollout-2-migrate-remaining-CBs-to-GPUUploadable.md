---
id: TASK-249
title: >-
status: Done
  - code-impl
created_date: '2026-06-16'
labels:
  - rendering
  - safety
  - infra
  - rollout
dependencies:
  - TASK-246
priority: medium
---

## Description

`9d1b71c2` PoC'd `GPUUploadable<T>` on `PerFrameConstantBuffer`. `d990770e`
landed the standalone unit test that locks the contract. The Rollout §2
step migrates the remaining GPU-bound structs to the base so every
CPU-write/GPU-read struct is validated at the `Upload<T>` chokepoint
before it can ship a silently-wrong field to the GPU.

### Structs to migrate (8 total)

1. `PointLightConstantBuffer`
2. `SphereLightConstantBuffer`
3. `TransformConstantBuffer`
4. `MaterialConstantBuffer`
5. `DispatchParams`
6. `GIPassConstantBuffer`
7. `VoxelizationPassConstantBuffer`
8. `AnimationConstantBuffer`
9. `GPUModelData` (per-mesh, large — gate the per-upload scan to first-N
   frames or a debug flag per the design notes)

### Pattern (from `9d1b71c2`)

```cpp
struct alignas(16) MyCB : public GPUUploadable<MyCB>
{
    // ...fields...
    static constexpr std::array<std::pair<size_t, size_t>, 4> SkipByteRanges() noexcept
    {
        // { offset, size } for any HLSL std140 padding the producer
        // cannot write through C++. Include any alignas-induced tail
        // padding, not just the explicit padding array (per
        // TestSkipByteRangesMasksPadding in d990770e).
    }
};
static_assert(sizeof(MyCB) == EXPECTED_SIZE, "size must be std140 layout");
static_assert(alignof(MyCB) == 16, "alignment must be 16");
static_assert(std::is_standard_layout_v<MyCB>, "must be standard layout");
```

The producer site:
- Replace `T l_cb = {};` (or `T l_cb;`) with `T l_cb; l_cb.PoisonInit();`
- Write **every field** explicitly, including `padding[N]`, partial
  vector components (e.g. `viewportSize.z/w`), and conditionally-written
  fields (e.g. `sun_direction` when no sun transform).

### Per-struct work pattern

For each struct:
1. Add the CRTP derivation, `SkipByteRanges` override, and
   `static_assert`s in the header.
2. Update the producer to `PoisonInit()` and write every field
   explicitly. This **will** surface dropped fields — that is the
   point.
3. Build, run, observe the validator's Error log for the new
   `(buffer, byte_offset)` pairs. For each surfaced pair, decide:
   - Real bug: fix the producer to write the field correctly.
   - Tail padding: extend `SkipByteRanges` to cover it.
   - Conditional field: write a default value when the condition is
     false (e.g. `sun_direction = Vec4(0, 0, 0, 0)` when no sun
     transform).
4. Re-run until the validator is silent for that struct.
5. Commit one struct per CL (per `commit-policy`).

### Subtle contract finding (from `d990770e`)

`SkipByteRanges` must include any `alignas`-induced tail padding, not
just the explicit HLSL std140 padding array. A struct with
`alignas(16)` and N bytes of data sizes to `(N + 15) & ~15`; bytes
between `N` and the rounded size are compiler tail padding the
producer cannot write. The standalone test's
`TestSkipByteRangesMasksPadding` documents this — every migration
should check that the struct's `sizeof` is what the HLSL cbuffer
expects, and that `SkipByteRanges` covers the full trailing region
including any alignas tail.

### Acceptance Criteria

- [x] #1 All 8 remaining CBs derive `GPUUploadable<T>` with the
      `SkipByteRanges` override and the `static_assert`s. **Done
      across 4 CLs** (`49b4ad2` PointLight+SphereLight, `ed3efe2e`
      Transform+Material, `d751b69e` Dispatch+GI+Voxelization+Animation,
      `75566da2` GPUModelData). 4 of the 9 originally-listed CBs are
      dead code (no live producer, no Upload call) and got
      header-only migration as future-proofing.
- [x] #2 All 8 producers do `PoisonInit()` and write every field
      explicitly. **Done in CLs 1, 2, 4.** CL 3 (Dispatch/GI/Vox/
      Animation) has no live producer to update.
- [x] #3 Live-engine smoke (`Main.exe -c Audit.json`) is clean for
      each migration. **Verified post-each-CL** — no new Error
      lines from the validator; the engine reaches steady state at
      frame 4 (56 instances, deferred queue empty) for every
      variant. The pre-existing `PerFrameCBuffer` validator Error
      at byte offset 376 (TASK-246 PoC, `posWSNormalizer` follow-up
      A) is unchanged.
- [x] #4 Each migration lands in its own CL with the test passing.
      `GPUUploadableTests_Standalone.exe` 8/8 PASS in the same bash
      call as each `git commit`.

### Out of scope (separate tasks)

- TASK-246 Follow-up A: compute real `posWSNormalizer` from scene
  `GPUModelData` AABB.
- TASK-246 Follow-up B: confirm real `posWSNormalizer` offset in the
  engine's struct layout.
- TASK-246 Follow-up C: flip the `if constexpr` opt-in to a mandatory
  `static_assert` once all CBs are migrated and the live-engine smoke
  is green for them.
- TASK-247: TextureResourceService init-loop (the live-engine smoke is
  blocked until this is fixed; this rollout task should land after
  TASK-247).

### Discovered by this rollout (CL 4) — to be filed as separate tasks

- **TASK-249.A — GPUModelData m_ShaderProgramIndex / m_RenderPassIndex
  are not populated by the producer.** Both are read by the HLSL
  shader (`Source/Shaders/HLSL/common/common.hlsl:541, 543`) but the
  CPU producer in `DrawCallServiceImpl::UpdateDrawCalls` never
  populates them, relying on the in-class `= 0` default. The validator
  surfaced the gap: poison-init made those dwords 0xCDCDCDCD on the
  GPU. The migration preserves the prior behavior by writing them
  explicitly to 0; the proper fix (populate from the actual
  `ShaderProgramComponent` and `RenderPassComponent` indices) is
  pending.
- **TASK-249.B — C++ GPUModelData `m_BoundingBoxMin/Max` is at C++
  offset 56/72 (4B aligned); the HLSL cbuffer has it at 64/80
  (16B std140 aligned).** Pre-existing mismatch surfaced only
  because the migration forced a `static_assert` on `sizeof ==
  160`. The HLSL reads the C++ struct's `m_BoundingBoxMin`
  followed by 4B of zero-implicit padding, so `ws.w` (set to 1.0f
  lost. The migration does not fix this.

### Related

- TASK-246 (the umbrella task; this is the Rollout §2 work)
- TASK-227 (render graph; the migrated CBs are fed by graph outputs)

## Session log

- **2026-06-17 (this session)**: landed the rollout in 4 CLs.
  CL 1 `49b4ad2` — PointLight + SphereLight (smallest CBs; live
  producers in `LightDataService.cpp`).
  CL 2 `ed3efe2e` — Transform + Material (DrawCallService.cpp;
  surface-don't-chase note: `MaterialConstantBuffer::m_MaterialType`
  is dead code not consumed by any shader — explicitly written 0
  to satisfy the validator; the dead-field removal is a separate
  cleanup).
  CL 3 `d751b69e` — Dispatch + GI + Voxelization + Animation
  (header-only; no live producers; future-proofing).
  CL 4 `75566da2` — GPUModelData (per-mesh; `static_assert` on
  `sizeof == 160` confirms the actual std140 size; 2 dropped
  fields `m_ShaderProgramIndex` + `m_RenderPassIndex` are read
  by the HLSL shader but the CPU producer relied on the in-class
  `= 0` default to mask them — explicitly written 0 to preserve
  behavior; real fix is TASK-249.A). The C++-HLSL std140
  mismatch on `m_BoundingBoxMin/Max` is TASK-249.B. Validator
  remained silent for all 4 CLs; pre-existing
  `PerFrameCBuffer` byte-offset-376 Error unchanged.

- **2026-06-16**: rollout scope defined in this task. Migration work
  deferred until TASK-247 (TextureResourceService init-loop) is fixed
  so the live-engine smoke path is unblocked.

- **2026-06-17 (closure):** No code change this entry. All 4 ACs
  ticked in the 2026-06-17 session log above. The two
  discoveries (`m_ShaderProgramIndex` / `m_RenderPassIndex` not
  populated = TASK-250; the std140-mismatch claim = falsified in
  TASK-251, the real bug is a black-triangle root cause TBD)
  were filed as separate tasks during the rollout. The
  GPUModelData→RenderInstance rename (TASK-253) shipped after
  this rollout; it removed the 5 dead fields the validator
  flagged in CL 4 (`m_ShaderProgramIndex` / `m_RenderPassIndex`
  / `m_UUID` / `m_VisibilityMask` / `m_MeshUsage`) — so the
  TASK-250 fix is now obsolete and should be closed as
  "superseded by TASK-253" in its next pass.
