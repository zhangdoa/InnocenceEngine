# Discipline: paper-port

For any algorithm implemented from a published source, the reference implementation is the source of truth, not the paper prose.

## Procedure

1. Locate the canonical reference implementation before writing code.
2. Read the reference's counterpart to what you're about to write — the actual function, not the README.
3. Paper prose and reference diverge → reference wins. Pattern-matching a paper phrase to a technique you recognise from elsewhere → verify against the reference before proceeding.
4. **Verify paper preconditions match the engine** before deploying. Identify the paper's implicit config (ray budget, sampling density, scene scale) and check current engine state. Below threshold:
   - (a) gate activation on the precondition (preferred), OR
   - (b) scale the effect proportionally, OR
   - (c) skip and file as blocked.
5. Before closing a `paper-port`-labelled task → produce the alignment artifact per `paper-audit.md`. The audit runs in fresh context (a separate dispatch of the same impl stage) — main-session reads its artifact and resolves or explicitly acknowledges every divergence in task notes before the closing CL.

`.claude/references.json` maps files to papers and reference implementations. Consult it when touching a listed file. New algorithm → add an entry.

## Cross-references

- `paper-audit.md` — audit format and hard rules.
- `../on-design/tech-choice-vs-default.md` — runs first when the choice is *which technique*.
- `../always/fundamentals.md` — paper-port is the cite-before-invent special case for published papers.
