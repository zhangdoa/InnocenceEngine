# Discipline: paper-port

For any algorithm implemented from a published source:

1. Locate the canonical reference implementation before writing code. The paper gives shape; the reference gives parameters, unwritten invariants, and the specific decisions.
2. Read the reference's counterpart to what you're about to write — the actual function, not the README.
3. When paper prose and your mental model diverge from the reference, the reference wins. If you find yourself pattern-matching a paper phrase to a technique you recognise from elsewhere, verify against the reference before proceeding. That recognition is the drift vector.
4. Before closing a task labelled as a paper-port, invoke the paper-auditor subagent. Read its artifact. Resolve or explicitly acknowledge every divergence in task notes before the closing CL.

`.claude/references.json` maps files to papers and reference implementations. Consult it when touching a listed file. When introducing a new algorithm, add an entry.
