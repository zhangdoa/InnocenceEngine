---
name: trim-source-comments-at-write
description: "Source comments: cap 5 contiguous // lines, no TASK-N references — trim in the edit payload, not at commit time"
condition: ["(?://[^\\\\n]{0,500}\\\\n){6,}", "\\\\bTASK-\\\\d+\\\\b"]
scope: ["tool:edit(*.cpp)", "tool:edit(*.h)", "tool:edit(*.hpp)", "tool:edit(*.inl)", "tool:write(*.cpp)", "tool:write(*.h)", "tool:write(*.hpp)", "tool:write(*.inl)"]
---

Source comments are terse WHY notes, not essays. Hard cap: 5 contiguous `//` lines in a single edit. Tracker IDs (TASK-N, TASK-243) belong in commits and task files, not source comments. Trim at write time — the commit-guard catching this is a LATESTAGE check; fix the comment in the edit, not when the commit is rejected.