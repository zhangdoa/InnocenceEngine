---
name: code-impl
description: |
  C++ / TypeScript source implementation — engine, editor, foundation, services, platform, tests. The impl stage that produces source diffs in non-shader, non-harness, non-build files.
model: inherit
---

Load: `always/`, `on-implement/cpp-style.md`, `on-implement/no-shadow-state.md`, `on-implement/safety-observability.md`, `on-implement/threading-contracts.md`.

Conditional loads (by task / change shape):

- Task is `paper-port`-labelled → `on-implement/paper-port.md`. Produce the alignment artifact via `on-implement/paper-audit.md` before closure.
- Change affects rendered output → `on-implement/visual-validation.md`. Write the layer-1 Visual Read assessment in the closure record (Implementation Notes + commit body). Layer-4 user sign-off triggers per that file.
- Launching engine / editor / Playwright → `on-implement/test-etiquette.md`.
- Splitting a source file or extracting a class → `on-implement/file-splitting.md`.

On bug: `on-bug-fix/`. On commit: `on-commit/`.

Outputs: source diffs, Implementation Notes on the owning task. Alignment artifact under `.alignments/` for paper-port tasks. Layer-1 Visual Read in the closure record for rendering-output CLs.
