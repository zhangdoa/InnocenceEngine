---
name: commit-message-policy
description: Use when drafting a commit message in this project. commit-guard footer schema + body cap.
---

# Skill: commit-message-policy

## Body cap

commit-guard caps the body (between subject and trailer block) at 40 lines. Trailers
(`Key: Value` tail lines) are excluded from the count.

## Subject

≤72 visible characters. Imperative mood. `type(scope): summary` shape.

## Footers (commit-guard enforced)

| Field | Gate |
|---|---|
| `Reviewed-By: <stage>` or `Review-Skipped: <reason>` | peer-review |
| `Reviewed-Visually:` / `Review-Skipped-Visual:` (CLs citing `Build/captures/`) | visual-review |
| `Code-AI-Generated-By:` / `Message-AI-Generated-By:` | attribution |
| `Closure-Reason: <value>` (closure-CL test exemption) | test-run |
| `Co-Authored-By: …` | (informational) |

Gates enforce presence, not truthfulness. All gates live in `.omp/extensions/commit-guard/`
and fire when `git commit` runs through omp's `bash` tool.

## Scratch file

Drafts go in `Build/commit-message.txt` (gitignored). Commit with
`git commit -F Build/commit-message.txt`. Write it with the `write` tool, **not** a bash
heredoc — a heredoc chained to `git commit` leaves stale content when a gate blocks, so the
retry commits the previous message.

## No retroactive amend

Historical commits without attribution are NOT fixed by `git commit --amend` or by
rewriting public history. Log a known historical gap in the tracker instead.
