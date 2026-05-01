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

## Cross-references

- `../agent-dispatch.md` — background-by-default and the `[foreground-required]` sentinel; this file does not duplicate that contract.
- `../peer-review-required.md` — reviewer-selection and BLOCKED loop bound; this file shapes the implementer brief, not the reviewer brief.
- `../surface-dont-chase.md` — when a closure surfaces new scope, the dispatcher does not auto-expand into the next dispatch.
