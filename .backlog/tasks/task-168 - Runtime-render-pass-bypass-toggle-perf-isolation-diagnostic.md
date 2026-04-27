---
id: TASK-168
title: 'Runtime render-pass bypass toggle (perf + isolation diagnostic)'
status: To Do
assignee: []
created_date: '2026-04-27 21:00'
labels:
  - rendering
  - tooling
  - diagnostic
dependencies: []
priority: high
references:
  - Source/Engine/Interface/IRenderPass.h
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
  - Source/Editor-Next/src/components/inspector/
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**User-flagged 2026-04-27**: "we should be able to quickly bypass a render pass without doing anything offline, as a toggle, which has been a long overdue task. in that way, it's super easy to measure performance or narrow down issues."

Without this, every perf isolation or visual A/B today requires editing source, recompiling, redeploying, restarting. The cost makes engineers skip the experiment, which means real cost regressions ride longer than they should — exactly the failure mode that produced TASK-169 (Sponza ~20 FPS) and TASK-170 (sphere shadow artifacts) before the user surfaced them.

### Required behaviour

A runtime toggle, per render pass, that:

1. **Skips dispatch when disabled** — the pass's `PrepareCommandList` either doesn't run, or runs but emits no GPU work. The pass's *resources* must still be allocated (consumers downstream may sample the texture; bypass should leave the texture in a defined state — last-frame's content or a cleared default).
2. **Is editable at runtime** — not compile-time. Editor inspector with a checkbox per pass is the lightest UX; an in-engine console / overlay would also satisfy. Editor-Next is the existing path of least resistance.
3. **Persists per-session, not per-scene** — so a perf measurement against the same scene survives reload.
4. **Surfaces the bypass state in the log** so it's auditable across runs.

### Design space

- **Per-pass `m_Bypassed : bool`** on the IRenderPass interface, read each frame in the dispatch loop. Editor inspector exposes a list of passes with checkboxes, IPC GET/UPDATE per the existing TASK-101 contract.
- **Pass registry** that the dispatcher walks, instead of the current hard-coded `ExampleRenderingClient::ExecuteCommandList` sequence. Bigger refactor; deferred unless naturally surfaced.
- **Console-only** — minimal viable: a key binding cycles through passes and toggles. No editor UI. Lower bar but harder to remember which pass is which.

The minimum-viable cut: per-pass `m_Bypassed` field + editor checkbox. If pass-listing in the editor is non-trivial without a pass registry, ship the field + a small list-of-passes harness, defer the registry.

### Out-of-scope for this task

- Performance measurement itself (TASK-169).
- Visualisation modes for pass output (texture viewer, etc.).
- Full pass-registry refactor.

### Owner

Likely a coordinated `rendering-researcher` (pass interface) + `editor-tooling-expert` (inspector + IPC). Producer to decompose.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 IRenderPass exposes a runtime-mutable `m_Bypassed : bool` (or equivalent); reading it during dispatch costs zero when false
- [ ] #2 Engine dispatch loop honours the flag — bypassed passes emit no GPU work; resources remain in defined state
- [ ] #3 Editor inspector exposes the toggle for at least the major passes (Sun shadow RT, point shadow geometry process, GI passes, light pass, post-FX); IPC GET/UPDATE round-trips per TASK-101 contract
- [ ] #4 Toggle change is logged ("Pass X bypass = ON/OFF") for audit trail
- [ ] #5 Bypassing PointShadowGeometryProcessPass + SunShadowRTPass and re-running GISponza shows the perf delta (validation that the toggle works as a diagnostic)
- [ ] #6 No regression in the non-bypassed path — engine + smoke clean
<!-- AC:END -->
