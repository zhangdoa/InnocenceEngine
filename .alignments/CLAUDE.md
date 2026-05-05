Multi-stage scratchpad for durable design / audit / post-mortem artifacts that outlive a single CL but do not belong in `.backlog/tasks/` (which tracks task status) or in source comments (which document present state at point of decision). Stages write artifacts within their own task scope; directory shape, naming, and lifecycle are defined here.

## Stage write rights

| Artifact shape | Producing stage(s) |
|---|---|
| `TASK-<id>-<topic>-design.md` | `design` (or `code-impl` / `shader-impl` when the design step folds into the implementer's turn) |
| `TASK-<id>-<topic>-diagnostic.md` | `code-impl`, `shader-impl`, `bug-fix` |
| `TASK-<id>-<topic>-paper-port-audit.md` | `code-impl` or `shader-impl` on `paper-port`-labelled tasks |
| `post-TASK-<id>-<topic>.md` | any stage |
| `_<prefix>/` | any stage; gitignored via `.gitignore` rule `/.alignments/_*/` |

## Naming convention

`<id>` matches the backlog task id (e.g. `77.1`, `6.10`). `<topic>` is a short kebab-case slug.

## Lifecycle

- **`TASK-*` and `post-TASK-*` `.md` artifacts** persist as historical record. Referenced by commit messages, future stage dispatches, and the citation gate via `.claude/references.json`. Do not delete after the consuming CL lands.
- **`_*/` auxiliary snapshots** are deletable after the consuming CL lands. They are local-only by design (gitignored) — third-party reference source and capture dumps belong with the stage that needed them, not in repo history.

## What does NOT belong here

- **Task status, ACs, Implementation Notes** — use `.backlog/tasks/`.
- **In-code rationale for a decision** — use a comment at the point of decision per `.claude/disciplines/always/fundamentals.md`.
- **Commit-only ephemera** — use the commit message body.
- **Tracked third-party source** — vendored code goes in `Source/Engine/ThirdParty/` or `Source/External/`; reference-only snapshots stay under `_*/` and remain local.
