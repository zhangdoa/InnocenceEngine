# Discipline: commit-message-policy

## Standard Format

All commits follow this structure:

```
Subject line: Clear, imperative, under 72 characters

Brief description explaining what and why the changes were made.

- Bullet point list of specific changes
- Each change should be clear and actionable  
- Use imperative mood consistently

[Peer-Review Line - see below]
[Attribution Line - see below]
```

### Footer fields

Two footer fields are commit-gate-enforced. Both are audit-trail lines —
the artifact in `git log` is the discipline:

| Field | Purpose | Discipline | Gate |
|-------|---------|------------|------|
| `Reviewed-By: <reviewer-agent>` *or* `Review-Skipped: <reason>` | Peer-review of the diff (or explicit skip per "When required") | `peer-review-required.md` | `gates/peer-review.js` |
| `Code-AI-Generated-By: <model>` *or* `Message-AI-Generated-By: <model>` | Authorship of code and/or commit message | this file | `gates/attribution.js` |

Multiple `Reviewed-By:` lines are valid (peer + architect on a
cross-cutting CL). Skip categories live in `peer-review-required.md`
§ "When required" — the gate enforces *presence* of a reason, not the
truthfulness of one. Neither field has an escape sentinel; the line
itself is the audit trail.

## Attribution Requirements

### 🤖 AI-Generated Commits

**Mandatory attribution for AI-authored code and/or commit message:**

```
Code-AI-Generated-By: [AI Agent Model Name]
```

### 👤 Human Commits  

**No attribution line needed.**

### 🤝 Hybrid Scenarios

**AI Code + Human Message:**

```
Message-Human-Written: [Human Name]
Code-AI-Generated-By: [AI Agent Model Name]
```

**Human Code + AI Message:**

```
Message-AI-Generated-By: [AI Agent Model Name]
Code-Human-Written: [Human Name]
```

## AI Agent Scratch File

AI agents draft messages in `Build/commit-message.txt` (gitignored) and commit
with `git commit -F Build/commit-message.txt`.

## Harness enforcement

`.claude/hooks/commit-gate.js` runs as a `PreToolUse` hook on Bash and blocks
`git commit` unless the commit message contains the required footer fields
(see "Footer fields" above). It reads messages from either `-m ...` or
`-F path`. See `CLAUDE.md` → Harness enforcement for the test-run gate that
runs alongside.

### Ordering invariants

These are load-bearing — do not change without re-validating against the
synthetic-commit reproduction in `commit-gate.js` + the historical
`55cf6a72` artefact:

- The attribution and peer-review gates have **no** escape sentinel.
  `[skip-test-gate]` and `[skip-size-gate]` are scoped to their respective
  gates only; neither bypasses attribution or peer-review.
- Both gates run in the **transcript-independent phase** of the
  dispatcher. A missing or unreadable transcript fails the dispatcher
  open for transcript-dependent gates only — attribution and peer-review
  still run. Any refactor that re-couples either to the transcript-fetch
  envelope reintroduces the `55cf6a72` bypass.
- Attribution is the **last** gate in the transcript-independent phase
  (after `file-size`, `paper-port`, and `peer-review`). Other failures in
  that phase surface first because they require more work to fix than
  appending an attribution line. Peer-review runs before attribution
  because it is the richer claim — if both lines are missing the user
  gets the more informative error first.

### Retroactive amend prohibition

Historical commits that landed without attribution are **not** to be
fixed by `git commit --amend` or by rewriting history. The standing rule
is: rewrites of public history are off-limits unless the user explicitly
requests them. A discovered missing-attribution artefact is logged in
the backlog as a known historical gap and left in place; the gate fix
prevents recurrence.

## Example

```
Fix ObjectPool memory leak with proper destructor

Resolves critical memory leak where ObjectPool heap was never deallocated,
causing accumulating memory usage in long-running applications.

- Add destructor with proper heap deallocation
- Add null pointer safety check  
- Use engine memory management system
- Add debug logging for allocation tracking

Reviewed-By: ai-expert
Code-AI-Generated-By: Claude Sonnet 4.6
```

## Enforcement

- **AI commits require attribution** - missing attribution blocks commit
- **Attribution scope** - indicates who authored code and/or message
- **Git history transparency** - clear distinction between human and AI work
