---
name: peer-review-required
description: Use after every non-trivial implementation dispatch. Defines verdict tiers and the commit-message footer schema enforced by gates/peer-review.js and gates/visual-review.js.
---

# Skill: peer-review-required

Every non-trivial implementation dispatch ends with a peer-review dispatch (fresh agent, never main-session, never the implementer) before commit.

## Skip categories (`Review-Skipped: <reason>`)

- `backlog-only` — `.backlog/`, `.md`, `.claude/`, `.gitignore` only
- `harness-internal` — `.claude/hooks/**` self-edits
- `mechanical-rename` — single deterministic transformation (rename, file move)

When unsure → required.

## Verdict tiers

| Tier | Meaning |
|---|---|
| PASS | Meets brief + ships |
| ADVISORY | AC met, non-blocking notes |
| BLOCKED | At least one finding with file:line — re-dispatch |
| UNVERIFIED | AC technically met but not visually/runtime confirmed |

## Commit-message footers (gate-enforced)

```
Reviewed-By: <reviewer-stage>            # one or more
Review-Skipped: <reason>                 # alternative to Reviewed-By

# Rendering CLs (when body references Build/captures/):
Reviewed-Visually: <reviewer-stage> — <improvement|regression|uncertain|per-scene-mixed>
Review-Skipped-Visual: <reason>          # alternative
```

Gates enforce presence, not truthfulness: `gates/peer-review.js`, `gates/visual-review.js`.
