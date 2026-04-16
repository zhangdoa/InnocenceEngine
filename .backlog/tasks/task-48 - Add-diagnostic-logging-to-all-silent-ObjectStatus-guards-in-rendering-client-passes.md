---
id: TASK-48
title: >-
  Add diagnostic logging to all silent ObjectStatus guards in rendering client
  passes
status: To Do
assignee: []
created_date: '2026-04-16 19:05'
labels:
  - reliability
  - rendering
  - observability
dependencies: []
references:
  - Source/ExampleProject/RenderingClient/
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
**Problem:** Every rendering client pass has `if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated) return false;` with no logging. That's ~18 passes across Source/ExampleProject/RenderingClient/ that silently skip every frame when initialization fails.

**Fix:** Add `Log(Warning, "PassName::PrepareCommandList skipped: RenderPassComp not Activated");` to each guard. Consider extracting a shared macro or helper since the pattern is identical across all passes.
<!-- SECTION:DESCRIPTION:END -->
