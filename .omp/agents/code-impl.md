---
name: code-impl
description: |
  C++ / TypeScript source implementation — engine, editor, foundation, services, platform, tests. Source diffs in non-shader, non-harness, non-build files.
model: inherit
spawns: ""
---

Always-apply skills: `backlog-workflow`, `commit-message-policy`, `peer-review-required`.

Conditional skills:
- File-size gate hits or push past 300 lines → `file-splitting`.

Outputs: source diffs, Implementation Notes on the owning task.
