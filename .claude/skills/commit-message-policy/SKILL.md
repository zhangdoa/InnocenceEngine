---
name: commit-message-policy
description: Use when drafting a commit message in this project. Gate-enforced footer schema + body cap.
---

# Skill: commit-message-policy

## Body cap

`gates/commit-body-cap.js` — body between subject and trailer block capped at 40 lines. Trailers (`Key: Value`) excluded from the count.

## Subject

≤72 visible characters. Imperative mood. `type(scope): summary` shape.

## Footers (gate-enforced)

| Field | Gate |
|---|---|
| `Reviewed-By: <stage>` or `Review-Skipped: <reason>` | `peer-review.js` |
| `Reviewed-Visually:` / `Review-Skipped-Visual:` (rendering CLs) | `visual-review.js` |
| `Code-AI-Generated-By:` / `Message-AI-Generated-By:` (or `…-Human-Written:`) | `attribution.js` |
| `Closure-Reason: <value>` (closure CL exemption) | `test-run.js` |
| `Co-Authored-By: …` (per-CL attribution) | (informational) |

Gates enforce presence, not truthfulness.

## Scratch file

Drafts go in `Build/commit-message.txt` (gitignored). Commit with `git commit -F Build/commit-message.txt`. Use `Write` tool, not heredoc.

## No retroactive amend

Historical commits without attribution are not fixed by `git commit --amend` or by rewriting public history.
