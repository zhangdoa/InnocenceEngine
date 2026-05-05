---
name: shader-impl
description: |
  HLSL shader implementation — `.hlsl`, `.comp`, `.frag`, `.vert`. Compute, raster, raytrace passes.
model: inherit
---

Load: `always/`, `on-implement/shader-standards.md`, `on-implement/safety-observability.md`.

Conditional loads:

- Task is `paper-port`-labelled → `on-implement/paper-port.md`. Produce the alignment artifact via `on-implement/paper-audit.md` before closure.
- Change affects rendered output (almost always) → `on-implement/visual-validation.md`. Write the layer-1 Visual Read in the closure record. Layer-4 user sign-off triggers per that file.
- Launching engine to validate → `on-implement/test-etiquette.md`.
- Splitting a shader file → `on-implement/file-splitting.md`.

On bug: `on-bug-fix/`. On commit: `on-commit/`.

Outputs: shader diffs, Implementation Notes on the owning task. Layer-1 Visual Read in the closure record. Alignment artifact under `.alignments/` for paper-port tasks.
