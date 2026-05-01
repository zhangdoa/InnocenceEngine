# Discipline: agent-dispatch

When a dispatcher (main-session Claude, or any agent that delegates to a sub-agent) invokes the `Agent` tool, the default is `run_in_background: true`. Foreground dispatch is the exception, justified per call.

## When

Foreground (`run_in_background: false`) is appropriate **only when both** hold:

1. The dispatcher's immediate next action depends on the sub-agent's result, AND
2. There is no parallel work the dispatcher could be doing while the sub-agent runs.

If either fails, dispatch in background.

A bounded, short research dispatch ("report back in under 200 words, citing files") whose result blocks the dispatcher's next decision is a legitimate foreground call — the cost is bounded and the dispatcher genuinely cannot proceed.

### Worked examples

**Foreground OK** — Routing a single user question that needs one specialist's read before the dispatcher can answer, with a tight word-budget on the sub-agent. Result is required for the next sentence; nothing else to do.

**Background required** — Implementation dispatches (multi-file edits, build runs, test suites). These are minutes-to-hours of work; the dispatcher should free its surface for the user and pick up the result via Monitor or on completion.

**Background required** — Research / audit dispatches whose result feeds a later decision but is not blocking the very next dispatcher action. File the dispatch, continue staging the next piece of work, integrate the result when it lands.

**Background required** — Any case where the dispatcher could be reading more code, drafting a follow-up dispatch, or fielding a redirect from the user while the sub-agent runs.

Multiple `Agent` calls issued in a single tool-use block (parallel dispatch) are not affected by this rule — the dispatcher already chose concurrency.

## How

### Harness enforcement

Prose drifted in practice — the rule is now enforced structurally by `.claude/hooks/gates/agent-dispatch.js` (wired through `.claude/hooks/session-gate.js`).

The gate fires on every `Agent` (and legacy `Task`) tool call. It blocks any call whose `tool_input.run_in_background` is anything other than `true` *unless* the `prompt` field contains the literal sentinel `[foreground-required]`. The sentinel is the per-call justification, surfaced where the user can see it — there is no env-var bypass and no commit-message sentinel; foreground is opted into, in writing, at the dispatch site.

Use the sentinel only when the two-condition test above is genuinely met. Typing it for convenience defeats the gate; if you find yourself reaching for it on every dispatch, the planning gap is the thing to fix.

### Producer-specific override

The `producer` agent is granted `Agent`-tool chain-dispatch under one constraint: producer chain-dispatches are **background-only**. Producer must call `Agent` with `run_in_background: true`; the `[foreground-required]` sentinel does not apply to producer and producer must not emit it. See `.claude/agents/producer.md` § "Chain dispatch" for the role-side framing.

The shape: producer orchestrates async work (drift audits, dependency walks, periodic reconciliations) whose result lands in the task graph as a backlog commit or status flip — not as a return-to-main-session value. Synchronous, result-blocking dispatches remain a dispatcher concern (main-session Claude). Peer-reviewer dispatches are NOT covered by this override — they originate from whoever originated the implementer dispatch (typically main-session).

## Anti-patterns

- **"It's just a short dispatch, I'll wait."** Short turns into long without warning — a sub-agent that hits an unfamiliar path, a long build, or a flaky test stretches a 30-second plan into 10 minutes of occupied dispatcher.
- **Foreground dispatch as a substitute for thinking about ordering.** If the dispatcher cannot articulate what it would do in parallel, that is a planning gap, not a justification — pause to plan, then dispatch in background and pick up the parallel work.
- **Treating Esc as the discipline.** The user can always interrupt, but every interrupt costs them attention; the dispatcher's job is to not need rescuing.
- **Assuming this rule applies only to main-session Claude.** Any agent that delegates to a sub-agent (e.g. `paper-auditor`) is a dispatcher for the duration of that call and is governed by this discipline.

## Cross-references

- `peer-review-required.md` — peer-reviewer dispatches are not covered by the producer override; they follow the main-session dispatcher pattern.
- `session-start.md` — session-start producer dispatch is one of the few legitimate foreground calls (result blocks the next sentence; no parallel work).
- `test-etiquette.md` — both disciplines protect user attention; this one governs dispatcher-surface occupancy, that one governs editor / engine launch budgets.
