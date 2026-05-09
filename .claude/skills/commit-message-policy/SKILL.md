---
name: commit-message-policy
description: Use when drafting a commit message. Defines required footers (Reviewed-By, attribution, Closure-Reason) and the commit-gate-enforced format.
---

# Skill: commit-message-policy

Every commit carries two commit-gate-enforced footer lines: peer-review and attribution. No escape sentinels — bypass is path-derived (extend `DOCS_ONLY_PATH` in `.claude/hooks/lib/common.js`).

## Standard format

```
Subject: clear, imperative, under 72 chars

Brief description.
- Specific changes
- Imperative mood

Reviewed-By: <reviewer-agent>            # or Review-Skipped: <reason>
Code-AI-Generated-By: <model>            # or Message-AI-Generated-By: <model>
```

## Footer fields

| Field | Variants | Discipline | Gate |
|---|---|---|---|
| Peer-review | `Reviewed-By:` / `Review-Skipped:` | `peer-review-required` | `gates/peer-review.js` |
| Attribution | `Code-AI-Generated-By:` / `Message-AI-Generated-By:` / `Code-Human-Written:` / `Message-Human-Written:` | this file | `gates/attribution.js` |
| Closure-reason | `Closure-Reason:` | `backlog-workflow` | `gates/test-run.js` |

Multiple `Reviewed-By:` lines valid. Skip categories live in `peer-review-required`. Gate enforces *presence*, not truthfulness.

## Attribution shapes

- **AI code or AI message** → `Code-AI-Generated-By:` and/or `Message-AI-Generated-By:`.
- **Human commit** → no attribution line.
- **Mixed** → use both forms (e.g. `Message-Human-Written:` + `Code-AI-Generated-By:`).

## Scratch file

Drafts go in `Build/commit-message.txt` (gitignored). Commit with `git commit -F Build/commit-message.txt`.

Write the file with the `Write` tool, not a bash heredoc. A heredoc chained to a `git commit` in the same compound command (`cat > Build/commit-message.txt <<'EOF' ... EOF && git commit -F Build/commit-message.txt`) silently leaves the file with stale content when the commit hits a gate block — the next retry then commits with the previous CL's message. The Write tool persists reliably across retries.

## Ordering invariants

Re-validate against the synthetic-commit reproduction in `commit-gate.js` before changing:

- Both gates run in the **transcript-independent phase**. Missing transcript fails open for transcript-dependent gates only — these still run.
- Attribution is the **last** gate in that phase (after `file-size`, `paper-port`, `peer-review`).

## No retroactive amend

Historical commits without attribution are **not** fixed by `git commit --amend` or by rewriting public history (standing rule). Discovered missing-attribution → log in backlog as known historical gap.

## Anti-patterns

- Reaching for a string sentinel — none exist.
- `git commit --amend` on a commit that landed without attribution.
- Skip reasons that don't match `peer-review-required` § "Skip categories".

## Cross-references

- `peer-review-required` — owns `Reviewed-By:` / `Review-Skipped:` semantics.
- `backlog-workflow` — `TASK-NN` references parsed by closure-staleness.
