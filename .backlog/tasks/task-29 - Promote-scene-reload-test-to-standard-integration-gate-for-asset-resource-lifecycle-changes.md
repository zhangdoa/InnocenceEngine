---
id: TASK-29
title: >-
  Promote scene reload test to standard integration gate for
  asset/resource/lifecycle changes
status: To Do
assignee: []
created_date: '2026-04-13 18:12'
labels:
  - testing
  - process
  - architecture
dependencies: []
references:
  - CLAUDE.md
  - Scripts/InteractiveTest.ps1
priority: medium
---

## Description

<!-- SECTION:DESCRIPTION:BEGIN -->
The texture accumulation bug (aef0f866) required multiple scene loads to manifest. The basic 10-frame integration test (single GISponza load) does not exercise this. The 20-frame reload test is currently classified as "heavy" and only used for scene lifecycle changes — but any asset loading, resource management, or deferred init change is equally at risk.

**Structural weakness:** The test tier classification underestimates reload-path risk. Single-pass tests can green-light changes that have severe multi-load bugs. Any persistent state (shared asset handles, pooled resources, component storage) is an accumulation hazard that only manifests at reload time.

**Target improvement:**
- Update CLAUDE.md testing policy: the scene reload test should be the minimum bar for any change touching asset loading, scene lifecycle, GPU resource management, or deferred initialization — not just "heavy" service refactors
- Consider making the reload test the default integration CI gate (replacing or supplementing the 10-frame test) for the relevant change categories
- Audit the current tier 2/3 boundary and make it more precise so "reload-safe" is a first-class concern, not an afterthought
<!-- SECTION:DESCRIPTION:END -->
