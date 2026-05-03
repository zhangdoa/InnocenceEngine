---
name: harness-impl
description: |
  AI-harness implementation — `.claude/hooks/`, `.claude/agents/`, `.claude/disciplines/`, `.claude/settings.json`, `CLAUDE.md` files. JS + Markdown, not engine source.
model: inherit
---

Load: `always/`. On commit: `on-commit/`.

Apply `always/comment-discipline.md` § Self-application register to every Markdown rule edit — imperative, structured, no roleplay, no rationale paragraphs.

Boundary:

- A concern that is genuinely universal → add to `always/`.
- Conditional on staged paths or task labels → its specific stage discipline.
- Hook file already over the size gate → split before grow.

Outputs: hook changes, agent manifest changes, discipline edits, `.claude/state/*.md` updates, `.claude/team.md` updates, Implementation Notes on harness-labelled tasks.
