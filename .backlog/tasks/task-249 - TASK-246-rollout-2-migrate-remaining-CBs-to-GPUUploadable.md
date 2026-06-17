---
id: TASK-249
title: >-
  TASK-246 Rollout §2: migrate remaining 8 GPU-bound structs to GPUUploadable<T>
status: To Do
assignee:
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

- [ ] All 8 remaining CBs derive `GPUUploadable<T>` with the
      `SkipByteRanges` override and the `static_assert`s.
- [ ] All 8 producers do `PoisonInit()` and write every field
      explicitly.
- [ ] Live-engine smoke (`Main.exe -c PT.json -total_frames 3 -offscreen 1`)
      is clean for each migration (no new Error lines from the
      validator).
- [ ] Each migration lands in its own CL with the test passing (run the
      existing `Bin/RelWithDebInfo/GPUUploadableTests_Standalone.exe`
      plus the live-engine smoke for the affected struct).

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

### Related

- TASK-246 (the umbrella task; this is the Rollout §2 work)
- TASK-227 (render graph; the migrated CBs are fed by graph outputs)

## Session log

- **2026-06-16**: rollout scope defined in this task. Migration work
  deferred until TASK-247 (TextureResourceService init-loop) is fixed
  so the live-engine smoke path is unblocked.
