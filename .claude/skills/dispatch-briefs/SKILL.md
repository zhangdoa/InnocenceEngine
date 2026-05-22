---
name: dispatch-briefs
description: Main-session-only. Use when dispatching a sub-agent via the Agent tool. Defines the agent-dispatch gate + the sub-agents-cannot-self-dispatch-review constraint.
---

# Skill: dispatch-briefs (dispatcher-only)

## Agent-dispatch gate

`gates/agent-dispatch.js` (wired through `session-gate.js`) blocks any `Agent` call whose `tool_input.run_in_background` is anything other than `true`, unless the `prompt` field contains the literal sentinel `[foreground-required]`.

Foreground is opted into in writing at the dispatch site. No env-var bypass, no commit-message sentinel.

## Sub-agents cannot dispatch sub-agents

Implementer agents do not have access to the `Agent` tool from within their own dispatch. They cannot spin up a peer-review on their own diff.

Brief shape:
- Implementer validates (build green, smoke green, capture A/B) but STOPS at the commit step OR commits + reports.
- Main-session receives the report, dispatches a fresh code-review, fills the commit-message footer with the verdict, commits if needed.

## task-mgmt override

`task-mgmt` agent's `Agent`-tool chain-dispatch is background-only. The `[foreground-required]` sentinel does not apply to task-mgmt.

## Search before authorising new skills

Before creating a new `.claude/skills/<name>/SKILL.md`, grep existing skills for the same mechanism. Extend, don't fork. (Better yet: prefer a gate over a new skill if the rule is enforceable.)
