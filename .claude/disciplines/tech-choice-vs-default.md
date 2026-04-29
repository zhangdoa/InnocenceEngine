# Discipline: tech-choice-vs-default

When picking the implementation technique for any non-trivial feature — algorithm, rendering technique, data structure, sync primitive, IPC mechanism, denoiser, sampling strategy, anything load-bearing — name three references and justify the pick against all three before writing code. Pairs with `cite-prior-art.md`: that discipline covers *finding* precedent, this one covers *evaluating* precedent against alternatives.

## Why

LLMs pattern-match on training-corpus mass. Shadow-mapping literature outweighs RT-shadow literature 100:1 in the corpus an agent draws from, even though RT is the right answer in 2026 for any pipeline that already has a TLAS. The same skew applies across the stack: SSAO over RT-AO, mutex over lock-free, OpenMP over task graphs, etc. Without a forcing function, the agent ships the textbook answer.

The forcing function is the three-reference rule: surfacing (a) by name lets it be rejected by name. Surfacing (b) by name puts the SOTA in the same paragraph as the default, where the comparison is unavoidable. Surfacing (c) catches the most embarrassing class of error — building a second copy of capability the project just shipped.

This discipline is the structural form of the user's 2026-04-28 feedback: *"do not blindly choose tech just because you were trained on those materials, always strive for the best SOTA and most suitable."*

## How

Name three references and justify the pick:

- **(a) Training-corpus default** — the textbook / commonly-cited approach. The thing an LLM (or a textbook-trained engineer) reaches for first because shadow-map literature, AO literature, lock literature, etc. dominates the training corpus. Identify it explicitly so it can be rejected explicitly.
- **(b) Current SOTA** — what the literature / industry would actually do today. Hardware ray-tracing for shadows. Gradient noise for procedural content. Async compute for unrelated GPU work. The technique a 2026 talk at SIGGRAPH / GDC / HPG would advocate, not the technique a 2008 talk did.
- **(c) Recent project precedent** — what this codebase has shipped in adjacent commits / nearby subsystems. The "we just built X for problem Y; reuse the X infrastructure for problem Z" check.

When (c) exists and is recent (cite file:line and commit), prefer it. The cost of a second implementation of an already-shipped capability is almost never worth paying. When (c) is absent, weigh (b) > (a) with the rationale recorded. Default to (a) only when (b) is genuinely unsuitable for this codebase, and state the case in writing — *"RT not available because no DXR support on target"* is fine; *"cube atlas because that's how the tutorial does it"* is not.

### Recording the pick

Before any non-trivial implementation dispatch — design phase, not after — record in the brief or in task notes:

```
Tech choice: <picked technique>
- (a) default: <name>; rejected because <rationale>
- (b) SOTA:    <name>; <picked / not-picked, why>
- (c) project: <recent precedent or "none"; cite file:line if precedent>
- Pick:        <(a) | (b) | (c) | hybrid> because <one-sentence rationale>
```

If (c) exists and is recent, the default pick is (c). If (c) is absent, the default pick is (b) unless (b) is unsuitable for the project's constraints (no DXR, no async-compute support, etc.). Falling back to (a) requires the unsuitability to be in writing.

The reviewer (per `peer-review-required.md`) checks the tech-choice block against the diff: does the picked technique match the rationale, did the implementer skip (c) when it existed, is (a) being rationalised after the fact.

## Anti-patterns

- **Defaulting to (a) without naming (a).** "Use cube shadow maps for point lights" with no acknowledgment that cube atlases are the 2000s-rasterizer answer and RT is the 2026 answer.
- **Picking the FIRST technique that comes to mind as the implementation default.** Time-on-task and pattern-strength are not arguments for correctness.
- **Naming (a) and (b) but skipping (c).** Often the answer is "the project shipped X 10 days ago for an adjacent problem; reuse X." Skipping the recency check produces parallel implementations of the same capability.
- **"That's what \<famous engine\> did in \<year\>."** Crytek's stochastic SSAO was the right answer when GI did not darken cavities; it is not the right answer when the project's own GI does. Year the technique. Year your engine. They have to overlap.
- **"Following the paper exactly"** when a battle-tested reference implementation exists. See `paper-port.md` for the ref-impl-over-paper case. This discipline complements that one for cases where there *isn't* a reference implementation — i.e. a fresh tech pick, no paper to port, just an implementation to choose.
- **Choosing without checking the past 30 days of commits.** The recency check on (c) is cheap (`git log --oneline -- <subsystem>` plus a glance at the rendering subtree's recent CLs); skipping it is the most common way (c) gets missed.
- **Re-using (a) reflexively because "we already have it for X."** The recency check is for capability that solves the *current* problem better, not for grandfathered code that happens to solve a different problem. The cube-atlas stack solved point-light shadows in 2008's terms; that does not make it the right (c) for sphere-light or spot-light shadows in 2026.
- **Listing all three but picking (a) without rationale.** The pick is the discipline, not the list. PASS-without-evidence is the same anti-pattern `peer-review-required.md` calls out for review verdicts.

## Recorded incident

**TASK-66 → TASK-138 → TASK-175** (2026-04-28).

- TASK-66 shipped point/sphere shadows via 8 lights × 6 faces × full-scene rasterize at 256² = 48 viewports per frame ("cube shadow atlas"). Default-from-corpus tech pick. No agent surfaced (a)/(b)/(c).
- TASK-138 — *same session, days earlier* — proved hardware RT shadows for sun direct lighting: ~0.2 ms vs CSM+PCSS ~1.5 ms, 7-14× cheaper. The RT shadow-ray infrastructure was sitting in the engine, already validated, already shipping for the parallel sun-shadow problem.
- Result: PointShadow dominated GPU cost at 93.5 ms / frame; Sponza ran at ~10 FPS GPU-bound, well below the engine's ≥60 FPS bar. Discovered post-ship when the user ran the engine.
- TASK-175 (now in flight) deletes the entire cube-shadow stack and unifies all light types under RT shadow rays inline in `lightPass.comp`. The fix is to remove (a), keep (b), and acknowledge that (c) — the RT infra from TASK-138 — was the answer all along.

The user surfaced it: *"this shadow approach is too prehistorical, as couldn't we do anything better with RT?"* This discipline exists so the next equivalent decision is challenged before ship, not after.

## Cross-references

- `cite-prior-art.md` — paired discipline. Cite-prior-art covers *finding* precedent; this one covers *evaluating* precedent against alternatives.
- `paper-port.md` — for the case where there *is* a paper and a reference implementation. This discipline applies when the choice is which technique to use *at all*, before the paper-port decision arises.
- `peer-review-required.md` — the reviewer checks the tech-choice block against the diff; PASS-without-evidence is the same anti-pattern for both disciplines.
- `regression-fix-flow.md` — the *understand why* step often surfaces a wrong-tech-pick that should have been caught by this discipline before ship.
- `owner-mode.md` — *small parameter tweaks → wrong approach* is the operational form; this discipline is how an owner challenges the picked technique before tweaking it.
