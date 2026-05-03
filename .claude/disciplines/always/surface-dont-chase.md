# Discipline: surface-dont-chase

When implementation surfaces work beyond the originally-scoped task — adjacent fixes, follow-up issues, "while we're here" cleanups — **stop and choose**: discard (not worth tracking) or log as a separate backlog task. Continuing to chase the discovery within the current CL is wrong by default.

The originally-scoped task closes (or hands back) when its acceptance criteria are met or honestly marked unmet. Discoveries become their own work.

## Three workflow points

1. **Implementer mid-CL.** Surface the finding back to the dispatcher in the closure summary and stop chasing. Do not silently fold it in. The surfaced finding lists what was observed and the file:line, not a fix.
2. **Dispatcher receiving a surfaced finding.** Decide discard-or-log against the original task's ACs. If logged → new task. The current task closes or hands back on its original ACs.
3. **Main-session reviewing a user directive.** When executing a user directive surfaces unexpected scope, surface back to the user. Auto-expansion is the dispatcher acting as direction-setter.

## Not the same as

- A ban on follow-up work — worthy discoveries get filed (after the current CL closes).
- A ban on filing tasks — filing is the right outcome for worthy discoveries.
- Scope creep within an in-flight task's ACs — genuine in-scope work appends to the current task per `always/backlog-workflow.md` § anti-patterns.

## Cross-references

- `always/backlog-workflow.md` — file-task-first applies to discovered work too.
- `on-commit/peer-review-required.md` — reviewer's BLOCKED findings are *in-scope feedback on the current diff*, not surfaced new scope.
- `always/persistence-venue.md` — harness-authoring register: paraphrase, do not transcribe.
