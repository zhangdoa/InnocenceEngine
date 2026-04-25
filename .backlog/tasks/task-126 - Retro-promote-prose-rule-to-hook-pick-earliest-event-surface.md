---
id: TASK-126
title: >-
  Retro: when promoting a prose rule to a hook, pick the earliest event surface
  that can still observe the violation
status: Done
assignee: []
created_date: '2026-04-25 11:10'
updated_date: '2026-04-25 11:10'
labels:
  - harness
  - retrospective
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
Structural retrospective for the SessionStart producer-brief gate (commit following d39a2a0c).

**1. What implicit contract was violated.** `CLAUDE.md` (root § "Session start") requires the producer subagent to be the first action of every new session. Commit d39a2a0c attached enforcement to PreToolUse, but PreToolUse fires *after* the model has already chosen what to do this turn. A purely-textual reply or a Read-only investigation never reaches PreToolUse, so main-session Claude could drift into "What should we work on?" without ever invoking the producer. The user exercised exactly that failure mode.

**2. What structural weakness allowed it.** When promoting a prose rule from CLAUDE.md to a hook, the design step considered only one Claude Code event surface (PreToolUse — the surface most other harness rules already use) and stopped at the first one that "could work." The rule is fundamentally about *prompt routing*, not *tool intent*; PreToolUse is the wrong layer because the violation it is meant to catch happens before any tool call is selected.

**3. The improvement / discipline.** When promoting a prose rule from `CLAUDE.md` to a hook: enumerate every Claude Code event the rule could attach to (SessionStart, UserPromptSubmit, PreToolUse, PostToolUse, Stop, SubagentStop, Notification, CwdChanged, FileChanged) and pick the earliest one in the turn lifecycle that can still observe the violation. Do not stop at the first surface that "could work." Belt-and-suspenders is fine — the existing PreToolUse `producer-brief` gate stays as backup — but the *primary* enforcement should be at the earliest event that catches the failure mode the rule exists to prevent.
<!-- SECTION:DESCRIPTION:END -->

## Definition of Done
<!-- DOD:BEGIN -->
- [x] Lesson captured in a tracked file (this task) so it survives session boundaries
- [x] Discipline candidate identified — see Implementation Notes
<!-- DOD:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
2026-04-25: Filed as a retrospective. The lesson is small enough that adding a dedicated discipline file (`.claude/disciplines/hook-event-surface-selection.md`) would be over-engineering for a single observation. If a second instance of the same drift surfaces (a future rule promoted to the wrong event surface), promote this Implementation Note to a discipline fragment then.

Concrete artifact this retro produced: `.claude/hooks/session-start.js` + `.claude/hooks/gates/session-start-brief.js`, wired via `SessionStart` matcher in `.claude/settings.json`. The PreToolUse `producer-brief` gate is unchanged and stays as belt-and-suspenders.
<!-- SECTION:NOTES:END -->

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
PreToolUse was the wrong primary enforcement surface for "the first action this session must be the producer subagent" because the rule governs the model's turn-routing decision, not its tool-intent decision — and PreToolUse fires only on the latter. SessionStart fires before the model's first turn and lets the harness inject `hookSpecificOutput.additionalContext` so the directive reaches the model before it picks an action. Discipline: when promoting a prose rule to a hook, enumerate event surfaces and pick the earliest one that still observes the violation; settling for the first surface that "could work" leaves a turn-shaped hole the rule cannot enforce.
<!-- SECTION:FINAL_SUMMARY:END -->
