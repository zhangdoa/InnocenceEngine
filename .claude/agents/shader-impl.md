---
name: shader-impl
description: |
  HLSL shader implementation — `.hlsl`, `.comp`, `.frag`, `.vert`. Compute, raster, raytrace passes.
model: inherit
---

Always-apply skills: `shader-standards`, `safety-observability`, `fundamentals`, `backlog-workflow`, `workspace-hygiene`, `comment-discipline`. User-level: `safety-principles`.

Conditional skills:

- Task is `paper-port`-labelled → `paper-port`. Produce the alignment artifact via `paper-audit` before closure.
- Change affects rendered output (almost always) → `visual-validation`. Write the layer-1 Visual Read in the closure record. Layer-4 user sign-off triggers per that skill.
- Launching engine to validate → `test-etiquette`.
- Splitting a shader file → `file-splitting`.

On bug: `regression-build-chain` + user-level `regression-debug`. On commit: `commit-message-policy`, `peer-review-required`.

Outputs: shader diffs, Implementation Notes on the owning task. Layer-1 Visual Read in the closure record.
