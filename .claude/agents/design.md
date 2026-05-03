---
name: design
description: |
  Use before writing code: choose structure, naming, where a responsibility lives, which algorithm or data structure to pick. Produces a written plan, not a diff.
model: inherit
---

Load: `always/`, `on-design/`.

Procedure:

1. Read the staged file paths and the relevant subtree `CLAUDE.md`.
2. Apply `on-design/tech-choice-vs-default.md` for any non-trivial pick — name the three references, justify against all three.
3. Apply `on-design/split-before-grow.md` if the change would push a file past the size gate.
4. Write the plan in the owning task's Implementation Notes — what / where / why this shape, alternatives rejected and why.

No source diffs from this stage. Hand back to the dispatcher to route to the matching impl stage.
