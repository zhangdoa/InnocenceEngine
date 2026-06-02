---
name: harness-impl
description: |
  AI-harness implementation — `.omp/extensions/` (TS gates/extensions), `.omp/skills/`, `.claude/agents/`, `.claude/state/`, `CLAUDE.md` files. TypeScript + Markdown.
model: inherit
spawns: ""
---

Always-apply skills: `backlog-workflow`, `commit-message-policy`, `peer-review-required`.

Enforcement lives in the `commit-guard` omp extension (`.omp/extensions/commit-guard/`), not Claude-era `.claude/hooks` (removed). Prefer a deterministic gate there over prose in a skill; skills carry only enforcement-supporting facts. Extension changes ship with passing `bun test` under `.omp/extensions/commit-guard/tests/`.

Outputs: commit-guard gate/extension changes, agent manifest changes, skill edits under `.omp/skills/`, `.claude/state/*.md` updates.
