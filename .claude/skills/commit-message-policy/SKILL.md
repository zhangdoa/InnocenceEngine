---
name: commit-message-policy
description: Use when drafting a commit message in this project. Project-specific extensions to user-level `commit-message-policy`: gate-enforced footers, scratch-file path, ordering invariants in commit-gate.js.
---

# Skill: commit-message-policy (project extension)

Generic commit-message shape, attribution conventions, scratch-file pattern, and the never-amend-to-fix-metadata rule live in user-level `commit-message-policy`. This skill carries project-specific gate bindings.

## Footer fields enforced by gates

Every commit carries two commit-gate-enforced footer lines. No escape sentinels — bypass is path-derived (extend `DOCS_ONLY_PATH` in `.claude/hooks/lib/common.js`).

| Field | Variants | Discipline | Gate |
|---|---|---|---|
| Peer-review | `Reviewed-By:` / `Review-Skipped:` | `peer-review-required` | `gates/peer-review.js` |
| Attribution | `Code-AI-Generated-By:` / `Message-AI-Generated-By:` / `Code-Human-Written:` / `Message-Human-Written:` | user-level `commit-message-policy` | `gates/attribution.js` |
| Closure-reason | `Closure-Reason:` | `backlog-workflow` | `gates/test-run.js` |

Multiple `Reviewed-By:` lines valid. Skip categories live in `peer-review-required`. Gate enforces *presence*, not truthfulness.

## Scratch file

Drafts go in `Build/commit-message.txt` (gitignored). Commit with `git commit -F Build/commit-message.txt`. Use the `Write` tool, not a heredoc — see user-level skill for the silent-stale-content failure mode.

## Ordering invariants (commit-gate.js)

Re-validate against the synthetic-commit reproduction in `commit-gate.js` before changing:

- Both gates run in the **transcript-independent phase**. Missing transcript fails open for transcript-dependent gates only — these still run.
- Attribution is the **last** gate in that phase (after `file-size`, `paper-port`, `peer-review`).

## No retroactive amend

Historical commits without attribution are **not** fixed by `git commit --amend` or by rewriting public history (standing rule per user-level skill). Discovered missing-attribution → log in `.backlog/` as a known historical gap.

## Cross-references

- User-level `commit-message-policy` — generic shape, attribution conventions, scratch-file pattern.
- `peer-review-required` — owns `Reviewed-By:` / `Review-Skipped:` semantics.
- `backlog-workflow` — `TASK-NN` references parsed by closure-staleness.
