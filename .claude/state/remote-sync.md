# Remote sync state

Snapshot of the relationship between the active branch and `origin`. Update when the snapshot drifts (push, fetch surprise, branch swap).

## ecs-overhaul (active)

`ecs-overhaul` is ahead of `origin/ecs-overhaul` by local commits; `origin` has no remote contributors, so no merge or rebase is required. The user pushes periodically at their own discretion.

Operational rule:

- No proactive sync at session start. Run `git fetch` once if confirming the snapshot above; otherwise treat the branch as ahead-only.
- Push only when the user asks. Do not propose a push as part of session wrap-up.
