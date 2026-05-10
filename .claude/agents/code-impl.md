---
name: code-impl
description: |
  C++ / TypeScript source implementation — engine, editor, foundation, services, platform, tests. The impl stage that produces source diffs in non-shader, non-harness, non-build files.
model: inherit
---

Always-apply skills: `cpp-style`, `safety-observability`, `fundamentals`, `backlog-workflow`, `workspace-hygiene`, `comment-discipline`. User-level: `safety-principles`, `no-shadow-state`, `threading-contracts`.

Conditional skills (by task / change shape):

- Task is `paper-port`-labelled → `paper-port`. Produce the alignment artifact via `paper-audit` before closure.
- Change affects rendered output → `visual-validation`. Write the layer-1 Visual Read assessment in the closure record (Implementation Notes + commit body). Layer-4 user sign-off triggers per that skill.
- Launching engine / editor / Playwright → `test-etiquette`.
- Splitting a source file or extracting a class → `file-splitting`.

On bug: `regression-build-chain` + user-level `regression-debug`. On commit: `commit-message-policy`, `peer-review-required`.

Outputs: source diffs, Implementation Notes on the owning task. Alignment artifact under `.alignments/` for paper-port tasks. Layer-1 Visual Read in the closure record for rendering-output CLs.
