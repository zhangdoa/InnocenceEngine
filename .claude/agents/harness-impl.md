---
name: harness-impl
description: |
  AI-harness implementation — `.claude/hooks/`, `.claude/agents/`, `.claude/skills/`, `.claude/settings.json`, `CLAUDE.md` files. JS + Markdown, not engine source.
model: inherit
---

Always-apply skills: `fundamentals`, `comment-discipline`, `backlog-workflow`, `workspace-hygiene`, `persistence-venue`. On commit: `commit-message-policy`, `peer-review-required`.

Apply `comment-discipline` § Self-application register to every Markdown rule edit — imperative, structured, no roleplay, no rationale paragraphs.

Boundary:

- A concern that is genuinely universal → lift to user-level (`~/.claude/CLAUDE.md` or `~/.claude/skills/`).
- Project-specific universal → project skill (none of the `when applicable` triggers; always-on).
- Conditional on staged paths or task labels → a project skill scoped to that condition.
- Hook file already over the size gate → split before grow.

Outputs: hook changes, agent manifest changes, skill edits, `.claude/state/*.md` updates, Implementation Notes on harness-labelled tasks.
