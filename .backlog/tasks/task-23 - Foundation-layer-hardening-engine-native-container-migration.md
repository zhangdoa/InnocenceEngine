---
id: TASK-23
title: Foundation layer hardening + engine-native container migration
status: To Do
assignee: []
created_date: '2026-04-13 11:25'
updated_date: '2026-05-22 09:39'
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
- [ ] #1 Every foundation concern listed in the Description has been broken out as a subtask (task-23.N), filed, and either Done or explicitly closed with a reason.
- [ ] #2 Allocator is used by every engine-internal STL-style container (no std::vector / std::unordered_map / std::queue without it, in engine code).
- [ ] #3 Array is a growable vector-like with reallocation and is the default sequence container in engine code (std::vector usages either replaced or have a documented reason to remain).
- [ ] #4 ThreadSafe* wrappers no longer wrap STL containers; they wrap engine-native Array / Queue / HashMap.
- [ ] #5 No header in Source/Engine/Common/ has 'Data' as a suffix unless the type genuinely is a serialisable blob.
- [ ] #6 Handle vs AssetHandle naming collision resolved — each type has a name that fits its actual semantics, or one is removed in favour of the other.
- [ ] #7 IOService.h public methods use PascalCase; TASK-30 sequenced or merged with this work.
- [ ] #8 GPUDataStructure.h has zero references to dead-GI types (Surfel/Brick/Probe/etc.).
- [ ] #9 Atomic / AtomicObject usage decision applied (removed or adopted; not left as test-only).
- [ ] #10 RingBuffer + DoubleBuffer either ship with hardened lock discipline + tests, or are removed/inlined.
- [ ] #11 FixedSizeString off-by-one resolved (fix + migration OR documented-known-safe + comment block on the class).
- [ ] #12 Memory::Reallocate UB resolved.
- [ ] #13 Every shipped foundation feature has unit + smoke/stress tests in Source/TestSuite/; perf-vs-STL comparison where applicable.
- [ ] #14 RenderTest.exe and Main.exe -total_frames 10 both exit 0 after full migration.
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Session 2026-05-22 progress

**Closed (12 subtasks):**
- 23.6 DoubleBuffer audit — real race fixed, replaced atomic protocol with shared_mutex; SPMC test caught + verifies.
- 23.7 RingBuffer audit — size()/currentElementPos() now lock; ThreadSafe operator[] / currentElement() return by value; concurrent test added.
- 23.8 Atomic removed (zero production callers).
- 23.9 AtomicObject deleted (cascade from 23.10).
- 23.10 Handle → std::shared_ptr (decision D; resolves Handle vs AssetHandle naming collision).
- 23.11 AssetData renamed to MeshAsset/TextureAsset/MaterialAsset; file AssetData.h → AssetTypes.h.
- 23.12 AssetImportData.h deleted (zero call-site usage of the typedef).
- 23.13 IOService 19 methods → PascalCase via sed.
- 23.14 GPUDataStructure stale GI types removed (Surfel/Brick/Probe/etc.).
- 23.15 FixedSizeString — actual fix already historically landed (fef48be5); this CL fixed a stale test input typo from c22b3b647.
- 23.16 Memory::Reallocate UB resolved (malloc/realloc/free throughout).
- 23.17 ObjectPool audit — alignment static_assert; contract comment; slot-reuse test.

**Partial (1 subtask):**
- 23.1 Allocator hardening (overflow guard + alignment doc + unit tests done). Engine-wide STL-plumb sweep (AC #3) and perf-vs-STL (AC #6) deferred.

**Open / multi-session (5 subtasks):**
- 23.2 Array growable + std::vector replacement engine-wide.
- 23.3 Engine-native Queue<T> (new container).
- 23.4 Engine-native HashMap<K,V> (new container).
- 23.5 ThreadSafe* containers migrate to wrap engine-native Array/Queue/HashMap (depends on 23.2/3/4).
- 23.18 TestSuite cross-cutting coverage matrix (depends on container arc).

## Sequencing for next session

The remaining 5 form one arc: 23.1 plumb → 23.2 (Array growable) → 23.3 + 23.4 (Queue, HashMap) → 23.5 (ThreadSafe migrate) → 23.18 (coverage roll-up).

Start with finishing 23.1's plumb sweep (mechanical) OR jump to 23.2 (substantial new code). 23.2's Array rewrite has the highest impact — every std::vector usage downstream depends on it.

## Bugs fixed in passing (not in any subtask but worth noting)

- DoubleBuffer atomic-protocol race (snapA != snapB) — found and fixed in 23.6.
- RingBuffer ThreadSafe variant size()/[]/currentElement() races — found and fixed in 23.7.
- AssetService_h had a stale `#include "AssetData.h"` after the rename (auto-handled in 23.11 sweep).
- PhysicsSimulationService.cpp had a stale `#include "DoubleBuffer.h"` (removed in 23.6).
- FixedSizeStringTests `Component` typo (lost slash from c22b3b647, fixed in 23.15).
<!-- SECTION:NOTES:END -->
