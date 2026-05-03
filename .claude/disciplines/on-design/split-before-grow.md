# Discipline: split-before-grow

The file-size gate blocks commits that grow a file past 300 lines. No commit-message escape, no string sentinel.

## Two split shapes

1. **Multiple classes → multiple files.** If a class does more than one thing, give each responsibility its own translation unit.
2. **Same class → multiple source files.** When a single class genuinely owns multiple coherent feature surfaces, spread the implementation across `Foo.cpp` (core), `Foo_FeatureBar.cpp`, `Foo_Something.cpp`. CMake's `file(GLOB ... *.cpp)` picks them up. Header stays single; only `.cpp` splits.

Files already over the limit are grandfathered but still subject to the gate on growth. When editing such a file, the turn summary notes whether the split lands in this CL or as a follow-up refactor task.

## Naming for partial-class splits

`Foo_FeatureBar.cpp` (underscore-separated feature suffix). The suffix names the responsibility (`Mesh_Allocation.cpp`, `Mesh_AsyncBinaryLoad.cpp`). Avoid `FooImpl.cpp`, `Foo2.cpp`.

## Path exemptions

If a path legitimately must exceed the limit (third-party drop, generated output, vendored asset) → add it to `FILE_SIZE_EXCLUDE_RE` in `.claude/hooks/lib/common.js`. That's a path policy change with its own diff and review surface.

## Cross-references

- `always/fundamentals.md` — *orthogonality* is the underlying principle.
- `always/comment-discipline.md` — files often grow because of comment bloat. Strip prose first.
