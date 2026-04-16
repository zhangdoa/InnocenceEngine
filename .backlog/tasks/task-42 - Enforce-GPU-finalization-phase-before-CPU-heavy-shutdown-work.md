---
id: TASK-42
title: Enforce GPU finalization phase before CPU-heavy shutdown work
status: To Do
assignee: []
created_date: '2026-04-16 16:00'
labels:
  - structural
  - lifecycle
dependencies: []
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**What implicit contract was violated?** GPU must be alive when readback happens, but Terminate runs CPU path tracer (~14s) before readback, exceeding TDR timeout.

**What structural weakness allowed it?** No explicit ordering between GPU result finalization and CPU-heavy shutdown work. The readback was placed after the path tracer by coincidence, not design.

**What improvement?** Add a FinalizeGPUResults phase to the shutdown sequence that runs before any long-running CPU work. This makes the invariant (GPU alive during readback) structural rather than depending on code placement within Terminate.

Immediate fix was applied in commit de88a521 — moved readback to last Update frame. This task tracks making the ordering invariant explicit.
<!-- SECTION:DESCRIPTION:END -->
