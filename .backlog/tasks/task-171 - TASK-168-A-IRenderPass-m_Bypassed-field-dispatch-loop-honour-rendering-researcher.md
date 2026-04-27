---
id: TASK-171
title: 'TASK-168-A: IRenderPass m_Bypassed field + dispatch-loop honour (rendering-researcher)'
status: To Do
assignee: []
created_date: '2026-04-26'
labels:
  - rendering
  - tooling
  - diagnostic
dependencies: []
parent_task_id: TASK-168
priority: high
references:
  - Source/Engine/Interface/IRenderPass.h
  - Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Subtask A of TASK-168 (runtime render-pass bypass toggle).** Owner: `rendering-researcher`.

Engine-side foundation. Lands the per-pass bypass field and the dispatch-loop check, so subsequent editor work (TASK-172) has a real surface to bind against.

### What this subtask delivers

1. **`std::atomic<bool> m_Bypassed { false }` (or equivalent runtime-mutable field)** on `IRenderPass` (`Source/Engine/Interface/IRenderPass.h`). Atomic because the editor IPC writes from a non-render thread; the dispatch loop reads from the render thread.

2. **Dispatch-loop honour** in `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (`PrepareCommandList` call sites at lines 303–388). Bypass-when-true must:
   - Skip the `PrepareCommandList()` call (zero GPU work), AND
   - Leave the pass's owned resources in a defined state (last-frame content is acceptable; downstream consumers must not read undefined memory). If a pass clears its RTV at the top of `PrepareCommandList`, that clear is also skipped — document the contract.

3. **Per-pass bypass-state log** on toggle change: `LogService::Log(LogLevel::Verbose, "RenderPass: <PassName> bypass = ON|OFF")`. Edge-triggered, not per-frame, so the log isn't spammy. The pass-name string lookup is whatever the pass already exposes via `GetRenderPassComp()->m_InstanceName` (or equivalent).

4. **Pass-listing entry point** for the editor: a single function that returns the list of `IRenderPass*` currently dispatched in `ExampleRenderingClient::ExecuteCommandList`, so TASK-172 can iterate without touching every pass header. Minimum-viable shape is fine — a `std::vector<IRenderPass*> GetDispatchedPasses()` on `ExampleRenderingClient` that the editor IPC calls. **Do NOT build a full pass registry** — that is deferred per the parent task spec.

### Project invariants (anchor — read before implementing)

- **60-FPS bar (rendering-researcher manifest, 2026-04-27)**: rendering changes must keep Sponza at ≥60 FPS. The bypass field reads-when-false must cost zero — an `if (m_Bypassed) return;` at the top of `PrepareCommandList` (or a check before the dispatch-site call) is fine. Do not add per-pass overhead beyond the single atomic load.
- **Threading**: the editor IPC writes the flag from a worker thread; render thread reads it. `std::atomic<bool>` with default memory order is the minimum correct primitive. Do NOT use a mutex.
- **Owner-mode**: this subtask owns `Source/Engine/Interface/IRenderPass.h` and `Source/ExampleProject/RenderingClient/ExampleRenderingClient.cpp` (rendering-researcher subtree per `Source/ExampleProject/RenderingClient/CLAUDE.md`). Do not edit editor or IPC code — that's TASK-172.

### What this subtask does NOT do

- No editor inspector UI (TASK-172).
- No IPC GET/UPDATE wiring (TASK-172).
- No pass registry refactor.
- No measurement of bypass perf delta (TASK-169 consumes this).

### Validation

- Engine builds clean.
- Smoke test: launch GISponza, confirm baseline render unchanged (m_Bypassed defaults false everywhere).
- Manual flag flip via debugger or temporary code: set `PointShadowGeometryProcessPass::Get().m_Bypassed = true`, confirm the pass is skipped (RenderDoc capture or a temporary log line confirms `PrepareCommandList` not entered).

### Peer review

Per `peer-review-required.md`: after rendering-researcher implements, dispatch a second agent (graphics-api-expert is the natural reviewer — they own the GPU command-recording side and will catch resource-state hazards) to peer-review before commit.
<!-- SECTION:DESCRIPTION:END -->

## Acceptance Criteria
<!-- AC:BEGIN -->
- [ ] #1 `IRenderPass::m_Bypassed` atomic bool field; default false; zero-cost read when false
- [ ] #2 `ExampleRenderingClient::ExecuteCommandList` honours the flag — bypassed passes skip `PrepareCommandList`
- [ ] #3 Edge-triggered log line on toggle change ("RenderPass: <Name> bypass = ON|OFF")
- [ ] #4 Pass-listing entry point exposed for editor consumption (e.g. `GetDispatchedPasses()`)
- [ ] #5 No regression: GISponza baseline render visually unchanged with all m_Bypassed=false
- [ ] #6 No FPS regression on Sponza (≥60 FPS bar from rendering-researcher manifest)
- [ ] #7 Peer review by graphics-api-expert before commit
<!-- AC:END -->
