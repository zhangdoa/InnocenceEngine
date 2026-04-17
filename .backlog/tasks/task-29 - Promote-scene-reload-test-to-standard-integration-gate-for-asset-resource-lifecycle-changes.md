---
id: TASK-29
title: >-
  Promote scene reload test to standard integration gate for
  asset/resource/lifecycle changes
status: Done
assignee: []
created_date: '2026-04-13 18:12'
updated_date: '2026-04-17 04:46'
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

## Implementation Notes

<!-- SECTION:NOTES:BEGIN -->
**2026-04-17 (completion):** Updated `CLAUDE.md` tier 3 (scene reload) guidance so it applies to **any** change touching asset loading, scene lifecycle, GPU resource management, deferred initialization, or any state that persists across scene boundaries (shared asset handles, component pools, descriptor heaps). Explicitly called out the two concrete examples of multi-load-only failures we've seen — texture name accumulation (aef0f866) and the TASK-52 stale GPU VA pipeline — so the broader class is on the reader's mind the next time they're scoping tests.

Not in scope: promoting the reload test to the default integration CI gate. The 10-frame single-load test is cheaper; asking every code change to run the reload tier is too heavy for a trivial edit. The updated wording ensures the relevant *categories* of change always take tier 3.
<!-- SECTION:NOTES:END -->
