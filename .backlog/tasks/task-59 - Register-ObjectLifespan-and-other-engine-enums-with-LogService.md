---
id: TASK-59
title: Register ObjectLifespan (and other engine enums) with LogService
status: To Do
assignee: []
created_date: '2026-04-18 09:00'
labels:
  - architecture
  - logging
  - fix-at-right-layer
dependencies: []
references:
  - Source/Engine/Common/LogService.h
  - Source/Engine/Services/AssetService.cpp
  - Source/Engine/Services/EntityRegistry.cpp
priority: low
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
`Log(..., lifespan, ...)` fails to compile because `ObjectLifespan` is not in `IsRegisteredEnum`. Current call sites cast to `int` at the point of use — a high-level logging concern patched inside each caller.

Resolve by registering `ObjectLifespan` (and any other engine enums commonly logged) with `LogService`'s enum registry, so `Log(..., lifespan, ...)` just works and prints the enum name. Then strip the `static_cast<int>(lifespan)` hacks at the call sites.
<!-- SECTION:DESCRIPTION:END -->
