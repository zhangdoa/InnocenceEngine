---
id: TASK-234
title: >-
  Adopt test-first discipline for low-level features — unit tests at
  introduction
status: To Do
assignee: []
created_date: '2026-05-17 16:09'
labels:
  - testing
  - discipline
  - process
  - followup
dependencies: []
references:
  - f407f461
  - >-
    .backlog/tasks/task-228 -
    Pre-existing-TAA-upper-half-inversion-artifact-in-GISponza-autotest-—-root-cause-and-fix.md
  - 'C:/Users/zhangdoa/.claude/skills/test-first/SKILL.md'
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## Trigger

TASK-228 surfaced a per-slot channel-source bug in the BC4 compressor path used by glTF Metallic-Roughness import (`f407f461` was the fix). The bug existed at introduction time. It was caught only at visual-validation review — every integration capture had been silently wrong.

The bug shape is precisely what unit tests catch cheaply:
- Pure input → output mapping (per-slot RGBA channel selection → BC4 source byte).
- Deterministic, no GPU dependency.
- Small input space, easy oracles.

Integration tests missed it because the artifact appeared as "roughness looks slightly off" — within the noise band of GI/denoise variation, not visibly diagnostic. A unit test at introduction would have failed immediately on the first wrong-channel mapping.

## Scope

Project bias today is toward integration tests (visible in `task-77.4` DOD: "Self-authored mock-based tests are not the sole validation"). That bias is correct for system-behaviour features (render passes, scene flow, denoise pipelines) — integration is the only honest signal there.

It is wrong for **low-level features**: pure input/output transforms where the bug surface is "did I route bits correctly," not "did the system behave correctly." Those should be unit-tested at introduction, in the same CL.

## "Low-level feature" working definition (refine as part of this task)

- Importers and exporters (glTF, FBX, custom formats — channel routing, byte order, swizzle, encoding selection).
- Codecs and compressors (BC1-7, ASTC, custom — input mapping, block layout).
- Asset-pipeline data transforms (texture-channel packing/unpacking, mesh format conversion, animation-curve resampling).
- Parsers (scene file, material file, config).
- Math primitives (matrix ops, quaternion, color-space conversions).
- Pure host-side routing/dispatch logic with no GPU/IO dependency.

## Infrastructure question (first concrete step)

Does the engine currently have a unit-test runner? Scan: `Source/`, `Build/`, `Scripts/` for GoogleTest / Catch2 / doctest / custom. If none exists, the first sub-deliverable is choosing one and wiring it into the build — that itself is a substantial CL and may want its own task.

## Anchor references

- `f407f461` — BC4 importer fix commit (TASK-228 resolution).
- `Source/Engine/Asset/AssetImporter*.cpp` (or wherever BC4 routing lives) — first retroactive unit-test target.
- User-level skill `test-first` — generic policy.
- `.claude/skills/visual-validation` — what integration validation looks like in this engine.

Pre-existing bias, not a new bug. Surfaced separately per `surface-dont-chase`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 "Low-level feature" category documented as a concrete file/path list (not just adjectives) in a project-discoverable venue (skill or state snapshot)
- [ ] #2 Engine unit-test infrastructure status assessed: existing runner identified, or gap documented with a recommendation for which runner to adopt
- [ ] #3 BC4 importer per-slot channel-source routing gets a unit test covering the TASK-228 regression case (per-slot RGBA → BC4 source byte)
- [ ] #4 Project policy lands: new low-level-feature CL includes unit test in the same CL — either as a project skill extension or in CLAUDE.md/skill `test-first`
- [ ] #5 Existing low-level-feature files surveyed; retroactive unit-test backfill scoped (in-task list, or filed as follow-up tasks per file)
<!-- AC:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
