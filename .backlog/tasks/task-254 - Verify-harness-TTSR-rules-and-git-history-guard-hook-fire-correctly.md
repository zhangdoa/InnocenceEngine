---
id: TASK-254
title: >-
  Verify harness TTSR rules + git-history-guard hook fire correctly (live)
status: To Do
assignee:
  - harness-impl
created_date: '2026-06-19'
labels:
  - harness
priority: medium
---

## Description

The harness rework (commits `087c8af6`, `b26bcb07`, `e979d181`, `43309cec`
+ global `~/.omp/agent` changes) was authored from a Notebooks-rooted
session, so the project TTSR rules never loaded there and could not be
fire-tested. One global rule that *was* loaded — `no-retroactive-amend`
(`scope: "tool:bash"`) — misfired at turn start on prose that quoted its
triggers, and was replaced by the `git-history-guard` hook. The remaining
project TTSR rules share the same scope shapes and need a live check from
an InnocenceEngine-rooted session.

## Acceptance Criteria

- [ ] `engine-log-levels` fires when an edit to a `.cpp`/`.h` adds
      `Log(Success, …)` or `Log(Info, …)`; does NOT fire on `Log(Error, …)`.
- [ ] `pre-commit-tracker-sync` fires on `git add` / `git commit`; does
      NOT fire on `git status` / `git diff` / `git log`.
- [ ] `renderdoc-capture-is-in-engine` fires on `renderdoccmd capture`;
      does NOT fire on `renderdoccmd convert` (the inspect-skill path).
- [ ] `no-speculative-debug-loop` fires on trial-and-error phrasing
      ("another Log", "let me try a different…"); does NOT fire on a bare
      "maybe" / "let me check".
- [ ] `git-history-guard` hook blocks an actual `git commit --amend`,
      `git rebase`, and `git push --force`; passes a normal commit; the
      `# history-rewrite-ok` escape lets an amend through.
- [ ] For any TTSR that misfires on prose (as `no-retroactive-amend` did),
      migrate that command-gate into a `commit-guard` tool_call check
      instead — a hook only sees the real command and cannot misfire.

## Notes

- Obsolete after the peer-review abolition: TASK-166 / TASK-167
  (peer-review gate work) are moot — close or repurpose.
