---
id: TASK-244
title: >-
  AST prefix-sweep: every bool-returning render-pipeline function uses
  __FUNCTION__-only logs
status: Done
assignee:
  - code-impl
created_date: '2026-06-15'
labels:
  - rendering
  - dx12
  - logging
  - cleanup
  - rules
dependencies:
  - TASK-241
  - TASK-242
references:
  - .omp/rules/prefer-ast-rewrites.md
  - .omp/rules/prefer-ast-rewrites.md
---

## Description

Cross-cutting sweep to make the render-pipeline easier to debug. The harness's
Log macro already prepends `__FUNCTION__` as the first arg slot — most call
sites were duplicating it as `"DX12FrameManagementService::BeginFrame: ..."`
in the body, which was redundant noise. A new rule
`.omp/rules/prefer-ast-rewrites.md` was also added in this session, encoding
the user's preference for AST rewrites over plain-text edit calls for
non-trivial source changes.

### What landed (one CL, pending commit)

51 disjoint AST rewrites across 12 source files. Every rewrite:
- Strips a redundant `ClassName::FunctionName:` or `ClassName ` prefix from a
  log body — the macro already provides that context.
- Touches one log statement at a time, with disjoint AST regions to avoid
  patch-the-patch damage.

### Files swept

- `Source/Engine/Services/Common/MaterialResourceServiceImpl.cpp` (5)
- `Source/Engine/Services/Common/MeshResourceServiceImpl.cpp` (9)
- `Source/Engine/Services/Common/TextureResourceServiceImpl.cpp` (9)
- `Source/Engine/Services/Common/GPUBufferResourceServiceImpl.cpp` (6)
- `Source/Engine/Services/Common/ShaderProgramResourceServiceImpl.cpp` (2)
- `Source/Engine/Services/Common/SamplerResourceServiceImpl.cpp` (2)
- `Source/Engine/Services/Common/CommandListResourceServiceImpl.cpp` (2)
- `Source/Engine/Services/Common/GraphicsHardwareService.cpp` (3)
- `Source/Engine/Services/DX12/DX12FrameManagementService_Bind.cpp` (2)
- `Source/Engine/Services/DX12/DX12FrameManagementService_Draw.cpp` (4)
- `Source/Engine/Services/DX12/DX12FrameManagementService_Recording.cpp` (1)
- `Source/Engine/Services/DX12/DX12FrameManagementService_SwapChainImages.cpp` (6)

Plus the early-out + per-branch log pattern applied earlier in this same
session (TASK-241, TASK-242) to:
- `Source/Engine/Services/Common/RenderPassResourceServiceImpl.cpp`
- `Source/Engine/Services/DX12/DX12RenderPassResourceService.cpp`
- `Source/Engine/Services/DX12/DX12FrameManagementService_Frame.cpp`
- `Source/Engine/Services/Common/FrameManagementServiceImpl.cpp` + `*_Commands.cpp` + `*_FrameQueries.cpp` + `*_Resize.cpp`
- `Source/Engine/Services/DX12/DX12FrameManagementService.cpp`

### Files where the sweep was a no-op (already clean)

DX12 resource services (`DX12MaterialResourceService.cpp`,
`DX12MeshResourceService.cpp`, `DX12GPUBufferResourceService*.cpp`,
`DX12TextureResourceService_*.cpp`, `DX12SamplerResourceService.cpp`,
`DX12ShaderProgramResourceService.cpp`, `DX12CommandListResourceService.cpp`,
`DX12FrameManagementService_RenderTargets.cpp`) already use the correct
`entity-name, message` pattern (no function-name prefix to strip).

### New rule

`.omp/rules/prefer-ast-rewrites.md` — encodes the user's preference for
`ast_edit` / `ast_grep` over plain-text `edit` calls for non-trivial source
changes, with rationale (region disjointness, no patch-the-patch) and
pitfalls (whole-node matching, no prefix-suffix).

## Acceptance Criteria

- [x] #1 All reachable bool-returning functions in the rendering pipeline
      use `__FUNCTION__`-only logs (no `ClassName::FunctionName:` prefix in
      the body).
- [x] #2 Disjoint-pattern AST rewrites (every match's region is disjoint)
      so patch-the-patch damage is impossible.
- [x] #3 Build green; TestSuite 115/116 unchanged.
- [x] #4 New rule registered in AGENTS.md harness layout.
