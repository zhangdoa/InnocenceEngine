Owned by the `software-architect` agent (directory structure, naming conventions, lifecycle of artifacts). See `.claude/agents/software-architect.md` and `.claude/team.md`.

This subtree is a multi-agent shared scratchpad for durable design / audit / post-mortem artifacts that outlive a single CL but do not belong in `.backlog/tasks/` (which tracks task status) or in source comments (which document present state at point of decision). Specialist agents write artifacts within their own task scope; the `software-architect` owns directory shape, naming, and lifecycle.

## Specialist write rights

Any agent may write artifacts under its own task scope. Specifically:

- `paper-auditor` writes `TASK-<id>-<topic>-paper-port-audit.md` and may stage reference snapshots locally under `_audit_refs/` (gitignored).
- `rendering-researcher` writes `TASK-<id>-<topic>-design.md` and `TASK-<id>-<topic>-diagnostic.md`.
- Any agent may write `post-TASK-<id>-<topic>.md` post-mortem artifacts.

## Naming convention

| Filename shape | Purpose |
|----------------|---------|
| `TASK-<id>-<topic>-design.md` | Design artifact for an in-flight task. |
| `TASK-<id>-<topic>-diagnostic.md` | Mid-task diagnostic write-up. |
| `TASK-<id>-<topic>-paper-port-audit.md` | Paper-port alignment audit. |
| `post-TASK-<id>-<topic>.md` | Post-mortem produced after closure. |
| `_<prefix>/` | Auxiliary snapshots (third-party source, capture dumps). Gitignored via `.gitignore` rule `/.alignments/_*/`. |

`<id>` matches the backlog task id (e.g. `77.1`, `6.10`). `<topic>` is a short kebab-case slug.

## Lifecycle

- **`TASK-*` and `post-TASK-*` `.md` artifacts** persist as historical record. Referenced by commit messages, future agent dispatches, and the citation gate via `.claude/references.json`. Do not delete after the consuming CL lands.
- **`_*/` auxiliary snapshots** are deletable after the consuming CL lands. They are local-only by design (gitignored) — third-party reference source and capture dumps belong with the agent that needed them, not in repo history.

## What does NOT belong here

- **Task status, ACs, Implementation Notes** — use `.backlog/tasks/`.
- **In-code rationale for a decision** — use a comment at the point of decision per `.claude/disciplines/fundamentals.md`.
- **Commit-only ephemera** — use the commit message body.
- **Tracked third-party source** — vendored code goes in `Source/Engine/ThirdParty/` or `Source/External/`; reference-only snapshots stay under `_*/` and remain local.
