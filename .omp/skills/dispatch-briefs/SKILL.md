---
name: dispatch-briefs
description: Main-session-only. Use when dispatching a sub-agent via the task tool. Reviewer-brief discipline + sub-agents-cannot-self-review.
---

# Skill: dispatch-briefs (dispatcher-only)

## Background by default

Dispatch via the `task` tool, background by default (see the user-level `agent-dispatch`
skill). omp's `task` tool has no `run_in_background` flag and no `[foreground-required]`
sentinel — the Claude-era agent-dispatch gate is gone; background-default is convention now.

## Sub-agents cannot dispatch sub-agents

Impl agents (`code-impl`, `shader-impl`, `ci-build-impl`, `harness-impl`) are defined with
`spawns: ""`, so they cannot spawn a peer-review on their own diff. Review is always a fresh
dispatch from the main session.

Brief shape:
- Implementer validates (build green, smoke green, capture A/B), then STOPS at the commit
  step OR commits + reports.
- Main session dispatches a fresh `code-review`, fills the commit-message review footer with
  the verdict, commits if needed.

## task-mgmt

`task-mgmt` (`spawns: "*"`) is the dispatcher that chain-dispatches impl stages, background
only.

## Before authoring a new skill

Search existing skills for the same mechanism — extend, don't fork. Prefer a commit-guard
gate (`.omp/extensions/commit-guard/`) over a skill when the rule is deterministically
enforceable. New project skills go in `.omp/skills/<name>/SKILL.md`.
