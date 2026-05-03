# Discipline: split-before-grow

The universal file-size gate blocks commits that grow a file past the 300-line limit. When adding to a file approaching that limit, the default move is to split — there is no commit-message escape, and there is no string sentinel to reach for.

## How

Two split shapes carry most of the load:

1. **Multiple classes, multiple files.** If a class is doing more than one thing, give each responsibility its own translation unit. The original principle of orthogonality (`fundamentals.md`) makes this the first move.
2. **Same class, multiple source files.** When a single class genuinely owns multiple coherent feature surfaces and splitting it would force unnecessary coupling, spread the implementation across `Foo.cpp` (core), `Foo_FeatureBar.cpp`, `Foo_Something.cpp`. CMake's `file(GLOB ... *.cpp)` picks them up automatically. The header stays single; only the .cpp side splits.

Files already over the limit are grandfathered but still subject to the gate on growth. When editing such a file, the turn summary should note whether the split lands in this CL or as a follow-up refactor task — don't accumulate growth on top of growth.

## Naming for partial-class splits

`Foo_FeatureBar.cpp` (underscore-separated feature suffix). Avoid `FooImpl.cpp`, `Foo2.cpp`, or other patterns that don't say what the file is for. The suffix should describe the responsibility carried in that file (e.g. `Mesh_Allocation.cpp`, `Mesh_AsyncBinaryLoad.cpp`).

## Genuine path exemptions

If a path legitimately must exceed the limit (third-party drop, generated output, vendored asset), add it to `FILE_SIZE_EXCLUDE_RE` in `.claude/hooks/lib/common.js`. That's a path policy change with its own diff and review surface — much harder to abuse than a string token.

## Cross-references

- `fundamentals.md` — *orthogonality* is the underlying principle; oversized files signal a responsibility that should be split.
- `comment-discipline.md` — files often grow because of comment bloat, not code. Strip prose first; the file may be under the limit already.
