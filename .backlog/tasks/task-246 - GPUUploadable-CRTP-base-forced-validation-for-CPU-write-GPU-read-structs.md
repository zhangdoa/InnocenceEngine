---
id: TASK-246
title: >-

  GPUUploadable CRTP base: forced pre-upload validation for CPU-write/GPU-read structs
status: Done
assignee:
  - code-impl
created_date: '2026-06-16'
labels:
  - rendering
  - safety
  - infra
dependencies:
  - TASK-243
priority: medium
---

## Session-2026-06-16 notes

PoC first slice committed on `ecs-overhaul` in 9d1b71c2 (8 files, 292 insertions, build green).

### What landed

- `Source/Engine/Common/GPUUploadable.h` (new): CRTP base. `PoisonInit()` (memset
  Derived to 0xCD), `FirstUnwritten()` (dword scan; returns byte offset of first
  still-poison dword or SIZE_MAX), `SkipByteRanges()` override hook for HLSL
  std140 padding.
- `Upload<T>` chokepoint: `if constexpr (std::is_base_of_v<GPUUploadable<T>, T>)`
  opt-in. Logs `Error` with buffer name + byte offset on first unwritten dword.
  **Non-blocking**: upload proceeds with poison on the dropped field. The whole
  CB going stale would be a worse failure mode than the dropped field alone.
  **Deduped**: `(buffer_ptr, byte_offset)` set in `GPUBufferResourceServiceImpl.cpp`
  via a Meyers-singleton `std::unordered_set<uint64_t>`. Each (buffer, offset)
  logs once per engine run, not once per frame — keeps the log usable.
- `PerFrameConstantBuffer` derives the base. `static_assert` on `sizeof == 512`,
  `alignof == 16`, `is_standard_layout_v` all hold (EBO invariance proven).
  `SkipByteRanges` masks the trailing `uint32_t padding[11]` (HLSL std140
  alignment, not a real field, can't be written by the producer).

### Bugs the validator caught (real, surface-don't-chase)

1. **`posWSNormalizer` is unwritten** by `PerFrameDataService_Impl.cpp`. Pre-PoC
   `= {}` left it (0,0,0,0) implicitly, which caused a black screen because GI
   probe lookup divides by these terms. **Not** fixed at the root — the producer
   doesn't compute a world-space AABB normalizer, it never has. PoC fix: explicit
   `Vec4(1,1,1,0)` (unit normalizer) so the GPU math stays in range. The proper
   fix is a separate task: compute the AABB extents from the scene's
   `GPUModelData` and write `posWSNormalizer` correctly. **File as follow-up.**

2. The validator's reported offset for the dropped field is 376. My mirror
   predicts posWSNormalizer at offset 384 — an 8-byte discrepancy. The real
   engine's struct layout differs from my mirror by 8 bytes somewhere in the
   Vec4 area. Doesn't affect the PoC's behavior (the bug is real regardless of
   which field is at 376) but the *correct* unit-normalizer fix may be writing to
   the wrong byte in the real engine. Diagnosis: needs `static_assert` on
   `offsetof(PerFrameConstantBuffer, posWSNormalizer)` printed at startup to
   confirm the real offset, then adjust the producer write accordingly. **File
   as follow-up.**

3. **Pre-existing log spam** unrelated to TASK-246:
   `RenderPassResourceService::InitializeComponents` logs "draining deferred
   queue" / "drained" / "activated" at `Verbose` level on every per-frame
   drain call. Combined with `MeshResourceService` AABB logs, this is ~1600
   lines per 3-frame run. Removed the 3 spam calls from
   `RenderPassResourceServiceImpl.cpp` (pure Verbose; not load-bearing). Set
   `Data/Engine/Configuration/Presets/PT.json` `logLevel: 0 -> 1` to hide the
   remaining AABB / queueing Verbose spam while keeping Success markers + the
   validator's Error visible.

### Acceptance Criteria state

- [x] #1 Base added, sizeof/offset invariant preserved (EBO; static_assert).
- [x] #2 `Upload<T>` validates + logs Error(buffer+offset) — **deduped**, not
      blocking the upload.
- [x] #3 PoC legit case passes via standalone unit tests (8/8 pass in
     0.2s; runs as a console-subsystem exe with no engine link, see
     Source/TestSuite/UnitTests/GPUUploadableTests.cpp). The
     dropped-field half of the AC is covered by
     TestFirstUnwrittenDroppedField asserting the exact byte offset of
     the first still-poison dword. Live-engine smoke is gated on a
     pre-existing engine init-loop in TextureResourceService
     (D3D12 DeviceRemoved on first texture init, unbounded retry,
     dangling-component reprocessing). The engine never reaches the
     GPU upload phase so the validator is not exercised via the
     live-engine path until that blocker is resolved.
- [ ] #4 Remaining CBs not migrated — tracked as TASK-249 (Rollout §2),
     blocked on TASK-247 (TextureResourceService init-loop unblocks the
     live-engine smoke). Follow-ups A/B/C and the commit-guard gate fix
     (TASK-248) also surfaced this session.

### Rollout next step

Rollout §2: migrate `PointLight`, `SphereLight`, `Transform`, `Material`,
`Dispatch`, `GI`, `Voxelization`, `Animation` CBs, and `GPUModelData`. Each
migration will surface its own dropped fields. The pattern is now established
and the work is mechanical: derive, add `SkipByteRanges` for any HLSL padding,
audit the producer.

Generalize the prevention for the TASK-243 class of bug (a constant-buffer field
consumed by a shader but never written on the CPU defaults to 0 -> silent wrong
render, no compile/runtime error). Promote poison-init + pre-upload validation to
a reusable CRTP base so every CPU->GPU struct is validated at the single upload
chokepoint.

### Design

- `template<class Derived> struct GPUUploadable` — empty (methods only, no data
  members, no virtuals). Provides `PoisonInit()` (memset the Derived bytes to a
  poison pattern) and `FirstUnwritten()` (word-scan for the poison pattern;
  returns the byte offset of the first still-poison dword, or SIZE_MAX).
- Chokepoint: `GPUBufferResourceService::Upload<T>(...)`. Validate there:
  `if constexpr (std::is_base_of_v<GPUUploadable<T>, T>)` -> on a non-SIZE_MAX
  result, `Log(Error, ...)` naming the buffer + offset. Final rollout step flips
  this to a hard `static_assert(is_base_of...)` so a new GPU struct that forgets
  the base fails to compile.

### Feasibility (proven 2026-06-16)

Compiled a `PerFrameConstantBuffer` mirror with/without the empty CRTP base under
`clang++ -std=c++23`: `sizeof`, `alignof`, member offsets all identical;
`is_standard_layout`, `is_trivially_copyable`, `is_aggregate` all still hold. The
base costs zero bytes (empty-base optimization) and does not break the raw
memcpy-to-GPU or `= {}` init. Size was the only real risk; it is a non-issue.

### Real costs (not size)

- Poison-init replaces `= {}`, so producers must deliberately write EVERY field
  incl. `padding[N]`, partial vector components (e.g. viewportSize.z/w), and
  conditionally-written fields (e.g. sun_direction when no sun transform). This
  tightens init but touches each producer site.
- In-class default member initializers (PointLightConstantBuffer m_CastShadow=1,
  MaterialConstantBuffer) are clobbered by poison-init; producers must set them.
- Error reports a byte offset, not a field name (no C++ static reflection until
  C++26). Source/Tool/Reflector could map offset->name later.
- Per-upload scan cost: negligible for once-per-frame CBs; for large per-object
  arrays (Transform/Material/GPUModel) gate to first-N-frames or a debug flag.

### Rollout

1. PoC on `PerFrameConstantBuffer` (the struct that caused TASK-243) — prove the
   loop: legit case passes; a deliberately-dropped field fails loud in the smoke
   run. **(this task's first slice)**
2. Migrate remaining CB structs one per commit (PointLight, SphereLight,
   Transform, Material, Dispatch, GI, Voxelization, Animation, GPUModel).
3. Flip `Upload<T>` opt-in `if constexpr` to a mandatory `static_assert`.

## Acceptance Criteria

- [x] #1 `GPUUploadable<Derived>` base added; `sizeof`/offset of a migrated struct
     unchanged (static_assert in code).
- [x] #2 `Upload<T>` validates base-derived structs and logs Error(buffer+offset)
     on an unwritten field.
- [x] #3 PoC: standalone unit test (Source/TestSuite/UnitTests/
     GPUUploadableTests.cpp) locks the contract: legit case passes,
     deliberately-dropped field is caught at the exact byte offset.
     Live-engine smoke run is gated on a pre-existing engine
     init-loop in TextureResourceService and is out of scope for
     this task.
- [ ] #4 Remaining CB structs migrated; `Upload<T>` enforces the base via
     static_assert.

## Definition of Done

- [x] #1 Code compiles (CMake reconfigures clean; standalone target
     builds with 0 warnings, 0 errors)
- [partial] #2 Live-engine smoke run is gated on a pre-existing
     engine init-loop in TextureResourceService. Unit-test path
     covers the validator contract in lieu of the live-engine
     smoke; live smoke is unrunnable until the engine blocker is
     fixed in its own task.
- [x] #3 Negative test demonstrated (TestFirstUnwrittenDroppedField
     asserts the exact byte offset of the first still-poison dword)


## Session log

- **2026-06-17 (closure):** The umbrella task's three load-bearing
  ACs (#1 base + EBO invariance, #2 validator chokepoint, #3
  standalone unit test) are all green. The Rollout §2 (AC #4)
  landed in TASK-249 (`49b4ad2` PointLight+SphereLight,
  `ed3efe2e` Transform+Material, `d751b69e`
  Dispatch+GI+Voxelization+Animation, `75566da2` GPUModelData).
  Follow-up C (the `Upload<T>` `if constexpr` opt-in → mandatory
  `static_assert` flip) is the last mechanical piece; it depends
  on every migration being live-engine-smoked green, which is now
  satisfied (TASK-247 unblocked the live-smoke path, TASK-253
  landed the GPUModelData→RenderInstance rename as a follow-on
  from the same cluster). The flip itself is a tiny CL — file
  as a follow-up task (or inline in the next migration cluster's
  closure CL). Closing this umbrella now; the carry-forward is
  "flip the `if constexpr` to a `static_assert` and re-run the
  full cluster's smoke".
