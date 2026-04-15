---
id: TASK-36
title: >-
  Fix incompatible UAV barrier layout for Radiance Cache Integration Result
  texture
status: To Do
assignee: []
created_date: '2026-04-15 20:18'
labels:
  - bug
  - dx12
  - rendering
  - barriers
dependencies: []
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Bug

D3D12 GPU-based validation (enabled via new `-gpu_validation` launch arg) reports:

```
D3D12 ERROR: GPU-BASED VALIDATION: Dispatch, Incompatible texture barrier layout:
Resource: 'Radiance Cache Integration Result_DefaultHeap_Texture_Frame0'
Binding Type In Descriptor: UAV
Layout: D3D12_BARRIER_LAYOUT_RENDER_TARGET(0x2)
Shader Code: Source/Shaders/HLSL/RadianceCacheIntegration.comp(71,30)
Pipeline State: RadianceCacheIntegrationPass_PSO
Command List Type: D3D12_COMMAND_LIST_TYPE_COMPUTE
```

The Radiance Cache Integration pass binds its output texture as a UAV in a compute dispatch, but the texture's barrier layout is still `RENDER_TARGET` from a prior pass. The required transition to a UAV-compatible layout is missing, so the dispatch reads/writes a resource in the wrong layout. Triggered fast-fail (exit code 0xC0000409) when validation is on; silent corruption when off.

## Reproduction

```
Main.exe -mode 0 -renderer 0 -loglevel 0 -offscreen -gpu_validation -total_frames 15
```

## Investigation plan (task-first, no rogue fix)

1. **Discover** — trace how `Radiance Cache Integration Result` is created and which passes write to it. Identify who last leaves it in RENDER_TARGET layout.
2. **Test** — confirm the missing barrier by logging or inspecting the pass's prologue transitions for this resource.
3. **Narrow** — determine whether the gap is a per-pass barrier bug, a shared helper missing a case, or a broader layout-tracking issue.
4. **Fix** — add the correct transition (`RENDER_TARGET` → `UNORDERED_ACCESS`) at the right synchronization point. Prefer a structural fix over a local patch if the layout tracker has a gap.
5. **Validate** — rerun `Main.exe -offscreen -gpu_validation -total_frames 15`; zero GPU validation errors required. Also rerun the interactive/scene-reload tests.
6. **Commit** — atomic commit following `Documents/commit-message-policy.md`.

## Files likely involved

- `Source/ExampleProject/RenderingClient/RadianceCacheRaytracingPass.cpp`
- `Source/ExampleProject/RenderingClient/RadianceCacheReprojectionPass.cpp`
- Radiance Cache Integration pass setup (search for `RadianceCacheIntegrationPass_PSO` owner)
- `Source/Shaders/HLSL/RadianceCacheIntegration.comp`
- DX12 barrier/layout helpers in `Source/Engine/Services/DX12/`

## Structural angle (per CLAUDE.md)

If barrier layouts are tracked manually per pass, that's the weakness — an explicit layout-tracker that fails loudly on mismatch would have caught this at queue submission instead of requiring GPU validation. Consider whether the fix should include tightening the barrier API to make invalid transitions unrepresentable.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 Running Main.exe with -gpu_validation -offscreen -total_frames 15 exits 0 with zero D3D12 GPU-based validation errors
- [ ] #2 Root cause identified and documented in the commit message
- [ ] #3 Fix is structural where applicable (not a local patch if the layout tracker has a systemic gap)
- [ ] #4 Scene reload test still passes
- [ ] #5 Interactive test (full scenario) still passes
<!-- AC:END -->
