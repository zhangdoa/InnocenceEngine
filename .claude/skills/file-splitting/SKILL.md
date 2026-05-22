---
name: file-splitting
description: Use when the file-size gate blocks a commit, or before adding code that would push a file past 300 lines.
---

# Skill: file-splitting

## Gate

`gates/file-size.js` — touched file > 300 lines AND growing past pre-image size → block. Renames are detected via `git diff --cached --find-renames` and use the pre-image size as the no-grow baseline.

No commit-message escape. Path exemptions live in `FILE_SIZE_EXCLUDE_RE` (`.claude/hooks/lib/common.js`) — `ThirdParty/`, `External/`, `Generated/` only.

## C++ split shape

- TU: `Foo.cpp` + `Foo_<Responsibility>.cpp` (e.g. `Mesh_Allocation.cpp`). CMake `file(GLOB ... *.cpp)` picks them up.
- Header: stays single; if a free-function header gets too big, split by domain (`Foo_Domain.h`) with `Foo.h` as umbrella that `#includes` the parts.
- Suffix names the responsibility, not "Impl" / "2".

## After creating new files

Run `cmake -B Build -S .` to refresh `.vcxproj` (CMake's `file(GLOB)` does not auto-pick new files until configure re-runs).
