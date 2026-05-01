# Discipline: fundamentals

This is a one-developer project. Every CL aims at orthogonality, explicit contracts, fail-loudly, and reload-safe — regardless of language, layer, or domain. The rules below are the always-on baseline; role-specific style and operational disciplines layer on top.

## Code shape

- Functional first; deterministic inputs to outputs unless quantum-randomized.
- Data-oriented; serve, observe, manipulate, deliver — don't process for processing's sake.
- Design by contract; pre/post-conditions enforced through types, assertions, invariants.
- No premature abstraction; pragmatic models, not OOP-because-`class`.
- No premature optimization; baseline first, measure, optimize only against the gap.

## Quality bar

- **Orthogonality** — one responsibility per module; services own operation domains, not component types.
- **Explicit contracts** — preconditions, postconditions, ownership through types and assertions, never assumed.
- **Fail loudly** — invalid state errors at the violation point; silent guard-clause `return false` is a defect.
- **Reload-safe by default** — any twice-loadable resource handles re-init without stale-state accumulation.

## Comments

Default to none. Add only when the WHY is non-obvious (hidden constraint, subtle invariant, surprising behaviour). Comments describe the present state — not history, not the current task, not callers. Banned phrases: "now via X / used to be Y", "replaces / subsumes / retires", "migrated from", "deleted alongside", "first consumer in next commit", "added in `<sha>`", "before this fix", "fixed in TASK-NN".

## Cite before invent

Before introducing any mechanism, abstraction, or algorithm, search in this order and cite the source: (1) inside the project — adjacent subtrees, sibling modules, existing helpers, line-by-line; (2) official documentation of the tool / SDK / library / platform; (3) canonical reference implementation for any paper or spec being translated. If no precedent exists, justify the new pattern in writing before implementing.

## Each CL delivers

Each closure delivers a net improvement on at least one user-visible dimension over the CL that opened the task. Plumbing-only CLs are honest only when labeled as such — never as "phase N+1 will deliver." Trades that regress any committed dimension require approval at commit time, not deferral.
