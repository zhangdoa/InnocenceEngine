# Discipline: file-splitting

When a source file would exceed the size ratchet, split it. Never escape the gate.

## When applicable

When the `file-size` gate blocks a commit, or when a touched file is already over the limit.

## Rule

Decide the split shape by asking: does the extracted code belong to the original class?

| Answer | Pattern |
|---|---|
| **Yes** — same class, different responsibility cluster | `Foo_SubsectionName.cpp` next to `Foo.cpp`. Declarations stay in `Foo.h`. Partial-class TU split. |
| **No** — separate concern | Define a new class for the concern. The original holds an instance and routes calls through it. Composition, not friendship. |

Subsection name follows the existing `// ---` domain divider (`// --- GPU timer queries ---` → `Foo_GpuTimers.cpp`).

Free-function or template-heavy headers (no class to anchor): split by domain into `Foo_Domain.h`; the original `Foo.h` stays as an umbrella that includes the parts.

## Procedure

1. Identify the smallest cohesive seam — typically already marked by a `// ---` divider.
2. Apply the rule above to pick the file shape.
3. Move the code; each new TU's `#include` graph is a strict subset of the original's.
4. Build green; run the qualifying integration test for the affected subsystem.
5. Confirm the file-size gate passes. No `FILE_SIZE_EXCLUDE_RE` additions.

## Anti-patterns

- **Splitting by line count.** Pick the seam by content domain, not "first 500 lines."
- **`friend` to keep cross-class access after composition extraction.** If the new class needs the original's internals, the seam was wrong — re-pick.
- **Adding `FILE_SIZE_EXCLUDE_RE` entries to dodge the gate.** The exclusion is for `ThirdParty/` / `External/` / `Generated/` only.

## Cross-references

- `on-implement/cpp-style.md` — naming and include conventions for new TUs.
- `.claude/hooks/gates/file-size.js` — the gate this discipline serves.
