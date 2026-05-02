# Discipline: dispatch-briefs (dispatcher-only)

How main-session Claude shapes a dispatch brief before invoking an implementer. These rules govern the dispatcher; sub-agents themselves do not need to read this file. They are filed under `.claude/disciplines/dispatcher/` to keep them out of the universal preamble that every agent reads.

## Anchor invariants in the brief, not just scope

When dispatching an agent for a fix that touches build configuration, `.gitignore`, schemas, generated-data paths, or any other "structural" file, the brief must list the project-level invariants the fix must respect — not only the task scope. Implementers reach for the layer that makes their immediate problem disappear, not necessarily the right one.

Brief checklist before dispatch:

- Which files / directories are sacrosanct (never tracked, never edited).
- Which "right-layer" alternatives exist for the obvious-but-wrong fix.
- Cross-reference any prior incident or commit-gate that already enforces the rule.

If the dispatcher does not know the invariants for the area, the dispatcher reads the relevant subtree's `CLAUDE.md` or asks the producer first. Dispatching blind and hoping is a discipline violation.

Recorded incident: TASK-133 deploy fix (commit `652c7a29`, 2026-04-25). The implementer found `Data/Generated/Fonts/FreeSans.otf` was needed at boot but not in the deploy and edited `.gitignore` to track the font under `Data/Generated/` — solving the symptom while violating the project rule that `Data/Generated/` is gitignored derived runtime output. Recovery required orphaning the bad commit and adding the data-generated commit-gate. The dispatch brief had failed to anchor the rule.

## Sequence audit-first when paper interpretation defines the scope

When a task's *scope* depends on paper interpretation (for example "replace X with the paper-faithful structural fix", "implement §N.M of the paper"), dispatch the `paper-auditor` sequentially first, then the implementer once the auditor's invariants are in. When the scope is engine-determined (for example "find the bug in pipeline Y", "extents disagree somewhere"), parallel dispatch is fine — the auditor's findings constrain the implementer's choices but do not redefine what the implementer is solving.

The dispatcher's read at scope-time: "could the paper-auditor's finding *change what the implementer is solving*, or only *constrain how they solve it*?" If the former, sequential. If the latter, parallel.

Recorded incident: TASK-6.3 (2026-04-26) was scoped on the assumption that the structural fallback for failed 4-corner interpolation should sample the world cache at the LightPass site — an engine-internal pattern that *seemed* paper-adjacent. Auditor and rendering-researcher were dispatched in parallel; the auditor proved the paper-prescribed fallback is denoiser_hint propagation, not a world-cache lookup. By that point the implementer was already mid-add of a `LightPass.cpp` slot 22 binding. Work TaskStop'd and reverted; agent-time and dispatch-cycle wasted.

Contrast: TASK-127 (same session) had auditor + researcher in parallel and worked because the scope was engine-determined ("the 2/3 cutoff comes from somewhere in the GI pipeline; find it"). The auditor's negative finding ruled out wrong directions but did not redefine the task.

## Keep rework in the same session that uncovered the failure

When a closure is found to be premature (numeric green over a visible regression, structural wrong-direction), the dispatcher does not stop at "revert and re-open backlog tasks; we will dispatch next session." The dispatcher reverts in this session, drafts the rework brief in this session, and dispatches the implementer in this session. Context across the failure is freshest in the dispatcher's hands right now; a fresh session re-loads the producer brief and reconstructs the same context from commits and tasks, paying continuity cost for no benefit.

Application: the same response that lands the revert also surfaces the rework brief sketch (AC ordering, discipline contract, capture protocol) for user confirmation. Stop only if the user signals end-of-session, or if the brief itself depends on a long-running capture or measurement that should run in background and resume cold.

## Answer questions; act only on explicit confirmation

When the user poses a question — especially a Socratic-shaped one ("what about X?", "why Y?", "is Z reasonable?") — the dispatcher's default is to answer with reasoning and a recommendation, not to act on the recommendation. A question is not a directive. Even a question shaped as a critique ("why are you doing X?") is asking for the answer, not asserting that X must stop. Action requires explicit "do it" / "yes" / "go ahead".

Failure mode this prevents: the dispatcher over-interprets a question as a directive, dispatches a refactor, and the user has to TaskStop the inflight work because the question was status-checking, not direction-giving.

## Search existing mechanism docs before authorizing new files

Before a brief tells an implementer to author a new file under `.claude/disciplines/` or a new `.claude/` subtree, the dispatcher first searches existing disciplines and subtrees for the same mechanism. If the mechanism is already documented (even partially), the brief directs the implementer to **extend the existing home**, not fork it. Two homes for the same mechanism drift independently — that is the "do not diverge" failure shape.

The dispatcher's pre-dispatch question: "is this mechanism already named anywhere in `.claude/disciplines/`?" Grep first; brief second.

Failure mode this prevents: implementer authors `.claude/disciplines/X.md` for a mechanism already covered by `.claude/disciplines/Y.md §N`. The user has to catch the divergence and direct a fold; the original brief should have caught it.

## Don't bundle unrelated tasks into one dispatch / CL

Two topics surfacing in the same user message are not necessarily one task. Before issuing a brief, the dispatcher evaluates **scope coupling**: do the topics share a reviewer context, a revert axis, a lifecycle, an owning agent? If any of those diverge, dispatch separately. This is `split-before-grow.md` applied to *CL bundling*, not file growth.

Failure mode this prevents: one CL containing two unrelated changes. When one half needs revision, the other is held; the reviewer carries context burden across two topics; the commit message conflates two narratives.

## Reviewer-brief role-instructions must distinguish reviewer vs implementer

When a reviewer brief contains role-specific lines about what does or does not apply to the reviewer (e.g. "Code-AI-Generated-By does not apply"), the brief must phrase them so they cannot be misread as applying to the diff under review. The reviewer reads only their own brief — they do not see the implementer's brief — so role-instructions written ambiguously can be misapplied as findings against the implementer.

Concrete shape: prefer "you (the reviewer) do not author code; do not add `Code-AI-Generated-By:` to your review notes" over "Code-AI-Generated-By does not apply." The first is unambiguous about audience; the second is ambient and can attach to whichever artifact the reviewer is currently looking at.

Failure mode this prevents: reviewer reads "Code-AI-Generated-By does not apply" (meant: to the reviewer's own output), interprets as "to this CL's commit message", flags the implementer's correctly-signed attribution line as a finding to remove.

## Stop-the-line on accumulating carry-forward advisories

Carry-forward advisories under the same backlog cross-reference are a degradation signal, not a continuation signal. When a CL closes with an "ADVISORY (carry-forward)" or "Post-closure follow-up" note pointing at the same TASK-N, and a *prior* CL on the same chain already logged an advisory under that ID, the dispatcher does not roll forward into the next implementation CL. The chain pauses; the dispatcher hands back to the user with the accumulated advisory chain laid out, and waits for direction.

The trigger is **N > 1 carry-forward advisories under the same backlog cross-reference**, not the absolute count of advisories. One advisory is information; two is a degradation pattern; three is a rework signal that the chain has been operating in a degraded validation regime for multiple CLs.

Operational check: before dispatching the next implementation CL on a chain, the dispatcher runs `git log --grep=<TASK-N>` against the recent branch history; if two or more carry-forward advisory commits already mention the same TASK-N, hand back to the user. Dispatcher recall has been demonstrated unreliable across long chains (the TASK-77.1 incident below is the proof); commit history is observable, deterministic, and survives session boundaries — memory across the conversation is not.

The smell shape that triggers this:

- The same TASK-N appears in advisory carry-forwards across consecutive implementation CLs.
- Each advisory *expands* the scope of the previous one (one scene → one camera → all scenes; one symptom → one mechanism → an entire validation axis).
- The validation signal that the per-CL closure protocol relies on degrades scene by scene as the chain proceeds.

When this fires, hand back. Do not attempt to fold the rework decision into the next dispatch brief — the user sets direction; auto-rollover is the dispatcher acting as direction-setter on a chain whose validation foundation is no longer trusted.

Recorded incident: TASK-77.1 rework chain (commits `20b6dbdf` → `b9a103cc`, May 2026). Three TASK-210 carry-forward CLs accumulated across the chain (`473c9985` PT TLAS-race in one scene → `150dcaab` GISponza camera viewpoint nondeterminism → `391af203` cross-binary nondeterminism generalises to all 3 scenes). After CL 3 the closure record explicitly stated "fresh-rebuild visual A/B against a baseline binary is structurally unreliable for ANY of the three test scenes" — at that moment the chain's per-CL visual signal had collapsed to "within-binary toggle A/B on a single scene that is itself camera-nondeterministic across runs." The dispatcher rolled forward into one more implementation CL anyway; the user opened the resulting capture and rejected it for visible artifacts. Stopping-the-line at the third carry-forward would have surfaced the chain to user-direction before that CL was authored.

The complementary check on the artifact side — that reviewer visual inspection lands in the commit record — is enforced by the `visual-review` gate (`commit-gate.js`) and `peer-review-required.md` § "Reviewer visual inspection". This rule is the upstream pause; that rule is the downstream catch.

## Cross-references

- `../agent-dispatch.md` — background-by-default and the `[foreground-required]` sentinel; this file does not duplicate that contract.
- `../peer-review-required.md` — reviewer-selection and BLOCKED loop bound; this file shapes the implementer brief, not the reviewer brief. The reviewer-visual-inspection mandate lives there.
- `../surface-dont-chase.md` — when a closure surfaces new scope, the dispatcher does not auto-expand into the next dispatch. The carry-forward stop-the-line rule above is a special case applied to advisory accumulation across CLs, not within one CL.
- `../visual-validation.md` — owns the layered visual-validation protocol the carry-forward chain was operating against; degraded layer-3 references are the smell.
