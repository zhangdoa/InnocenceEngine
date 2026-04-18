---
id: TASK-42
title: Enforce GPU finalization phase before CPU-heavy shutdown work
status: Done
assignee: []
created_date: '2026-04-16 16:00'
updated_date: '2026-04-18 17:45'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added `IRenderingClient::FinalizeGPUResults()` as an explicit phase called by `Engine::Terminate` AFTER `WaitForGPUIdle` and BEFORE any LogicClient CPU-heavy work. ExampleRenderingClient overrides it to run the auto-capture readback as a structural fallback when the per-frame trigger in ExecuteCommands didn't fire (e.g. user exited before the trigger frame). Readback extracted into `TryWriteAutoCapture` to share between the two call sites. The "GPU alive during readback" invariant is now structural: the phase is named, doc'd in the interface, and invoked at the single correct point in shutdown — no more relying on line placement inside Terminate.
<!-- SECTION:FINAL_SUMMARY:END -->
