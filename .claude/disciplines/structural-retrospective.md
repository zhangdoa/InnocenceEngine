# Discipline: structural-retrospective

After every non-trivial CL, run a brief retrospective and file each structural finding as its own backlog task in the same turn. Structural observations must never live only in conversation or in a single commit message — they evaporate at turn boundaries and at session boundaries.

## How

Three questions, 2-3 sentences each:

1. **What implicit contract was violated** that the bug, defect, or friction exposed?
2. **What structural weakness allowed it** — was a responsibility split wrongly, was a boundary missing, was an invariant implicit instead of enforced?
3. **What improvement moves the codebase toward its target qualities** (orthogonality, explicit contracts, fail loudly, reload-safe)?

The retrospective is short. The act of writing it is what catches drift; the artefact is cheap. Each finding becomes a backlog task — the task carries it across session boundaries; conversation memory does not.

## Cross-references

- `target-qualities.md` — the four qualities the third question measures against.
- `owner-mode.md` — owner-mode produces structural observations during the work; this discipline is where they get formalised at CL close.
- `backlog-workflow.md` — findings file as tasks; that discipline governs what goes in them.
