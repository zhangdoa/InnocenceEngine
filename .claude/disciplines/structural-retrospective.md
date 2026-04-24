# Discipline: structural-retrospective

After every non-trivial CL, a brief retrospective — three questions, 2–3 sentences each:

1. **What implicit contract was violated** that the bug, defect, or friction exposed?
2. **What structural weakness allowed it** — was a responsibility split wrongly, was a boundary missing, was an invariant implicit instead of enforced?
3. **What improvement moves the codebase toward its target qualities** (orthogonality, explicit contracts, fail loudly, reload-safe)?

Each structural finding is filed as its own backlog task in the same turn. Structural observations must never live only in conversation or in a single commit message — they evaporate at turn boundaries and at session boundaries.

The retrospective is short. The act of writing it is what catches drift; the artefact is cheap.
