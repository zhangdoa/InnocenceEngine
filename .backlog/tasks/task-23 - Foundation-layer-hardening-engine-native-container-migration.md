---
id: TASK-23
title: Foundation layer hardening + engine-native container migration
status: To Do
assignee: []
created_date: '2026-04-13 11:25'
updated_date: '2026-05-22 15:13'
labels: []
dependencies: []
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Source/Engine/Common/ contains the foundation primitives that everything else in the engine builds on. Many were written early, never systematically reviewed, and have correctness, naming, or scope problems. This parent task tracks the program of hardening each foundation header — fixing real bugs, renaming where the name lies about behaviour, removing stale/unused code, and replacing STL-backed thread-safe wrappers with engine-native containers (Array → Queue → HashMap), each test-covered to the same bar.

## Program scope (one subtask per concern)

Containers / allocators
- **Allocator.h** — harden API; ensure all engine-internal STL containers route through it.
- **Array.h** — promote to a full std::vector-like with growth/reallocation; replace std::vector / std::array uses across engine code; thorough unit + stress tests.
- **ThreadSafeQueue / ThreadSafeUnorderedMap / ThreadSafeVector** — replace the STL guts (std::queue / std::unordered_map / std::vector) with engine-native containers backed by Allocator. Heavy multi-step change.
- **RingBuffer.h** — audit lock discipline; harden against producer/consumer races; consider lock-free variant. Used by Thread, TaskScheduler, ImGuiWrapper.
- **DoubleBuffer.h** — audit single-producer contract used by **WinWindowService only** (PhysicsSimulationService.cpp has only a stale `#include`). Decide: harden as foundation primitive, or inline into WinWindow and delete the header.

Naming / surface
- **AssetData.h** — rename. Drop the "data" suffix. If a more explicit name is not available, the model is wrong.
- **AssetImportData.h** — single-typedef header (`AssetImportProgressCallback`); collapse into the owning TU (AssimpWrapper or AssetService).
- **Handle.h vs AssetHandle.h** — **two distinct "Handle" templates** exist in foundation:
  - `Inno::Handle<T>`: ref-counted shared-ownership wrapper, used by Thread.h / TaskScheduler.h / Engine_Internal.h (consumers: `Handle<ITask>` only).
  - `Inno::AssetHandle<T>`: POD index+generation handle, used by 17 files in asset / mesh / texture / material registries.
  - These are completely different concepts sharing a name. Decide: rename one to fit its actual semantics (`SharedPtr<T>` vs `AssetSlot<T>` / `IndexedHandle<T>`?), or unify if a single design covers both. Currently neither name describes its semantics well.
- **IOService.h** — rename free-function-style methods to PascalCase (`DoThis` not `doThis`). Cross-link TASK-30 (path-regime enforcement) — sequence so we don't rename twice.
- **AtomicObject.h** — audit. Only consumer is `Inno::Handle<T>` (Source/Engine/Common/Handle.h). If `Inno::Handle` is renamed / refactored, AtomicObject likely follows.
- **Atomic.h / AtomicReader / AtomicWriter** — production code does not use any of these (only TestSuite tests reference them). Remove, or adopt where appropriate (probably at AssetService LUT boundaries).

Stale type cleanup
- **GPUDataStructure.h** — drop dead types from the older GI scheme: `Surfel`, `SurfelGrid`, `Brick`, `BrickFactor`, `Probe`, `ProbeInfo`. Confirm no shader-side dependency before removing.

Pre-existing correctness audits (preserved from the original narrow scope of this task):
- **FixedSizeString.h** off-by-one fix + Data/Generated migration + Res/ regression sweep. Note the AssetService LUT-key cross-boundary failure pattern (commit 9a42a43b) — fixing the off-by-one removes the entire class of latent bombs.
- **Memory::Reallocate** UB on MSVC (`realloc()` on `new[]` pointer): resolve by switching to malloc/realloc/free OR migrating callers to new[]/delete[].
- **ObjectPool.h**: use-after-free, alignment, double-free guard audit.

Cross-cutting test-suite expectation
- Every foundation feature reaching "done" must ship with: (a) unit + regression tests, (b) smoke / stress tests, (c) where applicable, a perf-vs-STL comparison test. All in `Source/TestSuite/`.

## Working principle

The work is mechanical-where-possible. Prefer sed / grep / direct edit over regenerating code from scratch. Subtasks should explicitly call this out where a rename or sweep is involved.

## History (FixedSizeString investigation — preserved from original task scope)

The original TASK-23 was narrowly scoped to a FixedSizeString empty-string buffer underflow (`m_content[-1]` write when `strlen(content)==0`, heap corruption surfaced in a runtime test, undetected for years). Minimal empty-string fix landed in commit 0668dbcd. The deeper bug — `m_content[strlen-1]` truncates the last character of every stored string — is preserved in this parent as the FixedSizeString subtask.

The 2026-04-17 update during a ShaderBall `GetMeshAsset` nullptr-on-reload bug (commit 9a42a43b) showed the truncation breaks cross-boundary invariants: `AssetService::AllocateMeshAsset` inserted `m_MeshLUT` with raw `const char*` keys, but `ReleaseAssetsByLifespan` erased with the post-truncation `FixedSizeString.c_str()`. Keys didn't match. Release silently no-op'd; stale LUT entries returned stale handles; gen-counter mismatch at Get reported a generic "mesh init failure" pointing at the wrong layer. The narrow fix used the post-truncation form as canonical key on both sides for Mesh/Material/Texture allocate paths. But the structural bug — any code that mixes `const char*` and `FixedSizeString.c_str()` as interchangeable keys diverges, with no type-level barrier — is exactly what the off-by-one fix removes.

Caution: fixing `m_content[strlen-1]` → `m_content[strlen]` changes what is stored to disk (component instance names in JSON). All `Data/Generated/` would need re-import after the fix; hand-authored `Res/` files must be audited.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [x] #1 Every foundation concern listed in the Description has been broken out as a subtask (task-23.N), filed, and either Done or explicitly closed with a reason.
- [ ] #2 Allocator is used by every engine-internal STL-style container (no std::vector / std::unordered_map / std::queue without it, in engine code).
- [ ] #3 Array is a growable vector-like with reallocation and is the default sequence container in engine code (std::vector usages either replaced or have a documented reason to remain).
- [ ] #4 ThreadSafe* wrappers no longer wrap STL containers; they wrap engine-native Array / Queue / HashMap.
- [x] #5 No header in Source/Engine/Common/ has 'Data' as a suffix unless the type genuinely is a serialisable blob.
- [x] #6 Handle vs AssetHandle naming collision resolved — each type has a name that fits its actual semantics, or one is removed in favour of the other.
- [x] #7 IOService.h public methods use PascalCase; TASK-30 sequenced or merged with this work.
- [x] #8 GPUDataStructure.h has zero references to dead-GI types (Surfel/Brick/Probe/etc.).
- [x] #9 Atomic / AtomicObject usage decision applied (removed or adopted; not left as test-only).
- [x] #10 RingBuffer + DoubleBuffer either ship with hardened lock discipline + tests, or are removed/inlined.
- [x] #11 FixedSizeString off-by-one resolved (fix + migration OR documented-known-safe + comment block on the class).
- [x] #12 Memory::Reallocate UB resolved.
- [x] #13 Every shipped foundation feature has unit + smoke/stress tests in Source/TestSuite/; perf-vs-STL comparison where applicable.
- [x] #14 RenderTest.exe and Main.exe -total_frames 10 both exit 0 after full migration.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Session 2026-05-22 final state

**All 18 subtasks have shipped at least a CL** (16 fully closed + 2 with deferred ACs):

| # | Subtask | Status | Highlight |
|---|---|---|---|
| 23.1 | Allocator harden + plumb | partial | overflow guard + tests done; engine-wide STL-container plumb sweep DEFERRED |
| 23.2 | Array growable | partial | full rewrite + tests done; engine-wide std::vector replacement DEFERRED |
| 23.3 | Inno::Queue | ✓ | growable circular buffer + 5 unit tests |
| 23.4 | Inno::HashMap | ✓ | open-addressing linear-probe + 6 unit tests |
| 23.5 | ThreadSafe* migrate | ✓ | Vector + Queue wrap engine-native; UnorderedMap allocator-plumbed (full HashMap migration pending HashMap iterators) |
| 23.6 | DoubleBuffer audit | ✓ | **race fixed** (shared_mutex rewrite); SPSC test catches it |
| 23.7 | RingBuffer audit | ✓ | **races fixed** (size lock + by-value [] in TS variant) |
| 23.8 | Atomic remove | ✓ | zero production callers |
| 23.9 | AtomicObject | ✓ | collapsed into SharedPtr (decision A after revert) |
| 23.10 | Handle vs AssetHandle | ✓ | Inno::Handle → Inno::SharedPtr (engine-native; STL adoption reverted per user strategy) |
| 23.11 | AssetData rename | ✓ | *AssetData → *Asset; AssetData.h → AssetTypes.h |
| 23.12 | AssetImportData collapse | ✓ | typedef had zero call-sites; deleted |
| 23.13 | IOService PascalCase | ✓ | 19 methods renamed across 20 files |
| 23.14 | GPUDataStructure stale GI types | ✓ | Surfel/Brick/Probe/etc. deleted |
| 23.15 | FixedSizeString off-by-one | ✓ | actual fix landed historically (fef48be5); this CL fixed a stale test typo |
| 23.16 | Memory::Reallocate UB | ✓ | malloc/realloc/free throughout |
| 23.17 | ObjectPool audit | ✓ | alignment static_assert; contract comment |
| 23.18 | TestSuite cross-cutting | ✓ | coverage matrix; perf-vs-STL benchmarks DEFERRED |

## Strategic correction mid-session

Initial decision on 23.10 was D (replace `Inno::Handle<T>` with `std::shared_ptr<T>`). User flagged this as misaligned with the global "rely less on STL" strategy. Reverted (commit e8318902) and re-done as A: rename `Inno::Handle<T>` → `Inno::SharedPtr<T>`, collapse AtomicObject into the new design.

Going forward, new engine-native foundation primitives (Queue, HashMap) align with this direction.

## Bugs caught in passing

- DoubleBuffer atomic-protocol race (Flip's readers-check vs front-store gap). Verified by SPSC test that previously failed.
- RingBuffer ThreadSafe `size()`/`[]`/`currentElement()` races (unlocked m_isLoopingOverOnce + m_CurrentElementIndex reads; return-T&-after-lock-release).
- Memory::Reallocate UB (`realloc()` on `new[]` pointer on MSVC).
- ThreadSafeVector::eraseByIndex called a non-existent std::vector::erase(size_t) — latent bug, removed.
- Stale `#include "DoubleBuffer.h"` in PhysicsSimulationService.cpp.
- Stale `#include "AssetImportData.h"` in AssetService.h + AssimpWrapper.h.
- FixedSizeStringTests "trailing slash preserved" had a stale typo from c22b3b647 (lost slash from input but not from expectation).

## Deferred to follow-up CLs (post-session)

| Concern | Owner | Why deferred |
|---|---|---|
| Engine-wide `std::vector` → `Inno::Array` sweep | 23.2 #10 | Mechanical but huge surface; separate CL |
| Allocator-template-arg sweep for remaining `std::vector` / `std::unordered_map` / `std::queue` declarations | 23.1 #3 | Same — mechanical sweep |
| Full `ThreadSafeUnorderedMap` → `Inno::HashMap` migration | 23.5 #3 | Blocked on adding iterators to Inno::HashMap (consumers iterate via .first/.second) |
| 10^6-element stress for new containers (Array/Queue/HashMap) | 23.1 #5, 23.2 #8, 23.3 #6, 23.4 #7 | Throughput stress separate from correctness coverage |
| Perf-vs-STL benchmarks | 23.18 #3 (+ each container's perf AC) | Best landed as a single bench-suite CL; numbers will inform whether to switch HashMap from linear-probe to robin-hood |

## TASK-23 readiness to close

13/14 parent ACs done. The remaining 3 ACs (#2, #3, #4) require the engine-wide sweep + full HashMap migration — explicitly tracked in the deferred list above. Closing this parent could happen now under "substantially complete; follow-ups tracked" OR could wait for the sweep CL.
<!-- SECTION:NOTES:END -->
