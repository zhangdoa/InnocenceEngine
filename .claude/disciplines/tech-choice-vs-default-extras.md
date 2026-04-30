# Tech-choice-vs-default extras

Long-form companion to `tech-choice-vs-default.md`. Read on demand. Operative core (the (a)/(b)/(c) rule + recording template + reviewer's check) is in the short file.

## Why (full prose)

LLMs pattern-match on training-corpus mass. Shadow-mapping literature outweighs RT-shadow literature 100:1 in the corpus an agent draws from, even though RT is the right answer in 2026 for any pipeline that already has a TLAS. The same skew applies across the stack: SSAO over RT-AO, mutex over lock-free, OpenMP over task graphs, etc. Without a forcing function, the agent ships the textbook answer.

The forcing function is the three-reference rule: surfacing (a) by name lets it be rejected by name. Surfacing (b) by name puts the SOTA in the same paragraph as the default, where the comparison is unavoidable. Surfacing (c) catches the most embarrassing class of error — building a second copy of capability the project just shipped.

This discipline is the structural form of the user's 2026-04-28 feedback: *"do not blindly choose tech just because you were trained on those materials, always strive for the best SOTA and most suitable."*

## Anti-patterns (full)

- **Defaulting to (a) without naming (a).** "Use cube shadow maps for point lights" with no acknowledgment that cube atlases are the 2000s-rasterizer answer and RT is the 2026 answer.
- **Picking the FIRST technique that comes to mind as the implementation default.** Time-on-task and pattern-strength are not arguments for correctness.
- **Naming (a) and (b) but skipping (c).** Often the answer is "the project shipped X 10 days ago for an adjacent problem; reuse X." Skipping the recency check produces parallel implementations of the same capability.
- **"That's what \<famous engine\> did in \<year\>."** Crytek's stochastic SSAO was the right answer when GI did not darken cavities; it is not the right answer when the project's own GI does. Year the technique. Year your engine. They have to overlap.
- **"Following the paper exactly"** when a battle-tested reference implementation exists. See `paper-port.md` for the ref-impl-over-paper case. This discipline complements that one for cases where there *isn't* a reference implementation — i.e. a fresh tech pick, no paper to port, just an implementation to choose.
- **Choosing without checking the past 30 days of commits.** The recency check on (c) is cheap (`git log --oneline -- <subsystem>` plus a glance at the rendering subtree's recent CLs); skipping it is the most common way (c) gets missed.
- **Re-using (a) reflexively because "we already have it for X."** The recency check is for capability that solves the *current* problem better, not for grandfathered code that happens to solve a different problem. The cube-atlas stack solved point-light shadows in 2008's terms; that does not make it the right (c) for sphere-light or spot-light shadows in 2026.
- **Listing all three but picking (a) without rationale.** The pick is the discipline, not the list. PASS-without-evidence is the same anti-pattern `peer-review-required.md` calls out for review verdicts.

## Recorded incident — TASK-66 → TASK-138 → TASK-175 (2026-04-28)

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
