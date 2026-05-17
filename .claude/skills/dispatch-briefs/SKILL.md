---
name: dispatch-briefs
description: Main-session-only. Use when shaping a dispatch brief. Anchor engine invariants, sequence audit-first when paper interpretation defines scope, build/CI verification rules, agent-dispatch gate enforcement.
---

# Skill: dispatch-briefs (dispatcher-only)

Generic dispatcher principles (`run_in_background: true` default, anchor invariants, answer-questions vs act, don't-bundle-unrelated, reviewer-shaped briefs, keep-rework-in-session, reject-fictional-follow-up-CL-deferrals) live in user-level `dispatch-briefs`. This skill covers project-specific extensions.

## Anchor engine invariants

For fixes touching build configuration, `.gitignore`, schemas, generated-data paths, or any "structural" file → brief lists project-level invariants the fix must respect, not just the task scope. Read the relevant subtree's `CLAUDE.md` and `.claude/state/engine-invariants.md` before drafting.

Pre-dispatch checklist:

- Sacrosanct files / directories.
- Right-layer alternatives to the obvious-but-wrong fix.
- Prior incident or commit-gate that already enforces the rule.

Don't know the invariants → read the subtree's `CLAUDE.md` or ask `task-mgmt` first.

## Sequence audit-first when paper interpretation defines scope

Scope depends on paper interpretation ("replace X with the paper-faithful structural fix", "implement §N.M") → dispatch a fresh-context audit pass first (the impl stage that owns the work, scoped to "produce the alignment artifact only — no source diffs"), then the impl stage proper once invariants are in.

Scope is engine-determined ("find the bug in pipeline Y", "extents disagree somewhere") → parallel dispatch fine.

Read at scope-time: *could the audit finding change what the impl stage is solving, or only constrain how?* Former → sequential. Latter → parallel.

## Search existing skill docs before authorizing new files

Before authorizing a new `.claude/skills/<name>/SKILL.md` or subtree → grep existing skills for the same mechanism. Already documented (even partially) → brief directs the implementer to **extend the existing home**, not fork it.

## Build / CI script CLs verify in the user's shell, not the sub-agent's sandbox

For CLs touching user-shell-invoked scripts (`Scripts/*.ps1`, `BuildWin.ps1`, automation drivers, anything the user runs from their interactive prompt), the brief must explicitly require verification by running the script from main-session, not only from inside the sub-agent's execution.

Failure mode: a sub-agent's shell often inherits environment state — PATH entries, DEV-tool installer dirs, registry-derived variables — that the user's interactive shell does not. A script that "works in the sandbox" can silently fail on every user invocation.

How to apply:

- Brief includes a "verify in main-session shell" line for any CL whose deliverable is invoked from an interactive prompt.
- Reviewer's brief inherits this — peer review of build/CI scripts re-runs the script from main-session as part of the verdict.
- Self-contained scripts (no PATH dependencies, no inherited env) are exempt; flag the inheritance audit explicitly when claiming the exemption.

## Stop-the-line on accumulating carry-forward advisories

Carry-forward advisories under the same backlog cross-reference are a degradation signal.

Trigger: **N > 1 carry-forward advisories under the same `TASK-N`.** Operational check before dispatching the next implementation CL: `git log --grep=<TASK-N>`. Two+ matches → hand back to the user.

Smell: same TASK-N across consecutive advisory CLs; each advisory expands scope (one scene → all scenes; one symptom → entire validation axis); per-CL validation signal degrades.

Hand back. Do not fold the rework decision into the next brief.

Downstream catch (reviewer visual inspection in commit record): `visual-review` gate + `peer-review-required` § "Reviewer visual inspection".

## Agent-dispatch gate enforcement

`gates/agent-dispatch.js` (wired through `session-gate.js`) blocks any `Agent` / `Task` call whose `tool_input.run_in_background` is anything other than `true`, unless the `prompt` field contains the literal sentinel `[foreground-required]`. No env-var bypass, no commit-message sentinel — foreground is opted into in writing at the dispatch site.

Reaching for the sentinel on every dispatch → the planning gap is the thing to fix.

## Sub-agents cannot dispatch sub-agents

When granting an implementer agent commit-authority on a non-trivial dispatch, peer-review is main-session's responsibility AFTER the implementer returns. The implementer does not have access to the `Agent` / `Task` tool from within its own dispatch — it cannot spin up a fresh code-review on its own diff. Brief shape:

- Tell the implementer to validate (build green, smoke green, capture A/B) but STOP at the commit step.
- Main-session receives the implementer's report, dispatches a fresh code-review on the diff, fills the commit-message footer with the verdict, then commits.

Failure mode if the brief asks the implementer to "self-dispatch peer-review": the implementer either hangs trying to invoke an unavailable tool, or commits without review. Either way the `peer-review-required` discipline is silently bypassed.

## Broaden scope after multiple narrow-probe dispatches

Narrow-bisect dispatches that successively falsify candidate carriers (skip-feature probes, RT-dumps at each stage) are the right shape early in an investigation — each cheap dispatch narrows the bracket. After N > ~5 such dispatches without root cause, granting a broader scope ("bisect AND attempt fix within bounds, with explicit anti-scope") often cracks the bug in one dispatch. The implementer with broader authority can pivot in-flight when a probe inverts the brief's hypothesis (the prior brief's assumed carrier turns out to be innocent and the real bug is one layer further).

Apply when: 5+ diagnostic dispatches have landed on the same task; the brackets are narrowing but still excluding the real bug; the implementer is repeatedly hitting "this layer is innocent, surface and stop." At that point switch from "diagnose only" to "diagnose + attempt fix within scope X / Y / Z; stop and surface if scope is exceeded."

Anti-scope is critical — without it, broadened authority becomes overreach. The scope list should name the files / passes / subsystems the fix can touch and explicitly forbid the ones it cannot.

## task-mgmt-specific override

The `task-mgmt` agent's `Agent`-tool chain-dispatch is **background-only**. The `[foreground-required]` sentinel does not apply to task-mgmt; task-mgmt must not emit it.

- Use case: drift audits, dependency walks, periodic reconciliations whose result lands in the task graph as a backlog commit or status flip.
- Synchronous, result-blocking dispatches → main-session.
- Peer-reviewer dispatches NOT covered by this override — they originate from whoever originated the implementer dispatch (typically main-session).

## Cross-references

- `peer-review-required`, `visual-validation`, `session-start`.
- User-level `agent-dispatch`, `surface-dont-chase`.
