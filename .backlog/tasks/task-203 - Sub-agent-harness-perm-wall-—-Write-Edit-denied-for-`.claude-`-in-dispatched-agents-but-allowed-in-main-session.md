---
id: TASK-203
title: >-
  Sub-agent harness perm wall — Write/Edit denied for `.claude/**` in dispatched
  agents but allowed in main-session
status: Done
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

- [x] Root cause identified (inheritance model documented vs. regression)
- [x] Sub-agent ai-expert can `Write` to `.claude/hooks/lib/*.js` and `.claude/disciplines/*.md` under the same conditions main-session can
- [x] Add a regression-test recipe — a quick "dispatch a no-op ai-expert that tries to write a tmp file under `.claude/`; verify it succeeds" probe
- [x] If the fix is allow-list expansion, the rules are explicitly enumerated for the paths each agent owns (per agent manifests)

## Final Summary

**Root cause** (not a regression, accidental side-effect). Claude Code's `defaultMode: "auto"` + `skipAutoPermissionPrompt` flags from `~/.claude/settings.json` (user-global) do not propagate into dispatched sub-agent permission scopes. Sub-agents run against the project's `.claude/settings.json` allow-list, where Write/Edit rules were absent. Main-session inherited the user-global auto-mode and bypassed the wall silently. Earlier in-session sub-agent successes (e.g., commit `66327f34`) were per-dispatch user approval prompts that the user accepted, not a stable allow.

**Fix.** Two-part:

1. Discarded `.claude/settings.local.json` entirely (gitignored, ~50 entries of cruft including hyper-specific MSBuild invocations, a stale user path `C:/Users/zhang/...`, and unused MCP entries). Per user direction, "use the committed one instead".
2. Added a `permissions.allow` block to the committed `.claude/settings.json` with per-agent-owned-subtree Write/Edit rules and a curated Bash/Read/MCP set. Paths use `${CLAUDE_PROJECT_DIR}/...` (Claude Code's project-dir variable) instead of absolute paths so the committed config doesn't leak personal directory layout to the public repo.

**Per-agent scoping** (option 1b from the user's call):
- `Write/Edit(${CLAUDE_PROJECT_DIR}/.alignments/**)` — paper-port
- `Write/Edit(${CLAUDE_PROJECT_DIR}/.backlog/**)` — producer
- `Write/Edit(${CLAUDE_PROJECT_DIR}/.claude/**)` — ai-expert
- `Write/Edit(${CLAUDE_PROJECT_DIR}/Build/**)` — implementers (commit-message drafts, build artefacts)
- `Write/Edit(${CLAUDE_PROJECT_DIR}/Documents/**)` — docs (rare, gated by `feedback_no_ai_authored_docs.md`)
- `Write/Edit(${CLAUDE_PROJECT_DIR}/Scripts/**)` — ci-build-expert
- `Write/Edit(${CLAUDE_PROJECT_DIR}/Source/**)` — engine implementers

**Probes (verify-then-commit per the user's recommendation 3):**

| Probe | Path form | Result |
|---|---|---|
| v1 | `Write(//c/GitRepo/InnocenceEngine/.claude/**)` (MSYS) | FAIL — Write tool receives `C:\...` paths; no match |
| v2 | `Write(C:/GitRepo/InnocenceEngine/.claude/**)` (absolute, fwd-slash) | PASS — Write+Edit succeeded; but absolute path leaks personal layout |
| v3 | `Write(${CLAUDE_PROJECT_DIR}/.claude/**)` (env-var, repo-relative) | PASS — Write+Edit succeeded; portable, no leak. Final form. |

**What was NOT verified.**
- Sub-agent path-deletion via `Bash rm` and `PowerShell Remove-Item` was denied even with `Bash(rm:*)` in the allow list. Main-session cleanup worked. Suspected: Claude Code applies a per-target-path check on deletion that the wildcard `Bash(rm:*)` doesn't satisfy, or `rm` invocation form mismatches the wildcard. Not a TASK-203 blocker — sub-agent deletes weren't a stated AC. Worth a probe in a future session if any agent legitimately needs to delete files under its own subtree.
- Other agent dispatches (rendering-researcher Write under `Source/Shaders/HLSL/**`, low-level-expert Write under `Source/Engine/Common/**`) were not probed individually. The rule shape `Write(${CLAUDE_PROJECT_DIR}/Source/**)` should cover both, but the path-syntax surprise from probe v1 means uniformity is not guaranteed across rule paths until exercised.

**Unblocks.**
- TASK-201 AC#4 (codify `.claude/hooks/lib/audit-backlog-drift.js`) — sub-agent ai-expert can now Write under `.claude/**`.
- TASK-201 AC#5 (wire drift-check into `.claude/disciplines/session-start.md`) — same.

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
