# Discipline: no-shadow-state

State must arise from the data structures that already represent it; do not introduce a counter or boolean that mirrors information another structure already holds. No hand-crafted shadow state machines — query the source of truth directly.

## Why

A shadow counter (`m_ExpectedCount` / `m_ActivatedCount`) or shadow flag (`m_IsLoaded`, `m_IsReady`) creates a parallel state machine the engineer has to keep in lockstep with the real data at every write site. It drifts: a forgotten decrement on unload, a missing increment on a new code path, an off-by-one on an edge case. The bug surfaces far from the cause, because the predicate is reading a synthetic field rather than observing the world. Worse, the synthetic field cannot answer *which* element is not ready — only "how many," which is debugging-blind.

The state already exists in the data — per-element status (`m_ObjectStatus`), residency (`m_Residency`), pool emptiness, the presence of a value in `std::optional`. The "is it done?" predicate should iterate the existing collection and ask each element. Predicates that observe data are reload-safe by construction; predicates that consult a counter are reload-safe only if every writer remembered to update it.

## When

Applies whenever code is tempted to add an `int` or `bool` member whose job is to summarise the state of *another* data structure: residency tracking, pass-completion checks, "wait until ready" predicates, "have we seen X yet?" guards, lazy-init flags.

Does **not** apply to:

- Counters that are themselves the data, not a shadow of it — frame number, sample count in a running mean, geometric instance count where the count *is* the state.
- Atomic single-bit signals where the bit is the message — `m_needResize`, edge-triggered request flags between threads, dirty bits whose only purpose is "act once, then clear."
- Harness/discipline files describing state — these are inherently state-about-state by design.

The smell is a field that *mirrors* information already present elsewhere; it is not "any int member."

## How

When implementing a "is the work done?" predicate or a "ready" check:

1. **Locate the source of truth.** Per-element status, residency enum, pool contents, optional, queue emptiness — the data the work actually mutates.
2. **Iterate, don't tally.** The predicate observes the source of truth: `for (auto& e : pool) if (e.m_ObjectStatus != Activated) return false;`. The collection is already there; reuse it.
3. **Surface which, not how many.** When the predicate fails, return or log the first non-ready element's identity. A counter cannot do this; iteration can.
4. **Delete the shadow.** If a counter or flag exists today and the predicate has been rewritten to observe the source of truth, remove the shadow field and every site that writes it. Leaving both creates a diverging-truths bug surface.

If the iteration cost is genuinely measured to matter, the right response is a smarter data structure (a status-stratified pool, an intrusive list of not-ready elements) — still observation-of-data, not a shadow counter glued to the side.

## Anti-patterns

- **Expected/Activated counter pair.** A service adds `m_ExpectedComponentCount` and `m_ActivatedComponentCount`, increments at queue/init sites, returns "complete" when they match. Two writers, one reader, invariant fragile across new code paths. Replace with: iterate `m_Pool`, check `m_ObjectStatus == Activated` per element.
- **`bool m_IsLoaded` set in `LoadScene`, cleared on unload.** Two writers, one reader. Replace with: query `m_currentScene.has_value()` or "do the resource pools contain anything for this scene." The data already knows.
- **Session-scoped "have we seen X yet?" flag.** Tempting in dispatcher-style code. Replace with: query the artifact X would have produced (the file exists, the entry is in the registry, the task is in the backlog).

## Cross-references

- `fundamentals.md` — *data-oriented* and *reload-safe by default* are the principles this discipline operationalises; oversized state-mirroring is the violation those principles forbid.
- `surface-dont-chase.md` — when removing a shadow field surfaces drift bugs that were masked by the shadow, those are separate work; file-or-discard rather than chase in-CL.
