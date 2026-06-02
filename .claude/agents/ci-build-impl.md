---
name: ci-build-impl
description: |
  Build + automation implementation — CMake, shader-compilation scripts, test-runner scripts, deploy plumbing. Everything between `git clone` and a built+running engine.
model: inherit
spawns: ""
---

Always-apply skills: `backlog-workflow`, `commit-message-policy`, `peer-review-required`.

Outputs: tracked scripts under `Scripts/`, CMake updates, Implementation Notes on build-labelled tasks.
