---
id: TASK-154
title: 'commit-gate.js: MSYS-style absolute paths (/c/...) fall through path.isAbsolute'
status: To Do
assignee: []
created_date: '2026-04-27 01:00'
labels:
  - infrastructure
  - harness
  - bug
dependencies: []
references:
  - .claude/hooks/commit-gate.js
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Surfaced by producer agent during TASK-66 closure, 2026-04-27.**

`.claude/hooks/commit-gate.js` `collectCommitMessageText` uses Node's `path.isAbsolute` to resolve `git commit -F <path>` arguments. On Windows Node, `path.isAbsolute("/c/GitRepo/InnocenceEngine/Build/commit-message.txt")` returns false — MSYS-style absolute paths (`/c/...` produced by Git Bash) are not recognized, so the message file is effectively unreadable. The gate then runs against an empty message; the attribution gate falls through and other gates may misbehave.

### Workaround

Use the project-relative form: `git commit -F Build/commit-message.txt`. Verified to work in the producer dispatch.

### Required fix

Detect MSYS-style absolute paths in addition to native Windows ones, e.g. by:
- Treating any path starting with `/<single-letter>/` as absolute (and translating to native form).
- Falling back to `fs.existsSync` against both the literal path and a normalized form.
- Or rejecting the commit with a clear error message if the path can't be resolved, instead of silently fallthrough.

The third option is the safest from a "no silent failure" perspective per `feedback_silent_failures.md` — if the gate can't read the message, it should fail loudly rather than allow a malformed commit through.

### Why low priority

The Bash workaround (relative path) is well-known and widely used. The current symptom is not a silent bypass per se — `[skip-test-gate]` and other escape sentinels still work; only the file-reading path is broken. But the fall-through is a sharp edge for any future agent that uses absolute paths from Git Bash.

### Owner

`ai-expert` — owns `.claude/hooks/`.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `commit-gate.js collectCommitMessageText` recognizes MSYS-style absolute paths OR fails loudly when the path argument is unreadable
- [ ] #2 Repro test (synthetic commit fixture) covers the MSYS-path case and is added to the existing commit-gate.js test suite if one exists
- [ ] #3 The producer's TASK-66 closure repro (`git commit -F /c/GitRepo/InnocenceEngine/Build/commit-message.txt`) is documented as the validation case
<!-- AC:END -->
