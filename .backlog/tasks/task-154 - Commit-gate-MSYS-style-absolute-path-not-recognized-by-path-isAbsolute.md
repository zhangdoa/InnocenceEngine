---
id: TASK-154
title: 'commit-gate.js: MSYS-style absolute paths (/c/...) fall through path.isAbsolute'
status: Done
assignee:
  - ai-expert
created_date: '2026-04-27 01:00'
updated_date: '2026-04-27 02:30'
labels:
  - infrastructure
  - harness
  - bug
dependencies: []
references:
  - .claude/hooks/commit-gate.js
  - .claude/hooks/lib/common.js
  - .claude/hooks/tests/commit-gate.test.js
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
- [x] #1 `commit-gate.js collectCommitMessageText` recognizes MSYS-style absolute paths OR fails loudly when the path argument is unreadable
- [x] #2 Repro test (synthetic commit fixture) covers the MSYS-path case and is added to the existing commit-gate.js test suite if one exists
- [x] #3 The producer's TASK-66 closure repro (`git commit -F /c/GitRepo/InnocenceEngine/Build/commit-message.txt`) is documented as the validation case
<!-- AC:END -->

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
## Root cause — narrower than the spec hypothesis

The spec attributed the bug to `path.isAbsolute` returning `false` for `/c/...` on Windows Node. Verified false: `path.win32.isAbsolute('/c/...')` returns `true`. The gate's `path.isAbsolute(p) ? p : path.join(cwd, p)` therefore hands the literal MSYS string straight to `fs.readFileSync`, which Node Win32 resolves as drive-relative — `'/c/GitRepo/...'` becomes `C:\c\GitRepo\...`, ENOENT. The previous `try/catch` swallowed the failure and the gate fell through to "empty message", letting attribution silently slip.

So the fix is two-layered, exactly as the spec asked:

1. **MSYS translation in `collectCommitMessageText`.** `/<letter>/...` → `<letter>:/...`, attempted as a second candidate after the native form fails. The user's mental model ("/c/... is absolute") is preserved.
2. **Loud-failure backstop.** When every candidate fails, the function returns `{ text, fileError }` with `fileError` populated; the dispatcher emits a structured stderr block and `process.exit(2)` (block). No silent fall-through to empty `messageText` — per `feedback_silent_failures.md`.

## Files

- `.claude/hooks/lib/common.js` — `collectCommitMessageText` rewritten. Returns `{ text, fileError }`. Tries native + MSYS-translated forms. Documents the Windows / Git Bash interaction that motivates the translation.
- `.claude/hooks/commit-gate.js` — destructures the new return shape. Adds `blockUnreadableMessageFile(fileError)` helper that prints the attempted paths and a remediation hint, then `exit(2)`.
- `.claude/hooks/tests/commit-gate.test.js` — new zero-dep test runner. 15 assertions across 4 groups: happy paths (native abs, relative, no -F), MSYS translation (the TASK-66 repro), loud failure (unreadable in any form), quoted -F. All pass.

## Validation

1. **Unit suite** — `node .claude/hooks/tests/commit-gate.test.js` reports `15 passed, 0 failed.` Includes the producer's TASK-66 repro form (rendered against a temp file).
2. **End-to-end happy path** — synthesized `git commit -F /c/GitRepo/InnocenceEngine/Build/<temp>.txt` with an attribution-bearing message body. Hook exits 0 (only the documented phase-2 transcript fail-open writes to stderr).
3. **End-to-end loud-failure** — synthesized `git commit -F /q/no/such/path.txt`. Hook exits 2; stderr lists both attempted resolutions (`/q/no/such/path.txt (ENOENT)` and `q:/no/such/path.txt (ENOENT)`) plus the remediation hint.
4. **Regression** — `git commit -m "..."` with attribution → exit 0; without attribution → exit 2 (attribution gate still fires); `git commit -F C:/GitRepo/...` native absolute → exit 0. No prior behaviour broken.

## Why the bug existed at all

`collectCommitMessageText` was the gate's only guard with a bare `catch {}` — every other I/O failure in the dispatcher already either fails loud (gate blocks) or fails open with a stderr-loud reason. The empty `catch` was load-bearing-by-accident: it short-circuited the only path that wanted to differentiate "no -F flag at all" from "-F flag pointed at unreadable file". The new shape keeps that distinction explicit (`fileError === null` vs object) and exposes it to the caller.

## Prior art cited

- TASK-146 / TASK-151's mirror-semantic clangd index purge (`Scripts/PurgeStaleClangdIndex.ps1`) is the closest pattern: silently translate the resolvable cases (orphan `.idx` paths whose URI no longer exists), fail loud on the rest. Same shape applied here at the path-resolution layer.
- `feedback_silent_failures.md` — the bare `catch {}` was the exact anti-pattern this feedback exists to prevent.
- `feedback_no_dismissing_tool_noise.md` — the gate must shout when it cannot do its job, not whisper. The blocked-with-instructions stderr block matches that posture.
<!-- SECTION:NOTES:END -->
