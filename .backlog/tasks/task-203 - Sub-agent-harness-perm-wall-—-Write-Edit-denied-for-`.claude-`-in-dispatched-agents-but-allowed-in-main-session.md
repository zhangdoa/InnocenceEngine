---
id: TASK-203
title: >-
  Sub-agent harness perm wall — Write/Edit denied for `.claude/**` in dispatched
  agents but allowed in main-session
status: To Do
assignee:
  - ai-expert
created_date: '2026-04-29 08:19'
labels:
  - harness
  - permissions
  - blocker
dependencies: []
references:
  - .claude/settings.local.json
  - ~/.claude/settings.json
  - .claude/agents/ai-expert.md
priority: high
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
## What's hurting us

ai-expert sub-agent dispatches reliably hit `Permission to use Write has been denied` and `Permission to use Bash has been denied` (for redirect-write commands) when targeting paths under `.claude/**`. Two dispatches in this session (`adfe8c2fc7ff4c182` and `a7fc71385058829ad`) both blocked at the same wall.

**Inconsistency**: earlier this session, ai-expert dispatch (the one that landed `66327f34` closure-staleness commit-gate) successfully wrote `.claude/hooks/gates/closure-staleness.js` and edited `.claude/disciplines/backlog-workflow.md`. Same agent type, same paths. Main-session also writes `.claude/` files freely (verified `touch .claude/disciplines/.permission-test-*` succeeded this session).

So the deny is:
- Per-dispatch (not per-agent-type)
- Sub-agent-only (main-session unaffected)
- `.claude/**` (and possibly `.backlog/**`) targeted

## Permissions config snapshot (this session)

`.claude/settings.local.json` has empty `deny: []`. `allow: [...]` lists ~50 entries — all `Bash(...)` patterns, MCP tools, and a couple `Read(...)` entries. **No `Write(...)` or `Edit(...)` rules at all.** Yet main-session and earlier ai-expert dispatches wrote files successfully — implying auto-mode default behavior is relied upon.

`~/.claude/settings.json` has `defaultMode: "auto"`, `skipDangerousModePermissionPrompt: true`, `skipAutoPermissionPrompt: true`. The auto-approval + skip-prompt combination presumably handles main-session writes. **Hypothesis**: sub-agent dispatches don't inherit auto-approval the same way — they get a stricter inherited permission profile, and the absence of explicit `Write(.claude/**)` allow rules causes silent denials.

## Goal

1. Diagnose: confirm the inheritance gap. Is it documented harness behavior or a regression?
2. Fix: either (a) add explicit `Write(C:/GitRepo/InnocenceEngine/.claude/**)` and `Edit(...)` allow rules covering the paths agents legitimately need to modify, OR (b) align sub-agent permission inheritance with main-session where the user has already auto-approved.

## Acceptance criteria

- [ ] Root cause identified (inheritance model documented vs. regression)
- [ ] Sub-agent ai-expert can `Write` to `.claude/hooks/lib/*.js` and `.claude/disciplines/*.md` under the same conditions main-session can
- [ ] Add a regression-test recipe — a quick "dispatch a no-op ai-expert that tries to write a tmp file under `.claude/`; verify it succeeds" probe
- [ ] If the fix is allow-list expansion, the rules are explicitly enumerated for the paths each agent owns (per agent manifests)

## Blocks

- TASK-201 AC#4 (codify backlog-drift audit recipe in `.claude/hooks/lib/audit-backlog-drift.js`) — cannot complete until sub-agent can write to `.claude/`.
- TASK-201 AC#5 (wire drift-check into `.claude/disciplines/session-start.md`) — same reason.
- Any future ai-expert harness work that needs to land hooks / disciplines.

## Owner

`ai-expert` (harness ownership). Diagnose + fix.

## Workaround until fixed

For TASK-201 specifically: the producer's manual audit recipe (commit `6f1247e4`'s methodology — list open tasks, grep `git log --all` per task, classify) provides operational coverage. Re-run periodically by hand.

## References

- Failed dispatches: `adfe8c2fc7ff4c182`, `a7fc71385058829ad` (both this session)
- Successful dispatch (for contrast): commit `66327f34` closure-staleness gate landed by an earlier ai-expert in same session
- `.claude/settings.local.json`, `~/.claude/settings.json`
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [ ] #1 Code compiles — build output quoted in the final summary (tier of build depends on domain — engine/editor/shader)
- [ ] #2 Pre-existing integration tests covering the changed area were re-run against the change and green — spec file names and pass/fail counts quoted in the final summary
- [ ] #3 If no pre-existing integration test covers the change: a new integration test (NOT a mock-based unit test) was written and run — state why this was the only path
- [ ] #4 Self-authored mock-based tests are not the sole validation — if they are the only tests run then the summary must explicitly flag this gap
- [ ] #5 User-observable outcome verified — screenshot; RenderDoc capture; terminal transcript of a real interaction; or specific DOM/state assertion observed in a running system
- [ ] #6 Final summary lists what was NOT verified — honestly and specifically — not as a boilerplate disclaimer
<!-- DOD:END -->
