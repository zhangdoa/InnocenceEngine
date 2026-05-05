# Discipline: workspace-hygiene

## Rules

- No scratch files in the repo root or any tracked directory. Transient output (build logs, captures, experiments) → `Build/` only (gitignored).
- Scripts → `Scripts/` (tracked). Never in `Build/`.
- `Data/Generated/` is derived runtime output. Do not track files there. Do not modify `.gitignore` to permit it. Enforced by `.claude/hooks/gates/data-generated.js`.
- If a fresh clone needs an asset → source it from a tracked location (`Data/Engine/`, `Source/External/`) or a setup-script download. Never by un-ignoring derived output.
- Do not touch `Source/External/` (git submodules) or `Source/Engine/ThirdParty/` (vendored) from AI-authored CLs.

## New documentation files

Do not create new `*.md`, `README`, design doc, roadmap, spec, or architecture note under any tracked directory without explicit user request. The only AI-authored `*.md` files allowed:

- `.claude/agents/`, `.claude/disciplines/` — agent manifests and disciplines.
- `.backlog/tasks/` — backlog tasks.
- Subtree `CLAUDE.md` — the venue for **all** subtree-level conventions: ownership, scope, policies, build-tool quirks, naming rules, inventory tables. Anything a future agent entering the subtree needs to know.
- `.alignments/` — paper-alignment audits.

Cross-session context goes in backlog task `## Implementation Notes`, not a standalone doc.

### No subtree README.md

`README.md` files inside tracked subtrees are **not allowed**. CLAUDE.md auto-loads when an agent enters the subtree; README.md does not. In a single-user-Claude project, the only readers are Claude and the user, and neither benefits from the split. Splitting "ownership" into CLAUDE.md and "policy" into README.md leaves the policy invisible to auto-load.

A task brief that asks for a `Scripts/README.md` (or any other subtree README) is wrong — push back and route the content into `Scripts/CLAUDE.md` instead. The repo-root `README.md` is exempt (it's the GitHub landing page); subtree READMEs are not.

If you find an existing subtree README.md, the cleanup is mechanical: absorb the content into the sibling CLAUDE.md, delete the README, update any in-script cross-refs.

## Cross-references

- `always/persistence-venue.md` — venue routing table.
- `always/backlog-workflow.md` — backlog is the single AI-authored project-state medium.
