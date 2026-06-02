---
name: file-splitting
description: Use when the commit-guard file-size gate blocks a commit, or before adding code that would push a file past 300 lines.
---

# Skill: file-splitting

## Gate

commit-guard's file-size gate (`.omp/extensions/commit-guard/`): a touched source file
> 300 lines AND growing past its pre-image size → block. Renames are followed via
`git diff --cached --find-renames` and use the pre-image size as the no-growth baseline.

No commit-message escape. Exemptions (`HARNESS_OR_VENDOR_RE` in
`.omp/extensions/commit-guard/constants.ts`): `ThirdParty/`, `External/`, `Generated/`,
`node_modules/`, `dist/`, and the harness dirs `.omp/` and `.claude/`.

## C++ split shape

- TU: `Foo.cpp` + `Foo_<Responsibility>.cpp` (e.g. `Mesh_Allocation.cpp`). CMake
  `file(GLOB ... *.cpp)` picks them up.
- Header: stays single; if a free-function header gets too big, split by domain
  (`Foo_Domain.h`) with `Foo.h` as an umbrella that `#includes` the parts.
- Suffix names the responsibility, not "Impl" / "2".

## After creating new files

Run `cmake -B Build -S .` to refresh `.vcxproj` (CMake's `file(GLOB)` does not auto-pick
new files until configure re-runs).
