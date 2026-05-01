# Discipline: surface-dont-chase

When implementation surfaces work beyond the originally-scoped task — adjacent fixes, follow-up issues, "while we're here" cleanups — the agent or dispatcher must **stop and choose**: *discard* the discovery (not worth tracking) or *log it as a separate backlog task* (worth tracking). Continuing to chase the discovery within the current CL is a third option that is wrong by default. The originally-scoped task closes (or hands back) when its acceptance criteria are met or honestly marked unmet; discoveries become their own work.

The rule is about **bounded execution per CL** and **clean phase boundaries**, not about avoiding follow-up work.

## Three workflow points

1. **Implementer mid-CL.** When a fix surfaces a related-but-out-of-scope issue, surface it back to the dispatcher in the closure summary and stop chasing. Do not silently fold it in. The surfaced finding lists what was observed and the file:line, not a fix.
2. **Dispatcher receiving a surfaced finding.** Do not reflexively expand the next dispatch to address it. First decide discard-or-log against the original task's acceptance criteria. If logged, it is a new task; the current task closes or hands back on its original ACs.
3. **Main-session reviewing a user directive.** When executing a user directive surfaces unexpected scope, surface back to the user. Do not auto-expand the directive. The user sets direction; auto-expansion is the dispatcher acting as direction-setter.

## What this is not

- **Not a ban on follow-up work.** Worthy discoveries get filed. The discipline governs *when* — after the current CL closes, not folded into it.
- **Not a ban on filing tasks.** Filing is the correct outcome for worthy discoveries. The complementary rule is `feedback_dont_pile_on_backlog_tasks` (auto-memory): don't pile noise. The two compose: only worthy discoveries get filed; worthy discoveries get filed *separately*, not chased.
- **Not the same as scope creep within an in-flight task's ACs.** Genuine in-scope ACs append to the current task per `backlog-workflow.md` § anti-patterns. The judgement is whether the discovery serves the *original* ACs or is its own work.

## Recorded incident

TASK-77.1.3 closure shape (this session): implementer surfaced a D9 root cause; dispatcher dispatched a fix; the fix surfaced three more algorithmic gaps; dispatcher kept escalating instead of closing-and-logging. Each escalation step was the wrong choice — the original AC had been met or honestly unmet at every transition, and the discoveries belonged in their own tasks.

## Cross-references

- `backlog-workflow.md` — file-task-first applies to the discovered work too; the new task is filed before the next dispatch.
- `task-decomposition.md` — when a discovery is large enough to be its own multi-slice work.
- `peer-review-required.md` — reviewer's BLOCKED findings are *in-scope feedback on the current diff*, not surfaced new scope; the two are different signals.
- `persistence-venue.md` — governs harness-authoring register (paraphrase, do not transcribe).
