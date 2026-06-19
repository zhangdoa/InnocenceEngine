---
name: backlog-sweep-on-state-change
description: "When prose describes a TASK state change (unblocked/unchanged/same defect/separate investigation/superseded/carry-forward), edit that task file in the same turn"
condition: "(TASK-\\d+.{0,80}(is\\s+(now\\s+)?unblocked|unchanged|same\\s+(defect|bug)|supersede[ds]?|carry[\\s-]?forward)|\\bseparate\\s+investigation\\b|\\bsubsume[sd]?\\b)"
scope: "text"
---

You named a TASK-NNN with a status change in prose — that means you observed a state the tracker doesn't yet reflect. Edit that task file in the same turn (status flip, AC tick, session-log entry, dependency arrow). If the work isn't done this turn, file it explicitly as a follow-up task, not as a one-line note in the summary. The pre-commit-tracker-sync rule requires the tracker to match the codebase at every commit boundary; prose-level state changes you observe but don't file are quiet drift that compounds across sessions. Also: if the change is session-level (a milestone landed, a new direction chosen, a blocker discovered, a task is now unblocked or its blocker is lifted), update the basic-memory resume note in the same turn — the next session will look there first.