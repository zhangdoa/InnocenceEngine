---
id: TASK-180
title: 'LightPass TLAS-not-ready early-frame guard (TASK-176 ADVISORY)'
status: To Do
assignee: []
created_date: '2026-04-28 14:50'
labels:
  - rendering
  - dx12
  - bug
dependencies:
  - TASK-176
priority: low
references:
  - Source/ExampleProject/RenderingClient/LightPass.cpp
  - Source/Engine/Services/DX12/DX12FrameManagementService.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**TASK-176 review ADVISORY (graphics-api-expert peer, 2026-04-28).**

`LightPass.cpp:370` unconditionally binds `GetTLASBuffer()` and `:379` uses `Dispatch` (no `IsTLASReady()` guard like `DispatchRays` at `DX12FrameManagementService.cpp:401`). The engine convention at `LightPass.cpp:363-364` (nullptr-bind when SunShadowRT not Activated) is not followed for the new t14 binding.

If TLAS isn't built yet (early frames before async build completes), the inline RayQuery reads an uninitialized acceleration structure → undefined behaviour. Engine reaches `Dispatch` before SunShadowRT's `IsTLASReady()` short-circuit fires; the equivalent guard for LightPass needs to either nullptr-bind or skip the dispatch.

### Required fix

Mirror the SunShadowRT pattern: either (a) check `IsTLASReady()` before binding the TLAS root SRV at LightPass dispatch site and nullptr-bind when not ready (visibility falls back to "always lit" via the inline RayQuery returning `COMMITTED_NOTHING` on null AS — verify behaviour), OR (b) gate the entire LightPass dispatch on TLAS ready (regress to a non-RT lighting eval path until ready).

Option (a) is closer to the existing convention. Option (b) is heavier but unambiguous.

### Why low priority

Early-frame UB is bounded — first few frames may have visual artifacts, but engine recovers as soon as TLAS builds. No GBV ERROR observed during TASK-176 testing (probably because TLAS is ready by frame 5+ when smoke runs report). Real risk is on extremely cold-cache scenarios (just-cloned engine with no warm caches).

### Owner

`rendering-researcher` (LightPass.cpp pass binding) or `graphics-api-expert` (if the fix lives in `DX12FrameManagementService` IsTLASReady check). One-line fix likely.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 LightPass TLAS binding gated on IsTLASReady (or equivalent fallback)
- [ ] #2 Cold-cache smoke (`Main.exe -total_frames 5`) clean — no ERROR/WARNING from TLAS access
- [ ] #3 Existing warm smoke unchanged (no regression)
<!-- AC:END -->
