# Discipline: paper-port

For any algorithm implemented from a published source, the reference implementation is the source of truth, not the paper prose. The paper gives shape; the reference gives parameters, unwritten invariants, and the specific decisions.

## How

1. Locate the canonical reference implementation before writing code.
2. Read the reference's counterpart to what you're about to write — the actual function, not the README.
3. When paper prose and your mental model diverge from the reference, the reference wins. If you find yourself pattern-matching a paper phrase to a technique you recognise from elsewhere, verify against the reference before proceeding. That recognition is the drift vector.
4. **Verify paper preconditions match the engine** before deploying. Papers ship with implicit assumptions — ray budget, sample density, buffer size, scene scale. Paper-faithful implementation ≠ paper-good outcome if those assumptions don't match. Identify the paper's implicit config (ray budget, sampling density, scene scale, etc.) and check current engine state. If below threshold, either (a) gate activation on the precondition (preferred — feature dormant but future-proof), (b) scale the effect proportionally, or (c) skip and file as blocked. Recorded incident: TASK-77 [S1.4] (2026-04-23, commit `adf5b18c`) — paper §2.1.4 "untraced cells get the average of traced cells" works under dense ray budget but doubled SH DC + caused per-probe banding under sparse-spawning config (1 ray per 2×2 tile per frame).
5. Before closing a task labelled as a paper-port, invoke the paper-auditor subagent. Read its artifact. Resolve or explicitly acknowledge every divergence in task notes before the closing CL.

`.claude/references.json` maps files to papers and reference implementations. Consult it when touching a listed file. When introducing a new algorithm, add an entry.

## Cross-references

- `paper-audit.md` — the audit format and hard rules the paper-auditor subagent follows when producing the alignment artifact.
- `tech-choice-vs-default.md` — runs first when the choice is *which technique to use at all*; this discipline runs when the technique is fixed and the question is *how faithfully to port the published version*.
- `cite-prior-art.md` — paper-port is the special case of citation when the source is a published paper with a reference implementation.
