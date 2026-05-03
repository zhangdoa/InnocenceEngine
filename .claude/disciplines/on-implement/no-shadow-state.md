# Discipline: no-shadow-state

State arises from the data structures that already represent it. Do not introduce a counter or boolean that mirrors information another structure already holds. Query the source of truth directly.

## When applicable

When tempted to add an `int` or `bool` member whose job is to summarise another data structure: residency tracking, pass-completion checks, "wait until ready" predicates, "have we seen X yet?" guards, lazy-init flags.

Does **not** apply to:

- Counters that are themselves the data, not a shadow — frame number, sample count in a running mean, geometric instance count where the count *is* the state.
- Atomic single-bit signals where the bit is the message — `m_needResize`, edge-triggered request flags between threads, dirty bits whose only purpose is "act once, then clear."
- Harness/discipline files describing state — inherently state-about-state by design.

The smell: a field that *mirrors* information present elsewhere.

## Procedure

When implementing an "is the work done?" predicate or a "ready" check:

1. **Locate the source of truth** — per-element status, residency enum, pool contents, optional, queue emptiness — the data the work actually mutates.
2. **Iterate, don't tally** — `for (auto& e : pool) if (e.m_ObjectStatus != Activated) return false;`. Reuse the existing collection.
3. **Surface which, not how many** — when the predicate fails, return or log the first non-ready element's identity. Counters can't do this; iteration can.
4. **Delete the shadow** — if a counter or flag exists today and the predicate now observes the source of truth, remove the shadow field and every site that writes it.

If iteration cost genuinely matters (measured), use a smarter data structure (status-stratified pool, intrusive list of not-ready elements) — still observation-of-data, not a shadow counter.

## Anti-patterns

- **Expected/Activated counter pair** — `m_ExpectedComponentCount` + `m_ActivatedComponentCount` incremented at queue/init sites. Replace with: iterate `m_Pool`, check `m_ObjectStatus == Activated` per element.
- **`bool m_IsLoaded`** set in `LoadScene`, cleared on unload. Replace with: query `m_currentScene.has_value()` or the resource pools.
- **Session-scoped "have we seen X yet?" flag.** Replace with: query the artifact X would have produced (file exists, entry in the registry, task in the backlog).

## Cross-references

- `always/fundamentals.md` — *data-oriented* and *reload-safe by default* are the principles operationalised.
- `always/surface-dont-chase.md` — drift bugs masked by a shadow are separate work; file-or-discard rather than chase in-CL.
