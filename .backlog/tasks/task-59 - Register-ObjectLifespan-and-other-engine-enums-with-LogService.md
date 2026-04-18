---
id: TASK-59
title: Register ObjectLifespan (and other engine enums) with LogService
status: Done
assignee: []
created_date: '2026-04-18 09:00'
updated_date: '2026-04-18 17:49'
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

## Final Summary

<!-- SECTION:FINAL_SUMMARY:BEGIN -->
Added `INNO_REGISTER_EXTERNAL_ENUM` macro (complements the existing `INNO_ENUM`) so enums declared in the ordinary `Inno::` namespace — like `ObjectStatus` and `ObjectLifespan` that pre-date the enum registry — can be made loggable without relocating them. Registered both engine-wide enums in `Object.h`; stripped four `static_cast<int>(lifespan|status)` call-site hacks (AssetService, GPUBufferResourceServiceImpl, EntityRegistry ×2). Log output now reads `Lifespan=ObjectLifespan::Scene` instead of `Lifespan=2`.
<!-- SECTION:FINAL_SUMMARY:END -->
