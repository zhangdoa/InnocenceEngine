# /wrap-up — end-of-session capture

Run before `/clear`. After `/clear`, the conversation is gone; commits and discipline files are all that survive. This is the moment to write things down.

The command does not automate capture. It walks main-session through a four-question checklist and produces a *concrete CL plan* — a list of follow-up edits to land (or trivial inline commits to make) before the session is cleared. Main-session does the reading, drafting, and committing; the checklist is the prompt structure.

## How

Run the four passes in order. Each pass produces zero or more entries in the CL plan. Stop with an empty plan only after all four passes return empty.

### Pass 1 — discipline gaps surfaced this session

Review session commits and dispatch transcripts. For each, ask:

- Did a rule get *spoken* this session that is not yet *written* in `.claude/disciplines/`?
- Did an agent (or main-session itself) repeat a mistake that an existing discipline already covers? If yes, the discipline either failed to surface in the brief or its wording is too weak — file the fix.
- Did a new pattern emerge (e.g. a new architectural seam, a new dispatch shape, a new gate) that deserves a discipline of its own?

Output per finding: target file (existing discipline edit, or new `disciplines/<name>.md`), one-sentence rule, owning agent (almost always `ai-expert`).

### Pass 2 — ADVISORY items not filed

Read the session's commit bodies *and the corresponding tasks' Implementation Notes `## Review` blocks* for `ADVISORY` lines from peer reviewers (verdict tier per `peer-review-required.md`). The implementer's commit body sometimes summarizes a verdict as "PASS+ADVISORY" without enumerating the advisories — those live in the task's Implementation Notes. For each ADVISORY:

- Did it become a backlog task? If yes, skip.
- Is it small enough to land inline as a follow-up CL in this session? If yes, queue it in the plan.
- Is it a structural concern that needs its own task? File via `mcp__backlog__task_create` (per `backlog-workflow.md` § "Don't pile on backlog tasks" — only file if the work is genuinely *worth tracking*, not as a dump).
- Is it a discipline gap masquerading as an ADVISORY? Promote to Pass 1.

Output per finding: action (inline CL / file task / discipline edit / discard with reason).

### Pass 3 — process gaps patched superficially

Look for fixes this session that closed the immediate failure but left the underlying gap exposed. The shape: "I shipped a hook / script / check that catches *this* path, but the same class of failure is reachable through other paths."

Worked example from this session: the `npm test` pretest hook that builds `dist/` before running tests fixes `npm test` but does not protect direct `npx playwright test` invocations. The patch closed *one* path; the gap is the build step being separable from the test invocation at all.

For each gap:

- Is there a structural fix at a higher layer? (Move the build into the test runner; make the gap unreachable.)
- If yes and it is small, queue an inline CL.
- If yes and it is non-trivial, file a task.
- If no, capture the limitation in the relevant discipline so future-Claude knows the patch is partial.

Output per finding: target file or task, one-sentence rationale.

### Pass 4 — dispatcher patterns and feedback

Reusable rules that govern *main-session itself*, not the sub-agents it dispatches. These belong in `.claude/disciplines/dispatcher/`. Examples: a brief-shaping rule, a sequencing rule (parallel vs sequential dispatch), a recovery pattern when a sub-agent reports a wrong-layer fix.

For each:

- Is this a new dispatcher rule, or an extension of an existing one in `disciplines/dispatcher/`?
- Cite the session incident (commit SHA or transcript moment) so the recorded incident in the discipline file is anchored to evidence, not paraphrase.

Output per finding: target file under `disciplines/dispatcher/`, one-sentence rule, incident anchor.

## CL plan output shape

After the four passes, emit a single block with this shape, then stop:

```
## Wrap-up CL plan

1. <action>: <file or task ID> — <one-line rationale>
2. ...

Total: N follow-up CLs / M new backlog tasks / K inline edits.
```

If a plan entry is small enough to land inline as a single CL with no peer-review surface (per `peer-review-required.md` § "When required" — backlog/docs-only or harness self-edit), main-session may commit it directly in this session. Larger entries get dispatched to the owning agent.

## What this command is not

- Not a substitute for in-flight discipline updates. If a rule is clear mid-session, write it then; do not defer to wrap-up.
- Not an opportunity to dump every session observation as a backlog task. Per `backlog-workflow.md` § "Don't pile on backlog tasks", file only what is genuinely worth tracking.
- Not a victory lap. The output is a CL plan, not a summary of what got shipped.

## Future automation (note, not a task)

Claude Code exposes a `UserPromptSubmit` hook matcher; a future enhancement could match `/clear` and inject a pre-flight `/wrap-up` reminder, so the checklist runs before context is cleared rather than relying on user discipline. The adjacent `PreCompact` matcher is **not** the right venue — it conflates auto-compact and manual `/compact` with session-end, so a reminder injected there fires too often. Not filing as a backlog task — note here so a future session looking at this command sees the upgrade path. Promote to a task if the discipline pattern proves load-bearing across multiple sessions.
