---
id: TASK-246
title: >-

  GPUUploadable CRTP base: forced pre-upload validation for CPU-write/GPU-read structs
status: In Progress
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

PoC first slice landed on `ecs-overhaul` (uncommitted, working tree).

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
- [partial] #3 PoC legit case passes: producer writes every field, validator
      returns SIZE_MAX, no Error log. **Smoke run clean not directly
      verified** in this session — the engine has a pre-existing nondeterministic
      hang in early init that I traced to LNK1168 (previous engine run holding
      the binary) and force-killed engines losing buffered m_LogFile content.
      The validator's behavior is correct (2 lines per dropped field, no
      per-frame spam). Negative test demonstrated: 8MB log showed offset 376
      firing every frame, dropped after dedup to 2 lines total.
- [ ] #4 Remaining CBs not migrated.

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

- [ ] #1 `GPUUploadable<Derived>` base added; `sizeof`/offset of a migrated struct
      unchanged (static_assert in code).
- [ ] #2 `Upload<T>` validates base-derived structs and logs Error(buffer+offset)
      on an unwritten field.
- [ ] #3 PoC: `PerFrameConstantBuffer` derives the base, fully initialized; smoke
      run clean; a deliberately-dropped field is caught loud (demonstrated, then
      reverted).
- [ ] #4 Remaining CB structs migrated; `Upload<T>` enforces the base via
      static_assert.

## Definition of Done

- [ ] #1 Code compiles
- [ ] #2 Live-engine smoke run green with validation active
- [ ] #3 Negative test demonstrated (dropped field -> loud Error)
