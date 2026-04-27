# Discipline: task-decomposition

Applies when breaking umbrella tasks into agent-scoped subtasks, or filing new work surfaced during a session.

- Each subtask names its owning agent explicitly — by frontmatter label or by a line in the Description pointing at the agent. Unlabelled tasks drift.
- A task whose Description spans multiple agents' scopes is a symptom of missing decomposition, not an acceptable end-state. Split before closing.
- Include reproduction / entry-point information in the Description so the next session can pick up without needing conversation context.
- If the work is paper-driven, add the `paper-port` label at creation time so the alignment-audit requirement at closure isn't forgotten.

## Carry-forward corrections between subtasks

When a downstream subtask discovers an error or oversight in an upstream subtask's deliverable (e.g. a design call's VRAM math used the wrong format constant; a foundation's slot allocator mis-handled a field with overloaded semantics), the correction lives in the downstream task's Implementation Notes AND in the upstream task as an appended addendum. Without the upstream addendum, future readers seeing the upstream task's Final Summary will inherit the original error.

Precedent: TASK-150 design call cited `Float32 = 4B/pixel` for an `RGBA × Float32` atlas (actual: 16B/pixel, R32G32B32A32_FLOAT); TASK-147 caught it during VRAM-fit validation, corrected to `RG × Float32` (8B/pixel, R32G32_FLOAT) and recorded the correction in TASK-147's Implementation Notes. TASK-148 then implemented against the corrected format. Pattern is healthy when each downstream subtask has the discipline to verify its inputs against backend-format constants rather than trusting upstream prose.

## Single-agent dispatch as a decomposition probe

When a multi-agent task is dispatched to a single agent and that agent correctly returns "this spans my scope plus N others", treat the bounce as a useful signal — the dispatch surfaced the ownership boundary that the producer missed at filing time. Re-decompose into agent-scoped subtasks before re-dispatching; do not collapse back to a single-agent attempt.

Precedent: TASK-66 (point/sphere shadows) was originally dispatched as a single task; the assigned agent surfaced graphics-api + rendering-researcher + software-architect + editor-tooling-expert scope and returned. Producer decomposed into TASK-147 / TASK-148 / TASK-149 / TASK-150 with explicit dispatch sequencing. The chain landed cleanly with no scope-thrash.
