---
id: TASK-251
title: >-
  Black triangles in UnitTest scene (many camera angles, not all) — actual root cause TBD
status: To Do
assignee:
  - code-impl
created_date: '2026-06-17'
labels:
  - rendering
  - bug
  - gpu-data
  - regression
  - follow-up
dependencies:
  - TASK-249
priority: medium
---

## Description

Re-scoped from the original std140-mismatch claim, which was wrong.
The user observed: "the unit test scene's default camera angle, and
many angles would produce broken black triangles over the screen, but
not some other angles, must be a cbuffer issue made by your recent
CLs". The original framing assumed the issue was a C++/HLSL layout
mismatch in `GPUModelData::m_BoundingBoxMin/Max`. After the
2026-06-17c learning session, that framing was falsified — see
"Original claim: falsified" below.

The user's empirical report remains. The actual root cause has not
been identified. This task is the open investigation.

### Original claim: falsified

The original TASK-251 body claimed:
- C++ `GPUModelData::m_BoundingBoxMin` is at C++ offset 56 (Vec4,
  4B aligned).
- HLSL `GPUModelData::m_BoundingBoxMin` is at HLSL offset 64 (float4,
  16B aligned under cbuffer rules).
- The 8-byte delta is the bug.

**This is wrong.** Verified against
[Microsoft/DirectXShaderCompiler Buffer Packing wiki](https://github.com/microsoft/DirectXShaderCompiler/wiki/Buffer-Packing):

> "Structured Buffers follow the rules under **Basic Type Alignment
> and Size** without the additional padding or alignment constraints
> for legacy constant buffers."

> "Vector and Matrix types are aligned by their component type
> alignment."

`GPUModelData` on the HLSL side is used as
`StructuredBuffer<GPUModelData> u_ModelDataBuffer` (in
`Source/Shaders/HLSL/opaqueGPUCulling.comp:13` and
`opaqueGeometryProcessPass.frag:23`), NOT as a cbuffer member. The
float4 is 4B aligned in StructuredBuffer (not 16B). So the HLSL
`m_BoundingBoxMin` is at offset 56, same as C++. **The layout
matches.**

Same conclusion for the other 8 migrated CBs — all used as
`StructuredBuffer<>` on the HLSL side. C++ and HLSL layouts match
because both use basic type alignment.

### What is verified about my 4 code CLs

The 4 code CLs (TASK-249, `49b4ad2` / `ed3efe2e` / `d751b69e` /
`75566da2`) are byte-equivalent to the pre-CL state:

- The CRTP base `GPUUploadable<T>` is empty (EBO verified by
  `TestEBOInvariance` in `GPUUploadableTests_Standalone.exe`); the
  `static_assert` on `sizeof` confirms each migrated CB is unchanged.
- The producer-side changes (PoisonInit + explicit field writes) write
  the same values as the prior in-class defaults (e.g. `m_CastShadow
  = 1` becomes `l_data.m_CastShadow = l_Light.m_CastShadow ? 1u : 0u;`
  — same end value when the producer always wrote this field).
- The `m_ShaderProgramIndex = 0` and `m_RenderPassIndex = 0` writes
  match the in-class `= 0` defaults.
- The on-GPU byte stream is identical pre-CL-1 and post-CL-4.

Therefore, **my CLs are NOT the cause of the black triangles.** The
user's attribution of the cause to my CLs is likely incorrect.

### Hypotheses to investigate (ranked)

**H1 (most likely):** A pre-existing rendering bug, unrelated to the
GPUUploadable migration. Possible candidates:
- `m_VisibilityMask` is written by the producer as
  `VisibilityMask::MainCamera` (line 177 of `DrawCallService.cpp`).
  The HLSL cbuffer struct defines `m_VisibilityMask` but the GPU
  culling pass does not use it. So the GPU's draw command's
  `InstanceCount` depends only on `IsInFrustum`. If a mesh's
  world-space AABB is computed wrong (e.g. when `l_world` is null
  and the local AABB is used as a fallback), culling could mark
  visible meshes as culled for some camera angles.
- The `m_UUID = static_cast<float>(l_Entity)` field (line 175 of
  `DrawCallService.cpp`) is float-cast from an EntityID. For large
  entity IDs this can produce NaN or inf. If the shader uses
  `m_UUID` for sorting, NaN UUIDs could misbehave. (Pre-CL behavior:
  same — so not introduced by my CLs.)

**H2:** The 0-byte log issue (engine dies before writing any log)
masks the actual rendering path. The user may be seeing the
**default-fallback** (engine in unconfigured state), not the actual
configured scene. The fallback may not be the same scene they
think they're testing.

**H3:** A binary-cache issue (DXIL, PDB, .pdb) — the user may be
running a stale binary that doesn't include my CLs but they're
attributing the symptom to them based on the recent commits. The
DXC shader cache (`Bin/RelWithDebInfo/Shaders/DXIL`) is
`wipe + copy` per build, but a build failure could leave stale
DXIL on disk.

**H4:** A specific scene asset (material, mesh, or texture) in
UnitTest has a defect that only manifests at certain camera angles.
The UnitTest scene has 700+ entities. A single misconfigured
material or wrong-winding mesh could produce the symptom.

**H5 (least likely):** The culling pass (`opaqueGPUCulling.comp`
`IsInFrustum`) is genuinely using wrong values. But the C++ and
HLSL layouts match (verified), so this is unlikely unless there's
a different bug I haven't identified (e.g. a producer-side error
in computing the AABB, or a camera-matrix issue in the culling
shader).

### Acceptance Criteria

- [ ] **Capture a failing state**: a PNG from
      `Build/captures/reg_unittest_*.png` (or similar) showing the
      black-triangle symptom. The user provides this, OR a
      headless run with the regression capture preset produces it.
- [ ] **Compare to baseline**: the same camera angle / frame index
      from the pre-CL state (TASK-77.4 CL 3 / CL 4 captures in
      `Build/captures/TASK-77.4-CL-*-UnitTest*.png`).
- [ ] **Identify the actual CB or engine primitive that produces
      the black triangles.** Use the structured-debug methodology
      (per `.omp/rules/no-speculative-debug-loop.md`): enumerate
      hypotheses, rank by likelihood, pick ONE diagnostic per
      round, run, observe, then pick the next.
- [ ] **File a follow-up task** for the actual root cause (or
      surface-don't-chase if the cause is outside the task's
      scope, per `.omp/rules/surface-dont-chase.md`).
- [ ] **Close this task as Done** with a pointer to the actual
      root-cause task.

### Investigation hints (do this in order)

1. Run the engine with the regression capture preset (5-frame
   `dumpFramesStart=5, dumpFramesEnd=12`, UnitTest scene, camera
   orbit). Save the per-frame PNGs to
   `Build/captures/regression_2026-06-17/`.
2. If a capture shows black triangles, diff it against the
   pre-CL reference (`Build/captures/TASK-77.4-CL-4-UnitTest-static.png`).
3. If the captures match the pre-CL reference (no black triangles),
   the user's report was transient or from a different state.
   Close this task with the negative result.
4. If the captures show black triangles, the cause is in the
   current code. Bisect: revert CL 1, retest. Revert CL 2, retest.
   Revert CL 4, retest. Identify which CL (if any) is responsible.
5. If no CL is responsible (i.e. the symptom pre-exists), the cause
   is in pre-CL code. File a new task with the bisect result.

### Out of scope

- TASK-250 (`m_ShaderProgramIndex` / `m_RenderPassIndex` not
  populated): real but lower-priority; the GPU consumes 0 in both
  pre-CL and post-CL states (in-class default `= 0` vs explicit
  `= 0`), so no behavior change from my CLs. Filed as separate
  task.
- Reverting my 4 code CLs: NOT appropriate. The CLs are
  byte-equivalent, the engine is in a known-good state, and
  reverting wastes the diagnostic trail. The `no-revert-on-engine-gap`
  rule applies.

### Related

- TASK-249 (the GPUUploadable Rollout §2; the 4 code CLs).
- TASK-243 (GBuffer-black regression; another CB-related silent-wrong
  bug for reference).
- TASK-77.4 (the most recent pre-CL captures of the UnitTest scene;
  baseline for comparison).
- Session 2026-06-17c resume note (in basic-memory `InnocenceEngine/
  innocence-engine/render-graph/`): the HLSL/C++ padding learning
  session that falsified the original std140-mismatch claim.

## Session log

- **2026-06-17 (filed):** original claim was a C++/HLSL std140
  mismatch in `GPUModelData::m_BoundingBoxMin/Max`. Filed as
  TASK-251 by TASK-249 CL 4.
- **2026-06-17c (this entry):** claim falsified. The HLSL
  `GPUModelData` is used as `StructuredBuffer<>` (basic type
  alignment), not as a cbuffer member (16-byte row-alignment).
  Per the Microsoft/DirectXShaderCompiler Buffer Packing wiki,
  `StructuredBuffer<T>` follows basic type alignment, so `float4`
  is 4B aligned. C++ and HLSL layouts match at 160 bytes with
  `m_BoundingBoxMin/Max` at offset 56/72 in both. The user's
  report of black triangles remains unverified. Task re-scoped
  to "find the actual root cause" instead of "fix the std140
  mismatch".
- **2026-06-23 (findings; still open):** On a fresh `BuildWin.ps1`
  build of the current thread (post TASK-252/253 data-model split),
  the headless **Audit preset** (UnitTest scene, fixed camera, 35
  frames) renders cleanly: all 17 render-graph pass HDRs dump
  non-black (`Bin/audit_*.hdr`, sizes 280–666 KB; a collapsed/black
  frame would RLE-compress to a few KB), exit 0, no D3D12 errors,
  validators silent. The **OpaquePass GBuffer (BaseColor/Normal/ORM/
  Emissive)** is non-black — so geometry is NOT collapsed at the audit
  camera angle. TASK-252's degenerate-transform fix (missing
  WorldTransform now substitutes identity instead of a zero matrix
  that collapsed geometry) is a plausible contributor and is now in
  the build.
  WHAT THIS DOES NOT SETTLE: the user's symptom was **interactive and
  camera-angle-dependent** ("many angles produce black triangles, not
  some others"). The offscreen Audit uses ONE fixed camera, so it
  cannot exercise the angle dependence. There is no camera-orbit
  headless capture preset to reproduce it. **Needs interactive
  confirmation on the current build**: does the symptom still
  reproduce at the default + other camera angles? If gone, close
  (likely fixed by the TASK-252 identity-transform fix). If present,
  the next diagnostic is a multi-angle capture (extend a preset with
  a per-frame camera orbit) + RenderDoc on a failing frame.
