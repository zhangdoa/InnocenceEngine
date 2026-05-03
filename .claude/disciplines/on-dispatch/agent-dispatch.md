# Discipline: agent-dispatch

Default for every `Agent` tool call: `run_in_background: true`. Foreground is the exception, justified per call.

## When foreground is OK

Both must hold:

1. The dispatcher's immediate next action depends on the sub-agent's result.
2. There is no parallel work the dispatcher could be doing while it runs.

Either fails → background.

A bounded research dispatch ("report back in under 200 words, citing files") whose result blocks the next decision is a legitimate foreground call.

## Examples

| Case | Background or foreground |
|---|---|
| Single specialist read needed before answering one user question, tight word budget | foreground |
| Implementation dispatch (multi-file edits, build runs, test suites) | background |
| Research / audit whose result feeds a later decision | background |
| Dispatcher could be reading code, drafting a follow-up dispatch, or fielding a redirect | background |

Multiple `Agent` calls in a single tool-use block (parallel dispatch) bypass this rule — concurrency is already chosen.

## Harness enforcement

`gates/agent-dispatch.js` (wired through `session-gate.js`) blocks any `Agent` / `Task` call whose `tool_input.run_in_background` is anything other than `true`, unless the `prompt` field contains the literal sentinel `[foreground-required]`. No env-var bypass, no commit-message sentinel — foreground is opted into in writing at the dispatch site.

Reaching for the sentinel on every dispatch → the planning gap is the thing to fix.

## task-mgmt-specific override

The `task-mgmt` agent's `Agent`-tool chain-dispatch is **background-only**. The `[foreground-required]` sentinel does not apply to task-mgmt; task-mgmt must not emit it.

- Use case: drift audits, dependency walks, periodic reconciliations whose result lands in the task graph as a backlog commit or status flip.
- Synchronous, result-blocking dispatches → main-session.
- Peer-reviewer dispatches NOT covered by this override — they originate from whoever originated the implementer dispatch (typically main-session).

## Anti-patterns

- "It's just a short dispatch, I'll wait." Short turns long without warning.
- Foreground as a substitute for thinking about ordering.
- Treating Esc as the discipline. Every interrupt costs the user attention.
- Assuming this rule applies only to main-session. Any agent delegating to a sub-agent is a dispatcher for the duration of that call.

## Cross-references

- `../on-commit/peer-review-required.md` — peer-reviewer dispatches not covered by the task-mgmt override.
- `../on-session-start/session-start.md` — session-start task-mgmt dispatch is one of the few legitimate foreground calls.
- `../on-implement/test-etiquette.md` — protects user attention via launch budgets; this one via dispatcher-surface occupancy.
