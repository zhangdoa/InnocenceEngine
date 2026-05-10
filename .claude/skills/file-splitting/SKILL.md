---
name: file-splitting
description: Use when the file-size gate blocks a commit, when a touched file is over the 300-line ratchet, or before adding code that would push a file past the limit. Project-specific extensions to user-level `file-splitting`: 300-line gate, CMake regen step, FILE_SIZE_EXCLUDE_RE policy.
---

# Skill: file-splitting (project extension)

Generic split shapes, naming conventions, and anti-patterns live in user-level `file-splitting`. This skill carries the project-specific gate threshold + build-system bindings + path policy.

## The 300-line gate

`gates/file-size.js` blocks commits that grow a file past 300 lines. No commit-message escape, no string sentinel.

Files already over the limit are grandfathered but still subject to the gate **on growth**. When editing such a file, the turn summary notes whether the split lands in this CL or as a follow-up refactor task.

## Engine-specific naming

C++ TUs: `Foo.cpp` (core) + `Foo_FeatureBar.cpp` for partial-class splits. CMake's `file(GLOB ... *.cpp)` picks them up. Header stays single; only `.cpp` splits.

Free-function or template-heavy headers (no class to anchor): split by domain into `Foo_Domain.h`; the original `Foo.h` stays as an umbrella that includes the parts.

Engine convention for the suffix: name the responsibility (`Mesh_Allocation.cpp`, `Mesh_AsyncBinaryLoad.cpp`). Avoid `FooImpl.cpp`, `Foo2.cpp`. The suffix typically follows an existing `// ---` domain divider in the original file.

## CMake regen step

After creating new files, run `cmake .` from `Build/` to refresh `.vcxproj`. CMake's `file(GLOB)` source enumeration does not auto-pick up new files until configure re-runs.

## Path exemptions

Path-policy exemptions live in `FILE_SIZE_EXCLUDE_RE` in `.claude/hooks/lib/common.js`. The exclusion is for `ThirdParty/` / `External/` / `Generated/` only. Adding to the exemption list is its own diff with its own review surface — not a per-commit escape.

## Engine-specific anti-patterns

- **`friend` to keep cross-class access** after composition extraction. If the new class needs the original's internals, the seam was wrong — re-pick.
- **Smuggling style touches** (pointer-placement `T *x` → `T* x`, formatting normalization, dead-include trimming beyond what the split structurally requires). Either hold them for a separate CL, or call them out explicitly in the commit body / Implementation Notes — a mechanical-split claim is structurally falsified by uncalled-out edits.

## Cross-references

- User-level `file-splitting` — generic split decision, naming across stacks, procedure, anti-patterns.
- `cpp-style` — naming and include conventions for new TUs.
- `comment-discipline` — files often grow because of comment bloat. Strip prose first.
- `.claude/hooks/gates/file-size.js` — the gate this discipline serves.
