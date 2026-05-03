# Discipline: fundamentals

Always-on baseline. Role-specific style and operational disciplines layer on top.

## Code shape

- Functional first. Deterministic inputs to outputs unless quantum-randomized.
- Data-oriented. Serve, observe, manipulate, deliver — don't process for processing's sake.
- Design by contract. Pre/post-conditions enforced through types, assertions, invariants.
- No premature abstraction. Pragmatic models, not OOP-because-`class`.
- No premature optimization. Baseline first, measure, optimize against the gap.

## Quality bar

- **Orthogonality** — one responsibility per module. Services own operation domains, not component types.
- **Explicit contracts** — preconditions, postconditions, ownership through types and assertions, never assumed.
- **Fail loudly** — invalid state errors at the violation point. Silent guard-clause `return false` is a defect.
- **Reload-safe by default** — any twice-loadable resource handles re-init without stale-state accumulation.

## Comments

- Default to none.
- Add only when the WHY is non-obvious (hidden constraint, subtle invariant, surprising behaviour).
- Comments describe the present state — not history, not the current task, not callers.
- Banned phrases: "now via X / used to be Y", "replaces / subsumes / retires", "migrated from", "deleted alongside", "first consumer in next commit", "added in `<sha>`", "before this fix", "fixed in TASK-NN".

## Cite before invent

Before introducing any mechanism, abstraction, or algorithm, search in this order and cite the source:

1. Inside the project — adjacent subtrees, sibling modules, existing helpers, line-by-line.
2. Official documentation of the tool / SDK / library / platform.
3. Canonical reference implementation for any paper or spec being translated.

If no precedent exists, justify the new pattern in writing before implementing.

## Verify data before hypothesizing

Before chasing an "output looks wrong" bug — colour inversion, channel swap, hue shift, off-by-one offset — read the actual source data, not what the file name implies.

- Image asset: `python -c "from PIL import Image; print(Image.open('path').getpixel((x, y)))"` or `Read` the image directly.
- Binary blob: hex-dump and check the first bytes.
- JSON / config: cat the file.

Names lie; bytes do not.

## Treat tool diagnostics as signal, not noise

When an IDE diagnostic, linter warning, hook complaint, or CI signal fires on something "not related to my change" or "the real build does not care," fix the source of the false alarm at the layer that produces it.

- clangd false positive → run `Scripts/RegenClangdIndex.ps1`.
- Other false-positive sources (lint config, hook script, generator output) → fix at its layer.
- Never use `// noqa`, `# eslint-disable`, `--no-verify`, exclusion patterns, or "this is fine" mental notes as a substitute.
- If the noise genuinely cannot be fixed at the source → file a backlog task to replace or work around the tool.

## Each CL delivers

Every closure delivers a net improvement on at least one user-visible dimension over the CL that opened the task. Plumbing-only CLs are honest only when labelled as such — never as "phase N+1 will deliver." Trades that regress any committed dimension require approval at commit time, not deferral.
