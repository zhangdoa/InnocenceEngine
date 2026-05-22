---
name: harness-impl
description: |
  AI-harness implementation — `.claude/hooks/`, `.claude/agents/`, `.claude/skills/`, `.claude/settings.json`, `CLAUDE.md` files. JS + Markdown.
model: inherit
---

Always-apply skills: `backlog-workflow`, `commit-message-policy`, `peer-review-required`.

Prefer gates over prose. Skills carry only gate-error-message-supporting facts; rules that don't fire 100% are either gates or deleted.

Outputs: hook changes, agent manifest changes, skill edits, `.claude/state/*.md` updates.
