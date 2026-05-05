# Discipline: dispatch-briefs (dispatcher-only)

How main-session shapes a dispatch brief. Sub-agents do not read this file.

## Anchor invariants in the brief

For fixes touching build configuration, `.gitignore`, schemas, generated-data paths, or any "structural" file → brief lists project-level invariants the fix must respect, not just the task scope.

Pre-dispatch checklist:

- Sacrosanct files / directories.
- Right-layer alternatives to the obvious-but-wrong fix.
- Prior incident or commit-gate that already enforces the rule.

Don't know the invariants → read the subtree's `CLAUDE.md` or ask `task-mgmt` first.

## Sequence audit-first when paper interpretation defines scope

Scope depends on paper interpretation ("replace X with the paper-faithful structural fix", "implement §N.M") → dispatch a fresh-context audit pass first (the impl stage that owns the work, scoped to "produce the alignment artifact only — no source diffs"), then the impl stage proper once invariants are in.

Scope is engine-determined ("find the bug in pipeline Y", "extents disagree somewhere") → parallel dispatch fine.

Read at scope-time: *could the audit finding change what the impl stage is solving, or only constrain how?* Former → sequential. Latter → parallel.

## Keep rework in the same session that uncovered the failure

Premature closure (numeric green over visible regression, structural wrong-direction):

- Revert in this session.
- Draft the rework brief in this session.
- Dispatch the implementer in this session.

Same response that lands the revert surfaces the rework brief sketch (AC ordering, discipline contract, capture protocol) for user confirmation. Stop only on user end-of-session signal, or if the brief depends on a long-running capture / measurement that should resume cold.

## Answer questions; act only on explicit confirmation

User question — Socratic ("what about X?", "why Y?") or critical ("why are you doing X?") — gets reasoning + recommendation. **Does not act.** A question is not a directive.

Action requires explicit "do it" / "yes" / "go ahead".

## Search existing mechanism docs before authorizing new files

Before authorizing a new `.claude/disciplines/` file or subtree → grep existing disciplines for the same mechanism. Already documented (even partially) → brief directs the implementer to **extend the existing home**, not fork it.

## Don't bundle unrelated tasks into one CL

Two topics in the same user message ≠ one task. Evaluate scope coupling: shared reviewer context, revert axis, lifecycle, owning impl stage? Any divergence → dispatch separately.

This is `../on-design/split-before-grow.md` applied to CL bundling.

## Reviewer-brief role-instructions must distinguish reviewer vs implementer

Reviewer-only lines like "Code-AI-Generated-By does not apply" → phrase as *"you (the reviewer) do not author code; do not add `Code-AI-Generated-By:` to your review notes."* Unambiguous about audience. The reviewer reads only their own brief.

## Build / CI script CLs verify in the user's shell, not the sub-agent's sandbox

For CLs touching user-shell-invoked scripts (`Scripts/*.ps1`, `BuildWin.ps1`, automation drivers, anything the user runs from their interactive prompt), the brief must explicitly require verification by running the script from main-session, not only from inside the sub-agent's execution.

Failure mode: a sub-agent's shell often inherits environment state — PATH entries, DEV-tool installer dirs, registry-derived variables — that the user's interactive shell does not. A script that "works in the sandbox" can silently fail on every user invocation.

Concrete incident: a `BuildWin.ps1` post-step invoked `VsDevCmd.bat`, which internally probed bare `vswhere.exe`. The implementer's sub-agent shell had `Microsoft Visual Studio\Installer` on PATH; the user's interactive shell did not. Sub-agent verification reported success; user's actual builds failed and exited 1 on every run. Reviewer reproduced the failure live in the user's shell and BLOCKED the CL.

How to apply:

- Brief includes a "verify in main-session shell" line for any CL whose deliverable is invoked from an interactive prompt.
- Reviewer's brief inherits this — peer review of build/CI scripts re-runs the script from main-session as part of the verdict.
- Self-contained scripts (no PATH dependencies, no inherited env) are exempt; flag the inheritance audit explicitly when claiming the exemption.

## Stop-the-line on accumulating carry-forward advisories

Carry-forward advisories under the same backlog cross-reference are a degradation signal.

Trigger: **N > 1 carry-forward advisories under the same `TASK-N`.** Operational check before dispatching the next implementation CL: `git log --grep=<TASK-N>`. Two+ matches → hand back to the user.

Smell: same TASK-N across consecutive advisory CLs; each advisory expands scope (one scene → all scenes; one symptom → entire validation axis); per-CL validation signal degrades.

Hand back. Do not fold the rework decision into the next brief.

Downstream catch (reviewer visual inspection in commit record): `visual-review` gate + `../on-commit/peer-review-required.md` § "Reviewer visual inspection".

## Cross-references

- `../on-dispatch/agent-dispatch.md`, `../on-commit/peer-review-required.md`, `../always/surface-dont-chase.md`, `../on-implement/visual-validation.md`.
