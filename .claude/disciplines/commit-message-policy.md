# Discipline: commit-message-policy

All commits follow a standard format with two commit-gate-enforced footer lines: peer-review (`Reviewed-By:` / `Review-Skipped:`) and attribution (`Code-AI-Generated-By:` / `Message-AI-Generated-By:`). The footer lines are the discipline — the artifact in `git log` is what makes the rules auditable after the fact.

## How

### Standard format

```
Subject line: Clear, imperative, under 72 characters

Brief description explaining what and why the changes were made.

- Bullet point list of specific changes
- Each change should be clear and actionable
- Use imperative mood consistently

[Peer-review footer]
[Attribution footer]
```

### Footer fields

| Field | Purpose | Discipline | Gate |
|-------|---------|------------|------|
| `Reviewed-By: <reviewer-agent>` *or* `Review-Skipped: <reason>` | Peer-review of the diff (or explicit skip per `peer-review-required.md` § "When") | `peer-review-required.md` | `gates/peer-review.js` |
| `Code-AI-Generated-By: <model>` *or* `Message-AI-Generated-By: <model>` | Authorship of code and/or commit message | this file | `gates/attribution.js` |

Multiple `Reviewed-By:` lines are valid (peer + architect on a cross-cutting CL). Skip categories live in `peer-review-required.md` § "When" — the gate enforces *presence* of a reason, not the truthfulness of one. Neither field has an escape sentinel; the line itself is the audit trail.

### Attribution shapes

**AI-authored code and/or commit message:**

```
Code-AI-Generated-By: [AI Agent Model Name]
```

**Human commits:** No attribution line needed.

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

### AI Agent scratch file

AI agents draft messages in `Build/commit-message.txt` (gitignored) and commit with `git commit -F Build/commit-message.txt`.

### Example

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

### Harness enforcement

`.claude/hooks/commit-gate.js` runs as a `PreToolUse` hook on Bash and blocks `git commit` unless the commit message contains the required footer fields. It reads messages from either `-m ...` or `-F path`.

#### Ordering invariants

These are load-bearing — do not change without re-validating against the synthetic-commit reproduction in `commit-gate.js` + the historical `55cf6a72` artefact:

- The attribution and peer-review gates have **no** escape sentinel. No commit-gate has one anymore — every gate's bypass is path-derived (e.g. test-run auto-skips on docs-only paths via DOCS_ONLY_PATH). If you need an exemption, the right fix is to extend the path regex in `.claude/hooks/lib/common.js`, not to reach for a string token.
- Both gates run in the **transcript-independent phase** of the dispatcher. A missing or unreadable transcript fails the dispatcher open for transcript-dependent gates only — attribution and peer-review still run. Any refactor that re-couples either to the transcript-fetch envelope reintroduces the `55cf6a72` bypass.
- Attribution is the **last** gate in the transcript-independent phase (after `file-size`, `paper-port`, and `peer-review`). Other failures in that phase surface first because they require more work to fix than appending an attribution line. Peer-review runs before attribution because it is the richer claim — if both lines are missing the user gets the more informative error first.

#### Retroactive amend prohibition

Historical commits that landed without attribution are **not** to be fixed by `git commit --amend` or by rewriting history. The standing rule is: rewrites of public history are off-limits unless the user explicitly requests them. A discovered missing-attribution artefact is logged in the backlog as a known historical gap and left in place; the gate fix prevents recurrence.

## Anti-patterns

- **Reaching for a string sentinel.** None exist. Every gate's bypass is path-derived. If a gate fires on a path it shouldn't, fix the regex.
- **`git commit --amend` on a commit that landed without attribution.** Standing rule: no rewriting public history. Log the gap in backlog; don't rewrite.
- **Skip reasons that don't match `peer-review-required.md` § When.** The gate enforces *presence*, but a `Review-Skipped: didn't-feel-like-it` line is a discipline violation even if the gate accepts it.

## Cross-references

- `peer-review-required.md` — owns the `Reviewed-By:` / `Review-Skipped:` semantics; this discipline owns the artifact format and the gate.
- `backlog-workflow.md` — `TASK-NN` references in commit messages are what the closure-staleness gate parses; commit messages are part of the audit trail this discipline produces.
- `fundamentals.md` — historical context (what changed, why) belongs in the commit message, not in source comments. The footer lines are the audit form.
